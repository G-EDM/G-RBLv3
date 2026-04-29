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

#include "ui_interface.h"
#include "phantom_page.h"
#include "widget_button.h"


// Fill the screen and iterates over the widgets and shows them
bool PhantomPage::show(){
    if( tabs.find(active_tab) == tabs.end() ){
        debuglog("*Active tab not found", DEBUG_LOG_DELAY );
        return false;
    }

    if( page_id == PAGE_FRONT ){
        ui.logger->width  = 320-20;
        ui.logger->height = 75-19;
        ui.logger->ypos   = 50;
        ui.logger->xpos   = 10;
        ui.logger->is_visible = true;
    } else if ( 
        page_id == PAGE_SD_CARD //|| page_id == PAGE_FILE_MANAGER_GCODES || page_id == PAGE_FILE_MANAGER 
    ){
        ui.logger->width  = 130;
        ui.logger->height = 5*16;
        ui.logger->ypos   = 110;
        ui.logger->xpos   = 180;
        ui.logger->is_visible = true;
    } else if( page_id == PAGE_IN_PROCESS ){
        ui.logger->width  = 220;
        ui.logger->height = 90;
        ui.logger->ypos   = 105;
        ui.logger->xpos   = 90;
        ui.logger->is_visible = true;
    } else if( page_id == PAGE_INFO ){
        ui.logger->width  = 215;
        ui.logger->height = 75;
        ui.logger->ypos   = 100;
        ui.logger->xpos   = 15;
        ui.logger->is_visible = true;
    } else if( page_id == PAGE_PROBING ){
        ui.logger->width  = 180;
        ui.logger->height = 140;
        ui.logger->ypos   = 15;
        ui.logger->xpos   = 15;
        ui.logger->is_visible = true;
    } else if( page_id == PAGE_MOTION ){ // only for homing tab logger will get enabled
        ui.logger->width  = 100;
        ui.logger->height = 120;
        ui.logger->ypos   = 40;
        ui.logger->xpos   = 5;
        ui.logger->is_visible = active_tab == 1 ? true : false;
    }
    is_active = true;
    fill_screen( bg_color );
    auto tab = tabs.find( active_tab );
    if( tab==tabs.end()){
        debuglog("*Pagetab not found!");
        return false;
    }
    for( auto& widget : tab->second->widgets ){
        widget.second->is_visible = true;
        widget.second->show();
    }
    
    logs_available.store( page_id == PAGE_FRONT ? false : true ); // front page will build the scope with a flush that will also set this flag; frontpage has the console layering on the scope
    //show_logger();
    return true;
}
// iterates over the widgets and refreshs them on change
IRAM_ATTR void PhantomPage::refresh(){
    if( is_refreshing || tabs.find(active_tab) == tabs.end() ){ 
        if( is_refreshing ){
            break_refresh = true;
            while( is_refreshing ){ vTaskDelay(1); }
        } else {
            return;
        }        
    }
    break_refresh = false;
    is_refreshing = true;
    for( auto& widget : tabs[active_tab]->widgets ){
        if( widget.second->is_refreshable ){
            if( !widget.second->show_on_change() ){ // return false if enforced to stop by enforce_redraw
                break; 
            }
        }
        vTaskDelay(1);
    }
    is_refreshing = false;
}



PhantomPage::~PhantomPage(){
    for( auto& tab : tabs ){
        tabs.erase( tab.first );
    }
    tabs.clear();
    for( auto& widget : tab_items ){
        tab_items.erase( widget.first );
    }
    tab_items.clear();
};



PhantomPage::PhantomPage( uint8_t id, uint8_t total_tabs ){
    is_temporary = false;
    tab_item_ids = 0;
    bg_color     = TFT_BLACK;
    start( id, total_tabs );
};


void PhantomPage::start( uint8_t id, uint8_t total_tabs = 1 ){
    page_on_close      = PAGE_FRONT;
    page_on_close_lock = false;
    page_id            = id;
    num_tabs           = total_tabs;
    active_tab         = 0;
    for( int i = 0; i < num_tabs; ++i ){
        tabs[i] = std::unique_ptr<PhantomTabContainer>( new PhantomTabContainer() );
    }
};

bool PhantomPage::is_temporary_page( void ){
    return is_temporary;
}
void PhantomPage::close( bool final ){
    is_active = false;
    if( is_temporary_page() && final ){
    } else {
    }
    for( auto& widget : tabs[active_tab]->widgets ) {
        widget.second->is_visible = false;
        widget.second->close();
    }
    fill_screen( TFT_BLACK );
}
// select tab by index; 0=first tab, 1=second tab ...
// if out of range it will not throw an error but ignore the selection
void PhantomPage::select_tab( uint8_t index, bool update_tab ){
    if( index == num_tabs-1 ){
        //return;
    }
    active_tab = index;
    if( update_tab ) {
        ui.update_active_tabs(page_id, active_tab);
    }
};
uint8_t PhantomPage::get_active_tab(){
    return active_tab;
};
bool PhantomPage::active_tab_is_last(){
    if( active_tab == num_tabs-1 ){
        return true;
    } return false;
}
bool PhantomPage::active_tab_is_first(){
    if( active_tab == 0 ){
        return true;
    } return false;
}
// return the number of tabs; not the index of the tab,
uint8_t PhantomPage::get_total_tabs(){
    return num_tabs;
}
uint8_t PhantomPage::get_page_id(){
    return page_id;
}
uint8_t PhantomPage::add_widget( std::shared_ptr<PhantomWidget> widget ){
    uint8_t widget_index = tabs[active_tab]->num_widgets;
    widget->page_id = page_id;
    widget->tab_id  = active_tab;
    widget->index   = widget_index;
    widget->update_page_ids_for_items(); // add page_id to all items in the widget
    tabs[active_tab]->widgets[widget_index] = widget;
    ++tabs[active_tab]->num_widgets;
    return widget_index;
}




const std::function<void(PhantomWidgetItem *item)> phantom_set_tab_state_cb = [](PhantomWidgetItem *item) {
    auto page = ui.get_page( item->get_parent_page_id() );
    if( page == nullptr ){ return; }
    uint8_t active_tab = page->get_active_tab();
    uint8_t target_tab = item->intValue;
    if( target_tab == active_tab ){
        item->touch_enabled = false;
        item->is_active     = true;
    } else {
        item->touch_enabled = true;
        item->is_active     = false;
    }
};

const std::function<void(PhantomWidgetItem *item)> phantom_to_tab_cb = [](PhantomWidgetItem *item) {
    auto page = ui.get_page( item->get_parent_page_id() );
    if( page == nullptr ){ return; }
    uint8_t target_tab = item->intValue;
    page->close( false );
    page->select_tab( target_tab, true );
    page->show();
};

const std::function<void(PhantomWidgetItem *item)> phantom_previous_cb = [](PhantomWidgetItem *item) {
    auto page = ui.get_page( item->get_parent_page_id() );
    if( page == nullptr ){ return; }
    uint8_t active_tab = page->get_active_tab();
    uint8_t target_tab = 0;
    if( page->active_tab_is_first() ){ // start tab, go to last
        target_tab = page->get_total_tabs()-1; // array index starts with 0, tab 1 = index 0..
    } else {
        target_tab = active_tab-1;
    }
    page->close( false );
    page->select_tab( target_tab, true );
    page->show();
};
const std::function<void(PhantomWidgetItem *item)> phantom_next_cb = [](PhantomWidgetItem *item) {
    auto page = ui.get_page( item->get_parent_page_id() );
    if( page == nullptr ){ return; }
    uint8_t active_tab = page->get_active_tab();
    uint8_t target_tab = 0;
    if( page->active_tab_is_last() ){ // start tab, go to last
        target_tab = 0; // array index starts with 0, tab 1 = index 0..
    } else {
        target_tab = active_tab+1;
    }
    page->close( false );
    page->select_tab( target_tab, true );
    page->show();
};
const std::function<void(PhantomWidgetItem *item)> phantom_close_cb = [](PhantomWidgetItem *item) {
    // return to previous page
    ui.show_page( ui.get_previous_active_page() );
};
const std::function<void(PhantomWidgetItem *item)> phantom_set_state_cb = [](PhantomWidgetItem *item) {
    auto page = ui.get_page( item->get_parent_page_id() );
    if( page == nullptr ){ return; }
    item->is_active = false;
    if( item->param_id == PARAM_ID_PAGE_NAV_LEFT ){
        // previous nav button
        if( page->active_tab_is_first() ){
            item->is_active = true;
        }
    } else if( item->param_id == PARAM_ID_PAGE_NAV_RIGHT ){
        // next nav button
        if( page->active_tab_is_last() ){
            item->is_active = true;
        }
    }
};

std::shared_ptr<PhantomPage> PhantomPage::add_tab_item( uint8_t __tab_id, const char* __label ){
    PhantomTabItem item;
    item.label              = __label;
    item.tab_id             = __tab_id;
    tab_items[tab_item_ids] = item;
    ++tab_item_ids;
    return shared_from_this();
};

std::shared_ptr<PhantomPage> PhantomPage::make_temporary(){
    is_temporary = true;
    return shared_from_this();
}

std::shared_ptr<PhantomPage> PhantomPage::set_bg_color( int color ){
    bg_color = color;
    return shared_from_this();
};

// call this only after the page has all tabs filled with all widgets
// currently it will add the nav widget to every tab
// instead of using a single nav for all tabs... Will change this. 
// type 1 = normal close/prev/next at bottom of the page
// type 2 = not done yet but will be visible tabs on the top
std::shared_ptr<PhantomPage> PhantomPage::add_nav( uint8_t type ){

    if( num_tabs == 1 ){ return shared_from_this(); } // only a single tab; ignore;    

    static const int button_height = 30;
    
    //##############################################
    // Add navigation
    //##############################################
    uint8_t __active_tab = active_tab; // backup active tab

    std::shared_ptr<PhantomWidget> settings_navigation = factory_button( 320, button_height, 0, 240-button_height, 0, true );
    
    if( type == 1 ){

        //#################################################
        // previous / next buttons
        //#################################################
        settings_navigation->addItem(OUTPUT_TYPE_NONE,NAV_LEFT, PARAM_TYPE_INT, PARAM_ID_PAGE_NAV_LEFT,80,button_height,160,0,true,-1)
                           ->set_outlines(2)
                           ->on(TOUCHED,phantom_previous_cb).on(EVENT_LOAD,phantom_set_state_cb);
        settings_navigation->addItem(OUTPUT_TYPE_NONE,NAV_RIGHT, PARAM_TYPE_INT, PARAM_ID_PAGE_NAV_RIGHT,80,button_height,240,0,true,-1)
                           ->on(TOUCHED,phantom_next_cb).on(EVENT_LOAD,phantom_set_state_cb);

    } else if( type == 2 ){

        //###############################################
        // Include top tabs
        //###############################################
        int16_t widget_width = 320;
        int16_t item_width   = widget_width / tab_item_ids;
        int16_t current_x    = 0;
        std::shared_ptr<PhantomWidget> tab_nav = factory_button( widget_width, 35, 0, 0, 0, true );
        //tab_nav              = new PhantomButton( widget_width, 35, 0, 0, 0, true );
        for( int i = 0; i < tab_item_ids; ++i ){
            tab_nav->addItem(OUTPUT_TYPE_NONE,tab_items[i].label, PARAM_TYPE_INT, PARAM_ID_PAGE_NAV_LEFT,item_width,35,current_x,0,true,DEFAULT_FONT)
                   ->set_outlines(2)
                   //->set_colors(0, BUTTON_LIST_LI, TFT_WHITE ) //def
                   ->set_colors(1, PAGE_TAB_BG, PAGE_TAB_FG ) //touch
                   ->set_colors(2, PAGE_TAB_BG, PAGE_TAB_FG ) //active
                   //->set_colors(1,bg_color,TFT_WHITE)
                   ->set_int_value( tab_items[i].tab_id )
                   ->on(TOUCHED,phantom_to_tab_cb).on(EVENT_LOAD,phantom_set_tab_state_cb);
            current_x += item_width;
        }

        for( int i = 0; i < num_tabs; ++i ){
            active_tab = i;
            add_widget( tab_nav );
        }
        active_tab = __active_tab;

    }

    //##############################
    // close button
    //##############################
    settings_navigation->addItem(OUTPUT_TYPE_NONE,DICT_CLOSE, PARAM_TYPE_NONE, PARAM_ID_PAGE_NAV_CLOSE,type==1?160:320,button_height,0,0,true,DEFAULT_FONT)
                       ->set_outlines(2)
                       ->on(TOUCHED,phantom_close_cb);



    
    for( int i = 0; i < num_tabs; ++i ){
        active_tab = i;
        add_widget( settings_navigation );
    }
    active_tab = __active_tab; // restore active tab
    //select_tab( active_tab );
    return shared_from_this();
}
