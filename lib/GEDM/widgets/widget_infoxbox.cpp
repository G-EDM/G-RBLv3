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

#include "widget_infobox.h"


void PhantomInfo::create( void ){};
IRAM_ATTR void PhantomInfo::redraw_item( int16_t item_index, bool reload ){};

void PhantomInfo::show( void ){
    int16_t text_width = 0;
    tft.fillScreen( TFT_BLACK );
    uint16_t x, y;
    //while( tft.getTouch(&x, &y, 500) ){ // enter blocking until touched
    while( !tft.getTouch(&x, &y, 500) ){ // enter blocking until touched
        vTaskDelay(100);
    }
};
