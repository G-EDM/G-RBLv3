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

#include <Update.h>
#include "sd_card.h"
#include "ui_interface.h"
#include "char_helpers.h"
#include "api.h"
#include "Preferences.h"
#include "pwm_controller.h"

const int MAX_FILENAME_TRIES                     = 1000;  // Prevents an infinite loop but also limits the total possible autonamed setting files to 1000
DRAM_ATTR static const int64_t refresh_interval  = 10000000; // refresh interval time in micros
DRAM_ATTR              int64_t esp_micros        = 0;

PhantomSD SD_CARD;

enum sd_codes {
    SD_CNT_FAILED,
    SD_INIT,
    SD_NOT_ENA,
    SD_NOT_FND,
    SD_DIR_SKIP,
    SD_REMOVED,
    SD_MKDIR_TRY,
    SD_DIR_EXIST,
    SD_UPDATE_CK,
    SD_UPDATE_NO,
    SD_FIRMWARE_DO,
    SD_HODL,
    SD_HODL_MORE,
    SD_UPDATE_OK,
    SD_UPDATE_FAIL,
    SD_RENAME_FW,
    SD_RENAME_OK,
    SD_REBOOT,
    SD_F_NOT_FOUND,
    SD_FOPEN_ERR,
    SD_FOPEN_OK
};

std::map<sd_codes, const char*> sd_messages = {
    { SD_CNT_FAILED,  "Counting files failed" },
    { SD_INIT,        "SD init" },
    { SD_NOT_ENA,     "SD card support disabled" },
    { SD_NOT_FND,     "SD card not found" },
    { SD_DIR_SKIP,    "Building directory tree skipped" },
    { SD_REMOVED,     "SD card removed" },
    { SD_MKDIR_TRY,   "Trying to create folder:" },
    { SD_DIR_EXIST,   "Folder exists:" },
    { SD_UPDATE_CK,   "Checking for firmware update" },
    { SD_UPDATE_NO,   "    No update found" },
    { SD_FIRMWARE_DO, "Firmware updating" },
    { SD_HODL,        "    This can take some time" },
    { SD_HODL_MORE,   "    Don't disconnect the ESP" },
    { SD_UPDATE_OK,   "Update finished" },
    { SD_UPDATE_FAIL, "Update failed" },
    { SD_RENAME_FW,   "Renamed firmware.bin to .bak" },
    { SD_RENAME_OK,   "Renaming firmware failed" },
    { SD_REBOOT,      "Rebooting ESP" },
    { SD_F_NOT_FOUND, "File not found:" },
    { SD_FOPEN_ERR,   "Failed to open file:" },
    { SD_FOPEN_OK,    "File opened:" }
};

std::atomic<bool> gcode_line_shifting( false );
std::atomic<bool> sd_card_found_flag(false);
std::atomic<int>  sd_card_state( SD_NOT_AVAILABLE );

static DRAM_ATTR int  file_index_tracker[5];
static DRAM_ATTR File current_file;

//##############################################################
// SD state manager 
//##############################################################
IRAM_ATTR bool PhantomSD::is_free(){
    return get_state() == SD_FREE ? true : false;
}
IRAM_ATTR void PhantomSD::set_state( int _state ){
    sd_card_state.store( _state, std::memory_order_release ); // new way
}
IRAM_ATTR int PhantomSD::get_state(){
    return sd_card_state.load(std::memory_order_acquire); // new one
}
IRAM_ATTR bool PhantomSD::unavailable(){
    return get_state() == SD_NOT_AVAILABLE ? true : false;
}

//################################################
// Keeps track of the sd card state to monior
// removals and insertions and trigger some ui
// reload functions if needed
// this is only called in the connect function
// state is either FREE or NOT_AVAILABLE at this
// point. Refresh does call connect() too
// if needed
//################################################
IRAM_ATTR void PhantomSD::manage_change(){

    bool new_state = get_state() == SD_FREE ? true : false;
    bool old_state = sd_card_found_flag.load(std::memory_order_acquire);

    sd_card_found_flag.store(new_state,std::memory_order_release);

    if( old_state != new_state ){
        release_lock_for( ATOMIC_LOCK_SDCARD );
        int data = new_state;
        ui.emit( SD_CHANGE, &data );
        //enforce_redraw.store( true );
    }

}

//##################################################################
// Refresh the availability of an SD card and flag changes if card
// was inserted or removed
// This is a oneshot function and not part of any extended
// sd card session; if SD busy it can be skipped as the 
// refresh function is called every now and then if no edm process
// is running
//##################################################################
IRAM_ATTR bool PhantomSD::refresh(){

    if( 
        get_state() > SD_NOT_AVAILABLE                         || 
        esp_timer_get_time() - last_refresh < refresh_interval ||
        lock_is_taken( ATOMIC_LOCK_SDCARD )                    || 
        is_machine_state( STATE_REBOOT )
    ){ 
        vTaskDelay(1);
        return false; 
    } 

    acquire_lock_for( ATOMIC_LOCK_SDCARD );

    if( ! exists(settings_folder) || SD.cardSize() <= 0 ){
        set_state( SD_NOT_AVAILABLE );
        close_active();
        SD.end();
        int error = connect( true );               // state is set within the connect function
        last_refresh = esp_timer_get_time(); // reset the timer
        release_lock_for( ATOMIC_LOCK_SDCARD );
        return true;

    } else { 

        if( get_state() == SD_NOT_AVAILABLE ){
            set_state( SD_FREE );
        }

    }

    last_refresh = esp_timer_get_time();
    release_lock_for( ATOMIC_LOCK_SDCARD );
    return false;

}

//##############################################################
// Create a folder, only for the build directorytree 
// function.
//##############################################################
int PhantomSD::create_folder( const char* folder ){
    int error  = SD_OK;
    if( !SD.exists(folder) ){
        if( !SD.mkdir(folder) ){
            error = SD_MKDIR_FAILED;
        }
        vTaskDelay(1);
    }
    return error;
}

//##################################################################
// Initial SD start function called after boot
//##################################################################
bool PhantomSD::run(){
    int error = connect( true );
    return error == SD_BEGIN_FAILED ? false : true;
}

//#################################################
// Check if a firmware.bin file is present
//#################################################
bool PhantomSD::firmware_update_available(){ // check if firmware binary is available
    if( SD.exists( firmware_bin ) ){
        return true;
    } return false;
}
//#################################################
// Update firmware
//#################################################
bool PhantomSD::firmware_update(){ 
    bool failed = false;
    if( get_state() != SD_FREE || !firmware_update_available() ){ 
        return false; 
    } 
    set_state( SD_BUSY_LOCK );
    
    charge_logger( "@Updating firmware!" );
    charge_logger( "This can take some time" );
    charge_logger( "Don't disconnect the ESP!" );

    if( open_file( firmware_bin, FILE_READ ) == SD_OK ){
        //Update.onProgress( update_progress_callback );
        Update.begin( current_file.size(), U_FLASH );
        Update.writeStream( current_file );
	    if( !Update.end() ){ failed = true; } 
        close_active();
        SD.remove(firmware_bak);
        SD.rename( firmware_bin, firmware_bak );
        firmware_update_clean();
        delay(1000);
    }
    set_state( SD_FREE );
    return failed?false:true;
}
//#################################################
// Enforce a factory reset on next boot by deleting
// all the cached settings. This will write the 
// default settings into the NVS storage next boot
//#################################################
bool PhantomSD::firmware_update_clean(){
    Preferences p;
    if( p.begin(RELEASE_NAME, false) ){
        p.putBool( "has_settings", false );
        p.end();
        return true;
    } return false;
}

//##############################################################
// Connect function 
//##############################################################
int PhantomSD::connect( bool firmware_check ){ // this connect function is used on refreshs if needed and also checks for a new firmware
    int error = SD_OK;
    if( !SD.begin( SS_OVERRIDE, __spi ) ){
        error = SD_BEGIN_FAILED;
        set_state( SD_NOT_AVAILABLE );
    } else {
        set_state( SD_FREE );
        SD_CARD.build_directory_tree();
        if( firmware_check && firmware_update() ){
            set_machine_state( STATE_REBOOT );
        }
    }
    manage_change();
    vTaskDelay(1);
    return error;
}

//##############################################################
// Only called for rebbots to exit clean before reboot
//##############################################################
bool PhantomSD::exit(){
    //if( sd_card_task_handle != NULL ){ vTaskDelete( sd_card_task_handle ); };
    set_state( SD_NOT_AVAILABLE );
    close_active();
    SD.end();
    return true;
}

void PhantomSD::set_spi_instance( SPIClass &_spi ){
    __spi = _spi;
}

//#################################################
// Creates the initial directory tree on boot
//#################################################
int PhantomSD::build_directory_tree(){
    if( lock_is_taken( ATOMIC_LOCK_SDCARD ) ) return SD_OK;
    acquire_lock_for( ATOMIC_LOCK_SDCARD );
    if( get_state() != SD_FREE ) return SD_OK;
    int error = create_folder( root_folder );
    if( error == SD_OK ){
        error = create_folder( settings_folder );
        error = create_folder( gcode_folder );
    } 
    release_lock_for( ATOMIC_LOCK_SDCARD );
    return error;
};

//#################################################
// This doesn't really end the sd card
// it resets some stuff, closes the current file
// and flag the state as free
//#################################################
bool PhantomSD::end(){
    file_end     = false;
    current_line = 0;
    close_active();
    set_state( SD_FREE );
    return true;
}

//#################################################
// Creates the initial directory tree on boot
//#################################################
void update_progress_callback(size_t currSize, size_t totalSize) {
	return; 
}

//##############################################################
// Start the GCode readout session
// This doesn't reads the lines. It just starts the session
// and sets the readout flag used later
//##############################################################
bool PhantomSD::start_gcode_job( std::string full_path ){
    // lock sd reader; reset everything needed; load gcode file
    file_end = false;
    if( begin_multiple( full_path, FILE_READ ) ){ // open the file and lock sd reader
        // success
        gcode_line_shifting.store( true ); // set flag to shift out lines from the file
        return true;
    } else {
        // failed to open file
        gcode_line_shifting.store( false ); // unset flag to shift lines from file
    }
    return false;
}

//##############################################################
// Begin a bigger SD operation and lock the state
// to prevent other tasks from messing around
// This function will do multiple attempts to begin the session
//##############################################################
bool PhantomSD::begin_multiple( std::string full_path, const char* type, int max_rounds ){
    bool success = false;
    while( --max_rounds > 0 ){
        if( begin( full_path.c_str(), type ) ){
            success = true;
            break;
        } 
    }
    if( success ){
    } else {
    }
    return success;
}

//##############################################################
// Begin a bigger SD operation and lock the state
// to prevent other tasks from messing around
//##############################################################
bool PhantomSD::begin( const char* path, const char* type ){ // default type = read aka "r"
    if( get_state() > SD_FREE ){
        return false; // sd not available or busy
    }
    set_state( SD_BUSY_LOCK );
    int error = open_file( path, type );
    bool success = error == SD_OK ? true : false;
    if( success ){
        current_line = 0;
    } else {
        set_state( SD_FREE );
    }
    return success;
}

//##############################################################
// Cleanup after gcode file finished
//##############################################################
bool PhantomSD::end_gcode_job( void ){
    gcode_line_shifting.store( false ); // unset flag to shift lines from file
    end();                 // close file and unlock reader
    return true;
}

// warning: not thread safe
// sd is locked in the process and it should be safe to call
// this from core 1 too
// but only from one core 
// if this function returns false it will disable 
// line shifting and assume there are no more lines left
bool PhantomSD::shift_gcode_line(){

    if( file_end ){
        vTaskDelay(1);
        return false;
    }

    if( !api::buffer_available() ){
        // buffer not free; skip this round
        // move this one out of the sd logic
        // best would be to trigger this in the protocol loop...
        vTaskDelay(1);
        return true;
    }

    static DRAM_ATTR char buffer[DEFAULT_LINE_BUFFER_SIZE];

    bool lock_taken = arcgen.lock();
    
    if( SD_CARD.get_line( buffer, DEFAULT_LINE_BUFFER_SIZE, true ) ){
        // submit the line
        api::push_cmd( buffer, false );
    } else {
        // file end
        api::move_to_origin();
        file_end = true;
    }

    arcgen.unlock( lock_taken );
    
    return file_end ? false : true;
}

// this function is used to get gcode lines and also normal lines
// the gcode lines need a linebreak while normal lines don't
// gcode lines are pushed to the inputbuffer and the protocol collects them
// without linebreak the protocol will not be able to parse the line
IRAM_ATTR bool PhantomSD::get_line( char *line, int size, bool add_linebreak ){
    if(!current_file){
        return false;
    }
    current_line += 1;
    int length = 0;
    char c;
    while( current_file.available() ){
        //current_file.readStringUntil();
        c = current_file.read();
        if (c == '\n' || c == '\r'){
            if( length == 0 ){
                // linebreak at the beginning
                continue;
            }
            break;
        }
        line[length] = c;
        if( ++length >= size-1 ){
            return false;
        }
    }
    if( length > 0 && add_linebreak ){
        line[length] = '\n'; // line end needed for the protocol
        ++length;
    } 
    line[length] = '\0';
    int _continue = length || current_file.available();
    return _continue; 
}




//##########################################
// Core SD card section
//##########################################
bool PhantomSD::add_line( const char* line ){
    if( !has_open_file || !current_file ){
        return false; // no open file
    }
    current_file.println(line);vTaskDelay(1);
    current_file.flush();vTaskDelay(1);
    return true;
}

//##############################################################
// Delete a file; Uses lock! Don't call if within an already
// locked situation or it will deadlock
//##############################################################
bool PhantomSD::delete_file( const char* path ){
    acquire_lock_for( ATOMIC_LOCK_SDCARD );
    if( exists( path ) ){
        return SD.remove(path);
    }
    release_lock_for( ATOMIC_LOCK_SDCARD );
    return false;
}

//##############################################################
// Check if file exists
//##############################################################
bool PhantomSD::exists( const char* path ){
    return SD.exists(path);
}

//##############################################################
// Close active file or folder
//##############################################################
void PhantomSD::close_active(){
    if( ! has_open_file ){
        return; // no open file
    }
    current_file.close();vTaskDelay(1);
    has_open_file = false;
}

//##############################################################
// Open a file (mode = FILE_READ or FILE_WRITE )
//##############################################################
int PhantomSD::open_file( const char* file, const char* mode ){ // full path to the file 
    if( strcmp(mode, FILE_READ) == 0 && !exists( file ) ){
        return SD_FILE_NOT_FOUND;
    }
    close_active();
    current_file = SD.open( file, mode ); vTaskDelay(1);
    if( ! current_file ){
        return SD_OPEN_FILE_FAILED;
    }
    has_open_file = true;
    return SD_OK;
}

//##############################################################
// Open a folder (FILE_WRITE)
//##############################################################
void PhantomSD::open_folder( const char* folder ){
    if( open_file( folder, FILE_READ ) == SD_OK ){
        current_file.rewindDirectory();vTaskDelay(1);
    } else {
    }
}


//############################################################################
// Creates a unique file path like Phantom-settings-999.conf
// returns the full path like /Phantom/settings/Phantom-settings-999.conf
//############################################################################
std::string PhantomSD::generate_unique_filename(){
    int start = 0;
    int vTaskDelayAfter = 10;
    while( ++start < MAX_FILENAME_TRIES ){
        if(--vTaskDelayAfter<=0){vTaskDelay(1);vTaskDelayAfter=10;}
        std::string unique_path = RELEASE_NAME;
        unique_path += "-settings-";
        unique_path += intToString( start );
        unique_path += ".conf";
        std::string full_path = SD_SETTINGS_FOLDER;
        full_path += "/";
        full_path += unique_path;
        if (!SD_CARD.exists(full_path.c_str())) {
            return full_path; // file name available
        }
    }
    throw std::runtime_error("too many files");
}

//##############################################################
// Get the total number of files in a given folder
//##############################################################
int PhantomSD::count_files_in_folder( const char* path ){
    // open folder and store it in current_file
    int max_rounds = 200;
    std::string full_path = path;
    bool success = begin_multiple( full_path, FILE_READ );
    if( ! success ){
        // problem
        return -1; // return -1 on error
    }
    current_file.rewindDirectory();
    int num_files = 0;
    File entry    = current_file.openNextFile();
    while( entry ){
        if( !entry.isDirectory() ){
            // check if the file has the correct extension
            ++num_files;
        }
        entry = current_file.openNextFile();
    }
    end(); // close the current cile and reset the state etc.
    return num_files;
}

//##############################################################
// File manager specific
// All those function are called withint file manager context
//##############################################################
void PhantomSD::reset_current_file_index( int tracker_index ){
    file_index_tracker[tracker_index] = 0;
}
int PhantomSD::get_current_tracker_index( int tracker_index ){
    return file_index_tracker[tracker_index];
}
void PhantomSD::decrement_tracker( int tracker_index, int decremet_by ){
    file_index_tracker[tracker_index] -= decremet_by;
    if( file_index_tracker[tracker_index] < 0 ){
        file_index_tracker[tracker_index] = 0;
    }
}
bool PhantomSD::go_to_last( int tracker_index ){
    if( current_file && current_file.isDirectory() ){
        for( int i = 0; i < file_index_tracker[tracker_index]; ++i ){
            current_file.openNextFile();
        }
        return true;
    } return false;
}
bool PhantomSD::get_next_file( int tracker_index, std::function<void( const char* file_name, int file_index )> callback ){
    if( callback != nullptr && current_file && current_file.isDirectory() ){
        File file = current_file.openNextFile();
        if( !file ){
            callback( "", file_index_tracker[tracker_index] );
        } else {
            if( file.isDirectory() ){}
            ++file_index_tracker[tracker_index];
            callback( get_filename_from_path( file.name() ).c_str(), file_index_tracker[tracker_index] );
            file.close();
        }
        //file.close();
        return true;
    }
    return false;
}






