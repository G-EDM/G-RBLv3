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

#include "widget_vertical_button_list.h"

void PhantomVerticalButtonList::create(){}
void PhantomVerticalButtonList::show(){

    if( ! is_visible ){ return; }

    TFT_eSPI* display = use_sprite ? get_sprite( width, height ) : &tft;
    display->setTextSize( 1 );

    if( num_items <= 0 ){
        num_items = 1; //
    }

    int16_t item_height        = int16_t( height / num_items ); // item height is widget height / num items
    int16_t item_width         = width;                         // vertical button list items are as width as the widget
    int16_t current_y          = 0;                             // current y position of the item inside the widget
    int16_t y_offset           = 0;                             // y offset to center the text vertically in the item
    int16_t text_width         = 0;                             // text width is needed for horizontal centering
    bool    ignore_top_outline = true;                          // first item does not get the top outline

    for( auto& _item : items ){
        _item.second.emit_load();
        _item.second.set_font( *display );
        const char* label = _item.second.label.c_str(); // convert the label to std::string
        y_offset          = int16_t( ( item_height - display->fontHeight() ) / 2 );
        _item.second.height = item_height;            // save the item height to the item
        _item.second.width  = item_width;             // save the item widht to the item
        _item.second.truex  = xpos+_item.second.xpos; // save the true x position relative to the display
        _item.second.truey  = ypos+current_y;         // save the true y position relative to the display
        int bg_color, fg_color;
        set_item_colors( _item.second.item_index, bg_color, fg_color );
        display->fillRect(0,current_y,item_width,item_height,bg_color);vTaskDelay(1); // draw the item background
        if( !ignore_top_outline ){ // draw top outline
            _item.second.item_outline = 1;
            display->drawFastHLine( 0, current_y, item_width, BUTTON_LIST_LI ); // draw the item outline
        }
        display->setTextColor( fg_color );
        text_width = display->textWidth( label );
        display->drawString( label, int16_t((item_width-text_width)/2), current_y+y_offset );vTaskDelay(1);
        current_y         +=item_height;
        ignore_top_outline = false;
    }
    if( use_sprite ){
        push( xpos, ypos );
    }
}


IRAM_ATTR void PhantomVerticalButtonList::redraw_item( int16_t item_index, bool reload ){
    items[item_index].changed = false;
    if( ! is_visible ){ return; }
    if( reload ){ items[item_index].emit_load(); }
    TFT_eSPI* display = get_sprite( items[item_index].width, items[item_index].height );
    int bg_color, fg_color;
    set_item_colors( item_index, bg_color, fg_color );
    display->fillRect(0,0,items[item_index].width, items[item_index].height, bg_color);vTaskDelay(1); // draw the item background
    if( items[item_index].item_outline ){
        display->drawFastHLine( 0, 0, items[item_index].width, BUTTON_LIST_LI ); // draw the item outline
    }
    items[item_index].set_font( *display );
    display->setTextColor( fg_color ); // draw the label
    int16_t text_width = display->textWidth( items[item_index].label.c_str() );
    int16_t y_offset   = int16_t( ( items[item_index].height - display->fontHeight() ) / 2 );
    display->drawString( items[item_index].label.c_str(), int16_t((items[item_index].width-text_width)/2), y_offset );vTaskDelay(1);
    push( items[item_index].truex,items[item_index].truey );
}