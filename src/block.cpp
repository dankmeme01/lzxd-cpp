#include <lzxd/block.hpp>
#include <lzxd/error.hpp>
#include <array>

namespace lzxd {

static auto FOOTER_BITS = std::array<uint8_t, 289>{
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13,
    13, 14, 14, 15, 15, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
};

static auto BASE_POSITION = std::array<uint32_t, 290>{
    0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536,
    2048, 3072, 4096, 6144, 8192, 12288, 16384, 24576, 32768, 49152, 65536, 98304, 131072, 196608,
    262144, 393216, 524288, 655360, 786432, 917504, 1048576, 1179648, 1310720, 1441792, 1572864,
    1703936, 1835008, 1966080, 2097152, 2228224, 2359296, 2490368, 2621440, 2752512, 2883584,
    3014656, 3145728, 3276800, 3407872, 3538944, 3670016, 3801088, 3932160, 4063232, 4194304,
    4325376, 4456448, 4587520, 4718592, 4849664, 4980736, 5111808, 5242880, 5373952, 5505024,
    5636096, 5767168, 5898240, 6029312, 6160384, 6291456, 6422528, 6553600, 6684672, 6815744,
    6946816, 7077888, 7208960, 7340032, 7471104, 7602176, 7733248, 7864320, 7995392, 8126464,
    8257536, 8388608, 8519680, 8650752, 8781824, 8912896, 9043968, 9175040, 9306112, 9437184,
    9568256, 9699328, 9830400, 9961472, 10092544, 10223616, 10354688, 10485760, 10616832, 10747904,
    10878976, 11010048, 11141120, 11272192, 11403264, 11534336, 11665408, 11796480, 11927552,
    12058624, 12189696, 12320768, 12451840, 12582912, 12713984, 12845056, 12976128, 13107200,
    13238272, 13369344, 13500416, 13631488, 13762560, 13893632, 14024704, 14155776, 14286848,
    14417920, 14548992, 14680064, 14811136, 14942208, 15073280, 15204352, 15335424, 15466496,
    15597568, 15728640, 15859712, 15990784, 16121856, 16252928, 16384000, 16515072, 16646144,
    16777216, 16908288, 17039360, 17170432, 17301504, 17432576, 17563648, 17694720, 17825792,
    17956864, 18087936, 18219008, 18350080, 18481152, 18612224, 18743296, 18874368, 19005440,
    19136512, 19267584, 19398656, 19529728, 19660800, 19791872, 19922944, 20054016, 20185088,
    20316160, 20447232, 20578304, 20709376, 20840448, 20971520, 21102592, 21233664, 21364736,
    21495808, 21626880, 21757952, 21889024, 22020096, 22151168, 22282240, 22413312, 22544384,
    22675456, 22806528, 22937600, 23068672, 23199744, 23330816, 23461888, 23592960, 23724032,
    23855104, 23986176, 24117248, 24248320, 24379392, 24510464, 24641536, 24772608, 24903680,
    25034752, 25165824, 25296896, 25427968, 25559040, 25690112, 25821184, 25952256, 26083328,
    26214400, 26345472, 26476544, 26607616, 26738688, 26869760, 27000832, 27131904, 27262976,
    27394048, 27525120, 27656192, 27787264, 27918336, 28049408, 28180480, 28311552, 28442624,
    28573696, 28704768, 28835840, 28966912, 29097984, 29229056, 29360128, 29491200, 29622272,
    29753344, 29884416, 30015488, 30146560, 30277632, 30408704, 30539776, 30670848, 30801920,
    30932992, 31064064, 31195136, 31326208, 31457280, 31588352, 31719424, 31850496, 31981568,
    32112640, 32243712, 32374784, 32505856, 32636928, 32768000, 32899072, 33030144, 33161216,
    33292288, 33423360,
};


BlockHeader readBlockHeader(BitStream& stream) {
    BlockHeader block;
    uint8_t val = stream.readBits<uint8_t>(3);
    switch (val) {
        case 0b001:
            block.type = BlockType::Verbatim;
            break;
        case 0b010:
            block.type = BlockType::Aligned;
            break;
        case 0b011:
            block.type = BlockType::Uncompressed;
            break;
        default:
            block.type = BlockType::Invalid;
            break;
    }

    if (block.type == BlockType::Invalid) {
        throw LzxdError("readBlockFromStream: invalid block type");
    }

    block.size = stream.readU24be();
    return block;
}

Block readBlock(BitStream &stream, const BlockHeader& header) {
    switch (header.type) {
        case BlockType::Uncompressed:
            return UncompressedBlock{};
        case BlockType::Verbatim:
            return VerbatimBlock{};
        case BlockType::Aligned:
            return AlignedOffsetBlock{};
        default:
            throw LzxdError("readBlock: invalid block type");
    }
}

struct DecodeInfo {
    const Tree* alignedOffsetTree = nullptr;
    const Tree* mainTree = nullptr;
    const Tree* lengthTree = nullptr;
};

static detail::DecodedPart decodeCompressedElement(BitStream& stream, uint32_t& outr0, uint32_t& outr1, uint32_t& outr2, DecodeInfo dinfo) {
    // decoding matches and literals (aligned and verbatim blocks)
    auto mainElement = dinfo.mainTree->decodeElement(stream);

    // check if it is a literal
    if (mainElement <= 255) {
        return detail::DecodedSingle{static_cast<uint8_t>(mainElement)};
    }

    // otherwise it is a match. a match has two components, offset and length
    uint16_t lengthHeader = (mainElement - 256) & 7;
    size_t matchLength;
    if (lengthHeader == 7) {
        // length of the footer
        matchLength = dinfo.lengthTree->decodeElement(stream) + 7 + 2;
    } else {
        matchLength = lengthHeader + 2; // no length footer
    }

    LZXD_ASSERT(matchLength != 0);

    uint16_t positionSlot = (mainElement - 256) >> 3;

    // Check for repeated offsets (positions 0, 1, 2).
    uint32_t matchOffset;
    if (positionSlot == 0) {
        matchOffset = outr0;
    } else if (positionSlot == 1) {
        matchOffset = outr1;
        std::swap(outr1, outr0);
    } else if (positionSlot == 2) {
        matchOffset = outr2;
        std::swap(outr2, outr0);
    } else {
        // Decode the offset
        auto offsetBits = FOOTER_BITS[positionSlot];
        uint32_t formattedOffset;

        if (dinfo.alignedOffsetTree) {
            uint32_t verbatimBits;
            uint16_t alignedBits;

            if (offsetBits >= 3) {
                verbatimBits = stream.readBits(offsetBits - 3) << 3;
                alignedBits = dinfo.alignedOffsetTree->decodeElement(stream);
            } else {
                verbatimBits = stream.readBits(offsetBits);
                alignedBits = 0;
            }

            formattedOffset = BASE_POSITION[positionSlot] + verbatimBits + alignedBits;
        } else {
            // block is verbatim
            auto verbatimBits = stream.readBits(offsetBits);
            formattedOffset = BASE_POSITION[positionSlot] + verbatimBits;
        }

        // decoding a match offset
        matchOffset = formattedOffset - 2;

        // update repeated offset least recently used queue
        outr2 = outr1;
        outr1 = outr0;
        outr0 = matchOffset;
    }

    return detail::DecodedMatch{matchOffset, matchLength};
}

detail::DecodedPart Block::decodeElement(BitStream& stream, uint32_t& outr0, uint32_t& outr1, uint32_t& outr2) const {
    DecodeInfo dinfo;

    switch (this->type()) {
        case BlockType::Aligned: {
            const auto& block = std::get<AlignedOffsetBlock>(m_data);
            dinfo.alignedOffsetTree = &block.alignedOffsetTree;
        } [[fallthrough]]; // fallthrough to verbatim

        case BlockType::Verbatim: {
            // AlignedOffsetBlock inherits VerbatimBlock, set their shared fields here
            const auto& block = [&]() -> const VerbatimBlock& {
                if (this->type() == BlockType::Aligned) {
                    return std::get<AlignedOffsetBlock>(m_data);
                } else {
                    return std::get<VerbatimBlock>(m_data);
                }
            }();

            dinfo.mainTree = &block.mainTree;

            if (block.lengthTree) {
                dinfo.lengthTree = &block.lengthTree.value();
            } else {
                dinfo.lengthTree = nullptr;
            }

            return decodeCompressedElement(stream, outr0, outr1, outr2, dinfo);
        } break;

        case BlockType::Uncompressed: {
            const auto& block = std::get<UncompressedBlock>(m_data);
            outr0 = block.r0;
            outr1 = block.r1;
            outr2 = block.r2;
            return detail::DecodedRead{block.remaining};
        } break;

        default:
            throw LzxdError("Block::decodeElement: invalid block type");
    }
}

BlockType Block::type() const {
    // hacky lol, dont change order of the variant and the enum
    return static_cast<BlockType>(1 + m_data.index());
}

uint32_t& Block::remaining() {
    return std::visit([](auto& block) -> uint32_t& { return block.remaining; }, m_data);
}

size_t Block::size() const {
    return std::visit([](const auto& block) { return block.size; }, m_data);
}

} // namespace lzxd
