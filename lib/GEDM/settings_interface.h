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

//############################################################################################################
/*

Work in progress to have a consitent interface on each class that allows threadsafe access to the paramaters
used in the class.

Not meant for high speed operation but jsut a safe storage where settings are stored and can be distributed

*/
//############################################################################################################


#ifndef SETTINGS_INTERFACE_H
#define SETTINGS_INTERFACE_H

#include "widgets/language/en_us.h"

//#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>

enum setting_types_enum {
    SETTING_TYPE_INT   = 0,
    SETTING_TYPE_FLOAT = 1,
    SETTING_TYPE_BOOL  = 2
};

typedef struct settings_container {
    int                value  = 0;
    float              fvalue = 0.0f;
    setting_types_enum type  = SETTING_TYPE_INT;
} settings_container;

class SETTINGS_INTERFACE {

    private:

        std::mutex mtx;
        std::unordered_map< setget_param_enum, settings_container > __map;

    public:

        int get_setting_int( setget_param_enum param_id ){
            std::lock_guard<std::mutex> lock( mtx );
            auto it = __map.find( param_id );
            return it != __map.end() ? it->second.value : 0;
        }

        bool get_setting_bool( setget_param_enum param_id){
            std::lock_guard<std::mutex> lock( mtx );
            auto it = __map.find( param_id );
            return it != __map.end() ? ( it->second.value == 0 ? false : true ) : false;
        }

        float get_setting_float( setget_param_enum param_id ){
            std::lock_guard<std::mutex> lock( mtx );
            auto it = __map.find( param_id );
            return it != __map.end() ? it->second.fvalue : 0.0f;
        }

        void add( setget_param_enum param_id, setting_types_enum param_type, int int_value = 0, float float_value = 0.0 ){
            settings_container ctr;
            ctr.type   = param_type;
            ctr.value  = int_value;
            ctr.fvalue = float_value;
            std::lock_guard<std::mutex> lock( mtx );
            __map[ param_id ] = ctr;
        }

        bool change_setting( setget_param_enum param_id, int int_value = 0, float float_value = 0.0 ){
            std::lock_guard<std::mutex> lock( mtx );
            auto it = __map.find( param_id );
            bool changed = false;
            if( it != __map.end() ){
                switch( it->second.type ){
                    case SETTING_TYPE_BOOL:
                        if( it->second.value != int_value ){
                            it->second.value = int_value == 0 ? 0 : 1;
                            changed = true;
                        }
                    break;
                    case SETTING_TYPE_FLOAT:
                        if( it->second.fvalue != float_value ){
                            it->second.fvalue = float_value;
                            changed = true;
                        }
                    break;
                    default:
                        if( it->second.value != int_value ){
                            it->second.value = int_value;
                            changed = true;
                        }
                    break;
                }
                if( changed ){
                    notify_change( param_id );
                }
                return true;
            }
            return false;
        }

        virtual void notify_change( setget_param_enum param_id ){}
};


#endif