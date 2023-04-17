#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {
    _buffer.resize(capacity);
}

long StreamReassembler::merge_block(block_node &elm1, const block_node &elm2) {
    block_node x, y;
    if(elm1.begin >elm2.begin) {
        x = elm2;
        y = elm1;
    } else {
        x = elm1;
        y = elm2;
    }
    //没有交集 不用merge 返回-1
    if(x.begin + x.length < y.begin) {
        return -1;
    //x 覆盖了 y, 合并之后数据为x，合并长度也就是交集长度，为y的数据长度
    } else if(x.begin + x.length >= y.begin + y.length) {
        elm1 = x;
        return y.length;
    } else {
        elm1.begin = x.begin;
        elm1.data = x.data + y.data.substr(x.begin + x.length - y.begin);
        elm1.length = elm1.data.length();
        return x.begin + x.length - y.begin;
    }
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    //缓存满了
    if(index >= _head_index + _capacity) {
        return;
    }

    block_node elm;
    if(index + data.length() <= _head_index) {
        goto JUDGE_EOF;
    //子串前缀重叠
    } else if(index < _head_index) {
        size_t offset = _head_index - index;
        elm.begin = index + offset;
        elm.data.assign(data.begin() + offset, data.end());
        elm.length = elm.data.length();
    } else {
        elm.begin = index;
        elm.data = data;
        elm.length = data.length();
    }

    _unassembler_byte = elm.length;

    //子串重叠，合并子串
    do {
        long merged_bytes = 0;
        auto iter = _blocks.lower_bound(elm);
        while(iter != _blocks.end() && (merged_bytes = merge_block(elm, *iter)) >= 0) {
            _unassembler_byte -= merged_bytes;
            _blocks.erase(iter);
            iter = _blocks.lower_bound(elm);
        }

        if(iter == _blocks.begin()) {
            break;
        }

        iter--;
        while((merged_bytes = merge_block(elm, *iter)) >= 0) {
            _unassembler_byte -= merged_bytes;
            _blocks.erase(iter);
            iter = _blocks.lower_bound(elm);
            if(iter == _blocks.begin()) {
                break;
            }
            iter--;
        }
    }while(false);
    _blocks.insert(elm);

    //写入字节流
    if(!_blocks.empty() && _blocks.begin()->begin == _head_index) {
        const block_node head_node = *_blocks.begin();
        size_t write_bytes = _output.write(head_node.data);
        _head_index += write_bytes;
        _unassembler_byte -= write_bytes;
        _blocks.erase(_blocks.begin());
    }

JUDGE_EOF:
    if(eof) {
        _eof_flag = true;
    }
    if(_eof_flag && empty()) {
        _output.end_input();
    }
}



size_t StreamReassembler::unassembled_bytes() const {
    return _unassembler_byte;
 }

bool StreamReassembler::empty() const {
    return _unassembler_byte == 0;
}
