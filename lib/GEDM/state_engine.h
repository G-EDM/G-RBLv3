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


#ifndef STATE_ENGINE_H
#define STATE_ENGINE_H


#include <esp_attr.h>
#include <atomic>
#include <map>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <InputBuffer.h>  

#define ENABLE_SCOPE_LOCK



//########################################################
// States >= ALARM will stop motion
// If machine is in a high priority state like
// ALARM, ESTOP, REBOOT it will not accept the change
// to lower priority states and the state needs to be
// reset with a different function first
//########################################################
enum machine_states {
    STATE_IDLE     = 0, // i dindu nuffin
    STATE_BUSY     = 1, // something going on like command received or motion
    STATE_HOMING   = 2, // homing motion
    STATE_JOG      = 3, // jog motion
    STATE_PROBING  = 4, // probing motion
    STATE_ALARM    = 5, // something not ok; no motion allowed
    STATE_ESTOP    = 6, // pretty bad thing like hitting a limit switch; no motion allowed
    STATE_REBOOT   = 7  // no motion allowed
};

enum system_op_modes {
    OP_MODE_NORMAL     = 0,
    OP_MODE_EDM        = 1,
    OP_MODE_EDM_WIRE   = 2,
    OP_MODE_EDM_SINKER = 3,
    OP_MODE_EDM_REAMER = 4
};



enum machine_errors {
    ERROR_NONE              = 0,
    ERROR_HARDLIMIT         = 1,
    ERROR_SOFTLIMIT         = 2,
    ERROR_PROBE_NO_CONTACT  = 3,
    ERROR_PROBE_HAD_CONTACT = 4
};



enum atomic_locks_num {
    ATOMIC_LOCK_GSCOPE,
    ATOMIC_LOCK_OPMODE,
    ATOMIC_LOCK_MSTATE,
    ATOMIC_LOCK_LOGGER,
    ATOMIC_LOCK_SDCARD,
    ATOMIC_LOCK_UISPRITE,
    ATOMIC_LOCK_INPUT,
    ATOMIC_LOCK_NVS,
    TOTAL_ATOMIC_LOCKS
};


extern std::map<machine_states, const char*> state_labels;

extern const char *alarm_labels[];
extern const char *alarm_messages[];

extern DRAM_ATTR std::atomic<int>  current_alarm_code;

extern DRAM_ATTR volatile uint8_t        system_block_motion; // this flag is used by the on/off switch
extern DRAM_ATTR volatile machine_states system_state;
extern DRAM_ATTR volatile machine_states old_state;


extern DRAM_ATTR volatile system_op_modes system_op_mode;
extern IRAM_ATTR void set_system_op_mode( system_op_modes mode = OP_MODE_NORMAL );
extern IRAM_ATTR bool is_system_op_mode(  system_op_modes mode = OP_MODE_NORMAL );
extern IRAM_ATTR system_op_modes get_system_op_mode();
extern IRAM_ATTR bool is_system_mode_edm();



//#########################################################
// Will acquire the provided lock
//#########################################################
extern IRAM_ATTR bool acquire_lock_for( atomic_locks_num lock_index );
extern IRAM_ATTR bool acquire_lock_for( atomic_locks_num lock_index, bool wait );

//#########################################################
// will release the provide lock
//#########################################################
extern IRAM_ATTR void release_lock_for( atomic_locks_num lock_index );
extern IRAM_ATTR bool lock_is_taken( atomic_locks_num lock_index );



extern IRAM_ATTR bool state_changed( void );

//########################################################
// Set machine state
//########################################################
extern IRAM_ATTR void set_machine_state( machine_states state );
extern IRAM_ATTR void set_busy_low_priority(); // set state to busy if idle

//########################################################
// Reset machine state to IDLE and ALARM to none
//########################################################
extern IRAM_ATTR void reset_machine_state( void );

//########################################################
// Get the current machine state
//########################################################
extern IRAM_ATTR machine_states get_machine_state( void );

//########################################################
// Returns true if the current state matches the expected
//########################################################
extern IRAM_ATTR bool is_machine_state( machine_states state );

//########################################################
// check if moion is enabled and state < ALARM
//########################################################
extern IRAM_ATTR bool get_quit_motion( bool check_for_edm_mode = false );

//########################################################
// returns true if state > ALARM
//########################################################
extern IRAM_ATTR bool get_show_error_page( void );

//########################################################
// Set an alarm and change state to alarm if state
// is < ESTOP
//########################################################
extern IRAM_ATTR void set_alarm( int alarm_code, bool set_state = true );

//########################################################
// Get the current alarm code
//########################################################
extern IRAM_ATTR int get_alarm( void );

//########################################################
// Get the alarm message for a given code
//########################################################
extern const char* get_alarm_msg( int alarm_code = 0 );

//########################################################
// Wait for a given state and block until it changes to
// This is used for homing etc. and should be used with 
// care. It finishes after the current state < block_unitl
// after a line is processed the state may first change to
// busy and then with a delay to idle if the buffer has
// more chars and it normally has one extra char after 
// a line. So it will rapidly change to busy first
// returns true if everything was smooth and false
// if the loops broke out early due to break condition
//########################################################
extern bool wait_for_block_until_state( machine_states wait_for, machine_states block_until, uint16_t task_delay = 10 );

//########################################################
// Blocking function to wait for idle state
// breaks early if get_quit_motion() returns true
// also checks the inputbuffer to ensure it doesn't give a
// false idle before the line in the buffer is processed
//########################################################
extern bool wait_for_idle_state( uint16_t task_delay = 50, bool exit_on_motion_block = true );

//########################################################
// Sets the state to busy if it is idle
// just a pre state until the real state like probing
// homing etc. is set. This is used for the input buffer
// to set a busy state as early as possible
//########################################################
extern IRAM_ATTR bool increase_state_to_busy( void );
#endif