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

#ifndef PHANTOM_UI_WIDGET_BASE
#define PHANTOM_UI_WIDGET_BASE

#include <map>
#include "widget_item.h"

#include <atomic>


extern DRAM_ATTR volatile bool break_refresh;
//extern std::atomic<bool> canvas_locked;



extern portMUX_TYPE msg_logger_mutex;

class PhantomWidget : 
    public EventManager, 
    public std::enable_shared_from_this<PhantomWidget> {

    public:

        static TFT_eSPI* get_sprite( uint16_t width, uint16_t height, uint16_t color = 0 );
        static void push( uint16_t xpos, uint16_t ypos, uint16_t color = 0 );

        virtual ~PhantomWidget();

        PhantomWidget( int16_t _width = 0, int16_t _height = 0, int16_t _xpos = 0, int16_t _ypos = 0, int8_t type = 0, bool _use_sprite = false ) 
        : width(_width),height(_height),xpos(_xpos),ypos(_ypos),num_items(0),widget_type(type),use_sprite(_use_sprite){};

        int16_t width;
        int16_t height;
        int16_t xpos;
        int16_t ypos;
        int16_t num_items;
        int8_t  widget_type;
        bool    use_sprite;
        int     intValue;
        float   floatValue;
        int     parameter_id;
        uint8_t index;
        uint8_t page_id                = 0;
        uint8_t tab_id                 = 0;
        int     float_decimals         = 3;
        bool    transparent_background = false;
        bool    is_visible             = true;
        bool    is_refreshable         = false;
        bool    changed                = false;
        bool    refreshing             = false;
        bool    no_emits               = false;

        std::shared_ptr<PhantomWidget> set_is_refreshable( void );
        std::shared_ptr<PhantomWidget> disabled_emits_global( void );

        virtual bool touch_handler( int16_t item_index ){ return false; }

        virtual std::shared_ptr<PhantomWidget> reset_widget( PhantomSettingsPackage *item, uint8_t __type ){
            return shared_from_this();
        };

        virtual void reset_item( void ){};
        virtual void create( void )                                       = 0;
        virtual void show( void )                                         = 0;
        IRAM_ATTR virtual void redraw_item( int16_t item_index, bool reload = true ) = 0;
        IRAM_ATTR virtual bool show_on_change( void );

        void update_page_ids_for_items( void );

        PhantomWidgetItem * addItem(
            uint8_t     item_type, // output type string, int, float etc..
            const char* label,
            int8_t      param_type, // valueType: 1=int, 2= float, 3=bool
            int8_t      param_id, 
            int16_t     width,
            int16_t     height,
            int16_t     _posx,
            int16_t     _posy, 
            bool       touch_enabled, // no touch event if touch_enabled is false
            int8_t     font           // if font is -1 it will load the icon font
        );

        void close( void );
        void set_item_colors( int16_t item_index, int &bg, int &fg );
        void disable_background_color( void );
        void set_suffix( int16_t item_index, uint8_t _suffix );
        void set_font( int16_t item_index, int _font );
        void set_colors_active( int16_t item_index, int bg, int fg );
        void set_colors_disabled( int16_t item_index, int bg, int fg );
        void set_use_sprite( bool _use_sprite );
        void set_float_decimals( int _decimals );
        int16_t get_item_index( void );
        // map contains the widget items; no pointers
        std::map<std::int16_t, PhantomWidgetItem> items; 
};


#endif