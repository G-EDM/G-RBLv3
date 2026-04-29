/*//###############################################################
//  _______  _        _______    _______  _______  _______           _______     ___  _______  _______ 
// (       )| \    /\(  ____ \  (  ____ \(  ____ \(  ____ )|\     /|(  ___  )   /   )/ ___   )(  ____ \
// | () () ||  \  / /| (    \/  | (    \/| (    \/| (    )|| )   ( || (   ) |  / /) |\/   )  || (    \/
// | || || ||  (_/ / | (_____   | (_____ | (__    | (____)|| |   | || |   | | / (_) (_   /   )| |      
// | |(_)| ||   _ (  (_____  )  (_____  )|  __)   |     __)( (   ) )| |   | |(____   _)_/   / | |      
// | |   | ||  ( \ \       ) |        ) || (      | (\ (    \ \_/ / | |   | |     ) ( /   _/  | |      
// | )   ( ||  /  \ \/\____) |  /\____) || (____/\| ) \ \__  \   /  | (___) |     | |(   (__/\| (____/\
// |/     \||_/    \/\_______)  \_______)(_______/|/   \__/   \_/   (_______)     (_)\_______/(_______/
//                                                                                                    
// Library to control the Makerbase Servo42C driver vesion >= 1.1.2 (v1.0 is not supported)
//###############################################################*/
#pragma once

#ifndef WIRE_CONTROLLER_CONFIG
#define WIRE_CONTROLLER_CONFIG

//#define RX_PIN 20
//#define TX_PIN 21


#define RX_PIN 16
#define TX_PIN 0

#define MKS42C_BAUDRATE 38400

#define MKS42C_MICROSTEPS_DEFAULT       255 // 255 is uint8 for 256 microsteps 0-255
#define MKS42C_ENABLEMICROSTEPS_DEFAULT 1
#define MKS42C_MAXCURRENT_DEFAULT       800 //mA in 200 step from 0,200,400,600.....
#define MKS42C_MAXTORQUE_DEFAULT        40  // no idea about the unit. Max is 1200

#define MKS42C_MAXCURRENT_PULLMOTOR     1600 //mA in 200 step from 0,200,400,600.....
#define MKS42C_MAXTORQUE_PULLMOTOR      1200  // no idea about the unit. Max is 1200

#define MKS42C_ADDRESS_DEFAULT          0   // default device slave address (0-9)
#define MKS42C_ENABLEMODE_DEFAULT       0   // active low enable pin
#define MKS42C_ZEROMODE_DEFAULT         1   //
#define MKS42C_ZEROMODE_DIR_DEFAULT     0   //
#define MKS42C_ZEROMODE_SPEED_DEFAULT   1   //
#define MKS42C_MOTOR_TYPE_DEFAULT       1   // 1 = 1.8° step angle, 0 = 0.9° step angle
#define MKS42C_MOTOR_SPEED_DEFAULT      6   // 1 = 1.8° step angle, 0 = 0.9° step angle



static const uint8_t  MKS_MAX_SEND_RETRIES       = 10;
static const uint32_t MKS_WAIT_TIMEOUT           = 1000;
static const uint32_t MKS_DEFAULT_RECEIVE_LENGTH = 3;
static const int MAX_POS_TORQUE  = 1200;
static const int MAX_POS_CURRENT = 2500; // via uart there is a different max limit

#endif