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


#include <iostream>
#include <cstring>
#include <sstream>
#include <cstdio>
#include <iomanip>
#include <cstdlib>

#include "char_helpers.h"

// 1, 2, 4, 8, 16, 32, 64, 128, 256, 512....
bool is_power_of_two( int n ){
    if (n <= 0) { return false; }
    return (n & (n - 1)) == 0;
}


IRAM_ATTR std::string float_to_string(float value, int precision = 6) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

// Warning: use delete[] after done with the result or it will leak memory
IRAM_ATTR const char* int_to_char(int value) {
    int length = snprintf(nullptr, 0, "%d", value);
    char* charArray = new char[length + 1];
    snprintf(charArray, length + 1, "%d", value);
    return charArray;
}
// Warning: use delete[] after done with the result or it will leak memory
IRAM_ATTR const char* float_to_char(float value) {
    int length = snprintf(nullptr, 0, "%.6f", value);
    char* charArray = new char[length + 1];
    snprintf(charArray, length + 1, "%.6f", value);
    return charArray;
}
IRAM_ATTR std::string floatToString( float value ){
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4) << value;
    std::string strValue = oss.str();
    return strValue;
}
IRAM_ATTR std::string intToString( int value ){
    std::ostringstream oss;
    oss << value;
    std::string strValue = oss.str();
    return strValue;
}

IRAM_ATTR bool char_to_bool( const char* value ){
    if( value[0] == 'T' || value[0] == 't' ){ return true; }
    return false;
}
IRAM_ATTR float char_to_float( const char* value ){
    return atof( value );
}
IRAM_ATTR int char_to_int( const char* value ){
    return atoi( value );
}


std::pair<int, std::string> split_key_value(char* input) {
    std::string key;
    std::string value;
    const char* colonPos = strchr(input, ':');
    if (colonPos == nullptr) {
        return std::make_pair(-1, "");
    }
    key.assign(input, colonPos - input);
    value.assign(colonPos + 1);
    int keyInt = atoi(key.c_str());
    return std::make_pair(keyInt, value);
}


bool path_has_extension(const char* path, const char* extension) {
    size_t ext_length = std::strlen(extension);
    const char* dot = std::strrchr(path, '.');
    if (!dot || dot == path + std::strlen(path) - 1) {
        return false;
    }
    size_t file_ext_Length = std::strlen(dot + 1);
    if (file_ext_Length != ext_length) {
        return false;
    }
    for (size_t i = 0; i < ext_length; ++i) {
        if (std::tolower(dot[i + 1]) != std::tolower(extension[i])) {
            return false;
        }
    }
    return true;
}

std::string remove_extension( const std::string& filePath ) {
    size_t lastSlashPos = filePath.find_last_of(".");
    if (lastSlashPos == std::string::npos) {
        return filePath;
    }    
    return filePath.substr(0,lastSlashPos);
}

//############################################################################
// Takes a full path and extracts the filename with extension
//############################################################################
std::string get_filename_from_path( const std::string& filePath ) {
    size_t lastSlashPos = filePath.find_last_of("/\\");
    if (lastSlashPos == std::string::npos) {
        return filePath;
    }    
    return filePath.substr(lastSlashPos + 1);
}
