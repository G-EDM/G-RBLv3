#pragma once

/*
  MotionControl.h - high level interface for issuing motion commands
  Part of Grbl

  Copyright (c) 2011-2015 Sungeun K. Jeon
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
*/

#include "Grbl.h"

#define HOMING_CYCLE_ALL 0  // Must be zero.

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

class MOTION_CONTROL{
    public:
        MOTION_CONTROL( void );
        ~MOTION_CONTROL();
        bool        dwell( int32_t milliseconds );
        bool        line( float* target, plan_line_data_t* pl_data );  // returns true if line was submitted to planner
        void        arc( float* target, plan_line_data_t* pl_data, float* position,float* offset, float radius, uint8_t axis_0, uint8_t axis_1, uint8_t axis_linear, uint8_t is_clockwise_arc );
        void        homing_cycle( uint8_t cycle_mask );
        GCUpdatePos probe_cycle( float* target, plan_line_data_t* pl_data, uint8_t parser_flags );
        Error       jog( plan_line_data_t* pl_data, parser_block_t* gc_block );

};

extern MOTION_CONTROL motion;