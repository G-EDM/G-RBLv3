/*
  NutsBolts.cpp - Shared functions
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
*/

#include "Grbl.h"
#include <cstring>

const int MAX_INT_DIGITS = 8;  // Maximum number of digits in int32 (and float)


uint8_t read_float(const char* line, uint8_t* char_counter, float* float_ptr) {
    const char*   ptr = line + *char_counter;
    unsigned char c;
    c = *ptr++;
    bool isnegative = false;
    if (c == '-') {
        isnegative = true;
        c          = *ptr++;
    } else if (c == '+') {
        c = *ptr++;
    }
    uint32_t intval    = 0;
    int8_t   exp       = 0;
    uint8_t  ndigit    = 0;
    bool     isdecimal = false;
    while (1) {
        c -= '0';
        if (c <= 9) {
            ndigit++;
            if (ndigit <= MAX_INT_DIGITS) {
                if (isdecimal) { exp--; }
                intval = intval * 10 + c;
            } else { if (!(isdecimal)) { exp++; } }
        } else if (c == (('.' - '0') & 0xff) && !(isdecimal)) {
            isdecimal = true;
        } else { break; }
        c = *ptr++;
    }
    if (!ndigit) { return false; }
    float fval;
    fval = (float)intval;
    if (fval != 0) {
        while (exp <= -2) {
            fval *= 0.01;
            exp += 2;
        }
        if (exp < 0) {
            fval *= 0.1;
        } else if (exp > 0) {
            do { fval *= 10.0; } while (--exp > 0);
        }
    }
    if (isnegative) { *float_ptr = -fval; } 
    else { *float_ptr = fval; }
    *char_counter = ptr - line - 1;  // Set char_counter to next statement
    return true;
}

// Non-blocking delay function used for general operation and suspend features.
bool delay_msec(int32_t milliseconds, DwellMode mode) {
    // Note: i must be signed, because of the 'i-- > 0' check below.
    int32_t i         = milliseconds / DWELL_TIME_STEP;
    int32_t remainder = i < 0 ? 0 : (milliseconds - DWELL_TIME_STEP * i);
    while (i-- > 0) {
        if( get_quit_motion() ){ return false; }
        if (mode == DwellMode::Dwell) {
        } else {  // DwellMode::SysSuspend
        }
        delay(DWELL_TIME_STEP);  // Delay DWELL_TIME_STEP increment
    }
    delay(remainder);
    return true;
}

float hypot_f(float x, float y) {
    return sqrt(x * x + y * y);
}

char* trim(char* str) {
    char* end;
    while (::isspace((unsigned char)*str)) {
        str++;
    }
    if (*str == 0) {  // All spaces?
        return str;
    }
    end = str + ::strlen(str) - 1;
    while (end > str && ::isspace((unsigned char)*end)) {
        end--;
    }
    end[1] = '\0';
    return str;
}
