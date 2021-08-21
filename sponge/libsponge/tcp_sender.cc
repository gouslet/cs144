#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { 
    return _bytes_in_flight; 
}

void TCPSender::fill_window() {
    
    if (fin_acked || fin_sent) {
        return;
    }

    // std::cout << "_window_size = " << _window_size << std::endl;
    while (_window_size) {
        TCPSegment seg;
        // std::cout << "_stream.buffer_empty() = " << _stream.buffer_empty() << std::endl;
        // std::cout << "next_seqno_absolute() = " << next_seqno_absolute() << std::endl;
        if (!_stream.buffer_empty()) {
            if (next_seqno_absolute()) {    //SYN待发送
                auto unread_size = _stream.buffer_size();
                if (unread_size < _window_size) {
                    auto size = unread_size < TCPConfig::MAX_PAYLOAD_SIZE?unread_size:TCPConfig::MAX_PAYLOAD_SIZE;
                    auto str = _stream.read(size);
                    if (_stream.input_ended()) {
                        seg = make_segment(next_seqno(),false,true,str);
                        _segments_out.push(seg);
                        fin_sent = true;
                        Out_Segment out_segment{seg,next_seqno_absolute(),_initial_retransmission_timeout};
                        out_segments.push_back(out_segment);
                        _window_size -= seg.length_in_sequence_space();
                        _bytes_in_flight += seg.length_in_sequence_space();
                        _next_seqno += seg.length_in_sequence_space();
                        break;
                    } else {
                        seg = make_segment(next_seqno(),false,false,str);
                        _segments_out.push(seg);
                        Out_Segment out_segment{seg,next_seqno_absolute(),_initial_retransmission_timeout};
                        out_segments.push_back(out_segment);
                        _window_size -= seg.length_in_sequence_space();
                        _bytes_in_flight += seg.length_in_sequence_space();
                        _next_seqno += seg.length_in_sequence_space();
                    }
                } else {
                    auto size = _window_size < TCPConfig::MAX_PAYLOAD_SIZE?_window_size:TCPConfig::MAX_PAYLOAD_SIZE;
                    auto str = _stream.read(size);
                    seg = make_segment(next_seqno(),false,false,str);
                    _segments_out.push(seg);
                    Out_Segment out_segment{seg,next_seqno_absolute(),_initial_retransmission_timeout};
                    out_segments.push_back(out_segment);
                    _window_size -= seg.length_in_sequence_space();
                    _bytes_in_flight += seg.length_in_sequence_space();
                    _next_seqno += seg.length_in_sequence_space();
                }
            } else {    //SYN已发送
                auto unread_size = _stream.buffer_size()-_stream.remaining_capacity();
                if (unread_size < _window_size - 1) {
                    if (unread_size <= TCPConfig::MAX_PAYLOAD_SIZE) {
                        auto str = _stream.read(unread_size);
                        seg = make_segment(next_seqno(),true,true,str);
                        _segments_out.push(seg);
                        fin_sent = true;
                        Out_Segment out_segment{seg,next_seqno_absolute(),_initial_retransmission_timeout};
                        out_segments.push_back(out_segment);
                        _window_size -= seg.length_in_sequence_space();
                        _bytes_in_flight += seg.length_in_sequence_space();
                        _next_seqno += seg.length_in_sequence_space();
                        break;
                    } else {
                        auto str = _stream.read(TCPConfig::MAX_PAYLOAD_SIZE);
                        seg = make_segment(next_seqno(),true,false,str);
                        _segments_out.push(seg);
                        Out_Segment out_segment{seg,next_seqno_absolute(),_initial_retransmission_timeout};
                        out_segments.push_back(out_segment);
                        _window_size -= seg.length_in_sequence_space();
                        _bytes_in_flight += seg.length_in_sequence_space();
                        _next_seqno += seg.length_in_sequence_space();
                    }
                } else if (unread_size < _window_size) {
                        auto size = unread_size < TCPConfig::MAX_PAYLOAD_SIZE?unread_size:TCPConfig::MAX_PAYLOAD_SIZE;
                        auto str = _stream.read(size);
                        seg = make_segment(next_seqno(),true,false,str);
                        _segments_out.push(seg);
                        Out_Segment out_segment{seg,next_seqno_absolute(),_initial_retransmission_timeout};
                        out_segments.push_back(out_segment);
                        _window_size -= seg.length_in_sequence_space();
                        _bytes_in_flight += seg.length_in_sequence_space();
                        _next_seqno += seg.length_in_sequence_space();
                } else {
                        auto size = _window_size - 1 > TCPConfig::MAX_PAYLOAD_SIZE?TCPConfig::MAX_PAYLOAD_SIZE:_window_size - 1;
                        auto str = _stream.read(size);
                        seg = make_segment(next_seqno(),true,false,str);
                        _segments_out.push(seg);
                        Out_Segment out_segment{seg,next_seqno_absolute(),_initial_retransmission_timeout};
                        out_segments.push_back(out_segment);
                        _window_size -= seg.length_in_sequence_space();
                        _bytes_in_flight += seg.length_in_sequence_space();
                        _next_seqno += seg.length_in_sequence_space();
                }
            } 

        } else {
            if (!next_seqno_absolute()) {
                if (_stream.input_ended() && _window_size >= 2) {
                    seg = make_segment(next_seqno(),true,true,"");
                    _segments_out.push(seg);
                    fin_sent = true;
                    Out_Segment out_segment{seg,next_seqno_absolute(),_initial_retransmission_timeout};
                    out_segments.push_back(out_segment);
                    _window_size -= seg.length_in_sequence_space();
                    _bytes_in_flight += seg.length_in_sequence_space();
                    _next_seqno += 2;
                    break;
                } else {
                    seg = make_segment(next_seqno(),true,false,"");
                    _segments_out.push(seg);
                    Out_Segment out_segment{seg,next_seqno_absolute(),_initial_retransmission_timeout};
                    out_segments.push_back(out_segment);
                    _window_size -= seg.length_in_sequence_space();
                    _bytes_in_flight += seg.length_in_sequence_space();
                    _next_seqno++;
                }
            } else if (_stream.input_ended()) {
                    seg = make_segment(next_seqno(),false,true,"");
                    _segments_out.push(seg);
                    fin_sent = true;
                    Out_Segment out_segment{seg,next_seqno_absolute(),_initial_retransmission_timeout};
                    out_segments.push_back(out_segment);
                    _window_size -= seg.length_in_sequence_space();
                    _bytes_in_flight += seg.length_in_sequence_space();
                    _next_seqno++;
                    break;
            } else {
                break;
            }
        }
    }
    // std::cout << "sender.next_seqno_absolute() = " << next_seqno_absolute() << std::endl;
    // std::cout << "bytes_in_flight() = " << bytes_in_flight() << std::endl;
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    _window_size = window_size;
    for (auto it = out_segments.begin();it != out_segments.end();) {
        auto nq = unwrap(ackno,_isn,_next_seqno);
        if ((*it).ackno() + (*it).segment().length_in_sequence_space() <= nq) {
            _consecutive_retransmissions = 0;
            _bytes_in_flight -= (*it).segment().length_in_sequence_space();
            if ((*it).segment().header().fin) {
                fin_acked = true;
            }
            it = out_segments.erase(it);
        } else {
            if ((*it).ackno() > _next_seqno) {
                _next_seqno = (*it).ackno();
            }
            it++;
        }
    }
    // fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 

    auto oldest_seg = out_segments.begin();
    // auto begin_timeout = (*oldest_seg).timeout();
    for (auto it = out_segments.begin();it != out_segments.end();it++) {
        // if (auto t = (*it).timeout();t > ms_since_last_tick) {
        // if ((*it).timeout() <= ms) {
            // (*it).timeout() -= ms;
        // } else if ((*it).timeout() == ms){
        //     (*it).restrans_time()++;
        //     _consecutive_retransmissions++;
        //     (*it).timeout() = _initial_retransmission_timeout << (*it).restrans_time();
        //     _segments_out.push((*it).segment());
        // } else {
        //     ms = ms_since_last_tick - (*it).timeout();
        //     tick((*it).timeout());
        //     tick(ms);
        //     break;
        // }
        // } else {
            if (((*it).restrans_time() > (*oldest_seg).restrans_time())) {
                oldest_seg = it;
            } else if ((*it).restrans_time() == (*oldest_seg).restrans_time()){
                if  ((*it).timeout() < (*oldest_seg).timeout()) {
                    oldest_seg = it;
                }
            }
        // }
    }
    // bool a = begin_timeout == (*out_segments.begin()).timeout();   //第一个元素没变
    // bool b = out_segments.begin() != oldest_seg;  //不是第一个元素
    bool c = (*oldest_seg).timeout() <= ms_since_last_tick; 
    
    if (c) {
        (*oldest_seg).restrans_time()++;
        _consecutive_retransmissions++;
        (*oldest_seg).timeout() = _initial_retransmission_timeout << (*oldest_seg).restrans_time();
        _segments_out.push((*oldest_seg).segment());
    } else {
            (*oldest_seg).timeout() -= ms_since_last_tick;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { 
    return _consecutive_retransmissions; 
}

void TCPSender::send_empty_segment() {
    TCPSegment seg = make_segment(next_seqno(),false,false,"");
    
    _segments_out.push(seg);
}

TCPSegment TCPSender::make_segment(const WrappingInt32 seqno,const bool syn,const bool fin,const string &payload) {
    // vector<uint8_t> header_str(20, 0);
    TCPSegment seg{};
    // uint32_t raw_seqn = seqno.raw_value();
    // for (size_t i = 0;i < 4;i++) {
    //     header_str[4+i] = raw_seqn >> 8*i;
    // }
    // NetParser p{string(header_str.begin(), header_str.end())};
    TCPHeader &header = seg.header();
    header.seqno = seqno;
    header.syn = syn;
    header.fin = fin;
    // string &&s = move(const_cast<string &>(payload));
    // if (!payload.empty()) {
    if (!payload.empty()) {
        seg.payload() = Buffer(string(move(payload)));
    }
    return seg; 
}
