#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace lzxd {

namespace detail {
    template <std::integral T>
    T byteswap(T input);
} // namespace detail

// A data stream where data is interpreted as 16-bit little-endian integers.
// See (https://github.com/Lonami/lzxd/blob/master/src/bitstream.rs)
// and (https://msopenspecs.azureedge.net/files/MS-PATCH/%5bMS-PATCH%5d.pdf) section 3
class BitStream {
public:
    BitStream(const uint8_t* data, size_t size);
    BitStream(std::vector<uint8_t> data);

    std::vector<uint8_t>& data();
    const std::vector<uint8_t>& data() const;

    size_t size() const;
    size_t position() const;
    size_t remainingBytes() const;

    bool eof() const;

    // Reads an integer from the bitstream, advances the buffer position
    template <typename T = uint8_t>
    T readBits() {
        return this->readBits<sizeof(T), T>();
    }

    // Reads a little-endian 32-bit integer from the bitstream, advances the buffer position
    uint32_t readU32le();

    // Reads a big-endian 24-bit integer from the bitstream, advances the buffer position
    uint32_t readU24be();

    // Reads a single bit from the bitstream, advances the buffer position
    template <typename T = bool>
    T readBit() {
        return static_cast<T>(this->_readBit());
    }

    // Reads a number of bits from the bitstream, advances the buffer position
    template <size_t Count, typename T = uint32_t>
    T readBits() {
        static_assert(Count <= sizeof(T) * 8, "Cannot fit the requested amount of bits into the requested type");
        return this->readBits<T>(Count);
    }

    // Reads a number of bits from the bitstream, advances the buffer position
    template <typename T = uint32_t>
    T readBits(size_t count) {
        auto bits = this->_readBits(count);
        return static_cast<T>(bits);
    }

    // Reads a byte from the buffer, ignores the bit offset
    uint8_t readByte();

    // Peeks an integer from the bitstream, does not advance the buffer position
    template <typename T = uint8_t>
    T peekBits() const {
        return this->peekBits<sizeof(T), T>();
    }

    // Peeks a little-endian integer from the bitstream, does not advance the buffer position
    template <typename T = uint8_t>
    T peekLittleEndian() const {
        return detail::byteswap<T>(this->peekBits<T>());
    }

    // Peeks a number of bits from the bitstream, does not advance the buffer position
    template <size_t Count, typename T = uint8_t>
    T peekBits() const {
        static_assert(Count <= sizeof(T) * 8, "Cannot fit the requested amount of bits into the requested type");
        return this->peekBits<T>(Count);
    }

    // Peeks a number of bits from the bitstream, does not advance the buffer position
    template <typename T = uint8_t>
    T peekBits(size_t count) {
        auto bits = this->_peekBits(count);
        return static_cast<T>(bits);
    }

    // Skips a certain amount of bits until the bit buffer is aligned to the given boundary, or EOF is reached. The argument is the wanted alignment in bytes.
    void align();

    // Resets the bitstream, returns the entire raw data.
    std::vector<uint8_t> intoVector();

    // Reads a certain amount of bytes from the bitstream, advances the buffer position.
    // This operation resets the bit offset, and the output vector starts from the next byte if it is non-zero.
    std::vector<uint8_t> readBytes(size_t count);
    // Peeks a certain amount of bytes from the bitstream, does not advance the buffer position.
    // Does not reset the bit offset, but the output vector starts from the next byte if it is non-zero.
    std::vector<uint8_t> peekBytes(size_t count) const;

    // Reads a certain amount of bytes from the bitstream into the given buffer, advances the buffer position.
    // This operation resets the bit offset, and the output vector starts from the next byte if it is non-zero.
    void readBytesInto(uint8_t* output, size_t count);
    // Peeks a certain amount of bytes from the bitstream into the given buffer, does not advance the buffer position.
    // Does not reset the bit offset, but the output vector starts from the next byte if it is non-zero.
    void peekBytesInto(uint8_t* output, size_t count) const;

private:
    std::vector<uint8_t> m_data;
    size_t m_position;
    uint16_t m_nextNumber;
    uint8_t m_remainingBits;  // remaining bits in m_nextNumber

    bool _readBit();

    uint16_t _readBitsOneWord(size_t count);
    uint32_t _readBits(size_t count);
    uint16_t _peekBitsOneWord(size_t count);
    uint32_t _peekBits(size_t count);

    void _oob() const;
    void _failIfEof() const;

    void _advanceBuffer();
    void _advanceBytes(size_t count);
};

} // namespace lzxd