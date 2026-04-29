#pragma once

#include <vector>
//#include "config/definitions.h"

extern void make_settings( void );
extern void settings_init( void );

class control_cmd {

    private:
        const char*   _cmd_name;
        Error ( *_action )( const char* );

    public:
        static std::vector<control_cmd*> commands_all;
        control_cmd( const char* cmd_name, Error ( *action )( const char* ) );
        const char* get_cmd_name( void );
        Error action(char* value);

};

class Coordinates {

    private:
        float       _coords[N_AXIS];
        const char* _name;
    public:
        Coordinates( const char* name );
        const char*  getName( void );
        void         setDefault( void );
        void         get( float* value ); // Copy the value to an array
        const float* get( void );         // Return a pointer to the array
        void         set( float* value );

};

extern Coordinates* coords[CoordIndex::End];