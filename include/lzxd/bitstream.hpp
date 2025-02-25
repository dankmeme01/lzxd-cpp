#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace lzxd {

namespace detail {
    template <std::integral T>
    T byteswap(T input);
} // namespace detail

class BitStream {
public:
    BitStream(const uint8_t* data, size_t size);
    BitStream(std::vector<uint8_t> data);

    std::vector<uint8_t>& data();
    const std::vector<uint8_t>& data() const;

    size_t size() const;
    size_t position() const;
    size_t bitOffset() const;
    size_t remainingBytes() const;

    bool eof() const;

    // Reads an integer from the bitstream, advances the buffer position
    template <typename T = uint8_t>
    T read() {
        return this->readBits<sizeof(T), T>();
    }

    // Reads a little-endian integer from the bitstream, advances the buffer position
    template <typename T = uint8_t>
    T readLittleEndian() {
        return detail::byteswap<T>(this->read<T>());
    }

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

    // Peeks an integer from the bitstream, does not advance the buffer position
    template <typename T = uint8_t>
    T peek() const {
        return this->peekBits<sizeof(T), T>();
    }

    // Peeks a little-endian integer from the bitstream, does not advance the buffer position
    template <typename T = uint8_t>
    T peekLittleEndian() const {
        return detail::byteswap<T>(this->peek<T>());
    }

    // Peeks a single bit from the bitstream, does not advance the buffer position
    template <typename T = uint8_t>
    T peekBit() const {
        return static_cast<T>(this->_peekBit());
    }

    // Peeks a number of bits from the bitstream, does not advance the buffer position
    template <size_t Count, typename T = uint8_t>
    T peekBits() const {
        static_assert(Count <= sizeof(T) * 8, "Cannot fit the requested amount of bits into the requested type");
        return this->peekBits<T>(Count);
    }

    // Peeks a number of bits from the bitstream, does not advance the buffer position
    template <typename T = uint8_t>
    T peekBits(size_t count) const {
        auto bits = this->_peekBits(count);
        return static_cast<T>(bits);
    }

    // Skips a certain amount of bits until the bit buffer is aligned to the given boundary, or EOF is reached. The argument is the wanted alignment in bytes.
    void align(size_t alignment);

    // Skips a given amount of bits
    void skipBits(size_t count);

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
    size_t m_bitOffset;

    bool _readBit();
    bool _peekBit() const;

    uint64_t _readBits(size_t count);
    uint64_t _peekBits(size_t count) const;

    void _oob() const;
    void _failIfEof() const;

    void _advanceBits(size_t count);
    void _advanceBytes(size_t count);
    void _advanceUntilAligned(size_t alignment);

};

} // namespace lzxd