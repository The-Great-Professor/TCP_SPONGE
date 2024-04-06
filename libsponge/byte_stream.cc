#include "byte_stream.hh"

#include <algorithm>
#include <iterator>
#include <stdexcept>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : m_capacity(capacity) {}

size_t ByteStream::write(const string &data) {
    size_t write_len = data.size();
    write_len = std::min(write_len, m_capacity - m_buffer.size());
    m_write_byte += write_len;
    for (size_t i = 0; i < write_len; i++)
        m_buffer.push_back(data[i]);
    return write_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t mutable_len = len;
    mutable_len = min(mutable_len, m_buffer.size());
    string read_str(m_buffer.begin(), m_buffer.begin() + mutable_len);
    return read_str;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t mutable_len = len;
    mutable_len = min(mutable_len, m_buffer.size());
    m_read_byte += mutable_len;
    while (mutable_len--)
        m_buffer.pop_front();
}

void ByteStream::end_input() { m_eof = true; }

bool ByteStream::input_ended() const { return m_eof; }

size_t ByteStream::buffer_size() const { return m_buffer.size(); }

bool ByteStream::buffer_empty() const { return m_buffer.size() == 0; }

bool ByteStream::eof() const { return buffer_empty() && m_eof; }

size_t ByteStream::bytes_written() const { return m_write_byte; }

size_t ByteStream::bytes_read() const { return m_read_byte; }

size_t ByteStream::remaining_capacity() const { return m_capacity - m_buffer.size(); }
