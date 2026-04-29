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

#include "widget_split_list.h"
#include "NutsBolts.h"

void PhantomSplitList::create( void ){};


std::shared_ptr<PhantomWidget> PhantomSplitList::set_on_off_text( const char* _on_text, const char* _off_text ){
    on_text  = _on_text;
    off_text = _off_text;
    use_custom_onoff_labels = true;
    return shared_from_this();
}


std::shared_ptr<PhantomWidget> PhantomSplitList::set_column_left_width( int16_t width ){
    column_width_left = width;
    return shared_from_this();
};

void PhantomSplitList::show( void ){

    if( ! is_visible ){ return; }

    TFT_eSPI* display = use_sprite ? get_sprite( width, height ) : &tft;
    display->setTextSize( 1 );

    if( num_items <= 0 ){
        num_items = 1; //
    }

    int16_t item_height = 0;
    int16_t item_width  = 0;
    int16_t current_y   = use_sprite ? 0 : ypos;
    int16_t y_offset    = 0;
    int16_t text_width  = 0;
    int16_t margin      = 20;
    int16_t xxpos       = 0;

    item_height              = 30;
    item_width               = width;
    int     width_half_onoff = 50;
    int16_t on_w = 0, off_w = 0, on_pos = 0, off_pos = 0;
    int bg_color, fg_color;

    for( auto& _item : items ){

        _item.second.emit_load();
        _item.second.set_font( *display );

        set_item_colors( _item.second.item_index, bg_color, fg_color );

        if( _item.second.output_type == OUTPUT_TYPE_TITLE ){
            item_height = 24;
            margin      = 5;
        } else {
            item_height = 30;
            margin      = 5;
        }

        y_offset            = int16_t( ( item_height - display->fontHeight() ) / 2 );
        _item.second.height = item_height; // save the item height to the item
        _item.second.width  = item_width;  // save the item widht to the item
        _item.second.truex  = xpos;        // save the true x position relative to the display
        _item.second.truey  = use_sprite?current_y+ypos:current_y;   // save the true y position relative to the display

        display->setTextColor( fg_color );
        display->drawString( _item.second.label.c_str(), margin, current_y+y_offset );vTaskDelay(1);
        text_width = column_width_left==-1?210:column_width_left;

        if( _item.second.disabled ){

            display->drawString( "Not available!", text_width, current_y+y_offset );vTaskDelay(1);

        } else {

            display->setTextColor( TFT_GREEN );


        switch ( _item.second.output_type ){
            case OUTPUT_TYPE_INT: // int
                display->drawNumber( _item.second.intValue, text_width, current_y+y_offset );vTaskDelay(1);
            break;
        
            case OUTPUT_TYPE_FLOAT: // float
                display->drawFloat( _item.second.floatValue, _item.second.float_decimals, text_width, current_y+y_offset );vTaskDelay(1);
            break;

            case OUTPUT_TYPE_BOOL: // bool
                on_w    = display->textWidth(use_custom_onoff_labels?on_text:"ON");
                off_w   = display->textWidth(use_custom_onoff_labels?off_text:"OFF");
                width_half_onoff = MAX( width_half_onoff, MAX(on_w+8,off_w+8) );
                on_pos  = (width_half_onoff-on_w)/2;
                off_pos = (width_half_onoff-off_w)/2;
                if( _item.second.boolValue ){
                    display->fillRect( text_width,                  current_y+6, width_half_onoff, item_height-12, TFT_GREEN);   vTaskDelay(1);
                    display->fillRect( text_width+width_half_onoff, current_y+6, width_half_onoff, item_height-12, TFT_DARKGREY);vTaskDelay(1);
                } else {
                    display->fillRect( text_width,                  current_y+6, width_half_onoff, item_height-12, TFT_DARKGREY);   vTaskDelay(1);
                    display->fillRect( text_width+width_half_onoff, current_y+6, width_half_onoff, item_height-12, TFT_ORANGE);vTaskDelay(1);
                }
                display->setTextColor(TFT_BLACK);
                display->drawString( use_custom_onoff_labels?on_text:"ON", text_width+on_pos, current_y+y_offset, 2 );vTaskDelay(1);
                display->drawString( use_custom_onoff_labels?off_text:"OFF", text_width+width_half_onoff+off_pos, current_y+y_offset, 2 );vTaskDelay(1);
            break;

            case OUTPUT_TYPE_STRING: // string
            case OUTPUT_TYPE_CUSTOM: // string
                display->drawString( _item.second.get_visible(), text_width, current_y+y_offset );vTaskDelay(1);
            break;

            default:
            break;
        }

        }
        current_y +=item_height;
    }
    if( use_sprite ){
        push( xpos, ypos );
    }

};
IRAM_ATTR void PhantomSplitList::redraw_item( int16_t item_index, bool reload ){
    items[item_index].changed = false;
    if( ! is_visible ){ return; }
    if( reload ){ items[item_index].emit_load(); }
    TFT_eSPI* display = get_sprite( items[item_index].width, items[item_index].height );
    int16_t margin = 5;
    int bg_color, fg_color;
    set_item_colors( item_index, bg_color, fg_color );
    items[item_index].set_font( *display );
    display->setTextColor( fg_color ); // draw the label
    int16_t text_width = display->textWidth( items[item_index].label.c_str() );
    int16_t y_offset   = int16_t( ( items[item_index].height - display->fontHeight() ) / 2 );
    display->drawString( items[item_index].label.c_str(), margin, y_offset );vTaskDelay(1);
    text_width = column_width_left==-1?210:column_width_left;

    int width_half_onoff = 50;
    int16_t on_w         = display->textWidth(use_custom_onoff_labels?on_text:"ON");
    int16_t off_w        = display->textWidth(use_custom_onoff_labels?off_text:"OFF");
    width_half_onoff = MAX( width_half_onoff, MAX(on_w+8,off_w+8) );
    int16_t on_pos       = (width_half_onoff-on_w)/2;
    int16_t off_pos      = (width_half_onoff-off_w)/2;

    if( items[item_index].disabled ){
        display->drawString( "Not available!", text_width, y_offset );vTaskDelay(1);
    } else {
        display->setTextColor( TFT_GREEN );
        switch ( items[item_index].output_type ){
            case OUTPUT_TYPE_INT: // int
                display->drawNumber( items[item_index].intValue, text_width, y_offset );vTaskDelay(1);
            break;
            case OUTPUT_TYPE_FLOAT: // float
                display->drawFloat( items[item_index].floatValue, items[item_index].float_decimals, text_width, y_offset );vTaskDelay(1);
            break;
            case OUTPUT_TYPE_BOOL: // bool
                if( items[item_index].boolValue ){
                    display->fillRect( text_width,                  6, width_half_onoff, items[item_index].height-12, TFT_GREEN);   vTaskDelay(1);
                    display->fillRect( text_width+width_half_onoff, 6, width_half_onoff, items[item_index].height-12, TFT_DARKGREY);vTaskDelay(1);
                } else {
                    display->fillRect( text_width,                  6, width_half_onoff, items[item_index].height-12, TFT_DARKGREY);   vTaskDelay(1);
                    display->fillRect( text_width+width_half_onoff, 6, width_half_onoff, items[item_index].height-12, TFT_ORANGE);vTaskDelay(1);
                }
                display->setTextColor(TFT_BLACK);
                display->drawString( use_custom_onoff_labels?on_text:"ON", text_width+on_pos, y_offset, 2 );vTaskDelay(1);
                display->drawString( use_custom_onoff_labels?off_text:"OFF", text_width+width_half_onoff+off_pos, y_offset, 2 );vTaskDelay(1);
            break;
            case OUTPUT_TYPE_STRING: // string
            case OUTPUT_TYPE_CUSTOM: // string
                display->drawString( items[item_index].get_visible(), text_width, y_offset );vTaskDelay(1);
            break;
            default:
            break;
        }
    }
    push( items[item_index].truex,items[item_index].truey );

};