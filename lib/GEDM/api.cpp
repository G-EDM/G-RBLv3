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

#include "api.h"
#include "GCode.h"
#include "widgets/ui_interface.h"
#include "widgets/char_helpers.h"


namespace api{

    //#######################################################
    // Check if the input buffer is empty
    //#######################################################
    bool buffer_available(){
        //return inputBuffer.peek() == -1 ? true : false;
        return ( inputBuffer.availableforwrite() > DEFAULT_LINE_BUFFER_SIZE );
    }

    //#######################################################
    // Push a gcode command to the input buffer
    //#######################################################
    IRAM_ATTR void push_cmd(const char *text, bool block_until_idle ){ 
        if( get_machine_state() >= STATE_ESTOP ){ return; } // Warning: Only way to recover from estop is with an unlock command. Before submitting the command change the state to STATE_ALARM
        increase_state_to_busy();
        acquire_lock_for( ATOMIC_LOCK_INPUT );
        inputBuffer.push(text); 
        release_lock_for( ATOMIC_LOCK_INPUT );
        debuglog(text,1);
        if( block_until_idle ){
            wait_for_idle_state( 20 );
        }
    }

    //############################################################################
    // Function returns an array with the max possible negative and positive
    // travel left to do for a given axis.
    // if UV axes are enabled it will combine XU and YV and provide the travels
    // based on the one with lesser travel left
    // return value is float[2] { negative_max, positive_max }
    // function is not perfect and can only be called with xyz 
    // to add UV and not with UV yet
    // pass only xyz axis as axis parameter. Not UV. It will chain UV axis to XY
    // but not vice versa
    // and only use this function limit XU YV combined motions
    // the code chains U to X if U is enabled and X is passed as parameter
    // and V to Y the same way
    // it does not chain X to U if U is passed as axis parameter and same for V
    //############################################################################
    void get_min_max_xyuvz( uint8_t axis, float *results ){

        uint8_t axis_mask = create_xu_yv_axis_mask( axis );

        if( axis != axes_active.z.index ){
            if( ( axis == axes_active.x.index ) && axes_active.u.enabled ){
                axis_mask |= bit(axes_active.x.index);
                axis_mask |= bit(axes_active.u.index);
            } else if( ( axis == axes_active.y.index ) && axes_active.v.enabled ){
                axis_mask |= bit(axes_active.y.index);
                axis_mask |= bit(axes_active.v.index);
            } else { axis_mask |= bit(axis); }
        } else { axis_mask |= bit(axis); }

        //############################################################################
        // Load the min and max possible travel left for the flagged axes
        //############################################################################
        float travel_left_negative = api::get_max_travel_possible( axis, 1 ); // 1=get negative max travel, negative value is returned, higher value equals less travel
        float travel_left_positive = api::get_max_travel_possible( axis, 0 ); // 0=get positive max travel; positive value is returned; smaller value equals less travel

        for( int _axis = 0; _axis < N_AXIS; ++_axis ){
            if( bitnum_istrue( axis_mask, _axis ) ){
                travel_left_negative = MAX( travel_left_negative, api::get_max_travel_possible( _axis, 1 ) );
                travel_left_positive = MIN( travel_left_positive, api::get_max_travel_possible( _axis, 0 ) );
            }
        }

        results[0] = travel_left_negative;
        results[1] = travel_left_positive;

    }

    uint8_t create_xu_yv_axis_mask( uint8_t axis ){
        uint8_t axis_mask = 0;
        if( axis != axes_active.z.index ){
            if( ( axis == axes_active.x.index ) && axes_active.u.enabled ){
                axis_mask |= bit(axes_active.x.index);
                axis_mask |= bit(axes_active.u.index);
            } else if( ( axis == axes_active.y.index ) && axes_active.v.enabled ){
                axis_mask |= bit(axes_active.y.index);
                axis_mask |= bit(axes_active.v.index);
            } else { axis_mask |= bit(axis); }
        } else { axis_mask |= bit(axis); }
        return axis_mask;
    }

    //#######################################################
    // 
    //#######################################################
    void send_axismaks_motion_gcode( uint8_t axis_mask, const char* cmd_start, const char* cmd_end, float* values, bool blocking ){
        char cmd_buffer[40]; // GCode for the sinker line
        int offset = 0;
        offset += snprintf( cmd_buffer + offset, sizeof(cmd_buffer) - offset, cmd_start );
        for ( int _axis = 0; _axis < N_AXIS; ++_axis ) {
            if( bitnum_istrue( axis_mask, _axis ) ){
                offset += snprintf( cmd_buffer + offset, sizeof(cmd_buffer) - offset, " %s%.5f", get_axis_name_const(_axis), values[_axis] );
            }
        }
        snprintf( cmd_buffer + offset, sizeof(cmd_buffer) - offset, cmd_end );
        push_cmd( cmd_buffer, blocking );
    }

    // get the max possible travel distance in mm for an axis into given direction
    // direction 1 = negative, 0 = positive
    // if will compensate the homing pulloff distance
    // not 100% exact
    // will return a negative value for max negative travel
    // will return a positive value for max positive minus homing pulloff 
    float get_max_travel_possible( int axis, int direction ){ 
        float* current_position = system_get_mpos(); 
        float current[N_AXIS];
        memcpy( current, current_position, sizeof(current_position[0]) * N_AXIS );
        float travel;
        if( direction ){
            // negative direction
            travel  = glimits.limits_negative_max( axis ); // -100.0 example for final min pos
            travel -= current[axis];                     // current = -60 = -40 (example with machine at -60 pos)
            travel += HOMING_PULLOFF;                               // remove a few steps just in case // -40+0,1 = -39,9
            travel  = MIN(0,travel);
        } else {
            // positive direction
            travel  = current[axis] * -1;
            travel -= HOMING_PULLOFF;
            travel  = MAX(0,travel);
        }
        return travel;
    }

    //##############################################################
    // Just very basic speed settings based on the slowest axis etc.
    // They may not represent the anything close to the real speed
    //##############################################################
    void update_speeds(){
        int slowest_axis                 = motor_manager.get_slowest_axis();
        process_speeds.RAPID             = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.RAPID );  
        process_speeds.INITIAL           = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.INITIAL );  
        process_speeds.EDM               = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.EDM );
        process_speeds.PROBING           = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.PROBING );
        process_speeds.REPROBE           = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.REPROBE );
        process_speeds.RETRACT           = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.RETRACT );
        process_speeds.HOMING_FEED       = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.HOMING_FEED );
        process_speeds.HOMING_SEEK       = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.HOMING_SEEK );
        process_speeds.WIRE_RETRACT_HARD = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.WIRE_RETRACT_HARD );
        process_speeds.WIRE_RETRACT_SOFT = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.WIRE_RETRACT_SOFT );
        vTaskDelay(10);
    }

    //#######################################################
    // Blocking loop while homing
    //#######################################################
    void block_while_homing( int axis_index ){
        wait_for_block_until_state( STATE_HOMING, STATE_BUSY );
        if( machine_is_fully_homed() ){
            push_cmd("G28.1\r\n"); // set home position
        }
    }

    //#######################################################
    // Get machine wpos
    //#######################################################
    IRAM_ATTR float* get_wpos(){
        float* current_position = system_get_mpos();
        mpos_to_wpos(current_position);
        return current_position;
    }

    //#######################################################
    // Check if machine is fully homed
    // Z is skipped if Z homing is disabled
    //#######################################################
    bool machine_is_fully_homed(){
        bool fully_homed = true;
        for( int axis = 0; axis < N_AXIS; ++axis ){

            if( 
                axis == axes_active.z.index && 
                !g_axis[ axis ]->homing_seq_enabled.get() 
            ){
                // skip Z if homing is not wanted. Electrode wears and if it starts to get shorter homing may be a problem.
                continue;
            }
            if( !g_axis[ axis ]->is_homed.get() ){
                fully_homed = false;
            }
        }
        return fully_homed;
    }

    //#######################################################
    // Check if a given axis is enabled
    // not too happy with it but ok #todo
    //#######################################################
    bool axis_enabled( uint8_t axis ){ // temporary solution; use only if  really needed
        if( axis == axes_active.x.index ){
            return axes_active.x.enabled;
        } else if( axes_active.y.index ){
            return axes_active.y.enabled;
        } else if( axes_active.z.index ){
            return axes_active.z.enabled;
        } else if( axes_active.u.index ){
            return axes_active.u.enabled;
        } else if( axes_active.v.index ){
            return axes_active.v.enabled;            
        }
        return false;
    }

    //#######################################################
    // Homeall homing sequence
    // Axes that are not enabled will be skipped
    //#######################################################
    void home_machine(){
        home_axis( axes_active.z.index );
        home_axis( axes_active.x.index );
        home_axis( axes_active.u.index );
        home_axis( axes_active.y.index );
        home_axis( axes_active.v.index );
    }

    //#######################################################
    // Home specific axis if the axis exists
    //#######################################################
    void home_axis( uint8_t axis ){
        if( axis >= N_AXIS || !axis_enabled( axis ) || system_block_motion ){ return; }
        if( axis == axes_active.z.index && axes_active.z.index < N_AXIS ){
            // z is a little special to prevent collisions where raising z would hit something
            if( !g_axis[ axis ]->homing_seq_enabled.get() ){
                g_axis[ axes_active.z.index ]->is_homed.set( true );
                return;
            }
        }
        char cmd_buffer[40]; // Ensure this buffer is large enough
        snprintf( cmd_buffer, sizeof(cmd_buffer), "$H%s\r", get_axis_name_const( axis ) );
        g_axis[ axis ]->is_homed.set( false );
        push_cmd( cmd_buffer );
        return;
        block_while_homing( axis );
        wait_for_idle_state();
    }

    //#######################################################
    // Move machine to all the zero positions
    // Z is ignored
    //#######################################################
    void move_to_origin(){ // ignores z
        char cmd_buffer[40];
        int offset = 0;
        offset += snprintf(cmd_buffer + offset, sizeof(cmd_buffer) - offset, "G90 G0");
        for (int i = 0; i < N_AXIS; ++i) {
            if( i == axes_active.z.index ){
                continue;
            }
            offset += snprintf(cmd_buffer + offset, sizeof(cmd_buffer) - offset, " %s0", get_axis_name_const(i));
        }
        snprintf(cmd_buffer + offset, sizeof(cmd_buffer) - offset, "\r\n");
        push_cmd(cmd_buffer);
    }

    //#######################################################
    // Convert probe position for given axis to mpos
    //#######################################################
    float probe_pos_to_mpos( int axis ){
        static float probe_position[N_AXIS];
        system_convert_array_steps_to_mpos( probe_position, sys_probe_position_final );
        return probe_position[axis];
    }

    //#######################################################
    // Check if machine is fully probed
    //#######################################################
    bool machine_is_fully_probed(){
        bool is_fully_probed = true;
        for( int i = 0; i < N_AXIS; ++i ){
            if( ! sys_probed_axis[i] ){
                is_fully_probed = false;
            }
        }
        return is_fully_probed;
    }

    //#######################################################
    // Reset work coordinates
    //#######################################################
    void reset_work_coords(){
        vTaskDelay(DEBUG_LOG_DELAY);
        char cmd_buffer[40];
        int offset = 0;
        offset += snprintf(cmd_buffer + offset, sizeof(cmd_buffer) - offset, "G91 G10 L20 P1");
        for (int i = 0; i < N_AXIS; ++i) {
            offset += snprintf(cmd_buffer + offset, sizeof(cmd_buffer) - offset, " %s0", get_axis_name_const(i));
        }
        snprintf(cmd_buffer + offset, sizeof(cmd_buffer) - offset, "\r\n");
        push_cmd(cmd_buffer);
    }

    //#######################################################
    // Reset probe positions for all axes
    //#######################################################
    void reset_probe_points(){
        for( int i = 0; i < N_AXIS; ++i ){
            unset_probepositions_for(i);
        }
    }

    //#######################################################
    // Unset probe position for given axis
    //#######################################################
    void unset_probepositions_for( int axis ){
        sys_probed_axis[axis]          = false;
        sys_probe_position_final[axis] = 0;
    };

    //#######################################################
    // Set current position as probe position for given axis
    //#######################################################
    void auto_set_probepositions_for( int axis ){
        char command_buf[40];
        sprintf(command_buf,"G91 G10 P1 L20 %s0\r\n", get_axis_name_const(axis));  
        push_cmd(command_buf,true); // blocking call
        sys_probed_axis[axis]          = true;
        sys_probe_position_final[axis] = sys_position[axis];
        vTaskDelay(100);
    };

    //#######################################################
    // Set current position as probe position for all axes
    // if the axis is not already probed!
    // After done it will also push a "G90 G54 G21" line
    //#######################################################
    void auto_set_probepositions(){
      for (int i = 0; i < N_AXIS; ++i){
          if (!sys_probed_axis[i]){
              auto_set_probepositions_for(i);
          }
      }
      push_cmd("G90 G54 G21\r\n",true);
      vTaskDelay(1000);
    }

    //#######################################################
    // Run a jog command for a given axis
    // If UV is enabled it will always combine XU and YV
    // if X, Y, U or V is jogged
    //#######################################################
    bool jog_axis( float mm, const char *axis_name, int speed, int axis ){ // todo: remove axis name requirenment
        if(  gconf.gedm_planner_line_running ){
          system_block_motion = true;
          while( gconf.gedm_planner_line_running ){
            vTaskDelay(1);
            system_block_motion = true;
          }
        }
        wait_for_idle_state();
        system_block_motion = digitalRead(ON_OFF_SWITCH_PIN) ? false : true;
        float* current_position = system_get_mpos();
        float position_copy[N_AXIS];
        memcpy(position_copy, current_position, sizeof(current_position[0]) * N_AXIS);

        uint8_t axis_mask = 0;

        char command_buf[40];

        if( xyuv_jog_combined &&
               (( axes_active.u.enabled && ( axis == axes_active.x.index || axis == axes_active.u.index ) )
            || ( axes_active.v.enabled && ( axis == axes_active.y.index || axis == axes_active.v.index )) )
        ){
                // combined jog enabled
                if( axis == axes_active.x.index || axis == axes_active.u.index ){
                    axis_mask |= bit(axes_active.x.index);
                    axis_mask |= bit(axes_active.u.index);
                    sprintf(command_buf,"$J=G91 G21 %s%.5f %s%.5f F%d\r\n", get_axis_name_const(axes_active.x.index), mm, get_axis_name_const(axes_active.u.index), mm, speed);    
                } else if ( axis == axes_active.y.index || axis == axes_active.v.index ){
                    axis_mask |= bit(axes_active.y.index);
                    axis_mask |= bit(axes_active.v.index);
                    sprintf(command_buf,"$J=G91 G21 %s%.5f %s%.5f F%d\r\n", get_axis_name_const(axes_active.y.index), mm, get_axis_name_const(axes_active.v.index), mm, speed);    
                } else {
                    axis_mask |= bit(axis);
                    sprintf(command_buf,"$J=G91 G21 %s%.5f F%d\r\n", axis_name, mm, speed);   
                }
        } else {
            axis_mask |= bit(axis);
            sprintf(command_buf,"$J=G91 G21 %s%.5f F%d\r\n", axis_name, mm, speed);   
        }

        for( int _axis = 0; _axis < N_AXIS; ++_axis ){
            if( bitnum_istrue( axis_mask, _axis ) ){
                position_copy[_axis] += mm;
            }
        }

        if( !glimits.target_within_reach( position_copy ) ){
            return false;
        }

        // cancel current jog motion if there is any
        // push new jog command to input buffer
        push_cmd( command_buf );
        //push_cmd("$J=G91 G21 X10 Y10 U10 V10 Z10 F100\r\n");
        return true;
    }


};