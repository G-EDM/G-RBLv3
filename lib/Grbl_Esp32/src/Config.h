#pragma once

/*
  Config.h - compile time configuration
  Part of Grbl

  Copyright (c) 2012-2016 Sungeun K. Jeon for Gnea Research LLC
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
// This file contains compile-time configurations for Grbl's internal system. For the most part,
// users will not need to directly modify these, but they are here for specific needs, i.e.
// performance tuning or adjusting to non-typical machines.
// IMPORTANT: Any changes here requires a full re-compiling of the source code to propagate them.
/*
ESP 32 Notes
Some features should not be changed. See notes below.
*/

#ifndef GCONFIG_h
#define GCONFIG_h

#include <Arduino.h>
#include "NutsBolts.h"
#include "definitions.h"

#define UNDEFINED_PIN 255
const int    TOOL_LENGTH_OFFSET_AXIS    = 2;  // Default z-axis. Valid values are X_AXIS, Y_AXIS, or Z_AXIS.
const int    DWELL_TIME_STEP            = 50;  // Integer (1-255) (milliseconds)
const int    N_ARC_CORRECTION           = 12;  // Integer (1-255)
const double ARC_ANGULAR_TRAVEL_EPSILON = 5E-7;  // Float (radians)
#define DEFAULT_STEP_ENABLE_DELAY   0
#define DEFAULT_INVERT_PROBE_PIN    0  // $6 boolean
#define DEFAULT_JUNCTION_DEVIATION  0.01  // $11 mm
#define DEFAULT_ARC_TOLERANCE       0.002  // $12 mm
#define DEFAULT_HOMING_SQUARED_AXES 0
#define PROBE_PIN UNDEFINED_PIN
enum class Error : uint8_t {
    Ok                          = 0,
    ExpectedCommandLetter       = 1,
    BadNumberFormat             = 2,
    InvalidStatement            = 3,
    NegativeValue               = 4,
    SystemGcLock                = 9,
    SoftLimitError              = 10,
    Overflow                    = 11,
    TravelExceeded              = 15,
    InvalidJogCommand           = 16,
    GcodeUnsupportedCommand     = 20,
    GcodeModalGroupViolation    = 21,
    GcodeUndefinedFeedRate      = 22,
    GcodeCommandValueNotInteger = 23,
    GcodeAxisCommandConflict    = 24,
    GcodeWordRepeated           = 25,
    GcodeNoAxisWords            = 26,
    GcodeValueWordMissing       = 28,
    GcodeUnsupportedCoordSys    = 29,
    GcodeG53InvalidMotionMode   = 30,
    GcodeAxisWordsExist         = 31,
    GcodeNoAxisWordsInPlane     = 32,
    GcodeInvalidTarget          = 33,
    GcodeArcRadiusError         = 34,
    GcodeNoOffsetsInPlane       = 35,
    GcodeUnusedWords            = 36,
    GcodeG43DynamicAxisError    = 37,
    GcodeMaxValueExceeded       = 38,
    Eol                         = 111,
    JogCancelled                = 130,
};

#endif