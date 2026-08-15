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

#ifndef PHANTOM_GAXIS
#define PHANTOM_GAXIS

#include <stdint.h>


class float_member {
    public:
        float_member() : __value(0.0){};
        ~float_member(){};
        float get(){
            return __value;
        };
        bool set( float value ){
            __value = value;
            return true;
        };
    private:
        float __value;
};

class int_member {
    public:
        int_member() : __value(0){};
        ~int_member(){};
        int get(){
            return __value;
        };
        bool set( int value ){
            __value = value;
            return true;
        };
    private:
        int __value;
};

class bool_member {
    public:
        bool_member() : __value(false){};
        ~bool_member(){};
        bool get(){
            return __value;
        };
        bool set( bool value ){
            __value = value;
            return true;
        };
    private:
        bool __value;
};


class GAxis {
    public:
        GAxis( const char * __axis_name) : axis_name(__axis_name ){};
        ~GAxis(){};
        const char*  axis_name;
        float_member steps_per_mm;
        float_member max_travel_mm;
        float_member home_position;
        bool_member  is_homed;
        int_member   axis_index;
        int_member   step_pin;
        int_member   dir_pin;
        int_member   motor_type;
        bool_member  homing_seq_enabled;
};


#endif