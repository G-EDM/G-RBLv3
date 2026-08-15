#pragma once

#include "stepper.h"
#include <driver/rmt.h>


class RMT_STEPPER : public G_STEPPER {

    public:
        RMT_STEPPER() : G_STEPPER() {};
        void           init( void );
        void           setup( GAxis* g_axis );
        void IRAM_ATTR step( void );
        bool IRAM_ATTR set_direction( bool dir, bool add_dir_delay = true ); 
        int            get_step_delay_for_speed( float mm_min );

    protected:
        rmt_channel_t rmt_channel;

    private:
        static rmt_channel_t rmt_next();
        static rmt_item32_t  rmt_item[2];
        static rmt_config_t  rmt_conf;

};


