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

#include <WString.h>
#include <InputBuffer.h>  
#include "Config.h"
#include <Serial.h>
#include <System.h>
#include <Motors/Motors.h>
#include <GCode.h>
#include <Settings.h>
#include <machine_limits.h>

namespace api{

    extern IRAM_ATTR float* get_wpos( void );
    extern IRAM_ATTR void   push_cmd(const char *text, bool block_until_idle = false);
    extern uint8_t create_xu_yv_axis_mask( uint8_t axis );
    extern void    get_min_max_xyuvz( uint8_t axis, float *results );
    extern float   get_max_travel_possible( int axis, int direction );
    extern float   probe_pos_to_mpos( int axis );
    extern void    update_speeds( void );
    extern void    home_machine( void );
    extern void    auto_set_probepositions( void );
    extern void    home_axis( uint8_t axis );
    extern void    block_while_homing( int axis_index );
    extern void    reset_probe_points( void );
    extern void    auto_set_probepositions_for( int axis );
    extern void    unset_probepositions_for( int axis );
    extern bool    buffer_available( void );
    extern bool    machine_is_fully_homed( void );
    extern bool    machine_is_fully_probed( void );
    extern bool    jog_axis(    float mm, const char *axis_name, int speed, int axis );
    extern void    reset_work_coords( void );
    extern void    move_to_origin( void );
    extern void    send_axismaks_motion_gcode( uint8_t axis_mask, const char* cmd_start, const char* cmd_end, float* values, bool blocking = false );
    extern bool    axis_enabled( uint8_t axis );
};