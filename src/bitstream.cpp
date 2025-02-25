#include <cstdint>
#include <lzxd/bitstream.hpp>
#include <lzxd/error.hpp>
#include <stdexcept>
#include <cstring>
#include <bit>

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

    uint16_t rotleftu16(uint16_t val, uint8_t bits) {
        // This is identical to Rust's u16::rotate_left
        return (val << bits) | (val >> (16 - bits));
    }
} // namespace detail

BitStream::BitStream(const uint8_t* data, size_t size) : BitStream(std::vector<uint8_t>(data, data + size)) {}

BitStream::BitStream(std::vector<uint8_t> data) : m_data(std::move(data)), m_position(0), m_nextNumber(0), m_remainingBits(0) {}

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

size_t BitStream::remainingBytes() const {
    return m_data.size() - m_position;
}

bool BitStream::eof() const {
    return m_position >= m_data.size();
}

uint8_t BitStream::readByte() {
    this->_failIfEof();
    uint8_t byte = m_data[m_position];
    this->_advanceBytes(1);
    return byte;
}

uint32_t BitStream::readU32le() {
    uint16_t lo = this->_readBitsOneWord(16);
    uint16_t hi = this->_readBitsOneWord(16);

    if constexpr (std::endian::native == std::endian::big) {
        lo = detail::byteswap(lo);
        hi = detail::byteswap(hi);
    }

    uint32_t ret;
    std::memcpy(&ret, &lo, sizeof(uint16_t));
    std::memcpy(reinterpret_cast<uint8_t*>(&ret) + sizeof(uint16_t), &hi, sizeof(uint16_t));

    // IDK how correct this is lol
    // Hopefully no one is running this on a big endian system anyway
    if constexpr (std::endian::native == std::endian::big) {
        ret = detail::byteswap(ret);
    }

    return ret;
}

uint32_t BitStream::readU24be() {
    uint16_t hi = this->_readBits(16);
    uint16_t lo = this->_readBits(8);

    return (hi << 8) | lo;
}

void BitStream::align() {
    if (m_remainingBits == 0) {
        this->readBits(16);
    } else {
        m_remainingBits = 0;
    }
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
    this->_advanceBytes(count);
}

void BitStream::peekBytesInto(uint8_t* output, size_t count) const {
    auto startPos = m_position;

    if (startPos + count > m_data.size()) {
        this->_oob();
    }

    std::copy(m_data.begin() + startPos, m_data.begin() + startPos + count, output);
}

bool BitStream::_readBit() {
    if (m_remainingBits == 0) {
        this->_advanceBuffer();
    }

    m_remainingBits--;
    m_nextNumber = detail::rotleftu16(m_nextNumber, 1);
    return m_nextNumber & 1;
}

uint16_t BitStream::_readBitsOneWord(size_t size) {
    LZXD_ASSERT(size <= 16);

    if (size <= m_remainingBits) {
        this->m_remainingBits -= size;

        // rotate `m_nextNumber` left by `size` bits
        m_nextNumber = detail::rotleftu16(m_nextNumber, size);

        return m_nextNumber & ((1 << size) - 1);
    }

    uint16_t hi = detail::rotleftu16(m_nextNumber, m_remainingBits) & ((1 << m_remainingBits) - 1);
    size -= m_remainingBits;
    this->_advanceBuffer();

    m_remainingBits -= size;
    this->m_nextNumber = detail::rotleftu16(m_nextNumber, size);

    // `bits` may be 16 which would overflow the left shift, operate on `u32` and trunc.
    uint16_t lo = m_nextNumber & (((uint16_t)(1ul << size)) - 1);

    return (uint16_t)((uint32_t)hi << size) | lo;
}

uint32_t BitStream::_readBits(size_t count) {
    if (count <= 16) {
        return this->_readBitsOneWord(count);
    }

    LZXD_ASSERT(count <= 32);

    auto hi = this->_readBitsOneWord(16);
    auto lo = this->_readBitsOneWord(count - 16);

    return (hi << (count - 16)) | lo;
}

uint16_t BitStream::_peekBitsOneWord(size_t count) {
    LZXD_ASSERT(count <= 16);

    if (count <= m_remainingBits) {
        return m_nextNumber >> (m_remainingBits - count);
    }

    uint16_t hi = detail::rotleftu16(m_nextNumber, m_remainingBits) & ((1 << m_remainingBits) - 1);
    count -= m_remainingBits;

    // We may peek more than we need (i.e. at the end of a chunk), due to the way
    // our decoder is implemented. This is a bit ugly but luckily we can pretend
    // there are just zeros after.
    uint16_t n;
    if (this->eof()) {
        n = 0;
    } else {
        std::memcpy(&n, m_data.data() + m_position, sizeof(uint16_t));
        if constexpr (std::endian::native == std::endian::big) {
            n = detail::byteswap(n);
        }
    }

    uint16_t lo = detail::rotleftu16(n, count) & (((uint16_t)(1ul << count)) - 1);
    return (uint16_t)((uint32_t)hi << count) | lo;
}

uint32_t BitStream::_peekBits(size_t count) {
    if (count <= 16) {
        return this->_peekBitsOneWord(count);
    }

    LZXD_ASSERT(count <= 32);

    // Store state of the bitstream
    struct State {
        uint16_t nextNumber;
        uint8_t remainingBits;
        size_t position;
    } state = {m_nextNumber, m_remainingBits, m_position};

    uint16_t hi = this->_readBitsOneWord(16);
    uint16_t lo = this->_peekBitsOneWord(count - 16);

    // Restore state of the bitstream
    m_nextNumber = state.nextNumber;
    m_remainingBits = state.remainingBits;
    m_position = state.position;

    return (hi << (count - 16)) | lo;
}

void BitStream::_oob() const {
    throw std::out_of_range("Out of bounds");
}

void BitStream::_failIfEof() const {
    if (this->eof()) {
        this->_oob();
    }
}

void BitStream::_advanceBuffer() {
    this->_failIfEof();
    this->m_remainingBits = 16;
    std::memcpy(&m_nextNumber, m_data.data() + m_position, sizeof(uint16_t));

    // Since we read a little-endian integer, byteswap if we are on a big-endian architecture
    if constexpr (std::endian::native == std::endian::big) {
        m_nextNumber = detail::byteswap(m_nextNumber);
    }

    m_position += sizeof(uint16_t);
}

void BitStream::_advanceBytes(size_t count) {
    m_position += count;
    if (m_position > m_data.size()) {
        this->_oob();
    }
}

} // namespace lzxd
