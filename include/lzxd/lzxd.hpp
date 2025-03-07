#pragma once

#include "block.hpp"
#include "tree.hpp"
#include "window.hpp"
#include <optional>

namespace lzxd {
namespace detail {
    class E8Translator {
    public:
        E8Translator(int32_t translationSize) : translationSize(translationSize) {}

    private:
        int32_t translationSize;
    };

    size_t positionSlotsFor(size_t windowSize);
} // namespace detail

class Decoder {
public:
    Decoder(size_t windowSize);
    Decoder();

    std::vector<uint8_t> decompressChunk(const std::vector<uint8_t>& data, size_t outputSize = 32768);
    std::vector<uint8_t> decompressChunk(const uint8_t* data, size_t size, size_t outputSize = 32768);

    size_t decompressChunkInto(const std::vector<uint8_t>& data, uint8_t* output, size_t outputSize = 32768);
    size_t decompressChunkInto(const uint8_t* data, size_t size, uint8_t* output, size_t outputSize = 32768);

    void reset();

private:
    size_t windowSize;
    size_t decodedChunks = 0;
    detail::Window window;
    CanonicalTree mainTree;
    CanonicalTree lengthTree;
    uint32_t r0 = 1, r1 = 1, r2 = 1;
    size_t chunkOffset = 0;
    Block currentBlock;

    std::optional<detail::E8Translator> e8Translator;

    void firstChunk(BitStream& stream);

    Block readBlock(BitStream& stream, const BlockHeader& header);
    void readMainAndLengthTrees(BitStream& stream);
};

} // namespace lzxd