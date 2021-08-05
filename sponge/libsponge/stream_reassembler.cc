#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) 
    : _output(capacity), _capacity(capacity), next_index(), buffer() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (buffer.size() < _output.remaining_capacity()) {
        if (index == next_index) {
            if (!_output.error() && !_output.input_ended()) {
                _output.write(data);
                buffer.erase(index);
                if (eof) {
                    _output.end_input();
                    return;
                }
            }
            size_t i = index + 1; 
            for (auto it = buffer.find(i);it != buffer.find(++i);it++,i++) {
                _output.write(((*it).second).first);
                buffer.erase(index);
                if (((*it).second).second) {
                    _output.end_input();
                    return;
                } 
            }
            next_index = i;
        } else {
            buffer.emplace(index,make_pair(data,eof));
        }
    }

}

size_t StreamReassembler::unassembled_bytes() const { return buffer.size(); }

bool StreamReassembler::empty() const { return buffer.empty(); }
