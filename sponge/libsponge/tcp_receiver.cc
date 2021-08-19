#include "tcp_receiver.hh"
#include <iostream>
// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    cout << "_reassembler.unassembled_bytes() = " << _reassembler.unassembled_bytes() << endl;
    cout << "seg.length_in_sequence_space() = " << seg.length_in_sequence_space() << endl;
    
    auto header = seg.header();
    bool isSYN = header.syn;
    bool eof = false;
    auto payload = seg.payload();
    bool flag = isSYN;
    if (flag&&!isn_set) {
        next_seqn = isn = header.seqno;
        isn_set = true;
                next_seqn = next_seqn + 1;
        // auto delta = seg.length_in_sequence_space() > 1?0:1;
        // payload.remove_prefix(1);
    }
    // if (header.syn) {
    //     if (header.fin) {
    //         if (seg.length_in_sequence_space()  <= 2) {
    //             next_seqn = next_seqn + 2;
    //         }
    //     } else {
    //         // if (seg.length_in_sequence_space()  < 2) {
    //             next_seqn = next_seqn + 1;
    //         // }
    //     }
    // } else {
    //     if (header.fin) {
    //         // if (seg.length_in_sequence_space()  < 2) {
    //             next_seqn = next_seqn + 1;
    //         // }
    //     } 
    // }
    if (header.fin) {
        // isn_set = false;
        eof = true;
                next_seqn = next_seqn + 1;
        // auto delta = seg.length_in_sequence_space() > 1 || flag?1:2;
        // next_seqn = next_seqn + delta;
        // payload.remove_prefix(1);
    }
    if (isn_set) {
        // if (next_seqn == header.seqno) {
        //     auto delta = (seg.length_in_sequence_space() > 0)?(seg.length_in_sequence_space()):1;
        //     next_seqn = header.seqno + delta;
        // }
        uint64_t old_written_bytes = _capacity - _reassembler.stream_out().remaining_capacity();//本次接收帧之前已写入ByteStream的字节数
        cout << "old_written_bytes = " << old_written_bytes << endl;
        auto checkpoint = _capacity - window_size() - _reassembler.unassembled_bytes();
        auto index = unwrap(header.seqno,isn,checkpoint);
        _reassembler.push_substring(payload.copy(),index,eof);
        uint64_t new_written_bytes = _capacity - _reassembler.stream_out().remaining_capacity();//本次接收帧之后已写入ByteStream的字节数
        cout << "new_written_bytes = " << new_written_bytes << endl;
        next_seqn = next_seqn + (new_written_bytes - old_written_bytes);
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (!isn_set) {
        return nullopt;
    }
    return make_optional<WrappingInt32>(next_seqn);
}

size_t TCPReceiver::window_size() const { return _reassembler.stream_out().remaining_capacity() - _reassembler.unassembled_bytes(); }
