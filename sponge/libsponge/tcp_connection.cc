#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;
enum STATE {
    LISTEN,
    SYN_SENT,
    SYN_RCVD,
    ESTABLISHED,
    CLOSE_WAIT,
    FIN_WAIT_1,
    FIN_WAIT_2,
    LAST_ACK,
    TIME_WAIT,
    CLOSING
};
STATE connection_state = LISTEN;
bool second_fin{false};
bool fin_acked{false};

// 至少产生一个TCPSegment
void TCPConnection::must_generate_segment(bool fin = false) {
    _sender.fill_window();
    if (_sender.segments_out().empty()) {
        _sender.send_empty_segment();
    }
    if (fin) {
        _sender.segments_out().back().header().fin = true;
    }
}

// 把segment从sender的发送队列中取出，扔到网络里(也就是放到_segments_out里)
void TCPConnection::send_segments() {
    TCPSegment seg;
    while (!_sender.segments_out().empty()) {
        seg = _sender.segments_out().front();
        // 加上ack和win_size
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }
        // unclean shutdown
        if (_sender.stream_in().error() || _receiver.stream_out().error()) {
            seg.header().rst = true;
        }
        /**
         cout << "send A =" << seg.header().ack  << " S = " << seg.header().syn << \
         " F =" << seg.header().fin << " R = " << seg.header().rst << " seqno = " << \
         seg.header().seqno << " ackno = " << seg.header().ackno << endl;
         **/
        cout << "Sent: \n" << seg.header().to_string() << endl;
        // if (seg.length_in_sequence_space()) {
        _segments_out.push(seg);
        _sender.segments_out().pop();
        // }
    }
}

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    cout << "Received: \n" << seg.header().to_string() << endl;

    auto header = seg.header();
    if (header.rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _active = false;
        return;
    }
    // if (connection_state != TIME_WAIT) {
    //     _time_since_last_segment_received = 0;
    // }
    _time_since_last_segment_received = 0;

    if (connection_state != CLOSE_WAIT) {
        _receiver.segment_received(seg);
    }
    // auto data = seg.payload().copy();

    // _sender.fill_window();
    _sender.ack_received(header.ackno, header.win);

    switch (connection_state) {
        case LISTEN:
            if (header.syn) {
                must_generate_segment();

                send_segments();
                connection_state = SYN_RCVD;
                syn_sent = true;
            }
            break;

        case SYN_SENT:
            if (header.syn) {
                must_generate_segment();
                if (header.ack) {
                    connection_state = ESTABLISHED;
                } else {
                    connection_state = SYN_RCVD;
                    // s.header().syn = true;
                }
                send_segments();
            }
            break;

        case ESTABLISHED:
            if (seg.length_in_sequence_space() > 0)
                must_generate_segment();
            if (header.fin) {
                _linger_after_streams_finish = false;
                connection_state = CLOSE_WAIT;
            }
            send_segments();
            break;

        case SYN_RCVD:
            if (header.ack) {
                connection_state = ESTABLISHED;
            }
            break;

        case CLOSE_WAIT:
            if (header.ack) {
                if (header.fin) {
                    must_generate_segment();
                }
                send_segments();
            }

            break;

        case LAST_ACK:
            if (header.ack && header.ackno == _sender.next_seqno()) {
                _active = false;
            }
            break;
        case FIN_WAIT_1:
            if (header.ack) {
                if (header.fin) {
                    must_generate_segment();
                    send_segments();
                    connection_state = TIME_WAIT;
                } else {
                    connection_state = FIN_WAIT_2;
                }
            } else {
                if (header.fin) {
                    must_generate_segment();
                    send_segments();
                    connection_state = CLOSING;
                }
            }
            break;

        case FIN_WAIT_2:
            if (header.fin) {
                must_generate_segment();
                send_segments();
                connection_state = TIME_WAIT;
            }
            break;

        case CLOSING:
            if (header.ack) {
                connection_state = TIME_WAIT;
            }
            break;
        case TIME_WAIT:
            if (header.fin) {
                second_fin = true;
                must_generate_segment();
                // send_segments();
            }
            break;

        default:
            cout << "0";
    }
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    // DUMMY_CODE(data);
    return _sender.stream_in().write(data);
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if (connection_state == LISTEN || connection_state == SYN_RCVD || connection_state == SYN_SENT) {
        return;
    }
    if (connection_state == ESTABLISHED) {
        send_segments();
        return;
    }

    if (connection_state == TIME_WAIT || connection_state == FIN_WAIT_1 || connection_state == CLOSING ||
        connection_state == LAST_ACK) {
        if (time_since_last_segment_received() < 10 * _cfg.rt_timeout) {
            if (time_since_last_segment_received() >= _cfg.rt_timeout) {
                // cout << second_fin << endl;
                if (connection_state == TIME_WAIT) {
                    if (second_fin) {
                        return;
                    }
                } else {
                    fin_acked = true;
                }
                must_generate_segment(true);
                send_segments();
            }
        } else {
            _active = false;
        }
    }
}

// if (!connected) {
//      _sender.fill_window();
// }

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    if (connection_state == ESTABLISHED || connection_state == SYN_RCVD || connection_state == CLOSE_WAIT) {
        must_generate_segment(true);
        send_segments();

        _time_since_last_segment_received = 0;
        if (!_linger_after_streams_finish) {
            connection_state = LAST_ACK;
        } else {
            connection_state = FIN_WAIT_1;
        }
    }
    // _written_finished = true;
    // // _sender.fill_window();
    // if (_sender.segments_out().empty()) {
    //     _sender.send_empty_segment();
    // }
    // auto s = _sender.segments_out().front();
    // // bool a = _sender.next_seqno_absolute() > 0 && _sender.next_seqno_absolute() == bytes_in_flight();
    // s.header().fin = true;
    // if (_receiver.ackno().has_value()) {
    //     s.header().ackno = _receiver.ackno().value();
    //     s.header().ack = true;
    // }
    // s.header().win = _receiver.window_size();
    // if (s.length_in_sequence_space()) {
    //     _segments_out.push(s);
    //     _sender.segments_out().pop();
    // }
}

void TCPConnection::connect() {
    _sender.fill_window();

    // _segments_out = _sender.segments_out();
    auto s = _sender.segments_out().front();
    s.header().syn = true;
    _segments_out.push(s);
    _sender.segments_out().pop();

    connected = true;
    syn_sent = true;
    connection_state = SYN_SENT;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // while (!_sender.segments_out().empty()) {
            //     auto s = _sender.segments_out().front();
            //     s.header().rst = true;
            //     _segments_out.push(s);
            //     _sender.segments_out().pop();
            // }
            // if (_sender.stream_in().error() && _receiver.stream_out().error()) {
            //     _active = false;
            // }
        }
        // Your code here: need to send a RST segment to the peer
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}