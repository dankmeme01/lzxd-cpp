#include <lzxd/tree.hpp>
#include <lzxd/error.hpp>

namespace lzxd {

Tree Tree::fromPathLengths(std::vector<uint8_t> lengths) {
    CanonicalTree ctree(lengths);
    return ctree.createInstance().value();
}

uint16_t Tree::decodeElement(BitStream& stream) const {
    // Perform the inverse translation, peeking as many bits as our tree is…
    uint32_t idx = stream.peekBits<uint32_t>(this->m_largestLength);
    auto code = this->m_huffmanCodes[idx];

    // …and then advancing the stream by the length of the code
    stream.readBits(this->m_lengths[code]);

    return code;
}

std::optional<Tree> CanonicalTree::createInstance() const {
    if (this->m_lengths.empty()) {
        return std::nullopt;
    }

    Tree tree;
    tree.m_lengths = this->m_lengths;
    tree.m_largestLength = *std::max_element(this->m_lengths.begin(), this->m_lengths.end());
    tree.m_huffmanCodes = std::vector<uint16_t>(1 << tree.m_largestLength);

    size_t pos = 0;
    for (uint8_t bit = 1; bit <= tree.m_largestLength; bit++) {
        size_t amount = 1 << (tree.m_largestLength - bit);

        for (size_t code = 0; code < this->m_lengths.size(); code++) {
            // As soon as a code's path length matches with our bit index write the code as
            // many times as the bit index itself represents.
            if (this->m_lengths[code] == bit) {
                for (size_t i = 0; i < amount; i++) {
                    tree.m_huffmanCodes[pos++] = code;
                }
            }
        }
    }

    // If we didn't fill the entire table, the path lengths are invalid

    if (pos != tree.m_huffmanCodes.size()) {
        return std::nullopt;
    }

    return tree;
}

static Tree readPretree(BitStream& stream) {
    std::vector<uint8_t> lengths(20);
    for (size_t i = 0; i < 20; i++) {
        lengths[i] = stream.readBits<uint8_t>(4);
    }

    return Tree::fromPathLengths(lengths);
}

void CanonicalTree::updateRangeWithPretree(BitStream& stream, size_t start, size_t end) {
    Tree pretree = readPretree(stream);

    for (size_t i = start; i < end;) {
        auto code = pretree.decodeElement(stream);

        if (code <= 16) {
            this->m_lengths[i] = (static_cast<uint8_t>(17 + this->m_lengths[i] - code) % 17);
            i++;
        } else if (code == 17) {
            auto zeros = stream.readBits<uint8_t>(4);

            for (size_t j = 0; j < zeros + 4; j++) {
                this->m_lengths[i + j] = 0;
            }

            i += zeros + 4;
        } else if (code == 18) {
            auto zeros = stream.readBits<uint8_t>(5);

            for (size_t j = 0; j < zeros + 20; j++) {
                this->m_lengths[i + j] = 0;
            }

            i += zeros + 20;
        } else if (code == 19) {
            auto same = stream.readBits<uint8_t>(1);

            // "Decode new code" is used to parse the next code from the bitstream, which
            // has a value range of [0, 16].
            auto newCode = pretree.decodeElement(stream);
            if (newCode > 16) {
                throw LzxdError("updateRangeWithPretree: invalid newCode");
            }

            auto value = static_cast<uint8_t>(17 + this->m_lengths[i] - newCode) % 17;
            for (size_t j = 0; j < same + 4; j++) {
                this->m_lengths[i + j] = value;
            }

            i += same + 4;
        } else {
            throw LzxdError("updateRangeWithPretree: invalid code");
        }
    }
}

} // namespace lzxd