#include "Grbl.h"

//#################################################
// Command set that holds all the command like
// Jog, Enable/Disable, HomeXYZ etc.
//#################################################
std::vector<control_cmd*> control_cmd::commands_all;

//########################################################
// Command object that holds the command name and callback
//########################################################
control_cmd::control_cmd( const char* cmd_name, Error ( *action )( const char* ) )  : _cmd_name( cmd_name), _action( action ) {
    commands_all.push_back(this);
}
Error control_cmd::action( char* value ) {
    return _action( (const char*)value );
};
const char* control_cmd::get_cmd_name(){ return _cmd_name; }


//########################################################
// Object to hold all coordinate system 
//########################################################
Coordinates* coords[CoordIndex::End];


//########################################################
// Coordinate system object
//########################################################
Coordinates::Coordinates( const char* name ) : _name( name ){}
//void Coordinates::set( float* value ) {
void Coordinates::set( float value[N_AXIS] ) {
    memcpy( &_coords, value, sizeof( _coords ) );
}
void Coordinates::get( float* value ){ 
    memcpy( value, _coords, sizeof(_coords) ); 
}
const float* Coordinates::get(){ 
    return _coords; 
}
const char* Coordinates::getName(){ 
    return _name; 
}
void Coordinates::setDefault(){
    float zeros[N_AXIS] = { 0.0, };
    set( zeros );
};



void make_coordinate( CoordIndex index, const char* name ) {
    float coord_data[N_AXIS] = { 0.0 };
    auto  coord                  = new Coordinates( name );
    coords[index]                = coord;
    coords[index]->setDefault();
}

void make_settings() {
    // Propagate old coordinate system data to the new format if necessary. G54 - G59 work coordinate systems, G28, G30 reference positions, etc
    make_coordinate(CoordIndex::G54, "G54");
    make_coordinate(CoordIndex::G55, "G55");
    make_coordinate(CoordIndex::G56, "G56");
    make_coordinate(CoordIndex::G57, "G57");
    make_coordinate(CoordIndex::G58, "G58");
    make_coordinate(CoordIndex::G59, "G59");
    make_coordinate(CoordIndex::G28, "G28");
    make_coordinate(CoordIndex::G30, "G30");
    //axis_conf* defaults;
    int index = 0;
    for( int axis = 0; axis < N_AXIS; ++axis ){
        //defaults = &axis_configuration[axis];
        axis_conf defaults = axis_configuration[axis];
        if( !defaults.enabled ){ continue; }
        g_axis[index] = new GAxis( defaults.name );
        g_axis[index]->steps_per_mm.set( defaults.steps_per_mm );
        g_axis[index]->home_position.set( defaults.home_mpos );
        g_axis[index]->is_homed.set( false );
        g_axis[index]->max_travel_mm.set( defaults.max_travel );
        g_axis[index]->axis_index.set( index );
        g_axis[index]->dir_pin.set( defaults.dir_pin );
        g_axis[index]->step_pin.set( defaults.step_pin );
        g_axis[index]->motor_type.set( defaults.motor_type );
        g_axis[index]->homing_seq_enabled.set( defaults.homing_seq_enabled );
        MASHINE_AXES_STRING[index] = defaults.name[0];
        enable_axis( defaults.name, index );
        ++index;
    }
}