#pragma once

#include "bitstream.hpp"
#include <vector>
#include <cstddef>
#include <cstdint>

namespace lzxd::detail {

struct Window {
    std::vector<uint8_t> data;
    size_t position;

    void push(uint8_t byte);
    void advance(size_t by);
    void copyFromSelf(size_t offset, size_t length);
    void copyFromBitstream(BitStream& stream, size_t length);
    std::vector<uint8_t> pastView(size_t len);

    Window(size_t size) : data(size), position(0) {}
};

} // namespace lzxd::detail