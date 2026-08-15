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

#include "state_engine.h"

std::atomic<int> current_alarm_code( 0 );
DRAM_ATTR volatile uint8_t system_block_motion    = 1;
DRAM_ATTR volatile machine_states  system_state   = STATE_IDLE;
DRAM_ATTR volatile machine_states  old_state      = STATE_REBOOT;
DRAM_ATTR volatile system_op_modes system_op_mode = OP_MODE_NORMAL;

std::map<machine_states, const char*> state_labels = {
    { STATE_IDLE,    "Idle"   },
    { STATE_BUSY,    "Busy"   },
    { STATE_HOMING,  "Homing" },
    { STATE_JOG,     "Jog"    },
    { STATE_PROBING, "Probe"  },
    { STATE_ALARM,   "Alarm"  },
    { STATE_ESTOP,   "eStop"  },
    { STATE_REBOOT,  "Reboot" }
};

const char *alarm_labels[] = {
    "Error code:",
    "Error msg:",
    "Touch to reset"
};

const char *alarm_messages[] = {
    "Unknown Error",
    "Hard limit was triggered",
    "Target out of reach",
    "Reset while in motion",
    "Probe had initial contact",
    "Probe failed",
    "Homing failed",
    "Homing failed",
    "Homing failed",
    "Homing failed",
    "",
    "",
    "",
    "",
    0
};

//#########################################################
// Atomic locks for different sections of the code
//#########################################################
DRAM_ATTR std::atomic<bool> atomic_locks[TOTAL_ATOMIC_LOCKS] = {
    ATOMIC_VAR_INIT(false),
    ATOMIC_VAR_INIT(false),
    ATOMIC_VAR_INIT(false),
    ATOMIC_VAR_INIT(false),
    ATOMIC_VAR_INIT(false),
    ATOMIC_VAR_INIT(false),
    ATOMIC_VAR_INIT(false),
    ATOMIC_VAR_INIT(false)
};
#include "ili9341_tft.h"

//#############################################################
// i2s has a lock
// gscope has a lock
// UI interface has a lock for the logger widget
// SD card has a lock
// Widget base has a lock for the sprite
// serial has a lock for the inputbuffer
// ledc has a lock (not fully implemented, only where needed) 
//#############################################################
IRAM_ATTR bool acquire_lock_for( atomic_locks_num lock_index ){
    //return true;
    bool expected;
    int counts = 0;
    int max = 50; // 5 seconds limit
    while (true) {
        expected = false;
        if( atomic_locks[ lock_index ].compare_exchange_strong(expected, true, std::memory_order_acquire) ) {
            return true; // Lock acquired
        }
        if( ++counts < 100 ){
            vTaskDelay(1);
        } else {
            counts = 0;
            if( --max <= 0 ){
                // problem....
                //tft.drawNumber( (int)lock_index, 30, 30 );
                release_lock_for( lock_index );
                return true; // can work but could also end in a race condition
            }
        }
    }
    return false;
}

//#############################################################
// Doesn't wait for the lock. if lock is taken it returns false
//#############################################################
IRAM_ATTR bool acquire_lock_for( atomic_locks_num lock_index, bool wait ){
    bool expected;
    while (true) {
        expected = false;
        if( atomic_locks[ lock_index ].compare_exchange_strong(expected, true, std::memory_order_acquire) ) {
            return true; // Lock acquired
        }
        if( !wait ) return false;
        vTaskDelay(1);
    }
    return false;
}

//#########################################################
// Release atomic lock by index
//#########################################################
IRAM_ATTR void release_lock_for( atomic_locks_num lock_index ){
    atomic_locks[ lock_index ].store(false, std::memory_order_release);
}

//#########################################################
// Check if lock is taken by index
//#########################################################
IRAM_ATTR bool lock_is_taken( atomic_locks_num lock_index ){
    return atomic_locks[ lock_index ].load(std::memory_order_acquire);
}

//#########################################################
// Set system operation mode / normal / edm / edmtype
//#########################################################
IRAM_ATTR void set_system_op_mode( system_op_modes mode ){
    if( system_op_mode == mode ) return;
    acquire_lock_for( ATOMIC_LOCK_OPMODE );
    system_op_mode = mode;
    release_lock_for( ATOMIC_LOCK_OPMODE );
}

//#########################################################
// Compare current mode against given mode
//#########################################################
IRAM_ATTR bool is_system_op_mode(  system_op_modes mode ){
    return ( system_op_mode == mode );
}

//#########################################################
// Get the current mode
//#########################################################
IRAM_ATTR system_op_modes get_system_op_mode(){
    return system_op_mode;
}

//#########################################################
// Check if current mode is an edm mode
//#########################################################
extern IRAM_ATTR bool is_system_mode_edm(){
    return ( system_op_mode >= OP_MODE_EDM );
}

//########################################################
// Set machine state
//########################################################
IRAM_ATTR void set_machine_state( machine_states state ){
    if( state == system_state || ( system_state >= STATE_ALARM && state < STATE_ESTOP ) ){ // only way to recove from alarm and estop is via the unlock command
        return;
    }
    acquire_lock_for( ATOMIC_LOCK_MSTATE );
    system_state = state;
    release_lock_for( ATOMIC_LOCK_MSTATE );
}

IRAM_ATTR void set_busy_low_priority(){
    if( system_state > STATE_IDLE ){ // only way to recove from alarm and estop is via the unlock command
        return;
    }
    acquire_lock_for( ATOMIC_LOCK_MSTATE );
    system_state = STATE_BUSY;
    release_lock_for( ATOMIC_LOCK_MSTATE );
}


//########################################################
// Check if state changed
//########################################################
IRAM_ATTR bool state_changed(){
    if( system_state != old_state ){
        old_state = system_state;
        return true;
    }
    return false;
}

//########################################################
// Reset machine state to IDLE and ALARM to none
//########################################################
IRAM_ATTR void reset_machine_state(){
    current_alarm_code.store( 0 );
    acquire_lock_for( ATOMIC_LOCK_MSTATE );
    system_state = STATE_IDLE;
    release_lock_for( ATOMIC_LOCK_MSTATE );
}

//########################################################
// Get the current machine state
//########################################################
IRAM_ATTR machine_states get_machine_state(){
    return system_state;
}

//########################################################
// Returns true if the current state matches the expected
//########################################################
IRAM_ATTR bool is_machine_state( machine_states state ){
    return ( system_state == state );
}

//########################################################
// check if moion is enabled and state < ALARM
//########################################################
IRAM_ATTR bool get_quit_motion( bool check_for_edm_mode ){
    return ( system_block_motion || system_state >= STATE_ALARM || ( check_for_edm_mode && !is_system_mode_edm() ) );
}

//########################################################
// returns true if state > ALARM
//########################################################
IRAM_ATTR bool get_show_error_page(){
    return ( system_state >= STATE_ALARM );
}

//########################################################
// Set an alarm and change state to alarm if state
// is < ESTOP
//########################################################
IRAM_ATTR void set_alarm( int alarm_code, bool set_state ){
    acquire_lock_for( ATOMIC_LOCK_MSTATE );
    current_alarm_code.store( alarm_code );
    release_lock_for( ATOMIC_LOCK_MSTATE );
    if( ! set_state ) return;
    set_machine_state( STATE_ALARM );
}

//########################################################
// Get the current alarm code
//########################################################
IRAM_ATTR int get_alarm(){
    return current_alarm_code.load();
}

//########################################################
// Get the alarm message for a given code
//########################################################
const char* get_alarm_msg( int alarm_code ){
    return alarm_messages[ alarm_code ];
}

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
bool wait_for_block_until_state( machine_states wait_for, machine_states block_until, uint16_t task_delay ){
    int64_t microsecs = esp_timer_get_time();
    while( !is_machine_state( wait_for ) ){
        if( get_quit_motion() ) return false;
        if( esp_timer_get_time() - microsecs > 3000000 ) break;
        vTaskDelay( 10 );
    }
    while( get_machine_state() > block_until ){
        if( get_quit_motion() ) return false;
        vTaskDelay(task_delay);
    }
    return true;
}

//########################################################
// Blocking function to wait for idle state
// breaks early if get_quit_motion() returns true
// also checks the inputbuffer to ensure it doesn't give a
// false idle before the line in the buffer is processed
//########################################################
bool wait_for_idle_state( uint16_t task_delay, bool exit_on_motion_block ){
    while( 
        !is_machine_state( STATE_IDLE ) // machine not idle
        || inputBuffer.peek() != -1     // inputbuffer not empty
    ){
        if( exit_on_motion_block ){
            if( get_quit_motion() ) return false;
        } else {
            if( system_state >= STATE_ALARM ) return false;
        }
        vTaskDelay(task_delay);
    }
    return true;
}

//########################################################
// Sets the state to busy if it is idle
// just a pre state until the real state like probing
// homing etc. is set. This is used for the input buffer
// to set a busy state as early as possible
//########################################################
IRAM_ATTR bool increase_state_to_busy(){
    if( is_machine_state( STATE_IDLE ) ){
        set_machine_state( STATE_BUSY );
        return true;
    }
    return false;
}