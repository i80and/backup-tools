// arguments.hpp - Part of WuffCrypt
// Copyright 2014 Andrew Aldridge <i80and@foxquill.com>

#pragma once

#include <string>
#include "securestring.hpp"

enum class Operation {
    None,
    Encrypt,
    Decrypt
};

class Arguments {
public:
    enum class Status {
        OK,
        UnknownOption,
        NoPath
    };

    Arguments(): _showHelp(false), _operation(Operation::None) {}

    Status parse(int argc, char** argv);

    Operation operation() const { return _operation; }
    const std::string& inPath() const { return _inPath; }
    const std::string& outPath() const { return _outPath; }
    const SecureString& password() const { return _password; }
    bool showHelp() const { return _showHelp; }

private:
    bool _showHelp;
    SecureString _password;
    std::string _inPath;
    std::string _outPath;
    Operation _operation;
};
