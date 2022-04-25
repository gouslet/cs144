#include "tcp_receiver.hh"

#include <iostream>
// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader header = seg.header();
    size_t abs_seqno = 0;
    if (header.syn) {
        // already get a SYN, refuse other SYN
        if (_syn_flag) {
            return;
        }
        // haven't get SYN, set isn
        _isn = header.seqno;
        _syn_flag = true;
        abs_seqno = 1;
    }

    // before get a SYN, refuse any segment
    if (!_syn_flag) {
        return;
    }

    // NOT a SYN segment, compute abs_seqno
    if (!header.syn) {
        abs_seqno = unwrap(header.seqno, _isn, abs_seqno);
    }

    if (header.fin) {
        // already get a FIN, refuse other FIN
        if (_fin_flag) {
            return;
        }
        _fin_flag = true;
    }

    if (abs_seqno == 0) {  // lab4测试特例, 0 - 1由于是无符号整数会变成一个超大数
        return;
    }
    // 2. push reassembler
    _reassembler.push_substring(seg.payload().copy(), abs_seqno - 1, header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_syn_flag) {  // null if haven't recv SYN
        return nullopt;
    }
    size_t first_unasm = _reassembler._first_unasm;  // stream_index
    uint64_t abs_seqno = first_unasm + 1;
    if (_reassembler.stream_out().input_ended()) {  // increase 1 if end
        abs_seqno++;
    }
    WrappingInt32 seqno = wrap(abs_seqno, _isn);
    return seqno;
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }