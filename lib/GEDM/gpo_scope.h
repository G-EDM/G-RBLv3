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



#ifndef GSCOPE_LOCK
#define GSCOPE_LOCK

#include <stdint.h>
#include <freertos/queue.h>
#include <atomic>

extern DRAM_ATTR std::atomic<bool> scope_batch_ready;

class GPO_SCOPE {

    private:
    
        void draw_scope_frame( void );
        IRAM_ATTR void draw_wave( void );
        float pwm_duty;      // duty cycle in percent
        float pwm_frequency; // frequency in khz

    public:

        GPO_SCOPE( void );

        IRAM_ATTR void set_meta_data( uint32_t sample_rate, uint8_t  motion_plan, uint32_t vfd_feedback, uint32_t cfd_fast_avg, uint32_t cfd_slow_avg, int misc_data );
        IRAM_ATTR bool add_to_scope( uint32_t adc_value, int8_t plan );
        IRAM_ATTR void draw_scope( void );
        IRAM_ATTR void reset( void );
        IRAM_ATTR bool is_blocked( void );
        IRAM_ATTR bool scope_is_running( void );

        void flush_frame( void );
        void start( void );
        void stop( void );
        void init( int16_t width, int16_t height, int16_t posx, int16_t posy );
        void update_config( int savg_size, int vdrop_thresh, int v_thresh, float duty_percent, float frequency, uint16_t smin, uint16_t smax );

};


extern GPO_SCOPE gscope;

#endif