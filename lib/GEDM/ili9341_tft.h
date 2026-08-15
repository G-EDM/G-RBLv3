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



#include <SPI.h>
#include "sensors.h"
#include "pwm_controller.h"
#include "api.h"
#include "probing_routines.h"
#include "gedm_planner/Planner.h"
#include "widgets/widget_item.h"
#include "widgets/char_helpers.h"
#include "wire_module/wire_module.h"

using setter_param      = std::pair<int, const char*>;
using getter_param      = PhantomSettingsPackage;
using setter_param_axis = PhantomSettingsPackage;

extern TaskHandle_t ui_task_handle;

extern void IRAM_ATTR ui_task( void * parameter );
extern void IRAM_ATTR remote_control( int data = 0 );

extern void update_scope_stats();
extern float clamp_float( float value, float min, float max );
extern int clamp_int( int value, int min, int max );


class GUI : 

    public PhantomWidgetItem,
    public G_EDM_PROBE_ROUTINES {

    public:

        GUI();
        ~GUI(){};

        void setup_events( void );
        void run_once_if_ready( void );
        void start_ui_task( void );
        void sinker_drill_single_axis( void );
        void wire_gcode_task( void );
        void pre_process_defaults( void );
        void reset_defaults( void );
        void set_operation_mode( int mode );
        void start_process(void);
        void reset_after_job(void);
        void alarm_handler( void );
        int  change_sinker_axis_direction( int direction );
        bool move_sinker_axis_to_zero( void );
        bool get_is_ready( void );
        bool ready_to_run( void );
        bool motion_input_is_blocked(void);
        bool probe_prepare( void );
        void probe_done( void );

        bool    has_gcode_file;
        int     single_axis_mode_direction;
        int     probe_dimension = 0;
        uint8_t active_page;

};

extern GUI ui_controller;