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

#include "widget_list.h"

void PhantomList::create( void ){};


//#####################################################################################
// This class is a little special as it is used for the machine positions in the
// process and needs to be as fast as possible
//#####################################################################################
IRAM_ATTR void PhantomList::draw_list_item( TFT_eSPI* display, int16_t item_index, bool reload, int16_t screen_x_offset, int16_t screen_y_offset ){

    items[item_index].changed = false;
    if( ! is_visible ){ return; }
    if( reload ){ items[item_index].emit_load(); }

    int bg_color, fg_color;
    set_item_colors( item_index, bg_color, fg_color );

    items[item_index].set_font( *display );
    display->setTextColor( fg_color ); 

    if( items[item_index].label_text_width == -1 ){ 
        // looks like this is the initial call to draw this item
        // perform all the initial calculations that are static as long as the item exists
        items[item_index].label_text_width = display->textWidth( items[item_index].label.c_str() );
    }

    int16_t  margin   = items[item_index].text_align == 1 ? 0 : 5;
    int16_t  y_offset = screen_y_offset + int16_t( ( items[item_index].height - display->fontHeight() ) >> 1 );
    uint16_t x_abs    = screen_x_offset + items[item_index].label_text_width + margin * 2; // x screen offset + label width + 2times margin

    if( items[item_index].label_text_width > 0 ){
        display->drawString( items[item_index].label.c_str(), margin+screen_x_offset, y_offset );vTaskDelay(1);
    }

    switch ( items[item_index].output_type ){
        case OUTPUT_TYPE_INT: // int
            display->drawNumber( items[item_index].intValue, x_abs, y_offset );vTaskDelay(1);
        break;
        case OUTPUT_TYPE_FLOAT: // float
            display->drawFloat( items[item_index].floatValue, items[item_index].float_decimals, x_abs, y_offset );vTaskDelay(1);
        break;
        case OUTPUT_TYPE_STRING: // string
        case OUTPUT_TYPE_CUSTOM: // string
            display->drawString( items[item_index].get_visible(), x_abs, y_offset );vTaskDelay(1);
        break;
        default:
        break;
    }

}

void PhantomList::show( void ){
    if( ! is_visible ){ return; }
    TFT_eSPI* display = use_sprite ? get_sprite( width, height, TFT_TRANSPARENT ) : &tft;
    display->setTextSize( 1 );
    if( num_items <= 0 ){ num_items = 1; }
    int16_t item_height = 0;
    int16_t item_width  = 0;
    int16_t current_y   = 0;
    int16_t current_x   = 0;
    if( widget_type <= 1 ){
        // single row horizontal list
        item_height = height;
        item_width  = width/num_items;
    } else if( widget_type == 2 ){
        // vertical list
        item_height = height/num_items;
        item_width  = width;
    }

    for( auto& _item : items ){

        _item.second.height = item_height;    // save the item height to the item
        _item.second.width  = item_width;     // save the item widht to the item
        _item.second.truex  = xpos+current_x; // save the true x position relative to the display
        _item.second.truey  = ypos+current_y; // save the true y position relative to the display

        draw_list_item( 
            display,
            _item.second.item_index,
            true,
            ( use_sprite ? current_x : _item.second.truex ),
            ( use_sprite ? current_y : _item.second.truey )
        );

        if( widget_type <= 1 ){
            current_x +=item_width;
        } else if( widget_type == 2 ){
            current_y +=item_height;
        }

    }

    if( use_sprite ){
        push( xpos, ypos, TFT_TRANSPARENT );
    }

};

IRAM_ATTR void PhantomList::redraw_item( int16_t item_index, bool reload ){
    items[item_index].changed = false;
    if( ! is_visible ){ return; }
    draw_list_item( 
        get_sprite( items[item_index].width, items[item_index].height ),
        item_index,
        reload
    );
    push( items[item_index].truex,items[item_index].truey );

};
