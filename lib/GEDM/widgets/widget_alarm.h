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

#ifndef PHANTOM_UI_WIDGET_ALARM
#define PHANTOM_UI_WIDGET_ALARM

#include "widget_base.h"

class PhantomAlarm : public PhantomWidget {
    public:
        PhantomAlarm( 
            int16_t w = 0, 
            int16_t h = 0, 
            int16_t x = 0, 
            int16_t y = 0, 
            int8_t type = 0, 
            bool _use_sprite = false 
        ) : PhantomWidget(w,h,x,y,type,_use_sprite ){};
        virtual ~PhantomAlarm(){};
        void create( void )                   override;
        void show( void )                     override;
        IRAM_ATTR void redraw_item( int16_t item_index, bool reload = true ) override;
        void set_error_code( uint8_t __error_code );

    private:
        uint8_t error_code;

};


#endif