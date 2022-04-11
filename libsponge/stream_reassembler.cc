#include "stream_reassembler.hh"

#include <iostream>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), _unassembled_bytes(0), next_index(0), buffer(), _eof{false} {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // if (index > next_index + _capacity) {  //序号超过下一个待写入bytestream的字节序号与容量之和
    //     return;
    // }
    size_t l = data.length(), sd_start_index = index;
    string sd;

    if (index + l >= next_index) {
        if (index < next_index) {
            sd = data.substr(next_index - index);
            sd_start_index = next_index;
        } else {
            sd = data;
        }
    } else {
        return;
    }

    // for(auto it = buffer.lower_bound(sd_start_index);it != buffer.end();) {
    if (buffer.empty()) {  // buffer为空直接插入
        buffer.emplace(make_pair(sd_start_index, sd));
        _unassembled_bytes += sd.length();
    } else {
        auto it = buffer.begin();
        size_t len = it->second.length();
        size_t sd_end_index = sd_start_index + sd.length();
        size_t start_index = it->first;
        auto end_index = it->first + len;
        auto rit = buffer.rbegin();

        if (start_index >= sd_end_index || sd_start_index >= rit->first + rit->second.length()) {
            buffer.emplace(make_pair(sd_start_index, sd));
            _unassembled_bytes += sd.length();
        } else if (start_index >= sd_start_index) {
            if (end_index <= sd_end_index) {
                _unassembled_bytes -= it->second.length();
                buffer.emplace(make_pair(sd_start_index, sd));
                _unassembled_bytes += sd.length();
                it = buffer.erase(it);

                while (it != buffer.end()) {
                    if (auto it_len = it->second.length(); it->first + it_len <= sd_end_index) {
                        _unassembled_bytes -= it_len;
                    } else if (it->first < sd_end_index) {
                        size_t dl = sd_end_index - it->first;  //此处有问题
                        auto i = it->second.substr(dl);
                        // cout << typeid(i).name() << endl;
                        _unassembled_bytes -= dl;
                        buffer.emplace(make_pair(sd_start_index, i));
                    } else {
                        break;
                    }
                    it = buffer.erase(it);
                }
            } else {
                size_t dl = start_index - sd_start_index;
                buffer.emplace(make_pair(sd_start_index, sd.substr(0, dl) + it->second));
                buffer.erase(it->first);
                _unassembled_bytes += dl;
            }
        } else {
            while (end_index < sd_start_index && it != buffer.end()) {
                it++;
                end_index = it->first + it->second.length();
            }
            if (start_index = it->first; start_index > sd_end_index) {
                buffer.emplace(make_pair(sd_start_index, sd));
                _unassembled_bytes += sd.length();
            } else if (start_index > sd_start_index) {
                if (end_index > sd_end_index) {
                    buffer.emplace(make_pair(sd_start_index, sd + it->second.substr(sd_end_index - start_index)));
                    _unassembled_bytes += end_index - sd_end_index;
                    buffer.erase(it);
                } else if (end_index == sd_end_index) {
                    buffer.emplace(make_pair(sd_start_index, sd));
                    _unassembled_bytes += sd.length() - it->second.length();
                    buffer.erase(it);
                } else {
                    it->second += sd.substr(end_index - sd_start_index);
                    _unassembled_bytes += sd_end_index - end_index;
                    it++;
                    while (it != buffer.end()) {
                        if (it->first < sd_end_index) {
                            if (it->first + it->second.length() <= sd_end_index) {
                                _unassembled_bytes -= it->second.length();
                            } else {
                                size_t dl = sd_end_index - it->first;
                                auto i = it->second.substr(dl);
                                // cout << typeid(i).name() << endl;
                                _unassembled_bytes -= dl;
                                buffer.emplace(make_pair(sd_end_index, i));
                            }
                            it = buffer.erase(it);
                        } else {
                            break;
                        }
                    }
                }
            } else {
                if (end_index < sd_end_index) {
                    it->second += sd.substr(end_index - sd_start_index);

                    _unassembled_bytes += sd_end_index - end_index;
                    it++;
                    while (it != buffer.end()) {
                        if (it->first < sd_end_index) {
                            if (it->first + it->second.length() <= sd_end_index) {
                                _unassembled_bytes -= it->second.length();
                            } else {
                                size_t dl = sd_end_index - it->first;
                                auto i = it->second.substr(dl);
                                // cout << typeid(i).name() << endl;
                                _unassembled_bytes -= dl;
                                buffer.emplace(make_pair(sd_end_index, i));
                            }
                            it = buffer.erase(it);
                        } else {
                            break;
                        }
                    }
                }
            }
        }
    }

    // }

    auto s = buffer.begin();
    while (!buffer.empty() && s->first == next_index) {
        size_t b_written = _output.write(s->second);
        next_index += b_written;
        _unassembled_bytes -= b_written;
        buffer.erase(s->first);
        s = buffer.begin();
    }
    // J:
    if (eof) {
        _eof = true;
    }

    if (_eof && !_unassembled_bytes) {
        _output.end_input();
    }

    // cout << "next_index = " << next_index << endl;
    // // cout << index << " " << l + index - 1 << endl;
    // cout << "-----after push_string(" << index << " - " << l + index << ")---" << endl;
    // for (auto it = buffer.begin(); it != buffer.end(); it++) {
    //     cout << it->first << " - " << it->first + it->second.length() - 1 << endl;
    // }
    // cout << "-------------------------------------------\n";
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return !_unassembled_bytes; }
