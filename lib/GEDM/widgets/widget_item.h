#pragma once

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

#ifndef PHANTOM_UI_WIDGET_ITEM
#define PHANTOM_UI_WIDGET_ITEM

#include "language/en_us.h"
#include "event_manager.h"
#include "ili9341_config.h"

typedef struct ItemIdentPackage {
    uint8_t page_id   = 0;
    uint8_t tab_id    = 0;
    uint8_t widget_id = 0;
    uint8_t item_id   = 0;
} ItemIdentPackage;

typedef struct PhantomSettingsPackage {
    int   intValue   = 0;
    float floatValue = 0.0;
    bool  boolValue  = false;
    int   param_id   = 0;
    int   axis       = 0;
    int   type       = 0;
} PhantomSettingsPackage;

struct PhantomWidgetItemColors {
    int default_bg  = BUTTON_LIST_BG;
    int default_fg  = BUTTON_LIST_FG;
    int touched_bg  = COLOR555555;
    int touched_fg  = BUTTON_LIST_FG;
    int active_bg   = COLOR555555;
    int active_fg   = BUTTON_LIST_FG;
    int disabled_bg = BUTTON_LIST_BG_DEACTIVATED;
    int disabled_fg = BUTTON_LIST_FG_DEACTIVATED;
};


class PhantomWidgetItem : public EventManager {
    private:
        uint8_t parent_page_id;

    public:
        PhantomWidgetItem();
        virtual ~PhantomWidgetItem();
        PhantomWidgetItemColors colors;

        PhantomWidgetItem* set_colors( int state, int bg, int fg );
        PhantomWidgetItem* set_int_value( int value );
        PhantomWidgetItem* set_string_value( std::string str );
        PhantomWidgetItem* set_float_value( float value );
        PhantomWidgetItem* set_outlines( uint value );
        PhantomWidgetItem* set_require_dpm( void );
        PhantomWidgetItem* set_require_motion( void );
        PhantomWidgetItem* set_require_sd( void );
        PhantomWidgetItem* set_is_refreshable( void );
        PhantomWidgetItem* set_suffix( uint8_t __suffix );
        PhantomWidgetItem* set_radius( uint8_t value );
        PhantomWidgetItem* set_float_decimals( int8_t value );
        PhantomWidgetItem* set_disabled( bool disable );
        PhantomWidgetItem* set_text_align( int align );
        
        void emit_touch( void );
        void emit_load( void );
        
        void set_font( TFT_eSPI &display );
        void set_parent_page_id( uint8_t page_id );
        void set_parent_page_tab_id( uint8_t tab_id );
        void set_widget_id( uint8_t widget_id );
        
        int16_t get_index( void );
        const char* get_visible( void );

        uint8_t get_parent_page_id( void );
        uint8_t get_parent_page_tab_id( void );
        uint8_t get_widget_id( void );

        uint8_t page_index;
        uint8_t page_tab_index;
        uint8_t widget_index;

        std::string label       = "";
        std::string strValue    = "";
        int     intValue        = 0;
        float   floatValue      = 0.0;
        bool    boolValue       = false;
        int8_t  valueType       = 0; // 1 = int, 2 = float, 3 = bool, 4 = string
        bool    touch_enabled   = false;
        bool    disabled        = false;
        bool    is_active       = false;
        bool    has_touch       = false;
        bool    changed         = false;
        int8_t  param_id        = 0;
        int16_t truex           = 0;
        int16_t truey           = 0;
        int16_t xpos            = 0;
        int16_t ypos            = 0;
        int16_t width           = 0;
        int16_t height          = 0;
        int16_t item_index      = 0;
        int8_t  item_outline    = 0;
        int8_t  font            = 2;
        int8_t  float_decimals  = 2;
        uint8_t  output_type     = 0;
        uint8_t radius          = 0;
        uint8_t outline         = 0;
        uint8_t suffix          = 0;
        bool    sd_required     = false;
        bool    motion_required = false;
        bool    dpm_required    = false;
        bool    refreshable     = false;
        int     text_align      = 0; //0=default
        int16_t label_text_width = -1;

};


ItemIdentPackage create_package( PhantomWidgetItem * item );


#endif