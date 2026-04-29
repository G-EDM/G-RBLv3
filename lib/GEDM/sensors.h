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

#ifndef GSENSE_LOCK
#define GSENSE_LOCK


#include "definitions.h"
#include "api.h"
#include "widgets/language/en_us.h"
#include "settings_interface.h"
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/timer.h>
#include <esp32-hal-gpio.h>

// buffer size needs to be power of two: 2,4,8,16,32,64,128,256...
static const int csense_sampler_buffer_size = 64;

enum sense_queue_commands {
    SENSE_LOAD_SAMPLES = 0,
    SENSE_BENCHMARK    = 1
};

enum motion_plans {
    MOTION_PLAN_ZERO       = 0,
    MOTION_PLAN_FORWARD    = 1,
    MOTION_PLAN_HOLD_SOFT  = 2,
    MOTION_PLAN_HOLD_HARD  = 3,
    MOTION_PLAN_SOFT_SHORT = 4,
    MOTION_PLAN_HARD_SHORT = 5,
    MOTION_PLAN_TIMEOUT    = 6
};

enum i2s_states {
    I2S_CTRL_IDLE = 0,
    I2S_RESTARTING,
    I2S_CTRL_NOT_AVAILABLE
};

extern DRAM_ATTR std::atomic<bool> motion_switch_changed;  // motion on/off state changed

extern xQueueHandle remote_control_queue;
extern xQueueHandle adc_read_trigger_queue; 

extern IRAM_ATTR bool sensors_task_running( void );
extern IRAM_ATTR int  get_calculated_motion_plan( bool enforce_fresh = false );
extern int   percentage_to_adc( float percentage );
extern float adc_to_percentage( int adc_value );


class G_SENSORS : public SETTINGS_INTERFACE {

    private:

        bool has_init;

        int  mclk;
        int  sample_rate;
        int  buffer_count;

        void create_queues( void );
        void create_tasks( void );

    public:

        G_SENSORS();
        ~G_SENSORS();

        IRAM_ATTR void       wait_for_idle( bool aquire_lock = false );
        IRAM_ATTR void       set_i2s_state( i2s_states state );
        IRAM_ATTR bool       is_state( i2s_states state );
        IRAM_ATTR i2s_states get_i2s_state( void );

        bool refresh_settings( void );
        void init_settings( void );
        void reset_feedback_data( void );
        void setup( void );
        void create_sensors( void );
        void sensor_end( void );
        void stop( void );
        void begin( void );
        void restart( void );
        void reset_sensor_global( void );
        int  set_sample_rate( int rate );

        static bool IRAM_ATTR motion_switch_read( void );
        static bool IRAM_ATTR unlock_motion_switch( void );
        static bool IRAM_ATTR limit_switch_read( void );

        void notify_change( setget_param_enum param_id ) override;

};

extern G_SENSORS gsense;

#endif