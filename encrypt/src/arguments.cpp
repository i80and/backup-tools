// arguments.cpp - Part of WuffCrypt
// Copyright 2014 Andrew Aldridge <i80and@foxquill.com>

#include <string.h>
#include <vector>
#include "arguments.hpp"
#include "securestring.hpp"

Arguments::Status Arguments::parse(int argc, char** argv) {
    std::vector<char*> plainArgs;

    enum class ParseMode {
        None,
        Password
    } mode = ParseMode::None;

    for(int i = 1; i < argc; i += 1) {
        if(mode != ParseMode::None) {
            switch(mode) {
                case ParseMode::None: { break; }
                case ParseMode::Password: {
                    SecureString(argv[i]).moveInto(_password);
                    break;
                }
            }

            mode = ParseMode::None;
            continue;
        }

        if(strcmp(argv[i], "-e") == 0) {
            _operation = Operation::Encrypt;
        }
        else if(strcmp(argv[i], "-d") == 0) {
            _operation = Operation::Decrypt;
        }
        else if(strcmp(argv[i], "-p") == 0) {
            mode = ParseMode::Password;
        }
        else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            _showHelp = true;
            return Status::OK;
        }
        else if(argv[i][0] == '-') {
            return Status::UnknownOption;
        }
        else {
            plainArgs.push_back(argv[i]);
        }
    }

    if(plainArgs.size() < 2) {
        return Status::NoPath;
    }

    if(plainArgs.size() > 3) {
        return Status::UnknownOption;
    }

    _inPath = plainArgs[0];
    _outPath = plainArgs[1];

    return Status::OK;
}
