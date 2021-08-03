#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity):bas{},cap{capacity},size{},b_read{},b_written{},i_ended{false}, o_ended{false},_error{false}{ }

size_t ByteStream::write(const string &data) {
    size_t i = 0;
    for (auto it = data.begin();it != data.end();++it) {
        if (!_error && !i_ended && size < cap) {
            bas.push_back(*it);
            i++;
            size++;
        } else {
            set_error();
            break;
        }
    }
    b_written += i;
    return i;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string s;
    if (!_error && size >= len) {
        for (size_t i = 0;i < len;i++) {
            s.push_back(bas.at(size - 1 - i));
        }
        return s;
    }
    return nullptr;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    if (_error) {
        return;
    }
    if (len < size) {
        set_error();
        return;
    }
    for (size_t i = 0;i < len;i++) {
        bas.pop_front();
    }
    b_read += len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string s;
    for (size_t i = 0;i < len;i++) {
        if (!_error && !o_ended && size > 0) {
            s.push_back(move(bas.front()));
            bas.pop_front();
            b_read++;
            size--;
        } else {
            set_error();
            break;
        }
    }
    return s;
}

void ByteStream::end_input() { i_ended = true; }

bool ByteStream::input_ended() const { return i_ended; }

size_t ByteStream::buffer_size() const { return size; }

bool ByteStream::buffer_empty() const { return cap > 0 && size == 0; }

bool ByteStream::eof() const { return o_ended || size <= 0; }

size_t ByteStream::bytes_written() const { return b_written; }

size_t ByteStream::bytes_read() const { return b_read; }

size_t ByteStream::remaining_capacity() const { 
    return cap - size; 
}
