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

#include "pwm_wire_motor.h"
#include <driver/mcpwm.h>
//#include <esp32-hal-gpio.h>
#include "Config.h"
#include "System.h"

volatile int total_steps_do = 0;
void IRAM_ATTR wire_pwm_isr( void ){
    if( total_steps_do <= 0 ){ return; }
    --total_steps_do;
}


WIRE_MOTOR_PWM::WIRE_MOTOR_PWM( uint8_t index, const char* name ) : 
    WIRE_MOTOR( index, name ) {
        motor_index = index;
    }

void WIRE_MOTOR_PWM::init(){
    debuglog( motor_name, DEBUG_LOG_DELAY );
    if( !enabled ){
        debuglog("*  Invalid configuration (GPIOs)!", DEBUG_LOG_DELAY );
    } else {
        if( dir_inverted ){
            digitalWrite( dir_pin, HIGH ); 
        }
    }
}


void WIRE_MOTOR_PWM::setup( uint8_t _step_pin, uint8_t _dir_pin, uint8_t address ){ // address ignored for PWM motor
   
    pwm_pin       = _step_pin;
    dir_pin       = _dir_pin;
    backup_speed  = mm_min_to_speed( DEFAULT_WIRE_SPEED_MM_MIN );
    speed         = mm_min_to_speed( DEFAULT_WIRE_SPEED_MM_MIN );
    dir_inverted  = WIRE_PULLING_MOTOR_DIR_INVERTED;

    if( dir_pin == PIN_NONE || pwm_pin == PIN_NONE || dir_pin == -1 || pwm_pin == -1 ){
        // problem
        enabled = false; // ignore motor but still list it
        running = false;
        return;
    }

    enabled = true;

    pinMode(dir_pin, OUTPUT);
    digitalWrite(dir_pin, LOW);

    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, pwm_pin);
    mcpwm_config_t pwm_config = {};
    pwm_config.frequency      = speed;
    pwm_config.cmpr_a         = 0;
    pwm_config.cmpr_b         = 0;
    pwm_config.counter_mode   = MCPWM_UP_COUNTER;
    pwm_config.duty_mode      = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
    running = true;
    stop();

}

void WIRE_MOTOR_PWM::stop(){
    if( ! running ) return;
    //mcpwm_set_frequency( MCPWM_UNIT_0, MCPWM_TIMER_2, 0 );
    if( enabled ){
        mcpwm_set_duty( MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0.0 );
    }
    running = false;
}

void WIRE_MOTOR_PWM::run(){
    if( running ) return; // already running
    int __speed = 0;      // soft start initial speed
    if( enabled ){
        mcpwm_set_frequency( MCPWM_UNIT_0, MCPWM_TIMER_0, 1 );
        mcpwm_set_duty( MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 50.0 );
        while( __speed < speed ){
            if( get_quit_motion() ) break;
            __speed += 1;
            mcpwm_set_frequency( MCPWM_UNIT_0, MCPWM_TIMER_0, __speed );
            mcpwm_set_duty( MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 50.0 );
            vTaskDelay(1);
        }
        mcpwm_set_frequency( MCPWM_UNIT_0, MCPWM_TIMER_0, speed );
        mcpwm_set_duty( MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 50.0 );
    }
    running = true;
}

void WIRE_MOTOR_PWM::reset(){
    digitalWrite( dir_pin, dir_inverted?HIGH:LOW ); 
}

void WIRE_MOTOR_PWM::backup(){
    backup_speed = speed;
}

bool WIRE_MOTOR_PWM::restore(){
    return set_speed( backup_speed, false );
}

//#########################################
// if backup is true it will set the given
// speed as the backup speed too
//#########################################
bool WIRE_MOTOR_PWM::set_speed( int _speed, bool backup ){
    if( speed == _speed ){ return true; }
    speed = _speed;
    if( backup ){ backup_speed = speed; }
    if( enabled ){
        mcpwm_set_frequency( MCPWM_UNIT_0, MCPWM_TIMER_0, speed );
        if( !running ){ return true; }
        mcpwm_set_duty( MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 50.0 );
    }
    return true;
}

//###############################################
// Move the wire stepper in positive or negative
// direction a given distance. This is not 100%
// accurate. The pulses are tracked but it can 
// overshott a few steps before the stepper 
// finally stops as the PWM is not stopped within
// the pulse counter ISR 
//###############################################
bool WIRE_MOTOR_PWM::move( float distance, bool dir ){
    if( !enabled ){ 
        debuglog("*Motor not available. Wrong config.");
        return true; 
    };
    int total_steps  = round( distance * float( get_steps_per_mm() ) ); // calculate the total steps to do for the given distance
    stop();                                                             // stop motion if there is any
    delayMicroseconds(20);                                              // add a little delay for the drivers
    int dir_pin_state = digitalRead( dir_pin );                         // backup the current state of the direction pin
    if( dir_inverted ){
        dir = !dir;
    }
    digitalWrite( dir_pin, dir );                                       // set the motor direction
    delayMicroseconds(20);                                              // add direction change delay
    total_steps_do = round( total_steps );                              // set the global variable for the pulse counter
    attachInterrupt( pwm_pin, wire_pwm_isr, RISING );                   // attach interupt on rising edge to count the pulses
    run();                                                              // start pwm pulsing
    while( total_steps_do > 0 ){                                        // poll the total steps and wait until it is zero
        if( get_quit_motion() ){ break; } // Warning: This will always overshoot a little and exact stepping is not required so this is ok
        vTaskDelay(10);
    }
    stop();                                 // stop the motor
    detachInterrupt( pwm_pin );             // remove interrupt to stop counting
    digitalWrite( dir_pin, dir_pin_state ); // restore direction
    delayMicroseconds(20);                  // add direction change delay
    return true;
}




int WIRE_MOTOR_PWM::mm_min_to_speed( float mm_min ){
    float steps_per_minute = mm_min * float( steps_per_mm );
    // needed is the step frequency in hertz; steps per second
    float steps_per_second = steps_per_minute / 60.0;
    int frequency_hz = round( steps_per_second );
    if( frequency_hz <= WIRE_MIN_SPEED_HZ ){
        frequency_hz = WIRE_MIN_SPEED_HZ;
    }
    return frequency_hz;
}

//###########################################
// Return the speed in mm/min for a given
// step frequency (hz)
//###########################################
float WIRE_MOTOR_PWM::speed_to_mm_min( float _speed ){
    float current_steps_per_minute = _speed * 60.0; // freq is in hertz
    float current_mm_min = current_steps_per_minute / float( steps_per_mm );
    return current_mm_min;
}




