#pragma once

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


#ifndef GEDM_DEFS
#define GEDM_DEFS

#include <stdint.h>
#include "esp_attr.h"
#include "definitions_common.h"
#include "gaxis.h"
#include <atomic>
#include "state_engine.h"


#define MOTION_PLAN_PROBE_CONFIRMATIONS 5 
#define STEP_PULSE_DELAY                0
#define PIN_NONE                        255

//#######################################################################################################################
// Pinout
// (The display pinout is not included here. The TFT pins are configured within the platform.io file)
// The complete Pinout can be found in here:
// /build-tutorial/ESP32-Pinout.txt
// Note: vSense uses ADC Channels 
// So changing the Pin here won't work. The ADC channel needs to be changed 
// A list with the GPIOs and the corresponding ADC channel can be found here:
// https://github.com/espressif/esp-idf/blob/e9d442d2b72/components/soc/esp32/include/soc/adc_channel.h
//#######################################################################################################################
#define ON_OFF_SWITCH_PIN        36
#define VOLTAGE_SENSE_PIN        35
#define CURRENT_SENSE_PIN        34
#define GEDM_PWM_PIN             22 // PWM for the pulsegenerator
#define STEPPERS_DISABLE_PIN     GPIO_NUM_32 // one disable pin for all
#define STEPPERS_LIMIT_ALL_PIN   GPIO_NUM_39 // one limit pin for all
#define CURRENT_SENSE_CHANNEL    ADC1_CHANNEL_6
#define VOLTAGE_SENSE_CHANNEL    ADC1_CHANNEL_7

/*
#define GEDM_PIN_MISO  19 // T_DO, SD_MISO, SDO<MISO>
#define GEDM_PIN_MOSI  23 // T_DIN, SD_MOSI, SDI<MOSI>
#define GEDM_PIN_SCLK  18 // T_CLK, SD_SCK, SCK
#define GEDM_PIN_CS    5  //
#define GEDM_PIN_RST   4
#define GEDM_PIN_T_CS  21
#define GEDM_PIN_DC    15
#define GEDM_PIN_SD_CS 2  // 
*/

//#######################################################################################################################
// Homing
//#######################################################################################################################
#define HOMING_PULLOFF     3.0 // mm
#define HOMING_DIR_MASK    0   // homing all axis into positive direction / somehow deprecated but still in use

#define SD_ROOT_FOLDER     "/phantom"
#define SD_SETTINGS_FOLDER "/phantom/settings"
#define SD_GCODE_FOLDER    "/phantom/gcode"

//#######################################################################################################################
// Create the pinout for the EVOII / EVOIII Motionboard
//#######################################################################################################################
// EVOIII Motionboard ha XYZA drivers
// On XYUV the connections are
// XY = XY; U = Z, V = A

//#########################################################################
// X
//#########################################################################
#define X_MAX_TRAVEL    X_TRAVEL // mm NOTE: Must be a positive value.
#define X_STEP_PIN      GPIO_NUM_27
#define X_DIRECTION_PIN GPIO_NUM_26
//#########################################################################
// Y
//#########################################################################
#define Y_MAX_TRAVEL    Y_TRAVEL // mm NOTE: Must be a positive value.
#define Y_STEP_PIN      GPIO_NUM_13
#define Y_DIRECTION_PIN GPIO_NUM_14

//#########################################################################
// Z only available for Z / XYZ
//#########################################################################
#if defined( CNC_TYPE_Z ) || defined( CNC_TYPE_XYZ )
    #define Z_MAX_TRAVEL    Z_TRAVEL // mm NOTE: Must be a positive value.
    #define Z_STEP_PIN      GPIO_NUM_25
    #define Z_DIRECTION_PIN GPIO_NUM_33
#endif


#if defined( CNC_TYPE_XYUV ) || defined( CNC_TYPE_XYUVZ )
    //################################################
    // U
    //################################################
    #define U_MAX_TRAVEL    U_TRAVEL // mm NOTE: Must be a positive value.
    #define U_STEP_PIN      GPIO_NUM_25
    #define U_DIRECTION_PIN GPIO_NUM_33
    //################################################
    // V
    //################################################
    #define V_MAX_TRAVEL    V_TRAVEL // mm NOTE: Must be a positive value.
    #define V_STEP_PIN      GPIO_NUM_12
    #define V_DIRECTION_PIN GPIO_NUM_17

    #ifdef ENABLE_PWM_PULLING_MOTOR // no pwm pulling motor for xyuv/z
        #undef ENABLE_PWM_PULLING_MOTOR
    #endif

#endif


//#########################################################################
// XYUVZ is not supported yet. Create invalid Z ghost axis
//#########################################################################
#if defined( CNC_TYPE_XYUVZ )
    // not supported yet. Make the pin config invalid
    // the motor will be listed but it will be a ghost axis
    #define Z_MAX_TRAVEL    Z_TRAVEL // mm NOTE: Must be a positive value.
    #define Z_STEP_PIN      PIN_NONE
    #define Z_DIRECTION_PIN PIN_NONE
#endif



//#############################################################################
//  __      __.__                                    .___    .__          
// /  \    /  \__|______   ____     _____   ____   __| _/_ __|  |   ____  
// \   \/\/   /  \_  __ \_/ __ \   /     \ /  _ \ / __ |  |  \  | _/ __ \ 
//  \        /|  ||  | \/\  ___/  |  Y Y  (  <_> ) /_/ |  |  /  |_\  ___/ 
//   \__/\  / |__||__|    \___  > |__|_|  /\____/\____ |____/|____/\___  >
//        \/                  \/        \/            \/               \/ 
//
// PWM driven A axis to pull the wire is only available for Z / XY /XYZ
// routers at the moment. Only 4 TMC drivers on the board
//#############################################################################
#if defined( CNC_TYPE_Z ) || defined( CNC_TYPE_XY ) || defined( CNC_TYPE_XYZ )
    //#######################################
    // A / Spindle (PWM) controlled
    // Not a real axis. Not possible for XYUV
    //#######################################
    #define A_STEP_PIN              12
    #define A_DIRECTION_PIN         17
#else
    #define A_STEP_PIN      PIN_NONE
    #define A_DIRECTION_PIN PIN_NONE
#endif



#ifdef CNC_TYPE_XYUV
    const int N_AXIS = 4;
#elif defined( CNC_TYPE_XYUVZ )
    const int N_AXIS = 5;
#elif defined( CNC_TYPE_XY )
    const int N_AXIS = 2;
#elif defined( CNC_TYPE_Z )
    const int N_AXIS = 1;
#else 
    const int N_AXIS = 3; // defaults to XYZ
#endif



// MCPWMs bottom limit for the frequency. Super slow. Don't change.
#define WIRE_MIN_SPEED_HZ   16 
#define INVERT_ST_ENABLE    0
#define PWM_FREQUENCY_MAX   100000 // never go that high. It is untested. Won't destroy anything but may not work with the code very well. Further testing needed.
#define PWM_FREQUENCY_MIN   2000   // same as above. The PWM function can only get as low as 1000. in theory it goes lower but throws errors below 1000. Still seems to work if lower.
#define PWM_DUTY_MAX        80.0  // 100% should never be used. Maximum 80% to give a small off time.
#define LOADER_DELAY        2000000
#define STACK_SIZE_A        2400 //1024
#define STACK_SIZE_B        4000 //3284
#define STACK_SIZE_SCOPE    1800 //1272
#define UI_TASK_STACK_SIZE  7000
#define UI_TASK_CORE        0
#define I2S_TASK_A_CORE     0
#define I2S_TASK_B_CORE     0
#define DEBUG_LOG_DELAY     5
#define DEBUG_LOG_ERROR_DELAY 2000


#define TASK_UI_PRIORITY              3 // the user interface task
#define TASK_PROTOCOL_PRIORITY        1 // protocol loop task on core 1
#define TASK_SENSORS_DEFAULT_PRIORITY 4 // default sensor task. Currently just checking for an of/off switch event to debounce the switch
#define TASK_VSENSE_RECEIVER_PRIORITY 1 // This task does the actual sampling after it received a request



#define DEFAULT_LINE_BUFFER_SIZE        256
#define DEFAULT_PWM_FREQUENCY           40000
#define DEFAULT_PWM_DUTY                8.0

#define DEFAULT_OPERATION_MODE          4
#define DEFAULT_REAMER_TRAVEL           10.0
#define DEFAULT_REAMER_DURATION         0.0
#define DEFAULT_TOOL_DIAMETER           0.0
#define DEFAULT_PROBING_FREQUENCY       40000
#define DEFAULT_PROBING_DUTY            20.0
#define DEFAULT_SINKER_TRAVEL_MM        10.0
#define DEFAULT_ENABLE_DPM              true
#define DEFAULT_DPM_VOLTAGE_PROBE       15.0
#define DEFAULT_DPM_CURRENT_PROBE       0.2
#define DEFAULT_HOME_ALL_WITH_Z         false 

#define DEFAULT_WIRE_SPEED_MM_MIN       600.0
#define DEFAULT_WIRE_SPEED_PROBING      50.0

#define DEFAULT_CFD_SETPOINT_MIN        2.0
#define DEFAULT_CFD_SETPOINT_MAX        36.0
#define DEFAULT_CFD_SETPOINT_PROBE      6.0
#define DEFAULT_VFD_SHORT_THRESH        2400
#define DEFAULT_SCOPE_USE_HIGH_RES      false

#define DEFAULT_CFD_AVERAGE_FAST        4
#define DEFAULT_CFD_AVERAGE_SLOW        30
#define DEFAULT_CFD_AVERAGE_RISE_BOOST  1

#define DEFAULT_CFD_SP_MD  0.7
#define DEFAULT_CFD_SP_FD  0.35

#define DEFAULT_EDGE_THRESHOLD          4

#define DEFAULT_VFD_THRESH              4000
#define DEFAULT_VFD_SHORTS_TO_POFF      2
#define DEFAULT_POFF_DURATION           2
#define DEFAULT_PWM_OFF_AFTER           0
#define DEFAULT_ENABLE_EARLY_EXIT       true
#define DEFAULT_EARLY_EXIT_ON_PLAN      1
#define DEFAULT_MAX_REVERSE_LINES       1
#define RETRACT_CONFIRMATIONS           2
#define DEFAULT_RESET_SENSE_QUEUE       true
#define DEFAULT_LINE_END_CONFIRMATIONS  4

#define DEFAULT_SHORTCIRCUIT_MAX_DURATION_US 500000

#define DPMDPH_MAX_CURRENT 6.0
#define DPMDPH_MAX_VOLTAGE 80.0


// the below settings highly depends on the machine and axis backlash
typedef struct retraction_distances {
  float soft_retraction      = 0.001; // mm
  float medium_retraction    = 0.002; // mm
  float hard_retraction      = 0.01;  // mm
  float wire_break_distance  = 1.0;   // (mm) this not only sets the no-load travel until a pause is triggered but is shared with the retraction to. If the machine moves without load this distance or retracts this distance it will pause
  bool  early_exit           = DEFAULT_ENABLE_EARLY_EXIT;  // if true it will allow to exit a retraction without moving the full distance
} retraction_distances;


typedef struct { // Don't change this. It get's overwritten and holds the calculated speeds as step delay timing 
    int RAPID             = 80;
    int INITIAL           = 150;
    int EDM               = 150;
    int PROBING           = 800;
    int REPROBE           = 250;
    int RETRACT           = 150;
    int HOMING_FEED       = 400;
    int HOMING_SEEK       = 90;
    int MICROFEED         = 29296;
    int WIRE_RETRACT_HARD = 400;    
    int WIRE_RETRACT_SOFT = 200;    
} speeds;

typedef struct axis_state {
    bool    enabled = false;
    uint8_t index   = 100; // valid indexes are from 0-8; to prevent unused axes from messing around..
} axis_state;

typedef struct enabled_axes { // max possible with an 8bit axis mask = 8 axes total; B11111111
    axis_state x;
    axis_state y;
    axis_state z;
    axis_state a;
    axis_state b;
    axis_state c;
    axis_state u;
    axis_state v;
} enabled_axes;

typedef struct gedm_conf {
    bool gedm_planner_line_running   = false;
    bool gedm_planner_sync           = false;
    bool gedm_flushing_motion        = false;
    bool edm_pause_motion            = false;
    bool gedm_retraction_motion      = false;
    bool gedm_disable_limits         = false;
    int current_line                 = 0;
} gedm_conf;


typedef struct {
  const char* name;
  float       max_travel;
  float       home_mpos;
  float       steps_per_mm;
  int         dir_pin;
  int         step_pin;
  bool        enabled;
  bool        invert_step;
  bool        invert_dir;
  int         motor_type;
  bool        homing_seq_enabled;
} axis_conf;

//###########################################
extern std::atomic<bool> enforce_redraw;
extern std::atomic<bool> start_logging_flag;
extern std::atomic<bool> protocol_ready;

extern DRAM_ATTR volatile bool    probing_success;
extern DRAM_ATTR volatile bool    probe_touched;
extern DRAM_ATTR volatile int64_t flush_retract_after; // retract tool after x us to allow flushing; set to 0 to disable;
extern DRAM_ATTR volatile float   flush_retract_mm;
extern DRAM_ATTR volatile int     flush_offset_steps; // after moving the axis up for flushing it goes back down to the start position minus the offset steps
extern DRAM_ATTR volatile bool    disable_spark_for_flushing;
extern volatile bool              enable_spindle;
extern volatile bool              simulate_gcode; // if enabled the gcode will only move XY and won't react to up/down commands

extern DRAM_ATTR volatile float xyuv_jog_combined; // only used for xyvu to jog xu and yv combined

extern DRAM_ATTR volatile gedm_conf gconf;
extern DRAM_ATTR volatile retraction_distances retconf;
extern DRAM_ATTR volatile speeds process_speeds;
extern enabled_axes axes_active;







extern char MASHINE_AXES_STRING[N_AXIS];
extern DRAM_ATTR GAxis* g_axis[];
extern axis_conf axis_configuration[N_AXIS]; // name, max_travel, homepos, stepspermm, dirpin,steppin,enabled,invert_step,invert_dir


extern bool           is_axis( const char* axis_name, uint8_t axis_index );
extern void           enable_axis( const char* axis_name, uint8_t axis_index );
extern void IRAM_ATTR debuglog( const char* msg, uint16_t task_delay = 0 );
extern const char*    get_axis_name_const(uint8_t axis);

#endif