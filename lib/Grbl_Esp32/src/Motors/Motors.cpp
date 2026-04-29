/*
	Motors.cpp
	Part of Grbl_ESP32
	2020 -	Bart Dring
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
	TODO
		Make sure public/private/protected is cleaned up.
		Only a few Unipolar axes have been setup in init()
		Get rid of Z_SERVO, just reply on Z_SERVO_PIN
		Class is ready to deal with non SPI pins, but they have not been needed yet.
			It would be nice in the config message though
	Testing
		Done (success)
			3 Axis (3 Standard Steppers)
			MPCNC (ganged with shared direction pin)
			TMC2130 Pen Laser (trinamics, stallguard tuning)
			Unipolar
		TODO
			4 Axis SPI (Daisy Chain, Ganged with unique direction pins)
	Reference
		TMC2130 Datasheet https://www.trinamic.com/fileadmin/assets/Products/ICs_Documents/TMC2130_datasheet.pdf
    
    2023 - Roland Lautensack (G-EDM) This file was heavily edited and may no longer be compatible with the default grbl

*/



#include "Motors/Motors.h"
#include "gedm_motor/rmt_stepper.h"

#include "Grbl.h"
#include <api.h>

G_STEPPER* motors_b[N_AXIS];


G_STEPPER_MANAGER motor_manager = G_STEPPER_MANAGER();

G_STEPPER_MANAGER::G_STEPPER_MANAGER(){}

void G_STEPPER_MANAGER::init() {

    n_axis = N_AXIS;

    for( int i = 0; i < n_axis; ++i ){
        int axis_index = g_axis[i]->axis_index.get();
        int motor_type = g_axis[i]->motor_type.get();
        switch( motor_type ){
            case 1:
                motors_b[ axis_index ] = new RMT_STEPPER();
                motors_b[ axis_index ]->setup( g_axis[i] );
            break;
        
            default:
            break;
        }
        //motors_b[ axis_index ] = new RMT_STEPPER();
        //motors_b[ axis_index ]->setup( g_axis[i] );
        //motors[ axis_index ] = new G_EDM_STEPPER( axis_index, g_axis[i]->step_pin.get(), g_axis[i]->dir_pin.get() );
    }

    if (STEPPERS_DISABLE_PIN != UNDEFINED_PIN) {
        pinMode(STEPPERS_DISABLE_PIN, OUTPUT); 
    }
    for (uint8_t axis = 0; axis < n_axis; axis++) {
        //motors[axis]->init();
        motors_b[axis]->init();
    }
}

int IRAM_ATTR G_STEPPER_MANAGER::convert_mm_to_steps( float mm, uint8_t axis){
    return round( ( float ) g_axis[axis]->steps_per_mm.get() * mm );
}

G_STEPPER* G_STEPPER_MANAGER::get_motor( int axis ) {
    return motors_b[axis];
}

void G_STEPPER_MANAGER::motors_set_disable(bool disable, uint8_t mask) {
    static bool    prev_disable = true;
    static uint8_t prev_mask    = 0;
    if ((disable == prev_disable) && (mask == prev_mask)) {
        return;
    }
    prev_disable = disable;
    prev_mask    = mask;
    if (INVERT_ST_ENABLE) {
        disable = !disable;  // Apply pin invert.
    }
    // global disable.
    digitalWrite(STEPPERS_DISABLE_PIN, disable);
    for (uint8_t axis = 0; axis < n_axis; axis++) {
        //motors[axis]->set_disable( disable );
    }
    // Add an optional delay for stepper drivers. that need time
    // Some need time after the enable before they can step.
    auto wait_disable_change = DEFAULT_STEP_ENABLE_DELAY;
    if (wait_disable_change != 0) {
        auto disable_start_time = esp_timer_get_time() + wait_disable_change;
        while ((esp_timer_get_time() - disable_start_time) < 0) {
            NOP();
        }
    }
}


float get_highest_maxtravel( uint8_t axis_mask ){
    float max_travel = 0;
    for (uint8_t axis = 0; axis < N_AXIS; axis++) {
        if (bit_istrue( axis_mask, bit( axis ) ) ){
            max_travel = MAX( max_travel, g_axis[axis]->max_travel_mm.get() );
        }
    }
    return max_travel;
}



// use this to tell all the motors what the current homing mode is
// They can use this to setup things like Stall
uint8_t G_STEPPER_MANAGER::motors_set_homing_mode( uint8_t homing_mask, bool isHoming ) {
    uint8_t can_home = 0;
    for (uint8_t axis = 0; axis < n_axis; axis++) {
        if (bitnum_istrue(homing_mask, axis)) {
            if( ! isHoming ){
                if( system_block_motion ){ continue; }
                g_axis[ axis ]->is_homed.set( true );
            } else{
                g_axis[ axis ]->is_homed.set( false );
            }
            bitnum_true(can_home, axis);
        }
    }

    if( api::machine_is_fully_homed() ){
        api::push_cmd("G28.1\r\n"); // set home position
    }

    return can_home;
}

int G_STEPPER_MANAGER::get_slowest_axis(){
    // get slowest axis (most pulses per mm)
    float max_pulses_per_mm = 0;
    //int max_pulses_per_mm = 0;
    int slowest_axis      = 0;
    for( int i = 0; i < n_axis; ++i ){
        if( g_axis[i]->steps_per_mm.get() >= max_pulses_per_mm ){
            max_pulses_per_mm = g_axis[i]->steps_per_mm.get();
            slowest_axis = i;
        }
    }
    return slowest_axis;
}

bool IRAM_ATTR G_STEPPER_MANAGER::motors_direction( uint8_t dir_mask ) {
    bool direction_has_changed = false;
    for (uint8_t axis = 0; axis < n_axis; axis++) {
        if( motors_b[axis]->set_direction( bitnum_istrue( dir_mask, axis ), false ) ){
            direction_has_changed = true;
        }
    }
    if( !direction_has_changed ){return false;}
    delayMicroseconds( STEPPER_DIR_CHANGE_DELAY );
    return true;
}

void IRAM_ATTR G_STEPPER_MANAGER::motors_step( uint8_t step_mask ) {
    for (uint8_t axis = 0; axis < n_axis; axis++) {
        if (bitnum_istrue(step_mask, axis)) {
            //motors[axis]->step();
            motors_b[axis]->step();
        }
    }
}
void G_STEPPER_MANAGER::motors_unstep(){}