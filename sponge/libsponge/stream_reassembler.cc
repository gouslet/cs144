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
    size_t rc = _output.remaining_capacity();
    size_t ds_index = index;
    string ds(data);
    if (!rc) 
        return;
    // if (unassem_bytes < rc) {
    if (next_index >= ds_index) {
        string_2_stream(ds,ds_index,eof);   
    } else {
        if (buffer.empty()) {
            size_t wslen = ds.length() < rc?ds.length():rc;
            string ws(ds.substr(0,wslen));
            buffer.emplace(ds_index,make_pair(ws,eof));
            unassem_bytes += wslen;
            // ds_index += wslen;
        } else {
            for (auto it = buffer.begin();it != buffer.end();it++) {
                string s((*it).second.first);
                size_t s_index = (*it).first;
                if (s_index <= ds_index) {
                    if (ds.empty()) {
                        break;
                    }
                    if (s_index + s.length() < index) {
                        continue;
                    } else {
                        if (index + ds.length() > s_index + s.length()) {
                            size_t start = s_index + s.length() - ds_index;
                            size_t len = ds_index + ds.length() - s_index - s.length();
                            ds = ds.substr(start,len);
                            ds_index += start;
                        }
                    }
                } else {
                    size_t start = s_index + s.length() - ds_index;
                    if (ds_index + ds.length() <= s_index + s.length()) {
                       size_t len = ds_index + ds.length() - s_index - s.length();
                        ds = ds.substr(start,len);
                        ds_index += start;
                    }   
                }
                rc = _output.remaining_capacity();
                if (it == buffer.end() && unassem_bytes < rc) {
                    size_t wslen = ds.length() < rc?ds.length():rc;
                    string ws = ds.substr(0,wslen);
                    buffer.emplace(ds_index,make_pair(ws,eof));
                    unassem_bytes += wslen;
                    break;                
                }
                auto nit = ++it;
                size_t ns_index = (*nit).first;
                if (ns_index > ds_index) {
                    size_t wslen = ds.length() < rc?ds.length():rc;
                    string ws(ds.substr(0,wslen));
                    buffer.emplace(ds_index,make_pair(ws,eof));
                    unassem_bytes += wslen;
                    ds_index += wslen;
                    ds = ds.substr(wslen,ds.length() - wslen);
                }
                it--;
            }
        }
    }

    int overflow = unassem_bytes - _output.remaining_capacity();

    for (auto i = buffer.rbegin();i != buffer.rend();i++) {
        string s((*i).second.first);
        size_t s_index = (*i).first;
        size_t slen = s.length();

        if (overflow <= 0) {
            break;
        }
        if (slen - overflow > 0) {
            s = s.substr(0,slen - overflow);
            (*i).second.first = s;
            unassem_bytes -= overflow;
        } else {
            buffer.erase(s_index);
            overflow -= slen;
            unassem_bytes -= slen;
        }
    }
    //     if (index <= next_index) {
    //         if (index + data.length() < next_index){
    //             return;
    //         }
    //         if (!_output.error() && !_output.input_ended()) {
    //             size_t pos = next_index - index;
    //             if (data.length() > )
    //             size_t len = data.length() - pos;
    //             string ws = data.substr(pos,len);
    //             _output.write(ws);
    //             // if (buffer.erase(index))
    //                 // unassem_bytes -= data.length();
    //             if (eof) {
    //                 _output.end_input();
    //                 return;
    //             }
    //             next_index += ws.length();
    //         }
    //         // size_t i = index + 1;
    //         for (auto it = buffer.begin(); it != buffer.end(); it++) {
    //             string s = ((*it).second).first;
    //             size_t s_index = (*it).first;
    //             if (s_index <= next_index) {
    //                 if (s_index + s.length() < next_index) {
    //                     buffer.erase(s_index);
    //                     unassem_bytes -= s.length();
    //                 }
    //                 else {
    //                     if (!_output.error() && !_output.input_ended() && _output.remaining_capacity() > 0) {
    //                         size_t start = next_index - s_index - 1;
    //                         size_t len = s.length() - start - 1;
    //                         _output.write(s.substr(start, len));
    //                         next_index += len;
    //                         buffer.erase(s_index);
    //                         unassem_bytes -= s.length();
    //                     } else {
    //                         break;
    //                     }
    //                 }
    //             } 
    //         }
    //     } else {
    //         if (buffer.empty()) {
    //             buffer.emplace(index, make_pair(data, eof));
    //             unassem_bytes += data.length();
    //         }
    //         string ds = data;
    //         size_t ds_index = index;
    //         for (auto it = buffer.begin(); it != buffer.end(); it++) {
    //             string s = ((*it).second).first;
    //             size_t s_index = (*it).first;
    //             // if (s_index <= index) {
    //             //     if (s_index + s.length() < next_index)
    //             //         continue;
    //             //     else {
    //             //         size_t delta = s_index + s.length() - ds_index;
    //             //         ds = ds.substr(delta, s.length() - 1 - delta);
    //             //         ds_index += delta;
    //             //     }
    //             // } else {
    //                 size_t delta = s_index - ds_index;
    //                 buffer.emplace(ds_index, make_pair(ds.substr(0, delta + 1), eof));
    //                 unassem_bytes += delta + 1;
    //                 ds = ds.substr(delta + 1, ds.length() - delta);
    //                 ds_index += delta + 1;
    //             // }
    //         }
    //     }
    // }
}

void StreamReassembler::string_2_stream(string& str,size_t index,bool eof) {
    if (index + str.length() > next_index) {
        size_t start = next_index - index;  //str中可写入ByteStream的起始位置  
        size_t slen = str.length() - (next_index - index);  //str中index大于等于next_index的总长度
        size_t rc = _output.remaining_capacity();
        bool flag = rc < slen;
        size_t len = flag?rc:slen;  //str中可写入ByteStream的长度
        string ws(str.substr(start,len));   //待写入ByteStream的str子串
        _output.write(ws);
        if (eof) {
            _output.end_input();
        }
        next_index += len;  
        if (!flag) {
            buffer_2_stream();
        } else {
            buffer.clear();
            unassem_bytes = 0;
        }
    }
}

void StreamReassembler::buffer_2_stream() {
    if (!buffer.empty()) {
        for (auto it = buffer.begin();it != buffer.end();it++) {
            string s((*it).second.first);
            size_t s_index = (*it).first;
            bool eof = (*it).second.second;
            if (s_index <= next_index) {
                if (s_index + s.length() < next_index) {
                    buffer.erase(s_index);
                    unassem_bytes -= s.length();
                } else 
                    string_2_stream(s,s_index,eof);
            } else {
                break;
            }
        }
    }

}

size_t StreamReassembler::unassembled_bytes() const { 
    return unassem_bytes; 
}

bool StreamReassembler::empty() const { return buffer.empty(); }