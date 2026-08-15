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

#include "mks42c_wire_motor.h"
#include "Config.h"
#include "System.h"
#include "widgets/language/en_us.h"
#include <float.h>

WIRE_MOTOR_MKS::WIRE_MOTOR_MKS( uint8_t index, const char* name ) : 
    SERVO42C(), 
    WIRE_MOTOR( index, name ) {
        motor_index = index;
    } // constructor



void WIRE_MOTOR_MKS::init(){
    debuglog( motor_name );vTaskDelay(DEBUG_LOG_DELAY);
    if( !set_defaults() ){
        debuglog("*  Connection failed!", DEBUG_LOG_DELAY );
        debuglog("   Tips:", DEBUG_LOG_DELAY );
        debuglog("   TX->RX and RX->TX", DEBUG_LOG_DELAY );
        debuglog("   Only 42C v1.1.2 boards are supported", DEBUG_LOG_DELAY );
        debuglog("   42C v1.0.0 boards do not work",1000);
    } else {
        debuglog("@  Communication success!", DEBUG_LOG_DELAY );
    }
}

void WIRE_MOTOR_MKS::setup( uint8_t _step_pin, uint8_t _dir_pin, uint8_t address ){ // step, dir pins ignored for uart motor

    backup_speed = MKS42C_MOTOR_SPEED_DEFAULT;
    speed        = MKS42C_MOTOR_SPEED_DEFAULT; // speed variable is not used, MKS42C class uses motor_speed variable
    dir_inverted = false;
    running      = false;

    begin( address ); // this does not write anything to the driver yet. It sets a few default values and creates the uart object if needed
                      // Slave address from 0-9.

}

bool WIRE_MOTOR_MKS::set_defaults(){

    debuglog("   Sending default config", DEBUG_LOG_DELAY );

    if( set_enable_mode( MKS42C_ENABLEMODE_DEFAULT ) ){

        set_zero_mode( MKS42C_ZEROMODE_DEFAULT );
        set_zero_mode_direction( MKS42C_ZEROMODE_DIR_DEFAULT );
        set_zero_mode_speed( MKS42C_ZEROMODE_SPEED_DEFAULT );
        set_zero_position();

        if( motor_index == WIRE_PULL_MOTOR ){
            set_max_current( MKS42C_MAXCURRENT_PULLMOTOR );
            set_max_torque( MKS42C_MAXTORQUE_PULLMOTOR );
        } else {
            set_max_current( get_setting_max_current() );
            set_max_torque( get_setting_max_torque() );
        }

        set_subdivision( MKS42C_MICROSTEPS_DEFAULT );
        set_subdivision_interpolation( MKS42C_ENABLEMICROSTEPS_DEFAULT );
        set_stop_motor();
        return true;

    } else {

        set_is_available( false );
        return false;

    }

}

bool WIRE_MOTOR_MKS::set( uint16_t param_id, uint16_t value ){

    bool success = false;

    switch ( param_id ){

        case PARAM_ID_MKS_MAXT:
            success = set_max_torque( value );
        break;

        case PARAM_ID_MKS_MAXC:
            success = set_max_current( value );
        break;
    
        default:
        break;

    }

    return success;

}
uint16_t WIRE_MOTOR_MKS::get( uint16_t param_id ){

    uint16_t value = 0;

    switch ( param_id ){

        case PARAM_ID_MKS_MAXT:
            value = get_setting_max_torque();
        break;

        case PARAM_ID_MKS_MAXC:
            value = get_setting_max_current();
        break;
    
        default:
        break;

    }

    return value;

}

void WIRE_MOTOR_MKS::stop(){
    if( !running ) return;
    set_stop_motor();
    set_zero_position();
    running = false;
}

void WIRE_MOTOR_MKS::run(){
    if( running ) return;
    uint8_t direction = dir_inverted ? 1 : 0;
    set_stop_motor();
    set_zero_position();
    set_run_continuous( direction, speed );
    running = true;
}

void WIRE_MOTOR_MKS::reset(){
    set_zero_position();
}

void WIRE_MOTOR_MKS::backup(){
    backup_speed = speed;
    set_zero_position();
}

bool WIRE_MOTOR_MKS::restore(){
    set_zero_position();
    return set_speed( backup_speed, false );
}

bool WIRE_MOTOR_MKS::set_speed( int _speed, bool backup ){
    if( speed == _speed ){ return true; } // speed already there
    speed = _speed;
    if( backup ){ backup_speed = speed; }
    set_mks_speed( _speed );
    if( running ){  // adjust speed on running motor...
        uint8_t direction = dir_inverted ? 1 : 0;
        //set_stop_motor();
        set_run_continuous( direction, speed );
    }
    return true;
}

//################################################################################
// Move the wire stepper in positive or negative direction a given distance. 
// With the MKS42C this should be accurate
//################################################################################
bool WIRE_MOTOR_MKS::move( float distance, bool dir ){
    if( dir_inverted ){
        dir = !dir;
    }
    int steps_to_do = round( distance * steps_per_mm ); // not very exact etc.. Depends on 256 microsteps
    return set_move_steps( dir, speed, steps_to_do, true ); // 
}

int WIRE_MOTOR_MKS::mm_min_to_speed( float mm_min ){
    float steps_per_rev = float( get_steps_per_rev() );
    if( steps_per_mm == 0 ) {
        debuglog("No steps per mm set!");
        return 0;
    }
    float mm_per_revolution = steps_per_rev / steps_per_mm; // wire moves this mm per motor revolution
    float vrpm              = mm_min / mm_per_revolution; // required revolutions per minute
    int   low               = 0;
    int   high              = 127;
    int   closest_speed     = 0;
    float min_diff          = FLT_MAX;
    while( low <= high ) { // binary lookup..
        int   mid        = (low + high) / 2;
        float rpm_at_mid = get_rpm_for_speed(mid);
        float diff       = fabs(rpm_at_mid - vrpm);
        if (diff < min_diff) {
            min_diff = diff;
            closest_speed = mid;
        }
        if (rpm_at_mid < vrpm) {
            low = mid + 1;
        } else if (rpm_at_mid > vrpm) {
            high = mid - 1;
        } else { break; }
    }
    if( closest_speed <= 0 && mm_min > 0.0 ){
        closest_speed = 1; // if a motion is wanted return the slowest possible motion
    }
    return closest_speed;
}

//################################################################################
// This function calculates the wire speed in mm/min based on 
// the motor settings and the configured steps per mm
// valid speed range is 0-127
//################################################################################
float WIRE_MOTOR_MKS::speed_to_mm_min( float _speed ){
    if( _speed < 0.0 ){ return 0.0; }
    int int_speed = round( _speed );
    if( int_speed > 127 ){ int_speed = 127; } // max speed is a 7bit value
    int_speed &= 0x7F; // redundant
    float vrpm              = get_rpm_for_speed( int_speed ); // rotations per minute at given speed
    float steps_per_rev     = float( get_steps_per_rev() );   // steps per rotation
    float mm_per_revolution = steps_per_rev / steps_per_mm;   // mm per full revolution
    float mm_min            = vrpm * mm_per_revolution;       // mm per minute
    return mm_min;
}


