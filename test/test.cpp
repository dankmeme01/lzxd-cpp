#include <lzxd/lzxd.hpp>
#include <lzxd/error.hpp>
#include <lzxd/ldi.hpp>
#include <iostream>
#include <filesystem>
#include <fstream>

#if defined(_MSC_VER) && !defined(__clang__)
# include <stdlib.h>
# define BSWAP16(val) _byteswap_ushort(val)
# define BSWAP32(val) _byteswap_ulong(val)
# define BSWAP64(val) _byteswap_uint64(val)
#else
# define BSWAP16(val) (uint16_t)((val >> 8) | (val << 8))
# define BSWAP32(val) __builtin_bswap32(val)
# define BSWAP64(val) __builtin_bswap64(val)
#endif

void testBitBuffer() {
    []{
        // Test readU32le
        lzxd::BitStream stream({0x56, 0x78, 0x12, 0x34});
        LZXD_ASSERT(stream.readU32le() == 873625686);
    }();

    [] {
        // Test readU24be
        lzxd::BitStream stream({0b00011000, 0b00001100, 0b00110000, 0b00011000});
        LZXD_ASSERT(stream.readBits(4) == 0);
        LZXD_ASSERT(stream.readU24be() == 0b1100'0001'1000'0001'1000'0011);
        LZXD_ASSERT(stream.readBits(4) == 0);
    }();

    [] {
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
    }();

    []{
        // Read of basic block
        std::vector<uint8_t> exampleData = {
            0x00, 0x30, 0x30, 0x00,
            0x01,0x00, 0x00, 0x00,
            0x01, 0x00, 0x00, 0x00,
            0x01, 0x00, 0x00, 0x00,
            0x61, 0x62, 0x63, 0x00
        };

        lzxd::BitStream bs{std::move(exampleData)};
        LZXD_ASSERT(bs.readBit() == false); // E8 translation
        auto block = lzxd::readBlockHeader(bs);

        LZXD_ASSERT(block.type == lzxd::BlockType::Uncompressed);
        LZXD_ASSERT(block.size == 3);

        bs.align();

        LZXD_ASSERT(bs.readU32le() == 1); // R0
        LZXD_ASSERT(bs.readU32le() == 1); // R1
        LZXD_ASSERT(bs.readU32le() == 1); // R2

        auto content = bs.readBytes(block.size);

        auto cnt = std::string(content.begin(), content.end());

        LZXD_ASSERT(cnt == "abc");
    }();
}

void testDecoder() {
    []{
        std::vector<uint8_t> data = {
            0x00, 0x30, 0x30, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
            0x00, 0x00, 'a', 'b', 'c', 0x00,
        };

        lzxd::Decoder decoder(0x80000);
        auto decompressed = decoder.decompressChunk(data, 3);

        auto dec = std::string(decompressed.begin(), decompressed.end());
        LZXD_ASSERT(dec == "abc");
    }();
}

void decodeBlock(std::filesystem::path path) {
    constexpr size_t windowSize = 0x80000;

    // Read the file
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file " << path << std::endl;
        return;
    }

    std::vector<uint8_t> data(std::istreambuf_iterator<char>(file), {});

    lzxd::Decoder decoder(windowSize);
    auto decompressed = decoder.decompressChunk(data, 32768);

    // Write file
    auto outpath = path.replace_extension(".out");
    std::ofstream outfile(outpath, std::ios::binary);
    if (!outfile) {
        std::cerr << "Failed to open file " << outpath << std::endl;
        return;
    }

    std::cout << "Saving " << decompressed.size() << " bytes to " << outpath << std::endl;

    outfile.write(reinterpret_cast<const char*>(decompressed.data()), decompressed.size());
    outfile.close();
}

void decodeDatabase(std::filesystem::path path) {
    // Decode a .gmsodf database

    // Read the file
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file " << path << std::endl;
        return;
    }

    std::vector<uint8_t> data(std::istreambuf_iterator<char>(file), {});

    uint16_t magic = BSWAP16(*reinterpret_cast<uint16_t*>(data.data()));
    if (magic != 0x5c42) {
        std::cerr << "Invalid magic number" << std::endl;
        return;
    }

    uint16_t flags = BSWAP16(*reinterpret_cast<uint16_t*>(data.data() + 2)); // no idea what this really is, usually 00 01
    uint16_t headerSize = BSWAP16(*reinterpret_cast<uint16_t*>(data.data() + 4));
    uint32_t remainingUncBytes = BSWAP32(*reinterpret_cast<uint32_t*>(data.data() + 6));

    size_t dataBeginOffset = headerSize;

    size_t currentPos = dataBeginOffset;
    constexpr size_t windowSize = 0x80000;

    lzxd::Decoder decoder(windowSize);

    std::vector<uint8_t> output;

    while (currentPos < data.size()) {
        // read the next chunk
        // uncompressed size is always equal to 32768, except for the last chunk.
        // compressed size is available as a 16-bit value at the beginning of the chunk
        uint16_t compressedSize = BSWAP16(*reinterpret_cast<uint16_t*>(data.data() + currentPos));
        currentPos += 2;

        std::cout << "Chunk of size " << compressedSize << std::endl;

        // if we are on the last chunk, the decompressed size of this chunk is `remainingUncBytes`
        size_t decSize = 32768;
        if (remainingUncBytes < decSize) {
            decSize = remainingUncBytes;
        }

        auto dechunk = decoder.decompressChunk(data.data() + currentPos, compressedSize, decSize);
        output.insert(output.end(), dechunk.begin(), dechunk.end());

        currentPos += compressedSize;
        remainingUncBytes -= decSize;
    }

    // write to file
    std::ofstream outfile(path.replace_extension(".gmsodf.raw"), std::ios::binary);
    outfile.write(reinterpret_cast<const char*>(output.data()), output.size());
    outfile.close();
}

int main(int argc, const char** argv) {
    // waa

    if (argc > 2) {
        std::string operation = argv[1];
        if (operation == "decode-block") {
            std::filesystem::path path = argv[2];
            decodeBlock(path);
            return 0;
        } else if (operation == "decode-database") {
            std::filesystem::path path = argv[2];
            decodeDatabase(path);
            return 0;
        }
    }

    testBitBuffer();
    testDecoder();

    return 0;
}
