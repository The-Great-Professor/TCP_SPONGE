#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint32_t num = static_cast<uint32_t>(n) + isn.raw_value();
    return WrappingInt32(num);
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // uint32_t offset = n.raw_value() - isn.raw_value();
    // uint64_t tmp = (checkpoint & 0xffffffff00000000) + offset;
    // uint64_t rtn = tmp;
    // if (abs(static_cast<int64_t>(tmp + (1ul << 32) - checkpoint)) < abs(static_cast<int64_t>(tmp - checkpoint)))
    //     rtn = tmp + (1ul << 32);
    // else if (tmp >= (1ul << 32) &&
    //          abs(static_cast<int64_t>(tmp - (1ul << 32) - checkpoint)) < abs(static_cast<int64_t>(rtn - checkpoint)))
    //     rtn = tmp - (1ul << 32);
    // return rtn;

    int32_t offset = n - wrap(checkpoint, isn);
    int64_t rtn = checkpoint + offset;
    return rtn >= 0 ? static_cast<uint64_t>(rtn) : rtn + (1ul << 32);
}
