#pragma once

/*
	Motors.h
	Header file for Motor Classes
	Here is the hierarchy
		Motor
			Nullmotor
			StandardStepper
				TrinamicDriver
			Unipolar
			RC Servo

	These are for motors coordinated by Grbl_ESP32
	See motorClass.cpp for more details

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

    2023 - Roland Lautensack (G-EDM) This file was heavily edited and may no longer be compatible with the default grbl

*/

#include "gedm_motor/stepper.h"

class G_STEPPER_MANAGER{

	private:

    	int n_axis;

	public:
	    G_STEPPER_MANAGER();
        void init( void );
        int get_slowest_axis( void );
        float get_highest_maxtravel( uint8_t axis_mask );
        uint8_t motors_set_homing_mode( uint8_t homing_mask, bool isHoming );
        bool IRAM_ATTR motors_direction( uint8_t dir_mask );
        void IRAM_ATTR motors_step(uint8_t step_mask);
        void motors_unstep( void );
        void motors_set_disable(bool disable, uint8_t mask = B11111111);  // default is all axes
        G_STEPPER* get_motor( int axis );
        int IRAM_ATTR convert_mm_to_steps( float mm, uint8_t axis);

};

extern G_STEPPER_MANAGER motor_manager;