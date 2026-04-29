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

#ifndef G_STEPPER_H
#define G_STEPPER_H

#include <cstdint>
#include "Config.h"
#include "definitions.h"
#include "NutsBolts.h"

class G_STEPPER {

    public:

        G_STEPPER();
        virtual void           init( void )                                         = 0;
        virtual void           setup( GAxis* g_axis )                               = 0;
        virtual void IRAM_ATTR step( void )                                         = 0;
        virtual bool IRAM_ATTR set_direction( bool dir, bool add_dir_delay = true ) = 0; 
        virtual int            get_step_delay_for_speed( float mm_min )             = 0;
        bool                   motor_enabled( void );

    protected:

        uint8_t axis_index;
        uint8_t step_pin;
        uint8_t dir_pin;
        bool    invert_dir;
        bool    invert_step;
        int     current_direction;
        bool    enabled;


    //private:

};

#endif