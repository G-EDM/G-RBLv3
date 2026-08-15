#include "en_us.h"

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

setget_param_enum set_get_param_enum;
page_map          page_ids;

DRAM_ATTR std::map<int,setting_cache_object> settings_values_cache;

const char *data_sets_numeric[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", 0 };
const char *data_sets_alpha[]   = { "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "-", "_", "@", 0 };

std::map<info_codes, const char*> info_messages = {
    { NOT_FULLY_HOMED,      "*Machine needs to be fully homed!" },
    { SELECT_GCODE_FILE,    "*Please select a gcode file" },
    { AXIS_NOT_HOMED,       "*Axis needs to be homed!" },
    { TARGET_OUT_OF_REACH,  "*Position out of reach" },
    { MOTION_NOT_AVAILABLE, "*Motion is not enabled" },
    { Z_AXIS_DISABLED,      "*Z axis is disabled" }
};




//###########################################################
// Parameters added here will be stored in the settings files
// and also sued to restore the last session after boot
// Only float/bool/int paramaters are supported yet
//###########################################################
std::map<int,param_types> settings_file_param_types = {
    {PARAM_ID_SCOPE_SHOW_VFD,PARAM_TYPE_BOOL},
    {PARAM_ID_USE_HIGH_RES_SCOPE,PARAM_TYPE_BOOL},
    {PARAM_ID_FLUSHING_FLUSH_NOSPRK,PARAM_TYPE_BOOL},
    {PARAM_ID_SPINDLE_ENABLE,PARAM_TYPE_BOOL},
    {PARAM_ID_EARLY_RETR,PARAM_TYPE_BOOL},
    {PARAM_ID_DPM_UART,PARAM_TYPE_BOOL},
    {PARAM_ID_SPINDLE_ONOFF,PARAM_TYPE_BOOL}, 
    {PARAM_ID_PULLING_MOTOR_DIR_INVERTED,PARAM_TYPE_BOOL},
    {PARAM_ID_XYUV_JOG_COMBINED,PARAM_TYPE_BOOL},// ./bool section end
    {PARAM_ID_X_RES,PARAM_TYPE_FLOAT}, // steps/mm x
    {PARAM_ID_Y_RES,PARAM_TYPE_FLOAT}, // steps/mm y
    {PARAM_ID_Z_RES,PARAM_TYPE_FLOAT}, // steps/mm z
    {PARAM_ID_U_RES,PARAM_TYPE_FLOAT}, // steps/mm u
    {PARAM_ID_V_RES,PARAM_TYPE_FLOAT}, // steps/mm v
    {PARAM_ID_FREQ, PARAM_TYPE_FLOAT},
    {PARAM_ID_DUTY, PARAM_TYPE_FLOAT},
    {PARAM_ID_SETMIN, PARAM_TYPE_FLOAT},
    {PARAM_ID_SETMAX, PARAM_TYPE_FLOAT},
    {PARAM_ID_MAX_FEED, PARAM_TYPE_FLOAT},
    {PARAM_ID_SPINDLE_SPEED, PARAM_TYPE_FLOAT},
    {PARAM_ID_SPINDLE_SPEED_PROBING, PARAM_TYPE_FLOAT},
    {PARAM_ID_BROKEN_WIRE_MM,PARAM_TYPE_FLOAT},
    {PARAM_ID_I2S_RATE,PARAM_TYPE_FLOAT},
    {PARAM_ID_RETRACT_H_MM,PARAM_TYPE_FLOAT},
    {PARAM_ID_RETRACT_S_MM,PARAM_TYPE_FLOAT},
    {PARAM_ID_RETRACT_H_F,PARAM_TYPE_FLOAT},
    {PARAM_ID_RETRACT_S_F,PARAM_TYPE_FLOAT},
    {PARAM_ID_DPM_PROBE_V,PARAM_TYPE_FLOAT},
    {PARAM_ID_DPM_PROBE_C,PARAM_TYPE_FLOAT},
    {PARAM_ID_PROBE_FREQ,PARAM_TYPE_FLOAT},
    {PARAM_ID_PROBE_DUTY,PARAM_TYPE_FLOAT},
    {PARAM_ID_PROBE_TR_C,PARAM_TYPE_FLOAT},
    {PARAM_ID_REAMER_DIST,PARAM_TYPE_FLOAT},
    {PARAM_ID_REAMER_DURA,PARAM_TYPE_FLOAT},
    {PARAM_ID_FLUSHING_INTERVAL,PARAM_TYPE_FLOAT},
    {PARAM_ID_FLUSHING_DISTANCE,PARAM_TYPE_FLOAT}, 
    {PARAM_ID_SPEED_RAPID,PARAM_TYPE_FLOAT},
    {PARAM_ID_SPEED_INITIAL,PARAM_TYPE_FLOAT},
    {PARAM_ID_TOOLDIAMETER,PARAM_TYPE_FLOAT},//float section end
    {PARAM_ID_FLUSHING_FLUSH_OFFSTP,PARAM_TYPE_INT},
    {PARAM_ID_SHORT_DURATION, PARAM_TYPE_INT},
    {PARAM_ID_I2S_BUFFER_L,PARAM_TYPE_INT},
    {PARAM_ID_I2S_BUFFER_C,PARAM_TYPE_INT},
    {PARAM_ID_FAST_CFD_AVG_SIZE,PARAM_TYPE_INT},
    {PARAM_ID_RETRACT_CONF,PARAM_TYPE_INT},
    {PARAM_ID_MAX_REVERSE,PARAM_TYPE_INT},
    {PARAM_ID_SLOW_CFD_AVG_SIZE,PARAM_TYPE_INT},
    {PARAM_ID_VDROP_THRESH,PARAM_TYPE_INT},
    {PARAM_ID_VFD_THRESH,PARAM_TYPE_INT},
    {PARAM_ID_POFF_DURATION,PARAM_TYPE_INT},
    {PARAM_ID_EARLY_X_ON,PARAM_TYPE_INT},
    {PARAM_ID_LINE_ENDS,PARAM_TYPE_INT},
    {PARAM_ID_PROBE_TR_V,PARAM_TYPE_INT},
    {PARAM_ID_EDGE_THRESH,PARAM_TYPE_INT},
    //{PARAM_ID_SINK_DIR,PARAM_TYPE_INT},
    {PARAM_ID_MODE,PARAM_TYPE_INT}, 
    {PARAM_ID_MKS_MAXT,PARAM_TYPE_INT},
    {PARAM_ID_MKS_MAXC,PARAM_TYPE_INT},
    {PARAM_ID_SPINDLE_STEPS_PER_MM,PARAM_TYPE_INT}// ./int section end
};


#define TEXT_NONE_DUMMY ""

//###########################################################
// Tooltips: Order needs to be exactly the same as in the
// setget_param_enum enum object. Not a beauty..
//###########################################################
const char* tooltips[TOTAL_PARAM_ITEMS] = {
    TT_PARAM_ID_FREQ,
    TT_PARAM_ID_DUTY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,//bool
    TT_PARAM_ID_SETMIN,
    TT_PARAM_ID_SETMAX,
    TT_PARAM_ID_MAX_FEED,
    TT_PARAM_ID_SPINDLE_SPEED,
    TEXT_NONE_DUMMY, //bool
    TT_PARAM_ID_SHORT_DURATION,
    TT_PARAM_ID_BROKEN_WIRE_MM,
    TT_PARAM_ID_EDGE_THRESH,
    TT_PARAM_ID_I2S_RATE,
    TT_PARAM_ID_I2S_BUFFER_L,
    TT_PARAM_ID_I2S_BUFFER_C,
    TT_PARAM_ID_MAIN_AVG,
    TEXT_NONE_DUMMY, //bool
    TT_PARAM_ID_RETRACT_CONF,
    TT_PARAM_ID_MAX_REVERSE,
    TT_PARAM_ID_RETRACT_H_MM,
    TT_PARAM_ID_RETRACT_S_MM,
    TT_PARAM_ID_RETRACT_H_F,
    TT_PARAM_ID_RETRACT_S_F,
    TT_PARAM_ID_FAVG_SIZE,
    TT_PARAM_ID_VDROP_THRESH,
    TT_PARAM_ID_VFD_THRESH,
    TT_PARAM_ID_POFF_DURATION,
    TT_PARAM_ID_EARLY_X_ON,
    TT_PARAM_ID_LINE_ENDS,
    TEXT_NONE_DUMMY,  //bool
    TEXT_NONE_DUMMY, //bool
    TT_PARAM_ID_DPM_VOLTAGE,
    TT_PARAM_ID_DPM_CURRENT,
    TT_PARAM_ID_X_RES,
    TT_PARAM_ID_Y_RES,
    TT_PARAM_ID_Z_RES,
    TEXT_NONE_DUMMY, //none
    TT_PARAM_ID_DPM_PROBE_V,
    TT_PARAM_ID_DPM_PROBE_C,
    TT_PARAM_ID_PROBE_FREQ,
    TT_PARAM_ID_PROBE_DUTY,
    TT_PARAM_ID_PROBE_TR_V,
    TT_PARAM_ID_PROBE_TR_C,
    TEXT_NONE_DUMMY, //bool
    TT_PARAM_ID_REAMER_DIST,
    TT_PARAM_ID_REAMER_DURA,
    TT_PARAM_ID_SINK_DIST,
    TT_PARAM_ID_SIMULATION, //bool
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TT_PARAM_ID_FLUSHING_INTERVAL,
    TT_PARAM_ID_FLUSHING_DISTANCE,
    TT_PARAM_ID_FLUSHING_FLUSH_OFFSTP,
    TT_PARAM_ID_FLUSHING_FLUSH_NOSPRK,
    // this part is ugly
    // would be better to have params for different axis commands
    // separated and instead of using a function for each move have it merged into one function an pass an axis bitmask
    // but for now this is ok....
    TT_PARAM_ID_XRIGHT,
    TT_PARAM_ID_XLEFT,
    TT_PARAM_ID_YUP,
    TT_PARAM_ID_YDOWN,
    TT_PARAM_ID_ZUP,
    TT_PARAM_ID_ZDOWN,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TT_PARAM_ID_TOOLDIAMETER,
    TT_PARAM_ID_SPEED_RAPID,
    TEXT_NONE_DUMMY,
    TT_PARAM_ID_SPINDLE_SPEED_PROBING,
    TT_PARAM_ID_SPINDLE_STEPS_PER_MM,
    TT_PARAM_ID_SPINDLE_MOVE_UP,
    TT_PARAM_ID_SPINDLE_MOVE_DOWN,
    TT_PARAM_ID_MKS_MAXT,
    TT_PARAM_ID_MKS_MAXC,
    TT_PARAM_ID_MKS_CW,
    TT_PARAM_ID_MKS_CCW,
    TT_PARAM_ID_U_RES,
    TT_PARAM_ID_V_RES,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TT_PARAM_ID_URIGHT,
    TT_PARAM_ID_ULEFT,
    TT_PARAM_ID_VUP,
    TT_PARAM_ID_VDOWN,
    TEXT_NONE_DUMMY,
    TT_PARAM_ID_SPEED_INITIAL,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY,
    TEXT_NONE_DUMMY
};


