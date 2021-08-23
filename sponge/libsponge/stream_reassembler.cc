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
    : _output(capacity), _capacity(capacity), next_index(), buffer(), unassem_bytes() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    string_2_buffer(data,index,eof);
    buffer_2_stream();
}

void StreamReassembler::string_2_buffer(const string& str,const size_t index,const bool eof) {
    auto str_hindex = index + str.length() - 1;
    auto str_len = str.length();
    if (str.empty() || buffer.empty()) {
        buffer.emplace(index,make_pair(str,eof));
        unassem_bytes += str_len;
    } else if (buffer.size() == 1) {
        auto s = buffer.begin();
        auto s_index = (*s).first;
        auto s_data = (*s).second.first;
        auto s_len = s_data.length();
        auto s_hindex = s_index + s_len - 1;
        if (index < s_index) {
            if (str_hindex < s_index) {
                buffer.emplace(index,make_pair(str,eof));
                unassem_bytes += str_len;
            }else {
                if (str_hindex >= s_hindex) {
                    buffer.clear();
                    buffer.emplace(index,make_pair(str,eof));
                    unassem_bytes += (str_len - s_len);
                }else {
                    auto ws = str.substr(0,s_index - index);
                    buffer.emplace(index,make_pair(ws,eof));
                    unassem_bytes += ws.length();
                }
            }
        }else {
            if (index > s_hindex) {
                buffer.emplace(index,make_pair(str,eof));
                unassem_bytes += str_len;
            }else {
                if (str_hindex <= s_hindex) {
                    return;
                } else {
                    auto ws = str.substr(s_hindex + 1 - index,str_hindex - s_hindex);
                    buffer.emplace(index,make_pair(ws,eof));
                    unassem_bytes += ws.length();
                }
            }
        }
    } else {
        string ds = str;
        size_t ds_index = index;
        auto ds_hindex = ds_index + ds.length() - 1;

        for (auto it = buffer.begin();it != buffer.end()&& !ds.empty();it++) {
            auto s_index = (*it).first;
            auto s = (*it).second.first;
            auto s_hindex = s_index + s.length() - 1;

            if (ds_index < s_index) {
                if (ds_hindex < s_index) {
                    buffer.emplace(ds_index,make_pair(ds,eof));
                    unassem_bytes += ds.length();
                    ds = "";
                }else {
                    if (ds_hindex <= s_hindex) {
                        auto len = s_index - ds_index;
                        auto ws = ds.substr(0,len);
                        buffer.emplace(ds_index,make_pair(ws,eof));
                        unassem_bytes += len;
                        ds = "";
                    } else {
                        auto len = s_index - ds_index;
                        auto ws = ds.substr(0,len);
                        buffer.emplace(ds_index,make_pair(ws,eof));
                        unassem_bytes += len;
                        ds = ds.substr(s_hindex + 1 - ds_index,ds_hindex - s_hindex);
                        ds_index = s_hindex + 1;
                    }
                }
            }else {
                if (ds_index > s_hindex) {
                    continue;
                }else if  (ds_hindex <= s_hindex) {
                    ds = "";
                } else {
                    ds = ds.substr(s_hindex + 1 - ds_index,ds_hindex - s_hindex);
                    ds_index = s_hindex + 1;
                }
            }
        }
     
        if (!ds.empty()) {
            buffer.emplace(ds_index,make_pair(ds,eof));
            unassem_bytes += ds.length();
        }        
    }
}

void StreamReassembler::buffer_2_stream() {
    bool eof(false);    
    for (auto it = buffer.begin();it != buffer.end();) {
        if (_output.input_ended()) {
            break;
        }else {
            auto rc = _output.remaining_capacity();
            if (rc == 0) {
                buffer.clear();
                unassem_bytes = 0;
                return;
            } else {
                auto s_index = (*it).first;
                auto s = (*it).second.first;
                auto s_hindex = s_index + s.length() - 1;
                if (s.empty()) {
                    auto s_eof = (*it).second.second;
                    // const string &ws = s;
                    if (s_index == next_index) {
                        _output.write(s);
                        next_index++;
                        buffer.erase(it++);
                        if (s_eof) {
                            eof = true;
                        }
                    } else {
                        it++;
                    }
                }else if (s_index <= next_index) {
                    if (s_hindex < next_index) {
                        buffer.erase(it++);
                        unassem_bytes -= s.length();
                    } else {
                        auto len = s_hindex - next_index + 1;
                        auto wlen = len > rc?rc:len;
                        auto s_eof = (*it).second.second;
                        string ws = s.substr(next_index - s_index,wlen);
                        _output.write(ws);
                        
                        next_index += wlen;
                        buffer.erase(it++);
                        unassem_bytes -= s.length();
                        if (wlen != len) {
                            auto ds = s.substr(next_index - s_index,s_hindex + 1 - next_index);
                            if (!ds.empty()) {
                            buffer.emplace(next_index,make_pair(ds,s_eof));
                            }
                            unassem_bytes += ds.length();
                        } else {
                            if (s_eof) {
                                eof = true;
                            }
                        }
                    }
                } else 
                    break;
            }
        }
    }

    if (eof) {
        _output.end_input();
    }

    auto ite = buffer.rbegin();
    auto rc = _output.remaining_capacity();

    while (ite != buffer.rend() && rc < unassem_bytes) {
        auto s = (*ite).second.first;
        auto s_len = s.length();
        auto len = unassem_bytes - rc > s_len?s_len:unassem_bytes - rc;
        auto ws = s.substr(0,s_len - len);
        if (!ws.empty()) {
            (*ite++).second.first = ws;
        } else {
            buffer.erase((*ite++).first);
        }
        unassem_bytes -= len;  
    }
}

size_t StreamReassembler::unassembled_bytes() const { 
    return unassem_bytes; 
}

bool StreamReassembler::empty() const { return buffer.empty(); }