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

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "gpo_scope.h"
#include "ili9341_config.h"
#include "definitions.h"
#include "widgets/char_helpers.h"

static DRAM_ATTR TFT_eSprite canvas_b = TFT_eSprite(&tft);

GPO_SCOPE gscope;

struct gpo_scope_data {
    int      misc_data             = 0;
    uint8_t  motion_signal         = 0;
    uint8_t  dstate                = 1;
    uint8_t  right_bar_width       = 5;
    uint8_t  vfd_state_old         = 10;
    uint8_t  cfd_state_old         = 10;
    uint32_t vrop_tresh            = 1500;
    uint32_t vfd_tresh             = 4000;
    uint32_t favg_size             = 0;
    uint32_t setpoint_min          = 0;
    uint32_t setpoint_max          = 0;
    uint32_t voltage_feedback      = 0;
    uint32_t scope_current_cursor  = 0;
    uint32_t scope_width           = 0;
    uint32_t scope_height          = 0;
    uint32_t scope_height_total    = 0;
    uint32_t scope_width_total     = 0;
    uint32_t scope_width_minus_one = 0;
    uint32_t scope_pos_x           = 0;
    uint32_t scope_pos_y           = 0;
    uint32_t scope_pos_y_offset    = 0;
    uint32_t sample_rate           = 0;
    uint32_t total_avg             = 0;
    uint32_t fast_avg              = 0;
    uint32_t vfd_indicator_x       = 0;
    uint32_t vfd_indicator_y       = 0;
    uint32_t cfd_indicator_x       = 0;
    uint32_t cfd_indicator_y       = 0;
    uint32_t h_res_boost           = 0;
    uint32_t hv_res_boost          = 0;
};

typedef struct draw_cache_data {
    int32_t  start_x        = 0;
    int32_t  start_y        = 0;
    int32_t  end_y          = 0;
    uint32_t  pixel_vertical = 0;
    int32_t  current_x      = 0;
} draw_cache_data;

static const           int      scope_buffer_size = 320; // width of the display
static const           uint16_t boost_scaler      = 1024;
static DRAM_ATTR const uint16_t plan_colors[] = { TFT_WHITE, TFT_WHITE, TFT_GREEN, TFT_GREEN, TFT_YELLOW, COLORORANGERED, COLORORANGERED, COLORORANGERED };
static DRAM_ATTR const uint32_t sig_colors[]  = { TFT_GREEN, TFT_GREEN, TFT_WHITE, TFT_WHITE, TFT_YELLOW, TFT_RED, TFT_RED, TFT_RED };

DRAM_ATTR std::atomic<bool> scope_is_running_flag(false);
DRAM_ATTR std::atomic<bool> scope_batch_ready( false );

static DRAM_ATTR int8_t          scope_values_plan[scope_buffer_size] = {0,}; // 0.3125 kilobytes.
static DRAM_ATTR uint32_t        scope_values[scope_buffer_size]      = {0,};
static DRAM_ATTR gpo_scope_data  gpo_data;
static DRAM_ATTR draw_cache_data draw_cache;



GPO_SCOPE::GPO_SCOPE(){}



IRAM_ATTR bool GPO_SCOPE::is_blocked(){
    return scope_batch_ready.load( std::memory_order_acquire );
}


//#######################################################################
// Set ADC value and motion_plan used
//#######################################################################
IRAM_ATTR bool GPO_SCOPE::add_to_scope( uint32_t adc_value, int8_t plan ){ // lock set in sensor
    if( scope_batch_ready.load( std::memory_order_acquire ) ){ return false; }
    scope_values[gpo_data.scope_current_cursor]      = adc_value & 0xFFF;
    scope_values_plan[gpo_data.scope_current_cursor] = plan;
    if( ++gpo_data.scope_current_cursor >= gpo_data.scope_width ){
        gpo_data.scope_current_cursor = 0;
        scope_batch_ready.store( true, std::memory_order_release );
    }
    return ( gpo_data.scope_current_cursor == gpo_data.scope_width_minus_one );
}

//#######################################################################
// Those are only really needed once a batch is full and ready
// for drawing
//#######################################################################
IRAM_ATTR void GPO_SCOPE::set_meta_data( // lock set in sensor
    uint32_t sample_rate, 
    uint8_t  motion_plan, 
    uint32_t vfd_feedback, // slow average
    uint32_t cfd_fast_avg,
    uint32_t cfd_slow_avg,
    int misc_data
){

    gpo_data.sample_rate      = sample_rate;
    gpo_data.motion_signal    = motion_plan;
    gpo_data.voltage_feedback = vfd_feedback & 0xFFF;
    gpo_data.fast_avg         = cfd_fast_avg & 0xFFF;
    gpo_data.total_avg        = cfd_slow_avg & 0xFFF;
    gpo_data.misc_data        = misc_data;

    gpo_data.dstate = vfd_feedback < gpo_data.vrop_tresh ? 0 : 1;

    //const int aaa = ( 4128 & 0xFFF );

}








IRAM_ATTR void GPO_SCOPE::draw_wave(){

    draw_cache.start_x        = 0;
    draw_cache.start_y        = gpo_data.scope_height;
    draw_cache.pixel_vertical = 0;

    for( int i = 0; i < gpo_data.scope_width_minus_one; ++i ){

        //const int iii = ( ( 75-20 ) * 1024 / VSENSE_RESOLUTION ); // 55 available height
        //const int hhh = ( ( iii * VSENSE_RESOLUTION ) >> 10 );

        // Calculate the pixel relative the the scope canvas
        draw_cache.pixel_vertical = scope_values[i] > 0
                                    ? ( ( gpo_data.h_res_boost * scope_values[i] ) >> 10 ) 
                                    : 0;

        // Draw the line and update the last y position
        draw_cache.end_y =  draw_cache.pixel_vertical < gpo_data.scope_height 
                            ? gpo_data.scope_height - draw_cache.pixel_vertical
                            : 0;

        canvas_b.drawLine( // y counts from top: full height = bottom
            draw_cache.start_x, 
            draw_cache.start_y, 
            draw_cache.start_x+=1, 
            draw_cache.end_y, 
            plan_colors[ scope_values_plan[i] ] 
        );
        draw_cache.start_y = draw_cache.end_y;
    }

    //###################################################
    // Calculate the pixel relative the the scope canvas
    //###################################################
    // slow average cFd
    draw_cache.start_x = gpo_data.scope_width - gpo_data.favg_size;
    draw_cache.start_y = gpo_data.scope_height - ( ( gpo_data.hv_res_boost * gpo_data.total_avg ) >> 10 );
    canvas_b.drawFastHLine( draw_cache.start_x, draw_cache.start_y+1, gpo_data.favg_size, ( gpo_data.total_avg > gpo_data.setpoint_max ? TFT_RED : TFT_WHITE )  );
    // fast average cfd
    draw_cache.start_x = gpo_data.scope_width - 10;
    draw_cache.start_y = gpo_data.scope_height - ( ( gpo_data.hv_res_boost * gpo_data.fast_avg ) >> 10 );
    canvas_b.drawFastHLine( draw_cache.start_x, draw_cache.start_y+1, 10, ( gpo_data.fast_avg > gpo_data.setpoint_max ? TFT_RED : TFT_CYAN )  );

    // vFd
    canvas_b.drawFastHLine( 0, gpo_data.scope_height-( ( gpo_data.hv_res_boost * gpo_data.voltage_feedback ) >> 10 ), 15, ( gpo_data.voltage_feedback < gpo_data.vrop_tresh ? TFT_RED : TFT_DARKGREY )  );

    canvas_b.drawNumber( gpo_data.voltage_feedback, 5,   2, 2 );
    canvas_b.drawNumber( gpo_data.total_avg,        48,  2, 2 );
    canvas_b.drawNumber( gpo_data.sample_rate,      91,  2, 2 );
    canvas_b.drawNumber( gpo_data.misc_data,        134, 2, 2 );

    /*if( gpo_data.dstate != gpo_data.vfd_state_old ){
        gpo_data.vfd_state_old = gpo_data.dstate;
        tft.fillCircle( gpo_data.vfd_indicator_x, gpo_data.vfd_indicator_y, 2, ( gpo_data.vfd_state_old==1 ? TFT_GREEN : TFT_RED ) ); 
    }*/

    if( gpo_data.motion_signal != gpo_data.cfd_state_old ){
        gpo_data.cfd_state_old = gpo_data.motion_signal;
        tft.fillCircle( gpo_data.cfd_indicator_x, gpo_data.cfd_indicator_y, 2, sig_colors[ gpo_data.motion_signal ] ); 
    }

    canvas_b.pushSprite( gpo_data.scope_pos_x, gpo_data.scope_pos_y_offset );vTaskDelay(1);

}




void GPO_SCOPE::flush_frame(){
    if( !scope_is_running() ) return;
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
        reset();
        canvas_b.fillSprite( TFT_BLACK );
        canvas_b.pushSprite( gpo_data.scope_pos_x, gpo_data.scope_pos_y_offset );vTaskDelay(1);
        reset();
    release_lock_for( ATOMIC_LOCK_GSCOPE );
}







IRAM_ATTR void GPO_SCOPE::draw_scope(){
    if( 
        !scope_is_running_flag.load( std::memory_order_acquire ) || 
        !scope_batch_ready.load( std::memory_order_acquire ) 
    ){ return; }
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
        draw_wave();
        reset(); 
    release_lock_for( ATOMIC_LOCK_GSCOPE );
}

void GPO_SCOPE::update_config( 
    int savg_size, 
    int vdrop_thresh,
    int v_thresh,
    float duty_percent,
    float frequency,
    uint16_t smin, 
    uint16_t smax
){
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
    gpo_data.favg_size    = savg_size;
    gpo_data.vrop_tresh   = vdrop_thresh;
    gpo_data.vfd_tresh    = v_thresh; 
    pwm_duty              = duty_percent;
    pwm_frequency         = frequency;
    gpo_data.setpoint_min = smin;
    gpo_data.setpoint_max = smax;
    release_lock_for( ATOMIC_LOCK_GSCOPE );
}


IRAM_ATTR bool GPO_SCOPE::scope_is_running(){
    return scope_is_running_flag.load( std::memory_order_acquire );
}
void GPO_SCOPE::start(){
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
        gpo_data.vfd_state_old   = 10;
        gpo_data.cfd_state_old   = 10;
        gpo_data.vfd_indicator_x = gpo_data.scope_pos_x+36;
        gpo_data.vfd_indicator_y = gpo_data.scope_pos_y+10;
        gpo_data.cfd_indicator_x = gpo_data.scope_pos_x+79;
        gpo_data.cfd_indicator_y = gpo_data.scope_pos_y+10;
        reset();
        scope_is_running_flag.store( true, std::memory_order_release );
    release_lock_for( ATOMIC_LOCK_GSCOPE );
}
IRAM_ATTR void GPO_SCOPE::reset(){ // lock set outside except ili class
    canvas_b.deleteSprite();
    canvas_b.createSprite( gpo_data.scope_width, gpo_data.scope_height );
    vTaskDelay(1);
    gpo_data.scope_current_cursor = 0;
    scope_batch_ready.store( false, std::memory_order_release );
}


void GPO_SCOPE::stop(){
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
        scope_is_running_flag.store( false, std::memory_order_release );
        scope_batch_ready.store( true, std::memory_order_release ); // prevent the sensor class from writing
        //canvas_b.deleteSprite();
    release_lock_for( ATOMIC_LOCK_GSCOPE );    
}
void GPO_SCOPE::init( int16_t width, int16_t height, int16_t posx, int16_t posy ){
    stop();
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
        gpo_data.scope_width           = width-gpo_data.right_bar_width;
        gpo_data.scope_width_total     = width;
        gpo_data.scope_height_total    = height;
        gpo_data.scope_height          = height-20;
        gpo_data.scope_pos_x           = posx;
        gpo_data.scope_pos_y           = posy;
        gpo_data.scope_pos_y_offset    = posy+20;
        gpo_data.scope_width_minus_one = gpo_data.scope_width-1;
        gpo_data.h_res_boost           = gpo_data.scope_height * boost_scaler / VSENSE_RESOLUTION;
        gpo_data.hv_res_boost          = gpo_data.scope_height * boost_scaler / VSENSE_RESOLUTION; // this was a little different in the past. To not mess all the code up i keep this for now

        reset();
        canvas_b.setTextColor( TFT_WHITE );
        gscope.draw_scope_frame();
        tft.drawFastHLine( gpo_data.scope_pos_x, gpo_data.scope_pos_y+height, gpo_data.scope_width, BUTTON_LIST_BG );
    release_lock_for( ATOMIC_LOCK_GSCOPE );
}

std::string get_pwm_text( float freq, float duty ){
    std::string text = "";
    text += float_to_string( freq, 1 );
    text += "kHz @ ";
    text += float_to_string( duty, 2 );
    text += "%";
    return text;
}


// Creates the outer scope frame 
void GPO_SCOPE::draw_scope_frame(){

    int         pos_x     = 5;
    int32_t     max       = ( gpo_data.scope_height - ( gpo_data.h_res_boost * gpo_data.setpoint_max ) / boost_scaler );
    int32_t     min       = ( gpo_data.scope_height - ( gpo_data.h_res_boost * gpo_data.setpoint_min ) / boost_scaler );
    std::string pwm_data  = get_pwm_text( pwm_frequency, pwm_duty );

    canvas_b.deleteSprite();
    canvas_b.createSprite( gpo_data.scope_width_total, gpo_data.scope_height_total );vTaskDelay(1);
    canvas_b.fillSprite( TFT_BLACK );
    canvas_b.pushSprite( gpo_data.scope_pos_x, gpo_data.scope_pos_y );vTaskDelay(1);
    canvas_b.deleteSprite();

    canvas_b.createSprite( gpo_data.scope_width_total, 19 );vTaskDelay(1);
    canvas_b.fillSprite( BUTTON_LIST_BG );vTaskDelay(1);
    canvas_b.setTextColor( TFT_LIGHTGREY );
    canvas_b.drawString( "vFd",  pos_x,     2,  2 );
    canvas_b.drawString( "cFd",  pos_x+=43, 2, 2 );
    canvas_b.drawString( "kSps", pos_x+=43, 2, 2 );

    canvas_b.setTextColor( TFT_DARKGREY );
    canvas_b.setTextFont( DEFAULT_FONT );
    pos_x = ( gpo_data.scope_width_total - canvas_b.textWidth( pwm_data.c_str() ) - gpo_data.right_bar_width - 5 );
    canvas_b.drawString( pwm_data.c_str(), pos_x, 2, 2 );
    canvas_b.pushSprite( gpo_data.scope_pos_x, gpo_data.scope_pos_y );vTaskDelay(1);
    canvas_b.deleteSprite();
    canvas_b.setTextColor( TFT_WHITE );

    canvas_b.deleteSprite();
    canvas_b.createSprite( gpo_data.right_bar_width, gpo_data.scope_height );vTaskDelay(1);
    canvas_b.fillSprite( TFT_DARKGREY );vTaskDelay(1);
    // draw setpoint range
    canvas_b.fillRect( 0,  max, gpo_data.right_bar_width, min-max, TFT_YELLOW );vTaskDelay(1);
    canvas_b.pushSprite( gpo_data.scope_pos_x+gpo_data.scope_width, gpo_data.scope_pos_y_offset );vTaskDelay(1);

    reset();
}
