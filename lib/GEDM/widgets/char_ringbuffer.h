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

#ifndef PHANTOM_UI_RINGBUFFER
#define PHANTOM_UI_RINGBUFFER

#include <iostream>
#include <cstring>
#include <atomic>
#include <mutex>
#include <cstring>

static const int max_items     = 20;
static const int log_item_size = 256;


class CharRingBuffer {

private:
    char** cringbuffer = nullptr;
    size_t capacity;
    int type_index;                     // Last read index

    std::atomic<uint8_t>* iringbuffer = nullptr;
    std::atomic<size_t> index;          // Next write position
    std::atomic<size_t> index_work;     // Read position for pop

public:
    CharRingBuffer() : capacity(0), index(0), index_work(0), type_index(0) {
        cringbuffer = nullptr;
        iringbuffer = nullptr;
    }

    ~CharRingBuffer() {
        if (cringbuffer) {
            for (size_t i = 0; i < capacity; ++i) {
                delete[] cringbuffer[i];
            }
            delete[] cringbuffer;
        }
        if (iringbuffer) {
            delete[] iringbuffer;
        }
    }

    void create_ringbuffer(size_t __capacity) {
        if (__capacity > max_items) {
            __capacity = max_items;
        }
        capacity = __capacity;

        // Allocate buffers
        cringbuffer = new char*[capacity];
        for (size_t i = 0; i < capacity; ++i) {
            cringbuffer[i] = new char[log_item_size];
            memset(cringbuffer[i], 0, log_item_size);
        }

        iringbuffer = new std::atomic<uint8_t>[capacity];

        // Initialize atoms
        index.store(0);
        index_work.store(0);
        type_index = 0;
    }

    size_t get_buffer_size() const {
        return capacity;
    }

    void add_line(const char* line) {
        // Determine log type
        uint8_t log_type = 0;
        bool trimmed = true;
        if (line[0] == ' ') {
            log_type = 1;
        } else if (line[0] == '@') {
            log_type = 2;
        } else if (line[0] == '*') {
            log_type = 3;
        } else {
            log_type = 0;
            trimmed = false;
        }

        // get current index
        size_t current_index = index.load(std::memory_order_relaxed);

        // Store type
        iringbuffer[current_index].store(log_type, std::memory_order_release);

        // Copy line into buffer
        char* dest = cringbuffer[current_index];
        const char* src = line + (trimmed ? 1 : 0);
        size_t copy_len = log_item_size - 1;

        size_t src_len = 0;
        while (src[src_len] != '\0' && src_len < copy_len) {
            src_len++;
        }

        size_t to_copy = (src_len < copy_len) ? src_len : copy_len;
        memcpy(dest, src, to_copy);
        dest[to_copy] = '\0';

        // Move to next index
        size_t next_index = (current_index + 1) % capacity;
        index.store(next_index, std::memory_order_relaxed);

    }

    void reset_work_index() {
        size_t current_index = index.load(std::memory_order_acquire);
        size_t work_idx = current_index;
        if (work_idx == 0) {
            work_idx = capacity - 1;
        } else {
            --work_idx;
        }
        index_work.store(work_idx, std::memory_order_release);
    }

    const char* pop_line() {
        size_t idx = index_work.load(std::memory_order_acquire);
        type_index = idx;
        size_t new_idx = (idx == 0) ? capacity - 1 : idx - 1;
        index_work.store(new_idx, std::memory_order_release);
        return cringbuffer[type_index];
    }

    const char* getLine(int idx) const {
        if (idx < 0 || idx >= static_cast<int>(capacity)) return nullptr;
        return cringbuffer[idx];
    }

    uint8_t getType() const {
        return iringbuffer[type_index].load(std::memory_order_acquire);
    }


};







/*class CharRingBuffer {

    public:

        CharRingBuffer( void ){}

        IRAM_ATTR void create_ringbuffer(size_t __capacity){
            if( __capacity > max_items ){
                __capacity = max_items;
            }
            capacity    = __capacity;
            index       = 0;
            index_work  = index;
            has_change  = true;
            cringbuffer = new char*[capacity];

            for (size_t i = 0; i < capacity; ++i) {
                cringbuffer[i] = new char[log_item_size];
                memset(cringbuffer[i], 0, log_item_size);
            }
        }
        ~CharRingBuffer() {
            for (size_t i = 0; i < capacity; ++i) {
                delete[] cringbuffer[i];
            }
            delete[] cringbuffer;
        }

        IRAM_ATTR size_t get_buffer_size(){
            return capacity;
        }

        IRAM_ATTR bool has_new_items(){
            return has_change;
        }

        IRAM_ATTR void add_line(const char* line) {
            bool    trimmed  = true;
            uint8_t log_type = 0;

            if( line[0] == ' '){
                log_type = 1;
            } else if( line[0] == '@' ){
                log_type = 2;
            } else if( line[0] == '*' ){
                log_type = 3;
            } else {
                log_type = 0;
                trimmed = false;
            }
            iringbuffer[index] = log_type;

            char* dest = cringbuffer[index];
            const char* src = line + (trimmed ? 1 : 0);
            size_t copy_len = log_item_size - 1;
            size_t src_len = 0;
            while (src[src_len] != '\0' && src_len < copy_len) { src_len++; }
            size_t to_copy = (src_len < copy_len) ? src_len : copy_len;
            memcpy(dest, src, to_copy);
            dest[ to_copy ] = '\0';

            if( ++index == capacity ){ index = 0; }
            has_change = true;
        }

        IRAM_ATTR void reset_work_index(void){
            index_work = index; // latest index points already to the future element
            if( index_work <= 0 ){ // move work index one back to the last written line
                index_work = capacity-1;
            } else {
                --index_work;
            }
            has_change = false;
        }

        IRAM_ATTR const char* pop_line(void){
            type_index = index_work;
            index_work = (index_work - 1 + capacity) % capacity;
            return cringbuffer[type_index];
        }

        IRAM_ATTR const char* getLine(int index) const {
            return cringbuffer[index];
        }

        IRAM_ATTR uint8_t getType( void ){
            return iringbuffer[type_index];
        }


    private:
        char** cringbuffer;
        uint8_t iringbuffer[max_items];
        size_t capacity;
        size_t index;
        size_t index_work;
        bool   has_change;
        int    type_index;

};*/


#endif