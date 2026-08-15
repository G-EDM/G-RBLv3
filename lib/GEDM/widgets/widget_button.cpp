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

#include "widget_button.h"

void PhantomButton::create( void ){};


void PhantomButton::show( void ){

    if( ! is_visible ){ return; }

    TFT_eSPI* display = use_sprite ? get_sprite( width, height, TFT_TRANSPARENT ) : &tft;

    for( auto& _item : items ){

        _item.second.truex = use_sprite ? xpos + _item.second.xpos : _item.second.xpos;
        _item.second.truey = use_sprite ? ypos + _item.second.ypos : _item.second.ypos;

        draw_button_item( 
            display,
            _item.second.item_index,
            true,
            ( use_sprite ? _item.second.xpos : _item.second.truex ),
            ( use_sprite ? _item.second.ypos : _item.second.truey )
        );

    }

    if( use_sprite ){
        push( xpos, ypos, TFT_TRANSPARENT );
    }

};


IRAM_ATTR void PhantomButton::redraw_item( int16_t item_index, bool reload ){
    //items[item_index].changed = false;
    if( ! is_visible ){ return; }
    if( reload ){ items[item_index].emit_load(); }

    draw_button_item( 
        get_sprite( items[item_index].width, items[item_index].height ),
        item_index,
        reload
    );
    push( items[item_index].truex,items[item_index].truey );

};


IRAM_ATTR void PhantomButton::draw_button_item( TFT_eSPI* display, int16_t item_index, bool reload, int16_t screen_x_offset, int16_t screen_y_offset ){

    //items[item_index].changed = false;
    if( ! is_visible ){ return; }
    if( reload ){ items[item_index].emit_load(); }

    int bg_color, fg_color;
    set_item_colors( item_index, bg_color, fg_color );

    items[item_index].set_font( *display );

    if( !transparent_background ){

        if( items[item_index].radius > 0 ){

            display->fillRoundRect(
                screen_x_offset,
                screen_y_offset,
                items[item_index].width, 
                items[item_index].height, 
                items[item_index].radius, 
                bg_color
            );
            vTaskDelay(1); // draw the item background

        } else {

            display->fillRect(
                screen_x_offset,
                screen_y_offset,
                items[item_index].width, 
                items[item_index].height, 
                bg_color
            );
            vTaskDelay(1); // draw the item background
        
        }

    } 

    if( items[item_index].outline != 0 ){

        switch (items[item_index].outline){

            case 1:
                display->drawFastVLine( screen_x_offset, screen_y_offset, items[item_index].height, TFT_BLACK );
                display->drawFastVLine( screen_x_offset+items[item_index].width-1, screen_y_offset, items[item_index].height, TFT_BLACK );
            break;

            case 2:
                display->drawFastVLine( screen_x_offset+items[item_index].width-1, screen_y_offset, items[item_index].height, TFT_BLACK );
            break;

            default:
            break;

        }

    }


    if( items[item_index].font == -1 ){ // if item font == -1 we select the icon font
        display->setFreeFont(FONT_A);
    } else {
        display->setTextFont( items[item_index].font ); 
        display->setTextSize( 1 );
    }

    display->setTextColor( fg_color ); // draw the label

    int16_t y_offset = screen_y_offset + int16_t( ( items[item_index].height - display->fontHeight() ) >> 1 );

    if( items[item_index].font == -1 ){
        y_offset += 4;
    }

    if( items[item_index].output_type == OUTPUT_TYPE_STRING ){
        int16_t text_width = display->textWidth( items[item_index].get_visible() );
        int16_t x_pos      = screen_x_offset + ( items[item_index].text_align == 1 ? 0 : int16_t( ( items[item_index].width - text_width ) >> 1 ) );
        display->drawString( items[item_index].get_visible(), x_pos, y_offset );vTaskDelay(1);

    } else {
        int16_t text_width = display->textWidth( items[item_index].label.c_str() );
        int16_t x_pos      = screen_x_offset + ( items[item_index].text_align == 1 ? 0 : int16_t( ( items[item_index].width - text_width ) >> 1 ) );
        display->drawString( items[item_index].label.c_str(), x_pos, y_offset );vTaskDelay(1);
    }


}


/*#include "widget_button.h"

void PhantomButton::create( void ){
};

void PhantomButton::show( void ){
    if( ! is_visible ){ return; }
    TFT_eSPI* display;
    if( use_sprite ){
        display = &canvas;
        canvas.createSprite( width, height );vTaskDelay(1);
        canvas.fillSprite(TFT_TRANSPARENT);
    } else {
        display = &tft; // only sprite is supported for this widget right now...
    }
    display->setTextSize( 1 );
    if( num_items <= 0 ){
        num_items = 1; //
    }
    int16_t text_width = 0;
    int16_t y_offset   = 0;
    int16_t x_pos      = 0;
    for( auto& _item : items ){
        _item.second.emit_load();
        _item.second.set_font( *display );
        _item.second.truex = use_sprite ? xpos + _item.second.xpos : _item.second.xpos;
        _item.second.truey = use_sprite ? ypos + _item.second.ypos : _item.second.ypos;
        int bg_color, fg_color;
        set_item_colors( _item.second.item_index, bg_color, fg_color );
        if( !transparent_background ){
            if( _item.second.radius > 0 ){
                display->fillRoundRect( _item.second.xpos, _item.second.ypos, _item.second.width, _item.second.height, _item.second.radius, bg_color );
            } else {
                display->fillRect( _item.second.xpos, _item.second.ypos, _item.second.width, _item.second.height, bg_color );
            }
        }
        if( _item.second.outline != 0 ){
            switch (_item.second.outline){
                case 1:
                    display->drawFastVLine( _item.second.xpos, _item.second.ypos, _item.second.height, TFT_BLACK );
                    display->drawFastVLine( _item.second.xpos+_item.second.width-1, _item.second.ypos, _item.second.height, TFT_BLACK );
                break;
                case 2:
                    display->drawFastVLine( _item.second.xpos+_item.second.width-1, _item.second.ypos, _item.second.height, TFT_BLACK );
                break;
                default:
                break;
            }
        }
        display->setTextColor( fg_color ); 
        y_offset = int16_t( ( _item.second.height - display->fontHeight() ) / 2 );
        if( _item.second.font == -1 ){
            y_offset += 4;
        }

        if( _item.second.output_type == OUTPUT_TYPE_STRING ){
            text_width = display->textWidth( _item.second.get_visible() );
            x_pos = _item.second.text_align == 1 ? _item.second.xpos : _item.second.xpos+int16_t((_item.second.width-text_width)/2);
            display->drawString( _item.second.get_visible(), x_pos, _item.second.ypos+y_offset );vTaskDelay(1);
        } else {
            text_width = display->textWidth( _item.second.label.c_str() );
            x_pos = _item.second.text_align == 1 ? _item.second.xpos : _item.second.xpos+int16_t((_item.second.width-text_width)/2);
            display->drawString( _item.second.label.c_str(), x_pos, _item.second.ypos+y_offset );vTaskDelay(1);
        }

    }
    if( use_sprite ){
        canvas.pushSprite(xpos,ypos,TFT_TRANSPARENT);vTaskDelay(1);
        canvas.deleteSprite();vTaskDelay(1);
    }
};
IRAM_ATTR void PhantomButton::redraw_item( int16_t item_index, bool reload ){
    items[item_index].changed = false;
    if( ! is_visible ){ return; }
    if( reload ){ items[item_index].emit_load(); }
    TFT_eSPI *display = &canvas;
    items[item_index].set_font( *display );
    canvas.createSprite( items[item_index].width, items[item_index].height );vTaskDelay(1);
    int bg_color, fg_color;
    set_item_colors( item_index, bg_color, fg_color );
    if( !transparent_background ){
        if( items[item_index].radius > 0 ){
            canvas.fillRoundRect(0,0,items[item_index].width, items[item_index].height, items[item_index].radius, bg_color);vTaskDelay(1); // draw the item background
        } else {
            canvas.fillRect(0,0,items[item_index].width, items[item_index].height, bg_color);vTaskDelay(1); // draw the item background
        }
    } 
    if( items[item_index].outline != 0 ){
        switch (items[item_index].outline){
            case 1:
                canvas.drawFastVLine( 0, 0, items[item_index].height, TFT_BLACK );
                canvas.drawFastVLine( items[item_index].width-1, 0, items[item_index].height, TFT_BLACK );
            break;
            case 2:
                canvas.drawFastVLine( items[item_index].width-1, 0, items[item_index].height, TFT_BLACK );
            break;
            default:
            break;
        }
    }
    if( items[item_index].font == -1 ){ // if item font == -1 we select the icon font
        canvas.setFreeFont(FONT_A);
    } else {
        canvas.setTextFont( items[item_index].font ); 
        canvas.setTextSize( 1 );
    }
    canvas.setTextColor( fg_color ); // draw the label
    int16_t y_offset   = int16_t( ( items[item_index].height - canvas.fontHeight() ) / 2 );
    if( items[item_index].font == -1 ){
        y_offset += 4;
    }

    if( items[item_index].output_type == OUTPUT_TYPE_STRING ){
        int16_t text_width = canvas.textWidth( items[item_index].get_visible() );
        int16_t x_pos = items[item_index].text_align == 1 ? 0 : int16_t((items[item_index].width-text_width)/2);
        canvas.drawString( items[item_index].get_visible(), x_pos, y_offset );vTaskDelay(1);
    } else {
        int16_t text_width = canvas.textWidth( items[item_index].label.c_str() );
        int16_t x_pos = items[item_index].text_align == 1 ? 0 : int16_t((items[item_index].width-text_width)/2);
        canvas.drawString( items[item_index].label.c_str(), x_pos, y_offset );vTaskDelay(1);

    }

    canvas.pushSprite(items[item_index].truex,items[item_index].truey);vTaskDelay(1);
    canvas.deleteSprite();vTaskDelay(1);
};
*/