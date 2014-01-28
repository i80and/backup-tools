// wuffcrypt.cpp - Part of WuffCrypt
// Copyright 2014 Andrew Aldridge <i80and@foxquill.com>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string>

#include <crypto_scrypt.h>

#include "wuffcrypt.hpp"
#include "util.hpp"

void kdf(const std::string& password, int workFactor, uint8_t* outBuf, size_t bufLen) {
    int result = crypto_scrypt(reinterpret_cast<const uint8_t*>(password.data()), password.size(),
                               nullptr, 0, static_cast<int>(powf(2, workFactor)), 8, 1, outBuf, bufLen);

    verify(result == 0);
}

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

WuffCryptFile::FileStatus WuffCryptFile::read(std::function<void(const SodiumMessageBuffer& msg)> blockHandler, const SecureString& password) const {
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

    // Decrypt each block, and feed it into the blockHandler
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

WuffCryptFile::FileStatus WuffCryptFile::write(std::function<void(uint8_t* outBuf, size_t& outBufWritten, size_t blockSize)> blockFeeder, const SecureString& password) {
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

    return FileStatus::OK;
}
