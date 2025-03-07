# lzxd-cpp

Implementation of Microsoft's LZX-DELTA compression algorithm, the specification of which can be found [here](https://learn.microsoft.com/en-us/openspecs/exchange_server_protocols/ms-patch/cc78752a-b4af-4eee-88cb-01f4d8a4c2bf). This library only supports decompression.

This library is essentially an unofficial C++ port of the Rust [lzxd crate](https://github.com/Lonami/lzxd) - please send all of the credit towards them. A good portion of the code is taken 1:1, with exceptions being some stuff that wasn't very C++-idiomatic.
