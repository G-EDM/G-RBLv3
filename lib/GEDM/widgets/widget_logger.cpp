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


#include "widget_logger.h"


DRAM_ATTR uint16_t type_colors[]    = { TFT_DARKGREY, TFT_LIGHTGREY, TFT_GREEN, COLORORANGERED, TFT_DARKGREY, TFT_WHITE };
DRAM_ATTR int16_t  mini_font_height = 0;


IRAM_ATTR void PhantomLogger::show(){

    while( blocked ){ vTaskDelay(1); }

    if( ! is_visible || height <= 0 ) return; 

    blocked = true;

    TFT_eSPI* display = use_sprite ? get_sprite( width, height ) : &tft;

    display->setTextFont( MINI_FONT ); 
    display->setTextSize( 1 );

    static const int16_t item_height = 16;
    static const int16_t y_offset    = int16_t( ( item_height - mini_font_height ) >> 1 );

    int16_t items_num   = height >> 4;        // max possible items for given height /=16
    int16_t current_y   = height-item_height; // start y position, console lines in reverse added from bottom to top
    size_t  size        = get_buffer_size();  // capacity of the ring buffer

    if( items_num > size ){ items_num = size; }    

    reset_work_index();

    for( int i = 0; i < items_num; ++i ){
        const char* line = pop_line();
        display->setTextColor( type_colors[ getType() ] );
        display->drawString( line, 0, current_y+y_offset );
        current_y -= item_height;
        vTaskDelay(1);
    }

    if( use_sprite ){
        push( xpos,ypos, TFT_TRANSPARENT );
    }

    blocked = false;

};

IRAM_ATTR void PhantomLogger::create(){
    tft.setTextFont( MINI_FONT ); 
    tft.setTextSize( 1 );
    mini_font_height = tft.fontHeight();
};
IRAM_ATTR void PhantomLogger::redraw_item( int16_t item_index, bool reload ){};
IRAM_ATTR void PhantomLogger::build_ringbuffer( size_t capacity ){
    create();
    if( capacity <= 0 ){
        capacity = 14;//height/15;
    } 
    blocked = false;
    create_ringbuffer( capacity );
};
