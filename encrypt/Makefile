VERSION=0.1.1-pre-`git rev-parse --short HEAD`

FLAGS=-Wall -Wextra -Wshadow -O2 -fstack-protector-all -DWUFFCRYPT_VERSION=\"$(VERSION)\"
CFLAGS=$(FLAGS) -std=c99 -fPIC `pkg-config --cflags libsodium`
CXXFLAGS=$(FLAGS) -std=c++11 -fpie -lsodium `pkg-config --cflags --libs libsodium`

SRC=src/arguments.cpp \
    src/main.cpp \
    src/util.cpp \
    src/wuffcrypt.cpp \

SRC_SCRYPT=src/thirdparty/scrypt/crypto_scrypt-ref.c \
           src/thirdparty/scrypt/sha256.c
OBJ_SCRYPT=$(SRC_SCRYPT:.c=.o)

.PHONY: clean test lint

wuffcrypt: $(SRC) $(OBJ_SCRYPT)
	$(CXX) $(CXXFLAGS) -o $@ -I src/thirdparty/scrypt $(SRC) $(OBJ_SCRYPT)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -Rf wuffcrypt
	find ./src -name "*.o" -exec rm {} \;

lint:
	cppcheck --enable=all --inconclusive $(SRC)
