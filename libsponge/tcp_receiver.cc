#include "tcp_receiver.hh"
// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    bool ret = false;
    static size_t abs_seqno = 0;
    size_t length;
    if(seg.header().syn) {
        //如果已经收到过SYN，不再继续接收SYN
        if(_syn_flag) {
            return false;
        }
        _syn_flag = true;
        ret = true;
        //获取初始序列号
        _isn = seg.header().seqno.raw_value();
        abs_seqno = 1;
        _base = 1;
        length = seg.length_in_sequence_space() - 1;
        //报文只包含1个SYN
        if(length == 0) {
            return true;
        }
    //之前收到过SYN，拒绝任何报文  
    } else if(!_syn_flag) {         
        return false;
    //不是SYN报文，计算绝对seq
    } else {  
        abs_seqno = unwrap(WrappingInt32(seg.header().seqno.raw_value()), WrappingInt32(_isn), abs_seqno);
        length = seg.length_in_sequence_space();
    }

    if(seg.header().fin) {
        //已经收到过FIN，拒绝接收其他FIN
        if(_fin_flag) {
            return false;
        }
        _fin_flag = true;
        ret = true;
    // 不是FIN，也不是SYN，检查边界
    } else if(seg.length_in_sequence_space() == 0 && abs_seqno == _base) {
        return true;
    } else if(abs_seqno >= _base + window_size() || abs_seqno + length <= _base) {
        if(!ret) {
            return false;
        }
    }

    _reassembler.push_substring(seg.payload().copy(), abs_seqno - 1, seg.header().fin);
    _base = _reassembler.head_index() + 1;
    if(_reassembler.input_ended()) {
        _base++;
    }
    return true;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(_base > 0) {
        return WrappingInt32(wrap(_base, WrappingInt32(_isn)));
    } else {
        return std::nullopt;
    }
}

size_t TCPReceiver::window_size() const {
    return _capacity - _reassembler.stream_out().buffer_size();
}