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


#include "Grbl.h"
#include <stdlib.h>
#include "ili9341_tft.h"

enum planner_msg {
    PLANNER_OK = 0,
    PLANNER_ERR_UNKNOWN,
    PLANNER_ERR_BROKEN_WIRE,
    PLANNER_ERR_SHORT,
    PLANNER_ERR_SHORT2,
    PLANNER_ERR_MP_TIMEOUT,
    PLANNER_ERR_RETRACT_END,
    PLANNER_WIRE_CONTACT
};

std::map<planner_msg, const char*> planner_messages = {
    { PLANNER_OK,              "" },
    { PLANNER_ERR_UNKNOWN,     "*Unknown error"},
    { PLANNER_ERR_BROKEN_WIRE, "*Check wire"},
    { PLANNER_ERR_SHORT,       "*Short circuit timeout"},
    { PLANNER_ERR_SHORT2,      "*Shorted 2"},
    { PLANNER_ERR_MP_TIMEOUT,  "*Motionplan timeout"},
    { PLANNER_ERR_RETRACT_END, "*Retract limit"},
    { PLANNER_WIRE_CONTACT,    "Wire contact made!" }
};

typedef struct {
    int    arc_counter;
    bool   wire_has_first_contact;
    bool   is_flushing;
    int8_t motion_plan;
    int    total_retraction_steps;
} planner_state;

typedef struct retraction_config {
  int soft_retract_start = 0;
  int hard_retract_start = 0;
  int steps_done         = 0;
  int steps_total        = 0;
  int steps_case_3       = 0;
  int steps_case_4       = 0;
  int steps_case_5       = 0;
  int steps_case_0       = 0;
  int early_exit_confirmations = 0;
} retraction_config;


planner_msg pause_reason = PLANNER_OK;


planner_state     DRAM_ATTR _state;
retraction_config DRAM_ATTR rconf;
DRAM_ATTR volatile planner_config plconfig;
DRAM_ATTR volatile flush_config flconf;
DRAM_ATTR volatile int64_t flush_retract_timer_us = 0;

DRAM_ATTR std::atomic<bool> process_paused( false );


int DRAM_ATTR tmp_counter                     = 0;
int DRAM_ATTR short_circuit_estop_reset_count = 0; // to prevent a single wrong ADC reading from resetting the timer
int DRAM_ATTR no_load_steps                   = 0;


G_EDM_PLANNER planner = G_EDM_PLANNER();

G_EDM_PLANNER::G_EDM_PLANNER(){
    gconf.gedm_retraction_motion = false;
    position_history_reset();
    reset_planner_state();
}

void IRAM_ATTR toggle_spindle( bool turn_off ){ // thread safe
    int data = turn_off ? 6 : 7;
    remote_control( data );
    while( ( turn_off ? wire_feeder.is_running() : !wire_feeder.is_running() ) ){ 
        if( get_quit_motion( true ) ) break;
        delayMicroseconds(10); 
    } 
}

void IRAM_ATTR toggle_pwm( bool turn_off, int delay ){
    if( turn_off == arcgen.get_pwm_is_off() ){ delayMicroseconds( delay ); return; }
    int data = turn_off ? 4 : 5;
    remote_control( data );
    while( turn_off != arcgen.get_pwm_is_off() ){
        if( get_quit_motion( true ) ) break;
        delayMicroseconds(10); 
    }
}

void IRAM_ATTR G_EDM_PLANNER::reset_flush_retract_timer(){
    flush_retract_timer_us = esp_timer_get_time();
}


bool IRAM_ATTR G_EDM_PLANNER::short_circuit_estop(){
    //########################################################
    // check for max retraction distance
    //########################################################
    if( is_system_op_mode( OP_MODE_EDM_WIRE ) && _state.total_retraction_steps >= rconf.steps_case_0 ){
        pause_reason = PLANNER_ERR_RETRACT_END;
        gconf.edm_pause_motion = true;
    }
    //########################################################
    // check for max duration
    //########################################################
    if( short_circuit_start_time != 0 ){
        if ( esp_timer_get_time() - short_circuit_start_time > plconfig.short_circuit_max_duration_us ){ 
            pause_reason           = PLANNER_ERR_SHORT;
            gconf.edm_pause_motion = true; 
        }
    }
    return true;
}


void IRAM_ATTR G_EDM_PLANNER::pre_process_history_line_forward( Line_Config &line ){
    reset_rconf();
    get_motion_plan(); 
    if( !_state.wire_has_first_contact ){
        reset_flush_retract_timer();
        _state.total_retraction_steps = 0;
        no_load_steps = 0;
    } 
}



//###############################
//###############################
int IRAM_ATTR G_EDM_PLANNER::get_motion_plan( bool enforce_fresh ){

    if( simulate_gcode ){
        _state.motion_plan = MOTION_PLAN_FORWARD;
        return 1;
    }

    _state.motion_plan = get_calculated_motion_plan( _state.wire_has_first_contact ? enforce_fresh : true );
    
    //################################################################
    // Check for negative plan that indicates the seek phase is done
    //################################################################
    if( _state.motion_plan < MOTION_PLAN_ZERO ){
        _state.motion_plan = (_state.motion_plan ^ (_state.motion_plan >> 7)) - (_state.motion_plan >> 7); //_state.motion_plan *= -1; // invert it back
        _state.wire_has_first_contact = true;
    }

    //####################################
    // If motion queue timed out retry
    // a timeout will return 9 as plan
    //####################################
    if( _state.motion_plan == MOTION_PLAN_TIMEOUT && !gconf.edm_pause_motion ){
        tmp_counter = 0;
        while( _state.motion_plan == MOTION_PLAN_TIMEOUT ){
            if( get_quit_motion( true ) ) break;
            toggle_pwm( true, 100 );
            toggle_pwm( false, 2 );
             _state.motion_plan = get_calculated_motion_plan( _state.wire_has_first_contact ? false : true );
             if( ++tmp_counter > 5 || gconf.edm_pause_motion ){
                pause_reason = PLANNER_ERR_MP_TIMEOUT;
                //debuglog( planner_messages[PLANNER_ERR_MP_TIMEOUT] );
                gconf.edm_pause_motion = true;
                _state.motion_plan     = MOTION_PLAN_SOFT_SHORT;
                break;
            }
        }

    }
    


    //####################################################################################
    //
    // Monitor no load feeds to stop on wire breaks. The sensor class will add a
    // soft hold after every forward plan to gain some time to adjust to new feedback.
    // soft hold and feed both happen on a broken wire
    //
    //####################################################################################
    if( _state.total_retraction_steps > 0 || _state.motion_plan > MOTION_PLAN_HOLD_SOFT ){
        no_load_steps = 0;
    } else if( no_load_steps > rconf.steps_case_0 ){
        no_load_steps = rconf.steps_case_0;
    }

    if( _state.motion_plan <= MOTION_PLAN_HOLD_SOFT ){

        if( 
            ( no_load_steps >= rconf.steps_case_0 )
            && is_system_op_mode( OP_MODE_EDM_WIRE ) // only in wire mode
            && _state.wire_has_first_contact         // only after it has the first contact
            && _state.total_retraction_steps <= 5    // only if there are no retraction leftovers
            && position_history_is_at_final_index()  // only if it is at the final index
            && !gconf.gedm_retraction_motion         // trigger wire break only in forward motion
        ){
            pause_reason           = PLANNER_ERR_BROKEN_WIRE;
            gconf.edm_pause_motion = true;
            _state.motion_plan     = MOTION_PLAN_SOFT_SHORT;
            no_load_steps          = 0;
        }
        
    }




    //####################################################################################
    //
    // Set or monitor short circuit timeout
    //
    //####################################################################################
    if( !is_machine_state( STATE_PROBING ) ){
        if( _state.motion_plan >= MOTION_PLAN_HARD_SHORT ){ // don't count soft retractions as short circuit..
            if( short_circuit_start_time == 0 ){
                short_circuit_start_time = esp_timer_get_time();
            }
            short_circuit_estop_reset_count = 0;
        } else if ( _state.motion_plan <= MOTION_PLAN_HOLD_HARD ){
            short_circuit_start_time = 0;
        }
    }





    return _state.motion_plan;
}


bool G_EDM_PLANNER::get_is_paused(){
    return process_paused.load();
}

void G_EDM_PLANNER::pause( bool redraw ){
    if( get_is_paused() ) return;
    if( is_machine_state( STATE_PROBING ) ){
        gconf.edm_pause_motion   = false;
        short_circuit_start_time = 0;
        return;
    }
    toggle_pwm( true, 0 );  // disable pwm
    toggle_spindle( true ); // stop wire
    gconf.gedm_flushing_motion = false; // better not enter a pause while flushing... Hard return..
    enforce_redraw.store( true );
    process_paused.store( true );

    if( pause_reason > PLANNER_OK ){
        debuglog( planner_messages[pause_reason] );
        pause_reason = PLANNER_OK;
    }

    while( true ){
        vTaskDelay(10);
        if( !gconf.edm_pause_motion || get_quit_motion( true ) ) break;
    }


    gconf.edm_pause_motion        = false;
    no_load_steps                 = 0;
    _state.total_retraction_steps = 0;
    set_retraction_steps();
    gsense.reset_sensor_global();
    reset_flush_retract_timer();
    if( get_quit_motion( true ) || simulate_gcode ){ 
        process_paused.store( false );
        return; 
    }
    was_paused          = true;
    pause_recover_count = 0;
    if( ! get_is_paused() ){ return; }
    if( enable_spindle ){
        toggle_spindle( false ); // start wire
    }
    vTaskDelay(10);
    process_paused.store( false );
    toggle_pwm( false, 4 );
    reset_flush_retract_timer();
    short_circuit_start_time = 0;
    gsense.reset_sensor_global();
    get_motion_plan();
}

// used only to limit retraction
// retracting around edges is a problem if the wire is stuck
// it is better to not do it and try other methods
// if nothing helps to recover from a short we enter a pause which is still better then making it worse
bool IRAM_ATTR G_EDM_PLANNER::is_end_of_line( Line_Config &line ){
    return line.step_count >= line.step_event_count ? true : false;
}






/** 
  * called while stepping and if probing is active; checks the probe state and 
  * inserts a pause until a positive is confirmed 
  * see sensors.cpp for more details
  **/
bool IRAM_ATTR G_EDM_PLANNER::probe_check( Line_Config &line ){
    if( is_machine_state( STATE_PROBING ) ){
        no_load_steps   = 0; // reset wire break for probing
        line.step_delay = process_speeds.PROBING;
        get_motion_plan();
        line.motion_plan_enabled     = false;
        line.enable_position_history = false;
        reset_flush_retract_timer();
        toggle_pwm( false, 4 );
        if( _state.motion_plan > MOTION_PLAN_FORWARD ){
            line.skip_feed = true;
        } else{
            line.skip_feed = false;
        }
        if( probe_state_monitor() ){
            toggle_pwm( true, 0 );
            return true;
        }
    }
    return false;
}



void IRAM_ATTR G_EDM_PLANNER::reset_rconf(){
    rconf.soft_retract_start = 0;
    rconf.hard_retract_start = 0;
    rconf.steps_done         = 0;
    rconf.steps_total        = 0;
}

// this one is only called after flushing
bool IRAM_ATTR G_EDM_PLANNER::reset_short_circuit_protection(){
    short_circuit_start_time        = 0;
    short_circuit_estop_reset_count = 0;
    _state.motion_plan              = MOTION_PLAN_FORWARD;
    _state.total_retraction_steps   = 0;
    no_load_steps                   = 0;
    gsense.reset_sensor_global();
    reset_rconf();
    return true;
}

// resets some stuff, checks for short circuit timeout and pause resumes..
bool IRAM_ATTR G_EDM_PLANNER::pre_process_history_line( Line_Config &line ){
    line.skip_feed         = false;
    line.ignore_feed_limit = false;
    if( pause_recover_count < 10 ){
        ++pause_recover_count;
        short_circuit_start_time = 0;
        gsense.reset_sensor_global();
    }
    short_circuit_estop();
    if( was_paused ){
        gsense.reset_sensor_global();
        short_circuit_start_time = 0;
        was_paused = false;
        if( !gconf.gedm_retraction_motion ){
            reset_rconf();
            rconf.hard_retract_start = 1;
            return false; // force backward motion after resume
        }
    } 
    return true;
}


void G_EDM_PLANNER::position_history_reset(){
    has_reverse                    = 0;
    was_paused                     = false;
    pause_recover_count            = 0;
    _state.total_retraction_steps  = 0;
    position_history_index_current = 1; 
    position_history_index         = 0;
    position_history_is_between    = false;
    planner_is_synced              = false;
    _state.arc_counter             = 0;
    _state.wire_has_first_contact  = false;
    memset(position_history, 0, sizeof(position_history[0]));
    push_break_to_position_history();
    push_current_mpos_to_position_history();
    position_history_force_sync();
    wire_feeder.reset();
}
















void G_EDM_PLANNER::reset_planner_state(){
    _state.arc_counter            = 0;
    _state.wire_has_first_contact = false;
    was_paused                    = false;
    pause_recover_count           = 0;
    set_ignore_breakpoints( false );
    short_circuit_start_time = 0;
}



bool G_EDM_PLANNER::position_history_move_forward( bool no_motion, Line_Config &line ){
    // get the previous position object
    if( !position_history_is_at_final_index() ){
        position_history_work_get_next( false ); 
        if( has_reverse > 0 ){ --has_reverse; }
    } else {
        has_reverse = 0;
    }
    if( position_history_is_break( position_history[position_history_index_current] ) ){
        return true;
    }
    bool _success = move_line( position_history[position_history_index_current], line );
    return _success;
}

/** this function exits if a short is canceled and also changes the z axis position **/
bool G_EDM_PLANNER::position_history_move_back(){
    // get the previous position object
    bool _success = true;
    // make sure it is not a break point
    bool previous_is_final    = future_position_history_is_at_final_index( true );
    uint16_t index_w_previous = position_history_work_get_previous( true ); // peek the previous index without changing the work index
    bool is_stop = ( 
        ( previous_is_final )
        || position_history_is_break( position_history[index_w_previous] ) // check if the previous index is a block position
    ) ? true : false;

    position_history_work_get_previous( false );
    if( !is_stop ){
        //position_history_work_get_previous( false );
    } else {
        return false;
    }

    Line_Config line;
    line.step_delay              = process_speeds.RETRACT;
    line.motion_plan_enabled     = true;
    //line.ignore_z_motion         = gconf.gedm_single_axis_drill_task ? false : true;
    line.ignore_feed_limit       = true;
    line.enable_position_history = true;
    ++has_reverse;

    _success = move_line( position_history[position_history_index_current], line );

    return _success;
}

//#############################################################################
// used only for homing to ignore breaks while seeking the limit switches in 
// positive direction
//#############################################################################
void G_EDM_PLANNER::set_ignore_breakpoints( bool ignore_break ){
    position_history_ignore_breakpoints = ignore_break;
}
bool G_EDM_PLANNER::position_history_is_break( int32_t* target ){
    // positive positions are interpreted as invalid
    // only while homing they are allowed
    if( position_history_ignore_breakpoints ){
        return false;
    }
    bool is_history_break = false; 
    for( int i = 0; i < N_AXIS; ++i ){
        if( target[i] > 0 ){
            is_history_break = true;
            break;
        }
    }
    return is_history_break;
}

uint16_t G_EDM_PLANNER::position_history_get_previous( bool peek ){
    uint16_t index = position_history_index;
    if (index == 0) {
        index = POSITION_HISTORY_LENGTH;
    }
    index--;
    if( ! peek ){
        position_history_index = index;
    }
    return index;
}
uint16_t G_EDM_PLANNER::position_history_get_next(){
    position_history_index++;
    if (position_history_index == POSITION_HISTORY_LENGTH) {
        position_history_index = 0;
    }
    return position_history_index;
}
uint16_t G_EDM_PLANNER::position_history_work_get_previous( bool peek ){
    uint16_t index = position_history_index_current;
    if (index == 0) {
        index = POSITION_HISTORY_LENGTH;
    }
    index--;
    if( ! peek ){
        position_history_index_current = index;
    }
    return index;
}
uint16_t G_EDM_PLANNER::position_history_work_get_next( bool peek ){
    uint16_t index = position_history_index_current;
    index++;
    if (index == POSITION_HISTORY_LENGTH) {
        index = 0;
    }
    if( ! peek ){
        position_history_index_current = index;
    }
    return index;
}
void IRAM_ATTR G_EDM_PLANNER::position_history_force_sync(){
    position_history_index_current = position_history_index;
}
/**
  * In reverse mode this checks if the previous work index is allowed to be used
  * if the previous index is the real final index the history run a full cycle backwards
  * 
  * In forward mode it checks if the next work index is the final index
  **/
bool G_EDM_PLANNER::position_history_is_at_final_index(){
    return position_history_index == position_history_index_current ? true : false;
}
bool G_EDM_PLANNER::future_position_history_is_at_final_index( bool reverse ){
    if( reverse ){
        uint16_t previous_w_index = position_history_work_get_previous( true );
        if( previous_w_index == position_history_index ){
            return true;
        } return false;
    }
    uint16_t next_w_index = position_history_work_get_next( true );
    if( next_w_index == position_history_index ){
        return true;
    } return false;
}
uint16_t IRAM_ATTR G_EDM_PLANNER::get_current_work_index(){
    return position_history_index_current;
}

uint16_t IRAM_ATTR G_EDM_PLANNER::push_to_position_history( int32_t* target, bool override, int override_index ){
    // move one step forward
    if( ! override ){ position_history_get_next(); }
    int index = override ? override_index : position_history_index;
    //position_history[ index ] = target;
    memcpy( position_history[ index ], target, sizeof(target[0]) * N_AXIS );
    return index;
}

/** 
  * Takes the target and line config, pushes the line to the history and syncs the history
  * All positions that are passed to this function are pushed to the history object
  * Not all motions use the history object and some are calling the move line function without storing
  * the positions in the history
  **/
bool IRAM_ATTR G_EDM_PLANNER::process_stage( int32_t* target, Line_Config &line ){
    // push the target to the history buffer
    uint16_t index = push_to_position_history( target, false, 0 );
    bool _success  = position_history_sync( line );
    if( ! _success || system_block_motion ){
        // if the position could not be reached 
        // it is necessary to update the history element for this position
        target = sys_position;
        //override_target_with_current( target );
        push_to_position_history( target, true, index );
    }
    return _success;
}

//########################################################################################
// Copies the current machine position into the target array
// Looks like it is only used on failures
//########################################################################################
void G_EDM_PLANNER::override_target_with_current( float* target ){
    float* current_position = system_get_mpos();
    memcpy(target, current_position, sizeof(current_position[0])*N_AXIS );
}

//########################################################################################
// target contains the target positon in mm
// __target will be filled with the targetposition in step position
//########################################################################################
void IRAM_ATTR G_EDM_PLANNER::convert_target_to_steps( float* target, int32_t* __target ){
    for( int axis=0; axis<N_AXIS; ++axis ) {
        __target[axis] = (int32_t)lround( (double)target[axis] * (double)g_axis[axis]->steps_per_mm.get() );
    }
}

//########################################################################################
// Add the current machine position to the history cache
//########################################################################################
void G_EDM_PLANNER::push_current_mpos_to_position_history(){
    int32_t target[N_AXIS]; // work target
    memcpy(target, sys_position, sizeof(sys_position));
    push_to_position_history( target, false, 0 );
}

/** 
  * breaks are ignored in forward direction but prevent the history from moving back 
  * This is just a cheap and easy way to prevent M3/M4 up/downs from becoming a problem
  * Z axis is dispatched in floating operation and would not follow the UP/DOWN path in the history
  * That would result in a crash. The easy way to prevent this is to add a break and 
  * stop the history from retractring further back
  * A break is basically just a position block with all positive coords
  * The only motion that uses positive targets is the homing motion
  * therefore while homing breaks are ignored
  * in normal operation there are only negative targets (all negative space)
  * Make sure your machine is set to all negative space!
  **/
void G_EDM_PLANNER::push_break_to_position_history(){
    int32_t target[N_AXIS];
    for( int axis = 0; axis < N_AXIS; ++axis ){ target[axis] = 1; }
    push_to_position_history( target, false, 0 );
}


bool G_EDM_PLANNER::position_history_sync( Line_Config &line ){
    gconf.gedm_retraction_motion  = false;
    gconf.gedm_planner_sync     = true;
    bool _success               = true;
    bool motion_ready           = false;
    bool current_is_last_block  = false;
    int  direction              = 0; // 0 = forward, 1 = backward
    int  last_direction         = 0; 
    bool enable_history         = line.enable_position_history; // history is only useful for wire/sinker edm    
    _state.total_retraction_steps = 0;

    motion_ready = position_history_is_at_final_index();

    while( !motion_ready ){

        motion_ready = false;

        if( get_quit_motion() ){ 
            _success = false;
            break; 
        }

        if( enable_history ){

            // default direction is forward
            // if history is enabled it can retract backwards
            // Note: in a backward retraction the line backwards is canceled as soon as the short
            // circuit is canceled. It results in a success=false
            // the line was not fully finished and therefore returns false
            // but this is actually a success in cancelling the short circuit
            direction = _success 
                         ? get_success_direction( last_direction ) 
                         : get_failure_direction( last_direction );

        }

        // move in the wanted direction
        _success = process_direction( direction, line );
        last_direction = direction;
        current_is_last_block = position_history_is_at_final_index();

        // break conditions
        if( _success ){
            if( direction == 0 ){
                // this is the same for motion with and without history
                // if the last block was succesfull in forward direction
                // it is finished
                if( current_is_last_block ){
                    motion_ready = true;
                    break;
                }
            }
        } else {
            if( ! enable_history && current_is_last_block ){
                // if it is a non history motion and reached the last block exit and return the success state
                motion_ready = true;
                break;
            }
        }
    }
    gconf.gedm_planner_sync      = 0;
    gconf.gedm_retraction_motion = false;
    gconf.gedm_flushing_motion   = false;
    // to be safe that history is synced
    // even after a hard motion cancel etc.
    position_history_force_sync();
    return _success;
}

/**
  * 
  * Success and failure only refers to the last line
  * If a line was finished it is a success
  * if a line was not finished it is a failure
  * A backward motion to cancel short circuits don't need to run the full line
  * So a failure in a backward motion always indicates that the short was canceled before the line finished
  * 
  **/
int G_EDM_PLANNER::get_success_direction( int last_direction ){
    // last motion executed succesfull
    switch (last_direction){
        case 0:
            // last success motion was in forward direction
            // this can be a normal feed or a recover forward motion 
            // if this is still within a recovery it needs some extra checks
            // to keep it in recovery until at the initial position
            return 0; // keep going forward
            break;
    
        case 1:
            // last success motion was backward
            // this motion was not enough to cancel a short circuit
            // a short was not canceled and needs more retraction
            no_load_steps = 0;
            // no matter what history depth is set
            // if within an arc it is overwritten
            // an arc is seen as a single line here
            if( 
                !is_system_op_mode( OP_MODE_EDM_SINKER ) && // allow unlimited retraction to the breakpoint in sinker mode
                has_reverse >= _state.arc_counter+plconfig.max_reverse_depth 
            ){ //MAX( _state.arc_counter, plconfig.max_reverse_depth ) ){
                return 0; // force forward
            }
            return 1;
            break;
    }
    return 0;
}
int G_EDM_PLANNER::get_failure_direction( int last_direction ){
    // last motion failed
    switch (last_direction){
        case 0:
            // last failed motion was in forward direction
            // this indicates that a normal feed motion 
            // or a forward recover motion created a short
            // a recover motion follows a retraction with the goal to get back to the initial position
            return 1; // no matter the details the next move is backwards/retraction
            break;

        case 1:
            // last failed motion in backward direction
            // this motion canceled a short circuit
            // there are no other options for a failed backward motion except the 
            // successfull cancelation of short circuits
            // the backward line was not fully drawn since the short was gone somewhere within the line
            //delayMicroseconds(200); // #todo #review this delay is very old. Maybe it should be removed?
            no_load_steps = 0;
            return 0; // back to forward???
            break;
    }
    return 1;
}
bool G_EDM_PLANNER::process_direction( int direction, Line_Config &line ){
    bool _success = true;
    switch (direction){
        case 0:
            return position_history_move_forward( false, line );
            break;
        case 1:
            gconf.gedm_retraction_motion = true;
            _success = position_history_move_back();
            gconf.gedm_retraction_motion = false;
            return _success;
            break;
    }
    return false;
}




















void IRAM_ATTR G_EDM_PLANNER::wire_line_end( Line_Config &line ){
    bool deep_check = false;
    int pass_counter = 0;
    int total_counts = round(plconfig.line_to_line_confirm_counts/2);
    if( position_history_is_at_final_index() ){ 
        if( _state.arc_counter == 0 ){
            total_counts = plconfig.line_to_line_confirm_counts;
            deep_check = true;
        }
    }
    // at the final position
    while( true ){
        get_motion_plan( true );
        if( _state.motion_plan <= deep_check ? MOTION_PLAN_FORWARD : MOTION_PLAN_HOLD_SOFT ){ 
            ++pass_counter; 
        } 
        else { pass_counter = 0; }
        if( pass_counter >= total_counts || _state.motion_plan >= MOTION_PLAN_SOFT_SHORT || get_quit_motion() ){ break; }
    }
}



bool IRAM_ATTR G_EDM_PLANNER::process_wire( Line_Config &line ){

    if( !pre_process_history_line( line ) ){
        return false;
    }  
    
    
    if( gconf.gedm_retraction_motion ){

        //########################################################################################
        // Inside a retraction / Moving back in history
        //########################################################################################

        line.step_delay        = process_speeds.WIRE_RETRACT_SOFT;
        line.ignore_feed_limit = false;
        bool end_of_line       = is_end_of_line( line );

        get_motion_plan();
        if( _state.motion_plan > plconfig.early_exit_on_plan ){
            rconf.early_exit_confirmations = 0;
        }

        if( retconf.early_exit && _state.motion_plan <= plconfig.early_exit_on_plan ){ 
            //if( ++rconf.early_exit_confirmations > 1 ){ 
                return false; 
            //}
        }

        if( rconf.soft_retract_start || rconf.hard_retract_start ){
            line.ignore_feed_limit   = true;
            line.step_delay          = _state.motion_plan == MOTION_PLAN_HARD_SHORT ? process_speeds.WIRE_RETRACT_HARD : process_speeds.WIRE_RETRACT_SOFT;
            if( rconf.hard_retract_start && rconf.steps_total >= rconf.steps_case_5 - 60 ){
                // only a few high speed steps before switching to soft speed
                line.step_delay = process_speeds.WIRE_RETRACT_SOFT;
            }
            if( --rconf.steps_total>0 ){ return true; } 
        }

        if( _state.motion_plan < MOTION_PLAN_HOLD_HARD ){
            return false;
        } 
        return true;
        
    } else {

        //########################################################################################
        // Moving forward in history
        //########################################################################################
        
        rconf.early_exit_confirmations = 0;
        
        pre_process_history_line_forward( line );

        switch ( _state.motion_plan ){

            case MOTION_PLAN_HARD_SHORT: // 
                rconf.steps_total        = rconf.steps_case_5;
                rconf.hard_retract_start = 1;
                return false;
            break; // unreachable code

            case MOTION_PLAN_SOFT_SHORT: // 
                rconf.steps_total        = rconf.steps_case_3;
                rconf.soft_retract_start = 1;
                return false;
            break; // unreachable code
        
            case MOTION_PLAN_HOLD_HARD: // 
                line.skip_feed  = true;
                line.step_delay = process_speeds.EDM;
            break;

            case MOTION_PLAN_HOLD_SOFT: // 
                line.skip_feed  = true;
                line.step_delay = process_speeds.EDM;
            break;

            default: // 
                line.skip_feed  = false;
                line.step_delay = process_speeds.EDM;
            break;

        }

        if( ! _state.wire_has_first_contact ){
            no_load_steps          = 0;
            line.ignore_feed_limit = true;
            line.step_delay        = process_speeds.INITIAL;
        } 


    }
    return true;
}

// lot of sinker specific stuff going on here... Needs a review
void G_EDM_PLANNER::configure(){
    _state.arc_counter = 0;
    process_paused.store( false );
    reset_flush_retract_timer();
};

//#############################################################
// Set the sinker axis; On XYUV X and Y will run combined
// with U and V. Motion in X will be XU and in Y it is YV
//#############################################################
void G_EDM_PLANNER::set_sinker_axis( int axis ){
    sinker_axis = axis;
}
void IRAM_ATTR G_EDM_PLANNER::set_retraction_steps(){
    // calculate the steps based on the axes with the most steps per mm
    // it is a hacky solution but works ok..
    uint8_t axis = is_system_op_mode( OP_MODE_EDM_WIRE ) ? axes_active.x.index : sinker_axis; //XYUV should all have the same step resolution
    rconf.steps_case_3 = MAX( 2, motor_manager.convert_mm_to_steps( retconf.soft_retraction,     axis ) );
    rconf.steps_case_4 = MAX( 2, motor_manager.convert_mm_to_steps( retconf.medium_retraction,   axis ) );
    rconf.steps_case_5 = MAX( 2, motor_manager.convert_mm_to_steps( retconf.hard_retraction,     axis ) );
    rconf.steps_case_0 =         motor_manager.convert_mm_to_steps( retconf.wire_break_distance, axis   );
}

int32_t IRAM_ATTR G_EDM_PLANNER::get_retraction_steps( float travel_mm ){
    // XU and YV should all have the same step resolution
    return round( travel_mm * g_axis[sinker_axis]->steps_per_mm.get() );
}




//###########################################################
// Interrupts a line and does a flushing retraction on the
// sinker axis in the oposite direction of where the sinker
// line is headed to.
// Note: This retraction is only for single axis sinker jobs
//###########################################################
bool IRAM_ATTR G_EDM_PLANNER::do_flush_if_needed(){

    if( flush_retract_after > 0 && ( esp_timer_get_time() - flush_retract_timer_us >= flush_retract_after ) ){
        reset_flush_retract_timer();
        return flush_begin();
    } return false;

}



bool G_EDM_PLANNER::flush_end(){
    if( flconf.pwm_was_disabled ){
        toggle_pwm( false, 0 ); // re-enable PWM
    }
    gconf.gedm_flushing_motion = false; // unflag flushing motion
    reset_short_circuit_protection();
    reset_flush_retract_timer(); // reset the timer
    return true;
}


void G_EDM_PLANNER::flush_reenable_pwm(){
    if( flconf.pwm_was_disabled ){
        toggle_pwm( false, 0 ); // re-enable PWM
    }
    flconf.pwm_was_disabled = false;
}



bool G_EDM_PLANNER::flush_begin(){
    gconf.gedm_flushing_motion = true;  // flag to inform that a flushing motion is performed
    flconf.is_retracting       = true;
    flconf.steps_done_back     = 0;
    flconf.steps_done_forward  = 0;
    flconf.full_retraction     = false; // flagged if the wanted retraction distance was fully possible
    if( disable_spark_for_flushing ){
        toggle_pwm( true, 0 ); // turn spark off while retracting; will be re-enabled for the return motion
        flconf.pwm_was_disabled = true;
    } else {
        flconf.pwm_was_disabled = false;
    }
    flconf.steps_wanted = get_retraction_steps( flush_retract_mm );
    return true;
}








//########################################################################################
// Runs the line based on target step positions (int32_t)
// todo: convert all the longs to int? Maybe?
//#########################################################################################
bool IRAM_ATTR G_EDM_PLANNER::move_line( int32_t* target, Line_Config &line ){

    if( system_block_motion ){
        return false;
    }

    gconf.current_line = position_history_index_current;

    line.last_motion_plan        = 0;
    line.step_bits               = 0;
    line.direction_bits          = 0;
    line.step_event_count        = 0;

    int32_t target_steps[N_AXIS], position_steps[N_AXIS];
    line.step_count            = 0;
    bool    _success           = true;
    int current_step_delay     = line.step_delay; // backup

    memcpy(position_steps, sys_position, sizeof(sys_position));



    for( int axis=0; axis<N_AXIS; ++axis ) {
        line.line_math[axis].counter = 0.0;
        target_steps[axis]           = target[axis];
        line.line_math[axis].steps   = labs( target_steps[axis] - position_steps[axis] );
        line.step_event_count        = MAX( line.step_event_count, line.line_math[axis].steps );
        // Bug: the division was performed as integer division resulting in invalid deltas if the values where <1
        // casting the delta result to float was missing and low negatives where rounded creating sudden direction
        // Thanks to Nikolay for finding this bug
        line.line_math[axis].delta = ( ( float ) (target_steps[axis] - position_steps[axis] ) ) / g_axis[axis]->steps_per_mm.get();
        if (line.line_math[axis].delta < 0.0) {
            line.direction_bits |= bit(axis);
        } 
    }


    if( line.step_event_count <= 0 ){
        if( is_system_mode_edm() ){
            if( gconf.gedm_retraction_motion ){
                return true;
            } 
            /*if( position_history_is_at_final_index() ){
                gconf.edm_process_finished = true;
            }*/
            return true;
        }
        delayMicroseconds(10);
        return false;
    }
    motor_manager.motors_direction( line.direction_bits );

    int accel_time   = 20;//us 
    int accel_rounds = line.motion_plan_enabled?0:100;
    if( accel_rounds > 0 && ( accel_rounds * 2 ) > line.step_event_count ){
        // ensure accel and deaccel...
        accel_rounds = MAX(1, int(line.step_event_count/2)-10);
        accel_time   = 2; // ratio?... yeah.. well...
    }
    //accel_rounds=0;

    int64_t stepped_at = esp_timer_get_time();
    int sync_steps     = 0;
    /** the final loop to pulse the motors **/
    while( line.step_count < line.step_event_count ){
        
        // reset defaults
        line.step_delay       = current_step_delay; // restore previous step delay
        line.step_bits        = 0;                  // reset step bits
        line.skip_feed        = false;
        // insert pause on request
        if( gconf.edm_pause_motion ){
            pause();
        }

        if( // break conditions
            get_quit_motion() || probe_check( line )
            || ( !line.ignore_limit_switch && GRBL_LIMITS::limits_get_state() )
         ){ _success = false; break; }



        //##################################################################
        // Only used for lines with motionplan enabled
        // normal jogmotions etc. will not access this section
        //##################################################################
        if( line.motion_plan_enabled ){
            if( !gconf.gedm_flushing_motion ){
                _success = process_wire( line );
                if( ! _success ){ break; }
                if( !gconf.gedm_retraction_motion ){
                    if( line.enable_flushing ){
                        if( do_flush_if_needed() ){
                            // return false to exit forward motion and enter a retraction
                            _success = false;
                            break;
                        }
                    }
                }
                if( !line.ignore_feed_limit && line.step_delay < process_speeds.EDM ){
                    line.step_delay = process_speeds.EDM;
                }
            } else {
                // set some params for the flushing motion
                line.step_delay        = process_speeds.RAPID;
                line.ignore_feed_limit = true;
                line.skip_feed         = false;
            }
        }


        //##################################################################
        // Most precise accelereration known to mankind
        //##################################################################
        if( accel_rounds > 0 ){
            int steps_left = line.step_event_count-line.step_count;
            // poor mans accel
            if( line.step_count <= accel_rounds ){ // 99 done; 100 accel
                // accel
                line.step_delay += accel_time*(accel_rounds-line.step_count);
            } else if( steps_left <= accel_rounds ){ // 1000 total - 300 done = 700left
                // deaccel
                int steps_to_deaccel = line.step_event_count-line.step_count;
                line.step_delay += accel_time*(accel_rounds-steps_to_deaccel);
            } else {
                line.step_delay = current_step_delay;
            }
        }




        if( line.skip_feed || esp_timer_get_time() < stepped_at+line.step_delay ){

            //##################################################################
            // Keep waiting if step delay is not reached or feed is skipped
            //##################################################################
            continue;

        } else {

            //#####################################################################
            // At this point we know that a step will be fired
            // and can do some stuff like internal counting of retraction steps etc.
            //#####################################################################
            if( line.enable_position_history ){

                if( gconf.gedm_flushing_motion ){

                    // inside a flushing motion with a step getting done
                    if( gconf.gedm_retraction_motion ){ 

                        // Note: If the wanted distance is not possible it will not reach the wanted step_num and enforce a forward motion once the limit is reached
                        // retracting
                        if( flconf.steps_done_back >= flconf.steps_wanted ){
                            // wanted position reached; retraction fully done
                            // returning this function with success false will break the retraction and return to forward
                            flconf.full_retraction = true;
                            _success = false;
                            break;
                        }
                        ++flconf.steps_done_back;

                    } else {
                        int mplan = 1;
                        if( flconf.steps_done_forward >= flconf.steps_done_back-flush_offset_steps*3 ){
                            flush_reenable_pwm();
                            mplan = get_calculated_motion_plan( true );
                        } 
                        // back forward
                        if( flconf.steps_done_forward >= flconf.steps_done_back-flush_offset_steps || ( mplan >= 4 && flconf.steps_done_back > 2 ) ){
                            // back to the initial position minus the offset steps
                            flush_end();
                            _success = false; //
                            break;
                        }
                        ++flconf.steps_done_forward;
                    }

                } else {
                    if( gconf.gedm_retraction_motion ){
                        ++_state.total_retraction_steps;
                        ++rconf.steps_done;
                    } else {
                        ++no_load_steps; // very dirty.. assuming a load until it reset, This resets all around the code and is not accurate at all #todo
                        if( _state.total_retraction_steps > 0 ){
                            --_state.total_retraction_steps;
                        }
                    }
                }
            }

        }


        //##################################################################
        // Bresenham line algo to flag the stepbits and update the position
        //##################################################################
        sync_steps = 0; // the number of parallel steps tol be fired: 1 to N_AXIS is possible
        for (int axis = 0; axis < N_AXIS; axis++) {
            line.line_math[axis].counter += line.line_math[axis].steps;
            //if( line.line_math[axis].counter > line.step_event_count ) { // "">" could it be ">="?
            if( line.line_math[axis].counter >= line.step_event_count ) { // "">" could it be ">="?
                line.step_bits |= bit(axis);
                line.line_math[axis].counter -= line.step_event_count;
                if( line.direction_bits & bit(axis) ) {
                    sys_position[axis]--;
                } else {
                    sys_position[axis]++;
                }
                ++sync_steps;
            } 
        }
        //##################################################################
        // Finally fire the steps and update the step time
        //##################################################################
        if( sync_steps > 1 ){
            // more then one axis is doing a step here
            // could add something here to compnesate the higher load
            // the step position is already update and there is no way back 
        }
        motor_manager.motors_step( line.step_bits );
        stepped_at = esp_timer_get_time();

        if( sync_steps > 1 && line.motion_plan_enabled && !gconf.gedm_flushing_motion ){
            stepped_at += process_speeds.RAPID; // if there where multiple steps give it some extra time
        }

        delayMicroseconds(1);
        ++line.step_count;

    } 


    if( _success && !simulate_gcode && !gconf.gedm_flushing_motion ){
        //##################################################################
        // Only used in wire mode to do a little extra stuff on a line end
        // in forward direction
        //##################################################################
        if( is_system_op_mode( OP_MODE_EDM_WIRE ) && !gconf.gedm_retraction_motion ){
            wire_line_end( line );
        }
    }


    // delay the minimum required
    int64_t time_has = esp_timer_get_time() - stepped_at;
    int delay_rest = process_speeds.RAPID - time_has;
    if( delay_rest > 0 ){ delayMicroseconds( delay_rest ); }
    return _success;
}



/** 
  * This is the main gateway for lines
  * All normal lines are passed through this except for some special motions
  **/
uint8_t IRAM_ATTR G_EDM_PLANNER::plan_history_line( float* target, plan_line_data_t* pl_data ) {
    bool _success = true;
    // exit clean on aborts and motionstops
    if( get_quit_motion() ){
        idle_timer = esp_timer_get_time();
        override_target_with_current( target );
        gcode_core.gc_sync_position();
        gconf.gedm_planner_line_running = false;
        return false;
    }

    gconf.gedm_planner_line_running = true; 

    Line_Config line;

    if( pl_data->is_arc ){
        line.is_arc = true;
        ++_state.arc_counter;
    } else {
        line.is_arc = false;
        _state.arc_counter = 0;
    }

    // change the step delay / speed for this line; This may be overwritten in the process and is just a default value
    if( pl_data->step_delay ){
        line.step_delay = pl_data->step_delay;
    } else{
        line.step_delay = process_speeds.RAPID;
    }

    // set the default line configuration
    line.ignore_limit_switch = pl_data->use_limit_switches ? false : true; // defaults is no limits except for homing!

    if( 

        ( !is_machine_state( STATE_PROBING ) 
        && !pl_data->motion.systemMotion) 
        && is_system_mode_edm()

    ){ 

        if( ! simulate_gcode ){
            line.enable_position_history = true;
            line.step_delay              = process_speeds.EDM;
            line.motion_plan_enabled     = true;

            if( is_system_op_mode( OP_MODE_EDM_SINKER ) ){ // flushing interval needed only in sinker mode
                line.enable_flushing = true;
            }

        } else {
            line.enable_position_history = false;
            line.motion_plan_enabled     = false;
            line.step_delay              = process_speeds.RAPID;
        }

    } else if( is_machine_state( STATE_PROBING ) ){
        line.step_delay = process_speeds.PROBING;
    }

    if( line.motion_plan_enabled ){
        set_retraction_steps();
    }

    int32_t __target[N_AXIS];
    convert_target_to_steps( target, __target ); 

    _success = process_stage( __target, line );
    if( ! _success || system_block_motion ){
        override_target_with_current( target ); // if the line failed or motion got canceled override the target with the current position to update the position inside the gcode parser
    }
    position_history_force_sync(); // redundant? After sync this is also called.. Keept it for now..
    gcode_core.gc_sync_position(); 
    idle_timer = esp_timer_get_time();
    gconf.gedm_planner_line_running = false;


    return _success;
}
