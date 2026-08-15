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

#ifndef WIRE_MOTOR_PWM_H
#define WIRE_MOTOR_PWM_H

#include "wire_motor.h"

class WIRE_MOTOR_PWM : public WIRE_MOTOR {

    public:
        WIRE_MOTOR_PWM( uint8_t index, const char* name );
        ~WIRE_MOTOR_PWM() override {};

        void  setup( uint8_t _step_pin = 0, uint8_t _dir_pin = 0, uint8_t address = 0 );
        void  init( void );
        void  backup();
        bool  restore( void );
        void  reset( void );
        void  run( void );
        void  stop( void );
        bool  set_speed( int _speed, bool backup = true );
        bool  move( float distance, bool dir );
        int   mm_min_to_speed( float mm_min );
        float speed_to_mm_min( float _speed );

        bool     set( uint16_t param_id, uint16_t value ){ return true; }
        uint16_t get( uint16_t param_id ){ return 0; }

    private:

        int pwm_pin;
        int dir_pin;
        int backup_speed;
        bool enabled;

};

#endif