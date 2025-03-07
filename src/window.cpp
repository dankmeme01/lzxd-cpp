#include <lzxd/window.hpp>
#include <lzxd/error.hpp>
#include <cstring>

namespace lzxd::detail {

void Window::push(uint8_t byte) {
    this->data[this->position] = byte;
    this->advance(1);
}

void Window::advance(size_t by) {
    this->position += by;
    if (this->position >= this->data.size()) {
        this->position -= this->data.size();
    }
}

void Window::copyFromSelf(size_t offset, size_t length) {
    // For the fast path:
    // * Source cannot wrap around
    // * `memmove` won't overwrite as we go but we need that
    // * Destination cannot wrap around

    if (offset <= this->position && length <= offset && this->position + length < this->data.size()) {
        auto start = this->position - offset;
        std::memmove(
            this->data.data() + this->position,
            this->data.data() + start,
            length
        );
    } else {
        auto mask = this->data.size() - 1;

        for (size_t i = 0; i < length; i++) {
            auto dst = (this->position + i) & mask;
            auto src = (this->data.size() + this->position + i - offset) & mask;
            this->data[dst] = this->data[src];
        }
    }

    this->advance(length);
}

void Window::copyFromBitstream(BitStream& stream, size_t length) {
    if (length > this->data.size()) {
        throw LzxdError("Window::copyFromBitstream: length is too large");
    }

    if (this->position + length > this->data.size()) {
        auto shift = this->position + length - this->data.size();
        this->position -= shift;

        // No need to actually save the part we're about to overwrite because when reading
        // with the bitstream we would also overwrite it anyway.
        std::memmove(
            this->data.data(),
            this->data.data() + shift,
            this->data.size() - shift
        );
    }

    stream.readBytesInto(this->data.data() + this->position, length);
    this->advance(length);
}

std::vector<uint8_t> Window::pastView(size_t len) {
    if (len > 32 * 1024) {
        throw LzxdError("Window::pastView: chunk is too long");
    }

    // Being at zero means we're actually at max length where is impossible for `len` to be
    // bigger and we would not want to bother shifting the entire array to end where it was.
    if (this->position != 0 && len > this->position) {
        auto shift = len - this->position;
        this->advance(shift);

        auto tmp = std::vector<uint8_t>(
            this->data.begin() + this->data.size() - shift,
            this->data.end()
        );

        std::memmove(
            this->data.data() + shift,
            this->data.data(),
            this->data.size() - shift
        );

        std::memcpy(this->data.data(), tmp.data(), shift);
    }

    // Because we want to read behind us, being at zero means we are at the end
    size_t pos;
    if (this->position == 0) {
        pos = this->data.size();
    } else {
        pos = this->position;
    }

    return std::vector<uint8_t>(
        this->data.begin() + pos - len,
        this->data.begin() + pos
    );
}

} // namespace lzxd::detail