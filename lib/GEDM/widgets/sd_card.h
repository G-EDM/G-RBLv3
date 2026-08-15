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

#ifndef PHANTOM_UI_SDCARD
#define PHANTOM_UI_SDCARD

#include "SD.h"
#include <stdint.h>
#include "definitions.h"
#include "event_manager.h"

enum sd_errors {
    SD_OK = 0,
    SD_FIRMWARE_UPDATE_FAILED,
    SD_FILE_NOT_FOUND,
    SD_OPEN_FILE_FAILED,
    SD_BEGIN_FAILED,
    SD_MKDIR_FAILED
};

enum sd_states {
    SD_FREE          = 0, // sd is free to use
    SD_NOT_AVAILABLE = 1, // sd not found or removed
    SD_NOT_ENABLED,       // sd card support disabled by config
    SD_HAS_FILE_OPEN,     // 
    SD_BUSY_LOCK,         // locked in process
    SD_ERROR
};

enum sd_events {
    LOAD_SETTINGS,
    SAVE_SETTINGS
};


extern std::atomic<bool> gcode_line_shifting;

static const int DEFAULT_BUFFER_SIZE = 256;



class PhantomSD : public EventManager {

    public:

        ~PhantomSD(){};
        PhantomSD() : current_line(0), 
                      has_open_file(false), 
                      last_refresh(0),
                      file_end(false),
                      root_folder(SD_ROOT_FOLDER),
                      settings_folder(SD_SETTINGS_FOLDER),
                      gcode_folder(SD_GCODE_FOLDER),
                      firmware_bin("/firmware.bin"),
                      firmware_bak("/firmware.bak"){};
                
        std::string generate_unique_filename( void );
        
        IRAM_ATTR void manage_change( void );
        IRAM_ATTR bool is_free( void );
        IRAM_ATTR bool refresh( void );
        IRAM_ATTR void set_state( int _state );
        IRAM_ATTR int  get_state( void );
        IRAM_ATTR bool unavailable( void );

        bool run( void );
        bool end( void );
        bool exit( void );
        int  connect( bool firmware_check = false );
        bool firmware_update( void );
        bool firmware_update_clean( void );
        bool firmware_update_available( void );
        int  build_directory_tree( void );
        void  set_spi_instance( SPIClass &_spi );
        bool  begin( const char* path, const char* type = FILE_READ );
        // begin multiple starts an sd session and blocks sd access for everything else until
        // it is released with SD_CARD.end()
        bool  begin_multiple( std::string folder, const char* type, int max_rounds = 100 );
        bool  add_line( const char* line );
        IRAM_ATTR bool get_line( char *line, int size, bool add_linebreak = false );
        bool  start_gcode_job( std::string full_path );
        bool  end_gcode_job( void );
        bool  shift_gcode_line( void );
        bool  delete_file( const char* path );
        bool  exists( const char* path );
        bool  get_next_file( int tracker_index, std::function<void( const char* file_name, int file_index )> callback = nullptr );
        bool  go_to_last( int tracker_index );
        int   count_files_in_folder( const char* path );
        int   get_current_tracker_index( int tracker_index );
        void  decrement_tracker( int tracker_index, int decremet_by );
        void  reset_current_file_index( int tracker_index );

    private:

        int  create_folder( const char* folder );
        int  open_file( const char* file, const char* mode = FILE_READ );
        void open_folder( const char* folder );
        void close_active( void );

        int      current_line;
        bool     has_open_file;
        int64_t  last_refresh;
        bool     file_end;

        const char* root_folder;
        const char* settings_folder;
        const char* gcode_folder;
        const char* firmware_bin;
        const char* firmware_bak;

        SPIClass __spi;

};


extern PhantomSD SD_CARD;

#endif