// LZXD compressed data structure:
// * Header
// * Block
// * Block
// * ...

// Each block can be one of those types: uncompressed, verbatim, aligned

// LZXD bitstream is encoded as a sequence of aligned 2-byte integers stored as little endian

// Window size must be a power of 2 from 2^17 to 2^25 bytes. It is not stored in the stream and must be specified to the decoder.
// The window size SHOULD be the smallest power of two between 2^17 and 2^25 that is greater than or equal to the sum of the size of the reference data rounded up to a multiple of 32,768 and the size of the subject data.