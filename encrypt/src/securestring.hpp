// securestring.hpp - Part of WuffCrypt
// Copyright 2014 Andrew Aldridge <i80and@foxquill.com>

#pragma once

#include <string.h>
#include <sodium.h>

class SecureString {
public:
    SecureString(): _data(nullptr), _len(0) {}

    explicit SecureString(char* str): SecureString(strlen(str)) {
        snprintf(_data, _len+1, "%s", str);

        // Clear our original source
        sodium_memzero(str, _len);
    }

    // Always allocates one extra byte for a terminating nul character
    explicit SecureString(size_t len): _len(len) {
        _data = new char[_len+1];
        _data[_len] = '\0';
        sodium_mlock(_data, len+1);
    }

    explicit SecureString(uint8_t* buf, size_t len): SecureString(len) {
        // Copy buf into a buffer we control
        memcpy(_data, buf, len);
        _data[_len] = '\0';

        // Clear our original source
        sodium_memzero(buf, _len);
    }

    SecureString(const SecureString& orig) = delete;

    const char* c_str() const {
        return _data;
    }

    const uint8_t* data() const {
        return reinterpret_cast<const uint8_t*>(_data);
    }

    uint8_t* data() {
        return reinterpret_cast<uint8_t*>(_data);
    }

    size_t size() const {
        return _len;
    }

    bool empty() const {
        return size() == 0;
    }

    void moveInto(SecureString& other) {
        other.wipe();
        other._data = _data;
        other._len = _len;

        _data = nullptr;
        _len = 0;
    }

    ~SecureString() {
        if(_data != nullptr) {
            wipe();
            sodium_munlock(_data, _len+1);
            delete[] _data;
        }
    }

    SecureString& operator=(const SecureString& other) = delete;

private:
    char* _data;
    size_t _len;

    void wipe() {
        if(_data != nullptr) {
            sodium_memzero(_data, _len);
        }
    }
};
