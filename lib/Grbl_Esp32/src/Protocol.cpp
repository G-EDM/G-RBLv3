/*
  Protocol.cpp - controls Grbl execution protocol and procedures
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

  2023 - Roland Lautensack (G-EDM) This file was heavily edited and may no longer be compatible with the default grbl

*/
#include "Grbl.h"
#include "sensors.h"
#include "widgets/ui_interface.h"


typedef struct {
    char buffer[DEFAULT_LINE_BUFFER_SIZE];
    int  length;
} client_line_t;

client_line_t client_lines;

GRBL_PROTOCOL gproto;

GRBL_PROTOCOL::GRBL_PROTOCOL(){}


IRAM_ATTR void GRBL_PROTOCOL::empty_line() {
    client_lines.length    = 0;
    client_lines.buffer[0] = '\0';
}

IRAM_ATTR Error GRBL_PROTOCOL::add_char_to_line( char c ) {
    if( c == '\b' ) {
        if( client_lines.length ) {
            --client_lines.length;
            client_lines.buffer[client_lines.length] = '\0';
        }
        return Error::Ok;
    }
    if( client_lines.length == (DEFAULT_LINE_BUFFER_SIZE - 1 ) ){
        return Error::Overflow;
    }
    if( c == '\r' || c == '\n' ){
        client_lines.length = 0;
        return Error::Eol;
    }
    client_lines.buffer[client_lines.length++] = c;
    client_lines.buffer[client_lines.length]   = '\0';
    return Error::Ok;
}

IRAM_ATTR Error GRBL_PROTOCOL::execute_line( char* line ) {
    Error result = Error::Ok;
    if( line[0] == 0 || line[0] == '(' ){
        return Error::Ok;
    }
    if( line[0] == '$' || line[0] == '[' ){
        return system_execute_line(line);
    }
    if( system_state == STATE_ALARM || system_state == STATE_JOG ){
        return Error::SystemGcLock;
    }
    return gcode_core.gc_execute_line(line);
}


IRAM_ATTR void gcode_consumer_task(void *parameter){ 

    GRBL_PROTOCOL *__this = (GRBL_PROTOCOL *)parameter;

    gserial.client_reset_read_buffer();
    __this->empty_line();
    set_machine_state( STATE_IDLE );
    protocol_ready.store( true );
    idle_timer           = esp_timer_get_time();
    int64_t timestamp_us = 0;
    int c;
    vTaskDelay(10);

    for (;;) {

        if( is_machine_state( STATE_IDLE ) || gconf.gedm_retraction_motion ){
            vTaskDelay(1);
        }

        if( get_machine_state() > STATE_ESTOP ){
            if( is_machine_state( STATE_ESTOP ) ) {
                grbl.reset_variables();
                vTaskDelay(1);
                continue;
            } else if( is_machine_state( STATE_REBOOT ) ){
                return;
            }
        }

        if( gcode_line_shifting.load() || gconf.gedm_planner_line_running ){ // code is blocking but just in case...
            idle_timer = esp_timer_get_time();
        }

        while( ( c = gserial.client_read() ) != -1) {
            set_machine_state( STATE_BUSY ); // default; if in alarm or estop it will not set a lower priority state
            switch ( __this->add_char_to_line(c) ) {
                case Error::Ok:
                    // Continue reading
                    break;
                case Error::Eol:
                    if( is_machine_state( STATE_ESTOP ) ) break;
                    __this->execute_line( client_lines.buffer );
                    __this->empty_line();
                    break;
                case Error::Overflow:
                    __this->empty_line();
                    break;
                default:
                    break;
            }
            idle_timer = esp_timer_get_time();
        }  

        timestamp_us = esp_timer_get_time();
        if( timestamp_us - idle_timer > 500000 ){
            idle_timer = timestamp_us;
            if( system_state < STATE_ALARM ){
                set_machine_state( STATE_IDLE );
            }
        }


    }

    vTaskDelete(NULL);

}