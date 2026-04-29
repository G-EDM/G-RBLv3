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


#include <Preferences.h>
#include <iomanip>
#include "ui_interface.h"
#include "gpo_scope.h"
#include "ili9341_tft.h"
#include "char_helpers.h"
#include "widgets/widget_numkeyboard.h"
#include "widgets/widget_alphakeyboard.h"
#include "widgets/widget_alarm.h"
#include <atomic>
#include <nvs_flash.h>

#define DEBUG_SD_CARD


std::string PhantomUI::get_firmware_key( void ){
    std::string key = "";
    key += FIRMWARE_VERSION;
    key += MASHINE_AXES_STRING;
    return key;
}

//############################################################################################################
// Some wrappe functions for the preference library with locking etc.
//############################################################################################################
Preferences pref;
static volatile bool pref_init_ok = false;

bool pref_init(){
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
    if( !pref_init_ok ){
        int count = 0;
        while( !pref_init_ok && ++count < 200 ){
            if( pref.begin( RELEASE_NAME, false ) ){
                pref_init_ok = true;
                break;
            }
            vTaskDelay(20);
        }
        if(!pref_init_ok){ // still nothing
            debuglog("*Failed to open NVS", 500);
            debuglog("*This is bad", 1000);
        }
    }
    bool success = pref_init_ok;
    release_lock_for( ATOMIC_LOCK_GSCOPE );
    return success;
}

void pref_stop(){
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
    pref.end();
    pref_init_ok = false;
    release_lock_for( ATOMIC_LOCK_GSCOPE );
}

void pref_write_bool( const char* key, bool value ){
    //pref_init();
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
    if( pref.getBool( key ) != value ){
        pref.putBool( key, value );
    }
    release_lock_for( ATOMIC_LOCK_GSCOPE );
}

void pref_write_int( const char* key, int value ){
    //pref_init();
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
    if( pref.getInt( key ) != value ){
        pref.putInt( key, value );
    }
    release_lock_for( ATOMIC_LOCK_GSCOPE );
}
void pref_write_float( const char* key, float value ){
    //pref_init();
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
    if( pref.getFloat( key ) != value ){
        pref.putFloat( key, value );
    }
    release_lock_for( ATOMIC_LOCK_GSCOPE );
}

bool pref_get_bool( const char* key ){
    //pref_init();
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
    bool value = pref.getBool( key );
    release_lock_for( ATOMIC_LOCK_GSCOPE );
    return value;
}
int pref_get_int( const char* key ){
    //pref_init();
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
    int value = pref.getInt( key );
    release_lock_for( ATOMIC_LOCK_GSCOPE );
    return value;
}
float pref_get_float( const char* key ){
    //pref_init();
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
    float value = pref.getFloat( key );
    release_lock_for( ATOMIC_LOCK_GSCOPE );
    return value;
}

size_t pref_write_bytes( const char* key, void *buf, size_t maxLen ){
    //pref_init();
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
    size_t value = pref.putBytes( key, buf, maxLen );
    release_lock_for( ATOMIC_LOCK_GSCOPE );
    return value;
}

size_t pref_get_bytes( const char* key, void *buf, size_t maxLen ){
    //pref_init();
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
    size_t value = pref.getBytes( key, buf, maxLen );
    release_lock_for( ATOMIC_LOCK_GSCOPE );
    return value;
}

bool pref_remove( const char* key ){
    //pref_init();
    bool success = false;
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
    success = pref.remove( key );
    release_lock_for( ATOMIC_LOCK_GSCOPE );
    return success;
}

void pref_clear(){
    //pref_init();
    acquire_lock_for( ATOMIC_LOCK_GSCOPE );
    bool success = pref.clear();
    release_lock_for( ATOMIC_LOCK_GSCOPE );
    debuglog( success ? "@NVS deleted!" : "*Failed to delete NVS!" );
}


void nvs_full_delete() {
    pref_clear();
    pref_stop();

    nvs_flash_erase();
    nvs_flash_init();

    pref_init();

}

bool PhantomUI::firmware_is_recent_set_flag(){
    pref_write_bool( get_firmware_key().c_str(), true );
    return true;
}

bool PhantomUI::firmware_is_recent(){
    return pref_get_bool( get_firmware_key().c_str() );
}

bool PhantomUI::delete_nvs_storage(){
    debuglog("Deleting full NVS storage", DEBUG_LOG_DELAY );
    nvs_full_delete();
    vTaskDelay(1000);
    return true;
}



//############################################################################################################
// 
//############################################################################################################










//############################################################################################################
// Shared pointer to base class factory functions
//############################################################################################################
std::shared_ptr<PhantomWidget> factory_button(      int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s ){ return std::make_shared<PhantomButton>( w, h, x, y, t, s ); }
std::shared_ptr<PhantomWidget> factory_split_list(  int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s ){ return std::make_shared<PhantomSplitList>( w, h, x, y, t, s ); }
std::shared_ptr<PhantomWidget> factory_list(        int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s ){ return std::make_shared<PhantomList>( w, h, x, y, t, s ); }
std::shared_ptr<PhantomWidget> factory_vbuttonlist( int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s ){ return std::make_shared<PhantomVerticalButtonList>( w, h, x, y, t, s ); }
std::shared_ptr<PhantomWidget> factory_nkeyboard(   int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s ){ return std::make_shared<PhantomKeyboardNumeric>( w, h, x, y, t, s ); }
std::shared_ptr<PhantomWidget> factory_akeyboard(   int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s ){ return std::make_shared<PhantomKeyboardAlpha>( w, h, x, y, t, s ); }
std::shared_ptr<PhantomLogger> factory_logger(      int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s ){ return std::make_shared<PhantomLogger>( w, h, x, y, t, s ); }
std::shared_ptr<PhantomWidget> factory_elist(       int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s ){ return std::make_shared<file_browser>( w, h, x, y, t, s ); }


typedef struct touch_cache {
    uint16_t x, xf, xx, y, yf, yy, item_pos_x, item_pos_y, item_w, item_h;
    bool block_touch, touched, scanning;
    uint64_t last_time;
} touch_cache;


//############################################################################################################
// Symbols, suffixes and stuff
//############################################################################################################
const char *suffix_table[]                          = {"", " kHz", " %", " mm", " mm/min", " RPM", " kSps", " Lines", " ADC", " us", " V", " A", " s", "ms", " steps", " ", " mA", 0};
const char icons[9][2]                              = {"i","h","j","f","?","g","d","c","e"}; // icon font
void (*page_creator_callbacks[TOTAL_PAGES])( void ) = { nullptr };

DRAM_ATTR bool settings_changed = false;
DRAM_ATTR touch_cache touch;

PhantomUI ui;

std::atomic<bool> ui_ready(false);
//DRAM_ATTR std::atomic<bool> reload_waiting(false);
DRAM_ATTR std::atomic<bool> logs_available(false);
DRAM_ATTR std::atomic<bool> logger_show_on_line(false);


// reload only items flagged as refreshable
IRAM_ATTR void PhantomUI::reload_page_on_change(){

    if( 
        //reload_waiting.load() || 
        !get_is_ready() || 
        touch.block_touch || 
        pages.find(active_page) == pages.end() 
    ) return;

    //reload_waiting.store(true);
    //acquire_lock_for( ATOMIC_LOCK_LOGGER );
    //reload_waiting.store(false);
    pages[active_page]->refresh();
    //release_lock_for( ATOMIC_LOCK_LOGGER );

};

void show_logger() { // only used on page creation to enforce a redraw of the logger overlay
    if( 
        !ui.logger->is_visible || 
        !logs_available.load() || 
        !acquire_lock_for( ATOMIC_LOCK_LOGGER, false ) 
    ) return;
    logs_available.store(false);
    ui.logger->show();
    release_lock_for( ATOMIC_LOCK_LOGGER );
}


void charge_logger( const char *line ){
    acquire_lock_for( ATOMIC_LOCK_LOGGER );
    logs_available.store(false);
    if( logger_show_on_line.load() || !ui.get_is_ready() ){ 
        portENTER_CRITICAL(&msg_logger_mutex);
        ui.logger->add_line(line);
        ui.logger->show(); 
        portEXIT_CRITICAL(&msg_logger_mutex);
    } else {
        ui.logger->add_line(line);
    }
    logs_available.store(true);
    release_lock_for( ATOMIC_LOCK_LOGGER );
};







const std::function<void(PhantomWidgetItem *item)> load_previous_page = [](PhantomWidgetItem *item){
    ui.show_page( ui.get_previous_active_page() );
};

static void create_info_page( info_codes msg_id ){
    auto page = ui.addPage( PAGE_INFO, 1 ); // create page with only 1 tab
    std::shared_ptr<PhantomWidget> infobox = factory_list( 280, (4+tft.fontHeight(DEFAULT_FONT))*4, 10, 10, 2, true );
    infobox->addItem(OUTPUT_TYPE_STRING,"P",PARAM_TYPE_NONE,0,0,0,0,0,false,-1)->set_colors( 0, -1, TFT_DARKGREY );
    infobox->addItem(OUTPUT_TYPE_STRING,"Info",PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)->set_colors( 0, -1, TFT_GREENYELLOW );
    infobox->addItem(OUTPUT_TYPE_STRING,info_messages[msg_id],PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)->set_colors( 0, -1, TFT_LIGHTGREY );
    page->add_widget( infobox );
    std::shared_ptr<PhantomWidget> backbtn = factory_button( 300, 40, 10, 190, 1, true );
    backbtn->addItem(OUTPUT_TYPE_STRING,DICT_CLOSE,PARAM_TYPE_NONE,0,300,40,0,0,true,DEFAULT_FONT)->set_radius(4)->set_string_value(DICT_CLOSE)->on(TOUCHED,load_previous_page);
    page->add_widget( backbtn );
    page->select_tab( ui.get_active_tab_for_page( PAGE_INFO ), true ); // select first tab of this page again 
    page->make_temporary();
    page = nullptr;
}

//######################################################################################
// Default helper load function to get the values for given parameter, all load function should
// load the paramater with this function. If there is some custom logic needed it needs
// to be done within the callback
//######################################################################################
void param_load_raw( int param_id ){ // load the values for a parameter and stores it into the cache. Nothing else
    PhantomSettingsPackage package;
    package.intValue   = 0;
    package.floatValue = 0.0;
    package.boolValue  = false;
    package.param_id   = param_id;
    ui_controller.emit( PARAM_GET, &package );

    settings_values_cache[ param_id ] = setting_cache_object( package.intValue, package.floatValue, package.boolValue );
    //settings_values_cache.erase( param_id );
    //settings_values_cache.emplace( param_id, setting_cache_object( package.intValue, package.floatValue, package.boolValue ) );
}

IRAM_ATTR void param_load( PhantomWidgetItem *item, std::function<void(PhantomWidgetItem *item, PhantomSettingsPackage *package)> callback = nullptr ){
    if( item == nullptr ){ return; }
    PhantomSettingsPackage package;
    package.intValue   = item->intValue;
    package.floatValue = item->floatValue;
    package.boolValue  = item->boolValue;
    package.param_id   = item->param_id;

    ui_controller.emit( PARAM_GET, &package );
    item->intValue   = package.intValue;
    item->floatValue = package.floatValue;
    item->boolValue  = package.boolValue;

    settings_values_cache[ package.param_id ] = setting_cache_object( package.intValue, package.floatValue, package.boolValue );
    //settings_values_cache.erase( package.param_id );
    //settings_values_cache.emplace( package.param_id, setting_cache_object( package.intValue, package.floatValue, package.boolValue ) );
    if( callback != nullptr ){ callback( item, &package ); }
}






template<size_t N>
void log_pump( const char* (&logs)[N], uint16_t delay = DEBUG_LOG_DELAY ){
    vTaskDelay(DEBUG_LOG_DELAY);
    size_t item_num = N;
    for( int i = 0; i < item_num; ++i ){ debuglog( logs[i], 2 ); }
    vTaskDelay( delay );
}




static DRAM_ATTR volatile bool flush_scope = false;
void scope_flush(){
    if( !arcgen.get_pwm_is_off() ) return; // this is only needed if scope is inactive
    gscope.flush_frame();
    logs_available.store( true );
}






// callback is used by the ui_controller backend class
const std::function<void(int *page_id)> show_page_ev = [](int *page_id){
    int page = *page_id;
    if( gscope.scope_is_running() ){
        gscope.stop();
    }
    ui.show_page( page ); // using the page builder function
};


void PhantomUI::set_scope_and_logger( uint8_t page_id ){ // called after the page is drawn
    // very dirty temporary solution
    // to integrate the scope etc. without rewriting it
    // called after a page was created
    if( gscope.scope_is_running() ){ 
        gscope.stop(); 
    }

    if( 
        page_id == PAGE_FRONT || 
        page_id == PAGE_IN_PROCESS 
    ){
        update_scope_stats();
        gscope.init( 320, 75, 0, (page_id == PAGE_FRONT?31:0) );
        gscope.start();
        if( page_id == PAGE_FRONT ){
            flush_scope   = true;
            ui.deep_check = true;
        }
    }

}




static const std::function<void(info_codes *data)> info_box_lambda = [](info_codes *data){
    if( ui.get_active_page() == PAGE_INFO ){ return; }
    create_info_page( *data );
    ui.show_page( PAGE_INFO );
};

const std::function<void(int *data)> reload_page_if_changed = [](int *data){
    ui.reload_page_on_change();
};


//###################################################################
// End sd card job
//###################################################################
const std::function<void(int *data)> end_sd_job_lambda = [](int *data) {
    SD_CARD.end_gcode_job();
};

//###################################################################
// Start a gcode file. This opens the file and ensures it is valid.
//###################################################################
const std::function<void(int *data)> start_sd_job_lambda = [](int *data) {
    edm_stop_codes error = STOP_CLEAN; // default no error...
    if( ui.gcode_file.length() <= 0 ){ // should not happen easily
        error = STOP_NO_FILE_GIVEN;
        debuglog("No GCode file given!");
    } else {
        if( !SD_CARD.start_gcode_job( ui.gcode_file ) ){
            // failed to open file
            error         = STOP_SD_FAILED_TO_INIT;
            debuglog("*Failed to load file!");
        } else {
            debuglog("@File loaded",100);
        }
    }


    if( error != STOP_CLEAN ){
        gcode_line_shifting.store( false ); // disable line shifting
        const char * buffer = int_to_char( error );
        ui.emit_param_set( PARAM_ID_SET_STOP_EDM, buffer );
        delete[] buffer;
        buffer = nullptr;
    } else {
        gcode_line_shifting.store( true ); // enable line shifting
    }

};

//###################################################################
// SD card removed or inserted
//###################################################################
const std::function<void(int *state)> sd_change = [](int *state){
    if( ui.sd_card_state == *state ){
        // unchanged
        return;
    }
    ui.sd_card_state = *state;
    if( SD_CARD.get_state() == SD_NOT_AVAILABLE ){
        // sd card not available 
        // this state is not a busy state and means there is no sd card or sd card could not be found
        // reset the gcode file?
        ui.set_gcode_file(""); // this would reset the gcode file and inform the backend that no file is selected
                               // it is much easier to implement for now then every other option like checking for the file
                               // before running the job after a sd swap.. Let it be for now..... #todo
    } 
    ui.reload_page_on_change(); // not very specific... it will reload all changables
                                // need to write a function that only updates the items if the requirement changed
};


const std::function<void(int *data)> reset_lambda = [](int *data){
    end_sd_job_lambda( data );
    ui.load_next_page = false;
};




// todo: send motion state from ui_controller, default to not enabled
// and don't poll the state all the time
const std::function<void(PhantomWidgetItem *item)> check_requirements = [](PhantomWidgetItem *item){
    bool disable = false;
    if( item->dpm_required ){
        PhantomSettingsPackage package;
        package.param_id = PARAM_ID_DPM_UART;
        ui_controller.emit(PARAM_GET,&package);
        if( !package.boolValue ){
            disable = true;
        }
    }
    if( item->sd_required ){
        if( !SD_CARD.is_free() ){
            disable = true;
        }
    }
    if( item->motion_required ){
        PhantomSettingsPackage package;
        package.param_id = PARAM_ID_MOTION_STATE;
        ui_controller.emit(PARAM_GET,&package);
        if( !package.boolValue ){
            disable = true;
        }
    }
    if( disable ) {
        if( !item->disabled ){
            item->changed  = true;
            item->disabled = true;
        }
    } else {
        if( item->disabled ){
            item->changed  = true;
            item->disabled = false;
        }
    }
};


const std::function<void(PhantomWidgetItem *item)> get_homed_state = [](PhantomWidgetItem *item) {
    param_load( item, [](PhantomWidgetItem *item,PhantomSettingsPackage *package){
        if( !item->boolValue ){
            item->is_active = false;
        } else {
            item->is_active = true;
        }
    });
};
const std::function<void(PhantomWidgetItem *item)> get_homing_enabled = [](PhantomWidgetItem *item) {
    param_load( item, [](PhantomWidgetItem *item,PhantomSettingsPackage *package){
        if( item->boolValue ){
            item->is_active = true;
            item->strValue  = "Enabled";
        } else {
            item->is_active = false;
            item->strValue  = "Disabled";
        }
    });
};
const std::function<void(PhantomWidgetItem *item)> get_homing_enabled_2 = [](PhantomWidgetItem *item) {
    param_load( item, [](PhantomWidgetItem *item,PhantomSettingsPackage *package){
        if( item->boolValue ){
            item->disabled = false;
        } else {
            item->disabled = true;
        }
    });
};

const std::function<void(PhantomWidgetItem *item)> load_toogle_active_state = [](PhantomWidgetItem *item) {
    param_load( item, [](PhantomWidgetItem *item,PhantomSettingsPackage *package){
        if( item->boolValue ){
            item->is_active = true;
            item->set_string_value( "Resume" );
        } else {
            item->is_active = false;
            item->set_string_value( "Pause" );
        }
    });
};

const std::function<void(PhantomWidgetItem *item)> save_settings_lambda = [](PhantomWidgetItem *item) {
    int action_id = item->intValue;
    ui.save_settings( static_cast<sd_write_action_type>(action_id) );
};


//##########################################################################################
// Open a page based on the items intValue (page_id)
//##########################################################################################
const std::function<void(PhantomWidgetItem *item)> open_page = [](PhantomWidgetItem *item) {
    ui.show_page( item->intValue );
};




const std::function<void(PhantomWidgetItem *item)> mode_load = [](PhantomWidgetItem *item) {
    // load values here!!!!
    switch (item->intValue){
        case 1:
        item->strValue = DICT_MODE1;
        break;
        case 2:
        item->strValue = DICT_MODE2;
        break;
        case 3:
        item->strValue = DICT_MODE3;
        break;
        case 4:
        item->strValue = DICT_MODE4;
        break;
        default:
        item->strValue = DICT_UNDEF;
        break;
    }
    vTaskDelay(1);
};


const std::function<void(PhantomWidgetItem *item)> sinker_dir_lambda = [](PhantomWidgetItem *item) {
    // load values here!!!!
    switch(item->intValue){
        case 0:
        item->strValue = "Z down";
        break;
        case 1:
        item->strValue = "X right";
        break;
        case 2:
        item->strValue = "X left";
        break;
        case 3:
        item->strValue = "Y forward";
        break;
        case 4:
        item->strValue = "Y back";
        break;
        default:
        item->strValue = "Not found";
        break;
    }
    vTaskDelay(1);
};

const std::function<void(PhantomWidgetItem *item)> num_to_string_lambda = [](PhantomWidgetItem *item) {
    std::ostringstream sstream; // slow and maybe not good, does only support smaller numbers
    if( item->valueType == PARAM_TYPE_FLOAT ){
        sstream << item->floatValue;
    } else if( item->valueType == PARAM_TYPE_INT ){
        sstream << item->intValue;
    }
    item->strValue = sstream.str();
    if( item->suffix > 0 ){
        item->strValue += " ";
        item->strValue += item->suffix;
    }
};

const std::function<void(PhantomWidgetItem *item)> set_probe_pos_string = [](PhantomWidgetItem *item) {
    if( !item->boolValue ){
          item->strValue = item->label;
          item->strValue += ": ";
          item->strValue += DICT_NOTPROBE;
    } else {
        std::ostringstream sstream; // slow and maybe not good, does only support smaller numbers
        if( item->valueType == PARAM_TYPE_FLOAT ){
            sstream << item->floatValue;
        } else if( item->valueType == PARAM_TYPE_INT ){
            sstream << item->intValue;
        }
        item->strValue = item->label;
        item->strValue += ": ";
        item->strValue += sstream.str();
        //item->strValue.append( suffix_table[item->suffix] );
    }
    vTaskDelay(1);
};



























//########################################################################################
// Open the numeric keybard to change float and int parameters
//########################################################################################
static void build_keyboard( PhantomSettingsPackage * package, bool floatType, int param_type ) {
    package->type = floatType ? 1 : 0;
    auto page = ui.addPage( NUMERIC_KEYBOARD, 1 ); // page for numeric keyboard
    std::shared_ptr<PhantomWidget> num_keyboard = factory_nkeyboard( 320, 240, 0, 0, 2, false );
    auto __this = std::dynamic_pointer_cast<PhantomKeyboardNumeric>( num_keyboard );
    num_keyboard->create();
    page->make_temporary();
    num_keyboard->reset_widget( package, param_type );
    __this->set_tooltip( tooltips[package->param_id] );
    __this->set_callback([](PhantomSettingsPackage _package){
        const char* buffer = _package.type == 1 ? float_to_char( _package.floatValue ) : int_to_char( _package.intValue );
        ui.emit_param_set( _package.param_id, buffer );
        delete[] buffer;
        buffer = nullptr;
        settings_values_cache.erase( _package.param_id );
        ui.save_param_next = true;
        ui.save_param_id   = _package.param_id;
    });
    page->add_widget( num_keyboard );
    ui.show_page( NUMERIC_KEYBOARD );
    page = nullptr;
    num_keyboard = nullptr;
};










PhantomUI::PhantomUI(){

    refresh_sd_disabled = false;
    // tft_espi does not care about the SD chipselect pin
    // enforce deselect to prevent troubles until sd card is 
    // added to the spi
    pinMode( SS_OVERRIDE, OUTPUT );
    digitalWrite( SS_OVERRIDE, HIGH );
    // create the logger
    logger = factory_logger( 260, 128, 20, 100, 1, true );
    logger->build_ringbuffer( 0 );
    logger->is_visible = false;
}

void PhantomUI::restart_display(){
    if( get_active_page() != PAGE_IN_PROCESS ){ return; }
    tft.init();vTaskDelay(1);
    tft.setRotation(3);
    tft.setCursor(0, 0, 4);
    fill_screen(TFT_BLACK);
    touch_calibrate( false );vTaskDelay(1);
    tft.setCursor(0, 0, 4);
    tft.setTextColor(TFT_LIGHTGREY);
    show_page( get_active_page() );
    debuglog("Display reset called");
    debuglog("Touch outside button");
    debuglog("to trigger a reset");
}

// fully reload the page
void PhantomUI::reload_page(){
    if( !get_is_ready() || touch.block_touch ){return;}
    if( pages.find(active_page) == pages.end() ){ return; }
    pages[active_page]->show(); // direct access of the page without using the page builder function
};




//###################################################################
// get the next gcode line from sd card
//###################################################################
IRAM_ATTR void shift_gcode_line(){
    if( gcode_line_shifting.load() && !SD_CARD.shift_gcode_line() ){
        gcode_line_shifting.store( false );
        debuglog("GCode file end");
    }
};


//#############################################################################
// Display writes can be triggered by touch events
// but those touch events based draws are all happening within this function
//#############################################################################
const std::function<void(int *data)> monitor_touch_lambda = [](int *data){
    if( is_machine_state( STATE_REBOOT ) ){
        return;
    } else if( ! is_machine_state( STATE_HOMING ) ){
        shift_gcode_line();
        gscope.draw_scope();
    }


    if( ui.load_settings_file_next ){
        ui.load_settings( ui.settings_file );
    }

    if( ui.deep_check ){

        ui.deep_check = false; // prevent all the if checks if not needed using the deep check flag

        vTaskDelay(10);

        if( ui.load_settings_file_next ){ // needs to be before the page load
            ui.load_settings_file_next = false;
            ui.load_settings( ui.settings_file );
            vTaskDelay(1);
        }

        if( ui.load_next_page ){
            ui.load_next_page = false;
            ui.show_page( ui.next_page_to_load );
            vTaskDelay(1);
        }

        if( ui.save_param_next ){ // this needs to be after the page load!
            ui.save_param_next = false;
            ui.save_current_to_nvs( ui.save_param_id );
            vTaskDelay(1);
        }

        if( flush_scope ){
            scope_flush();
            vTaskDelay(1);
        }

        ui.deep_check = false;
    
    } else {

        ui.monitor_touch();
        if( !ui.refresh_sd_disabled && !is_machine_state( STATE_HOMING ) ){
            logger_show_on_line.store( true );
            SD_CARD.refresh();
            logger_show_on_line.store( false );
        }
        show_logger();
 

    }
    

};




// show a page based on the page id
void PhantomUI::show_page( uint8_t page_id ){ // this should only be called to draw new pages, not to draw the tabs

    ui.logger->is_visible = false;

    if( 
        page_id == PAGE_MAIN_SETTINGS       || 
        page_id == PAGE_FILE_MANAGER        || 
        page_id == PAGE_FILE_MANAGER_GCODES ||
        page_id == NUMERIC_KEYBOARD         ||
        page_id == ALPHA_KEYBOARD
    ){
        refresh_sd_disabled = true; // no sd card refresh on this pages
    } else {
        refresh_sd_disabled = false;
    }

    if( page_id < TOTAL_PAGES ){
        lock_touch(); // build page if needed
        if( pages.find(page_id) == pages.end() ){ // page not found in the map
            if( page_creator_callbacks[page_id] != nullptr ){
                page_creator_callbacks[page_id]();
                auto page = get_page( page_id );
                if( page != nullptr ){
                    page->select_tab( active_tabs[page_id], true );
                } else {
                    debuglog("*Creating page failed!");
                    unlock_touch();
                    return;
                }
            } else {
                unlock_touch();
                return;
            }
        } 
        if( active_page != page_id ){
            close_active();
            if( page_tree_index < page_tree_depth-2 ){
                page_tree_index++;
            }
        }
        if( page_id == PAGE_FRONT ){
            page_tree_index = 0;
        }
        page_tree[page_tree_index] = page_id;
        active_page = page_id;

        pages[page_id]->show();
        vTaskDelay(DEBUG_LOG_DELAY);

        set_scope_and_logger( page_id );

        const char * buffer = int_to_char( active_page );
        emit_param_set( PARAM_ID_SET_PAGE, buffer );
        delete[] buffer;
        buffer = nullptr;
        unlock_touch();

    }
};













//############################################################################################################
// Custom fill screen function that does not fill the screen in one long run but in slices 
// Helps with high speed interrupts
//############################################################################################################
void fill_screen( int color ){
    // drawing a full screen is a heavy task and the ISR doesn't like it
    // having an ISR running at highspeed parallel to a heavy display ui needs some hacks
    int divider = 10, display_width = 320, display_height = 240, pos_y = 0, height_fragment = display_height / divider;
    for( int i = 0; i < divider; ++i ){
      tft.fillRect( 0, pos_y, display_width, height_fragment, color );
      pos_y += height_fragment;
      vTaskDelay(1);
    }
}


//############################################################################################################
// Main touch monitor 
// Checks for a touch event and loops over all items in all widget in the current tab of the active page
// and triggers an event if the touch was done on an item thaqt is touchable
//############################################################################################################
IRAM_ATTR void PhantomUI::track_touch() {
    touch.xf = touch.x;
    touch.yf = touch.x;
    while( tft.getTouch( &touch.xx, &touch.yy, 500 ) ){ 
        touch.xf = touch.xx;
        touch.yf = touch.yy;
        vTaskDelay(1); 
    }
}

IRAM_ATTR bool PhantomUI::touch_within_bounds( int x, int y, int xpos, int ypos, int w, int h ){
    return ( x >= xpos && x <= (xpos + w) && y >= ypos && y <= (ypos + h) );
}




IRAM_ATTR void PhantomUI::monitor_touch(){

    if( touch.scanning == true ){
        return;
    }

    touch.scanning = true;

    if( !touch.block_touch && tft.getTouch( &touch.x, &touch.y ) ){

        if( pages.find(active_page) == pages.end() ){
            touch.scanning = false;
            return;
        }

        touch.touched = false;
        active_tab    = pages[active_page]->get_active_tab();

        auto& widgets = pages[active_page]->tabs[active_tab]->widgets;
    
        for( auto& widget : widgets ){

            if( touch.touched || touch.block_touch ){ break; }

            vTaskDelay(1); 

            if( touch_within_bounds( touch.x, touch.y, widget.second->xpos, widget.second->ypos, widget.second->width, widget.second->height ) ){

                // touch is within this widget container
                // lets scan for the specific item
                for( auto& item : widget.second->items ){

                    if( touch.block_touch ){ break; }
     
                    if( touch_within_bounds( touch.x, touch.y, item.second.truex, item.second.truey, item.second.width, item.second.height ) ){

                        vTaskDelay(1); 

                        if( item.second.touch_enabled && !item.second.disabled ){
                            bool lock_taken = arcgen.lock();
                                item.second.has_touch = true;
                                widget.second->redraw_item( item.second.item_index, false );
                                track_touch();
                                item.second.has_touch = false;
                                widget.second->redraw_item( item.second.item_index, false );
                                item.second.emit_touch();
                                touch.touched  = true;
                                touch.scanning = false;
                            arcgen.unlock( lock_taken );

                            return;
                        } else { // touch within this item but no touch enabled. Can exit.
                            touch.touched = true;
                            break;
                        }

                    }

                }

            }

            if( touch.touched || touch.block_touch ){ break; }

        }

        if( !touch.touched ){
            ui.restart_display(); // this only resets the display if within a running edm process and only if the touch was not at any touch element
                                  // if the display goes blank just touch anywhere on the display where there is no button
                                  // also only works if no at the settings menu
        }

    }

    touch.scanning = false;

};

// callback used to refresh the refreshable objects in the current page


//########################################################################################
// Main gateway to change setting. Creates a key, value pair and emits it to the backend
//########################################################################################
const std::function<void(PhantomWidgetItem *item)> change_setting = [](PhantomWidgetItem *item){

    bool lock_taken = arcgen.lock();

    item->has_touch = true;

    uint8_t page_id   = item->get_parent_page_id();
    uint8_t tab_id    = item->get_parent_page_tab_id();
    uint8_t widget_id = item->get_widget_id();
    uint8_t item_id   = item->get_index();
    uint8_t param_id  = item->param_id;

    PhantomSettingsPackage package;
    package.intValue   = item->intValue;
    package.floatValue = item->floatValue;
    package.boolValue  = item->boolValue;
    package.param_id   = item->param_id;
    uint8_t param_type = item->valueType;

    std::shared_ptr<PhantomWidget> widget = ui.get_widget(page_id, tab_id, widget_id, item_id);

    if( item->refreshable ){
    }
    widget->redraw_item( item->item_index, false );

    switch (item->valueType) {

        case PARAM_TYPE_BOOL:
        case PARAM_TYPE_BOOL_RELOAD_WIDGET:
        case PARAM_TYPE_BOOL_RELOAD_WIDGET_OC:
            item->boolValue = !item->boolValue;
            ui.emit_param_set( item->param_id, item->boolValue?"true":"false" );
            item->has_touch = false;
            if( item->valueType == PARAM_TYPE_BOOL_RELOAD_WIDGET ){
                if( widget->is_refreshable ){
                    item->changed = true;
                    widget->show_on_change();
                } else { 
                    widget->show(); 
                }
            } else if( item->valueType == PARAM_TYPE_BOOL_RELOAD_WIDGET_OC ){
                item->changed = true;
                widget->show_on_change();
            } else {
                widget->redraw_item( item->item_index, true );
            }
            ui.save_current_to_nvs( item->param_id );
            break;

        case PARAM_TYPE_INT:
        case PARAM_TYPE_INT_RAW_RELOAD_WIDGET:
        case PARAM_TYPE_INT_INCR:
            if( item->valueType == PARAM_TYPE_INT_INCR ){ // incremental value (currently without ranges...)

            } else if( item->valueType == PARAM_TYPE_INT_RAW_RELOAD_WIDGET ){ // no keyboard input
                const char* buffer = int_to_char( item->intValue );
                ui.emit_param_set( item->param_id, buffer );
                delete[] buffer;
                buffer = nullptr;
                item->has_touch = false;
                widget->show();
                vTaskDelay(1);
                ui.save_current_to_nvs( item->param_id );
            } else { // keyboard input
                build_keyboard( &package, false, param_type );
            }
        break;

        case PARAM_TYPE_FLOAT:
            build_keyboard( &package, true, param_type );    
            break;

        case PARAM_TYPE_NONE:
        case PARAM_TYPE_NONE_REDRAW_WIDGET:
        case PARAM_TYPE_NONE_REDRAW_PAGE:
            ui.emit_param_set( item->param_id, "" );
            item->has_touch = false;
            if (item->valueType == PARAM_TYPE_NONE_REDRAW_PAGE) {
                widget = nullptr;
                ui.reload_page();
            } else {
                if (item->valueType == PARAM_TYPE_NONE_REDRAW_WIDGET) {
                    widget->show();
                } else {
                    widget->redraw_item(item->item_index, true);
                }
            }
            break;
    }

    widget = nullptr;

    arcgen.unlock( lock_taken );
    vTaskDelay(1);
    return;
};



























//############################################################################################################
// Widget control methos
//############################################################################################################
// use with care as there are no checks if an item exists!
std::shared_ptr<PhantomWidget> PhantomUI::get_widget( uint8_t page_id, uint8_t page_tab_id, uint8_t widget_id, uint8_t item_id ){
    return pages[page_id]->tabs[page_tab_id]->widgets[widget_id];
}

std::shared_ptr<PhantomWidget> PhantomUI::get_widget_by_item( PhantomWidgetItem * item ){ // use with care due to the slow speed
    if( item == nullptr ){ return nullptr; }
    auto page = pages.find( item->get_parent_page_id() );
    if( page == pages.end() ){ return nullptr; } // page not found
    auto tab = page->second->tabs.find( item->get_parent_page_tab_id() );
    if( tab == page->second->tabs.end() ){ return nullptr; } // tab not found
    auto widget = tab->second->widgets.find( item->get_widget_id() );
    if( widget == tab->second->widgets.end() ){ return nullptr; } // widget not found
    return widget->second;
}

//############################################################################################################
// Page control methods
//############################################################################################################
// get the cached active tab for a given page
uint8_t PhantomUI::get_active_tab_for_page( uint8_t page_id ){
    return active_tabs[page_id];
}
// update the cached active tab for a given page
void PhantomUI::update_active_tabs( uint8_t page_id, uint8_t tab ){
    active_tabs[page_id] = tab;
}

// close active page and if flagged as temporary (work in progress!) it will delete the page and all the stuff in it to free memory
// Deleting pages does not fully work right now. It works with some pages but there is some black magic for some other pages
// that crashes the ESP. Maybe some pointer related issues I need to fix
void PhantomUI::close_active(){ 
    auto page = pages.find(active_page);
    if( page == pages.end() ){ 
        return; 
    }
    // backup active tab
    active_tabs[active_page] = page->second->get_active_tab();
    page->second->close(); // flags all the widgets in the page to hidden and fills the screen with bg color
    if( page->second->is_temporary_page() ){ // if temporary fully delete the page calling all destructors deleting all pointers etc.
        pages.erase(page);
    }
}
// get the previous page from the history tree to move back
uint8_t PhantomUI::get_previous_active_page(){
    if( page_tree_index > 0 ){ 
        page_tree_index--;
    } 
    uint8_t page_id = page_tree[page_tree_index];

    while( // history pages to ignore
            page_id == ALPHA_KEYBOARD 
         || page_id == NUMERIC_KEYBOARD 
         || page_id == PAGE_ALARM
         || page_id == PAGE_INFO
    ){
        if( page_tree_index == 0 ){ break; }
        --page_tree_index;
        page_id = page_tree[page_tree_index];
    }

    if( page_id == PAGE_NONE || page_tree_index == 0 ){ 
        page_id = PAGE_FRONT;
    }
    if( page_tree_index > 0 ) { page_tree_index--; }
    return page_id;
}

void PhantomUI::lock_touch( void ){
    touch.block_touch = true;
}
void PhantomUI::unlock_touch( void ){
    touch.block_touch = false;
}
void PhantomUI::show_page_next( uint8_t page_id ){
    next_page_to_load = page_id;
    load_next_page    = true;
    deep_check        = true;
}


// create a new page and add it to the pages map
// it will also create the tabs for the page
std::shared_ptr<PhantomPage> PhantomUI::addPage( uint8_t page_id, uint8_t num_tabs = 1 ){
    if( pages.find(page_id) != pages.end() ) {}
    pages.emplace(page_id, std::make_shared<PhantomPage>( page_id, num_tabs )); //
    return pages.find(page_id)->second;
};
// get a shared pointer to the page by page id
// returns a nullptr if page does not exist
std::shared_ptr<PhantomPage> PhantomUI::get_page( uint8_t page_id ){
    auto page = pages.find(page_id);
    return page == pages.end() ? nullptr : page->second;
};

uint8_t PhantomUI::get_active_page(){
    return active_page;
}
bool PhantomUI::get_motion_is_active( void ){
    return motion_is_active;
}
void PhantomUI::set_motion_is_active( bool state ){
    motion_is_active = state;
}







//######################################################################################
// Default load function
//######################################################################################
const std::function<void(PhantomWidgetItem *item)> load = [](PhantomWidgetItem *item){
    param_load( item );
    if( item->suffix != 0 ){
        std::ostringstream sstream; // slow and maybe not good, does only support smaller numbers
        if( item->valueType == PARAM_TYPE_FLOAT ){
            //sstream << item->floatValue;
            sstream << std::setprecision(4) << item->floatValue;
        } else if( item->valueType == PARAM_TYPE_INT ){
            sstream << item->intValue;
        }
        item->strValue = sstream.str();
        item->strValue.append( suffix_table[item->suffix] );
    }
    vTaskDelay(1);
};
const std::function<void(PhantomWidgetItem *item)> load_mode_custom = [](PhantomWidgetItem *item) {
    int backup_int_value = item->intValue;
    item->intValue = PARAM_ID_MODE; // very dirty hack here... The intValue of the item is not the param_id in this case
                                    // backup the int value and restore it after load
    param_load( item, [backup_int_value](PhantomWidgetItem *item,PhantomSettingsPackage *package){
        bool is_active = package->intValue == backup_int_value ? true : false;
        item->intValue = backup_int_value;
        if( is_active ){
        //if( package->boolValue ){
            item->boolValue = true;
            item->is_active = true;
            item->touch_enabled = false;
        } else {
            item->boolValue = false;
            item->is_active = false;
            item->touch_enabled = true;
        }
    });
};






const std::function<void(PhantomWidgetItem *item)> load_pwm_enabled = [](PhantomWidgetItem *item){

    param_load( item, [](PhantomWidgetItem *item,PhantomSettingsPackage *package){

        bool changed = item->is_active == package->boolValue ? false : true;
        
        if( package->boolValue ){
            item->boolValue = true;
            item->is_active = true;
            item->set_string_value( BTN_STOP );
        } else {
            item->boolValue = false;
            item->is_active = false;
            item->set_string_value( BTN_START );
        }

        item->changed = changed;

        if( changed ){
            if( !item->is_active ){
                flush_scope   = true;
                ui.deep_check = true;
            }
        }

    });

};














// Helper function
void PhantomUI::emit_param_set(int param_id, const char* value) {
    std::pair<int, const char*> key_value_pair = std::make_pair(param_id, value);
    ui_controller.emit(PARAM_SET, &key_value_pair);
};


//###########################################################################################
// Open the alphanumeric keybard, it is specific for sd setting files in the class callbacks
// Need to change this #todo
//###########################################################################################
bool build_alpha_keyboard( std::string str = "" ) {
    auto page = ui.addPage( ALPHA_KEYBOARD, 1 ); // page for numeric keyboard
    std::shared_ptr<PhantomWidget> alpha_keyboard = factory_akeyboard( 320, 240, 0, 0, 2, false );
    auto __this = std::dynamic_pointer_cast<PhantomKeyboardAlpha>( alpha_keyboard );
    __this->set_str_value( str.c_str() );
    __this->create();
    page->make_temporary();
    page->add_widget( alpha_keyboard );
    ui.show_page( ALPHA_KEYBOARD );
    page = nullptr;
    alpha_keyboard = nullptr;
    __this = nullptr;
    return true;
};






const std::function<void(PhantomWidgetItem *item)> navigation_callback = [](PhantomWidgetItem *item) {
    // open keyboard for float value with no default value
    // submit action command with axis or ?
    item->has_touch = false;
    std::string tooltip;
    int  axis   = axes_active.x.index;
    switch( item->param_id ){
        case PARAM_ID_XLEFT:
            axis    = axes_active.x.index;
        break;
        case PARAM_ID_XRIGHT:
            axis    = axes_active.x.index;
        break;
        case PARAM_ID_YUP:
            axis    = axes_active.y.index;
        break;
        case PARAM_ID_YDOWN:
            axis    = axes_active.y.index;
        break;
        case PARAM_ID_ZUP:
            axis    = axes_active.z.index;
        break;
        case PARAM_ID_ZDOWN:
            axis    = axes_active.z.index;
        break;
        default: // wire spindle
            axis = -1;
        break;
    }
    item->floatValue = 0.0; // reset
    item->intValue   = axis;

    PhantomSettingsPackage package;
    package.axis       = item->intValue;
    package.intValue   = item->intValue;
    package.floatValue = item->floatValue;
    package.boolValue  = item->boolValue;
    package.param_id   = item->param_id;
    package.type       = 1;// float input
    uint8_t param_type = item->valueType;


    build_keyboard( &package, true, param_type );

};
























void add_std_list_title( std::shared_ptr<PhantomWidget> &container, const char* label ){
    container->addItem( OUTPUT_TYPE_TITLE, label, PARAM_TYPE_NONE, 0, 0, 0, 0, 0, false, DEFAULT_FONT )
             ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
}
void add_std_list_item( 
    std::shared_ptr<PhantomWidget> &container, int out_type, const char* label, param_types ptype, setget_param_enum param_id, uint8_t suffix, 
    bool require_dpm = false 

){
    PhantomWidgetItem *item = container->addItem( out_type, label, ptype, param_id, 0, 0, 0, 0, true, DEFAULT_FONT )
             ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(suffix);
    item->on(EVENT_LOAD,load).on(TOUCHED,change_setting);
    if( require_dpm ){
        item->set_is_refreshable()->set_require_dpm()->on(CHECK_REQUIRES,check_requirements);
    }
    item = nullptr;
}
void add_std_list_item_bool( 
    std::shared_ptr<PhantomWidget> &container, const char* label, setget_param_enum param_id, 
    bool require_dpm = false,
    bool require_motion = false
){
    PhantomWidgetItem *item = container->addItem( OUTPUT_TYPE_BOOL, label, PARAM_TYPE_BOOL, param_id, 0, 0, 0, 0, true, DEFAULT_FONT )
                  ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE);
    item->on(EVENT_LOAD,load).on(TOUCHED,change_setting);

    if( require_dpm ){
        item->set_require_dpm();
    }

    if( require_motion ){
        item->set_require_motion();
    }

    if( require_dpm || require_motion ){
        item->set_is_refreshable()->on(CHECK_REQUIRES,check_requirements);
    }

    item = nullptr;
}

void create_settings_page(){

    int num_tabs = 11;
    #ifdef ENABLE_SERVO42C_TORQUE_MOTOR
        ++num_tabs;
    #endif

    int current_tab = 0; // settings pages
    auto page       = ui.addPage( PAGE_MAIN_SETTINGS, num_tabs );
    // basic settings
    std::shared_ptr<PhantomWidget> settings_1_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_1_ptr );

    add_std_list_title( settings_1_ptr, DICT_BASIC_PAGE_TITLE );
    add_std_list_item( settings_1_ptr, OUTPUT_TYPE_STRING, DICT_FREQUENCY,    PARAM_TYPE_FLOAT, PARAM_ID_FREQ,         1 );
    add_std_list_item( settings_1_ptr, OUTPUT_TYPE_STRING, DICT_DUTY,         PARAM_TYPE_FLOAT, PARAM_ID_DUTY,         2 );
    add_std_list_item( settings_1_ptr, OUTPUT_TYPE_STRING, DICT_SETPOINT_MIN, PARAM_TYPE_FLOAT, PARAM_ID_SETMIN,       2 );
    add_std_list_item( settings_1_ptr, OUTPUT_TYPE_STRING, DICT_SETPOINT_MAX, PARAM_TYPE_FLOAT, PARAM_ID_SETMAX,       2 );
    add_std_list_item( settings_1_ptr, OUTPUT_TYPE_STRING, DICT_VDROP_THRESH, PARAM_TYPE_INT,   PARAM_ID_VDROP_THRESH, 8 );
    add_std_list_item( settings_1_ptr, OUTPUT_TYPE_STRING, DICT_MAX_FEED,     PARAM_TYPE_FLOAT, PARAM_ID_MAX_FEED,     4 );

    // forward blocker
    ++current_tab;
    page->select_tab( current_tab );
    std::shared_ptr<PhantomWidget> settings_9_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_9_ptr );
    add_std_list_title( settings_9_ptr, DICT_PAGE_FBLOCK );
    add_std_list_item( settings_9_ptr, OUTPUT_TYPE_INT,    DICT_VFD_THRESH, PARAM_TYPE_INT, PARAM_ID_VFD_THRESH,  8 );
    add_std_list_item( settings_9_ptr, OUTPUT_TYPE_STRING, DICT_ETHRESH,    PARAM_TYPE_INT, PARAM_ID_EDGE_THRESH, 8 );
    add_std_list_item( settings_9_ptr, OUTPUT_TYPE_INT,    DICT_LINE_ENDS,  PARAM_TYPE_INT, PARAM_ID_LINE_ENDS,   0 );

    // protection settings
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_8_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_8_ptr );
    add_std_list_title( settings_8_ptr, DICT_PAGE_PROT );
    add_std_list_item( settings_8_ptr, OUTPUT_TYPE_STRING, DICT_SHORT_DURATION, PARAM_TYPE_INT,   PARAM_ID_SHORT_DURATION,  13 );
    add_std_list_item( settings_8_ptr, OUTPUT_TYPE_STRING, DICT_BROKEN_WIRE_MM, PARAM_TYPE_FLOAT, PARAM_ID_BROKEN_WIRE_MM,  3 );
    add_std_list_item( settings_8_ptr, OUTPUT_TYPE_INT,    DICT_POFF_DURATION,  PARAM_TYPE_INT,   PARAM_ID_POFF_DURATION,   0 );

    // retraction settings
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_6_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_6_ptr );
    add_std_list_title( settings_6_ptr, DICT_PAGE_R );
    add_std_list_item_bool( settings_6_ptr, DICT_EARLY_RETR_X, PARAM_ID_EARLY_RETR );
    add_std_list_item( settings_6_ptr, OUTPUT_TYPE_INT,    DICT_EARLY_X_ON,      PARAM_TYPE_INT, PARAM_ID_EARLY_X_ON,     0 );
    add_std_list_item( settings_6_ptr, OUTPUT_TYPE_INT,    DICT_RETR_CONFIRM,    PARAM_TYPE_INT, PARAM_ID_RETRACT_CONF,   0 );
    add_std_list_item( settings_6_ptr, OUTPUT_TYPE_STRING, DICT_MAX_REVERSE,     PARAM_TYPE_INT, PARAM_ID_MAX_REVERSE,    7 );

    // Feedback settings
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_3_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_3_ptr );
    add_std_list_title( settings_3_ptr, DICT_PAGE_FEEDBACK );
    add_std_list_item_bool( settings_3_ptr, DICT_SHOW_VFD, PARAM_ID_SCOPE_SHOW_VFD );
    add_std_list_item_bool( settings_3_ptr, DICT_VIEW_HIGH_RES, PARAM_ID_USE_HIGH_RES_SCOPE );
    add_std_list_item( settings_3_ptr, OUTPUT_TYPE_INT, DICT_MAIN_AVG,   PARAM_TYPE_INT, PARAM_ID_FAST_CFD_AVG_SIZE, 0 );
    add_std_list_item( settings_3_ptr, OUTPUT_TYPE_INT, DICT_FAVG_SIZE,  PARAM_TYPE_INT, PARAM_ID_SLOW_CFD_AVG_SIZE, 0 );


    // I2S settings
    ++current_tab;
    page->select_tab( current_tab );
    std::shared_ptr<PhantomWidget> settings_5_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_5_ptr );
    add_std_list_title( settings_5_ptr, DICT_I2S_PAGE_TITLE );
    add_std_list_item_bool( settings_5_ptr, DICT_VIEW_HIGH_RES, PARAM_ID_USE_HIGH_RES_SCOPE );
    add_std_list_item( settings_5_ptr, OUTPUT_TYPE_STRING, DICT_I2S_RATE,     PARAM_TYPE_FLOAT, PARAM_ID_I2S_RATE,     6 );
    add_std_list_item( settings_5_ptr, OUTPUT_TYPE_INT,    DICT_I2S_BUFFER_L, PARAM_TYPE_INT,   PARAM_ID_I2S_BUFFER_L, 0 );
    add_std_list_item( settings_5_ptr, OUTPUT_TYPE_INT,    DICT_I2S_BUFFER_C, PARAM_TYPE_INT,   PARAM_ID_I2S_BUFFER_C, 0 );

    // retraction speeds and distances
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_7_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_7_ptr );
    add_std_list_title( settings_7_ptr, DICT_PAGE_RS );
    settings_7_ptr->addItem(OUTPUT_TYPE_STRING,DICT_RETR_HARD_MM,PARAM_TYPE_FLOAT,PARAM_ID_RETRACT_H_MM,0,0,0,0,true,DEFAULT_FONT)
            ->set_float_decimals(3)->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(3) //3=mm
            ->on(EVENT_LOAD,load).on(TOUCHED,change_setting);
    add_std_list_item( settings_7_ptr, OUTPUT_TYPE_STRING, DICT_RETR_HARD_F, PARAM_TYPE_FLOAT, PARAM_ID_RETRACT_H_F, 4 );
    settings_7_ptr->addItem(OUTPUT_TYPE_STRING,DICT_RETR_SOFT_MM,PARAM_TYPE_FLOAT,PARAM_ID_RETRACT_S_MM,0,0,0,0,true,DEFAULT_FONT)
            ->set_float_decimals(3)->set_suffix(3) //3=mm
            ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(EVENT_LOAD,load).on(TOUCHED,change_setting);
    add_std_list_item( settings_7_ptr, OUTPUT_TYPE_STRING, DICT_RETR_SOFT_F, PARAM_TYPE_FLOAT, PARAM_ID_RETRACT_S_F, 4 );


    // DPM main
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_11_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_11_ptr );
    add_std_list_title( settings_11_ptr, DICT_DPM_PAGE_TITLE );
    settings_11_ptr->addItem(OUTPUT_TYPE_BOOL,DICT_DPM_UART,PARAM_TYPE_BOOL_RELOAD_WIDGET_OC,PARAM_ID_DPM_UART,0,0,0,0,true,DEFAULT_FONT)
           ->set_is_refreshable()->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(TOUCHED,change_setting).on(EVENT_LOAD,load);
    add_std_list_item_bool( settings_11_ptr, DICT_DPM_ONOFF, PARAM_ID_DPM_ONOFF, true );
    add_std_list_item( settings_11_ptr, OUTPUT_TYPE_STRING, DICT_DPM_VOLTAGE, PARAM_TYPE_FLOAT, PARAM_ID_DPM_VOLTAGE, 10, true );
    add_std_list_item( settings_11_ptr, OUTPUT_TYPE_STRING, DICT_DPM_CURRENT, PARAM_TYPE_FLOAT, PARAM_ID_DPM_CURRENT, 11, true );
    add_std_list_item( settings_11_ptr, OUTPUT_TYPE_STRING, DICT_DPM_PROBE_C, PARAM_TYPE_FLOAT, PARAM_ID_DPM_PROBE_C, 11, true );
    add_std_list_item( settings_11_ptr, OUTPUT_TYPE_STRING, DICT_DPM_PROBE_V, PARAM_TYPE_FLOAT, PARAM_ID_DPM_PROBE_V, 10, true );


    //#########################################################################################
    // Wire pulling motor settings
    //#########################################################################################
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_4_ptr = factory_split_list( 320, 150, 0, 0, 2, false );
    page->add_widget( settings_4_ptr );
    settings_4_ptr->set_is_refreshable();

    add_std_list_title( settings_4_ptr, DICT_SPINDLE_PAGE_TITLE );                          // tab title
    add_std_list_item_bool( settings_4_ptr, DICT_SPINDLE_ONOFF,  PARAM_ID_SPINDLE_ONOFF, false, true );  // Start/Stop button
    add_std_list_item_bool( settings_4_ptr, DICT_SPINDLE_ENABLE, PARAM_ID_SPINDLE_ENABLE ); // Enable automatic spindle controls
    add_std_list_item_bool( settings_4_ptr, DICT_SPINDLE_DIR_INVERTED, PARAM_ID_PULLING_MOTOR_DIR_INVERTED ); // change dir
    add_std_list_item( settings_4_ptr, OUTPUT_TYPE_STRING, DICT_SPINDLE_SPEED_B, PARAM_TYPE_FLOAT, PARAM_ID_SPINDLE_SPEED,        4 );
    add_std_list_item( settings_4_ptr, OUTPUT_TYPE_STRING, DICT_SPINDLE_SPM,     PARAM_TYPE_INT,   PARAM_ID_SPINDLE_STEPS_PER_MM, 14 );
    /*std::shared_ptr<PhantomWidget> spindle_move_btns = factory_button( 30, 60, 20, 150, 2, true );
    int w = 30, h = 30;
    spindle_move_btns->disable_background_color();
    spindle_move_btns->addItem(OUTPUT_TYPE_NONE,NAV_UP, PARAM_TYPE_FLOAT,PARAM_ID_SPINDLE_MOVE_UP,w,h,0,0,true,-1)
                           ->set_colors( 1, -1, TFT_GREEN )->set_colors( 3, -1, BUTTON_LIST_BG_DEACTIVATED )
                           ->set_require_motion()->set_is_refreshable()
                           ->on(TOUCHED,navigation_callback).on(CHECK_REQUIRES,check_requirements);
    spindle_move_btns->addItem(OUTPUT_TYPE_NONE,NAV_DOWN, PARAM_TYPE_FLOAT,PARAM_ID_SPINDLE_MOVE_DOWN,w,h,0,30,true,-1)
                           ->set_colors( 1, -1, TFT_GREEN )->set_colors( 3, -1, BUTTON_LIST_BG_DEACTIVATED )
                           ->set_require_motion()->set_is_refreshable()
                           ->on(TOUCHED,navigation_callback).on(CHECK_REQUIRES,check_requirements);
    spindle_move_btns->set_is_refreshable();
    page->add_widget( spindle_move_btns );*/ // add widget to active tab; default active tab is the first one







    // Flushing settings
    ++current_tab;
    page->select_tab( current_tab );
    std::shared_ptr<PhantomWidget> settings_f_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_f_ptr );
    add_std_list_title( settings_f_ptr, DICT_PAGE_FLUSHING );
    add_std_list_item_bool( settings_f_ptr, DICT_FLUSH_NOSPARK, PARAM_ID_FLUSHING_FLUSH_NOSPRK );
    add_std_list_item( settings_f_ptr, OUTPUT_TYPE_STRING, DICT_FLUSH_DIST,   PARAM_TYPE_FLOAT, PARAM_ID_FLUSHING_DISTANCE,     3 );
    add_std_list_item( settings_f_ptr, OUTPUT_TYPE_STRING, DICT_FLUSH_INTV,   PARAM_TYPE_FLOAT, PARAM_ID_FLUSHING_INTERVAL,     12 );
    add_std_list_item( settings_f_ptr, OUTPUT_TYPE_INT,    DICT_FLUSH_OFFSTP, PARAM_TYPE_INT,   PARAM_ID_FLUSHING_FLUSH_OFFSTP, 0 );


    // Probing settings
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_13_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_13_ptr );
    add_std_list_title( settings_13_ptr, DICT_PAGE_PROBING );
    add_std_list_item( settings_13_ptr, OUTPUT_TYPE_STRING, DICT_PROBE_FREQ,    PARAM_TYPE_FLOAT, PARAM_ID_PROBE_FREQ, 1 );
    add_std_list_item( settings_13_ptr, OUTPUT_TYPE_STRING, DICT_PROBE_DUTY,    PARAM_TYPE_FLOAT, PARAM_ID_PROBE_DUTY, 2 );
    add_std_list_item( settings_13_ptr, OUTPUT_TYPE_STRING, DICT_PROBE_TR_V,    PARAM_TYPE_INT,   PARAM_ID_PROBE_TR_V, 8 );
    add_std_list_item( settings_13_ptr, OUTPUT_TYPE_STRING, DICT_PROBE_TR_C,    PARAM_TYPE_FLOAT, PARAM_ID_PROBE_TR_C, 2 );
    add_std_list_item( settings_13_ptr, OUTPUT_TYPE_STRING, DICT_SPINDLE_SPEED, PARAM_TYPE_FLOAT, PARAM_ID_SPINDLE_SPEED_PROBING, 4 );

    #ifdef ENABLE_SERVO42C_TORQUE_MOTOR
        // MKS SERVO42C settings only if enabled

        ++current_tab;
        page->select_tab( current_tab ); 
        std::shared_ptr<PhantomWidget> settings_14_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
        page->add_widget( settings_14_ptr );

        add_std_list_title( settings_14_ptr, DICT_PAGE_MKS );
        add_std_list_item( settings_14_ptr, OUTPUT_TYPE_STRING, DICT_MKS_MAX_TORQUE,  PARAM_TYPE_INT, PARAM_ID_MKS_MAXT, 15 );
        add_std_list_item( settings_14_ptr, OUTPUT_TYPE_STRING, DICT_MKS_MAX_CURRENT, PARAM_TYPE_INT, PARAM_ID_MKS_MAXC, 16 );
        std::shared_ptr<PhantomWidget> mks_move_btns = factory_button( 30, 60, 20, 150, 2, true );
        w = 30;
        h = 30;
        mks_move_btns->disable_background_color();
        mks_move_btns->addItem(OUTPUT_TYPE_NONE,NAV_UP, PARAM_TYPE_FLOAT,PARAM_ID_MKS_CCW,w,h,0,0,true,-1)
                     ->set_colors( 1, -1, TFT_GREEN )->set_colors( 3, -1, BUTTON_LIST_BG_DEACTIVATED )->set_require_motion()->set_is_refreshable()
                     ->on(TOUCHED,navigation_callback).on(CHECK_REQUIRES,check_requirements);
        mks_move_btns->addItem(OUTPUT_TYPE_NONE,NAV_DOWN, PARAM_TYPE_FLOAT,PARAM_ID_MKS_CW,w,h,0,30,true,-1)
                     ->set_colors( 1, -1, TFT_GREEN )->set_colors( 3, -1, BUTTON_LIST_BG_DEACTIVATED )
                     ->set_require_motion()->set_is_refreshable()
                     ->on(TOUCHED,navigation_callback).on(CHECK_REQUIRES,check_requirements);
        mks_move_btns->set_is_refreshable();
        page->add_widget( mks_move_btns ); // add widget to active tab; default active tab is the first one

    #endif







    page->add_nav( 1 );  // add navigation button for tab selection
    page->select_tab( ui.get_active_tab_for_page( PAGE_MAIN_SETTINGS ), true ); // select first tab of this page again 
    page->make_temporary();

    page = nullptr;
    
}




void create_motion_page(){

    auto page = ui.addPage( PAGE_MOTION, 4 ); 
    page->set_bg_color( PAGE_TAB_BG );
    int current_tab = 0, w = 40, h = 40;
    int16_t grid_w = 3, grid_h = 3, cc_y = 0, btw_h = 32;
    int16_t r_w = 320-(w*grid_w+grid_w);
    // create widgets

    for( int i = 0; i < N_AXIS; ++i ){

        std::shared_ptr<PhantomWidget> container = factory_button( r_w, btw_h, w*grid_w+grid_w+10, 40+cc_y, 2, true );
       int16_t hcw  = round( float( r_w ) / 2.0 );
       int16_t hhcw = round( float( hcw ) / 2.0 );

       container->addItem(OUTPUT_TYPE_STRING,get_axis_name_const(i),PARAM_TYPE_FLOAT,PARAM_ID_GET_PROBE_POS,88,btw_h,0,0,false,MINI_FONT)
                ->set_int_value( i ) // show current probe pos
                ->set_colors(0, BUTTON_LIST_BG, TFT_WHITE )->set_colors(1, BUTTON_LIST_BG, TFT_WHITE )->set_colors(2, BUTTON_LIST_BG, TFT_WHITE )
                ->on(EVENT_LOAD,load).on(BUILDSTRING,set_probe_pos_string);

       container->addItem(OUTPUT_TYPE_NONE,"L",PARAM_TYPE_INT_RAW_RELOAD_WIDGET,PARAM_ID_UNSET_PROBE_POS,38,btw_h,89,0,true,-1)
                ->set_int_value( i ) // set axis probe pos to zero
                ->set_colors(0, TFT_MAROON, TFT_WHITE )->on(TOUCHED,change_setting);

       container->addItem(OUTPUT_TYPE_NONE,"Set",PARAM_TYPE_INT_RAW_RELOAD_WIDGET,PARAM_ID_SET_PROBE_POS,48,btw_h,128,0,true,DEFAULT_FONT)
                ->set_int_value( i ) // set axis probe pos to current
                ->set_colors(0, COLORGREENY, TFT_BLACK )->on(TOUCHED,change_setting);

       cc_y += btw_h+1;
       page->add_widget( container );

    }

    std::shared_ptr<PhantomWidget> ph_btn5 = factory_button( w*grid_w+grid_w, h*grid_h+grid_h, 5, 40, 2, true );
    ph_btn5->set_is_refreshable();
    int h_h = 0, c_y = 0, c_x = 0;
    int icon_color_default    = TFT_WHITE;
    int icon_bg_color_default = BUTTON_LIST_BG;
    for(int i = 0; i < grid_h*grid_w; ++i){
       switch (i){
           case 0:
           case 2:
           case 6:
           case 8:  icon_color_default = TFT_DARKGREY; icon_bg_color_default = BUTTON_LIST_BG; break;
           case 4:  icon_color_default = TFT_WHITE;    icon_bg_color_default = BUTTON_LIST_BG; break;
           default: icon_color_default = TFT_WHITE;    icon_bg_color_default = COLOR333333;    break;
       }
       ph_btn5->addItem(OUTPUT_TYPE_NONE,icons[i],PARAM_TYPE_INT_RAW_RELOAD_WIDGET,PARAM_ID_PROBE_ACTION,w,h,c_x,c_y,true,-1)
                   ->set_int_value( i )->set_is_refreshable()->set_require_motion()->set_colors(0,icon_bg_color_default,icon_color_default)
                   ->on(CHECK_REQUIRES,check_requirements).on(TOUCHED,change_setting);
       c_x += w+1;
       if( ++h_h == grid_h ){ // next row
              c_y += h+1;
              c_x  = 0;
              h_h  = 0;
       }
    }
    page->add_widget( ph_btn5 );


    std::shared_ptr<PhantomWidget> ph_btn6 = factory_button( ph_btn5->width-1, 28, 5, ph_btn5->height+50, 2, true );
    ph_btn6->set_is_refreshable();
    ph_btn6->addItem(OUTPUT_TYPE_NONE,"Move to 0,0",PARAM_TYPE_NONE,PARAM_ID_MOVE_TO_ORIGIN,ph_btn5->width,28,0,0,true,DEFAULT_FONT)
             ->set_require_motion()->set_is_refreshable()->on(TOUCHED,change_setting).on(CHECK_REQUIRES,check_requirements);
    page->add_widget( ph_btn6 );


    ++current_tab; // next tab
    page->select_tab(current_tab); // select tab
    c_y   = 0;
    h     = 32;
    r_w   = 202;
    cc_y  = 40;
    std::shared_ptr<PhantomWidget> container[N_AXIS];
    for( int i = 0; i < N_AXIS; ++i ){
        container[i] = factory_button( r_w, h, 113, cc_y, 2, true );
        container[i]->set_is_refreshable();
        container[i]->addItem(OUTPUT_TYPE_NONE,get_axis_name_const(i),PARAM_TYPE_NONE,PARAM_ID_HOME_AXIS,40,h,0,0,false,DEFAULT_FONT)
                    ->set_colors(0, BUTTON_LIST_BG, TFT_WHITE )->set_colors(1, BUTTON_LIST_BG, TFT_WHITE )->set_colors(2, BUTTON_LIST_BG, TFT_WHITE );
        container[i]->addItem(OUTPUT_TYPE_NONE,"N",PARAM_TYPE_NONE,PARAM_ID_HOME_AXIS,40,h,40,0,false,-1)
                    ->set_is_refreshable()->set_int_value( i ) ->set_colors(0, BUTTON_LIST_BG, COLORORANGERED )->set_colors(1, BUTTON_LIST_BG, COLORGREENY )->set_colors(2, BUTTON_LIST_BG, COLORGREENY )                
                    ->on(EVENT_LOAD,get_homed_state);
        container[i]->addItem(OUTPUT_TYPE_STRING,"",PARAM_TYPE_INT_RAW_RELOAD_WIDGET,PARAM_ID_HOME_ENA,60,h,81,0,true,DEFAULT_FONT)
                    ->set_int_value( i ) ->set_colors(0, BUTTON_LIST_BG, COLORORANGERED )->set_colors(2, BUTTON_LIST_BG, COLORGREENY )                
                    ->on(EVENT_LOAD,get_homing_enabled).on(TOUCHED,change_setting);
        container[i]->addItem(OUTPUT_TYPE_NONE,"5",PARAM_TYPE_INT_RAW_RELOAD_WIDGET,PARAM_ID_HOME_SEQ,60,h,142,0,true,-1)
                    ->set_int_value( i )->set_is_refreshable()->set_colors(1, COLORORANGERED, TFT_BLACK )->set_colors(2, COLORORANGERED, TFT_BLACK )->set_colors(3, COLORORANGERED, TFT_BLACK )
                    ->set_require_motion()->set_colors(0, COLORGREENY, TFT_BLACK )
                    ->on(EVENT_LOAD,get_homing_enabled_2).on(TOUCHED,change_setting).on(CHECK_REQUIRES,check_requirements);
       page->add_widget( container[i] );
       cc_y += h+1;
    }


    int hall_w = 80, hall_x = 10;
    int hall_y = ph_btn5->height+50;//240-30-5-h;

    std::shared_ptr<PhantomWidget> ph_btn7 = factory_button( hall_w, 28, hall_x, hall_y, 2, true );
    ph_btn7->set_is_refreshable();
    ph_btn7->addItem(OUTPUT_TYPE_NONE,"Home All",PARAM_TYPE_NONE_REDRAW_PAGE,PARAM_ID_HOME_ALL,hall_w,28,0,0,true,DEFAULT_FONT)
             ->set_int_value( -1 )->set_colors(0, COLORGREENY, TFT_BLACK )->set_require_motion()->set_is_refreshable()
             ->on(TOUCHED,change_setting).on(CHECK_REQUIRES,check_requirements);
    page->add_widget( ph_btn7 );

    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> speed_settings = factory_split_list( 320, 160, 0, 45, 2, false );
    page->add_widget( speed_settings );

    add_std_list_item( speed_settings, OUTPUT_TYPE_STRING, "Rapid (mm/min)",            PARAM_TYPE_FLOAT, PARAM_ID_SPEED_RAPID,   4 );
    add_std_list_item( speed_settings, OUTPUT_TYPE_STRING, "EDM Feed Max (mm/min)",     PARAM_TYPE_FLOAT, PARAM_ID_MAX_FEED,      4 );
    add_std_list_item( speed_settings, OUTPUT_TYPE_STRING, "EDM Feed Initial (mm/min)", PARAM_TYPE_FLOAT, PARAM_ID_SPEED_INITIAL, 4 );

    // motor settings
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_12_ptr = factory_split_list( 320, 190, 0, 45, 2, false );
    page->add_widget( settings_12_ptr );

    setget_param_enum param_id;

    for( int i = 0; i < N_AXIS; ++i ){
        const char* text;
        if( is_axis( "X", i ) ){
            param_id = PARAM_ID_X_RES; text = DICT_X_RES;
        } else if( is_axis( "Y", i ) ){
            param_id = PARAM_ID_Y_RES; text = DICT_Y_RES;
        } else if( is_axis( "Z", i ) ){
            param_id = PARAM_ID_Z_RES; text = DICT_Z_RES;
        } else if( is_axis("U", i) ){
            param_id = PARAM_ID_U_RES; text = DICT_U_RES;
        } else if( is_axis("V", i) ){
            param_id = PARAM_ID_V_RES; text = DICT_V_RES;
        } else { continue; }

        add_std_list_item( settings_12_ptr, OUTPUT_TYPE_FLOAT, text, PARAM_TYPE_FLOAT, param_id, 0 );
    }

    page->add_tab_item(0,DICT_PROBE_TAB);
    page->add_tab_item(1,DICT_HOMING_TAB);
    page->add_tab_item(2,"Speeds");
    page->add_tab_item(3,"Steps");

    page->add_nav( 2 ); 
    page->select_tab( ui.get_active_tab_for_page( PAGE_MOTION ), true ); // select first tab of this page again 
    page->make_temporary();

    page = nullptr;

}





void create_mode_page(){

    int tab_num = 2;//axes_active.z.enabled ? 3 : 2;

    auto page = ui.addPage( PAGE_MODES, tab_num ); // create new page with x tabs and index for mode (PAGE_MODE)
    int current_tab = 0;
    std::shared_ptr<PhantomWidget> ph_btn3 = factory_button( 300, 30, 10, 5, 2, true );
    int btn_w = 300/tab_num;
    ph_btn3->addItem(OUTPUT_TYPE_NONE,DICT_MODE1, PARAM_TYPE_INT_RAW_RELOAD_WIDGET,PARAM_ID_MODE,btn_w,30,0,0,true,DEFAULT_FONT)
                 ->set_outlines( 1 )->set_int_value( MODE_SINKER )->on(EVENT_LOAD,load_mode_custom).on(TOUCHED,change_setting);
    ph_btn3->addItem(OUTPUT_TYPE_NONE,DICT_MODE4, PARAM_TYPE_INT_RAW_RELOAD_WIDGET,PARAM_ID_MODE,btn_w,30,btn_w,0,true,DEFAULT_FONT)
                 ->set_outlines( 1 )->set_int_value( MODE_2D_WIRE )->on(EVENT_LOAD,load_mode_custom).on(TOUCHED,change_setting);

    if( tab_num == 3 ){
        ph_btn3->addItem(OUTPUT_TYPE_NONE,DICT_MODE2, PARAM_TYPE_INT_RAW_RELOAD_WIDGET,PARAM_ID_MODE,btn_w,30,btn_w*2,0,true,DEFAULT_FONT)
               ->set_outlines( 1 )->set_int_value( MODE_REAMER )->on(EVENT_LOAD,load_mode_custom).on(TOUCHED,change_setting);
    }

    // tab 1 = sinker specific
    // tab 2 = wire
    // tab 3 = reamer


    // tab 1 - mode settings sinker
    std::shared_ptr<PhantomWidget> ph_sli = factory_split_list( 320, 165, 0, 40, 2, false );
    ph_sli->addItem(OUTPUT_TYPE_TITLE,DICT_PAGE_MODE_S,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
                        ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
    ph_sli->addItem(OUTPUT_TYPE_BOOL,DICT_SIMULATION,PARAM_TYPE_BOOL,PARAM_ID_SIMULATION,0,0,0,0,true,DEFAULT_FONT)
           ->on(EVENT_LOAD,load).on(TOUCHED,change_setting); 
    ph_sli->addItem(OUTPUT_TYPE_STRING,DICT_SINK_DISTANCE,PARAM_TYPE_FLOAT,PARAM_ID_SINK_DIST,0,0,0,0,true,DEFAULT_FONT)
                        ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(3) //3=mm;
                        ->on(EVENT_LOAD,load).on(TOUCHED,change_setting); 
    ph_sli->addItem(OUTPUT_TYPE_CUSTOM,DICT_SINK_DIR,PARAM_TYPE_NONE,PARAM_ID_SINK_DIR,0,0,0,0,true,DEFAULT_FONT)
                        ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(EVENT_LOAD,load).on(BUILDSTRING,sinker_dir_lambda).on(TOUCHED,change_setting); 



    page->add_widget( ph_sli ); // add widget to pages active tab
    page->add_widget( ph_btn3 ); // mode buttons on all tabs


    // tab 2 - mode settings 2d wire
    ++current_tab; // next tab
    std::shared_ptr<PhantomWidget> ph_sli2 = factory_split_list( 320, 165, 0, 40, 2, false );
    page->select_tab(current_tab); // select tab
    ph_sli2->addItem(OUTPUT_TYPE_TITLE,DICT_PAGE_MODE_W,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
           ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
    ph_sli2->addItem(OUTPUT_TYPE_BOOL,DICT_SIMULATION,PARAM_TYPE_BOOL,PARAM_ID_SIMULATION,0,0,0,0,true,DEFAULT_FONT)
           ->on(EVENT_LOAD,load).on(TOUCHED,change_setting); 
    ph_sli2->addItem(OUTPUT_TYPE_STRING,DICT_TOOLDIA,PARAM_TYPE_FLOAT,PARAM_ID_TOOLDIAMETER,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(3) //3=mm;
           ->on(EVENT_LOAD,load).on(TOUCHED,change_setting);


    page->add_widget( ph_sli2 );
    page->add_widget( ph_btn3 ); // mode buttons on all tabs

    if( tab_num == 3 ){

        // tab 3 - mode settings reamer
        ++current_tab; // next tab
        std::shared_ptr<PhantomWidget> ph_sli3 = factory_split_list( 320, 165, 0, 40, 2, false );
        page->select_tab(current_tab); // select tab
        ph_sli3->addItem(OUTPUT_TYPE_TITLE,DICT_PAGE_MODE_R,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
                        ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
        ph_sli3->addItem(OUTPUT_TYPE_STRING,DICT_REAMER_DIS,PARAM_TYPE_FLOAT,PARAM_ID_REAMER_DIST,0,0,0,0,true,DEFAULT_FONT)
                        ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(3) //3=mm;
                        ->on(EVENT_LOAD,load).on(TOUCHED,change_setting); 
        ph_sli3->addItem(OUTPUT_TYPE_STRING,DICT_REAMER_DUR,PARAM_TYPE_FLOAT,PARAM_ID_REAMER_DURA,0,0,0,0,true,DEFAULT_FONT)
                        ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(12) //12=s;
                        ->on(EVENT_LOAD,load).on(TOUCHED,change_setting);
        page->add_widget( ph_sli3 );
        page->add_widget( ph_btn3 ); // mode buttons on all tabs

    }


    page->add_nav( 1 );   
    page->select_tab( ui.get_active_tab_for_page( PAGE_MODES ), true ); // select first tab of this page again 
    page->make_temporary();

    page = nullptr;

}




// direct access to the state..... 
const std::function<void(PhantomWidgetItem *item)> load_state = [](PhantomWidgetItem *item){
    int state = (int) get_machine_state();
    if( item->intValue == state ) return;
    switch( state ){
        case STATE_IDLE: item->set_colors( 0, TFT_BLACK, TFT_GREEN );       break;
        case STATE_BUSY: item->set_colors( 0, TFT_BLACK, TFT_GREENYELLOW ); break;
        default:         item->set_colors( 0, TFT_BLACK, COLORORANGERED );  break;
    }
    if( is_system_mode_edm() ){ // for debugging
        item->set_colors( 0, TFT_BLACK, TFT_MAROON );
    }
    std::string text = state_labels[ machine_states( state ) ];
    item->set_string_value( text );
    item->changed = true;
};

const std::function<void(PhantomWidgetItem *item)> set_gcode_name_lambda = [](PhantomWidgetItem *item){
    if( ui.gcode_file == "" ){
        item->set_string_value( "None" );
    }
};

void create_nav_button( std::shared_ptr<PhantomWidget> &container, const char* label, int16_t w, int16_t h, int16_t x, int16_t y, setget_param_enum param_id, bool enabled = true ){
    if( enabled ){
        container->addItem( OUTPUT_TYPE_NONE, label, PARAM_TYPE_FLOAT, param_id, w, h, x, y, true, -1 )
        ->set_radius(5)->set_colors( 0, COLORRAL6003, TFT_BLACK )->set_colors( 3, BUTTON_LIST_BG_DEACTIVATED, TFT_BLACK )
        ->set_require_motion()->set_is_refreshable()->on(TOUCHED,navigation_callback).on(CHECK_REQUIRES,check_requirements);
        return;
    }
    container->addItem( OUTPUT_TYPE_NONE, label, PARAM_TYPE_FLOAT, param_id, w, h, x, y, false, -1 )
    ->set_disabled( true )->set_radius(5)->set_colors( 0, COLORRAL6003, TFT_BLACK )->set_colors( 3, BUTTON_LIST_BG_DEACTIVATED, TFT_BLACK );
}

void add_std_text_label( std::shared_ptr<PhantomWidget> &container, const char* label, int w, int h, int x, int y ){
    container->addItem( OUTPUT_TYPE_CUSTOM, label, PARAM_TYPE_NONE, 0, w, h, x, y, false, MINI_FONT )->set_colors( 0, TFT_BLACK, TFT_GREEN );
}




void create_front_page(){

    int menu_height = 30;
    int menu_width  = 320;
    int xstart      = 50;
    int menu_item_w = (320-xstart)/4;

    //#################################################
    // Front page
    //#################################################
    auto page = ui.addPage( PAGE_FRONT, 1 ); // create page with only 1 tab
    std::shared_ptr<PhantomWidget> front_menu = factory_button( menu_width, menu_height, 0, 0, 1, true );

    int cindex = -1;
    front_menu->addItem(OUTPUT_TYPE_STRING,"",PARAM_TYPE_NONE,0,xstart,menu_height,0,0,false,DEFAULT_FONT)->set_int_value(-1)
              ->set_colors( 0, TFT_BLACK, TFT_GREEN )->set_string_value("")->set_is_refreshable()->on(EVENT_LOAD,load_state);

    front_menu->addItem(OUTPUT_TYPE_NONE,ICON_COGS,0,0,menu_item_w-1,menu_height,++cindex*menu_item_w+xstart,0,true,-1)
    ->set_int_value( PAGE_MAIN_SETTINGS )->on(TOUCHED,open_page);
    front_menu->addItem(OUTPUT_TYPE_NONE,MM_MODE, PARAM_TYPE_NONE,0,menu_item_w-1,menu_height,++cindex*menu_item_w+xstart,0,true,DEFAULT_FONT)
    ->set_int_value( PAGE_MODES )->on(TOUCHED,open_page);
    front_menu->addItem(OUTPUT_TYPE_NONE,MM_MOTION, PARAM_TYPE_NONE,0,menu_item_w-1,menu_height,++cindex*menu_item_w+xstart,0,true,DEFAULT_FONT)
    ->set_int_value(PAGE_MOTION)->on(TOUCHED,open_page);
    front_menu->addItem(OUTPUT_TYPE_NONE,MM_SD, PARAM_TYPE_NONE,0,menu_item_w-1,menu_height,++cindex*menu_item_w+xstart,0,true,DEFAULT_FONT)
    ->set_int_value( PAGE_SD_CARD )->set_require_sd()->set_is_refreshable()->on(TOUCHED,open_page).on(CHECK_REQUIRES,check_requirements);
    front_menu->set_is_refreshable();
    page->add_widget( front_menu );

    //##############################################
    // Basic info
    //##############################################
    std::shared_ptr<PhantomWidget> mdata = factory_list( 235, (4+tft.fontHeight(DEFAULT_FONT)), 5, 112, 1, true );
    mdata->addItem(OUTPUT_TYPE_CUSTOM,MM_MODE_B,PARAM_TYPE_INT,PARAM_ID_MODE,0,0,0,0,false,DEFAULT_FONT)
         ->set_colors( 0, -1, TFT_LIGHTGREY )->on(BUILDSTRING,mode_load).on(EVENT_LOAD,load);
    page->add_widget( mdata ); // add widget to active tab; default active tab is the first one



    std::shared_ptr<PhantomWidget> mpos  = nullptr;
    setget_param_enum param_id;
    int font = MINI_FONT;// N_AXIS > 4 ? MINI_FONT : DEFAULT_FONT;
    int odd_edven = 0;
    int mpos_pos_y = 115;
    int mpos_h     = (6+tft.fontHeight(font));
    int rows       = 0;
    int cells_max    = 0;
    int cell_current = 0;
    int cells = 2;
    for( int i = 0; i < N_AXIS; ++i ){
        if( is_axis("X", i) ){
            param_id = PARAM_ID_MPOSX;
        } else if( is_axis("Y", i) ){
            param_id = PARAM_ID_MPOSY;
        } else if( is_axis("Z", i) ){
            param_id = PARAM_ID_MPOSZ;
        } else if( is_axis("U", i) ){
            param_id = PARAM_ID_MPOSU;
        } else if( is_axis("V", i) ){
            param_id = PARAM_ID_MPOSV;
        } else {
            continue;
        }
        if( ++odd_edven > cells || mpos == nullptr ){
            ++rows;
            mpos = factory_list( 155, mpos_h, 320-155, mpos_pos_y, 1, true );
            mpos->set_is_refreshable();
            page->add_widget( mpos );
            mpos_pos_y += mpos_h;
            odd_edven = odd_edven > cells ? 1 : odd_edven;
            cell_current = 1;
        }
        if( ++cell_current > cells_max ){ cells_max = cell_current; }
        mpos->addItem(OUTPUT_TYPE_FLOAT,g_axis[i]->axis_name,PARAM_TYPE_FLOAT,param_id,0,0,0,0,false,font)->set_is_refreshable()->on(EVENT_LOAD,load);
    }
    if( cells_max > cell_current ){
        int missing = cells_max - cell_current;
        for( int i = 0; i < missing; ++i ){
            mpos->addItem(OUTPUT_TYPE_NONE,"",PARAM_TYPE_NONE,0,0,0,0,0,false,font);
        }
    }
    //##############################################
    // Navigation
    //##############################################
    int l = 3, w = 28, h = 30, t = 174, tw = 3*w+2*l, th = 2*h+l, mg = 5, r = 5, cur_x = mg;

    //##############################################
    // Combined Jog on/off Only for xyuv types
    //##############################################
    #if defined( CNC_TYPE_XYUV ) || defined( CNC_TYPE_XYUVZ )
        std::shared_ptr<PhantomWidget> XYUV_linked = factory_split_list( 155, h, mg, t-42, 2, true );
        auto __this = std::dynamic_pointer_cast<PhantomSplitList>( XYUV_linked );
        __this->set_column_left_width( 40 );
        __this->set_on_off_text( "Linked", "Normal" );
        page->add_widget( XYUV_linked );
        XYUV_linked->addItem(OUTPUT_TYPE_BOOL,"Jog",PARAM_TYPE_BOOL,PARAM_ID_XYUV_JOG_COMBINED,0,0,0,0,true,DEFAULT_FONT)->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(TOUCHED,change_setting).on(EVENT_LOAD,load);
        XYUV_linked->set_is_refreshable();
        page->add_widget( XYUV_linked );
    #else



    #endif

    //##############################################
    // Navigation buttons XY
    //##############################################
    std::shared_ptr<PhantomWidget> ph_btn_nav_a = factory_button( tw, th, cur_x, t, 2, true );
    cur_x += tw+2*mg;
    add_std_text_label( ph_btn_nav_a, "XY", w, h/2-2, 0,     0 );
    add_std_text_label( ph_btn_nav_a, "+",  w, h/2-5, 2*w+2, 0 );
    add_std_text_label( ph_btn_nav_a, "-",  w, h/2-4, 0,     th-h/2+4 );
    create_nav_button( ph_btn_nav_a, NAV_LEFT,  w, h, 0,       h/2, PARAM_ID_XLEFT,  axes_active.x.enabled );
    create_nav_button( ph_btn_nav_a, NAV_RIGHT, w, h, 2*w+2*l, h/2, PARAM_ID_XRIGHT, axes_active.x.enabled );
    create_nav_button( ph_btn_nav_a, NAV_UP,    w, h, w+l,     0,   PARAM_ID_YUP,    axes_active.y.enabled );
    create_nav_button( ph_btn_nav_a, NAV_DOWN,  w, h, w+l,     h+l, PARAM_ID_YDOWN,  axes_active.y.enabled );
    ph_btn_nav_a->set_is_refreshable();
    page->add_widget( ph_btn_nav_a );

    //##############################################
    // Navigation buttons UV
    //##############################################
    std::shared_ptr<PhantomWidget> ph_btn2 = factory_button( tw, th, cur_x, t, 2, true );
    cur_x += tw+mg;
    add_std_text_label( ph_btn2, "UV", w, h/2-2, 0,     0 );
    add_std_text_label( ph_btn2, "+",  w, h/2-5, 2*w+2, 0 );
    add_std_text_label( ph_btn2, "-",  w, h/2-4, 0,     th-h/2+4 );
    create_nav_button( ph_btn2, NAV_LEFT,  w, h, 0,       h/2, PARAM_ID_ULEFT,  axes_active.u.enabled );
    create_nav_button( ph_btn2, NAV_RIGHT, w, h, 2*w+2*l, h/2, PARAM_ID_URIGHT, axes_active.u.enabled );
    create_nav_button( ph_btn2, NAV_UP,    w, h, w+l,     0,   PARAM_ID_VUP,    axes_active.v.enabled );
    create_nav_button( ph_btn2, NAV_DOWN,  w, h, w+l,     h+l, PARAM_ID_VDOWN,  axes_active.v.enabled );
    ph_btn2->set_is_refreshable();
    page->add_widget( ph_btn2 ); // add widget to active tab; default active tab is the first one

    //##############################################
    // Navigation buttons Z
    //##############################################
    std::shared_ptr<PhantomWidget> ph_btnjogz = factory_button( w+20, 2*h+l, cur_x, t, 2, true );
    add_std_text_label( ph_btnjogz, "Z", 19, h/2-1, 0, 0 );
    create_nav_button( ph_btnjogz, NAV_UP,   w, h, 20, 0,   PARAM_ID_ZUP,   axes_active.z.enabled );
    create_nav_button( ph_btnjogz, NAV_DOWN, w, h, 20, h+l, PARAM_ID_ZDOWN, axes_active.z.enabled );
    ph_btnjogz->set_is_refreshable();
    page->add_widget( ph_btnjogz ); // add widget to active tab; default active tab is the first one

    //##############################################
    // Start/Stop button
    //##############################################    
    std::shared_ptr<PhantomWidget> ph_btn = factory_button( 50, th, 320-50, t, 1, true );
    ph_btn->set_is_refreshable();
    ph_btn->addItem(OUTPUT_TYPE_STRING,BTN_START,PARAM_TYPE_BOOL,PARAM_ID_PWM_STATE,50,th,0,0,true,DEFAULT_FONT)
                ->set_string_value( BTN_START )->set_radius(r)->set_colors( 0, BUTTON_LIST_BG, TFT_LIGHTGREY )
                ->set_colors( 1, BUTTON_LIST_BG_DEACTIVATED, TFT_LIGHTGREY )->set_colors( 2, COLORORANGERED, TFT_WHITE )
                ->set_is_refreshable()
                ->on(TOUCHED,change_setting).on(EVENT_LOAD,load_pwm_enabled);
    page->add_widget( ph_btn );     // add widget to active tab; default active tab is the first one


    page->select_tab( ui.get_active_tab_for_page( PAGE_FRONT ), true ); // select first tab of this page again 
    page->make_temporary();

    page = nullptr;

}


void build_file_manager( int type ){

    int  folder_page = type == 0 ? PAGE_FILE_MANAGER : PAGE_FILE_MANAGER_GCODES;
    auto page        = ui.addPage( folder_page, 1 );
    // precalculate based on the number of file on the card matching the extension
    // less trouble for now compared to keeping track of the file index and return stuff
    int total_files = SD_CARD.count_files_in_folder( type == 0 ? SD_SETTINGS_FOLDER : SD_GCODE_FOLDER );
    if( total_files == -1 ){ total_files = 0; }
    std::shared_ptr<PhantomWidget> file_manager = factory_elist( 300, 205, 10, 0, 0, false );
    std::shared_ptr<file_browser> __this = std::dynamic_pointer_cast<file_browser>(file_manager);
    std::string page_title = type == 0 ? SD_SETTINGS_FOLDER : SD_GCODE_FOLDER;
    page_title += " (";
    page_title += intToString( total_files );
    page_title += " files)";
    __this->set_file_type( type );
    __this->set_page_title( page_title );                                   // set page title
    __this->set_num_items( FILE_ITEMS_PER_PAGE );                           // number of files to view per page
    __this->set_folder( type == 0 ? SD_SETTINGS_FOLDER : SD_GCODE_FOLDER ); // set the folder to look for files
    __this->create();                                                       // setup some internal callbacks and initialize the sd folder
    __this->start();
    page->add_widget( file_manager );
    int page_id   = page->get_page_id();
    int page_tab  = page->get_active_tab();
    int widget_id = file_manager->index;
    //##########################################################################################
    // Bottom navigation buttons 
    //##########################################################################################
    const std::function<void(PhantomWidgetItem *item)> button_is_active = [widget_id,total_files](PhantomWidgetItem *item){
        item->disabled = item->param_id == PARAM_ID_PAGE_NAV_LEFT ? 
                             ( SD_CARD.get_current_tracker_index(0) <= FILE_ITEMS_PER_PAGE || !SD_CARD.is_free() ) :
                             ( SD_CARD.get_current_tracker_index(0) >= total_files         || !SD_CARD.is_free() );
    };

    const std::function<void(PhantomWidgetItem *item)> previous = [widget_id,folder_page](PhantomWidgetItem *item){
        // load previous batch 
        if( ui.get_active_page() != folder_page ){ return; }
        auto widget = ui.pages[folder_page]->tabs[0]->widgets.find( widget_id );
        std::shared_ptr<file_browser> __this = std::dynamic_pointer_cast<file_browser>( widget->second );
        __this->load_file_batch( false );
        ui.reload_page();
    };
    const std::function<void(PhantomWidgetItem *item)> next = [widget_id,folder_page](PhantomWidgetItem *item){
        // load next batch
        if( ui.get_active_page() != folder_page ){ return; }
        auto widget = ui.pages[folder_page]->tabs[0]->widgets.find( widget_id );
        std::shared_ptr<file_browser> __this = std::dynamic_pointer_cast<file_browser>(widget->second);
        __this->load_file_batch( true );
        ui.reload_page();
    };
    std::shared_ptr<PhantomWidget> nav = factory_button( 320, 30, 0, 210, 0, true );

    nav->addItem(OUTPUT_TYPE_NONE,NAV_LEFT, PARAM_TYPE_INT, PARAM_ID_PAGE_NAV_LEFT,60,35,200,0,true,-1)->set_is_refreshable()->set_require_sd()->set_outlines(2)->on(TOUCHED,previous).on(CHECK_REQUIRES,button_is_active);
    nav->addItem(OUTPUT_TYPE_NONE,NAV_RIGHT, PARAM_TYPE_INT, PARAM_ID_PAGE_NAV_RIGHT,60,35,260,0,true,-1)->set_is_refreshable()->set_require_sd()->on(TOUCHED,next).on(CHECK_REQUIRES,button_is_active);
    nav->addItem(OUTPUT_TYPE_NONE,DICT_CLOSE, PARAM_TYPE_NONE, PARAM_ID_PAGE_NAV_CLOSE,200,35,0,0,true,DEFAULT_FONT)->set_outlines(2)->on(TOUCHED,load_previous_page);
    nav->set_is_refreshable();
    page->add_widget( nav );

    page->select_tab( ui.get_active_tab_for_page( folder_page ), true ); // select first tab of this page again 
    page->make_temporary();
    page   = nullptr;
    __this = nullptr;


}

void create_file_manager_page_gcode(){
    build_file_manager( 1 );
}

void create_file_manager_page(){
    build_file_manager( 0 );
}

void create_probing_page(){
    auto page = ui.addPage( PAGE_PROBING, 1 );
    std::shared_ptr<PhantomWidget> nav = factory_button( 320, 30, 0, 210, 0, true );
    nav->addItem(OUTPUT_TYPE_NONE,DICT_CLOSE, PARAM_TYPE_NONE, PARAM_ID_PAGE_NAV_CLOSE,320,30,0,0,true,DEFAULT_FONT)->set_outlines(2)->on(TOUCHED,load_previous_page);
    page->add_widget( nav );
    page->select_tab( ui.get_active_tab_for_page( PAGE_PROBING ), true ); // select first tab of this page again 
    page->make_temporary();
    page = nullptr;
}

const std::function<void(PhantomWidgetItem *item)> open_page_pause_process = [](PhantomWidgetItem *item) {
    ui.emit_param_set( PARAM_ID_EDM_PAUSE, "true" );
    ui.load_next_page    = true;
    ui.next_page_to_load = item->intValue ;
};


void create_process_page(){

    auto page = ui.addPage( PAGE_IN_PROCESS, 1 );

    int w = 160, h = 16, current_y = 105, total_items = N_AXIS;
    std::shared_ptr<PhantomWidget> mpos_wpos = factory_button( 80, 20, 15, current_y, 2, true );
    mpos_wpos->disabled_emits_global()->disable_background_color();
    mpos_wpos->addItem(OUTPUT_TYPE_NONE,"WPos",PARAM_TYPE_NONE,0,80,h,0,0,false,DEFAULT_FONT)->set_colors( 0, -1, TFT_LIGHTGREY )->set_colors( 1, -1, TFT_LIGHTGREY )->set_colors( 2, -1, TFT_LIGHTGREY )->set_int_value( -1 )->set_text_align( 1 );
    page->add_widget(mpos_wpos);
    current_y += 20; 

    std::shared_ptr<PhantomWidget> axis_names = factory_list( 15, total_items*h, 0, current_y, 2, true );
    axis_names->disabled_emits_global()->disable_background_color();
    for( int i = 0; i < N_AXIS; ++i ){
        std::string axis_name;
        axis_name = get_axis_name_const(i);
        axis_names->addItem(OUTPUT_TYPE_STRING,"",PARAM_TYPE_NONE,0,0,0,0,0,false,MINI_FONT)->set_int_value( -1 )->set_string_value( axis_name )->set_colors( 0, -1, TFT_LIGHTGREY )->set_text_align(1);
    }
    page->add_widget(axis_names);

    std::shared_ptr<PhantomWidget> axis_wpos = factory_list( 70, total_items*h, 15, current_y, 2, true );
    axis_wpos->disable_background_color();
    axis_wpos->set_is_refreshable();
    setget_param_enum param_id;
    for( int i = 0; i < N_AXIS; ++i ){
        if( is_axis( "X", i ) ){
            param_id = PARAM_ID_WPOSX;
        } else if( is_axis( "Y", i ) ){
            param_id = PARAM_ID_WPOSY;
        } else if( is_axis( "Z", i ) ){
            param_id = PARAM_ID_WPOSZ;
        } else if( is_axis("U", i) ){
            param_id = PARAM_ID_WPOSU;
        } else if( is_axis("V", i) ){
            param_id = PARAM_ID_WPOSV;
        } else { continue; }
        axis_wpos->addItem(OUTPUT_TYPE_FLOAT,"",PARAM_TYPE_FLOAT,param_id,0,0,0,0,false,MINI_FONT)->set_float_decimals( 3 )->set_colors( 0, -1, TFT_GREEN )->set_text_align(1)->set_is_refreshable()->on(EVENT_LOAD,load);
    }
    page->add_widget(axis_wpos);


    std::shared_ptr<PhantomWidget> bottom_controls = factory_button( 320, 30, 0, 210, 0, true );
    bottom_controls->set_is_refreshable();
    bottom_controls->addItem(OUTPUT_TYPE_NONE,"Settings",PARAM_TYPE_NONE,0,80,30,0,0,true,DEFAULT_FONT)->set_int_value( PAGE_MAIN_SETTINGS )->on(TOUCHED,open_page_pause_process);
    bottom_controls->addItem(OUTPUT_TYPE_STRING,"Pause",PARAM_TYPE_BOOL,PARAM_ID_EDM_PAUSE,239,30,81,0,true,DEFAULT_FONT)->set_is_refreshable()->set_string_value( "Pause" )->set_colors( 2, COLORORANGERED, TFT_LIGHTGREY )->on(TOUCHED,change_setting).on(EVENT_LOAD,load_toogle_active_state);
    page->add_widget(bottom_controls);
    page->select_tab( ui.get_active_tab_for_page( PAGE_IN_PROCESS ), true ); // select first tab of this page again 
    page->make_temporary();
    page = nullptr;

}



const std::function<void(PhantomWidgetItem *item)> factory_reset_lambda = [](PhantomWidgetItem *item){
    ui.lock_touch();
    ui.delete_nvs_storage();
    set_machine_state( STATE_REBOOT );
};



void create_file_widget( std::shared_ptr<PhantomWidget> &container, page_map page_id, std::string text, std::string text_b ){
    container->disable_background_color();
    container->addItem(OUTPUT_TYPE_STRING,"",PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
             ->set_int_value( -1 )->set_string_value( text )->set_require_sd()
             ->set_colors( 0, -1, TFT_LIGHTGREY )->on(CHECK_REQUIRES,check_requirements);
    PhantomWidgetItem *item = container->addItem(OUTPUT_TYPE_STRING,"",PARAM_TYPE_NONE,0,0,0,0,0,true,DEFAULT_FONT)
             ->set_int_value( page_id )->set_string_value( text_b )
             ->set_require_sd()->set_is_refreshable()->set_colors( 0, -1, TFT_YELLOW );
    item->on(CHECK_REQUIRES,check_requirements).on(TOUCHED,open_page);
    if( page_id == PAGE_FILE_MANAGER_GCODES ){
        item->on(EVENT_LOAD,set_gcode_name_lambda);
    }
    item = nullptr;
    container->set_is_refreshable();
}


void create_sd_page(){

    auto page = ui.addPage( PAGE_SD_CARD, 3 );

    // fill tab 0 = save/load settings
    std::shared_ptr<PhantomWidget> settings_save_container = factory_button( 310, 40, 5, 50, 2, true );
    settings_save_container->addItem(OUTPUT_TYPE_NONE,"Save Settings",PARAM_TYPE_NONE,0,154,40,0,0,true,DEFAULT_FONT)
             ->set_int_value( SD_SAVE_AUTONAME )->set_require_sd()->set_is_refreshable()
             ->on(CHECK_REQUIRES,check_requirements).on(TOUCHED,save_settings_lambda);
    settings_save_container->addItem(OUTPUT_TYPE_NONE,"Load Settings",PARAM_TYPE_NONE,0,154,40,155,0,true,DEFAULT_FONT)
             ->set_int_value( PAGE_FILE_MANAGER )->set_require_sd()->set_is_refreshable()
             ->on(CHECK_REQUIRES,check_requirements).on(TOUCHED,open_page);
    settings_save_container->set_is_refreshable();
    page->add_widget(settings_save_container);

    //####################################
    // Loaded settings info
    //####################################
    std::shared_ptr<PhantomWidget> settings_current_container = factory_list( 150, 50, 5, 110, 2, true );
    create_file_widget( settings_current_container, PAGE_FILE_MANAGER, "Loaded settings:", get_filename_from_path( ui.settings_file ) );
    page->add_widget(settings_current_container);
    settings_current_container->set_is_refreshable();



    page->select_tab( 1 ); // gcode tab
    std::shared_ptr<PhantomWidget> gcode_load_container = factory_button( 150, 40, 5, 50, 2, true );
    gcode_load_container->addItem(OUTPUT_TYPE_NONE,"Set GCode file",PARAM_TYPE_NONE,0,154,40,0,0,true,DEFAULT_FONT)
             ->set_int_value( PAGE_FILE_MANAGER_GCODES )
             ->set_require_sd()->set_is_refreshable()
             ->on(CHECK_REQUIRES,check_requirements).on(TOUCHED,open_page);
    page->add_widget(gcode_load_container);

    //####################################
    // Loaded gcode info
    //####################################
    std::shared_ptr<PhantomWidget> gcode_current_container = factory_list( 150, 50, 5, 110, 2, true );
    create_file_widget( gcode_current_container, PAGE_FILE_MANAGER_GCODES, "Loaded GCode file:", get_filename_from_path( ui.gcode_file ) );
    page->add_widget(gcode_current_container);
    gcode_current_container->set_is_refreshable();


    page->select_tab( 2 ); // others
    std::shared_ptr<PhantomWidget> settings_factory_reset = factory_button( 310, 24, 5, 50, 0, true ); // widget_type is not used in the button widget. The type/t value doesn't matter
    settings_factory_reset->addItem(OUTPUT_TYPE_NONE,"Factory Reset",PARAM_TYPE_NONE,0,310,24,0,0,true,DEFAULT_FONT)
             ->set_colors( 0, COLORORANGERED, TFT_WHITE )
             ->on(TOUCHED,factory_reset_lambda);
    page->add_widget(settings_factory_reset);

    page->add_tab_item(0,DICT_SD_SET_LABEL);
    page->add_tab_item(1,DICT_SD_GCODE_LABEL);
    page->add_tab_item(2,"Others");
    page->add_nav( 2 ); 
    page->select_tab( ui.get_active_tab_for_page( PAGE_SD_CARD ), true ); // select first tab of this page again 
    page->make_temporary();
    page = nullptr;

}

//##############################################
// As long as the ui runs on the same esp
// it needs some faster solutions in the process
// skipping all the emitter stuff etc
//##############################################
IRAM_ATTR void PhantomUI::wpos_update( void ){
    if( ui.load_next_page ){
        ui.load_next_page = false;
        ui.show_page( ui.next_page_to_load );
        vTaskDelay(1);
        return;
    }
    ui.reload_page_on_change();
}


//############################################################################################################
// Boot screen
//############################################################################################################
void PhantomUI::loader_screen( bool fill_screen ){
    canvas.setTextSize(3);
    canvas.setTextColor(TFT_DARKGREY);
    canvas.setTextSize(3);
    canvas.setTextFont(2);
    canvas.createSprite( canvas.textWidth(BRAND_NAME), canvas.fontHeight()+20 );
    canvas.fillScreen(TFT_BLACK);
    canvas.drawString(BRAND_NAME,0,20);
    int16_t x = 20, y = -60;
    do { canvas.pushSprite(x,y); vTaskDelay(5); } while (++y < 0);
    canvas.deleteSprite();
    canvas.setTextSize(2);
    canvas.createSprite( 320, canvas.fontHeight() );
    canvas.fillScreen(TFT_BLACK);
    canvas.setTextColor(COLORRAL6003);
    canvas.setTextSize(2);
    canvas.drawString(MASHINE_AXES_STRING, 20, 0, 2);vTaskDelay(1);
    canvas.setTextColor(BUTTON_LIST_BG);
    x = 320, y = 70;
    do { canvas.pushSprite(x,60); vTaskDelay(1); } while (--x != 0);
    canvas.deleteSprite();
    vTaskDelay(100);
}

//############################################################################################################
// PhantomUI init function
//############################################################################################################
void PhantomUI::init(){
    msg_id        = INFO_NONE;
    sd_card_state = -1;
    settings_file = "Last Session";
    gcode_file    = "";
    memset( page_tree, 0, sizeof(page_tree) );
    memset( active_tabs, 0, sizeof(active_tabs) );
    page_tree_index    = 0;
    tft.init();vTaskDelay(1);
    tft.setRotation(3);vTaskDelay(1);
    tft.setCursor(0, 0, 2);vTaskDelay(1);
    fill_screen(TFT_BLACK);
    tft.setCursor(0, 0, 2);
    tft.setTextColor(TFT_LIGHTGREY);
    tft.setCursor(0, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
}



void PhantomUI::set_is_ready(){
    ui_ready.store(true);
}
bool PhantomUI::get_is_ready(){
    return ui_ready.load();
}
static const std::function<void(int *data)> alarm_lamda = [](int *data){
    reset_lambda( data );
    PhantomAlarm * alarm_handler = new PhantomAlarm( 320, 240, 0, 0, 2, false );
    int erro_code = *data;
    alarm_handler->set_error_code( erro_code );
    alarm_handler->show();
    delete alarm_handler;
    alarm_handler = nullptr;
};


//###################################################################
// This is a hacky bugfix for now. If sd card is initialized after the new boot screen it has 
// problems with updates via sd card and creating the initial directorytree after a power is turned on.
// Strange thing: It does work after a software triggered reboot or a reboot via the small button on the ESP board.
// This one makes all the SD stuff before showing the boot screen and uses only basic tft features.
// Not a beauty but it works. 
//###################################################################
void PhantomUI::init_sd(){
    if( is_machine_state( STATE_REBOOT ) ) return;
    tft.setTextSize(1);
    SD_CARD.set_spi_instance( tft.getSPIinstance() );
    charge_logger( "Starting SD card" );vTaskDelay(DEBUG_LOG_DELAY);
    if( !SD_CARD.run() ){
        tft.setTextColor( TFT_YELLOW );
        const char* logs[] = {
            "*SD card (FAT32) not found", 
            "Some cards and adapters don't work",
            "Some don't work stable",
            "Test different cards and adapters",
            "@Works: Sandisk ultra 64gb A1",
            "@Works: Transcend microSDHC 4Gb Class 6",
            " Jumper J1 on the back of the ILI9341",
            " needs to be bridged with solder"
        };
        log_pump( logs, 1000 );
        return;
    } else {
        tft.setTextColor( TFT_LIGHTGREY );
        const char* logs[] = {
            "@SD card (FAT32) found", 
            "If the card disappears without",
            "removing it try a different SD card!",
            "Many cards and adapters don't work well",
            "@Works: Sandisk ultra 64gb A1",
            "@Works: Transcend microSDHC 4Gb Class 6",
            " Jumper J1 on the back of the ILI9341",
            " needs to be bridged with solder"
        };
        log_pump( logs, 1000 );
    }
}



// setup events and everything and start sd card
void PhantomUI::build_phantom(){ // 1; called after gui events are set; first ui call

    bool nvs_ok = pref_init();



    set_motion_is_active( false ); // default to motion not active until loaded

    init();

    fill_screen(TFT_BLACK);

    if( ! is_machine_state( STATE_REBOOT ) ){
        loader_screen();vTaskDelay(1);
        ui.logger->is_visible = true;vTaskDelay(10);

        if( touch_calibrate( false ) ){
            loader_screen();vTaskDelay(1);
        }
        vTaskDelay(500);
        tft.setCursor(0, 0, 2);
        tft.setTextColor(TFT_LIGHTGREY);
        tft.setCursor(0, 0);
        tft.setTextFont(2);
        tft.setTextSize(1);

        if( !firmware_is_recent() ){ 
            const char* logs[] = {
                "#######################################",
                " New firmware version or different", 
                " axis configuration. Needs a full NVS",
                " factory reset.",
                " It will reboot after deleting the",
                " storage cache",
                "#######################################"
            };
            log_pump( logs, 1000 );
            delete_nvs_storage();
            firmware_is_recent_set_flag();
            write_calibration_data();
            set_machine_state( STATE_REBOOT );
        } 

    }

    page_creator_callbacks[PAGE_FRONT]               = create_front_page;
    page_creator_callbacks[PAGE_MAIN_SETTINGS]       = create_settings_page;
    page_creator_callbacks[PAGE_MODES]               = create_mode_page;
    page_creator_callbacks[PAGE_MOTION]              = create_motion_page;
    page_creator_callbacks[PAGE_SD_CARD]             = create_sd_page;
    page_creator_callbacks[PAGE_FILE_MANAGER]        = create_file_manager_page;
    page_creator_callbacks[PAGE_FILE_MANAGER_GCODES] = create_file_manager_page_gcode;
    page_creator_callbacks[PAGE_IN_PROCESS]          = create_process_page;
    page_creator_callbacks[PAGE_PROBING]             = create_probing_page;

    logger->is_visible = true;

    ui.on(EVENT_TOUCH_PULSE,monitor_touch_lambda);     //const std::function<void (int *data)> monitor_touch_lambda
    ui.on(SHOW_PAGE,show_page_ev);                     //const std::function<void (int *data)> show_page_ev
    ui.on(ALARM,alarm_lamda);                          //static const std::function<void (int *data)> alarm_lamda
    ui.on(RELOAD_ON_CHANGE,reload_page_if_changed);    //const std::function<void (int *data)> reload_page_if_changed
    ui.on(RESET,reset_lambda);                         
    ui.on(INFOBOX,info_box_lambda);
    ui.on(RUN_SD_CARD_JOB,start_sd_job_lambda);
    ui.on(END_SD_CARD_JOB,end_sd_job_lambda);
    ui.on(SD_CHANGE,sd_change);

    vTaskDelay(DEBUG_LOG_DELAY);

}


//########################################################
// Last session settings are stored in the NVS on the ESP
// and don't need an SD card 
//########################################################

// restore last session on boot
bool PhantomUI::restore_last_session(){
    if( is_machine_state( STATE_REBOOT ) ){ return false; }

   if( !pref_get_bool( "has_settings" ) ){
        debuglog("*No valid last session data", DEBUG_LOG_DELAY );
        debuglog("*Setting defaults", 1000);
        save_current_to_nvs(-1);
        return false;
    } else {
        debuglog("@Restoring settings",100);
    }

    int param_id;
    for( auto& param : settings_file_param_types ){
        param_id = param.first;
        const char* key_buffer = int_to_char( param_id );

        auto param_type = settings_file_param_types[param_id]; 

        switch( param_type ){
            case PARAM_TYPE_NONE:
            case PARAM_TYPE_STRING:
            case PARAM_TYPE_NONE_REDRAW_WIDGET:
            case PARAM_TYPE_NONE_REDRAW_PAGE:
            case PARAM_TYPE_BOOL_RELOAD_WIDGET:
            case PARAM_TYPE_INT_RAW_RELOAD_WIDGET:
            case PARAM_TYPE_INT_INCR:
            case PARAM_TYPE_BOOL_RELOAD_WIDGET_OC:
                break;
            case PARAM_TYPE_BOOL:
                emit_param_set( param_id, pref_get_bool( key_buffer ) ? "true" : "false" );
                break;
            case PARAM_TYPE_INT:
            case PARAM_TYPE_FLOAT:
                const char* buffer = param_type == PARAM_TYPE_INT 
                                     ? int_to_char(   pref_get_int(   key_buffer ) ) 
                                     : float_to_char( pref_get_float( key_buffer ) );
                emit_param_set( param_id, buffer );
                delete[] buffer;
                buffer = nullptr;
                break;

        }
        delete[] key_buffer;
        key_buffer = nullptr;
    }

    return true;
}

//#####################################################################################################
// Iterate over all settings that are storable and pass them to a callback lambda function
//#####################################################################################################
bool param_loop( std::function<void(int param_id, setting_cache_object package)> callback = nullptr ){
    if( callback == nullptr ){ return false; }
    for( auto& param : settings_file_param_types ){
        int param_id = param.first;
        if( settings_values_cache.find( param_id ) == settings_values_cache.end() ){
            // no cached value found
            param_load_raw( param_id ); // load param and update the cached data
            if( settings_values_cache.find( param_id ) == settings_values_cache.end() ){
                // still not found, skip
                continue;
            }
        }
        // param package available
        callback( param_id, settings_values_cache.find( param_id )->second );
    }
    return true;
}



// save specific paramater to NVS or save all parameters if no param_id is passed
bool PhantomUI::save_current_to_nvs( int _param_id ){

    const std::function<void( int param_id, setting_cache_object package )> update_param = []( int param_id, setting_cache_object package ) {

        if( settings_file_param_types.find( param_id ) == settings_file_param_types.end() ){ return; }

        //acquire_lock_for( ATOMIC_LOCK_GSCOPE ); // this function is tricky...
    
        // std::string key_string = intToString( param_id );
        const char* key_buffer = int_to_char( param_id );
        switch( settings_file_param_types[param_id] ){
            case PARAM_TYPE_BOOL:
                pref_write_bool( key_buffer, package.boolValue );
            break;
            case PARAM_TYPE_INT:
                pref_write_int( key_buffer, package.intValue );
            break;
            case PARAM_TYPE_FLOAT:
                pref_write_float( key_buffer, package.floatValue );
            break;
            default:
            break;
        }

        delete[] key_buffer;
        key_buffer = nullptr;
    
        //release_lock_for( ATOMIC_LOCK_GSCOPE );

    };


    if( _param_id != -1 ){


        bool change = true;
        if( settings_values_cache.find( _param_id ) == settings_values_cache.end() ){ // no existing cache object found for this parameter id
            param_load_raw( _param_id ); // try to load it
            if( settings_values_cache.find( _param_id ) != settings_values_cache.end() ){ // still not found; skip the writeout
                change = false;
            }
        }
        
        if( change ){
            update_param( _param_id, settings_values_cache.find( _param_id )->second );
        }


    } else { 

        debuglog("Writing Settings to NVS", DEBUG_LOG_DELAY );

        param_loop( update_param ); 

        pref_write_bool( "has_settings", true );
        pref_write_bool( get_firmware_key().c_str(), true );

    }


    return true;
}

void PhantomUI::set_settings_file( const char* file_path ){
    ui.settings_file           = file_path;
    ui.load_settings_file_next = true;
}

void PhantomUI::set_gcode_file( const char* file_path ){
    bool update = false;
    if( ui.gcode_file != file_path ){
        update = true;
    }
    ui.gcode_file = file_path;
    if( !update ){ return; }
    ui.emit_param_set( PARAM_ID_SD_FILE_SET, strlen( file_path ) > 0?"true":"false" );
}

//###################################################################
// Only called in clean_reboot()
//###################################################################
void PhantomUI::exit(){
    debuglog("UI quit");
    SD_CARD.exit();
    pref_stop();
}

//###################################################################
// If sd_line_shift is flagged true it will try to load the next line
// from the loaded gcode file and check for touch input
// This function is only called while in the process to heavily
// reduce the average load and get better performance
//###################################################################
IRAM_ATTR void PhantomUI::process_update( void ){
    shift_gcode_line();
    ui.monitor_touch(); // touch monitor
    show_logger();
}
IRAM_ATTR void PhantomUI::process_update_sd( void ){
    shift_gcode_line();
    show_logger();
}



//###################################################################
// Saves current settings to a file on SD card
//###################################################################
bool PhantomUI::save_settings_writeout( std::string file_name ){
    //return true;
    if( SD_CARD.unavailable() ){
        debuglog("*SD card not found!", DEBUG_LOG_DELAY );
        return false;

    }
    std::string file_path = SD_SETTINGS_FOLDER;
    file_path += "/";
    file_path += file_name;
    file_path += ".conf";
    SD_CARD.delete_file( file_path.c_str() ); // delete the settings file if it already exists

    if( SD_CARD.begin_multiple( file_path.c_str(), FILE_WRITE ) ){
    //if( SD_CARD.begin( file_path.c_str(), FILE_WRITE ) ){
        param_loop([](int param_id, setting_cache_object package){ // iterate over saveable settings
            
            bool skip = false;
            switch( param_id ){ // ignore the steps/mm settings to allow the files to be shared easily
                case PARAM_ID_X_RES:
                case PARAM_ID_Y_RES:
                case PARAM_ID_Z_RES:
                case PARAM_ID_U_RES:
                case PARAM_ID_V_RES:
                case PARAM_ID_SPINDLE_STEPS_PER_MM:
                    skip = true;
                    break;
                default:
                    break;
            }

            if( !skip ){
                std::string key_value_str = "";
                std::string key_str       = intToString( param_id );
                key_value_str += key_str.c_str();
                key_value_str += ":";
                switch( settings_file_param_types[param_id] ){
                    case PARAM_TYPE_BOOL:
                        key_value_str += package.boolValue ? "true" : "false";
                        SD_CARD.add_line( key_value_str.c_str() );
                        break;
                    case PARAM_TYPE_INT:
                        key_value_str += intToString( package.intValue );
                        SD_CARD.add_line( key_value_str.c_str() );
                        break;
                    case PARAM_TYPE_FLOAT:
                        key_value_str += floatToString( package.floatValue );
                        SD_CARD.add_line( key_value_str.c_str() );
                        break;
                    default: break;
                }
            } 
        });
        SD_CARD.end();
        debuglog("@Writing settings done", DEBUG_LOG_DELAY );
        return true;
    } else { 
        debuglog("*Failed to open file for write", DEBUG_LOG_DELAY );
    }
    return false;
}

//###################################################################
// Save settings to SD card with auto generated name or custom name
//###################################################################
bool PhantomUI::save_settings( sd_write_action_type action ){
    std::string file_name;
    std::string file_path;
    try {
        SD_CARD.begin_multiple( "/", FILE_READ );
        file_path = SD_CARD.generate_unique_filename();
        file_name = get_filename_from_path( file_path );
        SD_CARD.end();
    } catch ( const std::exception& e ){
        // looped out of max range
        debuglog("Failed to create filename");
        return false;
    }
    return build_alpha_keyboard( remove_extension( file_name ) );
}

//#########################################################
// Load the settings from a file on the SD Card
//#########################################################
void PhantomUI::load_settings( std::string path_full ){

    if( SD_CARD.begin_multiple( path_full, FILE_READ ) ){
        // file opened and sd state locked
        char buffer[DEFAULT_LINE_BUFFER_SIZE];
        // iterate over the lines from the file
        // each line contains a key:value pair
        int param_id;
        while( SD_CARD.get_line( buffer, DEFAULT_LINE_BUFFER_SIZE, false ) ){
            auto kvpair = split_key_value( buffer );
            if( kvpair.first > -1 ){
                param_id        = kvpair.first;
                auto param_type = settings_file_param_types[param_id]; 
                switch( param_type ){
                    case PARAM_TYPE_NONE:
                    case PARAM_TYPE_STRING:
                    case PARAM_TYPE_NONE_REDRAW_PAGE:
                    case PARAM_TYPE_NONE_REDRAW_WIDGET:
                    case PARAM_TYPE_BOOL_RELOAD_WIDGET:
                    case PARAM_TYPE_INT_RAW_RELOAD_WIDGET:
                    case PARAM_TYPE_INT_INCR:
                    case PARAM_TYPE_BOOL_RELOAD_WIDGET_OC:
                    break;
                    case PARAM_TYPE_BOOL:
                        emit_param_set( param_id, kvpair.second.c_str() );
                        break;
                    case PARAM_TYPE_INT:
                    case PARAM_TYPE_FLOAT:
                        emit_param_set( param_id, kvpair.second.c_str() );
                        break;
                }
            }
        } 

        SD_CARD.end();
        debuglog("Settingsfile loaded");
        debuglog(path_full.c_str());

        save_current_to_nvs( -1 );

    } else {
    
        debuglog("Failed to load settings from SD");
    
    }

}




void PhantomUI::write_calibration_data(){

    if( pref_write_bytes( "calibration", calibration_data, CALDATA_SIZE * sizeof(uint16_t) ) == CALDATA_SIZE * sizeof(uint16_t) ) {

    } else {

    }

}


//############################################################################################################
// Touch calibration
//############################################################################################################
bool PhantomUI::touch_calibrate( bool force ){
    bool recalibrated = false;
    bool  success    = 0;

    if( !get_is_ready() ){
        debuglog(" Press screen on boot to recalibrate!");
        uint16_t z, count = 0, confirms = 0;
        while( ++count < 10 ){
            z = tft.getTouchRawZ();
            if( z > 400 ){ 
                ++confirms; 
                vTaskDelay(100); 
            }
        }
        if( confirms > 5 ){ 
            debuglog(" Recalibration enforced");
            force = true; 
        }
    }

    if (force){

        // delete calibration data from NVS
        pref_remove( "calibration" );

    } else {

        // load calibration data from NVS
        if( pref_get_bytes( "calibration", calibration_data, CALDATA_SIZE * sizeof(uint16_t) ) == CALDATA_SIZE * sizeof(uint16_t) ) {
            debuglog("Calibration data loaded");
            success = true; // found the data
        } 

    }

    
    if( success ){

        // display calibration data found
        tft.setTouch(calibration_data);

    } else {
        
        fill_screen(TFT_BLACK);
        while( tft.getTouchRawZ() > 100 ){ vTaskDelay(100); }
        tft.setCursor(20, 0);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE);
        tft.println("Touch corners as indicated");vTaskDelay(1);
        tft.println("It should start with the");vTaskDelay(1);
        tft.println("left top corner! If a different");vTaskDelay(1);
        tft.println("corner was the first to do");vTaskDelay(1);
        tft.println("restart the calibration!");vTaskDelay(1);
        tft.setTextFont(1);
        tft.println();vTaskDelay(1);
        tft.calibrateTouch(calibration_data, TFT_MAGENTA, TFT_BLACK, 15);
        tft.setTextColor(TFT_GREENYELLOW);
        tft.println("Calibration finished.");vTaskDelay(1);
        write_calibration_data();
        fill_screen(TFT_BLACK);
        recalibrated = true;

    }

    return recalibrated;
}
