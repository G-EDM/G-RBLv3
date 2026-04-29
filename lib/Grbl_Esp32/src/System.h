#pragma once
/*
  System.h - Header for system level commands and real-time processes
  Part of Grbl
  Copyright (c) 2014-2016 Sungeun K. Jeon for Gnea Research LLC

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

extern DRAM_ATTR int64_t idle_timer;
extern DRAM_ATTR int32_t sys_position[N_AXIS];             // Real-time machine (aka home) position vector in steps.
extern           int32_t sys_probe_position[N_AXIS];       // Last  probe position in machine coordinates and steps.
extern           int32_t sys_probe_position_final[N_AXIS]; // final probe position in machine coordinates and steps.
extern volatile  bool    sys_probed_axis[N_AXIS];

Error            system_execute_line(char* line);
Error            system_execute_line(char* line);
Error            do_command_or_setting(const char* key, char* value);
IRAM_ATTR void   system_flag_wco_change();
IRAM_ATTR int    system_convert_mm_to_steps(float mm, uint8_t idx);
IRAM_ATTR float  system_convert_axis_steps_to_mpos(int32_t steps, uint8_t idx);
IRAM_ATTR void   system_convert_array_steps_to_mpos(float* position, int32_t* steps);
IRAM_ATTR float* system_get_mpos();
IRAM_ATTR float  system_get_mpos_for_axis( int axis );
IRAM_ATTR void   mpos_to_wpos( float* position );
float*           get_wco( void );
