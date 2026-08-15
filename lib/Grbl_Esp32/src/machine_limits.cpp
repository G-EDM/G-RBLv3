/*
  Limits.cpp - code pertaining to limit-switches and performing the homing cycle
  Part of Grbl

  Copyright (c) 2012-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud

	2018 -	Bart Dring This file was modifed for use on the ESP32
					CPU. Do not use this with Grbl for atMega328P
  2018-12-29 - Wolfgang Lienbacher renamed file from limits.h to grbl_limits.h
          fixing ambiguation issues with limit.h in the esp32 Arduino Framework
          when compiling with VS-Code/PlatformIO.

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
#include "Grbl.h"

GRBL_LIMITS glimits;

// Homing axis search distance multiplier. Computed by this value times the cycle travel.
#define HOMING_AXIS_SEARCH_SCALAR 1.1  // Must be > 1 to ensure limit switch will be engaged.

static volatile bool invert_limit = false;

GRBL_LIMITS::GRBL_LIMITS(){}

IRAM_ATTR bool GRBL_LIMITS::limits_get_state() {
    //int state_is_high = ( GPIO_REG_READ( GPIO_IN1_REG ) >> ( STEPPERS_LIMIT_ALL_PIN - 32 ) ) & 0x1;
    //return state_is_high ? true : false;
    int state = digitalRead( STEPPERS_LIMIT_ALL_PIN );
    return ( invert_limit ? !state : state );
}





//#############################################################
// Homing routine; with a single limit pin for all axes there
// is no need for parallel stuff with masks
// one axis at a time
//#############################################################
void GRBL_LIMITS::limits_go_home( uint8_t cycle_mask ) {

    if( get_quit_motion() ) return; // early exit if needed

    planner.set_ignore_breakpoints( true );

    cycle_mask = motor_manager.motors_set_homing_mode( cycle_mask, true );  // tell motors homing is about to start

    if (cycle_mask == 0) {
        planner.set_ignore_breakpoints( false );
        return;
    }
      
    plan_line_data_t  plan_data;
    plan_data.motion              = {};
    plan_data.motion.systemMotion = 1;
    plan_data.step_delay          = process_speeds.HOMING_SEEK; // first move at faster speed
    plan_data.feed_rate           = 30.0; // not used anymore..

    uint8_t total_cycles = 4; // move to switch, backoff, move to switch again, pulloff
    uint8_t step_pin[N_AXIS];
    uint8_t axislock;
    uint8_t involved_axis_num = 0;

    float max_travel = 0.0;
    float homing_rate = process_speeds.HOMING_SEEK;
    float target[N_AXIS];
    float* __target = system_get_mpos();
    memcpy( target, __target, sizeof(__target[0]) * N_AXIS );


    bool limit_state;
    bool move_towards_switch = true;
    bool is_final_pulloff    = false;

    // Loop over all axes and set to max travel
    // if the wanted axes are in the cycle mask
    // it will use the axis with the largest travel
    // to create the travel distance
    for( uint8_t axis = 0; axis < N_AXIS; axis++ ){
        if( bit_istrue( cycle_mask, bit( axis ) ) ){
            sys_probed_axis[axis] = false; // unflag probe state for axis
            ++involved_axis_num;
            max_travel = MAX( max_travel, ( HOMING_AXIS_SEARCH_SCALAR ) * g_axis[axis]->max_travel_mm.get() );
        }
    }

    bool line_success = false;

    do {

        if( get_quit_motion() ) break;

        vTaskDelay( 100 );

        if( move_towards_switch ){
            debuglog("Seeking switch!",DEBUG_LOG_DELAY);
        } else {
            debuglog("Pulloff move",DEBUG_LOG_DELAY);
        }

        is_final_pulloff = total_cycles==1 ? true : false;

        plan_data.use_limit_switches = is_final_pulloff ? false : true;// ( move_towards_switch ? true : false );

        //#####################################################################
        // Reset sys position and set the target position for all involved axes
        //#####################################################################
        for( uint8_t axis = 0; axis < N_AXIS; axis++ ) {
            if( bit_istrue( cycle_mask, bit( axis ) ) ){
                sys_position[axis] = 0;
                if( bit_istrue( HOMING_DIR_MASK, bit( axis ) ) ){ // inverted homing direction
                    target[axis] = move_towards_switch ? -max_travel : ( is_final_pulloff ? (HOMING_PULLOFF) : max_travel );//HOMING_PULLOFF;
                } else { // normal homing into positive direction
                    target[axis] = move_towards_switch ? max_travel : ( is_final_pulloff ? -(HOMING_PULLOFF) : -max_travel );//-HOMING_PULLOFF;
                }
            }
        }

        //#####################################################################
        // Run the line ( Note: code is blocking! )
        //#####################################################################
        line_success = planner.plan_history_line( target, &plan_data );

        //#####################################################################
        // Line finished
        //#####################################################################
        move_towards_switch = !move_towards_switch; // toggle direction
        invert_limit = !invert_limit;
        plan_data.step_delay = move_towards_switch ? process_speeds.HOMING_FEED : process_speeds.HOMING_SEEK;

    } while ( --total_cycles > 0 );

    invert_limit = false;

    if( get_quit_motion() ){
        debuglog("*Homing aborted",DEBUG_LOG_DELAY);
    } else {

        for( uint8_t axis = 0; axis < N_AXIS; axis++ ) {
            if( bit_istrue( cycle_mask, bit( axis ) ) ){
                //sys_position[axis] = 0;
            }
        }

        debuglog("@Homing finished",DEBUG_LOG_DELAY);
    }


    motor_manager.motors_set_homing_mode(cycle_mask, false);  // tell motors homing is done
    planner.set_ignore_breakpoints( false );

}


float GRBL_LIMITS::limits_positive_max(uint8_t axis) {
    float mpos = g_axis[axis]->home_position.get();
    return bitnum_istrue(HOMING_DIR_MASK, axis) ? mpos + g_axis[axis]->max_travel_mm.get() : mpos;
}

float GRBL_LIMITS::limits_negative_max(uint8_t axis) {
    float mpos = g_axis[axis]->home_position.get();
    return bitnum_istrue(HOMING_DIR_MASK, axis) ? mpos : mpos - g_axis[axis]->max_travel_mm.get();
}

// Returns true if the given target is within reach
// does not set an alarm
bool GRBL_LIMITS::target_within_reach(float* target) {
    for( uint8_t axis = 0; axis < N_AXIS; axis++ ) {
        if( ( target[axis] < limits_negative_max(axis) || target[axis] > limits_positive_max(axis) ) && g_axis[axis]->max_travel_mm.get() > 0 ) { 
            return false; 
        }
    }
    return true;
}

// sets an alarm if target if out of reach if state is not jog or homing
void GRBL_LIMITS::limits_check_soft( float* target ) {
    if( system_state != STATE_JOG && system_state != STATE_HOMING ) {
        if( !target_within_reach(target) ) {
            set_alarm( ERROR_SOFTLIMIT );
            return;
        }
    }
}
