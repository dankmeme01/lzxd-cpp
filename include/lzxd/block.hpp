#pragma once

#include "bitstream.hpp"
#include "tree.hpp"
#include <variant>

namespace lzxd {
namespace detail {
    struct DecodedSingle {
        uint8_t value;
    };

    struct DecodedMatch {
        size_t offset, length;
    };

    struct DecodedRead {
        size_t value;
    };

    using DecodedPart = std::variant<DecodedSingle, DecodedMatch, DecodedRead>;
}

enum class BlockType {
    Invalid, Verbatim, Aligned, Uncompressed
};

struct BlockHeader {
    BlockType type;
    uint32_t size;     // uncompressed size
};

struct BaseBlock {
    uint32_t size, remaining;
};

struct UncompressedBlock : public BaseBlock {
    uint32_t r0, r1, r2;
};

struct VerbatimBlock : BaseBlock {
    Tree mainTree;
    std::optional<Tree> lengthTree;
};

struct AlignedOffsetBlock : VerbatimBlock {
    Tree alignedOffsetTree;
};

using BlockData = std::variant<VerbatimBlock, AlignedOffsetBlock, UncompressedBlock>;

class Block {
public:
    Block(UncompressedBlock block) : m_data(block) {}
    Block(VerbatimBlock block) : m_data(block) {}
    Block(AlignedOffsetBlock block) : m_data(block) {}

    detail::DecodedPart decodeElement(BitStream& stream, uint32_t& r0, uint32_t& r1, uint32_t& r2) const;

    BlockType type() const;

    uint32_t& remaining();
    size_t size() const;

private:
    BlockData m_data;

};

BlockHeader readBlockHeader(BitStream& stream);

} // namespace lzxd