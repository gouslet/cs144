#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    auto header = seg.header();
    bool isSYN = header.syn;
    bool eof = false;
    auto payload = seg.payload();
    if (isSYN && !isn_set) {
        isn = header.seqno;
        isn_set = true;
        // payload.remove_prefix(1);
    }
    if (header.fin) {
        // isn_set = false;
        eof = true;
        // payload.remove_prefix(1);
    }
    if (isn_set) {
        // if (next_seqn == header.seqno) {
            auto delta = (seg.length_in_sequence_space() > 0)?(seg.length_in_sequence_space()):1;
            next_seqn = header.seqno + delta;
        // }
        auto checkpoint = _capacity - window_size() - _reassembler.unassembled_bytes();
        auto index = unwrap(header.seqno,isn,checkpoint);
        _reassembler.push_substring(payload.copy(),index,eof);
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (!isn_set) {
        return nullopt;
    }
    
    return make_optional<WrappingInt32>(next_seqn);
}

size_t TCPReceiver::window_size() const { return _reassembler.stream_out().remaining_capacity() - _reassembler.unassembled_bytes(); }
