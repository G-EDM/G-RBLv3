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

#ifndef WIRE_MODULE_H
#define WIRE_MODULE_H

#include <stdint.h>
#include <esp_attr.h>
#include "wire_motor.h"

//extern volatile DRAM_ATTR bool wire_is_running;

class WIRE_MODULE {

    public:

        WIRE_MODULE();
        ~WIRE_MODULE();

        WIRE_MOTOR* get_motor( int motor );

        void     create();
        void     init();
        void     delete_motors();
        void     delete_motor( uint8_t motor );
        void     create_pulling_motor( uint8_t _step_pin = -1, uint8_t _dir_pin = -1 );
        void     create_torque_motor();
        void     start( float _speed = -1.0 );
        void     stop();
        void     backup();
        void     restore();
        void     reset();
        bool     is_running();
        bool     set_speed( int motor, int speed );
        bool     move( int motor, float distance, bool dir = false );
        int      get_steps_per_mm( int motor );
        int      set_steps_per_mm( int motor, int steps_per_mm );
        int      convert_speed_from_mm_min( int motor, float _speed );
        float    get_speed_in_mm_min( int motor, int _speed = -1 );
        void     change_state( bool state );
        bool     motor_is_valid( int motor );
        bool     set_max_torque( int motor, uint16_t torque = 0 );
        bool     set_max_current( int motor, uint16_t current = 1200 );
        uint16_t get_setting_max_torque( int motor );
        uint16_t get_setting_max_current( int motor );

        void invert_dir( int motor, bool invert_dir );
        bool get_dir_is_inverted( int motor );

        bool     set( int motor, uint16_t param_id, uint16_t value = 0 );
        uint16_t get( int motor, uint16_t param_id );

    private:

        bool pulling_motor_enabled;
        bool torque_motor_enabled;
        bool pulling_type_normal;

};


extern WIRE_MODULE wire_feeder;

#endif