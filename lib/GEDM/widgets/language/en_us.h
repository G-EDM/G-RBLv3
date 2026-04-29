#pragma once

//############################################################################################################
/*

 ______     _______ ______  _______
|  ____ ___ |______ |     \ |  |  |
|_____|     |______ |_____/ |  |  |
___  _  _ ____ __ _ ___ ____ _  _
|--' |--| |--| | \|  |  [__] |\/|

This is a beta version for testing purposes.
Only for personal use. Commercial use or redistribution without permission is prohibited. 
Copyright (c) Roland Lautensack    


*/
//############################################################################################################

#ifndef PHANTOM_UI_LANG_EN
#define PHANTOM_UI_LANG_EN


#include <map>
#include <esp_attr.h>

#define BRAND_NAME   "GEDM"
#define RELEASE_NAME "Phantom"



enum param_types {
    PARAM_TYPE_NONE = 0,
    PARAM_TYPE_NONE_REDRAW_WIDGET,
    PARAM_TYPE_NONE_REDRAW_PAGE,
    PARAM_TYPE_BOOL_RELOAD_WIDGET,
    PARAM_TYPE_INT_RAW_RELOAD_WIDGET,
    PARAM_TYPE_INT,
    PARAM_TYPE_FLOAT,
    PARAM_TYPE_BOOL,
    PARAM_TYPE_STRING,
    PARAM_TYPE_INT_INCR,
    PARAM_TYPE_BOOL_RELOAD_WIDGET_OC
};

enum getset_commands_events {
    PARAM_GET      = 0,
    PARAM_SET      = 1,
    PARAM_SET_AXIS = 2
};

enum edm_operation_modes {
    MODE_NONE     = 0,
    MODE_SINKER   = 1,
    MODE_REAMER   = 2,
    MODE_2D_FLOAT = 3,
    MODE_2D_WIRE  = 4
};

enum page_map {
    PAGE_NONE,
    PAGE_FRONT,
    PAGE_MAIN_SETTINGS,
    NUMERIC_KEYBOARD,
    ALPHA_KEYBOARD,
    PAGE_MODES,
    PAGE_MOTION,
    PAGE_SD_CARD,
    PAGE_FILE_MANAGER,
    PAGE_FILE_MANAGER_GCODES,
    PAGE_ALARM,
    PAGE_IN_PROCESS,
    PAGE_INFO,
    PAGE_PROBING,
    PAGE_CONSOLE,
    TOTAL_PAGES
};

enum setget_param_axis {
    PARAM_JOG_AXIS,
    TOTAL_AXIS_PARAM_ITEMS
};

//###########################################################
// Order needs to be exactly the same as in the tooltips!!!
//###########################################################
enum setget_param_enum {
    PARAM_ID_FREQ,
    PARAM_ID_DUTY,
    PARAM_ID_MODE,
    PARAM_ID_MPOSX,
    PARAM_ID_MPOSY,
    PARAM_ID_MPOSZ,
    PARAM_ID_PWM_STATE,
    PARAM_ID_SETMIN,
    PARAM_ID_SETMAX,
    PARAM_ID_MAX_FEED,
    PARAM_ID_SPINDLE_SPEED,
    PARAM_ID_SPINDLE_ENABLE,
    PARAM_ID_SHORT_DURATION,
    PARAM_ID_BROKEN_WIRE_MM,
    PARAM_ID_EDGE_THRESH,
    PARAM_ID_I2S_RATE,
    PARAM_ID_I2S_BUFFER_L,
    PARAM_ID_I2S_BUFFER_C,
    PARAM_ID_FAST_CFD_AVG_SIZE,
    PARAM_ID_EARLY_RETR,
    PARAM_ID_RETRACT_CONF,
    PARAM_ID_MAX_REVERSE,
    PARAM_ID_RETRACT_H_MM,
    PARAM_ID_RETRACT_S_MM,
    PARAM_ID_RETRACT_H_F,
    PARAM_ID_RETRACT_S_F,
    PARAM_ID_SLOW_CFD_AVG_SIZE,
    PARAM_ID_VDROP_THRESH,
    PARAM_ID_VFD_THRESH,
    PARAM_ID_POFF_DURATION,
    PARAM_ID_EARLY_X_ON,
    PARAM_ID_LINE_ENDS,
    PARAM_ID_DPM_UART,
    PARAM_ID_DPM_ONOFF,
    PARAM_ID_DPM_VOLTAGE,
    PARAM_ID_DPM_CURRENT,
    PARAM_ID_X_RES,
    PARAM_ID_Y_RES,
    PARAM_ID_Z_RES,
    PARAM_ID_ESP_RESTART,
    PARAM_ID_DPM_PROBE_V,
    PARAM_ID_DPM_PROBE_C,
    PARAM_ID_PROBE_FREQ,
    PARAM_ID_PROBE_DUTY,
    PARAM_ID_PROBE_TR_V,
    PARAM_ID_PROBE_TR_C,
    PARAM_ID_SPINDLE_ONOFF,
    PARAM_ID_REAMER_DIST,
    PARAM_ID_REAMER_DURA,
    PARAM_ID_SINK_DIST,
    PARAM_ID_SIMULATION,
    PARAM_ID_SINK_DIR,
    PARAM_ID_PROBE_ACTION,
    PARAM_ID_UNSET_PROBE_POS,
    PARAM_ID_SET_PROBE_POS,
    PARAM_ID_GET_PROBE_POS,
    PARAM_ID_HOME_SEQ,
    PARAM_ID_HOME_AXIS,
    PARAM_ID_HOME_ENA,
    PARAM_ID_MOVE_TO_ORIGIN,
    PARAM_ID_MOTION_STATE,
    PARAM_ID_FLUSHING_INTERVAL,
    PARAM_ID_FLUSHING_DISTANCE,
    PARAM_ID_FLUSHING_FLUSH_OFFSTP,
    PARAM_ID_FLUSHING_FLUSH_NOSPRK,
    // this part is ugly
    // would be better to have params for different axis commands
    // separated and instead of using a function for each move have it merged into one function an pass an axis bitmask
    // but for now this is ok....
    PARAM_ID_XRIGHT,
    PARAM_ID_XLEFT,
    PARAM_ID_YUP,
    PARAM_ID_YDOWN,
    PARAM_ID_ZUP,
    PARAM_ID_ZDOWN,
    PARAM_ID_WPOSX,
    PARAM_ID_WPOSY,
    PARAM_ID_WPOSZ,
    PARAM_ID_SD_FILE_SET,
    PARAM_ID_SET_PAGE,
    PARAM_ID_SET_STOP_EDM,
    PARAM_ID_EDM_PAUSE,
    PARAM_ID_TOOLDIAMETER,
    PARAM_ID_SPEED_RAPID,
    PARAM_ID_HOME_ALL,
    PARAM_ID_SPINDLE_SPEED_PROBING,
    PARAM_ID_SPINDLE_STEPS_PER_MM,
    PARAM_ID_SPINDLE_MOVE_UP,
    PARAM_ID_SPINDLE_MOVE_DOWN,
    PARAM_ID_MKS_MAXT,
    PARAM_ID_MKS_MAXC,
    PARAM_ID_MKS_CW,
    PARAM_ID_MKS_CCW,
    PARAM_ID_U_RES,
    PARAM_ID_V_RES,
    PARAM_ID_WPOSU,
    PARAM_ID_WPOSV,
    PARAM_ID_MPOSU,
    PARAM_ID_MPOSV,
    PARAM_ID_URIGHT,
    PARAM_ID_ULEFT,
    PARAM_ID_VUP,
    PARAM_ID_VDOWN,
    PARAM_ID_XYUV_JOG_COMBINED,
    PARAM_ID_SPEED_INITIAL,
    PARAM_ID_USE_HIGH_RES_SCOPE,
    PARAM_ID_PULLING_MOTOR_DIR_INVERTED,
    PARAM_ID_SCOPE_SHOW_VFD,
    TOTAL_PARAM_ITEMS // needs to be the last element!
};

enum info_codes {
    INFO_NONE,
    NOT_FULLY_HOMED,
    AXIS_NOT_HOMED,
    SELECT_GCODE_FILE,
    TARGET_OUT_OF_REACH,
    MOTION_NOT_AVAILABLE,
    Z_AXIS_DISABLED
};

enum edm_stop_codes {
    STOP_CLEAN, 
    STOP_NO_FILE_GIVEN,
    STOP_SD_FAILED_TO_INIT
};


enum EVENT_TYPES {
    EVENT_TOUCH_PULSE = 0,
    EVENT_LOAD        = 1,
    RELOAD_D          = 2,
    BUILDSTRING       = 3,
    TOUCHED           = 4,
    CHECK_REQUIRES    = 5,
    ALARM             = 6,
    SHOW_PAGE         = 7,
    RELOAD_ON_CHANGE  = 8,
    SD_CHANGE         = 9,
    RESET             = 10,
    INFOBOX           = 11,
    RUN_SD_CARD_JOB   = 12,
    END_SD_CARD_JOB   = 13
};


enum OUPUT_TYPES {
    OUTPUT_TYPE_NONE   = 0,
    OUTPUT_TYPE_INT    = 1,
    OUTPUT_TYPE_FLOAT  = 2,
    OUTPUT_TYPE_BOOL   = 3,
    OUTPUT_TYPE_STRING = 4,
    OUTPUT_TYPE_CUSTOM = 5,
    OUTPUT_TYPE_TITLE  = 6
};





extern std::map<info_codes, const char*> info_messages;

struct setting_cache_object {
    int   intValue   = 0;
    float floatValue = 0.0;
    bool  boolValue  = false;
    setting_cache_object( int i = 0, float f = 0.0f, bool b = false ) : intValue(i), floatValue(f), boolValue(b) {}
};

extern DRAM_ATTR std::map<int,setting_cache_object> settings_values_cache;  // yeah.. typesafety and stuff int is ok

//#########################################################
// Those are just the settings that are stored on SD
// when reading/writing the settings file it will lookup 
// the valuetype here. If item does not exit it will not 
// get written to the sd settings
//#########################################################
extern std::map<int,param_types> settings_file_param_types;

extern const char* tooltips[TOTAL_PARAM_ITEMS];
extern const char *data_sets_numeric[];
extern const char *data_sets_alpha[];





#define DICT_SHOW_VFD            "Show vfd_channel in scope"
#define DICT_FREQUENCY           "Frequency"
#define DICT_DUTY                "Duty"
#define DICT_T_ON                "T_O N"
#define DICT_T_OFF               "T_OFF"
#define DICT_T_PERIOD            "Period"
#define DICT_DONE                "Done"
#define DICT_FREQ                "Freq"
#define DICT_DEL                 "DEL"
#define DICT_OK                  "OK"
#define DICT_CANCEL              "CANCEL"
#define DICT_SETMIN              "SetMin"
#define DICT_SETMAX              "SetMax"
#define DICT_MODE1               "Sinker EDM"
#define DICT_MODE2               "Reamer EDM"
#define DICT_MODE3               "Float EDM"
#define DICT_MODE4               "Wire EDM"
#define DICT_UNDEF               "Not found"
#define DICT_MAX_FEED            "Max Feedrate (mm/min)"
#define DICT_SETPOINT_MIN        "Setpoint Min (cFd %)"
#define DICT_SETPOINT_MAX        "Setpoint Max (cFd %)"
#define DICT_SPINDLE_SPEED       "Wire Speed (mm/min)"
#define DICT_SPINDLE_SPEED_B     "Wire Speed (mm/min)"
#define DICT_SPINDLE_SPM         "Spindle steps per mm"
#define DICT_SPINDLE_ENABLE      "Autoenable Spindle"
#define DICT_SPINDLE_DIR_INVERTED "Dir inverted"
#define DICT_SPINDLE_ONOFF       "Spindle START/STOP"
#define DICT_CLOSE               "Close"
#define DICT_SHORT_DURATION      "Shortduration (ms)"
#define DICT_BROKEN_WIRE_MM      "Brokenwire (mm)"
#define DICT_DRAW_LINEAR         "Draw linear"
#define DICT_VIEW_HIGH_RES       "HighResolution scope"
#define DICT_ETHRESH             "Spark threshold (ADC)"
#define DICT_I2S_RATE            "I2S rate (kSps)"
#define DICT_I2S_BUFFER_L        "I2S Buffer Length"
#define DICT_I2S_BUFFER_C        "I2S Buffer Count"
#define DICT_MAIN_AVG            "Fast cFd average (Samples)"
#define DICT_FAVG_SIZE           "Slow cFd average (Samples)"
#define DICT_EARLY_RETR_X        "Early retract exit"
#define DICT_RETR_CONFIRM        "Retractconfirm (Samples)"
#define DICT_MAX_REVERSE         "Retract limit (Lines)"
#define DICT_RETR_HARD_MM        "Retract Hard (mm)"
#define DICT_RETR_SOFT_MM        "Retract Soft (mm)"
#define DICT_RETR_HARD_F         "Retract Hard (mm/min)"
#define DICT_RETR_SOFT_F         "Retract Soft (mm/min)"
#define DICT_VDROP_THRESH        "vDrop threshold (ADC)"
#define DICT_VFD_THRESH          "vFd threshold (ADC)"
#define DICT_POFF_DURATION       "Pulse off duration"
#define DICT_EARLY_X_ON          "Early exit on (Plan)"
#define DICT_LINE_ENDS           "Line end confirms"
#define DICT_DPM_UART            "Enable RXTX"
#define DICT_DPM_ONOFF           "Power"
#define DICT_DPM_VOLTAGE         "Voltage"
#define DICT_DPM_CURRENT         "Current"
#define DICT_U_RES               "U steps per mm"
#define DICT_V_RES               "V steps per mm"
#define DICT_X_RES               "X steps per mm"
#define DICT_Y_RES               "Y steps per mm"
#define DICT_Z_RES               "Z steps per mm"
#define DICT_RESTART             "Restart ESP now"
#define DICT_DPM_PROBE_V         "DPM Probe Voltage"
#define DICT_DPM_PROBE_C         "DPM Probe Current"
#define DICT_PROBE_FREQ          "Probe Frequency"
#define DICT_PROBE_DUTY          "Probe Duty"
#define DICT_PROBE_TR_V          "vFd Trigger (ADC)"
#define DICT_PROBE_TR_C          "cFd Trigger (%)"
#define DICT_REAMER_DIS          "Travel distance (mm)"
#define DICT_REAMER_DUR          "Process duration (s)"
#define DICT_SINK_DISTANCE       "Cutting depth (mm)"
#define DICT_SIMULATION          "Simulate G-Code"
#define DICT_SINK_DIR            "Sinker axis"
#define DICT_SET_PROBE_CUR       "Set current as 0"
#define DICT_UNSET_PROBE         "Unset 0 positions"
#define DICT_PROBE_TAB           "Probing"
#define DICT_HOMING_TAB          "Homing"
#define DICT_PROBEPOS_X          "Probe X:"
#define DICT_PROBEPOS_Y          "Probe Y:"
#define DICT_PROBEPOS_Z          "Probe Z:"
#define DICT_NOTPROBE            "Not set"
#define DICT_FLUSH_INTV          "Flushing interval (s)"
#define DICT_FLUSH_DIST          "Flushing distance (mm)"
#define DICT_FLUSH_OFFSTP        "Flushing offset steps"
#define DICT_FLUSH_NOSPARK       "Disable spark for flush"
#define DICT_TOOLDIA             "Tooldiameter (mm)"
#define DICT_MKS_MAX_TORQUE      "Max torque"
#define DICT_MKS_MAX_CURRENT     "Max current (mA)"
#define DICT_SD_GCODE_LABEL      "GCode"
#define DICT_SD_SET_LABEL        "Settings"
#define DICT_PAGE_MODE_S         "Sinker/Drill - Settings"
#define DICT_PAGE_MODE_W         "2D Wire - Settings"
#define DICT_PAGE_MODE_R         "Reamer - Settings"
#define DICT_PAGE_FEEDBACK       "Settings - Feedback"
#define DICT_PAGE_SPEED          "Settings - Speed"
#define DICT_PAGE_PROBING        "Settings - Probing"
#define DICT_PAGE_PROT           "Settings - Protection"
#define DICT_PAGE_FBLOCK         "Settings - Forward Motion Blocker"
#define DICT_PAGE_RS             "Settings - Retraction Speed & Distance"
#define DICT_PAGE_R              "Settings - Retraction"
#define DICT_SPINDLE_PAGE_TITLE  "Settings - Spindle"
#define DICT_DPM_PAGE_TITLE      "Settings - DPM/DPH"
#define DICT_MOTOR_PAGE_TITLE    "Settings - Motor"
#define DICT_STOPGO_PAGE_TITLE   "Settings - Stop/Go Signals"
#define DICT_I2S_PAGE_TITLE      "Settings - I2S ADC (Advanced)"
#define DICT_BASIC_PAGE_TITLE    "Settings - Basic"
#define DICT_PAGE_FLUSHING       "Settings - Flushing"
#define DICT_PAGE_MKS            "Settings - Servo42c v1.1"

#define MM_MODE     "Mode"
#define MM_MODE_B   "Mode:"
#define MM_MOTION   "Motion"
#define MM_SD       "SD_Card"
#define ICON_COGS   "x"
#define NAV_UP      "@"
#define NAV_DOWN    "\\"
#define NAV_LEFT    "]"
#define NAV_RIGHT   "["
#define BTN_START   "ON"
#define BTN_STOP    "OFF"
#define MPOS_X      "X:"
#define MPOS_Y      "Y:"
#define MPOS_Z      "Z:"












#define TT_PARAM_ID_FREQ                 "PWM Frequency (kHz)\rChanging the frequency requires different\rsettings."
#define TT_PARAM_ID_DUTY                 "PWM Duty (%)\rHigher duty increases the discharge energy."
#define TT_PARAM_ID_SETMIN               "Setpoint Min (cFd)\rNo forward motion allowed while cFd\ror any cFd AVG is above this."
#define TT_PARAM_ID_SETMAX               "Setpoint Max (cFd)\rMostly deprecated. Should be high enough\rtoo only touch some cFd spikes."
#define TT_PARAM_ID_MAX_FEED             "Max feedrate in the EDM process in mm/min\rForward motion will not exceed this value\rexcept for the initial rapid."
#define TT_PARAM_ID_SPINDLE_SPEED        "Spindle speed (mm/min)"
#define TT_PARAM_ID_SHORT_DURATION       "Shortcircuit duration (ms)\rMilliseconds needed for a short circuit\rto trigger a pause"
#define TT_PARAM_ID_BROKEN_WIRE_MM       "Max no load distance in mm\rTravel forward x mm without load will trigger\ra pause. Used a max reverse distance too"
#define TT_PARAM_ID_EDGE_THRESH          "If cFd feedback peaks above this threshold\rit is considered a successful spark"
#define TT_PARAM_ID_I2S_RATE             "I2S Rate in kSps\rNever change within the process!"
#define TT_PARAM_ID_I2S_BUFFER_L         "I2S Buffer length\rNumber of readings inside the I2S batch\rNever change within the process!"
#define TT_PARAM_ID_I2S_BUFFER_C         "I2S Buffer count\rNumber of buffers containing samples\rNever change within the process!"
#define TT_PARAM_ID_PROBE_TR_C           "cFd threshold in %\rIf cFd rises given \% it triggers a\rpositive probe."
#define TT_PARAM_ID_PROBE_TR_V           "vFd threshold in ADC (0-4095)\rIf vFd drops below this value it is \rconsidered a positive probe."
#define TT_PARAM_ID_PROBE_DUTY           "PWM duty used for probing"
#define TT_PARAM_ID_PROBE_FREQ           "PWM frequency used for probing"
#define TT_PARAM_ID_DPM_PROBE_C          "DPM current used for probing\rRequires DPM support enabled."
#define TT_PARAM_ID_DPM_PROBE_V          "DPM voltage used for probing\rRequires DPM support enabled."
#define TT_PARAM_ID_DPM_VOLTAGE          "Set the DPM voltage\rRequires DPM support enabled."
#define TT_PARAM_ID_DPM_CURRENT          "Set the DPM current\rRequires DPM support enabled."
#define TT_PARAM_ID_RETRACT_H_MM         "Hard retraction distance in mm"
#define TT_PARAM_ID_RETRACT_S_MM         "Soft retraction distance in mm"
#define TT_PARAM_ID_RETRACT_H_F          "Hard retraction speed in mm/min"
#define TT_PARAM_ID_RETRACT_S_F          "Soft retraction speed in mm/min"
#define TT_PARAM_ID_VDROP_THRESH         "Short circuit vFd threshold\rIf vFd drops below this threshold it is\rconsidered a real short circuit"
#define TT_PARAM_ID_LINE_ENDS            "Line end confirmations\rConfirm the feedback x times before\rentering the next line"
#define TT_PARAM_ID_MAX_REVERSE          "Max reverse lines\rMax number of full lines allowed to\rmove back in history"
#define TT_PARAM_ID_VFD_THRESH           "Forward motion is blocked if voltage (vFd)\ris above this value"
#define TT_PARAM_ID_POFF_DURATION         "PWM off duration\rTime the PWM is turned off for\rprotection or to break arcs."
#define TT_PARAM_ID_FAVG_SIZE             "Slow cFd average size\rNumber of readings from the multisampler\rto form the fAVG"
#define TT_PARAM_ID_MAIN_AVG              "Fast cFd average size\rNumber of readings from the multisampler\rto form the main AVG"
#define TT_PARAM_ID_RETRACT_CONF          "Retraction confirmation\rConfirm x times before actually\rretracting"
#define TT_PARAM_ID_X_RES                 "X axis steps per mm\rUse for testing only"
#define TT_PARAM_ID_Y_RES                 "Y axis steps per mm\rUse for testing only"
#define TT_PARAM_ID_Z_RES                 "Z axis steps per mm\rUse for testing only"
#define TT_PARAM_ID_U_RES                 "U axis steps per mm\rUse for testing only"
#define TT_PARAM_ID_V_RES                 "V axis steps per mm\rUse for testing only"
#define TT_PARAM_ID_EARLY_X_ON            "Early exit on (plan 1-4)\rExit a retraction on given plan.\rHigher value = more aggressive"
#define TT_PARAM_ID_REAMER_DIST           "Travel distance (mm)\rRange of motion for the reamer\roscillation"
#define TT_PARAM_ID_REAMER_DURA           "Process duration(s)\rReamer oscillation duration in seconds.\r0 = run until stopped"
#define TT_PARAM_ID_SINK_DIST             "Cutting depth (mm)\rDistance the axis will travel does not\requal the true cutting depth"
#define TT_PARAM_ID_SIMULATION            "Simulate G-Code\rXY axis will still move but it will skip\rfeedback controls and Z axis motion"
#define TT_PARAM_ID_FLUSHING_DISTANCE     "Flushing retraction distance\rAxis will retract given distance for flushing\rIt will not overshoot home"
#define TT_PARAM_ID_FLUSHING_INTERVAL     "Flushing interval in seconds\rAxis will perform a flushing retraction\revery x seconds. 0=No flushing"
#define TT_PARAM_ID_FLUSHING_FLUSH_OFFSTP "Flushing offset steps\rAxis will return to pre flush position\rminus this number of steps"
#define TT_PARAM_ID_FLUSHING_FLUSH_NOSPRK "Disable spark for flushing\rIf enabled it will turn PWM off while retracting"
#define TT_PARAM_ID_XRIGHT                "Jog given mm to the right"
#define TT_PARAM_ID_XLEFT                 "Jog given mm to the left"
#define TT_PARAM_ID_YUP                   "Jog given mm back"
#define TT_PARAM_ID_YDOWN                 "Jog given mm forward"
#define TT_PARAM_ID_ZUP                   "Jog given mm up"
#define TT_PARAM_ID_ZDOWN                 "Jog given mm down"
#define TT_PARAM_ID_URIGHT                "Jog given mm to the right"
#define TT_PARAM_ID_ULEFT                 "Jog given mm to the left"
#define TT_PARAM_ID_VUP                   "Jog given mm back"
#define TT_PARAM_ID_VDOWN                 "Jog given mm forward"
#define TT_PARAM_ID_TOOLDIAMETER          "Tooldiameter (mm)\rUsed to get the correct the probing\rposition"
#define TT_PARAM_ID_SPEED_RAPID           "Rapid speed (mm/min)\rMaximum axis speed used for\rrapids and jogs"
#define TT_PARAM_ID_SPEED_INITIAL         "Initial speed (mm/min)\rSpeed used until first contact\rwas made"
#define TT_PARAM_ID_SPINDLE_SPEED_PROBING "Probing wire speed (mm/min)\rWire speed while probing"
#define TT_PARAM_ID_SPINDLE_STEPS_PER_MM  "Steps per mm\rSteps needed to move the wire 1 mm"
#define TT_PARAM_ID_SPINDLE_MOVE_UP       "Pull wire given mm in"
#define TT_PARAM_ID_SPINDLE_MOVE_DOWN     "Push wire given mm out"
#define TT_PARAM_ID_MKS_MAXT              "Max torque (0-1200)"
#define TT_PARAM_ID_MKS_MAXC              "Max current in mA (0-2500)"
#define TT_PARAM_ID_MKS_CW                "Move MKS42C clockwise (mm)"
#define TT_PARAM_ID_MKS_CCW               "Move MKS42C counter clockwise (mm)\rNot reacting to motionswitch yet!"

#define PARAM_ID_PAGE_NAV_LEFT  18 // 
#define PARAM_ID_PAGE_NAV_RIGHT 19 // 
#define PARAM_ID_PAGE_NAV_CLOSE 20 // 




#endif