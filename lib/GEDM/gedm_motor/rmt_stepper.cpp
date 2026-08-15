#include "rmt_stepper.h"


rmt_item32_t RMT_STEPPER::rmt_item[2];
rmt_config_t RMT_STEPPER::rmt_conf;

//####################################################
// Basic setup
//####################################################
void RMT_STEPPER::setup( GAxis* g_axis ){
    step_pin          = g_axis->step_pin.get();
    dir_pin           = g_axis->dir_pin.get();
    axis_index        = g_axis->axis_index.get();
    current_direction = 2; // initial direction set to invalid 2
    invert_step       = false;
    invert_dir        = false;
    //enabled           = true;

    if( dir_pin == PIN_NONE || step_pin == PIN_NONE || dir_pin == -1 || step_pin == -1 ){
        // problem
        enabled = false; // ignore motor but still list it
        return;
    } else {
        enabled = true;
    }

    rmt_channel = rmt_next();
}

//####################################################
// Setup RMT and Pins
//####################################################
void RMT_STEPPER::init(){

    if( !enabled ){
        // problem
        enabled = false; // ignore motor but still list it
        return;
    }

    pinMode( dir_pin, OUTPUT );

    rmt_conf.rmt_mode                       = RMT_MODE_TX;
    rmt_conf.clk_div                        = 20;
    rmt_conf.mem_block_num                  = 2;
    rmt_conf.tx_config.loop_en              = false;
    rmt_conf.tx_config.carrier_en           = false;
    rmt_conf.tx_config.carrier_freq_hz      = 0;
    rmt_conf.tx_config.carrier_duty_percent = 50;
    rmt_conf.tx_config.carrier_level        = RMT_CARRIER_LEVEL_LOW;
    rmt_conf.tx_config.idle_output_en       = true;
    auto stepPulseDelay                     = STEP_PULSE_DELAY;
    rmt_item[0].duration0                   = stepPulseDelay < 1 ? 1 : stepPulseDelay * 4;
    rmt_item[0].duration1                   = 4 * DEFAULT_STEP_PULSE_MICROSECONDS;
    rmt_item[1].duration0                   = 0;
    rmt_item[1].duration1                   = 0;
    if ( rmt_channel == RMT_CHANNEL_MAX ) {
        return;
    }
    rmt_set_source_clk( rmt_channel, RMT_BASECLK_APB );
    rmt_conf.channel              = rmt_channel;
    rmt_conf.tx_config.idle_level = invert_step ? RMT_IDLE_LEVEL_HIGH : RMT_IDLE_LEVEL_LOW;
    rmt_conf.gpio_num             = gpio_num_t(step_pin);
    rmt_item[0].level0              = rmt_conf.tx_config.idle_level;
    rmt_item[0].level1              = !rmt_conf.tx_config.idle_level;
    rmt_config(&rmt_conf);
    rmt_fill_tx_items(rmt_conf.channel, &rmt_item[0], rmt_conf.mem_block_num, 0);

};

//####################################################
// Setup RMT
//####################################################
void IRAM_ATTR RMT_STEPPER::step(){
    if( !enabled ){ 
        //debuglog("*Motor is disabled!");
        //debuglog("*Invalid Pin definition");
        return; 
    }
    RMT.conf_ch[rmt_channel].conf1.mem_rd_rst = 1;
    RMT.conf_ch[rmt_channel].conf1.tx_start   = 1;
};

//####################################################
// Set motor direction and add delay after a change
// if wanted
//####################################################
bool IRAM_ATTR RMT_STEPPER::set_direction( bool dir, bool add_dir_delay ){
    if( !enabled ){ return false; }
    if( dir == current_direction ){ return false; } 
    current_direction = dir;
    digitalWrite(dir_pin, dir ^ invert_dir); 
    if( ! add_dir_delay ){ return true; }
    delayMicroseconds(STEPPER_DIR_CHANGE_DELAY);
    return true;
}; 

//####################################################
// Get the step delay for a given speed in mm/min
// Very rough stuff
//####################################################
int  RMT_STEPPER::get_step_delay_for_speed( float mm_min ){
    // mm per single step
    double mm_per_step = 1.0 / ( double ) g_axis[axis_index]->steps_per_mm.get();
    double delay = mm_per_step / ( double ) mm_min * 60000000;
    return round( delay );
};

//####################################################
// Get enxt RMT channel
//####################################################
rmt_channel_t RMT_STEPPER::rmt_next() {
    static uint8_t next_RMT_chan_num = uint8_t(RMT_CHANNEL_0);  // channels 0-7 are valid
    if( next_RMT_chan_num < RMT_CHANNEL_MAX ){
        next_RMT_chan_num++;
    }
    return rmt_channel_t(next_RMT_chan_num);
}
