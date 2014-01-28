// main.cpp - Part of WuffCrypt
// Copyright 2014 Andrew Aldridge <i80and@foxquill.com>

#include <stdio.h>
#include "arguments.hpp"
#include "wuffcrypt.hpp"

void printUsage(const char* path) {
    printf("wuffcrypt %s\n", WUFFCRYPT_VERSION);
    printf("Usage: %s [-d | -e] -p [password] infile outfile\n", path);
    printf("\t-d: Decrypt\n");
    printf("\t-e: Encrypt\n");
    printf("\t-p: Password.  If this argument is missing, a prompt will be offered.\n");
}

void printUsageError(const char* path, const char* msg) {
    printf("%s\n\n", msg);
    printUsage(path);
    exit(1);
}

int main(int argc, char** argv) {
    printf("\nwuffcrypt is experimental software; while it is belived to provide\n"
           "best-of-breed encryption, it has not undergone any third-party vetting\n"
           "or peer-review process.  It is therefore suggested that you use it only in\n"
           "circumstances where you are willing to accept the cost of faulty functioning.\n\n");

    Arguments args;
    auto argStatus = args.parse(argc, argv);

    switch(argStatus) {
        case Arguments::Status::UnknownOption: {
            printUsageError(argv[0], "Unknown option");
            break;
        }
        case Arguments::Status::NoPath: {
            printUsageError(argv[0], "No path provided");
            break;
        }
        case Arguments::Status::OK: {
            break;
        }
    }

    if(args.showHelp()) {
        printUsage(argv[0]);
        return 0;
    }

    if(sodium_init() < 0) {
        fprintf(stderr, "Error initializing Sodium\n");
        return 1;
    }

    if(args.operation() == Operation::None) {
        printUsageError(argv[0], "No operation provided");
    }

    if(args.password().empty()) {
        printUsageError(argv[0], "No password provided");
    }

    if(args.operation() == Operation::Encrypt) {
        FILE* inFile = fopen(args.inPath().c_str(), "rb");
        if(inFile == nullptr) {
            fprintf(stderr, "Error opening %s\n", args.inPath().c_str());
            return 1;
        }

        WuffCryptFile outFile(args.outPath());

        auto status = outFile.write([&inFile](uint8_t* outBuf, size_t& outBufWritten, size_t blockSize) {
            outBufWritten = fread(outBuf, sizeof(uint8_t), blockSize, inFile);
        }, args.password());

        if(status == WuffCryptFile::FileStatus::OpenError) {
            fprintf(stderr, "Error opening %s\n", args.outPath().c_str());
            return 1;
        }

        fclose(inFile);
    }
    else if(args.operation() == Operation::Decrypt) {
        FILE* outFile = fopen(args.outPath().c_str(), "wb");
        if(outFile == nullptr) {
            fprintf(stderr, "Error opening %s\n", args.outPath().c_str());
            return 1;
        }
        WuffCryptFile inFile(args.inPath());

        auto status = inFile.read([&outFile](const SodiumMessageBuffer& msg) {
            fwrite(msg.data(), sizeof(uint8_t), msg.size(), outFile);
        }, args.password());

        switch(status) {
            case WuffCryptFile::FileStatus::OpenError: {
                fprintf(stderr, "Error opening %s.\n", args.inPath().c_str());
                return 1;
            }
            case WuffCryptFile::FileStatus::InvalidFileType: {
                fprintf(stderr, "%s is not a wuffcrypt file.\n", args.inPath().c_str());
                return 1;
            }
            case WuffCryptFile::FileStatus::CorruptHeader: {
                fprintf(stderr, "%s is a corrupt wuffcrypt file.\n", args.inPath().c_str());
                return 1;
            }
            case WuffCryptFile::FileStatus::VerificationFailed: {
                fprintf(stderr, "Failed to decrypt.  Either the password is wrong, or the file has been tampered with in some way.\n");
                return 1;
            }
            case WuffCryptFile::FileStatus::WrongVersion: {
                fprintf(stderr, "File version mismatch\n");
                return 1;
            }
            case WuffCryptFile::FileStatus::OK: { break; }
        }

        fclose(outFile);
    }

    return 0;
}
