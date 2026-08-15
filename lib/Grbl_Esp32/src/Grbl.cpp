/*
  Grbl.cpp - Initialization and main loop for Grbl
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


//################################################
// GRBL_ESP32
//################################################
#include "Grbl.h"

GRBL_MAIN grbl;

GRBL_MAIN::GRBL_MAIN(){}

void GRBL_MAIN::init() {
    memset(sys_position, 0, sizeof(sys_position)); // Clear machine position.
    gserial.client_init();
    disableCore0WDT();
    disableCore1WDT();
    settings_init();
    motor_manager.init();
    inputBuffer.begin();
    set_machine_state( STATE_IDLE );
}

void GRBL_MAIN::reset_variables() {
    memset(sys_probe_position, 0, sizeof(sys_probe_position)); // Clear probe position.
    gserial.client_reset_read_buffer();
    gcode_core.gc_init();
    gcode_core.gc_sync_position();
    gcode_core.sync_xyz_block();
}
