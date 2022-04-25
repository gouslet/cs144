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
bool fin_rcvd{false};

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    auto header = seg.header();
    if (header.rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _active = false;
        return;
    }
    if (connection_state != TIME_WAIT) {
        _time_since_last_segment_received = 0;
    }
    if (connection_state != CLOSE_WAIT) {
        _receiver.segment_received(seg);
    }
    // auto data = seg.payload().copy();

    // _sender.fill_window();

    switch (connection_state) {
        case LISTEN:
            _sender.ack_received(header.ackno, header.win);
            if (header.syn) {
                _sender.fill_window();
                auto s = _sender.segments_out().front();
                s.header().syn = true;
                if (_receiver.ackno().has_value()) {
                    s.header().ackno = _receiver.ackno().value();
                    s.header().ack = true;
                }
                s.header().win = _receiver.window_size();
                _segments_out.push(s);
                _sender.segments_out().pop();
                connection_state = SYN_RCVD;
                syn_sent = true;
            }
            break;

        case SYN_SENT:
            _sender.ack_received(header.ackno, header.win);
            if (header.syn) {
                _sender.fill_window();
                if (_sender.segments_out().empty()) {
                    _sender.send_empty_segment();
                }
                auto s = _sender.segments_out().front();
                if (header.ack) {
                    connection_state = ESTABLISHED;
                } else {
                    connection_state = SYN_RCVD;
                    // s.header().syn = true;
                }
                if (_receiver.ackno().has_value()) {
                    s.header().ackno = _receiver.ackno().value();
                    s.header().ack = true;
                }
                s.header().win = _receiver.window_size();
                _segments_out.push(s);
                _sender.segments_out().pop();
            }
            break;

        case ESTABLISHED:
            _sender.ack_received(header.ackno, header.win);
            _sender.fill_window();
            // bool fl{false};
            // bool fin_st{false};
            // _receiver.stream_out().write(data);
            if (header.fin) {
                if (_sender.segments_out().empty()) {
                    _sender.send_empty_segment();
                }
                _linger_after_streams_finish = false;
                connection_state = CLOSE_WAIT;
            }

            while (!_sender.segments_out().empty()) {
                auto s = _sender.segments_out().front();
                if (_receiver.ackno().has_value()) {
                    s.header().ackno = _receiver.ackno().value();
                    s.header().ack = true;
                }
                s.header().win = _receiver.window_size();
                if (seg.length_in_sequence_space()) {
                    //         _segments_out.push(s);
                    //         _sender.segments_out().pop();
                    _sender.segments_out().pop();
                    _segments_out.push(s);
                }
            }
            break;

        case SYN_RCVD:
            _sender.ack_received(header.ackno, header.win);
            if (header.ack) {
                connection_state = ESTABLISHED;
            }
            break;

        case CLOSE_WAIT:
            _sender.ack_received(header.ackno, header.win);
            if (header.ack) {
                if (header.fin) {
                    _sender.fill_window();
                    if (_sender.segments_out().empty()) {
                        _sender.send_empty_segment();
                    }
                }
                while (!_sender.segments_out().empty()) {
                    auto s = _sender.segments_out().front();
                    if (_receiver.ackno().has_value()) {
                        s.header().ackno = _receiver.ackno().value();
                        s.header().ack = true;
                    }
                    s.header().win = _receiver.window_size();
                    if (seg.length_in_sequence_space()) {
                        _sender.segments_out().pop();
                        _segments_out.push(s);
                    }
                }
            }

            break;

        case LAST_ACK:
            _sender.ack_received(header.ackno, header.win);
            if (header.ack && header.ackno == _sender.next_seqno()) {
                _active = false;
            }
            break;
        case FIN_WAIT_1:
            _sender.ack_received(header.ackno, header.win);
            if (header.ack) {
                if (header.fin) {
                    _sender.fill_window();
                    if (_sender.segments_out().empty()) {
                        _sender.send_empty_segment();
                    }
                    auto s = _sender.segments_out().front();
                    if (_receiver.ackno().has_value()) {
                        s.header().ackno = _receiver.ackno().value();
                        s.header().ack = true;
                    }
                    s.header().win = _receiver.window_size();
                    _segments_out.push(s);
                    _sender.segments_out().pop();
                    connection_state = TIME_WAIT;
                } else {
                    connection_state = FIN_WAIT_2;
                }
            } else {
                if (header.fin) {
                    _sender.fill_window();
                    if (_sender.segments_out().empty()) {
                        _sender.send_empty_segment();
                    }
                    auto s = _sender.segments_out().front();
                    if (_receiver.ackno().has_value()) {
                        s.header().ackno = _receiver.ackno().value();
                        s.header().ack = true;
                    }
                    s.header().win = _receiver.window_size();
                    _segments_out.push(s);
                    _sender.segments_out().pop();
                    connection_state = CLOSING;
                }
            }
            break;

        case FIN_WAIT_2:
            _sender.ack_received(header.ackno, header.win);
            if (header.fin) {
                _sender.fill_window();
                if (_sender.segments_out().empty()) {
                    _sender.send_empty_segment();
                }
                auto s = _sender.segments_out().front();
                if (_receiver.ackno().has_value()) {
                    s.header().ackno = _receiver.ackno().value();
                    s.header().ack = true;
                }
                s.header().win = _receiver.window_size();
                _segments_out.push(s);
                _sender.segments_out().pop();
                connection_state = TIME_WAIT;
            }
            break;

        case CLOSING:
            _sender.ack_received(header.ackno, header.win);
            if (header.ack) {
                connection_state = TIME_WAIT;
            }
            break;
        case TIME_WAIT:
            _sender.ack_received(header.ackno, header.win);
            if (header.fin) {
                _sender.fill_window();
                if (_sender.segments_out().empty()) {
                    _sender.send_empty_segment();
                }
                auto s = _sender.segments_out().front();
                if (_receiver.ackno().has_value()) {
                    s.header().ackno = _receiver.ackno().value();
                    s.header().ack = true;
                }
                s.header().win = _receiver.window_size();
                _segments_out.push(s);
                _sender.segments_out().pop();
            }
            break;

        default:
            cout << "0";
    }
    // if (header.ack) {
    //     syn_sent = true;
    //     _sender.ack_received(header.ackno, header.win);
    //     if (header.fin) {
    //         _sender.send_empty_segment();
    //     }
    //     // auto next_seqno = _receiver.ackno();
    //     // auto win_size = _receiver.window_size();
    //     // auto segments = _sender.segments_out();

    //     // _sender.ack_received(_sender.next_seqno(), header.win);
    //     //     // _sender.send_empty_segment();
    //     //     // _sender.fill_window();
    // }

    // while (!_sender.segments_out().empty()) {
    //     auto s = _sender.segments_out().front();
    //     // bool a = _sender.next_seqno_absolute() > 0 && _sender.next_seqno_absolute() == bytes_in_flight();
    //     if (header.syn && !syn_sent) {
    //         s.header().syn = true;
    //     } else {
    //         //     s.header().syn = true;
    //         //     ack_times++;
    //         // } else {
    //         s.header().syn = false;
    //     }
    //     if (_receiver.ackno().has_value()) {
    //         s.header().ackno = _receiver.ackno().value();
    //         s.header().ack = true;
    //     }
    //     s.header().win = _receiver.window_size();
    //     if (_written_finished && header.fin) {
    //         s.header().fin = true;
    //         _written_finished = true;
    //     }
    //     if (seg.length_in_sequence_space()) {
    //         _segments_out.push(s);
    //         _sender.segments_out().pop();
    //     }
    // }
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
    if (connection_state == TIME_WAIT || connection_state == FIN_WAIT_1 || connection_state == CLOSING ||
        connection_state == LAST_ACK) {
        if (time_since_last_segment_received() < 10 * _cfg.rt_timeout) {
            if (time_since_last_segment_received() >= _cfg.rt_timeout) {
                // _sender.fill_window();
                // if (_sender.segments_out().empty()) {
                _sender.send_empty_segment();
                // }

                auto s = _sender.segments_out().front();
                if (_receiver.ackno().has_value()) {
                    s.header().ackno = _receiver.ackno().value();
                    s.header().ack = true;
                }
                s.header().fin = true;
                s.header().win = _receiver.window_size();
                _segments_out.push(s);
                _sender.segments_out().pop();
                cout << s.header().to_string() << endl;
            }
        } else {
            _active = false;
        }
    }

    // if (!connected) {
    //      _sender.fill_window();
    // }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    if (connection_state == ESTABLISHED || connection_state == SYN_RCVD || connection_state == CLOSE_WAIT) {
        _sender.fill_window();
        if (_sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
        auto s = _sender.segments_out().front();
        s.header().fin = true;
        if (_receiver.ackno().has_value()) {
            s.header().ackno = _receiver.ackno().value();
            s.header().ack = true;
        }
        s.header().win = _receiver.window_size();
        if (s.length_in_sequence_space()) {
            _segments_out.push(s);
            _sender.segments_out().pop();
        }
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