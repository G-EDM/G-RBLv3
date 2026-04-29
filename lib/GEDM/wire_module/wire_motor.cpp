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

#include "wire_motor.h"

WIRE_MOTOR::WIRE_MOTOR( uint8_t index, const char* name ) : 
    running(false), 
    speed(0),
    steps_per_mm(0),
    motor_index( index ),
    motor_name( name ){}


bool WIRE_MOTOR::is_running(){
    return running;
}

int WIRE_MOTOR::get_speed(){
    return speed;
}


int WIRE_MOTOR::get_steps_per_mm(){
    return steps_per_mm;
}

int WIRE_MOTOR::set_steps_per_mm( int steps ){
    steps_per_mm = steps;
    return steps_per_mm;
}

void WIRE_MOTOR::set_dir_is_inverted( bool inverted ){
    dir_inverted = inverted;
}

bool WIRE_MOTOR::get_dir_is_inverted(){
    return dir_inverted;
}