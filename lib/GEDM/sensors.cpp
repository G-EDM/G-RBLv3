//############################################################################################################
/*

 ______     _______ ______  _______
|  ____ ___ |______ |     \ |  |  |
|_____|     |______ |_____/ |  |  |
___  _  _ ____ __ _ ___ ____ _  _
|--' |--| |--| | \|  |  [__] |\/|

This is a beta version for testing purposes.
Only for personal use. Commercial use or redistribution without permission is prohibited. 
Copyright (c) Roland Lautensack    


*/
//############################################################################################################

#include "sensors.h"
#include "ili9341_tft.h"
#include "gpo_scope.h"
#include "soc/syscon_reg.h"
#include "soc/syscon_struct.h"
#include <soc/sens_struct.h>
#include <esp_attr.h>
#include <driver/i2s.h>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <hal/cpu_hal.h>

#define MAX_BUFFER_SIZE        1024
#define I2S_MASTER_CLOCK_SPEED 2000000

enum sensor_error_type {
    S_ERROR_NONE     = 0,
    S_ERROR_ARC      = 1,
    S_ERROR_OVERLOAD = 2,
    S_ERROR_SHORT    = 3
};

enum ism_sensor_states {
    SS_SEEK,
    SS_RAMP_UP,
    SS_ACTIVE,
    SS_CORRECTING,
    SS_ERROR
};

typedef struct external_sensors {
    bool on_off_switch_event_detected = false;
    bool limit_switch_event_detected  = false;
    bool block_on_off_switch          = false;
} external_sensors;

typedef struct feedback_misc_data {
    int32_t  plan                         = 0;
    int32_t  vfd_slope                    = 0;
    int32_t  cfd_slope                    = 0;
    uint32_t cycle_start                  = 0;
    int64_t  micros_start                 = 0;
    uint32_t discharge_count              = 0;
    uint32_t discharge_count_result       = 0;
    uint32_t discharges_in_a_row          = 0;
    uint32_t nodischarges_in_a_row        = 0;
} feedback_misc_data;

typedef struct wave_ctrl {
    bool     rise_block       = false;
    uint32_t risings_in_a_row = 0;
    uint32_t pre_rising_cfd   = 0;
    uint32_t deadlock_count   = 0;
    uint32_t cfd_peak         = 0;
    uint32_t cfd_peak_p       = 0;
    uint32_t vfd_lock         = 0;
} wave_ctrl;

typedef struct peak_track {
    bool     pre_discharge_collected = false;
    uint32_t peaks_per_pulse_max     = 0;
} peak_track;



typedef struct adc_feedback {
    // cfd = current feedback
    uint32_t cfd_recent            = 0;
    uint32_t cfd_avg_fast          = 0;
    uint32_t cfd_avg_slow          = 0;
    uint32_t cfd_recent_previous   = 0;
    uint32_t cfd_avg_fast_previous = 0;
    uint32_t cfd_avg_slow_previous = 0;
    uint32_t cfd_peak              = 0;
    uint32_t cfd_avg_fast_sum      = 0;
    uint32_t cfd_avg_slow_sum      = 0;
    uint32_t cfd_avg_fast_index    = 0;
    uint32_t cfd_avg_slow_index    = 0;
    uint32_t cfd_max_tracker       = 0;
    // vfd = voltage feedback
    uint32_t vfd_recent            = 0;
    uint32_t vfd_avg_fast          = 0;
    uint32_t vfd_avg_slow          = 0;
    uint32_t vfd_recent_previous   = 0;
    uint32_t vfd_avg_fast_previous = 0;
    uint32_t vfd_avg_slow_previous = 0;
    uint32_t vfd_peak              = 0;
    uint32_t vfd_peak_history      = 0;
    uint32_t vfd_variable_thresh   = 0;
    uint32_t vfd_pre_short_circuit = 0;
    uint32_t vfd_avg_fast_sum      = 0;
    uint32_t vfd_avg_slow_sum      = 0;
    uint32_t vfd_avg_fast_index    = 0;
    uint32_t vfd_avg_slow_index    = 0;
} adc_feedback;


std::mutex              mtx;
std::condition_variable cv;

static uint16_t* dma_buffer              = nullptr;
static size_t    dma_buffer_current_size = 0;

TaskHandle_t adc_monitor_task_handle;    // I2S readout task with a waitqueue
TaskHandle_t remote_control_task_handle; // Remote control task with a waitqueue

xQueueHandle adc_read_trigger_queue = NULL; // Queue used to read a batch of i2s samples; it is filled from within a timerinterrupt
xQueueHandle remote_control_queue   = NULL; // Queue used for some misc stuff like limit/estop readings, pwm on/off controls etc.
//############################################################
// HW Timer
//############################################################
hw_timer_t * benchmark_timer = NULL; 

static esp_adc_cal_characteristics_t adc1_chars;

static DRAM_ATTR feedback_misc_data adc_data;
static DRAM_ATTR adc_feedback       feedback;
static DRAM_ATTR wave_ctrl          wavectrl;
static DRAM_ATTR peak_track         peaktrack;
static DRAM_ATTR external_sensors   sensors; // used for the limits switch and motionswitch to hold the current state and flag onchange events
static DRAM_ATTR sensor_error_type  sensor_errors = S_ERROR_NONE;
static DRAM_ATTR ism_sensor_states  sensor_state  = SS_SEEK;



std::atomic<bool> sennsors_running( false );       // set to true after the task and queues are started (doesn't care if the tasks are already running)
std::atomic<bool> adc_monitor_task_running(false); // set to true once the adc monitor task enters the inner loop

DRAM_ATTR std::atomic<int>        motion_plan_atomic( 0 );                          // atomic int used to store/receive the motion plan
DRAM_ATTR std::atomic<bool>       new_motion_plan( false );                         // set to true if a new plan is available
DRAM_ATTR std::atomic<bool>       restart_i2s_flag( false );                        // if this flag is set to true the adc monitor task will restart i2s
DRAM_ATTR std::atomic<bool>       motion_switch_changed( false );                   // motion on/off state changed
DRAM_ATTR std::atomic<bool>       reset_sensor_state(false);
DRAM_ATTR std::atomic<bool>       sense_settings_changed(false);
DRAM_ATTR std::atomic<i2s_states> i2s_state( I2S_CTRL_IDLE ); 


//##################################################
// cFd = Current feedback settings
//##################################################
static DRAM_ATTR uint32_t cfd_setpoint_mid;
static DRAM_ATTR uint32_t cfd_setpoint_min;
static DRAM_ATTR uint32_t cfd_setpoint_max;
static DRAM_ATTR uint32_t cfd_setpoint_probing;
static DRAM_ATTR uint32_t cfd_average_slow_size;
static DRAM_ATTR uint32_t cfd_average_fast_size;   
static DRAM_ATTR uint32_t cfd_ignition_treshhold; // value is in adc resolution. 12bit = 0-4095. cFd sample peaking above this is a spark

//##################################################
// vFd = Voltage settings
//##################################################
static DRAM_ATTR int vfd_probe_trigger;            // adc threshold in adc value
static DRAM_ATTR int vfd_forward_thresh;           // if vfd is above this value forward motion is blocked
static DRAM_ATTR int vfd_short_circuit_threshhold; // vFd below this is considered a short circuit

//##################################################
// Other settings
//##################################################
static DRAM_ATTR int  i2s_buffer_length;
static DRAM_ATTR int  i2s_num_bytes;
static DRAM_ATTR int  pulse_off_duration;           // pulse of duration on short circuits in microseconds 
static DRAM_ATTR int  retract_confirmations;        // soft retraction confirmations before it enter into correction state
static DRAM_ATTR bool scope_use_high_res;           // show each sample in the i2s batch on the scope for either cfd or vfd
static DRAM_ATTR bool scope_show_vfd_channel;       // show vfd samples instead of cfd


//##################################################
// Stuff for kSps benchmarking
//##################################################
static DRAM_ATTR uint32_t benchmark_ksps          = 0;
static DRAM_ATTR uint32_t benchmark_adc_counter   = 0;
static DRAM_ATTR uint32_t bench_timer_interval    = 2048; // needs to be a power of two; don't change without changing the bits to right shift; Power of two: (2,4,8,16,32,64,128,256,512,1024,2048...)
static DRAM_ATTR size_t   benchmark_bits_to_shift = 11;   // Corresponds to the power of two from the comment above: (1,2,3,4,5,6,7,8,9,10,11...)




//###########################################################################
// Called from planner on core 1 to request the current motionplan
//###########################################################################
IRAM_ATTR int get_calculated_motion_plan( bool enforce_fresh ){
    if( 
        !gconf.edm_pause_motion && // prevent deadlock
        ( enforce_fresh || motion_plan_atomic.load() == MOTION_PLAN_FORWARD ) // if peek is forward or enforce true wait for a fresh plan
    ){
        std::unique_lock<std::mutex> lock(mtx);
        if( !cv.wait_for( lock, std::chrono::microseconds( 1000000 ), [] { return new_motion_plan.load(); } ) ){
            new_motion_plan.store( false );
            return MOTION_PLAN_TIMEOUT;
        }
    } 
    new_motion_plan.store( false );
    return motion_plan_atomic.load();
}



//###################################################
// Helper functions
//###################################################
int percentage_to_adc( float percentage ){ // convert percentage (0-100) into an ADC value
    if( percentage <= 0.0 ) return 0;
    if( percentage >= 100.0 ) return VSENSE_RESOLUTION;
    return round( ( float( VSENSE_RESOLUTION ) / 100.0 ) * percentage );
}

float adc_to_percentage( int adc_value ){ // convert an ADC value (0-VSENSE_RESOLUTION) into percentage
    if( adc_value <= 0 ) return 0.0;
    if( adc_value >= VSENSE_RESOLUTION ) return 100.0;
    return ( float( adc_value ) / float( VSENSE_RESOLUTION ) ) * 100.0;
}

//############################################################
// Create and return a DMA buffer of variable size
//############################################################
uint16_t* create_dma_buffer( size_t variable_length ){

    if( dma_buffer != nullptr ){ // seems to be more stable to always delete the buffer and create a new one
        heap_caps_free(dma_buffer);
        dma_buffer = nullptr;
    }

    /*if( dma_buffer != nullptr && dma_buffer_current_size != variable_length ){
        heap_caps_free(dma_buffer);
        dma_buffer = nullptr;
    }*/
    if( dma_buffer == nullptr ) {
        if( variable_length > MAX_BUFFER_SIZE ) {
            return nullptr;
        }
        dma_buffer = (uint16_t*)heap_caps_malloc(variable_length * sizeof(uint16_t), MALLOC_CAP_DMA);
        if( dma_buffer == nullptr ) {
            return nullptr;
        }
        dma_buffer_current_size = variable_length;
    } 
    memset(dma_buffer, 0, variable_length * sizeof(uint16_t));
    return dma_buffer;
}

//#######################################################################################################
// Some sample rates are just not working and i2s fails to start. Not all the time the error is catchable
// this function does also not 100% ensure success but it reduces possible errors by adjusting the rate
// to be within a given tolerance
//#######################################################################################################
bool sample_rate_valid( int rate, int master_clock = I2S_MASTER_CLOCK_SPEED, int bits_per_sample = 16, int channels = 2 ) {
    double bclkFreq      = double( rate ) * bits_per_sample * channels;
    double ratio         = double( master_clock ) / bclkFreq;
    double ratio_rounded = std::round(ratio);
    double diff          = std::abs(ratio - ratio_rounded);
    double tolerance     = 0.2;
    return diff <= tolerance ? true : false;
}

//###########################################################################
// Returns true if the sensors task started
//###########################################################################
IRAM_ATTR bool sensors_task_running(){
    return adc_monitor_task_running.load();
}


//###########################################################################
// Flag limit event
//###########################################################################
void IRAM_ATTR limit_switch_on_interrupt() {
    if( 
        !gconf.gedm_disable_limits && !sensors.limit_switch_event_detected &&
        (  get_machine_state() < STATE_ALARM && !is_machine_state( STATE_HOMING ) )
    ){
        sensors.limit_switch_event_detected = true;
        int data = 2;
        if( remote_control_queue != NULL ){ xQueueSendFromISR( remote_control_queue, &data, NULL ); }
    }
}

//###########################################################################
// Flag estop event
//###########################################################################
void IRAM_ATTR motion_switch_on_interrupt(){
    if( sensors.block_on_off_switch ){ return; }
    sensors.on_off_switch_event_detected = true;
    int data = 1;
    if( remote_control_queue != NULL ){ xQueueSendFromISR( remote_control_queue, &data, NULL ); }
}


//###########################################################################
// Notify the sense loop to create the ksps benchmark
//###########################################################################
IRAM_ATTR void bench_on_timer() {
    int data = SENSE_BENCHMARK;
    xQueueOverwriteFromISR( adc_read_trigger_queue, &data, NULL );
}




G_SENSORS gsense;
G_SENSORS::~G_SENSORS(){}

//############################################################
// Constructor
//############################################################
G_SENSORS::G_SENSORS() : mclk( I2S_MASTER_CLOCK_SPEED ){

    pinMode( ON_OFF_SWITCH_PIN,       INPUT_PULLDOWN ); // motion switch
    pinMode( STEPPERS_LIMIT_ALL_PIN,  INPUT_PULLDOWN ); // limit switches (one pin for all)
    adc1_config_width( ADC_WIDTH_12Bit );
    adc1_config_channel_atten( CURRENT_SENSE_CHANNEL, ADC_ATTEN_11db );
    adc1_config_channel_atten( VOLTAGE_SENSE_CHANNEL, ADC_ATTEN_11db ) ;
    adc1_get_raw( VOLTAGE_SENSE_CHANNEL ); 
    adc1_get_raw( CURRENT_SENSE_CHANNEL ); 
    esp_adc_cal_characterize( ADC_UNIT_1, ADC_ATTEN_11db, ADC_WIDTH_12Bit, 1100, &adc1_chars );
    pinMode( CURRENT_SENSE_PIN, INPUT );
    pinMode( VOLTAGE_SENSE_PIN, INPUT );

}

void G_SENSORS::create_sensors(){
    debuglog("Creating sensors", DEBUG_LOG_DELAY );
    unlock_motion_switch();
    debuglog("Starting sensor queues", DEBUG_LOG_DELAY );
    create_queues();
    debuglog("Starting sensor tasks", DEBUG_LOG_DELAY );
    create_tasks();
    sennsors_running.store( true );
}

//###################################################
// Create the queues
//###################################################
void G_SENSORS::create_queues(){
    int max_rounds = 10;
    while( adc_read_trigger_queue == NULL && --max_rounds > 0 ){ adc_read_trigger_queue = xQueueCreate( 1,  sizeof(int) ); vTaskDelay(5); }
    max_rounds = 10;
    while( remote_control_queue == NULL && --max_rounds > 0 ){ remote_control_queue  = xQueueCreate( 20,  sizeof(int) ); vTaskDelay(5); }
}

void G_SENSORS::sensor_end(){
    debuglog("Stopping sensors");
    if( ! sennsors_running.load() ){
        debuglog("Sensors not running",DEBUG_LOG_DELAY);
        return;
    }
    vTaskDelete( remote_control_task_handle );
    vTaskDelete( adc_monitor_task_handle );
    if( adc_read_trigger_queue != NULL ){ vQueueDelete( adc_read_trigger_queue ); adc_read_trigger_queue = NULL; }
    if( remote_control_queue != NULL ){ vQueueDelete( remote_control_queue ); remote_control_queue = NULL; }    
    vTaskDelay(DEBUG_LOG_DELAY);

    bool lock_taken = arcgen.lock();
    gsense.stop();
    arcgen.unlock( lock_taken );
}
















































//############################################################
// Set the sample rate in Hz
//############################################################
int G_SENSORS::set_sample_rate( int rate ){
    while( !sample_rate_valid( rate, mclk ) ){ ++rate; }
    if( rate > 1000000 ){ while( !sample_rate_valid( rate, mclk ) ){ --rate; } }
    return rate;
}

//############################################################
// Get the current state (idle, budy etc)
//############################################################
IRAM_ATTR i2s_states G_SENSORS::get_i2s_state(){
    return i2s_state.load();
}

//############################################################
// Compare the current state against a wanted state
//############################################################
IRAM_ATTR bool G_SENSORS::is_state( i2s_states state ){
    return ( i2s_state.load() == state );
}

//############################################################
// Set the state (idle, budy etc)
//############################################################
IRAM_ATTR void G_SENSORS::set_i2s_state( i2s_states state ){
    i2s_state.store( state );
}

//############################################################
// Wait for I2S to finish what it does
//############################################################
IRAM_ATTR void G_SENSORS::wait_for_idle( bool aquire_lock ){
    while( !is_state( I2S_CTRL_IDLE ) ){
        vTaskDelay(1);
    }
}



//############################################################
// Restarts the sampling service
//############################################################
void G_SENSORS::restart(){
    //#######################################
    // Restart only if i2s is running
    //#######################################
    if( is_state( I2S_CTRL_NOT_AVAILABLE ) ){
        return;
    }
    //#######################################
    // Set state to restart
    //#######################################
    set_i2s_state( I2S_RESTARTING );

    //while( !gscope.scope_is_running() ){ vTaskDelay(200); }

    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
    //#######################################
    // Stop I2S and uninstall driver
    //#######################################
    stop();
    //#######################################
    // Install i2s driver and start it
    //#######################################
    begin(); // state is now set to I2S_CTRL_IDLE by the reset function called inside begin()
    release_lock_for( ATOMIC_LOCK_GSCOPE );

}

//############################################################
// Stop I2S and uninstall driver
//############################################################
void G_SENSORS::stop(){
    if( is_state( I2S_CTRL_NOT_AVAILABLE ) ){ // wasn't ready
        return;
    }
    i2s_stop( I2S_NUM_0 );
    i2s_adc_disable(I2S_NUM_0);
    i2s_driver_uninstall( I2S_NUM_0 );
    set_i2s_state( I2S_CTRL_NOT_AVAILABLE );
}

//############################################################
// Starts the sampling service
//############################################################
void G_SENSORS::begin(){

    //############################################################
    // This little delay seems needed for stable init
    //############################################################
    static const int required_delay = 1000;
    //############################################################
    // Create a new DMA buffer if needed
    //############################################################
    while( create_dma_buffer( i2s_buffer_length ) == nullptr ){
        vTaskDelay(DEBUG_LOG_DELAY);
    }

    periph_module_reset(PERIPH_I2S0_MODULE);
    //############################################################
    // Some math can be done in advance
    //############################################################
    i2s_num_bytes = sizeof(uint16_t) * i2s_buffer_length;
    //############################################################
    // I2S core configuration
    //############################################################
    i2s_config_t i2s_config; 
    i2s_config.mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN);
    i2s_config.sample_rate          = set_sample_rate( sample_rate );  
    i2s_config.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT; 
    i2s_config.channel_format       = I2S_CHANNEL_FMT_ALL_LEFT;//I2S_CHANNEL_FMT_RIGHT_LEFT;//I2S_CHANNEL_FMT_ALL_LEFT; //I2S_CHANNEL_FMT_ONLY_LEFT,
    i2s_config.communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_STAND_MSB);
    i2s_config.intr_alloc_flags     = (ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_INTRDISABLED); //ESP_INTR_FLAG_LEVEL1,
    i2s_config.dma_buf_count        = buffer_count;
    i2s_config.dma_buf_len          = i2s_buffer_length;
    i2s_config.use_apll             = true;
    i2s_config.tx_desc_auto_clear   = true;
    i2s_config.fixed_mclk           = mclk;

    //############################################################
    // install i2s driver
    //############################################################
    while( ESP_OK != i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) ){ 
        vTaskDelay(5); 
    }

    i2s_zero_dma_buffer( I2S_NUM_0 );
    //############################################################
    // Setup some things and enable second channel
    //############################################################
    i2s_set_adc_mode(ADC_UNIT_1, CURRENT_SENSE_CHANNEL);
    i2s_adc_enable(I2S_NUM_0);
    SET_PERI_REG_BITS(SYSCON_SARADC_CTRL_REG, SYSCON_SARADC_SAR1_PATT_LEN, 1, SYSCON_SARADC_SAR1_PATT_LEN_S);
    WRITE_PERI_REG(SYSCON_SARADC_SAR1_PATT_TAB1_REG, 0x7F6F0000);


    //SYSCON.saradc_ctrl.sar1_patt_len = 1; // 2 items 0,1 = 1,2
    //SYSCON.saradc_sar1_patt_tab[0]   = 0x7F6F0000;
    //SYSCON.saradc_ctrl2.meas_num_limit = 0;
    //SYSCON.saradc_ctrl.work_mode = 2;

    //delay(required_delay);
    vTaskDelay(required_delay);

    i2s_start(I2S_NUM_0);
    set_i2s_state( I2S_CTRL_IDLE );

    reset_sensor_global();

}



//###########################################################################
// Check the limit switch
//###########################################################################
bool IRAM_ATTR G_SENSORS::limit_switch_read(){
    vTaskDelay( 32 / portTICK_PERIOD_MS ); 
    bool state  = false;
    if ( !is_machine_state( STATE_HOMING ) && GRBL_LIMITS::limits_get_state() ){
        if( !gconf.gedm_disable_limits ){
            if( !is_machine_state( STATE_ESTOP ) ){ 
                set_alarm( ERROR_HARDLIMIT );
                set_machine_state( STATE_ESTOP );
                vTaskDelay(DEBUG_LOG_DELAY);
            }
        }
        state = true;
    } else {
        state = false;
    }
    new_motion_plan.store( true ); // early exit on waits
    sensors.limit_switch_event_detected = false;
    return state;
}

bool IRAM_ATTR G_SENSORS::unlock_motion_switch(){
    sensors.block_on_off_switch = false;
    motion_switch_read();
    motion_switch_changed.store( true );
    return true;
}

//###########################################################################
// Check the estop switch
// Need to change this
// currently toggling the switch faster can create errors
// todo
//###########################################################################
IRAM_ATTR bool G_SENSORS::motion_switch_read(){
    vTaskDelay( 32 / portTICK_PERIOD_MS ); 
    bool state  = false;
    //int is_high = ( GPIO_REG_READ( GPIO_IN1_REG ) >> ( ON_OFF_SWITCH_PIN - 32 ) ) & 0x1;;
    //if( is_high ){ //
    if( digitalRead(ON_OFF_SWITCH_PIN) ){
        if( system_block_motion ){
            enforce_redraw.store( true );
            motion_switch_changed.store( true );
        }
        system_block_motion = false;
        state = true;
    } else {
        if( is_system_mode_edm() ){
            sensors.block_on_off_switch = true; // get some control over the shutdown and block reenabling motion until all is done
        }
        if( !system_block_motion ){
            enforce_redraw.store( true );
            motion_switch_changed.store( true );
        }
        system_block_motion = true;
        state = false;
    }
    new_motion_plan.store( true );
    sensors.on_off_switch_event_detected = false;
    return state;
}










//###################################################
// Set the flag for a sensor object reset
// Before processing the next i2s batch it will reset
// the sensor objects used to make decision
//###################################################
void G_SENSORS::reset_sensor_global(){ // called from planner on core1
    reset_sensor_state.store( true );
}

//##############################################
// Default event queue loop / remote control
//##############################################
IRAM_ATTR void remote_control_task(void *parameter){
    while( remote_control_queue == NULL ) vTaskDelay(10);

    // Timer interrupt for benchmarking the kSps
    benchmark_timer = timerBegin(0, 80, true);
    timerAttachInterrupt(benchmark_timer, &bench_on_timer, true);
    timerAlarmWrite(benchmark_timer, bench_timer_interval*1000, true);
    timerAlarmEnable(benchmark_timer);

    int data = 0;
    for (;;){
        xQueueReceive( remote_control_queue, &data, portMAX_DELAY ); 
        bool lock_taken = arcgen.lock(); // feedback task starves other stuff if not blocked
        switch ( data ){
            case 1:  G_SENSORS::motion_switch_read(); break;
            case 2:  G_SENSORS::limit_switch_read();  break;
            case 4:  arcgen.pwm_off();                break;
            case 5:  arcgen.pwm_on();                 break;
            case 6:  wire_feeder.stop();              break;
            case 7:  wire_feeder.start();             break;
            default: break;
        }
        data = 0;
        arcgen.unlock( lock_taken );
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}



//###########################################################################
// Reset the feedback data object
//###########################################################################
void G_SENSORS::reset_feedback_data(){
    memset( &feedback, 0, sizeof( feedback ) );
    memset( &adc_data, 0, sizeof( adc_data ) );
    memset( &wavectrl, 0, sizeof( wavectrl ) );
    memset( &peaktrack, 0, sizeof( peaktrack ) );
}


//###########################################################################
// Push samples to the scope ( the lock does nothing currently )
//###########################################################################
static IRAM_ATTR void adc_to_scope( uint16_t sample, int8_t plan ){

    if( 
        scope_batch_ready.load() || 
        !acquire_lock_for( ATOMIC_LOCK_GSCOPE, false ) // don't wait for lock if it is already taken
    ){ return; } // full batch waiting to get processed or lock not available

        if( gscope.add_to_scope( sample, plan ) ){

            gscope.set_meta_data(
                benchmark_ksps, 
                ( uint8_t ) plan, 
                feedback.vfd_avg_slow, 
                feedback.cfd_avg_fast, 
                feedback.cfd_avg_slow,
                ( int ) feedback.cfd_avg_fast
                //( int ) feedback.vfd_variable_thresh
            );
            
        }

    release_lock_for( ATOMIC_LOCK_GSCOPE );

}

#define DISCHARGE_COUNT_TRIGGER 100000  // us
#define RAMP_DURATION           1000000 // us
#define RAMP_DURATION_ACTIVE    500000  // us

IRAM_ATTR void adc_monitor_task(void *parameter){ 

    G_SENSORS *__this = (G_SENSORS *)parameter; // pointer to the sensor object

    __this->init_settings();

    static DRAM_ATTR int64_t micros_now       = 0;
    static DRAM_ATTR int64_t ramp_cycle_start = 0;
    static DRAM_ATTR int64_t rampup_duration  = RAMP_DURATION;
    static DRAM_ATTR int64_t ramp_up_interval = rampup_duration / cfd_setpoint_min;
    


    //#######################################################
    // Multisamplers for current and voltage feedbacks
    // The size needs to be a power of two due to bitwise
    // operations
    //#######################################################
    static       DRAM_ATTR uint32_t loop_index                                     = 0;
    static       DRAM_ATTR uint32_t work_index                                     = 0;
    static const DRAM_ATTR uint32_t csense_sampler_buffer_size_n1                  = csense_sampler_buffer_size-1;
    static       DRAM_ATTR uint32_t multisample_counts                             = 0;
    static       DRAM_ATTR uint32_t multisample_buffer[csense_sampler_buffer_size] = {0,};
    static const DRAM_ATTR uint32_t v_multisample_size                             = 16; // needs to be a power of two! If not the average creation will fail 2,4,8,16,32
    static const DRAM_ATTR uint32_t v_multisample_size_n1                          = v_multisample_size-1; 
    static       DRAM_ATTR uint32_t v_multisample_counts                           = 0;
    static       DRAM_ATTR uint32_t v_multisample_buffer[v_multisample_size]       = {0,};
    //#######################################################
    // I2S shift out specific
    //#######################################################
    static       DRAM_ATTR uint64_t          i2s_sum_cfd               = 0;
    static       DRAM_ATTR uint64_t          i2s_sum_vfd               = 0;
    static       DRAM_ATTR uint32_t          i2s_count_cfd             = 0;
    static       DRAM_ATTR uint32_t          i2s_count_vfd             = 0;
    static       DRAM_ATTR uint32_t          i2s_channel               = 0;
    static       DRAM_ATTR uint32_t          i2s_sample                = 0;
    //#######################################################
    // 
    //#######################################################
    static const DRAM_ATTR uint32_t          adc_jitter                = 20;
    static const DRAM_ATTR uint32_t          batches_to_bail_after_off = 3;
    static       DRAM_ATTR int               pulse_period              = arcgen.get_pulse_period_us();
    static       DRAM_ATTR ism_sensor_states previous_state            = sensor_state;
    static       DRAM_ATTR uint32_t          discharge_count_previous  = 0;
    static       DRAM_ATTR uint32_t          discharge_count_max       = 0;
    static       DRAM_ATTR int               data                      = 1;
    static       DRAM_ATTR int8_t            work_plan                 = MOTION_PLAN_FORWARD;
    static       DRAM_ATTR uint32_t          arc_pulse_off_duration_us = 3 * pulse_period;
    static       DRAM_ATTR uint32_t          bail_after_off_count      = 0;
    static       DRAM_ATTR uint32_t          retract_confirm_counter   = 0; // if plan is >=4 (abiver setpoint and something else-..) it counts for confirmation before providing a retraction instruction
    static       DRAM_ATTR uint32_t          ramp_up_setpoint          = 0; // 
    static       DRAM_ATTR uint32_t          release_confirmations     = 0;
    static       DRAM_ATTR bool              recovered_from_bail           = false;
    static       DRAM_ATTR bool              full_short_circuit            = false;
    static       DRAM_ATTR bool              invert_plan                   = false;
    static       DRAM_ATTR bool              first_seek_done               = false;
    static       DRAM_ATTR bool              feed_condition_met            = false;
    static       DRAM_ATTR bool              retract_condition_met         = false;
    static       DRAM_ATTR bool              retract_release_condition_met = false;
    static       DRAM_ATTR bool              high_res_show_vfd             = false;
    static       DRAM_ATTR bool              high_res_show_cfd             = false;
    static       DRAM_ATTR bool              has_peak_sample               = false;
    static       DRAM_ATTR bool              no_water_contact              = true;


    static DRAM_ATTR bool     fast_added             = false;
    static DRAM_ATTR uint16_t fast_cfd_avg_minus_one = cfd_average_fast_size - 1;

    __this->begin();         // start with the default settings
    __this->wait_for_idle(); // idle state is set after it is ready

    adc_monitor_task_running.store( true ); // set flag that task is running
    restart_i2s_flag.store( true );         // flag for i2s restart on initial run
    reset_sensor_state.store( true );

    for(;;){

        xQueueReceive( adc_read_trigger_queue, &data, portMAX_DELAY );

        if( stop_sampling.load() || arcgen.get_pwm_is_off() ){
            benchmark_adc_counter = 0;
            new_motion_plan.store( true ); // prevent lock
            vTaskDelay(DEBUG_LOG_DELAY);
            continue;
        }

        if( data == SENSE_BENCHMARK && benchmark_adc_counter > 0 ){
            benchmark_ksps        = ( uint32_t )( ( benchmark_adc_counter * i2s_buffer_length ) >> benchmark_bits_to_shift );
            benchmark_adc_counter = 0;
        }

        //#########################################################
        // Reset sensor object if needed. Set by the planner on
        // core 1 after pause / pause recovery end / flush end
        //#########################################################
        if( reset_sensor_state.load() ){
            reset_sensor_state.store( false );
            motion_plan_atomic.store( MOTION_PLAN_FORWARD );
            new_motion_plan.store( false );
            __this->refresh_settings();
            __this->reset_feedback_data();
            pulse_period                     = arcgen.get_pulse_period_us();
            invert_plan                      = false;
            first_seek_done                  = false;
            retract_confirm_counter          = 0;
            benchmark_adc_counter            = 0;
            discharge_count_previous         = 0;
            discharge_count_max              = 0;
            arc_pulse_off_duration_us        = 3 * pulse_period;
            ramp_up_interval                 = rampup_duration / cfd_setpoint_min;
            previous_state                   = SS_SEEK;
            sensor_state                     = SS_SEEK;
            release_confirmations            = 0;
            recovered_from_bail              = false;
            bail_after_off_count             = 1;

            high_res_show_vfd = ( scope_use_high_res &&  scope_show_vfd_channel ) ? true : false;
            high_res_show_cfd = ( scope_use_high_res && !scope_show_vfd_channel ) ? true : false;

            if( cfd_average_fast_size > cfd_average_slow_size ){ // just to be sure 
                cfd_average_fast_size = cfd_average_slow_size; 
            }

            sensor_errors                = S_ERROR_NONE;
            feedback.vfd_variable_thresh = vfd_forward_thresh; // reset

        }

        //peaktrack.pre_discharge_collected = false; // reset flag


        //###########################################################################
        // Get the i2s buffer with the precious samples
        //###########################################################################
        size_t bytes_read;
        if( ESP_OK == i2s_read( I2S_NUM_0, dma_buffer, i2s_num_bytes, &bytes_read, I2S_TIMEOUT_TICKS ) ){

            if( ( bytes_read >> 1 ) == i2s_buffer_length ){

                ++benchmark_adc_counter; // Increment the batch counter for the ksps benchmark

                //#################################################################################################
                //
                // Process the I2S buffer
                //
                //#################################################################################################
                i2s_sum_cfd = i2s_sum_vfd = i2s_count_cfd = i2s_count_vfd = 0; // chained reset for speed
                for( loop_index = 0; loop_index < i2s_buffer_length; ++loop_index ){

                    i2s_channel = ( dma_buffer[ loop_index ] >> 12 ) & 0x07;  // extract channel
                    i2s_sample  =   dma_buffer[ loop_index ]         & 0xFFF; // extract sample

                    if( i2s_channel == 6 ){ // current sense

                        i2s_sum_cfd += i2s_sample; // add sample to the cfd sum
                        ++i2s_count_cfd;

                        if( high_res_show_cfd ){ // Note: this only adds the current motionplan and not the one for this recent sample
                            adc_to_scope( i2s_sample, adc_data.plan ); 
                        }



                    } else if( i2s_channel == 7 ){ // voltage sense
                        
                        // filter out some of the off time, won't get it accurate but close enough to have it working across different duties
                        if( i2s_sample <= adc_jitter ){
                            continue;
                        }

                        i2s_sum_vfd += i2s_sample; // add sample to the vfd sum
                        ++i2s_count_vfd;

                        if( high_res_show_vfd ){ // Note: this only adds the current motionplan and not the one for this recent sample
                            adc_to_scope( i2s_sample, adc_data.plan ); 
                        }

                    }

                }

                feedback.cfd_recent = i2s_sum_cfd > 0 ? int( i2s_sum_cfd / i2s_count_cfd ) : 0; 
                feedback.vfd_recent = i2s_sum_vfd > 0 ? int( i2s_sum_vfd / i2s_count_vfd ) : 0; 


                //##########################################################################################################
                //
                // I2S sample acquisition and parsing finished
                // Creating the motion plan ( Probing uses different logic )
                //
                //##########################################################################################################
                if( is_machine_state( STATE_PROBING ) ){

                    bail_after_off_count = 0;
                    //############################################################################################################
                    // probing at 15v 40khz 0.2A creates around 300vfd on water contact drops below 50 on touch cfd > 400 on touch 
                    // with full range spikes. Probing motion plan is a little different
                    //############################################################################################################
                    work_plan = MOTION_PLAN_HOLD_SOFT; 
                    if( probe_touched ){ 
                        reset_sensor_state.store( true );
                        work_plan = MOTION_PLAN_SOFT_SHORT; 
                    } else {
                        if( feedback.cfd_recent >= cfd_setpoint_probing ){
                            work_plan     = MOTION_PLAN_SOFT_SHORT; // probe confirmed
                            probe_touched = true;
                            reset_sensor_state.store( true );
                        } else {
                            probe_touched = false;
                            work_plan     = MOTION_PLAN_FORWARD;
                        }
                    }

                } else {


                    //############################################################################################################
                    //
                    // Create the normal process motionplan
                    //
                    //############################################################################################################
                    work_plan       = MOTION_PLAN_FORWARD;                                         // Set default motion plan ( forward )
                    has_peak_sample = feedback.cfd_recent > cfd_ignition_treshhold ? true : false; // Flag if cfd feedback peaked above the threshold used to determine if we had a discharge
            
                    //############################################################################################################
                    //
                    // A true hard short circuit is when the DPM/DPH voltage drops below the short circuit threshold.
                    // The voltage feedback here is very tricky. The feedback behaves like this:
                    //
                    // Electrode not touching the water and no conductive path established: 
                    //     VFD = 0 ( plus minus some jitter )
                    //     0 0 3 0 1 0 5 0 0 0
                    //
                    // Electrode touching the water: 
                    //     VFD rapidly jumps up to some level that depends on water conductivity, electrode surface and duty.
                    //     VFD then slowly increases linear while electrode approaches the work
                    //     0 0 3 0 1 0 5 0 0 0 1800 1810 1900 1950 1960 1980
                    //
                    // Electrode touching the work and a full short circuit is established:
                    //     VFD may create a short peak in voltage and if the current exceeds the DPM/DPH setting the voltage 
                    //     will start to drop. 
                    //     0 0 3 0 1 0 5 0 0 0 1800 1810 1900 1950 1960 1980 2200 3000 4095 2000 1300 ( something like that.. Example. )
                    //
                    // How much the voltage drops on a short circuit depends on the settings. Lower max current on the DPM/DPH will
                    // result in sharper drops. But also higher duty cycles will create sharper drops.
                    //
                    // How high the voltage rises under normal conditions depends also on the duty cycle. The ON-TIME of the PWM
                    // will pull the feedbakc source low. The longer the ON-TIME the lower the final max voltage can be.
                    // In theory only the OFF-TIME samples would be the better source but on an ESP it is not possible to get that
                    // timings correct and the short circuit detections may also be a little tricky to figure out.
                    // Also noticed that if only the highest sample of the I2S batch is used it will produce high peaks on sparks
                    // All in all not easy to work with.
                    //     
                    //############################################################################################################

                    no_water_contact = ( ( feedback.vfd_recent == 0 && feedback.cfd_recent == 0 ) || ( 
                            feedback.vfd_recent             < adc_jitter
                            && feedback.cfd_recent          < adc_jitter // after a discharge it is also possible that this is a zero reading, needs harder proof
                            && feedback.cfd_recent_previous < adc_jitter // previous cfd above jitter 
                            && feedback.cfd_avg_fast        < adc_jitter ) // fast average above jitter
                    ) ? true : false;

                    full_short_circuit = (
                        feedback.vfd_recent < vfd_short_circuit_threshhold 
                        && !no_water_contact
                    ) ? true : false;
                    

                    //############################################################################################################
                    //
                    // Skip samples after PWM was shortly turned off for protection and ignore the samples collected in that time
                    // After such a protection PWM off, the next samples are loaded but the loop will continue early to skip them
                    // On true shorts it will bypass that and keep going on to reenter protection
                    //
                    //############################################################################################################
                    if( bail_after_off_count > 0 && !full_short_circuit ){
                        --bail_after_off_count;
                        motion_plan_atomic.store( adc_data.plan ); // keep the old motion plan and notify the collector
                        new_motion_plan.store(true);               // set flag that a new motionplan is available
                        cv.notify_all();                           // notify signal
                        recovered_from_bail = true;                // set the flag that we had a bail situation
                        if( !scope_use_high_res ){                 // even if the samples are not used we can still show them on the scope
                            adc_to_scope( 
                                scope_show_vfd_channel ? 
                                feedback.vfd_recent : 
                                feedback.cfd_recent, 
                                adc_data.plan 
                            ); 
                        }
                        continue; // skip this round early
                    }



                    //############################################################################################################
                    //
                    // Process the recent I2S averages (vfd/cfd) in the multisamper and get fast/slow averaging
                    //
                    //############################################################################################################

                    // moving slow (16 samples) and fast (4 samples) average
                    // buffer size is 16 (power of two)
                    // v_multisample_size_n1 = 15
                    // v_multisample_counts points to the slot to be overwritten (oldest slow sample)
                    // oldest slow sample (leaving 16-sample window)
                    // oldest fast sample (leaving 4-sample window)
                    // fast window contains the last 4 written samples:
                    // (count-1), (count-2), (count-3), (count-4)
                    // so the one leaving is (count-4)

                    // remove sample from slow vfd average sum
                    /*i2s_sample = v_multisample_buffer[ v_multisample_counts ]; // sample to be removed, the new sample is put to the buffer later and the current index points to the last added sample
                    if( feedback.vfd_avg_slow_sum >= i2s_sample ) {
                        feedback.vfd_avg_slow_sum -= i2s_sample;
                    } else {
                        // ensure we don't overshoot (uint)
                        feedback.vfd_avg_slow_sum = 0;
                    }

                    // remove sample from fast vfd average sum
                    i2s_sample = v_multisample_buffer[ (v_multisample_counts - 4) & v_multisample_size_n1 ]; // sample to be removed
                    if( feedback.vfd_avg_fast_sum >= i2s_sample ) {
                        feedback.vfd_avg_fast_sum -= i2s_sample;
                    } else {
                        feedback.vfd_avg_fast_sum = 0;
                    }

                    // add new sample
                    feedback.vfd_avg_slow_sum += feedback.vfd_recent; // add new sample
                    feedback.vfd_avg_fast_sum += feedback.vfd_recent; // add new sample
                    // divide bitwise for vfd average with static size
                    feedback.vfd_avg_slow = feedback.vfd_avg_slow_sum >> 4; // divide by 16
                    feedback.vfd_avg_fast = feedback.vfd_avg_fast_sum >> 2; // divide by 4
                    // store sample to buffer and advance index
                    v_multisample_buffer[v_multisample_counts] = feedback.vfd_recent;                                // store new sample
                    v_multisample_counts                       = (v_multisample_counts + 1) & v_multisample_size_n1; // advance ring index
                    */






                    work_index                                 = v_multisample_counts;
                    v_multisample_buffer[v_multisample_counts] = feedback.vfd_recent;                                // store new sample
                    v_multisample_counts                       = (v_multisample_counts + 1) & v_multisample_size_n1; // advance ring index
                    feedback.vfd_avg_slow = feedback.vfd_avg_fast = 0;
                    fast_added            = false;
                    for( loop_index = 0; loop_index < v_multisample_size; ++loop_index ){
                        if( loop_index < 4 ){ // fast vfd avergae
                            feedback.vfd_avg_fast += v_multisample_buffer[ work_index ];
                        } 
                        if( loop_index >= 3 ){
                            if( !fast_added ){
                                feedback.vfd_avg_slow = feedback.vfd_avg_fast;
                                fast_added            = true;
                            }
                            feedback.vfd_avg_slow += v_multisample_buffer[ work_index ]; // slow cfd average
                        }
                        work_index = ( work_index - 1 ) & v_multisample_size_n1; // move reverse
                    }                    
                    feedback.vfd_avg_slow >>= 4; // divide by 16
                    feedback.vfd_avg_fast >>= 2; // divide by 4



                    // moving slow and fast average for current channel
                    // buffer size is 64 (power of two)
                    // csense_sampler_buffer_size_n1 = 63
                    // multisample_counts points to the slot to be overwritten
                    // slow and fast average size is user defined
                    // in the example the slow average is 30 samples width and the fast is 4 samples width
                    // the highest sample within the slow average window is tracked
                    // while setting the sizes for slow and fast average there is protection logic to keep fast below slow
                    // and slow below max buffer size
                    // no overflow protection needed.. Lots of room in a uint32_t for the sum of a few samples

                    work_index                             = multisample_counts;
                    feedback.cfd_peak                      = feedback.cfd_recent;
                    multisample_buffer[multisample_counts] = feedback.cfd_recent;                                      // store new sample
                    multisample_counts                     = (multisample_counts + 1) & csense_sampler_buffer_size_n1; // advance ring index

                    if( cfd_average_slow_size <= 1 ){
                        feedback.cfd_avg_slow = feedback.cfd_recent;  
                        feedback.cfd_avg_fast = feedback.cfd_recent; 
                    } else {
                        feedback.cfd_avg_slow = feedback.cfd_avg_fast = 0;
                        fast_added            = false;
                        for( loop_index = 0; loop_index < cfd_average_slow_size; ++loop_index ){
                            if( multisample_buffer[ work_index ] > feedback.cfd_peak ){
                                feedback.cfd_peak = multisample_buffer[ work_index ];
                            }
                            if( loop_index < cfd_average_fast_size ){ // fast cfd avergae
                                feedback.cfd_avg_fast += multisample_buffer[ work_index ];
                            } 
                            if( loop_index > fast_cfd_avg_minus_one ){
                                if( !fast_added ){
                                    feedback.cfd_avg_slow = feedback.cfd_avg_fast;
                                    fast_added            = true;
                                }
                                feedback.cfd_avg_slow += multisample_buffer[ work_index ]; // slow cfd average
                            }
                            work_index = ( work_index - 1 ) & csense_sampler_buffer_size_n1; // move reverse
                        }
                        feedback.cfd_avg_fast /= cfd_average_fast_size;
                        feedback.cfd_avg_slow /= cfd_average_slow_size;
                    }


                    // remove sample from slow cfd average sum
                    /*i2s_sample = multisample_buffer[ ( multisample_counts - cfd_average_slow_size ) & csense_sampler_buffer_size_n1 ]; // sample to be removed
                    if( feedback.cfd_avg_slow_sum >= i2s_sample ) {
                        feedback.cfd_avg_slow_sum -= i2s_sample;
                    } else {
                        feedback.cfd_avg_slow_sum = 0;
                    }

                    // remove sample from fast cfd average sum
                    i2s_sample = multisample_buffer[ ( multisample_counts - cfd_average_fast_size ) & csense_sampler_buffer_size_n1 ]; // sample to be removed
                    if( feedback.cfd_avg_fast_sum >= i2s_sample ) {
                        feedback.cfd_avg_fast_sum -= i2s_sample;
                    } else {
                        feedback.cfd_avg_fast_sum = 0;
                    }

                    // add new sample
                    feedback.cfd_avg_slow_sum += feedback.cfd_recent; // add new sample
                    feedback.cfd_avg_fast_sum += feedback.cfd_recent; // add new sample
                    // divide bitwise for average
                    feedback.cfd_avg_slow = feedback.cfd_avg_slow_sum / cfd_average_slow_size; // no power of two so no bitshifting
                    feedback.cfd_avg_fast = feedback.cfd_avg_fast_sum / cfd_average_fast_size; // no power of two so no bitshifting
                    // store sample to buffer and advance index
                    multisample_buffer[multisample_counts] = feedback.cfd_recent;                                      // store new sample
                    multisample_counts                     = (multisample_counts + 1) & csense_sampler_buffer_size_n1; // advance ring index*/







                    
                    // If the new sample is larger than current peak, update peak
                    /*if( feedback.cfd_recent >= feedback.cfd_peak ){
                        feedback.cfd_peak        = feedback.cfd_recent; // New sample is peak
                        feedback.cfd_max_tracker = 0;                   // peak age = 0
                    } else {                                            
                        if( ++feedback.cfd_max_tracker > cfd_average_slow_size ){ // If peak age >= slow window, recompute
                            feedback.cfd_peak = 0;
                            work_index = multisample_counts; // scan backward through slow window
                            for( loop_index = 0; loop_index < cfd_average_slow_size; ++loop_index ){ // loop_index and work_index are local variables in dram
                                work_index = (work_index - 1) & csense_sampler_buffer_size_n1;
                                if (multisample_buffer[work_index] > feedback.cfd_peak) {
                                    feedback.cfd_peak = multisample_buffer[work_index];
                                    feedback.cfd_max_tracker = loop_index; // reset peak age
                                }
                            }
                        }
                    }*/







                    //############################################################################################################
                    //
                    // Preprocessing the collected data
                    //
                    //############################################################################################################
                    if( has_peak_sample ){                  // keep track of discharge count with current above or below threshold
                        adc_data.nodischarges_in_a_row = 0; // zero the no-discharge in a row count
                        ++adc_data.discharges_in_a_row;     // increment the discharge in a row count
                        ++adc_data.discharge_count;         // increment total discharge count
                    } else {
                        adc_data.discharges_in_a_row = 0;
                        ++adc_data.nodischarges_in_a_row;   // increment the no-discharge in a row count

                    }


                    //############################################################################################################
                    // Get the "slope" for the voltage change
                    //############################################################################################################
                    adc_data.vfd_slope = ( feedback.vfd_avg_slow > adc_jitter ) ? int( 
                        ( 90.0 / ( int ) feedback.vfd_avg_slow ) * 
                        ( ( int ) feedback.vfd_avg_fast - ( int ) feedback.vfd_avg_slow ) 
                    ) : 0;


                    micros_now = esp_timer_get_time();

                    //############################################################################################################
                    // Log the discharges for the timeframe and reset time start time is reset to 0 on initial reset. No need to 
                    // add extra logic for the first set. It happens so fast that it is not important if the first is not accurate
                    //############################################################################################################
                    if( micros_now - adc_data.micros_start > DISCHARGE_COUNT_TRIGGER ){     // 0.01s
                        discharge_count_previous        = adc_data.discharge_count_result;  // store the current count as previous
                        adc_data.discharge_count_result = adc_data.discharge_count;         // set the fresh count as current count
                        adc_data.discharge_count        = 0;                                // reset counter
                        adc_data.micros_start            = micros_now;                        // reset time
                        if( adc_data.discharge_count_result > discharge_count_max ){        // if count is larger then the max. update the max count
                            discharge_count_max = adc_data.discharge_count_result;
                        }
                    } 

                






                    //############################################################################################################
                    // 
                    // Constant feed lock until wave recovered
                    //
                    //############################################################################################################
                    if( !wavectrl.rise_block ){ // not in a rising wave block situation

                        if( has_peak_sample ){ // enter block
                            wavectrl.deadlock_count = 0;
                            wavectrl.rise_block     = true;
                            wavectrl.pre_rising_cfd = MIN( feedback.cfd_recent, feedback.cfd_recent_previous ) + adc_jitter;
                            wavectrl.cfd_peak       = feedback.cfd_recent;
                        }

                    } else { // in a rising wave block situation

                        // feed is locked until feedback recovers

                        if( feedback.cfd_recent > wavectrl.cfd_peak ){
                            wavectrl.cfd_peak = feedback.cfd_recent;
                            //wavectrl.cfd_peak_p = round( wavectrl.cfd_peak / 3 );
                            wavectrl.cfd_peak_p = wavectrl.cfd_peak >> 2;
                        }
                        
                        
                        if(

                            ( 
                                feedback.cfd_recent <= wavectrl.pre_rising_cfd 
                            ) || ( // prevent deadlocking
                                ++wavectrl.deadlock_count > 100 && feedback.cfd_recent < cfd_setpoint_min 
                            )

                        ){

                            // release
                            wavectrl.rise_block       = false; // exit block
                            wavectrl.deadlock_count   =
                            wavectrl.risings_in_a_row = 0;
                            
                        } else if( feedback.cfd_recent > wavectrl.cfd_peak_p ){
                            // arcs will oscillate upwards
                            ++wavectrl.risings_in_a_row;
                            wavectrl.deadlock_count = 0;
                        } else if( feedback.cfd_recent <= wavectrl.cfd_peak_p ) {
                            wavectrl.risings_in_a_row = 0;
                        }

                    }



                    //##########################################################################################################
                    //
                    // Data acquisition and preparation finished
                    //
                    //##########################################################################################################


                    //##########################################################################################################
                    //
                    // Error analysis
                    //
                    //##########################################################################################################
                    if(
                        full_short_circuit                          // pretty bad certified short circuit
                        || feedback.cfd_avg_slow > cfd_setpoint_max // slow average above max ( normally never happens )
                    ){

                        // real hard short circuit, very critical, wire can melt and or deform very very quick
                        sensor_errors = S_ERROR_SHORT;

                        if( 
                            sensor_state < SS_CORRECTING && 
                            feedback.cfd_avg_fast > feedback.vfd_pre_short_circuit 
                        ){
                            feedback.vfd_pre_short_circuit = feedback.cfd_avg_fast;
                        }

                    } else if(
                        ( 
                            feedback.vfd_pre_short_circuit > feedback.vfd_variable_thresh 
                            && feedback.cfd_avg_fast       > feedback.vfd_pre_short_circuit 
                        ) 
                        || feedback.cfd_avg_fast > cfd_setpoint_max // fast cfd average above setpoint max
                        //|| feedback.cfd_avg_slow > cfd_setpoint_mid
                    ){

                        // overload, critical error and precursor of real short circuits. A little fuzzy and heuristic
                        sensor_errors = S_ERROR_OVERLOAD;

                    } else if( 
                        //feedback.cfd_recent > cfd_setpoint_max ||
                        wavectrl.risings_in_a_row > ( sensor_state == SS_RAMP_UP ? 2 : 4 ) // failed to recove within given pulses
                    ){

                        // heursitic arc detection; arcs are bad; they make the surface ugly and create big heat on iny spots, wire can melt, normally followed by a real short circuit
                        sensor_errors = S_ERROR_ARC;
                        //wavectrl.risings_in_a_row = 0; // pwm gets turned off once; reset this counter to prevent further turn offs

                    } else { 

                        // no errors
                        sensor_errors = S_ERROR_NONE;

                    }


                    //##########################################################################################################
                    //
                    // Set the adjustable voltage threshold if safe; This thresholds resets after a pause and is generated on
                    // the fly if the feedback conditions are ok.
                    //
                    //##########################################################################################################
                    if( !sensor_errors && (
                            adc_data.nodischarges_in_a_row > 100 || (
                                !wavectrl.rise_block
                                && feedback.vfd_avg_slow > vfd_short_circuit_threshhold
                                && sensor_state < SS_CORRECTING
                                && feedback.cfd_peak < cfd_setpoint_mid 
                                && adc_data.nodischarges_in_a_row > 4
                           )
                        ) 
                    ){ feedback.vfd_variable_thresh = feedback.vfd_avg_slow; }





                    


                    //##########################################################################################################
                    //
                    // Feed/Hold/Retract/Release conditions
                    //
                    //##########################################################################################################

                    // Conditions to allow feed
                    feed_condition_met = (

                        // misc conditions
                        !sensor_errors
                        && !has_peak_sample
                        && !wavectrl.rise_block 
                        && !recovered_from_bail 



                        && no_water_contact || ( // apply only if water contact is detected
                               feedback.vfd_avg_slow >= feedback.vfd_variable_thresh - adc_jitter // 
                            && feedback.vfd_avg_slow <= feedback.vfd_variable_thresh + adc_jitter // maintain distance
                            && feedback.vfd_recent   <= feedback.vfd_variable_thresh              // ensure recent is cool with it
                            && feedback.vfd_avg_fast <= feedback.vfd_variable_thresh              // lock around setpoint
                        )

                        //&& adc_data.vfd_slope <= 1 
                        //&& adc_data.vfd_slope >= -1 




                        // discharge conditions
                        && adc_data.nodischarges_in_a_row  > 1 
                        && adc_data.discharge_count_result <= discharge_count_previous 

                        // current conditions
                        && feedback.cfd_peak      <= cfd_setpoint_max 
                        && feedback.cfd_avg_slow  <= cfd_setpoint_min 
                        && feedback.cfd_avg_slow  <= feedback.cfd_avg_slow_previous + adc_jitter 
                        && feedback.cfd_avg_fast  <= feedback.cfd_avg_fast_previous + adc_jitter 
                        && feedback.cfd_avg_fast  <= feedback.cfd_avg_slow          + adc_jitter 

                    ) ? true : false;
                
                    // Conditions to enter a correction retraction
                    retract_condition_met = ( 
                        sensor_errors > S_ERROR_ARC // arcs don't get retractions
                    ) ? true : false;
                
                    // Conditions to release from a retaction state 
                    retract_release_condition_met = ( 
                        sensor_errors == S_ERROR_NONE
                        && adc_data.nodischarges_in_a_row >  0
                        && feedback.cfd_avg_fast          <= feedback.cfd_avg_slow
                        && feedback.cfd_avg_slow          <=  feedback.cfd_avg_slow_previous
                        && feedback.cfd_avg_slow          <  cfd_setpoint_mid
                        //&& is_system_mode_edm()
                    ) ? true : false;

                    if( !retract_release_condition_met ){
                        release_confirmations = 0; // reset release counter
                    }


                    //##########################################################################################################
                    //
                    // Apply protection (PWM is turned off) if required
                    // Rapid response on errors
                    //
                    //##########################################################################################################
                    if( sensor_errors && is_system_mode_edm() ){

                        if( sensor_errors == S_ERROR_SHORT ){ 
                            sensor_state = SS_ERROR; // switch into error state instantly on hard short circuits
                            motion_plan_atomic.store( MOTION_PLAN_HARD_SHORT ); // instant update so the planner does not have to wait for the pwm return delay
                            new_motion_plan.store(true);
                            cv.notify_all();
                        } else if( sensor_errors == S_ERROR_OVERLOAD ){
                            motion_plan_atomic.store( MOTION_PLAN_SOFT_SHORT ); // instant update so the planner does not have to wait for the pwm return delay
                            new_motion_plan.store(true);
                            cv.notify_all();
                        }

                        // longer pwm off for real short circuit and short one for arcs and overload
                        arcgen.protection( 
                            sensor_errors == S_ERROR_SHORT 
                            ? pulse_off_duration 
                            : arc_pulse_off_duration_us 
                        );

                        bail_after_off_count = 3; // next x i2s batches will be ignored if required

                    }


                    recovered_from_bail = false; // unflag



                    //##########################################################################################################
                    //
                    // State specific decisions
                    //
                    //##########################################################################################################
                    switch ( sensor_state ){ // after a pause state is set to idle again in the reset section

                        //##########################################################################################################
                        // Seeking contact, less forward blocking, faster speed etc. Once contact is made it will switch to ramping
                        //##########################################################################################################
                        case SS_SEEK: // only happens until the first contact is etablished; seek until first spark is detected the switch to ramp up; it should have a least one tiny discharge now
                            
                            previous_state = SS_SEEK;

                            if( 
                                   feedback.cfd_recent   > cfd_setpoint_min
                                || feedback.cfd_avg_fast > cfd_setpoint_min 
                                || feedback.cfd_avg_slow > cfd_setpoint_min 
                            ){
                                first_seek_done  = true;
                                ramp_cycle_start = micros_now;
                                ramp_up_setpoint = 0;
                                sensor_state     = SS_RAMP_UP; // change state to ramp up
                                work_plan        = MOTION_PLAN_HOLD_HARD; // use hold hard to indicate touch for the planner; hold soft does the same motionwise but does not trigger the initial touch flag
                                for( int i = csense_sampler_buffer_size; i >= 0; --i ){
                                    if( multisample_buffer[i] < cfd_setpoint_min + adc_jitter ){
                                        multisample_buffer[i] = cfd_setpoint_min + adc_jitter;
                                    }
                                }
                                multisample_buffer[ multisample_counts ] = feedback.cfd_recent;
                                invert_plan = true;
                            } 

                        break;

                        //##########################################################################################################
                        // Ramp up phase
                        //##########################################################################################################
                        case SS_RAMP_UP:

                            previous_state = SS_RAMP_UP;

                            if( retract_condition_met && ++retract_confirm_counter >= retract_confirmations ){ 
                                work_plan    = MOTION_PLAN_SOFT_SHORT; // 
                                sensor_state = SS_CORRECTING;          // enter soft correction state
                            } else if( 
                                !feed_condition_met || 
                                feedback.cfd_avg_fast > ramp_up_setpoint || 
                                feedback.cfd_avg_slow > ramp_up_setpoint ||
                                feedback.cfd_recent   > ramp_up_setpoint
                            ){ work_plan = MOTION_PLAN_HOLD_HARD; } 

                            //##########################################################################################################
                            // Ramp up adjustments and exit
                            //##########################################################################################################
                            if( ramp_up_setpoint >= cfd_setpoint_min ){
                                // ramping done; change state and reduce ramping time for following ramps after short circuits
                                previous_state   = SS_ACTIVE;
                                sensor_state     = SS_ACTIVE;
                                ramp_up_interval = RAMP_DURATION_ACTIVE / cfd_setpoint_min; // reduce ramp time after the initial cold start ramp; it will reset to the intial ramp after pauses etc.
                            } else if( micros_now - ramp_cycle_start > ramp_up_interval ){
                                // ramp up setpoint
                                ++ramp_up_setpoint;
                                ramp_cycle_start = micros_now;
                            }

                        break;

                        //##########################################################################################################
                        // Active phase in normal operation
                        //##########################################################################################################
                        case SS_ACTIVE: // normal cutting phase; either move forward, hold or change state

                            previous_state = SS_ACTIVE;


                            if( adc_data.plan == MOTION_PLAN_FORWARD ){

                                work_plan = MOTION_PLAN_HOLD_SOFT; // skip every second forward in active state

                            }


                            if( retract_condition_met && ++retract_confirm_counter >= retract_confirmations ){ 

                                sensor_state = SS_CORRECTING;          // enter soft correction state
                                work_plan    = MOTION_PLAN_SOFT_SHORT; // 

                            } else if( !feed_condition_met ){

                                work_plan = ( 
                                    feedback.cfd_avg_slow >= cfd_setpoint_mid 
                                    || feedback.cfd_avg_fast > cfd_setpoint_mid 
                                ) ? MOTION_PLAN_HOLD_HARD : MOTION_PLAN_HOLD_SOFT;

                            } else if( adc_data.nodischarges_in_a_row > 200 ){

                                // switch to seek for faster moves
                                sensor_state = SS_SEEK;

                            }

                        break;


                        //###########################################################
                        // Retractions are evil and should be avoided at all cost if
                        // possible. Use them with care and exit retractions early
                        // They can make problems worse easily and will mess with
                        // the surface finish too
                        // release conditions are the same
                        //###########################################################
                        case SS_CORRECTING: // soft correction

                            if( retract_release_condition_met && ++release_confirmations > 2 ){ 
                                sensor_state = previous_state; 
                            }
                            work_plan = MOTION_PLAN_SOFT_SHORT; // 

                        break;

                        case SS_ERROR: // hard short circuit correction

                            if( retract_release_condition_met && ++release_confirmations > 4 ){ 
                                sensor_state = first_seek_done ? SS_RAMP_UP : previous_state; 
                            }
                            work_plan = MOTION_PLAN_HARD_SHORT; // 

                        break;

                        default:
                        break;

                    }

      
                    if( work_plan < MOTION_PLAN_HOLD_HARD ){
                        retract_confirm_counter = 0;
                    }
                

                }

                //###########################################################################
                // Store recent as previous for next iteration
                //###########################################################################
                feedback.cfd_avg_slow_previous = feedback.cfd_avg_slow;
                feedback.cfd_avg_fast_previous = feedback.cfd_avg_fast;
                feedback.cfd_recent_previous   = feedback.cfd_recent;
                feedback.vfd_recent_previous   = feedback.vfd_recent;
                feedback.vfd_avg_slow_previous = feedback.vfd_avg_slow;


                if( !is_system_mode_edm() ){
                    work_plan = MOTION_PLAN_FORWARD;
                } else if( adc_data.plan > work_plan && new_motion_plan.load() ){
                    // old plan was higher but not collected yet from the planner
                    // ensure at least one collection?
                    work_plan = adc_data.plan;
                }
            
                adc_data.plan = work_plan; // adc_data.plan is not allowed to be negative

                //###########################################################################
                // Distribute the motion plan
                //###########################################################################
                if( invert_plan ){                                                  // inverted plan is only for the planner to flag first contact
                    work_plan *= -1;                                                // only work_plan is allowed to be negative
                    if( motion_plan_atomic.load() < 0 && !new_motion_plan.load() ){ // previous plan was negative and already collected
                        invert_plan = false;                                        // unflag, planner already is notified of contact
                    } 
                }
    
                motion_plan_atomic.store( work_plan );
                new_motion_plan.store(true);
                cv.notify_all();


                if( !scope_use_high_res ){
                    adc_to_scope( scope_show_vfd_channel ? feedback.vfd_recent : feedback.cfd_recent, adc_data.plan ); 
                }

            }
        } 



    }
    vTaskDelete(NULL);
}






































//###########################################################################
// Oneshot timer to enter deep check in the sensor loop
//###########################################################################
static esp_timer_handle_t delay_timer = NULL;
void enter_deep_check_on_timer( void* args ){
    reset_sensor_state.store( true ); // set flag to enter deep check inside the sensor loop
}
void init_deep_check_timer(){
    const esp_timer_create_args_t timer_args = {
        .callback = &enter_deep_check_on_timer,
        .name     = "sense_oneshot_timer"
    };
    esp_timer_create( &timer_args, &delay_timer  );
}
void trigger_deep_check_timer( uint64_t delay_us ){
    if( delay_timer == NULL ){ init_deep_check_timer(); } // if timer was not created yet create it
    esp_timer_stop( delay_timer );                        // if time is active stop it
    esp_timer_start_once( delay_timer, delay_us );        // setup oneshot timer
}


void G_SENSORS::notify_change( setget_param_enum param_id ){

    switch ( param_id ){ // i2s requires a restart to apply those changes
        case PARAM_ID_I2S_RATE:
        case PARAM_ID_I2S_BUFFER_L:
        case PARAM_ID_I2S_BUFFER_C:
            restart_i2s_flag.store( true ); // flag to restart i2s
        break;
        default: break;
    }

    // Not very efficient right now but if a settings changed
    // we just fully refresh all settings and perform all required calculations etc
    // a little overweight now...
    sense_settings_changed.store( true );   // flag that a setting changed

    reset_sensor_state.store( true ); // set flag to enter deep check inside the sensor loop

    // the above flags will not force the sensor loop to enter deep check
    // in order for them to take effect the reset_sensor_state flag needs to be set to true
    // there are situations where this function is called multiple times in a row
    // to prevent heavy resetting overloads it uses a delayed timer to set the flag
    // if the timer is already running it will stop it and reset the delay 
    // so if for example a loop updates settings 10 times rapidly
    // the timer will reset 9 times and finally execute after the last one
    // in theory....
    // Seems using this timer interferes with the NVC writing. Can't tell for sure yet
    // but sometimes NVS gets bricked and I think it happens when this time interrupt fires 
    // while working with NVS. Error is not easy to repeat and I haven't seen it since 
    //trigger_deep_check_timer( 1000 ); // timeout in microseconds (us) 1000us = 1ms (millis)

};



//###########################################################################
// Create the settings when starting the sensor class
//###########################################################################
void G_SENSORS::init_settings(){

    add( PARAM_ID_SETMIN,             SETTING_TYPE_INT,  percentage_to_adc( DEFAULT_CFD_SETPOINT_MIN ), 0.0 );
    add( PARAM_ID_SETMAX,             SETTING_TYPE_INT,  percentage_to_adc( DEFAULT_CFD_SETPOINT_MAX ), 0.0 );
    add( PARAM_ID_FAST_CFD_AVG_SIZE,  SETTING_TYPE_INT,  DEFAULT_CFD_AVERAGE_FAST, 0.0 );
    add( PARAM_ID_SLOW_CFD_AVG_SIZE,  SETTING_TYPE_INT,  DEFAULT_CFD_AVERAGE_SLOW, 0.0 );
    add( PARAM_ID_RETRACT_CONF,       SETTING_TYPE_INT,  RETRACT_CONFIRMATIONS,    0.0 );
    add( PARAM_ID_VDROP_THRESH,       SETTING_TYPE_INT,  DEFAULT_VFD_SHORT_THRESH, 0.0 );
    add( PARAM_ID_VFD_THRESH,         SETTING_TYPE_INT,  DEFAULT_VFD_THRESH,       0.0 );
    add( PARAM_ID_POFF_DURATION,      SETTING_TYPE_INT,  DEFAULT_POFF_DURATION,    0.0 );
    add( PARAM_ID_EDGE_THRESH,        SETTING_TYPE_INT,  DEFAULT_EDGE_THRESHOLD,   0.0 );

    // probing settings
    add( PARAM_ID_PROBE_TR_V,         SETTING_TYPE_INT,  90, 0.0 );
    add( PARAM_ID_PROBE_TR_C,         SETTING_TYPE_INT,  percentage_to_adc( DEFAULT_CFD_SETPOINT_PROBE ), 0.0 );
    // scope related settings
    add( PARAM_ID_USE_HIGH_RES_SCOPE, SETTING_TYPE_BOOL, DEFAULT_SCOPE_USE_HIGH_RES?1:0, 0.0 );
    add( PARAM_ID_SCOPE_SHOW_VFD,     SETTING_TYPE_BOOL, 0, 0.0 );
    // I2S settings
    add( PARAM_ID_I2S_RATE,           SETTING_TYPE_INT,  I2S_SAMPLE_RATE, 0.0 );
    add( PARAM_ID_I2S_BUFFER_L,       SETTING_TYPE_INT,  I2S_NUM_SAMPLES, 0.0 );
    add( PARAM_ID_I2S_BUFFER_C,       SETTING_TYPE_INT,  I2S_BUFF_COUNT,  0.0 );

    sense_settings_changed.store( true );
    restart_i2s_flag.store( false ); // i2s not started yet

    refresh_settings(); // copy the initial settings 

}


//###########################################################################
// Reload all settings on runtime and if needed restart i2s 
// Once the sensor loop enters deep check if reset_sensor_state flag is set
// it will also call this function to check for other flags like 
// reloading settings or restarting i2s
//###########################################################################
bool G_SENSORS::refresh_settings(){

    if( sense_settings_changed.load() ){
        sense_settings_changed.store( false ); // unset flag 

        // load the settings from the settings container
        scope_use_high_res           = get_setting_bool( PARAM_ID_USE_HIGH_RES_SCOPE );
        scope_show_vfd_channel       = get_setting_bool( PARAM_ID_SCOPE_SHOW_VFD );
        cfd_setpoint_min             = get_setting_int( PARAM_ID_SETMIN );
        cfd_setpoint_max             = get_setting_int( PARAM_ID_SETMAX );
        cfd_setpoint_probing         = get_setting_int( PARAM_ID_PROBE_TR_C );
        cfd_average_fast_size        = get_setting_int( PARAM_ID_FAST_CFD_AVG_SIZE );
        cfd_average_slow_size        = get_setting_int( PARAM_ID_SLOW_CFD_AVG_SIZE );
        retract_confirmations        = get_setting_int( PARAM_ID_RETRACT_CONF );
        vfd_short_circuit_threshhold = get_setting_int( PARAM_ID_VDROP_THRESH );
        vfd_forward_thresh           = get_setting_int( PARAM_ID_VFD_THRESH );
        pulse_off_duration           = get_setting_int( PARAM_ID_POFF_DURATION );
        cfd_ignition_treshhold       = get_setting_int( PARAM_ID_EDGE_THRESH );
        vfd_probe_trigger            = get_setting_int( PARAM_ID_PROBE_TR_V );
        sample_rate                  = get_setting_int( PARAM_ID_I2S_RATE );
        i2s_buffer_length            = get_setting_int( PARAM_ID_I2S_BUFFER_L );
        buffer_count                 = get_setting_int( PARAM_ID_I2S_BUFFER_C );

        // postprocessing some stuff
        i2s_num_bytes    = sizeof(uint16_t) * i2s_buffer_length;
        cfd_setpoint_mid = cfd_setpoint_min + ( ( cfd_setpoint_max - cfd_setpoint_min ) >> 1 );

    }

    if( restart_i2s_flag.load() ){ // settings are fresh now; check for i2s restart requirenments
        restart_i2s_flag.store( false );
        vTaskDelay(40);
        restart();
        vTaskDelay(40); 
        return true;
    }

    return false;

}





//###################################################
// Start tasks
//###################################################
void G_SENSORS::create_tasks(){
    xTaskCreatePinnedToCore( remote_control_task, "remote_control_task_handle", STACK_SIZE_A, this, 1,                             &remote_control_task_handle, I2S_TASK_A_CORE);
    xTaskCreatePinnedToCore( adc_monitor_task,    "adc_monitor_task",           STACK_SIZE_B, this, TASK_VSENSE_RECEIVER_PRIORITY, &adc_monitor_task_handle,    I2S_TASK_B_CORE); 
    while( !adc_monitor_task_running.load() ) vTaskDelay(10);
}

void G_SENSORS::setup(){
    vTaskDelay(DEBUG_LOG_DELAY);
    detachInterrupt( digitalPinToInterrupt( ON_OFF_SWITCH_PIN ) );
    detachInterrupt( digitalPinToInterrupt( STEPPERS_LIMIT_ALL_PIN ) );
    attachInterrupt( ON_OFF_SWITCH_PIN,      motion_switch_on_interrupt, CHANGE );
    attachInterrupt( STEPPERS_LIMIT_ALL_PIN, limit_switch_on_interrupt,  CHANGE );
}