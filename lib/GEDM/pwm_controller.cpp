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


//############################################################################################################
/*



    GPIO_REG_READ(GPIO_IN_REG)
    Reads the current value of the GPIO input register. This register contains the logic levels of all GPIO pins.

    GEDM_PWM_PIN & 0x1F
    Masks the GEDM_PWM_PIN with 0x1F (which is 31 in decimal). This effectively extracts the lower 5 bits of GEDM_PWM_PIN, assuming GPIO pin numbers are within 0-31.

    (gpio_num_t)(GEDM_PWM_PIN & 0x1F)
    Casts the masked value to the gpio_num_t type, which is typically an enum or typedef representing GPIO pin numbers.

    GPIO_REG_READ(GPIO_IN_REG) >> (gpio_num_t)(GEDM_PWM_PIN & 0x1F)
    Shifts the register value right by the pin number, moving the target GPIO pin's bit to the least significant bit position.

    & 1U
    Masks out all bits except the least significant bit, effectively reading the state (0 or 1) of that specific GPIO pin.


*/
//############################################################################################################
#include "pwm_controller.h"
#include "sensors.h"
#include "gedm_dpm_driver.h"
#include "ili9341_tft.h"
#include <freertos/portmacro.h>
#include <esp32-hal-gpio.h>
#include <driver/ledc.h>
#include <soc/ledc_periph.h>
#include <soc/ledc_reg.h>
#include <soc/ledc_struct.h>

#define USE_HW_TIMER_LEDC

enum arc_gen {
    ARC_OK = 0,
    ARC_CREATE
};

std::map<arc_gen, const char*> arc_messages = {
    { ARC_OK,     "" },
    { ARC_CREATE, "Creating arc generator" }
};

static DRAM_ATTR std::mutex arc_mtx; // the protection function is called inside the i2s loop and needs to execute as fast as possible. Only reason to put the mutex into ram     


constexpr ledc_timer_t     timer_sel              = LEDC_TIMER_0;
constexpr ledc_timer_bit_t timer_bit_width        = LEDC_TIMER_8_BIT;
constexpr int              max_duty_int           = ( ( 1 << timer_bit_width ) - 1 ); // 8Bit range: 0-255
volatile  bool             lock_reference_voltage = false;


std::atomic<int>  pwm_period_us(0);
std::atomic<bool> timer_active(false);
std::atomic<bool> spark_generator_is_running_flag(false);

DRAM_ATTR std::atomic<bool> arc_generator_pwm_off_flag(false);
DRAM_ATTR std::atomic<bool> stop_sampling(false);

ARC_GENERATOR arcgen;


//####################################################################
// Timer interrupt that will unlock the I2S sampling task
//####################################################################
IRAM_ATTR void adc_on_timer(){

    static DRAM_ATTR bool timer_triggered = false;
    static DRAM_ATTR int data             = SENSE_LOAD_SAMPLES;

    if( 
        timer_triggered || 
        stop_sampling.load() ||
        xQueueIsQueueEmptyFromISR( adc_read_trigger_queue ) == pdFALSE
    ){ return; }

    timer_triggered = true;
    xQueueOverwriteFromISR(adc_read_trigger_queue, &data, NULL);
    timer_triggered = false;

}


#ifdef USE_HW_TIMER_LEDC

hw_timer_t *adc_timer = nullptr;

constexpr int MIN_INTERVAL_US = 40; // 50=20khz 100=10khz

IRAM_ATTR void timer_set_speed( int frequency = 0 ){
    static int timer_speed_current = 0;
    frequency = ( frequency > 0 ) ? ( 1000000 / ( frequency ) ) - 1 : MIN_INTERVAL_US;
    if( frequency < MIN_INTERVAL_US ){ frequency = MIN_INTERVAL_US; }
    if( timer_speed_current == frequency ) return; // nothing to do
    timer_speed_current = frequency;
    timerAlarmWrite(adc_timer, frequency, true);
}
void end_adc_timer( int frequency = 0 ){
    if( adc_timer ){
        timerDetachInterrupt( adc_timer );
        timerEnd( adc_timer );
        adc_timer = nullptr;
    }
}
void create_adc_timer( int frequency = 0 ){
    if( adc_timer ) return;
    frequency = ( frequency > 0 ) ? ( 1000000 / ( frequency ) ) - 1 : MIN_INTERVAL_US;
    if( frequency < MIN_INTERVAL_US ){
        frequency = MIN_INTERVAL_US;
    }
    adc_timer = timerBegin(3, 80, true);
    timerAttachInterrupt(adc_timer, &adc_on_timer, true);
    timer_set_speed( frequency );
}
void resume_adc_timer( int frequency = 0 ){
    if( timer_active.load() ) return;
    timer_set_speed( frequency );
    timerAlarmEnable(adc_timer);
    timer_active.store( true );
}
void pause_adc_timer(){
    if( !timer_active.load() ) return;
    timerAlarmDisable( adc_timer );
    timer_active.store( false );
}

#endif





IRAM_ATTR bool ARC_GENERATOR::lock( void ){ // not really a lock. It just sets a flag to stop processing samples in the sensor task; reduces load while doing important things
    if( stop_sampling.load( std::memory_order_acquire ) ) return false;
    stop_sampling.store( true, std::memory_order_acquire );
    return true;
}
IRAM_ATTR void ARC_GENERATOR::unlock( bool lock_taken ){
    if( !lock_taken ) return;
    stop_sampling.store( false, std::memory_order_release );
}












ARC_GENERATOR::ARC_GENERATOR() : 
    pwm_duty_probing( DEFAULT_PROBING_DUTY ),
    pwm_frequency_probing( DEFAULT_PROBING_FREQUENCY ),
    pwm_duty_cycle_percent( DEFAULT_PWM_DUTY ), 
    duty_percent_backup( DEFAULT_PWM_DUTY ),
    pwm_duty_intern( 1 ), // ledc seems to need a little duty on init
    pwm_frequency_intern( DEFAULT_PWM_FREQUENCY ), 
    frequency_backup( DEFAULT_PWM_FREQUENCY ),
    pwm_pin( GEDM_PWM_PIN ),
    has_isr( false ){};

ARC_GENERATOR::~ARC_GENERATOR(){}

void ARC_GENERATOR::end(){
    pwm_off();
    #ifdef USE_HW_TIMER_LEDC
        end_adc_timer();
    #endif
    stop_ledc();
}


void ARC_GENERATOR::create(){
    debuglog( arc_messages[ARC_CREATE] );
    pinMode( pwm_pin, OUTPUT );
    digitalWrite( pwm_pin, LOW );
    start_ledc();
    change_pwm_frequency( get_freq() );
    update_duty( get_duty_percent() );
    #ifdef USE_HW_TIMER_LEDC
        create_adc_timer( pwm_frequency_intern );
    #endif
    disable_spark_generator();
}




//####################################################################
// Check if spark generator is running
//####################################################################
bool ARC_GENERATOR::is_running(){
    return spark_generator_is_running_flag.load();
}

//####################################################################
// Check if PWM is on or off. Doesn't check if the arc generator is 
// running. Just if the PWM is currently running.
//####################################################################
bool IRAM_ATTR ARC_GENERATOR::get_pwm_is_off(){
    return arc_generator_pwm_off_flag.load();
}

//####################################################################
// Get the PWM frequency in Hz
//####################################################################
int ARC_GENERATOR::get_freq(){
    std::lock_guard<std::mutex> lock( arc_mtx );
    return pwm_frequency_intern;
}

//####################################################################
// Get the PWM duty cycle in percent (0-100%)
//####################################################################
float ARC_GENERATOR::get_duty_percent(){
    std::lock_guard<std::mutex> lock( arc_mtx );
    return pwm_duty_cycle_percent;

}

//####################################################################
// Get the PWM duty cycle in 8Bit int 0-255
//####################################################################
int ARC_GENERATOR::get_duty_internal(){
    std::lock_guard<std::mutex> lock( arc_mtx );
    return pwm_duty_intern;

}

//####################################################################
// Get the pulse period in microseconds (us)
//####################################################################
int ARC_GENERATOR::get_pulse_period_us(){
    return pwm_period_us.load( std::memory_order_acquire );
}

//####################################################################
// Fastly turn PWM off without disabling the arc generator
//####################################################################
void ARC_GENERATOR::pwm_off(){
    change_pwm_duty( 0 );
    {
        std::lock_guard<std::mutex> lock( arc_mtx );
        if( has_isr ){
            #ifdef USE_HW_TIMER_LEDC
                pause_adc_timer();
            #else
                detachInterrupt( pwm_pin );
            #endif
            has_isr = false;
        }
        LEDC.channel_group[ LEDC_HIGH_SPEED_MODE ].channel[ LEDC_CHANNEL_0 ].conf0.sig_out_en = 0;
    }
    arc_generator_pwm_off_flag.store( true );
}

//####################################################################
// Fastly turn PWM back on without affecting the arc generator state
//####################################################################
void ARC_GENERATOR::pwm_on(){
    arc_generator_pwm_off_flag.store( false );
    change_pwm_frequency( get_freq() );
    change_pwm_duty( pwm_duty_intern );
    std::lock_guard<std::mutex> lock( arc_mtx );
    if( !has_isr ){
        #ifdef USE_HW_TIMER_LEDC
            resume_adc_timer( pwm_frequency_intern ); // 20000 / 1000 = 20 10000 / 1000 = 10
        #else
            attachInterrupt(pwm_pin, adc_on_timer, RISING);
        #endif
        has_isr = true;
    }
    LEDC.channel_group[ LEDC_HIGH_SPEED_MODE ].channel[ LEDC_CHANNEL_0 ].conf0.sig_out_en = 1;
}

//####################################################################
// Turn the arc generator OFF
//####################################################################
void ARC_GENERATOR::disable_spark_generator(){
    pwm_off();
    spark_generator_is_running_flag.store(false);
}

//####################################################################
// Turn the arc generator ON
//####################################################################
void ARC_GENERATOR::enable_spark_generator(){
    spark_generator_is_running_flag.store(true); // needs to be set first else duty will be set to zero
    pwm_on();
    spark_generator_is_running_flag.store(true); // ???
}

void ARC_GENERATOR::stop_ledc( void ){
    std::lock_guard<std::mutex> lock( arc_mtx );
    ledc_stop( LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0 );
    digitalWrite( pwm_pin, LOW );
}

void ARC_GENERATOR::start_ledc( void ){

    std::lock_guard<std::mutex> lock( arc_mtx );
    vTaskDelay(10);
    ledc_timer_config_t   ledc_timer;        
    ledc_channel_config_t ledc_conf;

    ledc_timer.speed_mode      = LEDC_HIGH_SPEED_MODE;
    ledc_timer.timer_num       = timer_sel;
    ledc_timer.duty_resolution = timer_bit_width;
    ledc_timer.freq_hz         = pwm_frequency_intern;
    ledc_timer.clk_cfg         = LEDC_AUTO_CLK;

    ledc_conf.channel    = LEDC_CHANNEL_0;
    ledc_conf.duty       = pwm_duty_intern;
    ledc_conf.gpio_num   = pwm_pin;
    ledc_conf.hpoint     = 0;
    ledc_conf.intr_type  = LEDC_INTR_DISABLE;
    ledc_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_conf.timer_sel  = timer_sel;

    ledc_timer_config( &ledc_timer );
    ledc_channel_config( &ledc_conf );

}

void IRAM_ATTR ARC_GENERATOR::protection( int delay_us ){ // shut arc off for some time quickly
    if( arc_generator_pwm_off_flag.load() ) return;
    std::lock_guard<std::mutex> lock( arc_mtx );
    LEDC.channel_group[ LEDC_HIGH_SPEED_MODE ].channel[ LEDC_CHANNEL_0 ].conf0.sig_out_en = 0;
    delayMicroseconds( delay_us );
    if( arc_generator_pwm_off_flag.load() ) return;
    LEDC.channel_group[ LEDC_HIGH_SPEED_MODE ].channel[ LEDC_CHANNEL_0 ].conf0.sig_out_en = 1;
}

//####################################################################
// Change the duty etc. This is only called from within this 
// class to finally apply the new duty. Don't call it directly.
// duty is integer 0-255 (8Bit)
//####################################################################
void ARC_GENERATOR::change_pwm_duty( int duty ){
    if( is_system_mode_edm() && get_pwm_is_off() ){
        // gets here if value is changed in the process screen
        // Prevents the new duty from getting set after updating
        // required to keep PWM turned off until process is resumed
        if( duty == 0 ){
            digitalWrite( pwm_pin, LOW );
        }
        return;
    }

    std::lock_guard<std::mutex> lock( arc_mtx );
    if( !spark_generator_is_running_flag.load() && !lock_reference_voltage ){ 
        duty = 0; // force LEDC to go low
    }
    ledc_set_duty( LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty );
    ledc_update_duty( LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0 );
    if( duty == 0 ){
        digitalWrite( pwm_pin, LOW );
    }
}
//####################################################################
// Update the PWM duty cycle and change the duty
// duty in percent. This function is used to externally set a new duty
// the change_pwm_duty is also called to turn pwm on/off without
// updating the internal stored duty
//####################################################################
void ARC_GENERATOR::update_duty( float duty_percent ){
    {
        std::lock_guard<std::mutex> lock( arc_mtx );
        pwm_duty_cycle_percent = clamp_float( duty_percent, 0.0 , PWM_DUTY_MAX );
        pwm_duty_intern        = pwm_duty_cycle_percent > 0.0 ? round( ( pwm_duty_cycle_percent * max_duty_int ) / 100 ) : 0;
    }
    recalculate_meta_data();
    change_pwm_duty( get_duty_internal() );
}



//####################################################################
// 
//####################################################################
void ARC_GENERATOR::recalculate_meta_data(){   
    pwm_period_us.store( int( round( 1.0 / float( pwm_frequency_intern ) * 1000.0 * 1000.0 ) ), std::memory_order_acquire );
}


//####################################################################
// Change the PWM frequency (Hz)
//####################################################################
void ARC_GENERATOR::change_pwm_frequency( int freq ){

    {
        std::lock_guard<std::mutex> lock( arc_mtx );
        pwm_frequency_intern = clamp_int( freq, PWM_FREQUENCY_MIN , PWM_FREQUENCY_MAX );
        #ifdef USE_HW_TIMER_LEDC
        if( adc_timer ){
            timer_set_speed( pwm_frequency_intern );
        }
        #endif
    }

    recalculate_meta_data();

    if( is_system_mode_edm() && get_pwm_is_off() ){
        // gets here if value is changed in the process screen
        // don't apply until process is resumed
        return;
    }

    std::lock_guard<std::mutex> lock( arc_mtx );
    ledc_set_freq( LEDC_HIGH_SPEED_MODE, timer_sel, pwm_frequency_intern );

}




//####################################################################
// Enable probe mode. It will adjust PWM frequency and duty
// and also change the DPM voltage and current and turn it on if
// DPM support is enabled. The current settings are saved
// and restored after probing
//####################################################################
void ARC_GENERATOR::probe_mode_on(){

    {
        std::lock_guard<std::mutex> lock( arc_mtx );
        lock_reference_voltage = true;
        duty_percent_backup    = pwm_duty_cycle_percent; 
        frequency_backup       = pwm_frequency_intern;
    }
    pwm_on();                                      // ledc lock inside
    change_pwm_frequency( pwm_frequency_probing ); // ledc lock inside
    update_duty( pwm_duty_probing );               // ledc lock inside
    vTaskDelay(300);
}
//#####################################################################
// Restore the backup settings after probing
// restores PWM frequency, duty and set lock_reference_voltage to false
// then turns pwm_off without disabling the arc gen
//#####################################################################
void ARC_GENERATOR::probe_mode_off(){

    {
        std::lock_guard<std::mutex> lock( arc_mtx );
        lock_reference_voltage = false;
    }

    change_pwm_frequency( frequency_backup ); 
    update_duty( duty_percent_backup );
    pwm_off();
}



bool ARC_GENERATOR::change_setting( setget_param_enum param_id, int int_value, float float_value ){
    bool restart_i2s_req = false;
    switch( param_id ){
        case PARAM_ID_PROBE_DUTY: 
            {
                std::lock_guard<std::mutex> lock( arc_mtx );
                pwm_duty_probing = float_value;
            }
        break;
        case PARAM_ID_PROBE_FREQ: 
            {
                std::lock_guard<std::mutex> lock( arc_mtx );
                pwm_frequency_probing = int_value;
            }
        break;
        case PARAM_ID_FREQ: change_pwm_frequency( int_value ); break;
        case PARAM_ID_DUTY: update_duty( float_value );        break;
        case PARAM_ID_PWM_STATE: ( int_value == 1 ? enable_spark_generator() : disable_spark_generator() ); enforce_redraw.store( true ); break;
        default: break; 
    }
    vTaskDelay(50);
    return true;
}
int ARC_GENERATOR::get_setting_int( setget_param_enum param_id ){
    int value = 0;
    switch( param_id ){
        case PARAM_ID_FREQ: value = get_freq(); break;
        case PARAM_ID_PROBE_FREQ: 
            {
                std::lock_guard<std::mutex> lock( arc_mtx );
                value = pwm_frequency_probing;
            }
        break;

        default: break;
    }
    return value;
}
float ARC_GENERATOR::get_setting_float( setget_param_enum param_id ){
    float value = 0;
    switch( param_id ){
        case PARAM_ID_DUTY: value = get_duty_percent(); break;
        case PARAM_ID_PROBE_DUTY: 
            {
                std::lock_guard<std::mutex> lock( arc_mtx );
                value = pwm_duty_probing;
            }
        break;
        default: break;
    }
    return value;
}
bool ARC_GENERATOR::get_setting_bool( setget_param_enum param_id ){
    bool value = false;
    switch( param_id ){
        case PARAM_ID_PWM_STATE:  value = spark_generator_is_running_flag.load(); break;
        default: break;
    }
    return value;
}