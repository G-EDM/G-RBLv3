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

#ifndef PHANTOM_UI_WIDGET_SPLIST
#define PHANTOM_UI_WIDGET_SPLIST

#include "widget_base.h"

class PhantomSplitList : public PhantomWidget {
    public:
        PhantomSplitList( 
            int16_t w = 0, 
            int16_t h = 0, 
            int16_t x = 0, 
            int16_t y = 0, 
            int8_t type = 0, 
            bool _use_sprite = false 
        ) : PhantomWidget(w,h,x,y,type,_use_sprite ), use_custom_onoff_labels(false) {};
        virtual ~PhantomSplitList(){};
        void create( void )                   override;
        void show( void )                     override;
        IRAM_ATTR void redraw_item( int16_t item_index, bool reload = true ) override;
        std::shared_ptr<PhantomWidget> set_column_left_width( int16_t width );
        std::shared_ptr<PhantomWidget> set_on_off_text( const char* _on_text, const char* _off_text );
    private:
        bool use_custom_onoff_labels;
        int16_t column_width_left = -1;
        const char* on_text;
        const char* off_text;
};

#endif