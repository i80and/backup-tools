// util.cpp - Part of WuffCrypt
// Copyright 2014 Andrew Aldridge <i80and@foxquill.com>

#include <stdlib.h>
#include "util.hpp"

int __fail(const char* file, int line, const char* msg) {
    fprintf(stderr, "%s:%d: Assert failed (%s)\n", file, line, msg);
    exit(1);

    return 0;
}
