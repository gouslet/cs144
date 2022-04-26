#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <iostream>
#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    while (_window_size) {
        if (fin_acked || fin_sent) {
            return;
        }

        TCPSegment seg;

        if (!_stream.buffer_empty()) {
            if (next_seqno_absolute()) {  // SYN待发送
                auto unread_size = _stream.buffer_size();
                if (unread_size < _window_size) {
                    auto size = unread_size < TCPConfig::MAX_PAYLOAD_SIZE ? unread_size : TCPConfig::MAX_PAYLOAD_SIZE;
                    auto str = _stream.read(size);
                    if (_stream.input_ended()) {
                        seg = make_segment(next_seqno(), false, true, str);
                        _segments_out.push(seg);
                        fin_sent = true;
                        Out_Segment out_segment{
                            seg, next_seqno_absolute(), _initial_retransmission_timeout, zero_window_wize};
                        out_segments.push_back(out_segment);
                        _window_size -= seg.length_in_sequence_space();
                        _bytes_in_flight += seg.length_in_sequence_space();
                        _next_seqno += seg.length_in_sequence_space();
                        break;
                    } else {
                        seg = make_segment(next_seqno(), false, false, str);
                        _segments_out.push(seg);
                        Out_Segment out_segment{
                            seg, next_seqno_absolute(), _initial_retransmission_timeout, zero_window_wize};
                        out_segments.push_back(out_segment);
                        _window_size -= seg.length_in_sequence_space();
                        _bytes_in_flight += seg.length_in_sequence_space();
                        _next_seqno += seg.length_in_sequence_space();
                    }
                } else {
                    auto size = _window_size < TCPConfig::MAX_PAYLOAD_SIZE ? _window_size : TCPConfig::MAX_PAYLOAD_SIZE;
                    auto str = _stream.read(size);
                    seg = make_segment(next_seqno(), false, false, str);
                    _segments_out.push(seg);
                    Out_Segment out_segment{
                        seg, next_seqno_absolute(), _initial_retransmission_timeout, zero_window_wize};
                    out_segments.push_back(out_segment);
                    _window_size -= seg.length_in_sequence_space();
                    _bytes_in_flight += seg.length_in_sequence_space();
                    _next_seqno += seg.length_in_sequence_space();
                }
            } else {  // SYN已发送
                auto unread_size = _stream.buffer_size() - _stream.remaining_capacity();
                if (unread_size < _window_size - 1) {
                    if (unread_size <= TCPConfig::MAX_PAYLOAD_SIZE) {
                        auto str = _stream.read(unread_size);
                        seg = make_segment(next_seqno(), true, true, str);
                        _segments_out.push(seg);
                        fin_sent = true;
                        Out_Segment out_segment{
                            seg, next_seqno_absolute(), _initial_retransmission_timeout, zero_window_wize};
                        out_segments.push_back(out_segment);
                        _window_size -= seg.length_in_sequence_space();
                        _bytes_in_flight += seg.length_in_sequence_space();
                        _next_seqno += seg.length_in_sequence_space();
                        break;
                    } else {
                        auto str = _stream.read(TCPConfig::MAX_PAYLOAD_SIZE);
                        seg = make_segment(next_seqno(), true, false, str);
                        _segments_out.push(seg);
                        Out_Segment out_segment{
                            seg, next_seqno_absolute(), _initial_retransmission_timeout, zero_window_wize};
                        out_segments.push_back(out_segment);
                        _window_size -= seg.length_in_sequence_space();
                        _bytes_in_flight += seg.length_in_sequence_space();
                        _next_seqno += seg.length_in_sequence_space();
                    }
                } else {
                    if (unread_size < _window_size) {
                        auto size =
                            unread_size < TCPConfig::MAX_PAYLOAD_SIZE ? unread_size : TCPConfig::MAX_PAYLOAD_SIZE;
                        auto str = _stream.read(size);
                        seg = make_segment(next_seqno(), true, false, str);

                    } else {
                        auto size = _window_size - 1 > TCPConfig::MAX_PAYLOAD_SIZE ? TCPConfig::MAX_PAYLOAD_SIZE
                                                                                   : _window_size - 1;
                        auto str = _stream.read(size);
                        seg = make_segment(next_seqno(), true, false, str);
                    }
                    _segments_out.push(seg);
                    Out_Segment out_segment{
                        seg, next_seqno_absolute(), _initial_retransmission_timeout, zero_window_wize};
                    out_segments.push_back(out_segment);
                    _window_size -= seg.length_in_sequence_space();
                    _bytes_in_flight += seg.length_in_sequence_space();
                    _next_seqno += seg.length_in_sequence_space();
                }
            }

        } else {
            if (!next_seqno_absolute()) {
                if (_stream.input_ended() && _window_size >= 2) {
                    seg = make_segment(next_seqno(), true, true, "");
                    _segments_out.push(seg);
                    fin_sent = true;
                    Out_Segment out_segment{
                        seg, next_seqno_absolute(), _initial_retransmission_timeout, zero_window_wize};
                    out_segments.push_back(out_segment);
                    _window_size -= seg.length_in_sequence_space();
                    _bytes_in_flight += seg.length_in_sequence_space();
                    _next_seqno += 2;
                    break;
                } else {
                    seg = make_segment(next_seqno(), true, false, "");
                    _segments_out.push(seg);
                    Out_Segment out_segment{
                        seg, next_seqno_absolute(), _initial_retransmission_timeout, zero_window_wize};
                    out_segments.push_back(out_segment);
                    _window_size -= seg.length_in_sequence_space();
                    _bytes_in_flight += seg.length_in_sequence_space();
                    _next_seqno++;
                }
            } else if (_stream.input_ended()) {
                seg = make_segment(next_seqno(), false, true, "");
                _segments_out.push(seg);
                fin_sent = true;
                Out_Segment out_segment{seg, next_seqno_absolute(), _initial_retransmission_timeout, zero_window_wize};
                out_segments.push_back(out_segment);
                _window_size -= seg.length_in_sequence_space();
                _bytes_in_flight += seg.length_in_sequence_space();
                _next_seqno++;
                break;
            } else {
                break;
            }
        }
        zero_window_wize = false;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    auto _nq = unwrap(ackno, _isn, _next_seqno);
    if (_nq > _next_seqno) {
        return;
    }
    _window_size = window_size;

    // if (_nq < _next_seqno) {
    //     return;
    // }

    for (auto it = out_segments.begin(); it != out_segments.end();) {
        auto nq = unwrap(ackno, _isn, _next_seqno);
        if (it->ackno_absolute + it->seg.length_in_sequence_space() <= nq) {
            if (window_size != 0) {
                _window_size = window_size;
            } else {
                _window_size = 1;
                zero_window_wize = true;
            }
            _consecutive_retransmissions = 0;
            _bytes_in_flight -= it->seg.length_in_sequence_space();
            if (it->seg.header().fin) {
                fin_acked = true;
            }
            it = out_segments.erase(it);
        } else {
            if (it->ackno_absolute > _next_seqno) {
                _next_seqno = it->ackno_absolute;
            }
            if (it->ackno_absolute == nq) {
                _window_size = window_size;
            }
            it++;
        }
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (fin_acked || out_segments.empty()) {
        return;
    }

    auto oldest_seg = out_segments.begin();

    for (auto it = out_segments.begin(); it != out_segments.end(); it++) {
        if ((it->restrans_times > oldest_seg->restrans_times)) {
            oldest_seg = it;
        } else if (it->restrans_times == oldest_seg->restrans_times) {
            if (it->timeout < oldest_seg->timeout) {
                oldest_seg = it;
            }
        }
    }

    bool c = oldest_seg->timeout <= ms_since_last_tick;

    if (c) {
        oldest_seg->restrans_times++;
        _consecutive_retransmissions++;
        auto new_timeout = oldest_seg->zero ? _initial_retransmission_timeout
                                            : _initial_retransmission_timeout << oldest_seg->restrans_times;
        oldest_seg->timeout = new_timeout;
        _segments_out.push(oldest_seg->seg);
    } else {
        oldest_seg->timeout -= ms_since_last_tick;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg = make_segment(next_seqno(), false, false, "");

    _segments_out.push(seg);
}

TCPSegment TCPSender::make_segment(const WrappingInt32 seqno, const bool syn, const bool fin, const string &payload) {
    TCPSegment seg{};
    TCPHeader &header = seg.header();
    header.seqno = seqno;
    header.syn = syn;
    header.fin = fin;
    if (!payload.empty()) {
        seg.payload() = Buffer(string(move(payload)));
    }
    return seg;
}