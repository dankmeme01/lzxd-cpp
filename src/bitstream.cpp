#include <lzxd/bitstream.hpp>
#include <stdexcept>

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

namespace lzxd {

namespace detail {
    template<> uint8_t byteswap<uint8_t>(uint8_t input) { return input; }
    template<> int8_t byteswap<int8_t>(int8_t input) { return input; }

    template<> uint16_t byteswap<uint16_t>(uint16_t input) { return BSWAP16(input); }
    template<> uint32_t byteswap<uint32_t>(uint32_t input) { return BSWAP32(input); }
    template<> uint64_t byteswap<uint64_t>(uint64_t input) { return BSWAP64(input); }
} // namespace detail

BitStream::BitStream(const uint8_t* data, size_t size) : BitStream(std::vector<uint8_t>(data, data + size)) {}

BitStream::BitStream(std::vector<uint8_t> data) : m_data(std::move(data)), m_position(0), m_bitOffset(0) {}

std::vector<uint8_t>& BitStream::data() {
    return m_data;
}

const std::vector<uint8_t>& BitStream::data() const {
    return m_data;
}

size_t BitStream::size() const {
    return m_data.size();
}

size_t BitStream::position() const {
    return m_position;
}

size_t BitStream::bitOffset() const {
    return m_bitOffset;
}

size_t BitStream::remainingBytes() const {
    return m_data.size() - m_position;
}

bool BitStream::eof() const {
    return m_position >= m_data.size();
}

void BitStream::align(size_t alignment) {
    this->_advanceUntilAligned(alignment);
}

void BitStream::skipBits(size_t count) {
    this->_advanceBits(count);
}

std::vector<uint8_t> BitStream::intoVector() {
    return std::move(m_data);
}

std::vector<uint8_t> BitStream::readBytes(size_t count) {
    std::vector<uint8_t> ret(count);
    this->readBytesInto(ret.data(), count);
    return ret;
}

std::vector<uint8_t> BitStream::peekBytes(size_t count) const {
    std::vector<uint8_t> ret(count);
    this->peekBytesInto(ret.data(), count);
    return ret;
}

void BitStream::readBytesInto(uint8_t* output, size_t count) {
    this->peekBytesInto(output, count);
    this->align(1);
    this->_advanceBytes(count);
}

void BitStream::peekBytesInto(uint8_t* output, size_t count) const {
    auto startPos = m_position;
    if (m_bitOffset != 0) {
        startPos++;
    }

    if (startPos + count > m_data.size()) {
        this->_oob();
    }

    std::copy(m_data.begin() + startPos, m_data.begin() + startPos + count, output);
}

bool BitStream::_readBit() {
    auto ret = this->_peekBit();
    this->_advanceBits(1);
    return ret;
}

bool BitStream::_peekBit() const {
    this->_failIfEof();
    return (m_data[m_position] >> (8 - m_bitOffset)) & 1;
}

uint64_t BitStream::_readBits(size_t size) {
    this->_failIfEof();

    // check if there is enough space
    if (m_position + (m_bitOffset + size) / 8 > m_data.size()) {
        this->_oob();
    }

    uint64_t result = 0;
    size_t remainingBits = size;

    while (remainingBits) {
        if (m_position >= m_data.size()) {
            this->_oob();
        }

        uint8_t currentByte = m_data[m_position];
        size_t availableBits = 8 - m_bitOffset;
        size_t bitsToRead = std::min(remainingBits, availableBits);

        // calculate shift and mask to extract the desired bits
        size_t shift = availableBits - bitsToRead;
        uint8_t mask = (1 << bitsToRead) - 1;
        uint8_t chunk = (currentByte >> shift) & mask;

        // shift the result and append the new bits
        result = (result << bitsToRead) | chunk;
        remainingBits -= bitsToRead;

        // update position and bit offset
        this->_advanceBits(bitsToRead);
    }

    return result;
}

uint64_t BitStream::_peekBits(size_t size) const {
    this->_failIfEof();

    // check if there is enough space
    if (m_position + (m_bitOffset + size) / 8 > m_data.size()) {
        this->_oob();
    }

    uint64_t result = 0;
    size_t remainingBits = size;

    while (remainingBits) {
        if (m_position >= m_data.size()) {
            this->_oob();
        }

        uint8_t currentByte = m_data[m_position];
        size_t availableBits = 8 - m_bitOffset;
        size_t bitsToRead = std::min(remainingBits, availableBits);

        // calculate shift and mask to extract the desired bits
        size_t shift = availableBits - bitsToRead;
        uint8_t mask = (1 << bitsToRead) - 1;
        uint8_t chunk = (currentByte >> shift) & mask;

        // shift the result and append the new bits
        result = (result << bitsToRead) | chunk;
        remainingBits -= bitsToRead;
    }

    return result;
}

void BitStream::_oob() const {
    throw std::out_of_range("Out of bounds");
}

void BitStream::_failIfEof() const {
    if (this->eof()) {
        this->_oob();
    }
}

void BitStream::_advanceBits(size_t count) {
    this->_advanceBytes((count + m_bitOffset) / 8);
    m_bitOffset = (m_bitOffset + count) % 8;
}

void BitStream::_advanceBytes(size_t count) {
    m_position += count;
    if (m_position > m_data.size()) {
        this->_oob();
    }
}

void BitStream::_advanceUntilAligned(size_t alignment) {
    // advance until a whole byte
    if (m_bitOffset != 0) {
        m_bitOffset = 0;
        m_position++;
    }

    // advance until bytes are aligned
    while (m_position % alignment != 0 && m_position < m_data.size()) {
        m_position++;
    }
}

} // namespace lzxd
