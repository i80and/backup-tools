backup-tools
============
Simple command-line helper utilities that can fit into a backup system.

USE AT YOUR OWN RISK.

Tools
=====
  * backup
    * Can back up to Amazon S3, list archives, prune old archives, and restore archives.
  * wuffcrypt
    * Extremely fast and secure symmetric encryption tool using libsodium (derived from NaCl) with scrypt for key derivation.  This tool has not undergone peer-review, so treat it as the experimental software that it is.

Building wuffcrypt
==================
The following configurations have been tested and are known to work:
  * Linux x86_64
    * gcc 4.7+
    * clang 3.2+
  * Mac OS X
    * Apple LLVM 8.0.0
  * OpenBSD 6.0
    * GCC 4.9
  * Windows/MSYS
    * gcc 4.8
    * clang 3.4
  * Haiku
    * gcc 4.7

To build, run from the source root:
```
    $ cd encrypt
    $ make
    $ make test
```

Windows users will have to use `cmake .. -G "MSYS Makefiles"`, and Haiku users must force GCC 4.x:
```
    $ export CC=gcc-x86
    $ export CXX=g++-x86
```
