#pragma once

#include "bitstream.hpp"

namespace lzxd {

enum class BlockType {
    Invalid, Verbatim, Aligned, Uncompressed
};

struct Block {
    BlockType type;
    size_t size;     // uncompressed size
};

// Block readBlock


} // namespace lzxd