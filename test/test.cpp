#include <lzxd/lzxd.hpp>
#include <lzxd/error.hpp>
#include <lzxd/ldi.hpp>
#include <iostream>

void testBitBuffer() {
    std::vector<uint8_t> exampleData = {
        0b010'00001,
        0b0110'0001,
        0b0001'1100,
        0b0000'0000,
    };

    lzxd::BitStream stream1(exampleData);
    LZXD_ASSERT(stream1.readBit() == false);
    LZXD_ASSERT(stream1.readBit() == true);
    LZXD_ASSERT(stream1.readBit() == true);
    LZXD_ASSERT(stream1.readBit() == false);
    LZXD_ASSERT(stream1.readBit() == false);
    LZXD_ASSERT(stream1.readBit() == false);
    LZXD_ASSERT(stream1.readBit() == false);
    LZXD_ASSERT(stream1.readBit() == true);

    LZXD_ASSERT(stream1.readBits(4) == 0b0100);
    LZXD_ASSERT(stream1.readBits(4) == 0b0001);
    LZXD_ASSERT(stream1.readBits(8) == 0);
    LZXD_ASSERT(stream1.readBits(8) == 0b0001'1100);

    LZXD_ASSERT(stream1.eof());
}

int main() {
    testBitBuffer();

    std::vector<uint8_t> exampleData = {
        0b010'00001,
        0b0110'0001,
        0b0001'1100,
        0b0000'0000,
    };

    lzxd::BitStream bs{std::move(exampleData)};
    std::cout << "BS size: " << bs.size() << std::endl;
    auto block = lzxd::readBlockHeader(bs);

    std::cout << "Block type: " << static_cast<int>(block.type) << std::endl;
    std::cout << "Block size: " << block.size << std::endl;
    std::cout << "Expected block size: " << 0b00000000'00001011'00001000'11100000 << std::endl;

    std::vector<uint8_t> data = {
        0x00, 0x30, 0x30, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
        0x00, 0x00, 'a', 'b', 'c', 0x00,
    };

    lzxd::Decoder decoder(0x8000);
    auto decompressed = decoder.decompressChunk(data);
    std::cout << "Decompressed size: " << decompressed.size() << std::endl;
    std::cout << "Decompressed data: " << std::string(decompressed.begin(), decompressed.end()) << std::endl;

    return 0;

    // TODO: byteswap this all ...
    // the bitstream should read the input as 16-bit LE integers, like the rust crate
    // 0b00000000'00110000'00110000'00000000
}
