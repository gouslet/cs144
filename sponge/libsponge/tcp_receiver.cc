#include "tcp_receiver.hh"
#include <iostream>
// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    
    auto header = seg.header();
    bool eof = false;
    auto payload = seg.payload();

    if (header.syn&&!isn_set) {
        next_seqn = isn = header.seqno;
        isn_set = true;
                next_seqn = next_seqn + 1;
    }

    if (header.fin) {
        eof = true;
    }
    if (isn_set) {
        uint64_t old_written_bytes = _capacity - _reassembler.stream_out().remaining_capacity();//本次接收帧之前已写入ByteStream的字节数
        auto checkpoint = _capacity - window_size() - _reassembler.unassembled_bytes();
        auto index = unwrap(header.seqno,isn,checkpoint);
        _reassembler.push_substring(payload.copy(),index,eof);
        uint64_t new_written_bytes = _capacity - _reassembler.stream_out().remaining_capacity();//本次接收帧之后已写入ByteStream的字节数
        next_seqn = next_seqn + (new_written_bytes - old_written_bytes);
        
    }
    if (header.fin) {
        if (_reassembler.unassembled_bytes() == 0){
            next_seqn = next_seqn + 1;
        }
    } else {
        if (_reassembler.stream_out().input_ended()) {
            next_seqn = next_seqn + 1;
        }
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (!isn_set) {
        return nullopt;
    }
    return make_optional<WrappingInt32>(next_seqn);
}

size_t TCPReceiver::window_size() const { return _reassembler.stream_out().remaining_capacity(); }
