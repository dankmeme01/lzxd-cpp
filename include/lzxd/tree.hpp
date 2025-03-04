#pragma once

#include "bitstream.hpp"
#include <vector>
#include <cstdint>
#include <cstddef>
#include <optional>
#include <algorithm>

namespace lzxd {

class CanonicalTree;

class Tree {
public:
    std::vector<uint8_t> m_lengths;
    std::vector<uint16_t> m_huffmanCodes;
    uint8_t m_largestLength;

    static Tree fromPathLengths(std::vector<uint8_t> lengths);

    uint16_t decodeElement(BitStream& stream) const;
};

class CanonicalTree {
public:
    std::vector<uint8_t> m_lengths;

    CanonicalTree(std::vector<uint8_t> lengths) : m_lengths(std::move(lengths)) {}

    std::optional<Tree> createInstance() const;
    void updateRangeWithPretree(BitStream& stream, size_t start, size_t end);
};


} // namespace lzxd