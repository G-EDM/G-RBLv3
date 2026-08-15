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

#include "widget_alarm.h"
#include "definitions.h"

void PhantomAlarm::set_error_code( uint8_t __error_code ){
    error_code = __error_code;
};
void PhantomAlarm::create( void ){};
IRAM_ATTR void PhantomAlarm::redraw_item( int16_t item_index, bool reload ){};

void PhantomAlarm::show( void ){
    int16_t text_width = 0;
    uint16_t margin_left = 20;
    tft.fillScreen( TFT_BLACK );
    tft.setTextSize(2);
    tft.setTextColor(TFT_RED);
    tft.setTextFont(2);

    tft.drawRect( 0, 0, tft.width(), tft.height(), TFT_MAROON );

    tft.setTextColor(TFT_MAROON);
    tft.drawString("Alarm!", 20, 20);vTaskDelay(1);

    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY);

    text_width = tft.textWidth( alarm_labels[0] );
    tft.drawString( alarm_labels[0], margin_left, 100 ); vTaskDelay(1);
    tft.drawString( String( error_code ), margin_left+10+text_width, 100 ); vTaskDelay(1);

    text_width = tft.textWidth( alarm_labels[1] );
    tft.drawString( alarm_labels[1], margin_left, 120 ); vTaskDelay(1);
    tft.drawString( get_alarm_msg( error_code ), margin_left+10+text_width, 120 ); 

    tft.drawString(alarm_labels[2], margin_left, 160);vTaskDelay(1);

    tft.setTextSize(2);

    uint16_t x, y;
    //while( tft.getTouch(&x, &y, 500) ){ // enter blocking until touched
    bool flip = 1;
    uint64_t time_us = esp_timer_get_time();
    while( !tft.getTouch(&x, &y, 500) ){ // enter blocking until touched
        if( esp_timer_get_time() - time_us > 1000000 ){
            tft.setTextColor( flip ? TFT_YELLOW : TFT_MAROON );
            tft.drawString("Alarm!", 20, 20);vTaskDelay(1);
            flip = !flip;
            time_us = esp_timer_get_time();
        }
        vTaskDelay(100);
    }
    tft.setTextSize(1);

};
