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

#ifndef PHANTOM_UI_WIDGET_LOGGER
#define PHANTOM_UI_WIDGET_LOGGER


#include "widget_base.h"
#include "char_ringbuffer.h"

//extern portMUX_TYPE msg_logger_mutex;

class PhantomLogger : public PhantomWidget, 
                      public CharRingBuffer {
    public:
        PhantomLogger( 
            int16_t w = 0, 
            int16_t h = 0, 
            int16_t x = 0, 
            int16_t y = 0, 
            int8_t type = 0, 
            bool _use_sprite = false 
        ) : PhantomWidget(w,h,x,y,type,_use_sprite ){};
        virtual ~PhantomLogger(){};
        IRAM_ATTR void create( void )                                       override;
        IRAM_ATTR void show( void )                                         override;
        IRAM_ATTR void redraw_item( int16_t item_index, bool reload = true ) override;
        IRAM_ATTR void build_ringbuffer( size_t capacity );
        bool new_log_msg;


    private: 
        bool blocked;
};


#endif