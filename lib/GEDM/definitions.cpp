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

#include "definitions.h"
#include <ctype.h>
#include <string.h>

std::atomic<bool> enforce_redraw( false );
std::atomic<bool> start_logging_flag(false);
std::atomic<bool> protocol_ready(false);




DRAM_ATTR volatile bool probing_success = true;
DRAM_ATTR volatile bool probe_touched   = false;

volatile bool              enable_spindle = true;
//DRAM_ATTR volatile bool    edm_process_is_running     = false; // this is true if the machine actually is doing work
DRAM_ATTR volatile bool    disable_spark_for_flushing = true;
DRAM_ATTR volatile bool    simulate_gcode             = false; // runs the gcode file without moving z
DRAM_ATTR volatile int64_t flush_retract_after      = 0;     // retract tool after x us to allow flushing; set to 0 to disable;
DRAM_ATTR volatile float   flush_retract_mm         = 5.0;
DRAM_ATTR volatile int     flush_offset_steps       = 20;    // after moving the axis up for flushing it goes back down to the start position minus the offset steps

DRAM_ATTR volatile speeds    process_speeds;
volatile speeds_mm_min       process_speeds_mm_min;
DRAM_ATTR volatile gedm_conf gconf;
DRAM_ATTR volatile retraction_distances retconf;


DRAM_ATTR volatile float xyuv_jog_combined = true; // only used for xyvu to jog xu and yv combined


enabled_axes axes_active;

//#####################################################
// It is needed to have N_AXIS predefined
// So much code depends on having this value set
// before compiling. N_AXIS needs to match the number
// of axis with the enabled flag set to true
// As far as I know the max number of possible axis
// is 8. uint8_t bitmasks like B11111111 will be able
// to manage 8 axis. More axis would require to change
// this to uint16_t to have a larger bitmask and also 
// lots of code changes.
//#####################################################
DRAM_ATTR GAxis* g_axis[N_AXIS];
char MASHINE_AXES_STRING[N_AXIS] = {};

//#####################################################
// The index of the axis will be based on the order
// in this object. First element that is enabled
// will be at index 0 in the g_axis element.
// This does effect some stuff in the UI
// for example the position elements. Axis with
// index 0 will be shown first etc.
// the order doesn't matter for the functionality
// but visibility stuff is affected
//#####################################################
#define MOTOR_TYPE_RMT 1


axis_conf axis_configuration[N_AXIS] = {
    // make sure to adjust N_AXIS in definitions.h to match the number of axes here
    // if N_AXIS does not match the number of enabled axes it will crash the esp
    // could change it but are too lazy 
    // changing the axes order or adding/removing axes will also reset the firmware
    #if defined( CNC_TYPE_XY ) || defined( CNC_TYPE_XYZ ) || defined( CNC_TYPE_XYUV ) || defined( CNC_TYPE_XYUVZ )
        {
            "X",                  // axis name uppercase
            X_MAX_TRAVEL,         // axis max travel mm
            0.0,                  // home pos ( better don't change )
            X_STEPS_PER_MM,       // motor steps per mm
            X_DIRECTION_PIN,      // direction pin
            X_STEP_PIN,           // step pin
            true,                 // axis is enabled. If not true it will not create this axis
            false,                // step pin is inverted ( this is not done yet. CHanges will have no effect )
            false,                // dir pin is inverted ( this is not done yet. CHanges will have no effect )
            MOTOR_TYPE_RMT,
            true // part of the homing sequence if home_all is pressed
        },
        { "Y", Y_MAX_TRAVEL, 0.0, Y_STEPS_PER_MM, Y_DIRECTION_PIN, Y_STEP_PIN, true, false, false, MOTOR_TYPE_RMT, true },
    #endif
    #if defined( CNC_TYPE_XYUV ) || defined( CNC_TYPE_XYUVZ )
        { "U", U_MAX_TRAVEL, 0.0, U_STEPS_PER_MM, U_DIRECTION_PIN, U_STEP_PIN, true, false, false, MOTOR_TYPE_RMT, true },
        { "V", V_MAX_TRAVEL, 0.0, V_STEPS_PER_MM, V_DIRECTION_PIN, V_STEP_PIN, true, false, false, MOTOR_TYPE_RMT, true },
    #endif
    #if defined( CNC_TYPE_XYZ ) || defined( CNC_TYPE_XYUVZ ) || defined( CNC_TYPE_Z )
        { "Z", Z_MAX_TRAVEL, 0.0, Z_STEPS_PER_MM, Z_DIRECTION_PIN, Z_STEP_PIN, true, false, false, MOTOR_TYPE_RMT, DEFAULT_HOME_ALL_WITH_Z }
    #endif
};

const char* get_axis_name_const(uint8_t axis) {
    return axis_configuration[axis].name;
}

uint8_t get_index_for_axis_name(const char* axis_name) {
    return 0;
}

bool is_axis( const char* axis_name, uint8_t axis_index ){
    if( strcmp( axis_name, g_axis[axis_index]->axis_name ) == 0 ){
        return true;
    } return false;
}

void enable_axis( const char* axis_name, uint8_t axis_index ){
    if( strcmp(axis_name, "X") == 0 ){
        axes_active.x.enabled = true;
        axes_active.x.index   = axis_index;
    } else if( strcmp(axis_name, "Y") == 0 ){
        axes_active.y.enabled = true;
        axes_active.y.index   = axis_index;
    } else if( strcmp(axis_name, "Z") == 0 ){
        axes_active.z.enabled = true;
        axes_active.z.index   = axis_index;
    } else if( strcmp(axis_name, "A") == 0 ){
        axes_active.a.enabled = true;
        axes_active.a.index   = axis_index;
    } else if( strcmp(axis_name, "B") == 0 ){
        axes_active.b.enabled = true;
        axes_active.b.index   = axis_index;
    } else if( strcmp(axis_name, "C") == 0 ){
        axes_active.c.enabled = true;
        axes_active.c.index   = axis_index;
    } else if( strcmp(axis_name, "U") == 0 ){
        axes_active.u.enabled = true;
        axes_active.u.index   = axis_index;
    } else if( strcmp(axis_name, "V") == 0 ){
        axes_active.v.enabled = true;
        axes_active.v.index   = axis_index;
    } 
}