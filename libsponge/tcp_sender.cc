#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window(bool send_syn) {
    if (_fin)
        return;
    if (!_syn) {
        if (send_syn) {
            _syn = true;
            TCPSegment seg;
            seg.header().syn = true;
            send_tcpsegment(seg);
        }
        return;
    }

    uint16_t window_size = (_rwnd == 0 ? 1 : _rwnd);
    if (_stream.eof() && _recv_ackno + window_size > _next_seqno) {
        TCPSegment seg;
        seg.header().fin = true;
        _fin = true;
        send_tcpsegment(seg);
        return;
    }

    while (!_stream.buffer_empty() && _recv_ackno + window_size > _next_seqno) {
        size_t seg_size =
            min(TCPConfig::MAX_PAYLOAD_SIZE, static_cast<size_t>(window_size - _next_seqno + _recv_ackno));
        string bytes = _stream.read(seg_size);
        TCPSegment seg;
        seg.payload() = Buffer(move(bytes));

        if (_stream.eof() && seg.length_in_sequence_space() < window_size) {
            seg.header().fin = true;
            _fin = true;
        }
        send_tcpsegment(seg);
    }
}

void TCPSender::send_tcpsegment(TCPSegment &seg) {
    seg.header().seqno = wrap(_next_seqno, _isn);
    _next_seqno += seg.length_in_sequence_space();
    _bytes_in_flight += seg.length_in_sequence_space();
    _segments_out.push(seg);
    _segments_wait.push(seg);

    if (!_timer_running) {
        _timer_running = true;
        _timer = 0;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ackno = unwrap(ackno, _isn, _recv_ackno);
    if (abs_ackno > _next_seqno)
        return false;

    _rwnd = window_size;
    if (abs_ackno <= _recv_ackno)
        return true;
    _recv_ackno = abs_ackno;

    while (!_segments_wait.empty()) {
        TCPSegment seg = _segments_wait.front();
        if (abs_ackno < unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space())
            break;
        _segments_wait.pop();
        _bytes_in_flight -= seg.length_in_sequence_space();
    }

    fill_window();
    _retransmission_timeout = _initial_retransmission_timeout;
    _consecutive_retransmissions = 0;

    if (!_segments_wait.empty()) {
        _timer_running = true;
        _timer = 0;
    }
    return true;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (!_timer_running)
        return;

    _timer += ms_since_last_tick;
    if (_timer >= _retransmission_timeout && !_segments_wait.empty()) {
        TCPSegment seg = _segments_wait.front();
        _segments_out.push(seg);

        _timer = 0;
        _consecutive_retransmissions++;
        _retransmission_timeout *= 2;
    }
    if (_segments_wait.empty())
        _timer_running = false;
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}
