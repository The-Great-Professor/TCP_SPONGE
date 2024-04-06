#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

bool operator<(const block b1, const block b2) { return b1.begin_index < b2.begin_index; }

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (index >= _capacity + m_send_index)
        return;

    m_eof |= eof;
    if (index + data.size() > m_send_index) {
        block blk;
        if (index < m_send_index) {
            size_t offset = m_send_index - index;
            blk.begin_index = m_send_index;
            blk.length = data.size() - offset;
            blk.bytes.assign(data.begin() + offset, data.end());
        } else {
            blk.begin_index = index;
            blk.length = data.size();
            blk.bytes = data;
        }
        m_unassembled_byte += blk.length;

        auto it = m_unassembled_set.lower_bound(blk);
        int merge_byte;
        // merge后面
        while (it != m_unassembled_set.end() && (merge_byte = merge_block(blk, *it)) >= 0) {
            m_unassembled_byte -= merge_byte;
            m_unassembled_set.erase(it);
            it = m_unassembled_set.lower_bound(blk);
        }
        // merge前面
        if (it != m_unassembled_set.begin()) {
            it--;
            while ((merge_byte = merge_block(blk, *it)) >= 0) {
                m_unassembled_byte -= merge_byte;
                m_unassembled_set.erase(it);
                it = m_unassembled_set.lower_bound(blk);
                if (it == m_unassembled_set.begin())
                    break;
                it--;
            }
        }
        m_unassembled_set.insert(blk);

        if (!m_unassembled_set.empty()) {
            auto ptr = m_unassembled_set.begin();
            if (ptr->begin_index == m_send_index) {
                size_t write_byte = _output.write(ptr->bytes);
                m_send_index += write_byte;
                m_unassembled_byte -= write_byte;
                m_unassembled_set.erase(ptr);
            }
        }
    }

    if (m_eof && empty())
        _output.end_input();
}

int StreamReassembler::merge_block(block &blk1, const block &blk2)  // 返回被merge的长度
{
    block x, y;
    if (blk1.begin_index <= blk2.begin_index) {
        x = blk1;
        y = blk2;
    } else {
        x = blk2;
        y = blk1;
    }

    if (x.begin_index + x.length < y.begin_index)
        return -1;
    else if (x.begin_index + x.length >= y.begin_index + y.length) {
        blk1 = x;
        return y.length;
    } else {
        blk1.begin_index = x.begin_index;
        blk1.bytes = x.bytes.append(y.bytes.substr(x.begin_index + x.length - y.begin_index));
        blk1.length = blk1.bytes.size();
        return x.begin_index + x.length - y.begin_index;
    }
}

size_t StreamReassembler::unassembled_bytes() const { return m_unassembled_byte; }

bool StreamReassembler::empty() const { return m_unassembled_byte == 0; }
