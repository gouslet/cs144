#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return {}; }

void TCPConnection::segment_received(const TCPSegment &seg) { 
    auto header = seg.header();
    _receiver.segment_received(seg);

    if (header.ack) {
        if (ack_times == 3) {
            _sender.ack_received(header.seqno,header.win);
            // if (header.fin) {
            //     _sender.send_empty_segment();            
            // }
            // auto next_seqno = _receiver.ackno();
            // auto win_size = _receiver.window_size();
            // auto segments = _sender.segments_out();

        } else {
            ack_times++;    
            
            _sender.ack_received(_sender.next_seqno(),header.win);
            // _sender.send_empty_segment();
            // _sender.fill_window();
        }


    }
    _sender.fill_window();

    while (!_sender.segments_out().empty()) {
        auto s = _sender.segments_out().front();
        if (_receiver.ackno().has_value()) {
            s.header().ackno = _receiver.ackno().value();
            s.header().ack = true;
            s.header().syn = false;
        }
        s.header().win = _receiver.window_size();
        if (_written_finished && header.fin) {
            s.header().fin = true;
            _written_finished = true;
        }
        if (seg.length_in_sequence_space()) {
            _segments_out.push(s);
            _sender.segments_out().pop();
        }
    }
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    DUMMY_CODE(data);
    return {};
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    _sender.tick(ms_since_last_tick); 
    
    // if (!connected) {
    //      _sender.fill_window();
    // }
}

void TCPConnection::end_input_stream() { 
    _written_finished = true;
    _sender.fill_window();
}

void TCPConnection::connect() {
    _sender.fill_window();
    _segments_out = _sender.segments_out();
    _active = true;
    connected = true;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            
            _sender.fill_window();
            
            while (!_sender.segments_out().empty()) {
                auto s = _sender.segments_out().front();
                s.header().rst = true;
                _segments_out.push(s);
                _sender.segments_out().pop();
            }
            if (_sender.stream_in().error() && _receiver.stream_out().error()) {
                _active = false;
            }
        }
            // Your code here: need to send a RST segment to the peer
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
