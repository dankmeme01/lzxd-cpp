#include <lzxd/bitstream.hpp>
#include <stdexcept>

namespace lzxd {

BitStream::BitStream(uint8_t* data, size_t size) : BitStream(std::vector<uint8_t>(data, data + size)) {}

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

bool BitStream::eof() const {
    return m_position >= m_data.size();
}

bool BitStream::_readBit() {
    auto ret = this->_peekBit();
    this->_advanceBits(1);
    return ret;
}

bool BitStream::_peekBit() {
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
    }
}

void BitStream::_oob() {
    throw std::out_of_range("Out of bounds");
}

void BitStream::_failIfEof() {
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
    while (m_position % alignment != 0) {
        m_position++;
    }
}

} // namespace lzxd
