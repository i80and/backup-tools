// paddedbuffer.hpp - Part of WuffCrypt
// Copyright 2014 Andrew Aldridge <i80and@foxquill.com>

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "util.hpp"

// Wrapper around a buffer that includes an additional zero-padded buffer of length P at the
// beginning that is excluded from size checks.  The parameter D is an additional stretch of
// allocation added to all buffers, and included in size checks.
template <int P, int D>
class PaddedBuffer {
public:
    PaddedBuffer(size_t len): _data(new uint8_t[P+D+len]), _size(0), _capacity(D+len) {
        memset(_data, 0, P);
    }
    PaddedBuffer() = delete;
    PaddedBuffer(const PaddedBuffer& other) = delete;

    uint8_t* data() {
        return _data + P;
    }

    const uint8_t* data() const {
        return _data + P;
    }

    uint8_t* rawData() {
        return _data;
    }

    const uint8_t* rawData() const {
        return _data;
    }

    size_t size() const {
        return _size;
    }

    size_t rawSize() const {
        return size() + P;
    }

    void setSize(size_t newSize) {
        verify(newSize <= _capacity);
        _size = newSize;
    }

    static size_t padding() {
        return P;
    }

    ~PaddedBuffer() {
        delete[] _data;
    }

private:
    uint8_t* _data;
    size_t _size;
    size_t _capacity;
};
