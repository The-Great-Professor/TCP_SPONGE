#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    static size_t abs_seqno = 0;
    size_t length;
    bool rtn = false;

    if (seg.header().syn) {
        if (m_syn)
            return false;
        m_syn = true;
        m_isn = seg.header().seqno.raw_value();

        rtn = true;
        abs_seqno = 1;
        m_base = 1;
        length = seg.length_in_sequence_space() - 1;
        if (length == 0)
            return true;
    } else if (!m_syn)
        return false;
    else {
        abs_seqno = unwrap(seg.header().seqno, WrappingInt32(m_isn), abs_seqno);
        length = seg.length_in_sequence_space();
    }

    if (seg.header().fin) {
        if (m_fin)
            return false;
        m_fin = true;
        rtn = true;
    } else if (seg.length_in_sequence_space() == 0 && abs_seqno == m_base) {
        // 没有syn和fin,没有数据
        return true;
    } else if (abs_seqno >= m_base + window_size() || abs_seqno + length <= m_base) {
        // 超出窗口大小或不在窗口内
        if (rtn == false)
            return false;
    }

    _reassembler.push_substring(seg.payload().copy(), abs_seqno - 1, seg.header().fin);
    m_base = _reassembler.head_index() + 1;
    if (_reassembler.input_ended())  // fin+1
        m_base++;

    return true;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (m_base > 0)
        return WrappingInt32(wrap(m_base, WrappingInt32(m_isn)));
    else
        return nullopt;
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
