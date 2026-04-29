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

#ifndef WIRE_MOTOR_H
#define WIRE_MOTOR_H

#include <cstdint>

enum WIRE_MOTOR_INDEXES {
    WIRE_PULL_MOTOR,
    WIRE_TENSION_MOTOR
};


class WIRE_MOTOR {

    public:

        WIRE_MOTOR( uint8_t index, const char* name );
        virtual ~WIRE_MOTOR() = default;

        virtual void  setup( uint8_t _step_pin = 0, uint8_t _dir_pin = 0, uint8_t address = 0 ) = 0;
        virtual void  init( void ) = 0;
        virtual void  backup() = 0;
        virtual bool  restore( void ) = 0; // restore backup stuff
        virtual void  reset( void ) = 0;   // reset
        virtual void  run( void ) = 0;
        virtual void  stop( void ) = 0;
        virtual bool  set_speed( int _speed, bool backup = true ) = 0;
        virtual bool  move( float distance, bool dir ) = 0;
        virtual int   mm_min_to_speed( float mm_min ) = 0;
        virtual float speed_to_mm_min( float speed ) = 0;

        int  get_speed( void );
        bool is_running( void );
        int  get_steps_per_mm( void );
        int  set_steps_per_mm( int steps );
        void set_dir_is_inverted( bool inverted = false );
        bool get_dir_is_inverted( void );
        
        virtual bool set( uint16_t param_id, uint16_t value ) = 0;
        virtual uint16_t get( uint16_t param_id ) = 0;

    protected:

        bool    running;
        int     speed;
        int     steps_per_mm;
        uint8_t motor_index;
        uint8_t address;
        bool    dir_inverted;

        const char* motor_name;
    //private:

};

#endif