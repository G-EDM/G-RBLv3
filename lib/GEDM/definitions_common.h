#pragma once

#include <driver/adc.h>
#include <pins_arduino.h>

//###########################################################################
// OTA firmware update instructions for an already installed system:
// Compile the firmware with vStudio code and platform.io
// The file needed is firmware.bin location should be within the project 
// folder at: .pio/build/esp32dev/firmware.bin
// Copy the file to the sd card at top directory "/", put it into the G-EDM 
// and restart the machine. The file will be renamed to firmware.bak to 
// prevent another install. The previous session data stored in the NVS 
// storage gets deleted after an update
//###########################################################################
#define FIRMWARE_VERSION "v3.0.0"

#define MACHINE_CONFIG_GEDM
//#define MACHINE_CONFIG_NIKO
//#define MACHINE_CONFIG_PICO
//#define MACHINE_CONFIG_SARNOLD


//################################
// G-EDM and PICO pre configs
//################################
#ifdef MACHINE_CONFIG_PICO

    #define MACHINE_NAME "PICO"
    #define CNC_TYPE_XYUVZ

    #define X_STEPS_MM 3200.0
    #define Y_STEPS_MM 3200.0
    #define Z_STEPS_MM 3200.0
    #define A_STEPS_MM 3200.0
    #define U_STEPS_MM 3200.0
    #define V_STEPS_MM 3200.0    

    #define X_TRAVEL 300.0
    #define Y_TRAVEL 300.0
    #define Z_TRAVEL 100.0
    #define A_TRAVEL 300.0
    #define U_TRAVEL 300.0
    #define V_TRAVEL 300.0

    #define ENABLE_SERVO42C_PULLING_MOTOR  
    #define WIRE_PULLING_MOTOR_DIR_INVERTED true
    #define WIRE_PULL_MOTOR_STEPS_PER_MM    1224

#elif defined( MACHINE_CONFIG_SARNOLD )

    #define MACHINE_NAME "G-EDM"

    #define CNC_TYPE_XYZ

    #define X_STEPS_MM 3840.0
    #define Y_STEPS_MM 3840.0
    #define Z_STEPS_MM 10000.0
    #define A_STEPS_MM 3840.0
    #define U_STEPS_MM 3840.0
    #define V_STEPS_MM 3840.0

    #define X_TRAVEL 240.0
    #define Y_TRAVEL 300.0
    #define Z_TRAVEL 100.0
    #define A_TRAVEL 240.0
    #define U_TRAVEL 240.0
    #define V_TRAVEL 300.0

    #define ENABLE_PWM_PULLING_MOTOR
    #define WIRE_PULLING_MOTOR_DIR_INVERTED false
    #define WIRE_PULL_MOTOR_STEPS_PER_MM 153


#elif defined( MACHINE_CONFIG_GEDM )

    #define MACHINE_NAME "G-EDM"

    #define CNC_TYPE_XYZ

    #define X_STEPS_MM 3840.0
    #define Y_STEPS_MM 3840.0
    #define Z_STEPS_MM 3840.0
    #define A_STEPS_MM 3840.0
    #define U_STEPS_MM 3840.0
    #define V_STEPS_MM 3840.0

    #define X_TRAVEL 240.0
    #define Y_TRAVEL 300.0
    #define Z_TRAVEL 100.0
    #define A_TRAVEL 240.0
    #define U_TRAVEL 240.0
    #define V_TRAVEL 300.0

    #define ENABLE_PWM_PULLING_MOTOR
    #define WIRE_PULLING_MOTOR_DIR_INVERTED false
    #define WIRE_PULL_MOTOR_STEPS_PER_MM 153

#elif defined( MACHINE_CONFIG_NIKO )

    #define MACHINE_NAME "G-EDM"

    #define CNC_TYPE_XYZ

    #define X_STEPS_MM 3200.0
    #define Y_STEPS_MM 3200.0
    #define Z_STEPS_MM 3200.0
    #define A_STEPS_MM 3200.0
    #define U_STEPS_MM 3200.0
    #define V_STEPS_MM 3200.0    

    #define X_TRAVEL 300.0
    #define Y_TRAVEL 300.0
    #define Z_TRAVEL 100.0
    #define A_TRAVEL 300.0
    #define U_TRAVEL 300.0
    #define V_TRAVEL 300.0

    #define ENABLE_PWM_PULLING_MOTOR
    #define WIRE_PULLING_MOTOR_DIR_INVERTED false
    #define WIRE_PULL_MOTOR_STEPS_PER_MM    180

#else

    #define MACHINE_NAME "G-EDM"

    //#define CNC_TYPE_Z
    //#define CNC_TYPE_XY
    #define CNC_TYPE_XYZ
    //#define CNC_TYPE_XYUV
    //#define CNC_TYPE_XYUVZ // not done yet, don't use! The motionboard only has 4 TMC drivers. Need to figure something out here. Maybe MKS42C with UART for basic z actions without feedback

    #define X_STEPS_MM 3840.0
    #define Y_STEPS_MM 3840.0
    #define Z_STEPS_MM 3840.0
    #define A_STEPS_MM 3840.0
    #define U_STEPS_MM 3840.0
    #define V_STEPS_MM 3840.0 
    
    #define X_TRAVEL 240.0
    #define Y_TRAVEL 300.0
    #define Z_TRAVEL 100.0
    #define A_TRAVEL 240.0
    #define U_TRAVEL 240.0
    #define V_TRAVEL 300.0

    #define ENABLE_PWM_PULLING_MOTOR
    #define WIRE_PULLING_MOTOR_DIR_INVERTED false
    #define WIRE_PULL_MOTOR_STEPS_PER_MM    153

#endif


//#####################################################################
// The wire feeder can run on one single stepper with stepd/dir signals
// or one single makerbase Servo 42C stepper with the new board version
// ( not the old v1.0 one ) in UART mode
// The torque motor is optional and from what I have seen putting a 
// Nema17 on there without any power is already enough resistance
// The G-EDM motionboard only provides 4 TMC2209 outputs and if the 
// CNC is of XYUV type all available TMCs are used for the axes XY UV
// Z axis support (XYUVZ) is not finsihed yet and may only provide 
// basic Z features. On XYUV the wire module motors can only be run in
// UART mode and currently only the Makerbase motor is supported
// by default the slave address for the pulling motor is 0 which is
// also the default address on the driver. Torque motor address is 1
//#####################################################################
//#define ENABLE_SERVO42C_TORQUE_MOTOR // optional and overkill
//#define ENABLE_SERVO42C_PULLING_MOTOR  // comment out ENABLE_PWM_PULLING_MOTOR      if this is used
//#define ENABLE_PWM_PULLING_MOTOR     // comment out ENABLE_SERVO42C_PULLING_MOTOR if this is used

// if MKS motors are used those are the addresses used for them
#define MKS_TORQUE_MOTOR_UART_ADDRESS  1
#define MKS_PULLING_MOTOR_UART_ADDRESS 0



#define X_STEPS_PER_MM X_STEPS_MM
#define Y_STEPS_PER_MM Y_STEPS_MM
#define Z_STEPS_PER_MM Z_STEPS_MM
#define A_STEPS_PER_MM A_STEPS_MM // this is future music where the wire is controlled externally and the stepper currently used for the wire will be used as 4th axis
#define U_STEPS_PER_MM U_STEPS_MM // this is future music
#define V_STEPS_PER_MM V_STEPS_MM // this is future music


#define MKS42C_TORQUE_MOTOR_STEPS_PER_MM 1224 // veeeery rough stuff here. MKS42C in UART is 256 Microsteps, same pulley size as the pulling motor etc.


typedef struct {
    float RAPID             = 160.0;
    float INITIAL           = 50.0;
    float EDM               = 20.0;
    float PROBING           = 30.0;
    float REPROBE           = 58.5;
    float RETRACT           = 80.0;
    float HOMING_FEED       = 36.6;
    float HOMING_SEEK       = 220.0;
    float WIRE_RETRACT_HARD = 10.0;
    float WIRE_RETRACT_SOFT = 5.0;
} speeds_mm_min;

extern volatile speeds_mm_min process_speeds_mm_min;


#define VSENSE_RESOLUTION               4095 // default 12bit ESP ADC
#define STEPPER_DIR_CHANGE_DELAY        100
#define DEFAULT_STEP_PULSE_MICROSECONDS 4



#define I2S_DMA_BUF_LEN   (200)
#define I2S_TIMEOUT_TICKS (20) 
#define I2S_SAMPLE_RATE   (600000) 
#define I2S_BUFF_COUNT    (2)//100
#define I2S_NUM_SAMPLES I2S_DMA_BUF_LEN



//#####################################################################
// DPM specific section; DPM needs to run on address "01". 
// This is the default address and can be changed on the DPM controller 
// Warning: This implementation only works with the TTL simple protocol DPM version 
// The default baudrate on the DPM is 9600. This needs to be changed to 115200 on the DPM. See the DPM manual for details.
// 115200 is fast and needs a good level shifter circuit. I use a simple NPN transistor based level shifter circuit.
// Available cheap ready to use HighSpeed Fullduplex logic level converters may not work. I tried one from Amazon without success.
// DPM operates at 5v, ESP 3.3v. 
//#####################################################################
#define DPM_RXTX_SERIAL_COMMUNICATION_BAUD_RATE 115200 // valid: 2400, 4800,9600, 19200, 38400, 57600, 115200
#define DPM_INTERFACE_TX TX
#define DPM_INTERFACE_RX RX
