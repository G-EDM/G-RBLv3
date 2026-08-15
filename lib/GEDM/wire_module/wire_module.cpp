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

#include "wire_module.h"
#include "pwm_wire_motor.h"
#include "mks42c_wire_motor.h"
#include "definitions.h"
#include <freertos/portmacro.h>
#include <atomic>

#ifndef A_STEP_PIN
    #define A_STEP_PIN -1
#endif

#ifndef A_DIRECTION_PIN
    #define A_DIRECTION_PIN -1
#endif

std::atomic<bool> wire_running(false);

//volatile DRAM_ATTR bool wire_is_running = false;
static const int total_motors = 2;

portMUX_TYPE wire_module_mutex = portMUX_INITIALIZER_UNLOCKED;

WIRE_MODULE wire_feeder;

WIRE_MOTOR* wire_motors[total_motors];

WIRE_MODULE::WIRE_MODULE() : pulling_motor_enabled( false ), 
                             torque_motor_enabled( false ) {}

WIRE_MODULE::~WIRE_MODULE(){ // not needed..
    delete_motors();
}

void WIRE_MODULE::delete_motors(){
    for(int i=0; i<total_motors; i++){
        delete_motor( i );
    }
}

void WIRE_MODULE::delete_motor( uint8_t motor ){
    if( motor >= total_motors || !wire_motors[motor] || wire_motors[motor] == nullptr ){
        return;
    }
    delete wire_motors[motor];
    wire_motors[motor] = nullptr;
}

//####################################################################
// Create the motors and prepare stuff
//####################################################################
void WIRE_MODULE::create(){
    debuglog("Creating wire module", DEBUG_LOG_DELAY );
    delete_motors();

    #ifdef ENABLE_PWM_PULLING_MOTOR
        create_pulling_motor( A_STEP_PIN, A_DIRECTION_PIN );
    #endif

    #ifdef ENABLE_SERVO42C_PULLING_MOTOR
        create_pulling_motor();
    #endif

    #ifdef ENABLE_SERVO42C_TORQUE_MOTOR
        create_torque_motor();
    #endif
    change_state( false );
}

//####################################################################
// 
//####################################################################
void WIRE_MODULE::init(){
    vTaskDelay(DEBUG_LOG_DELAY);
    for( int i = 0; i < total_motors; ++i ){
        if( wire_motors[i] ){
            wire_motors[i]->init();
            vTaskDelay(DEBUG_LOG_DELAY);
        }
    }

    /*for( int i = 0; i < 128; ++i ){
        std::string text;
        std::string textb;
        textb = float_to_char( wire_motors[WIRE_TENSION_MOTOR]->speed_to_mm_min( i ) );
        text = int_to_char( i );
        debuglog( text.c_str() );
        debuglog( textb.c_str() );
        vTaskDelay(1000);
    }*/
}

//####################################################################
// Flag the wire is running state
//####################################################################
void WIRE_MODULE::change_state( bool state ){
    wire_running.store( state, std::memory_order_acquire );
}

//####################################################################
// Returns true if wire is running
//####################################################################
bool WIRE_MODULE::is_running(){
    return wire_running.load( std::memory_order_acquire );
}

//####################################################################
// Create the pulling side motor
//####################################################################
void WIRE_MODULE::create_pulling_motor( uint8_t _step_pin, uint8_t _dir_pin ){

    debuglog("Setup pulling motor", DEBUG_LOG_DELAY );

    delete_motor( WIRE_PULL_MOTOR ); // if called twice let's create a new motor

    #ifdef ENABLE_PWM_PULLING_MOTOR
        debuglog("   PWM motor", DEBUG_LOG_DELAY );
        // TMC2209 pulling stepper driven with step dir signals from the motionboard (A)
        wire_motors[WIRE_PULL_MOTOR] = new WIRE_MOTOR_PWM( WIRE_PULL_MOTOR, "@Pulling motor" );
        wire_motors[WIRE_PULL_MOTOR]->setup( _step_pin, _dir_pin );
        wire_motors[WIRE_PULL_MOTOR]->set_steps_per_mm( WIRE_PULL_MOTOR_STEPS_PER_MM );
        wire_motors[WIRE_PULL_MOTOR]->set_dir_is_inverted( WIRE_PULLING_MOTOR_DIR_INVERTED );
        wire_motors[WIRE_PULL_MOTOR]->set_speed( wire_motors[WIRE_PULL_MOTOR]->mm_min_to_speed( DEFAULT_WIRE_SPEED_MM_MIN ) );
        pulling_motor_enabled = true;
        return;
    #endif

    #ifdef ENABLE_SERVO42C_PULLING_MOTOR
        debuglog("   UART motor", DEBUG_LOG_DELAY );
        // MKS42C pulling stepper driven via UART
        wire_motors[WIRE_PULL_MOTOR] = new WIRE_MOTOR_MKS( WIRE_PULL_MOTOR, "@Pulling motor" );
        wire_motors[WIRE_PULL_MOTOR]->setup( PIN_NONE, PIN_NONE, MKS_PULLING_MOTOR_UART_ADDRESS ); // no step dir pins here
        wire_motors[WIRE_PULL_MOTOR]->set_steps_per_mm( WIRE_PULL_MOTOR_STEPS_PER_MM );
        wire_motors[WIRE_PULL_MOTOR]->set_dir_is_inverted( WIRE_PULLING_MOTOR_DIR_INVERTED );
        wire_motors[WIRE_PULL_MOTOR]->set_speed( wire_motors[WIRE_PULL_MOTOR]->mm_min_to_speed( DEFAULT_WIRE_SPEED_MM_MIN ) );
        pulling_motor_enabled = true;
    #endif

    vTaskDelay(DEBUG_LOG_DELAY);

}

//####################################################################
// Create the tension control motor if enabled
// Only UART MKS42C for this one
//####################################################################
void WIRE_MODULE::create_torque_motor(){

    delete_motor( WIRE_TENSION_MOTOR ); // if called twice let's create a new motor

    #ifdef ENABLE_SERVO42C_TORQUE_MOTOR
        // MKS42C stepper driven via UART for wire tension..
        wire_motors[WIRE_TENSION_MOTOR] = new WIRE_MOTOR_MKS( WIRE_TENSION_MOTOR, "@Tension motor" );
        wire_motors[WIRE_TENSION_MOTOR]->setup( PIN_NONE, PIN_NONE, MKS_TORQUE_MOTOR_UART_ADDRESS ); // step, dir pin set to 0, only address is needed
        wire_motors[WIRE_TENSION_MOTOR]->set_steps_per_mm( MKS42C_TORQUE_MOTOR_STEPS_PER_MM );
        torque_motor_enabled = true;
    #endif

}

//####################################################################
// Returns a motor ( WIRE_PULL_MOTOR / WIRE_TENSION_MOTOR )
// Returns a nullptr if motor is not created
//####################################################################
WIRE_MOTOR* WIRE_MODULE::get_motor( int motor ){
    return wire_motors[motor];
}

//####################################################################
// Start the wire; _speed is for the pulling motor; if _speed >= 0
// it will set the speed before starting the wire
//####################################################################
void WIRE_MODULE::start( float _speed ){ // speed in mm/min
    //if( !pulling_motor_enabled || is_running() ) return;
    //debuglog("Starting wire", DEBUG_LOG_DELAY );
    for( int i = 0; i < total_motors; ++i ){
        if( i != WIRE_PULL_MOTOR ) continue; // only pulling motor
        if( wire_motors[i] ){
            if( _speed > 0.0 ){
                // convert speed to motor specific unit (HZ for PWM and 1-127 for MKS)
                wire_motors[i]->set_speed( wire_motors[i]->mm_min_to_speed( _speed ), false );
            }
            wire_motors[i]->run();
        }
    }
    change_state( true );
}

//####################################################################
// Stop the wire
//####################################################################
void WIRE_MODULE::stop(){
    //if( !pulling_motor_enabled || !is_running() ) return;
    //debuglog("Stopping wire module", DEBUG_LOG_DELAY );
    for( int i = 0; i < total_motors; ++i ){
        if( !motor_is_valid(i) ) continue;
        if( wire_motors[i] ){
            wire_motors[i]->stop();
        }
    }
    change_state( false );
}

//####################################################################
// Backup speeds before probing
//####################################################################
void WIRE_MODULE::backup(){
    //debuglog("Backup wire module", DEBUG_LOG_DELAY );
    for( int i = 0; i < total_motors; ++i ){
        if( i != WIRE_PULL_MOTOR ) continue; // only pulling motor
        if( wire_motors[i] ){
            wire_motors[i]->backup();
        }
    }
    //debuglog("   Done", DEBUG_LOG_DELAY );
}

//####################################################################
// Restore speeds after probing
//####################################################################
void WIRE_MODULE::restore(){
    //debuglog("Restore wire module", DEBUG_LOG_DELAY );
    for( int i = 0; i < total_motors; ++i ){
        if( i != WIRE_PULL_MOTOR ) continue; // only pulling motor
        if( wire_motors[i] ){
            wire_motors[i]->restore();
        }
    }
    //debuglog("   Done", DEBUG_LOG_DELAY );
}

//####################################################################
// Reset.. Needed??
//####################################################################
void WIRE_MODULE::reset(){
    //debuglog("Reset wire module", DEBUG_LOG_DELAY );
    for( int i = 0; i < total_motors; ++i ){
        if( wire_motors[i] ){
            wire_motors[i]->reset();
        }
    }
    //debuglog("   Done", DEBUG_LOG_DELAY );
}

//####################################################################
// Check if a given motor_id is valid and available
//####################################################################
bool WIRE_MODULE::motor_is_valid( int motor ){
    if( motor < 0 || motor >= total_motors ) return false;
    if( ( motor == WIRE_PULL_MOTOR && pulling_motor_enabled ) || ( motor == WIRE_TENSION_MOTOR && torque_motor_enabled ) ){
        return wire_motors[motor] != nullptr;
    } return false;
}

//####################################################################
// Set the speed for a given motor
//####################################################################
bool WIRE_MODULE::set_speed( int motor, int speed ){
    if( motor_is_valid( motor ) ){
        //debuglog("Setting wire speed", DEBUG_LOG_DELAY );
        return wire_motors[motor]->set_speed( speed, false );
    } return false;
}

//####################################################################
// Move given distance in given direction with given motor
//####################################################################
bool WIRE_MODULE::move( int motor, float distance, bool dir_negative ){
    if( is_running() ) stop();
    if( motor_is_valid( motor ) ){
        return wire_motors[motor]->move(distance, dir_negative);
    } return false;
}

//####################################################################
// Get steps per mm setting for given motor
//####################################################################
int WIRE_MODULE::get_steps_per_mm( int motor ){
    if( motor_is_valid( motor ) ){
        return wire_motors[motor]->get_steps_per_mm();
    } return -1;
}

//####################################################################
// Set steps per mm for given motor
//####################################################################
int WIRE_MODULE::set_steps_per_mm( int motor, int steps_per_mm ){
    if( motor_is_valid( motor ) ){
        return wire_motors[motor]->set_steps_per_mm( steps_per_mm );
    } return -1;
}

//####################################################################
// Get speed for motor in mm/min
// if speed <= 0 it will return the current speed that is set
//####################################################################
float WIRE_MODULE::get_speed_in_mm_min( int motor, int _speed ){
    if( motor_is_valid( motor ) ){
        return wire_motors[motor]->speed_to_mm_min( _speed <= 0 ? wire_motors[motor]->get_speed() : _speed );
    } return -1.0;
}

//####################################################################
// Convert speed for motor from mm/min to int
//####################################################################
int WIRE_MODULE::convert_speed_from_mm_min( int motor, float _speed ){
    if( motor_is_valid( motor ) ){
        return wire_motors[motor]->mm_min_to_speed( _speed );
    } return -1;
}

//####################################################################
// More or less MKS42C motor specific getter and setter functions
// Set max current, torque etc.
//####################################################################
bool WIRE_MODULE::set( int motor, uint16_t param_id, uint16_t value ){
    if( motor_is_valid( motor ) ){
        return wire_motors[motor]->set( param_id, value );
    } return false;
}
uint16_t WIRE_MODULE::get( int motor, uint16_t param_id ){
    if( motor_is_valid( motor ) ){
        return wire_motors[motor]->get( param_id );
    } return 0;
}

void WIRE_MODULE::invert_dir( int motor, bool invert_dir ){
    if( motor_is_valid( motor ) ){
        wire_motors[motor]->set_dir_is_inverted( invert_dir );
        wire_motors[motor]->reset();
    } return;
}


bool WIRE_MODULE::get_dir_is_inverted( int motor ){
    if( motor_is_valid( motor ) ){
        return wire_motors[motor]->get_dir_is_inverted();
    } return false;
}