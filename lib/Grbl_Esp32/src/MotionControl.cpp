/*
  MotionControl.cpp - high level interface for issuing motion commands
  Part of Grbl

  Copyright (c) 2011-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud

    2018 -	Bart Dring This file was modifed for use on the ESP32
            CPU. Do not use this with Grbl for atMega328P

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.

  2023 - Roland Lautensack (G-EDM) This file was edited and may no longer be compatible with the default grbl

*/

//#include "Grbl.h"
#include "MotionControl.h"
#include "probing_routines.h"
#include "ili9341_tft.h"

MOTION_CONTROL motion;

MOTION_CONTROL::MOTION_CONTROL(){}
MOTION_CONTROL::~MOTION_CONTROL(){}

Error MOTION_CONTROL::jog( plan_line_data_t* pl_data, parser_block_t* gc_block ){
    pl_data->feed_rate = gc_block->values.f;
    if ( !glimits.target_within_reach(gc_block->values.xyz) ){
        return Error::TravelExceeded;
    }
    set_machine_state( STATE_JOG );
    if( !line(gc_block->values.xyz, pl_data) ){
        return Error::JogCancelled;
    }
    return Error::Ok;
}

bool MOTION_CONTROL::line( float* target, plan_line_data_t* pl_data ){
    if( // states that are not overwritten, for everything else it will set the default busy state
        is_machine_state( STATE_IDLE ) // should not happen
    ){ set_machine_state( STATE_BUSY ); }

    if( is_system_op_mode( OP_MODE_EDM_WIRE ) && axes_active.z.enabled ){
        // overwrite z motion with current position
        float* current              = system_get_mpos();
        target[axes_active.z.index] = current[axes_active.z.index];
    }

    planner.plan_history_line(target, pl_data);
    return true;
}


bool MOTION_CONTROL::dwell( int32_t milliseconds ){
    if( milliseconds <= 0 ) { return false; }
    return delay_msec( milliseconds, DwellMode::Dwell );
}

void MOTION_CONTROL::arc( 
    float* target, plan_line_data_t* pl_data, 
    float* position, float* offset, float radius, 
    uint8_t axis_0, uint8_t axis_1, uint8_t axis_linear, 
    uint8_t is_clockwise_arc 
){

    pl_data->is_arc = true;

    float center_axis0 = position[axis_0] + offset[axis_0];
    float center_axis1 = position[axis_1] + offset[axis_1];
    float r_axis0      = -offset[axis_0];  // Radius vector from center to current location
    float r_axis1      = -offset[axis_1];
    float rt_axis0     = target[axis_0] - center_axis0;
    float rt_axis1     = target[axis_1] - center_axis1;

    // CCW angle between position and target from circle center. Only one atan2() trig computation required.
    float angular_travel = atan2(r_axis0 * rt_axis1 - r_axis1 * rt_axis0, r_axis0 * rt_axis0 + r_axis1 * rt_axis1);
    if (is_clockwise_arc) {  // Correct atan2 output per direction
        if (angular_travel >= -ARC_ANGULAR_TRAVEL_EPSILON) { angular_travel -= 2 * M_PI; }
    } else {
        if (angular_travel <= ARC_ANGULAR_TRAVEL_EPSILON) { angular_travel += 2 * M_PI; }
    }

    //if (is_clockwise_arc && angular_travel > 0) {
    //  angular_travel -= 2 * M_PI;
    //} else if (!is_clockwise_arc && angular_travel < 0) {
    //  angular_travel += 2 * M_PI;
    //}

    // NOTE: Segment end points are on the arc, which can lead to the arc diameter being smaller by up to
    // (2x) arc_tolerance. For 99% of users, this is just fine. If a different arc segment fit
    // is desired, i.e. least-squares, midpoint on arc, just change the mm_per_arc_segment calculation.
    // For the intended uses of Grbl, this value shouldn't exceed 2000 for the strictest of cases.

    //uint16_t segments = (uint16_t)ceil(fabs(0.5 * angular_travel * radius) / sqrt( DEFAULT_ARC_TOLERANCE * (2 * radius - DEFAULT_ARC_TOLERANCE )));
    //if (segments == 0) segments = 1;

    uint16_t segments = floor(fabs(0.5 * angular_travel * radius) / sqrt( DEFAULT_ARC_TOLERANCE * (2 * radius - DEFAULT_ARC_TOLERANCE )));

    if( segments ){
        // Multiply inverse feed_rate to compensate for the fact that this movement is approximated
        // by a number of discrete segments. The inverse feed_rate should be correct for the sum of
        // all segments.
        if (pl_data->motion.inverseTime) {
            pl_data->feed_rate *= segments;
            pl_data->motion.inverseTime = 0;  // Force as feed absolute mode over arc segments.
        }
        float theta_per_segment  = angular_travel / segments;
        float linear_per_segment = (target[axis_linear] - position[axis_linear]) / segments;

        // Computes: cos_T = 1 - theta_per_segment^2/2, sin_T = theta_per_segment - theta_per_segment^3/6) in ~52usec
        float cos_T = 2.0 - theta_per_segment * theta_per_segment;
        float sin_T = theta_per_segment * 0.16666667 * (cos_T + 4.0);
        cos_T *= 0.5;
        float    sin_Ti;
        float    cos_Ti;
        float    r_axisi;
        uint16_t i;
        uint8_t  count             = 0;
        float    original_feedrate = pl_data->feed_rate;  // Kinematics may alter the feedrate, so save an original copy

        for (i = 1; i < segments; i++) {                  // Increment (segments-1).
            if( i >= segments-1 ){
                pl_data->is_arc = false; // remove arc flag for last segment
            }
            if (count < N_ARC_CORRECTION) {
                // Apply vector rotation matrix. ~40 usec
                r_axisi = r_axis0 * sin_T + r_axis1 * cos_T;
                r_axis0 = r_axis0 * cos_T - r_axis1 * sin_T;
                r_axis1 = r_axisi;
                count++;
            } else {
                // Arc correction to radius vector. Computed only every N_ARC_CORRECTION increments. ~375 usec
                // Compute exact location by applying transformation matrix from initial radius vector(=-offset).
                cos_Ti  = cos(i * theta_per_segment);
                sin_Ti  = sin(i * theta_per_segment);
                r_axis0 = -offset[axis_0] * cos_Ti + offset[axis_1] * sin_Ti;
                r_axis1 = -offset[axis_0] * sin_Ti - offset[axis_1] * cos_Ti;
                count   = 0;
            }
            // Update arc_target location
            position[axis_0] = center_axis0 + r_axis0;
            position[axis_1] = center_axis1 + r_axis1;
            position[axis_linear] += linear_per_segment;
            pl_data->feed_rate = original_feedrate;  // This restores the feedrate kinematics may have altered
            glimits.limits_check_soft(position);
            line(position, pl_data);
            if( get_quit_motion() ){ return; }
        }
    }
    // Ensure last segment arrives at target location.
    glimits.limits_check_soft(target);
    line(target, pl_data);
}

// Perform homing cycle to locate and set machine zero. Only '$H' executes this command.
// NOTE: There should be no motions in the buffer and Grbl must be in an idle state before
// executing the homing cycle. This prevents incorrect buffered plans after homing.
void MOTION_CONTROL::homing_cycle( uint8_t cycle_mask ) {
    gconf.gedm_disable_limits = true;
    if( cycle_mask ){ glimits.limits_go_home(cycle_mask); }
    if( get_quit_motion() ) return;
    gcode_core.gc_sync_position();
    gconf.gedm_disable_limits = false;
    remote_control( 2 ); // read the limit switch to ensure they are in a valid state
}



uint8_t get_move_directions( float* target, bool *is_negative ){
    uint8_t axis_mask = 0;
    float* current_mpos = system_get_mpos();
    for( int axis; axis < N_AXIS; ++axis ){
        if( target[axis] < current_mpos[axis] ){
            is_negative[axis] = true;
            axis_mask |= bit(axis);
        } else if( target[axis] > current_mpos[axis] ){
            is_negative[axis] = false;
            axis_mask |= bit(axis);
        } else {
            is_negative[axis] = false;
        }
    }
    return axis_mask;
}


// Perform tool length probe cycle. Requires probe switch.
// NOTE: Upon probe failure, the program will be stopped and placed into ALARM state.
GCUpdatePos MOTION_CONTROL::probe_cycle( float* target, plan_line_data_t* pl_data, uint8_t parser_flags ){

    if( get_quit_motion() ) return GCUpdatePos::None;  // Return if system reset has been issued.
    
    set_machine_state( STATE_PROBING );
    //probe_touched = false;
    vTaskDelay(DEBUG_LOG_DELAY);

    // After syncing, check if probe is already triggered. If so, halt and issue alarm.
    if( probe_get_state() ){ // Check probe pin state.
        set_alarm( ERROR_PROBE_HAD_CONTACT ); // will override the probe state
        return GCUpdatePos::None;
    }

    //#######################################################
    // Create the move directions and axis mask
    //#######################################################
    //bool move_directions[N_AXIS];
    //uint8_t axis_mask = get_move_directions( target, move_directions ); // move_directions[axis]: true = negative, false = positive or none


    // Initialize probing control variables
    uint8_t is_no_error = bit_istrue( parser_flags, GCParserProbeIsNoError );
    probing_success     = false; // Re-initialize probe history before beginning cycle.


    // Setup and queue probing motion. Auto cycle-start should not start the cycle.
    glimits.limits_check_soft(target);

    debuglog("@Starting probe line....", 1 );
    debuglog(" No UI updates while probing", DEBUG_LOG_DELAY );

    //######################################################################
    // Run the probe line
    //######################################################################
    planner.plan_history_line( target, pl_data );

    //######################################################################
    // Line done. The state should no be busy if it was a successful probe
    // if the state is still probe then it failed. Current mpos is the final
    // probe position on success.
    //######################################################################
    if( is_machine_state( STATE_PROBING ) ){ // not degraded to STATE_BUSY so not touched
        debuglog("*Failed", DEBUG_LOG_DELAY );
        if( is_no_error ){
            memcpy(sys_probe_position, sys_position, sizeof(sys_position));
        } else {
            set_alarm( ERROR_PROBE_NO_CONTACT );
        }
    } else {
        debuglog("@Success", DEBUG_LOG_DELAY );
        probing_success = true;  // Indicate to system the probing cycle completed successfully.
    }

    set_machine_state( STATE_BUSY );

    return ( probing_success ? GCUpdatePos::System : GCUpdatePos::Target );

}
