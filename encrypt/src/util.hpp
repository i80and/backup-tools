// util.hpp - Part of WuffCrypt
// Copyright 2014 Andrew Aldridge <i80and@foxquill.com>

#pragma once

#include <stdint.h>
#include <stdio.h>

int __fail(const char* file, int line, const char* msg);

#define verify(x) ((x)? 0: __fail(__FILE__, __LINE__, #x))

namespace byteorder {
    enum class ByteOrder {
        LittleEndian,
        BigEndian
    };

    inline uint32_t fromLittleEndian(uint32_t x) {
        uint8_t* data = reinterpret_cast<uint8_t*>(&x);
        return (data[0]<<0) | (data[1]<<8) | (data[2]<<16) | (data[3]<<24);
    }

    inline uint32_t fromBigEndian(uint32_t x) {
        uint8_t* data = reinterpret_cast<uint8_t*>(&x);
        return (data[3]<<0) | (data[2]<<8) | (data[1]<<16) | (data[0]<<24);
    }
}
