// wuffcrypt.cpp - Part of WuffCrypt
// Copyright 2014 Andrew Aldridge <i80and@foxquill.com>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <functional>
#include <math.h>
#include <string>
#include <vector>

#include <crypto_scrypt.h>
#include <sodium.h>

int __fail(const char* file, int line, const char* msg) {
    fprintf(stderr, "%s:%d: Assert failed (%s)\n", file, line, msg);
    exit(1);

    return 0;
}

#define verify(x) ((x)? 0: __fail(__FILE__, __LINE__, #x))

enum class Operation {
    None,
    Encrypt,
    Decrypt
};

namespace byteorder {
    enum class ByteOrder {
        LittleEndian,
        BigEndian
    };

    uint32_t fromLittleEndian(uint32_t x) {
        uint8_t* data = reinterpret_cast<uint8_t*>(&x);
        return (data[0]<<0) | (data[1]<<8) | (data[2]<<16) | (data[3]<<24);
    }

    uint32_t fromBigEndian(uint32_t x) {
        uint8_t* data = reinterpret_cast<uint8_t*>(&x);
        return (data[3]<<0) | (data[2]<<8) | (data[1]<<16) | (data[0]<<24);
    }
}

class SecureString {
public:
    SecureString(): _data(nullptr), _len(0) {}

    SecureString(char* str): _len(strlen(str)) {
        _data = new char[_len+1];
        strcpy(_data, str);

        // Clear our original source
        sodium_memzero(str, _len);
    }

    SecureString(size_t len): _len(len) {
        _data = new char[_len+1];
        _data[_len] = '\0';
    }

    SecureString(uint8_t* buf, size_t len): _len(len) {
        // Copy buf into a buffer we control
        _data = new char[_len+1];
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

void printUsage(const char* path) {
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

void kdf(const std::string& password, int workFactor, uint8_t* outBuf, size_t bufLen) {
    int result = crypto_scrypt(reinterpret_cast<const uint8_t*>(password.data()), password.size(),
                               nullptr, 0, static_cast<int>(powf(2, workFactor)), 8, 1, outBuf, bufLen);

    verify(result == 0);
}

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
typedef PaddedBuffer<crypto_secretbox_xsalsa20poly1305_ZEROBYTES,0> SodiumMessageBuffer;
typedef PaddedBuffer<crypto_secretbox_xsalsa20poly1305_BOXZEROBYTES,crypto_secretbox_xsalsa20poly1305_BOXZEROBYTES> SodiumEncryptedBuffer;

#define encrypt_NONCEPREFIXBYTES (crypto_secretbox_xsalsa20poly1305_NONCEBYTES-sizeof(uint32_t))
class Encrypter {
public:
    Encrypter(const SecureString& password, int workFactor): _key(crypto_secretbox_xsalsa20poly1305_KEYBYTES) {
        randombytes_buf(_nonce, sizeof(_nonce));
        kdf(password.c_str(), workFactor, _key.data(), _key.size());
    }

    void encrypt(const SodiumMessageBuffer& msg, SodiumEncryptedBuffer& ctext, uint32_t n) {
        uint8_t nonce[crypto_secretbox_xsalsa20poly1305_NONCEBYTES];
        memcpy(nonce, _nonce, sizeof(_nonce));

        *reinterpret_cast<uint32_t*>((nonce + sizeof(_nonce))) = n;

        crypto_secretbox_xsalsa20poly1305(ctext.rawData(), msg.rawData(), msg.rawSize(), nonce, _key.data());
        ctext.setSize(msg.size() + ctext.padding());
    }

    const uint8_t* noncePrefix() const {
        return _nonce;
    }

private:
    SecureString _key;
    uint8_t _nonce[encrypt_NONCEPREFIXBYTES];
};

class Decrypter {
public:
    Decrypter(const SecureString& password, const uint8_t* nonce, int workFactor): _key(crypto_secretbox_xsalsa20poly1305_KEYBYTES) {
        memcpy(_nonce, nonce, sizeof(_nonce));
        kdf(password.c_str(), workFactor, _key.data(), _key.size());
    }

    int decrypt(const SodiumEncryptedBuffer& ctext, SodiumMessageBuffer& msg, uint32_t n) {
        uint8_t nonce[crypto_secretbox_xsalsa20poly1305_NONCEBYTES];
        memcpy(nonce, _nonce, sizeof(_nonce));
        *reinterpret_cast<uint32_t*>(nonce + sizeof(_nonce)) = n;

        int status = crypto_secretbox_xsalsa20poly1305_open(msg.rawData(), ctext.rawData(), ctext.rawSize(), nonce, _key.data());
        if(status != 0) {
            // The message verification failed; the ciphertext has been tampered with
            msg.setSize(0);
            return 1;
        }

        msg.setSize(ctext.size() - ctext.padding());
        return 0;
    }

private:
    SecureString _key;
    uint8_t _nonce[encrypt_NONCEPREFIXBYTES];
};

class File {
public:
    File(const std::string& path, const std::string& options) {
        _f = fopen(path.c_str(), options.c_str());
    }

    template <typename T>
    size_t readValue(T& out) const {
        if(_f == nullptr) return 0;

        size_t valuesRead = fread(&out, sizeof(T), 1, _f);
        return valuesRead;
    }

    FILE* handle() {
        return _f;
    }

    ~File() {
        if(_f != nullptr) {
            fclose(_f);
        }
    }

private:
    FILE* _f;
};

class WuffCryptFile {
public:
    static const uint8_t VERSION = 0;
    static const uint8_t WORK_FACTOR = 17;
    static const size_t BLOCK_SIZE = 1024*1024;

    enum class FileStatus {
        OK,
        OpenError,
        InvalidFileType,
        CorruptHeader,
        VerificationFailed,
        WrongVersion
    };

    WuffCryptFile(const std::string& path): _path(path) {}

    FileStatus read(std::function<void(const SodiumMessageBuffer& msg)> blockHandler, const SecureString& password) const {
        File f(_path, "rb");
        if(f.handle() == nullptr) return FileStatus::OpenError;

        byteorder::ByteOrder byteOrder;

        // Check the file type and determine byte order
        {
            const size_t headerLength = 9;

            char header[10] = {0};
            size_t bytesRead = fread(header, sizeof(char), headerLength, f.handle());
            if(bytesRead < headerLength) {
                return FileStatus::InvalidFileType;
            }

            if(strcmp(header, "wuffcrypt") == 0) {
                byteOrder = byteorder::ByteOrder::LittleEndian;
            }
            else if(strcmp(header, "wuffcrytp") == 0) {
                byteOrder = byteorder::ByteOrder::BigEndian;
            }
            else {
                return FileStatus::InvalidFileType;
            }
        }

        // Check if the file format version is right
        uint8_t version = 0;

        // Read in the work factor
        uint8_t workFactor = 0;

        // Read in the nonce prefix
        uint8_t nonce[encrypt_NONCEPREFIXBYTES] = {0};

        bool headerOK = f.readValue(version)
            && f.readValue(workFactor)
            && f.readValue(nonce);

        if(!headerOK) {
            return FileStatus::CorruptHeader;
        }

        if(version != VERSION) {
            return FileStatus::WrongVersion;
        }

        // Decrypt each 10mb block, and feed it into the blockHandler
        Decrypter dec(password, nonce, workFactor);
        SodiumEncryptedBuffer buf(BLOCK_SIZE);
        SodiumMessageBuffer decBuf(BLOCK_SIZE);

        // Each encrypted block has an additional handful of bytes alongside it.
        const size_t encryptedBlockSize = BLOCK_SIZE + crypto_secretbox_xsalsa20poly1305_BOXZEROBYTES;

        uint32_t n = 0;
        size_t bytesRead = encryptedBlockSize;
        while(bytesRead == encryptedBlockSize) {
            bytesRead = fread(buf.data(), sizeof(uint8_t), encryptedBlockSize, f.handle());
            buf.setSize(bytesRead);

            // Because the nonce used will vary with system endianness, we have to adapt ourselves
            // to whatever platform created the file
            uint32_t endianN = (byteOrder == byteorder::ByteOrder::LittleEndian)? byteorder::fromLittleEndian(n) : byteorder::fromBigEndian(n);
            n += 1;

            int status = dec.decrypt(buf, decBuf, endianN);
            if(status != 0) {
                // Verification failed
                return FileStatus::VerificationFailed;
            }

            blockHandler(decBuf);
        }

        return FileStatus::OK;
    }

    FileStatus write(std::function<void(uint8_t* outBuf, size_t& outBufWritten, size_t blockSize)> blockFeeder, const SecureString& password) {
        File f(_path, "wb");
        if(f.handle() == nullptr) return FileStatus::OpenError;

        {
            uint16_t byteOrderIndicator = 0x7470;
            uint8_t* byteOrderIndicatorLens = reinterpret_cast<uint8_t*>(&byteOrderIndicator);
            fprintf(f.handle(), "wuffcry%c%c", byteOrderIndicatorLens[0], byteOrderIndicatorLens[1]);
        }

        Encrypter enc(password, WORK_FACTOR);

        {
            uint8_t version = VERSION;
            uint8_t workFactor = WORK_FACTOR;

            // Write the header parameters
            fwrite(&version, sizeof(version), 1, f.handle());
            fwrite(&workFactor, sizeof(workFactor), 1, f.handle());
            fwrite(enc.noncePrefix(), sizeof(uint8_t), encrypt_NONCEPREFIXBYTES, f.handle());
        }

        SodiumMessageBuffer buf(BLOCK_SIZE);
        SodiumEncryptedBuffer encBuf(BLOCK_SIZE);
        uint32_t n = 0;

        // Get data from the blockFeeder, encrypt it, and write it out until the blockFeeder provides
        // a partial block
        while(true) {
            size_t bufLen = 0;

            blockFeeder(buf.data(), bufLen, BLOCK_SIZE);
            buf.setSize(bufLen);

            enc.encrypt(buf, encBuf, n);
            fwrite(encBuf.data(), sizeof(uint8_t), encBuf.size(), f.handle());

            if(bufLen < BLOCK_SIZE) break;

            n += 1;
        }

        // Write a trailing backup copy of the nonce, to provide some avenue for partial recovery if
        // the file is corrupted XXX IMPLEMENT ME
        // fwrite(enc.noncePrefix(), sizeof(uint8_t), encrypt_NONCEPREFIXBYTES, f.handle());

        return FileStatus::OK;
    }

private:
    const std::string _path;
};

class Arguments {
public:
    enum class Status {
        OK,
        UnknownOption,
        NoPath
    };

    Arguments(): _showHelp(false), _operation(Operation::None) {}

    Status parse(int argc, char** argv) {
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

    Operation operation() const {
        return _operation;
    }

    const std::string& inPath() const {
        return _inPath;
    }

    const std::string& outPath() const {
        return _outPath;
    }

    const SecureString& password() const {
        return _password;
    }

    bool showHelp() const {
        return _showHelp;
    }

private:
    bool _showHelp;
    SecureString _password;
    std::string _inPath;
    std::string _outPath;
    Operation _operation;
};

int main(int argc, char** argv) {
    // Disclaimer
    fprintf(stderr, "====== DANGER DANGER Experimental software; do NOT trust DANGER DANGER ======\n\n");

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
