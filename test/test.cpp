#include <lzxd/lzxd.hpp>
#include <lzxd/error.hpp>
#include <lzxd/ldi.hpp>
#include <iostream>
#include <filesystem>
#include <fstream>

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

        lzxd::Decoder decoder(0x8000);
        auto decompressed = decoder.decompressChunk(data, 3);

        auto dec = std::string(decompressed.begin(), decompressed.end());
        LZXD_ASSERT(dec == "abc");
    }();
}

int main(int argc, const char** argv) {
    // If a path is passed as the first argument, decode it and save it.

    // For debugger
    const char* myargv[] = {
        argv[0], "./testdata/1-src.bin"
    };
    argv = myargv;
    argc = 2;

    if (argc > 1) {
        std::filesystem::path path = argv[1];

        size_t windowSize;
        if (argc > 2) {
            windowSize = std::stoul(argv[2]);
        } else {
            windowSize = 0x8000;
        }

        // Read the file
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open file " << path << std::endl;
            return 1;
        }

        std::vector<uint8_t> data(std::istreambuf_iterator<char>(file), {});

        lzxd::Decoder decoder(windowSize);
        auto decompressed = decoder.decompressChunk(data, 0x8000);

        // Write file
        auto outpath = path.replace_extension(".out");
        std::ofstream outfile(outpath, std::ios::binary);
        if (!outfile) {
            std::cerr << "Failed to open file " << outpath << std::endl;
            return 1;
        }

        outfile.write(reinterpret_cast<const char*>(decompressed.data()), decompressed.size());

        return 0;
    }

    testBitBuffer();
    testDecoder();

    return 0;
}
