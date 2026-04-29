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

#ifndef ARC_GENERATOR_H
#define ARC_GENERATOR_H


#include "definitions.h"
#include <stdio.h>
#include <map>
#include <atomic>
#include "widgets/language/en_us.h"


extern DRAM_ATTR std::atomic<bool> stop_sampling;

class ARC_GENERATOR {

    private:

        float pwm_duty_probing;
        int   pwm_frequency_probing;
        float pwm_duty_cycle_percent;
        float duty_percent_backup;
        int   pwm_duty_intern;
        int   pwm_frequency_intern;
        int   frequency_backup;
        int   pwm_pin;  // PWM+ output pin
        bool  has_isr;


    public:

        ARC_GENERATOR(void);
        ~ARC_GENERATOR(void);

        void IRAM_ATTR protection( int delay_us = 10000 );
        bool IRAM_ATTR get_pwm_is_off( void );

        void pwm_off( void );
        void pwm_on( void );
        void end( void );
        void start_ledc( void );
        void stop_ledc( void );
        void  create( void );
        bool  is_running(void);
        int   get_freq(void);
        float get_duty_percent(void);
        int   get_duty_internal( void );
        void  change_pwm_frequency(int freq);
        void  change_pwm_duty(int duty);
        void  recalculate_meta_data(void);
        void  disable_spark_generator(void);
        void  enable_spark_generator(void);
        void  update_duty(float duty_percent);
        void  probe_mode_on( void );
        void  probe_mode_off( void );

        int get_pulse_period_us();

        bool lock( void );
        void unlock( bool lock_taken = true );


        bool  change_setting(    setget_param_enum param_id, int int_value = 0, float float_value = 0.0 );
        int   get_setting_int(   setget_param_enum param_id );
        float get_setting_float( setget_param_enum param_id );
        bool  get_setting_bool(  setget_param_enum param_id );

};


extern ARC_GENERATOR arcgen;

#endif