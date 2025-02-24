#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace lzxd {

class BitStream {
public:
    BitStream(uint8_t* data, size_t size);
    BitStream(std::vector<uint8_t> data);

    std::vector<uint8_t>& data();
    const std::vector<uint8_t>& data() const;

    size_t size() const;
    size_t position() const;
    size_t bitOffset() const;

    bool eof() const;

    // Reads an integer from the bitstream, advances the buffer position
    template <typename T = uint8_t>
    T read() {
        return this->readBits<sizeof(T), T>();
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
    T peek() {
        return this->peekBits<sizeof(T), T>();
    }

    // Peeks a single bit from the bitstream, does not advance the buffer position
    template <typename T = uint8_t>
    T peekBit() {
        return static_cast<T>(this->_peekBit());
    }

    // Peeks a number of bits from the bitstream, does not advance the buffer position
    template <size_t Count, typename T = uint8_t>
    T peekBits() {
        static_assert(Count <= sizeof(T) * 8, "Cannot fit the requested amount of bits into the requested type");
        return this->peekBits<T>(Count);
    }

    // Peeks a number of bits from the bitstream, does not advance the buffer position
    template <typename T = uint8_t>
    T peekBits(size_t count) {
        auto bits = this->_peekBits(count);
        return static_cast<T>(bits);
    }

private:
    std::vector<uint8_t> m_data;
    size_t m_position;
    size_t m_bitOffset;

    bool _readBit();
    bool _peekBit();

    uint64_t _readBits(size_t count);
    uint64_t _peekBits(size_t count);

    void _oob();
    void _failIfEof();

    void _advanceBits(size_t count);
    void _advanceBytes(size_t count);
    void _advanceUntilAligned(size_t alignment);

};

} // namespace lzxd