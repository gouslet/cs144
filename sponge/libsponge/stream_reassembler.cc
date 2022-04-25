#include "stream_reassembler.hh"

#include <iostream>
// // Dummy implementation of a stream reassembler.

// // For Lab 1, please replace with a real implementation that passes the
// // automated checks run by `make check_lab1`.

// // You will need to add private members to the class declaration in `stream_reassembler.hh`

// // template <typename... Targs>
// // void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // 如果data的长度超过容量，截断超出的部分。
    size_t sr_written_len = data.length();
    if (sr_written_len + _map.size() > _capacity) {
        sr_written_len = _capacity - _map.size();
    }

    /**
     * Lab2 rev_window test
     * capcity = 2
     * index = 23454 data = "bc" bc先到，只放入1个长度即放入'b', 不然后续的a会放不进去,此时unasm = 23453
     * index = 23453 data = "a" 还没到
     * sr_written_len = 23453 + 2 - 23454 = 1
     */
    if (index + sr_written_len > _first_unasm + _capacity) {
        sr_written_len = _first_unasm + _capacity - index;
    }
    if (eof) {
        _eof = true;
        _end_idx = index + sr_written_len;
    }
    size_t data_start = 0;  // 从data的第data_start位开始读
    // 丢弃已经有序的部分, 从无序的部分开始, 以免后续处理中影响我们的_map.size()
    if (index < _first_unasm) {
        data_start = _first_unasm - index;
    }
    // check cap
    for (size_t i = data_start; (i < data_start + sr_written_len) && (i < data.length()); i++) {
        _map[i + index] = data[i];
    }

    // 尽可能扩大有序部分的长度
    while (_map.count(_first_unasm) == 1) {
        _first_unasm++;
    }

    // 把有序部分尽可能多的塞进_output
    string str;
    size_t bs_written_len = _first_unasm - _first_unread;
    if (bs_written_len > _output.remaining_capacity()) {
        bs_written_len = _output.remaining_capacity();
    }
    while (bs_written_len > 0) {
        // 逐字符拼接进字符串str
        str.append(1, _map[_first_unread]);
        _map.erase(_first_unread);
        _first_unread++;
        bs_written_len--;
    }
    _output.write(str);
    if (_eof && _first_unread == _end_idx) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const {
    // 有一部分虽然有序但是_output里已经放不下了，所以暂时还在map里，这部分要减去。
    size_t asm_bytes_in_map = (_first_unasm - _first_unread);
    return _map.size() - asm_bytes_in_map;
}
bool StreamReassembler::empty() const { return _map.size() == 0 && _output.buffer_empty(); }