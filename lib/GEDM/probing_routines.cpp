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

#include "probing_routines.h"
#include "api.h"


G_EDM_PROBE_ROUTINES::G_EDM_PROBE_ROUTINES(){
    tool_diameter = DEFAULT_TOOL_DIAMETER;
}


void G_EDM_PROBE_ROUTINES::set_tool_diameter( float _diameter ){
    tool_diameter = _diameter;
}

float G_EDM_PROBE_ROUTINES::get_tool_diameter(){
    return tool_diameter;
}


IRAM_ATTR bool probe_get_state() {
    return probe_touched;
}

IRAM_ATTR bool probe_state_monitor() {
    if( probe_get_state() || simulate_gcode ){
        set_machine_state( STATE_BUSY ); // probe touched; degrade the state to busy
        memcpy(sys_probe_position, sys_position, sizeof(sys_position));
        return true;
    }
    return false;
}
void G_EDM_PROBE_ROUTINES::probe_reset(){
    probing_success = true;
    probe_touched   = false;
}

void G_EDM_PROBE_ROUTINES::pulloff_axis( int axis, bool probe_negative ){
    if( get_quit_motion() ) return;
    float values[N_AXIS] = {0.0,};
    uint8_t axis_mask = api::create_xu_yv_axis_mask( axis );
    for( int _axis = 0; _axis < N_AXIS; ++_axis ){
        if( bitnum_istrue( axis_mask, _axis ) ){
            values[_axis] = probe_negative ? 2.0 : -2.0;
        }
    }
    api::send_axismaks_motion_gcode( axis_mask, MC_CMD6, "\r\n", values );
}


//######################################################################
// This function is the one that actually sends the probe gcode G38.2
//######################################################################
void G_EDM_PROBE_ROUTINES::probe_axis( int axis, bool probe_negative ){
    probe_touched   = false;
    probing_success = false;
    if( get_quit_motion() ) return;
    float min_max_travels[2];
    float values[N_AXIS] = {0.0,};
    api::get_min_max_xyuvz( axis, min_max_travels );
    uint8_t axis_mask = api::create_xu_yv_axis_mask( axis );
    bool valid = true;
    for( int _axis = 0; _axis < N_AXIS; ++_axis ){
        if( bitnum_istrue( axis_mask, _axis ) ){
            values[_axis] = probe_negative ? min_max_travels[0] : min_max_travels[1];
            if( values[_axis] == 0.0 ){
                valid = false;
            }
        }
    }
    if( !valid) { return;}
    api::send_axismaks_motion_gcode( axis_mask, MC_CMD5, " F40\r\n", values );
    wait_for_block_until_state( STATE_PROBING, STATE_IDLE, 400 ); // probing sets the state to busy on touch so it needs to wait for idle
}

//######################################################################
// Sends the gcode to set the probe position after success G10 P1 L20
//######################################################################
void G_EDM_PROBE_ROUTINES::set_probe( uint8_t axis, float pos ){
    float values[N_AXIS] = {0.0,};
    uint8_t axis_mask = api::create_xu_yv_axis_mask( axis );
    for( int _axis = 0; _axis < N_AXIS; ++_axis ){
        if( bitnum_istrue( axis_mask, _axis ) ){
            values[_axis] = pos;
        }
    }
    api::send_axismaks_motion_gcode( axis_mask, MV_CMD3, "\r\n", values );
}

//######################################################################
// Main gateway for probe motion
//######################################################################
bool G_EDM_PROBE_ROUTINES::do_probe( int axis, bool probe_negative, bool pulloff ){
    bool success = false;
    if( get_quit_motion() ) return success;
    probe_axis( axis, probe_negative );

    if( !system_block_motion && probing_success ){
        success = true;
        //###################################################
        // Compensate wire for XYUV probing
        //###################################################
        float tool_radius = 0.0;
        if( axis != axes_active.z.index ){ // compensate wire for XYUV; Z does not use horizontal compensation
            tool_radius = tool_diameter > 0.0 ? tool_diameter/2.0 : 0.0;
            if( probe_negative ){ tool_radius *= -1; } // probe direction was negative and the radius needs to be substracted. 
                                                       // Radius is value that is added later to the position using +=. Making it negative will substract it later.
        }
        uint8_t axis_mask = api::create_xu_yv_axis_mask( axis ); // creates a bitmask with all involved axes that are chained together: XU YV
        for( int _axis = 0; _axis < N_AXIS; ++_axis ){
            if( bitnum_istrue( axis_mask, _axis ) ){
                // the axismaks contains either one or two axes. Two axes are present only if they are chained togeter like XU and YV. 
                // Tool compensation will apply on all axes in the mask the same way relative to the axis positon after probing
                // tool radius at this point is either positive or negative or zero
                sys_probe_position[_axis]      += tool_radius;
                sys_probe_position_final[_axis] = sys_probe_position[_axis];
                sys_probed_axis[_axis]          = true;
            }
        }   
        set_probe( axis, tool_radius ); // sends a G91 G10 P1 L20 command with the tool radius as compensation
        if( pulloff ){ pulloff_axis( axis, probe_negative ); }
    }
    wait_for_idle_state();
    return success;
}
bool G_EDM_PROBE_ROUTINES::probe_z( bool probe_negative ){
    return do_probe( axes_active.z.index, probe_negative, true );
}
bool G_EDM_PROBE_ROUTINES::probe_x( bool probe_negative, bool pulloff ){
    return do_probe( axes_active.x.index, probe_negative, pulloff );
}
bool G_EDM_PROBE_ROUTINES::probe_y( bool probe_negative, bool pulloff ){
    return do_probe( axes_active.y.index, probe_negative, pulloff );
}
bool G_EDM_PROBE_ROUTINES::probe_x_negative(){
    debuglog("Probe X negative", DEBUG_LOG_DELAY );
    return probe_x( true, true );
}
bool G_EDM_PROBE_ROUTINES::probe_x_positive(){
    debuglog("Probe X positive", DEBUG_LOG_DELAY );
    return probe_x( false, true );
}
bool G_EDM_PROBE_ROUTINES::probe_y_negative(){
    debuglog("Probe Y negative", DEBUG_LOG_DELAY );
    return probe_y( true, true );
}
bool G_EDM_PROBE_ROUTINES::probe_y_positive(){
    debuglog("Probe Y positive", DEBUG_LOG_DELAY );
    return probe_y( false, true );
}
bool G_EDM_PROBE_ROUTINES::probe_z_negative(){
    debuglog("Probe Z negative", DEBUG_LOG_DELAY );
    return probe_z( true ); // only negative probes for z with pulloff enabled
}






/**
 * 3D probing routines 
 * They are meant for floating or sinker mode where Z axis is enabled
 * The center button in 3D mode is z down probe
 **/
/*void G_EDM_PROBE_ROUTINES::left_front_edge_3d(){
    probe_z(true); // first probe z
    api::push_cmd("G91 G0 X-10\r\n");
    api::push_cmd("G91 G0 Z-6\r\n", true);
    probe_x(false,true);
    api::push_cmd("G91 G0 Z6\r\n", true);
    api::push_cmd("G91 G0 X5 Y-10\r\n");
    api::push_cmd("G91 G0 Z-6\r\n", true);
    probe_y(false,true);
    api::push_cmd("G91 G0 Z6\r\n");
    api::push_cmd("G90 G54 G0 X0 Y0\r\n", true);
}
void G_EDM_PROBE_ROUTINES::left_back_edge_3d(){
    probe_z(true);
    // probe_mode_off();
    api::push_cmd("G91 G0 X-10\r\n");
    api::push_cmd("G91 G0 Z-6\r\n", true);
    probe_x(false,true);
    api::push_cmd("G91 G0 Z6\r\n", true);
    api::push_cmd("G91 G0 X5 Y10\r\n");
    api::push_cmd("G91 G0 Z-6\r\n", true);
    probe_y(true,true);
    api::push_cmd("G91 G0 Z6\r\n");
    api::push_cmd("G90 G54 G0 X0 Y0\r\n", true);
}
void G_EDM_PROBE_ROUTINES::right_back_edge_3d(){
    probe_z(true);
    api::push_cmd("G91 G0 X10\r\n");
    api::push_cmd("G91 G0 Z-6\r\n", true);
    probe_x(true,true);
    api::push_cmd("G91 G0 Z6\r\n", true);
    api::push_cmd("G91 G0 X-5 Y10\r\n");
    api::push_cmd("G91 G0 Z-6\r\n", true);
    probe_y(true,true);
    api::push_cmd("G91 G0 Z6\r\n");
    api::push_cmd("G90 G54 G0 X0 Y0\r\n", true);
}
void G_EDM_PROBE_ROUTINES::right_front_edge_3d(){
    probe_z(true);
    api::push_cmd("G91 G0 X10\r\n");
    api::push_cmd("G91 G0 Z-6\r\n", true);
    probe_x(true,true);
    api::push_cmd("G91 G0 Z6\r\n", true);
    api::push_cmd("G91 G0 X-5 Y-10\r\n");
    api::push_cmd("G91 G0 Z-6\r\n", true);
    probe_y(false,true);
    api::push_cmd("G91 G0 Z6\r\n");
    api::push_cmd("G90 G54 G0 X0 Y0\r\n", true);
}*/




//################################################################################
// Currently only able to move XY. If enabled it will combine XU and YV
// If use_x or use_y = 0.0 it will ignore the axis
//################################################################################
void move_axis( float use_x, float use_y, bool blocking, const char* cmd_start ){
    uint8_t axis_mask = 0, x_axis_mask = 0, y_axis_mask = 0;
    float values[N_AXIS] = {0.0f};
    if( use_x != 0.0 ){
        x_axis_mask = api::create_xu_yv_axis_mask( axes_active.x.index );
        for( int i = 0; i < N_AXIS; ++i ){
            if( bitnum_istrue( x_axis_mask, i ) ){
                axis_mask |= bit(i);
                values[i] = use_x;
            }
        }
    } 
    if( use_y != 0.0 ){
        y_axis_mask = api::create_xu_yv_axis_mask( axes_active.y.index );
        for( int i = 0; i < N_AXIS; ++i ){
            if( bitnum_istrue( y_axis_mask, i ) ){
                axis_mask |= bit(i);
                values[i] = use_y;
            }
        }
    }
    if( get_quit_motion() ) return;
    api::send_axismaks_motion_gcode( axis_mask, cmd_start, "\r\n", values, blocking );
}






 /**
 * 2D probing routines 
 * They are meant for wire mode where Z axis is disabled
 * The center button in 2D mode is a center finder for holes
 **/
void G_EDM_PROBE_ROUTINES::left_front_edge_2d(){
    probe_x(false,true);                // x axis probing from left to right and retracting a little after probing
    //api::push_cmd("G91 G0 Y-10\r\n"); 
    move_axis( 0.0, -10.0, false, MV_CMD1 ); // move y towards operator
    //api::push_cmd("G91 G0 X5\r\n");
    move_axis( 5.0, 0.0, false ,MV_CMD1 ); // move x to the right
    probe_y(false,true);                // y axis probing away from operator
    //api::push_cmd("G90 G54 X-2 Y-2\r\n", true); 
    move_axis( -2.0, -2.0, true, MV_CMD2 );// move xy 2mm away from zero position
}
void G_EDM_PROBE_ROUTINES::left_back_edge_2d(){
    probe_x(false,true);               // x axis probing from left to right and retracting a little after probing
    //api::push_cmd("G91 G0 Y10\r\n"); 
    move_axis( 0.0, 10.0, false, MV_CMD1 ); // move y away from operator
    //api::push_cmd("G91 G0 X5\r\n");  
    move_axis( 5.0, 0.0, false, MV_CMD1 ); // move x to the right
    probe_y(true,true);              // y axis probing towards operator
    //api::push_cmd("G90 G54 X-2 Y2\r\n", true); 
    move_axis( -2.0, 2.0, true, MV_CMD2 ); // move xy 2mm away from zero position
}
void G_EDM_PROBE_ROUTINES::right_back_edge_2d(){
    probe_x(true,true);              // x axis probing from right to left and retracting a little after probing
    //api::push_cmd("G91 G0 Y10\r\n"); 
    move_axis( 0.0, 10.0, false, MV_CMD1 ); // move y away from operator
    //api::push_cmd("G91 G0 X-5\r\n"); 
    move_axis( -5.0, 0.0, false, MV_CMD1 ); // move x to the left
    probe_y(true,true);              // y axis probing towards operator
    //api::push_cmd("G90 G54 X2 Y2\r\n", true); 
    move_axis( 2.0, 2.0, true, MV_CMD2 ); // move xy 2mm away from zero position
}
void G_EDM_PROBE_ROUTINES::right_front_edge_2d(){
    probe_x(true,true);               // x axis probing from right to left and retracting a little after probing
    //api::push_cmd("G91 G0 Y-10\r\n"); 
    move_axis( 0.0, -10.0, false, MV_CMD1 ); // move y towards operator
    //api::push_cmd("G91 G0 X-5\r\n");  
    move_axis( -5.0, 0.0, false, MV_CMD1 ); // move x to the left
    probe_y(false,true);                // y axis probing away from operator
    //api::push_cmd("G90 G54 X2 Y-2\r\n", true); 
    move_axis( 2.0, -2.0, true, MV_CMD2 ); // move xy 2mm away from zero position
}


bool G_EDM_PROBE_ROUTINES::center_finder_2d(){
    int y_neg, y_pos, x_neg, x_pos, x_target_away_from_current_pos, y_target_away_from_current_pos, y_len, x_len;
    float target_pos;
    bool success = false;
    //#########################################################################################
    // Electrode needs to be within the hole This routine doesn't generate any z motions.
    // First probe X in negative direction. If a U axis is enabled it will move X and U 
    // together. It will also set the zero position for U. If probe X touches the probe the 
    // current position of X and U is set  as zero position. The move_axis function also 
    // chains X and U if U is enabled. Same behavior for Y that can have an optional V axis. 
    // Machine concept of XY and XYUV (wire edm) can both use this center finder XYUV have all 
    // the same step resolution third parameter for move_axis() is to enable blocking until 
    // machine is idle again. Positions of every axis is in negative space from zero to 
    // negative max travel like 0 to -500 Both probe direction will produce a position 
    // value <= 0 x_pos and y_pos should be large then x_neg and y_neg
    //#########################################################################################
    if( probe_x( true, false ) ){                                                           // probe x in negative direction with no pulloff; sends a "G91 G10 P1 L20 X0 (U0)" command after probing to set the zero position
        x_neg = sys_probe_position_final[axes_active.x.index];                              // example: -300; save x absolute negative probe position; wire diamenter compensation is done in the probe function itself
        move_axis( 1.0, 0.0, true, MC_CMD4 );                                               // G90 G0 ... command to move X(U) 1mm away from the probe position in positive direction
        if( probe_x( false, false ) ){                                                      // probe x in positive direction with no pulloff; sends a "G91 G10 P1 L20 X0 (U0)" command after probing to set the zero position
            x_pos    = sys_probe_position_final[axes_active.x.index];                       // example: -200; save x absolute positive probe position; wire diamenter compensation is done in the probe function itself
            x_len    = x_neg - x_pos;
            x_target_away_from_current_pos = x_len / 2;                                     // example: (-300 - -200 ) / 2 = -100; find center position; should be like the radius of a circle; half the diameter or half the length between probe points
            target_pos = system_convert_axis_steps_to_mpos( x_target_away_from_current_pos, axes_active.x.index); // convert steps into mm
            move_axis( target_pos, 0.0, true, MC_CMD4 );                                    // G90 G0 X-100 (U-100)...; move axis into the center of the x probe points
            //##########################################################################################
            // X(U) axis fully probed and the wire is at the center position of the probe shape in X(U)
            // Probe Y in negative direction; If a V axis enabled it will move Y and V together
            //##########################################################################################
            if( probe_y( true, false ) ){                                                    // probe y in negative direction with no pulloff; sends a "G91 G10 P1 L20 Y0 (V0)" command after probing to set the zero position
                y_neg = sys_probe_position_final[axes_active.y.index];                       // save y absolute negative probe position; wire diamenter compensation is done in the probe function itself
                move_axis( 0.0, 1.0, false, MC_CMD4 );                                       // G90 G0 ... command to move Y(V) 1mm away from the probe position in positive direction
                if( probe_y( false, false ) ){                                               // probe y in positive direction with no pulloff; sends a "G91 G10 P1 L20 Y0 (V0)" command after probing to set the zero position
                    y_pos       = sys_probe_position_final[axes_active.y.index];             // save y absolute positive probe position; wire diamenter compensation is done in the probe function itself
                    y_len       = y_neg - y_pos;
                    y_target_away_from_current_pos = y_len / 2;                              // find center position; should be like the radius of a circle; half the diameter or half the length between probe points
                    target_pos = system_convert_axis_steps_to_mpos( y_target_away_from_current_pos, axes_active.y.index); // convert steps into mm
                    move_axis( 0.0, target_pos, true, MC_CMD4 );                             // G90 G0 ...; move axis into the center of the y probe points
                    //#################################################################################
                    // Wire is now at the center of pocket in X(U) and Y(V)
                    // Set the zero positions for this exact position for XY and if enabled also for UV
                    //#################################################################################
                    set_probe( axes_active.x.index, 0.0 ); // sends G91 G10 P1 L20 X0 (U0) 
                    set_probe( axes_active.y.index, 0.0 ); // sends G91 G10 P1 L20 Y0 (V0)
                    wait_for_idle_state();
                    success = true;                        // flag probe success
                }
            }
        }
    }
    return success;
}