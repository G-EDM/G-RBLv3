#include "Grbl.h"
//#include <map>


Error unlock_machine_state( const char* value ){
    reset_machine_state();
    return Error::Ok;
}

Error home( int cycle ){
    set_machine_state( STATE_HOMING );
    motion.homing_cycle(cycle);
    set_machine_state( STATE_IDLE ); // Set to IDLE when complete.
    return Error::Ok;
}
Error home_x( const char* value ){ return axes_active.x.enabled ? home(bit(axes_active.x.index)) : Error::Ok; }
Error home_y( const char* value ){ return axes_active.y.enabled ? home(bit(axes_active.y.index)) : Error::Ok; }
Error home_z( const char* value ){ return axes_active.z.enabled ? home(bit(axes_active.z.index)) : Error::Ok; }
Error home_a( const char* value ){ return axes_active.a.enabled ? home(bit(axes_active.a.index)) : Error::Ok; }
Error home_b( const char* value ){ return axes_active.b.enabled ? home(bit(axes_active.b.index)) : Error::Ok; }
Error home_c( const char* value ){ return axes_active.c.enabled ? home(bit(axes_active.c.index)) : Error::Ok; }
Error home_u( const char* value ){ return axes_active.u.enabled ? home(bit(axes_active.u.index)) : Error::Ok; }
Error home_v( const char* value ){ return axes_active.v.enabled ? home(bit(axes_active.v.index)) : Error::Ok; }

Error autohome_at_center( const char *value ){
    int   step_pos = 0;
    float mm_pos   = 0.0;
    for( int axis = 0; axis < N_AXIS; ++axis ){
        mm_pos   = float( (float)g_axis[ axis ]->max_travel_mm.get() / 2 ) * -1; // 240/2*-1=-120
        step_pos = round( mm_pos * g_axis[ axis ]->steps_per_mm.get() ); // -120*67=
        sys_position[ axis ]   = step_pos;
        g_axis[ axis ]->is_homed.set( true );
    }
    gcode_core.gc_sync_position();
    return Error::Ok;
}

Error doJog( const char* value ){
    // For jogging, you must give gc_execute_line() a line that begins with $J=.  There are several ways we can get here,
    // including  $J, $J=xxx, [J]xxx.  For any form other than $J without =, we reconstruct a $J= line for gc_execute_line().
    if( !value ){ return Error::InvalidStatement; }
    char jogLine[DEFAULT_LINE_BUFFER_SIZE];
    strcpy(jogLine, "$J=");
    strcat(jogLine, value);
    return gcode_core.gc_execute_line( jogLine );
}

Error motor_disable( const char* value ){
    motor_manager.motors_set_disable( true, B11111111 ); // enable/disable uses one GPIO for all motors. No need for single motor disable features here 
    return Error::Ok;
}

// normalize_key puts a key string into canonical form -
// without whitespace.
// start points to a null-terminated string.
// Returns the first substring that does not contain whitespace.
// Case is unchanged because comparisons are case-insensitive.
char* normalize_key( char* start ){
    char c;
    while (isspace(c = *start) && c != '\0') { ++start; }
    if (c == '\0') { return start; }
    char* end;
    for (end = start; (c = *end) != '\0' && !isspace(c); end++) {}
    *end = '\0';
    return start;
}

// This is the handler for all forms of settings commands,
// $..= and [..], with and without a value.
Error do_command_or_setting(const char* key, char* value) {
    // If value is NULL, it means that there was no value string, i.e.
    // $key without =, or [key] with nothing following.
    // If value is not NULL, but the string is empty, that is the form
    // $key= with nothing following the = .  It is important to distinguish
    // those cases so that you can say "$N0=" to clear a startup line.
    // If we did not find a setting, look for a command.  Commands
    // handle values internally; you cannot determine whether to set
    // or display solely based on the presence of a value.
    for( control_cmd* cmd : control_cmd::commands_all ){
        if ( ( cmd->get_cmd_name() && strcasecmp( cmd->get_cmd_name(), key ) == 0 ) ){ return cmd->action(value); }
    }
    return Error::InvalidStatement;
}

Error system_execute_line(char* line) {
    char* value;
    if (*line++ == '[') { // [ESPxxx] form
        value = strrchr(line, ']');
        if (!value) {
            return Error::InvalidStatement; // Missing ] is an error in this form
        }
        *value++ = '\0'; // ']' was found; replace it with null and set value to the rest of the line.
        if( *value == '\0' ){ // If the rest of the line is empty, replace value with NULL.
            value = NULL;
        }
    } else {
        value = strchr(line, '='); // $xxx form
        if (value) {
            *value++ = '\0'; // $xxx=yyy form.
        }
    }
    char* key = normalize_key(line);
    // At this point there are three possibilities for value
    // NULL - $xxx without =
    // NULL - [ESPxxx] with nothing after ]
    // empty string - $xxx= with nothing after
    // non-empty string - [ESPxxx]yyy or $xxx=yyy
    return do_command_or_setting(key, value);
}

void settings_init() {
    make_settings();
    new control_cmd("J",   doJog);                // Jog
    new control_cmd("HCT", autohome_at_center);   // Home/O
    new control_cmd("HX",  home_x);               // Home/X
    new control_cmd("HY",  home_y);               // Home/Y
    new control_cmd("HZ",  home_z);               // Home/Z
    new control_cmd("HA",  home_a);               // Home/A
    new control_cmd("HB",  home_b);               // Home/B
    new control_cmd("HC",  home_c);               // Home/C
    new control_cmd("HU",  home_u);               // Home/U
    new control_cmd("HV",  home_v);               // Home/V
}