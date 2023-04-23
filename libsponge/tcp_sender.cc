#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

#include<optional>

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
    , _stream(capacity)
    , _retransmission_timeout(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const {
    return _bytes_in_flight;
}

void TCPSender::fill_window(bool send_syn) {
    //在发送数据报文之前，需要发送 SYN 报文握手
    if(!_syn_flag) {
        if(send_syn) {
            TCPSegment seg;
            seg.header().syn = true;
            send_segment(seg);
            _syn_flag = true;
        }
        return;
    }
    //在收到 ack 之前，假设 窗口大小为 1
    size_t win = _window_size > 0 ? _window_size : 1;
    //窗口的剩余空间
    size_t remain;
    //窗口没有满 并且 还没有发送 FIN 报文
    while((remain = win - (_next_seqno - _recv_ackno)) != 0 && !_fin_flag) {
        size_t size = min(TCPConfig::MAX_PAYLOAD_SIZE, remain);
        TCPSegment seg;
        string str = _stream.read(size);
        seg.payload() = Buffer(std::move(str));
        //add FIN
        if(seg.length_in_sequence_space() < win && _stream.eof()) {
            seg.header().fin = true;
            _fin_flag = true;
        }
        //数据流为空
        if(seg.length_in_sequence_space() == 0) {
            return;
        }
        send_segment(seg);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    //讲 ackno 转换为 abs_seq
    size_t abs_ackno = unwrap(ackno, _isn, _recv_ackno);
    //超过了 win 范围， 无效的 ack
    if(abs_ackno > _next_seqno) {
        return false;
    }

    //ack 合法，先修改窗口大小
    _window_size = window_size;
    //ack 已经被接收了
    if(abs_ackno <= _recv_ackno) {
        return true;
    }
    _recv_ackno = abs_ackno;

    //采用累计确认，收到ack，表示ack之前的数据都被确认，将ack之前的数据弹出未被确认队列
    while(!_segments_outstanding.empty()) {
        TCPSegment seg = _segments_outstanding.front();
        if(unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space() <= abs_ackno) {
            _bytes_in_flight -= seg.length_in_sequence_space();
            _segments_outstanding.pop();
        } else {
            break;
        }
    }

    fill_window();

    _retransmission_timeout = _initial_retransmission_timeout;
    _consecutive_retransmission = 0;
    //如果还有未被确认的，开启计时器
    if(!_segments_outstanding.empty()) {
        _timer_running = true;
        _timer = 0;
    }
    return true;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer += ms_since_last_tick;
    if(_timer >= _retransmission_timeout && !_segments_outstanding.empty()) {
        _segments_out.push(_segments_outstanding.front());
        _consecutive_retransmission++;
        _retransmission_timeout *= 2;
        _timer_running = true;
        _timer = 0;
    }
    if(_segments_outstanding.empty()) {
        _timer_running = false;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const {
    return _consecutive_retransmission;
}

void TCPSender::send_empty_segment() {
    //空数据报文不需要存到未被确认队列
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}

void TCPSender::send_segment(TCPSegment &seg) {
    seg.header().seqno = wrap(_next_seqno, _isn);
    _next_seqno += seg.length_in_sequence_space();
    _bytes_in_flight += seg.length_in_sequence_space();
    _segments_outstanding.push(seg);
    _segments_out.push(seg);
    if (!_timer_running) {  // start timer
        _timer_running = true;
        _timer = 0;
    }
}