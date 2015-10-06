// wuffcrypt.hpp - Part of WuffCrypt
// Copyright 2014 Andrew Aldridge <i80and@foxquill.com>

#pragma once

#include <functional>
#include <stdint.h>
#include <sodium.h>
#include "paddedbuffer.hpp"
#include "securestring.hpp"

void kdf(const std::string& password, int workFactor, uint8_t* outBuf, size_t bufLen);

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

    explicit WuffCryptFile(const std::string& path): _path(path) {}

    FileStatus read(std::function<void(const SodiumMessageBuffer& msg)> blockHandler, const SecureString& password) const;
    FileStatus write(std::function<void(uint8_t* outBuf, size_t& outBufWritten, size_t blockSize)> blockFeeder, const SecureString& password);

private:
    const std::string _path;
};
