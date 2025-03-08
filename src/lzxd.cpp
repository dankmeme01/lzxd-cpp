#include <lzxd/lzxd.hpp>
#include <lzxd/error.hpp>
#include <bit>
#include <cstring>

namespace lzxd {

namespace detail {
    size_t positionSlotsFor(size_t windowSize) {
        switch (windowSize) {
            case 0x8000: // 32 KiB
                return 30;
            case 0x10000: // 64 KiB
                return 32;
            case 0x20000: // 128 KiB
                return 34;
            case 0x40000: // 256 KiB
                return 36;
            case 0x80000: // 512 KiB
                return 38;
            case 0x100000: // 1 MiB
                return 42;
            case 0x200000: // 2 MiB
                return 50;
            case 0x400000: // 4 MiB
                return 66;
            case 0x800000: // 8 MiB
                return 98;
            case 0x1000000: // 16 MiB
                return 162;
            case 0x2000000: // 32 MiB
                return 290;
            default:
                throw LzxdError("positionSlotsFor: invalid window size");
        }
    }
} // namespace detail

Decoder::Decoder(size_t windowSize)
    : windowSize(windowSize),
      window(windowSize),
      mainTree(std::vector<uint8_t>(256 + 8 * detail::positionSlotsFor(windowSize))),
      lengthTree(std::vector<uint8_t>(249)),
      currentBlock(UncompressedBlock {
        BaseBlock {0, 0},
        1, 1, 1
      }) {}

Decoder::Decoder() : Decoder(0x80000) {}

std::vector<uint8_t> Decoder::decompressChunk(const std::vector<uint8_t>& data, size_t outputSize) {
    return this->decompressChunk(data.data(), data.size(), outputSize);
}

std::vector<uint8_t> Decoder::decompressChunk(const uint8_t* data, size_t size, size_t outputSize) {
    std::vector<uint8_t> output(outputSize);
    size_t s = this->decompressChunkInto(data, size, output.data(), outputSize);
    output.resize(s);
    return output;
}

size_t Decoder::decompressChunkInto(const std::vector<uint8_t>& data, uint8_t* output, size_t outputSize) {
    return this->decompressChunkInto(data.data(), data.size(), output, outputSize);
}

size_t Decoder::decompressChunkInto(const uint8_t* data, size_t size, uint8_t* output, size_t outputSize) {
    BitStream stream(data, size);

    if (decodedChunks == 0) {
        this->firstChunk(stream);
    }

    size_t decodedLen = 0;
    while (decodedLen != outputSize) {
        if (this->currentBlock.remaining() == 0) {
            // Re-align the bitstream to 16 bits, by reading 1 byte
            if (this->currentBlock.type() == BlockType::Uncompressed && this->currentBlock.size() % 2 != 0) {
                stream.readByte();
            }

            this->currentBlock = this->readBlock(stream, lzxd::readBlockHeader(stream));
            LZXD_ASSERT(currentBlock.remaining() != 0);
        }

        auto decoded = this->currentBlock.decodeElement(stream, this->r0, this->r1, this->r2);
        size_t advance = 0;

        if (std::holds_alternative<detail::DecodedSingle>(decoded)) {
            auto value = std::get<detail::DecodedSingle>(decoded).value;
            this->window.push(value);
            advance = 1;
        } else if (std::holds_alternative<detail::DecodedMatch>(decoded)) {
            auto match = std::get<detail::DecodedMatch>(decoded);
            this->window.copyFromSelf(match.offset, match.length);
            advance = match.length;
        } else {
            auto read = std::get<detail::DecodedRead>(decoded).value;
            // read up to the end of chunk, to allow for larger blocks
            auto length = std::min(read, stream.remainingBytes());
            this->window.copyFromBitstream(stream, read);
            advance = read;
        }

        LZXD_ASSERT(advance != 0);
        decodedLen += advance;

        if (advance > this->currentBlock.remaining()) {
            throw LzxdError("decompressChunkInto: advance is larger than remaining");
        }

        this->currentBlock.remaining() -= (uint32_t) advance;
    }

    auto chunkOffset = this->chunkOffset;
    this->chunkOffset += decodedLen;

    auto viewStart = this->window.pastView(decodedLen);

    // E8 fixups are disabled after 1GB of input data, or if the chunk size is too small.
    if (e8Translator && chunkOffset < 0x40000000 && decodedLen > 10) {
        // TODO
        throw LzxdError("E8 translation not implemented!");
    }

    decodedChunks++;

    std::memcpy(output, viewStart, decodedLen);

    return decodedLen;
}

Block Decoder::readBlock(BitStream& stream, const BlockHeader& header) {
    if (header.type == BlockType::Invalid || header.size == 0) {
        throw lzxd::LzxdError("decompressChunkInto: invalid block header");
    }

    switch (header.type) {
        case BlockType::Uncompressed: {
            stream.align(); // Align to 16-bit boundary

            uint32_t r0 = stream.readU32le();
            uint32_t r1 = stream.readU32le();
            uint32_t r2 = stream.readU32le();

            return UncompressedBlock {
                BaseBlock {header.size, header.size},
                r0, r1, r2
            };
        } break;

        case BlockType::Verbatim: {
            this->readMainAndLengthTrees(stream);

            return VerbatimBlock {
                BaseBlock {header.size, header.size},
                this->mainTree.createInstance().value(),
                this->lengthTree.createInstance()
            };
        } break;

        case BlockType::Aligned: {
            // create the aligned offset tree
            std::vector<uint8_t> lengths(8);
            for (size_t i = 0; i < 8; i++) {
                lengths[i] = stream.readBits<uint8_t>(3);
            }

            Tree alignedOffsetTree = Tree::fromPathLengths(std::move(lengths));

            this->readMainAndLengthTrees(stream);

            return AlignedOffsetBlock {
                BaseBlock {header.size, header.size},
                this->mainTree.createInstance().value(),
                this->lengthTree.createInstance(),
                alignedOffsetTree
            };
        } break;

        case BlockType::Invalid:
        default: {
            // unreachable
            LZXD_ASSERT(false);
        } break;
    }
}

void Decoder::readMainAndLengthTrees(BitStream& stream) {
    this->mainTree.updateRangeWithPretree(stream, 0, 256);
    this->mainTree.updateRangeWithPretree(stream, 256, 256 + detail::positionSlotsFor(this->windowSize) * 8);
    this->lengthTree.updateRangeWithPretree(stream, 0, 249);
}

void Decoder::firstChunk(BitStream& stream) {
    // First bit of the first chunk controls whether E8 translation is enabled
    bool e8Translation = stream.readBit();

    if (e8Translation) {
        throw lzxd::LzxdError("E8 translation not implemented!");
        // this->e8Translator = detail::E8Translator{std::bit_cast<int32_t>(stream.readBits(32))};
    }
}

void Decoder::reset() {
    auto wsize = this->windowSize;

    this->~Decoder();

    new (this) Decoder(wsize);
}

} // namespace lzxd