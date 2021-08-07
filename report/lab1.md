# 任务
实现一个负责重组流数据的数据结构——treamReassembler. 
  - 它接收一系列字节形成的子串和第一个字节在更大的流中的序号
  - 每个字节都有其自己的序号，从零往上增加
  - StreamReassembler拥有一个用于输出的ByteStream，每当reassembler了解到流的下一个字节，就将其写入ByteStream.
  - 属主可以随时访问和读取ByteStream

## 第一次测试
- 原始实现为
  ```c++
  // stream_reassembler.hh
  #include <cstdint>
  #include <string>
  #include <map>

  using std::map;
  using std::pair;
  using std::string;

  //! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
  //! possibly overlapping) into an in-order byte stream.
  class StreamReassembler {
    private:
      // Your code here -- add private members as necessary.
    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes
    size_t next_index;  //!< The next index of a byte which could get into the in-order ByteStream
    map<int,pair<string,bool>> buffer;
  };
  ```
  ```c++
  // stream_reassembler.cc
  StreamReassembler::StreamReassembler(const size_t capacity) 
    : _output(capacity), _capacity(capacity), next_index(), buffer() {}

  //! \details This function accepts a substring (aka a segment) of bytes,
  //! possibly out-of-order, from the logical stream, and assembles any newly
  //! contiguous substrings and writes them into the output stream in order.
  void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (buffer.size() < _output.remaining_capacity()) {
        if (index == next_index) {
            if (!_output.error() && !_output.input_ended()) {
                _output.write(data);
                buffer.erase(index);
                if (eof) {
                    return;
                }
            }
            size_t i = index + 1; 
            for (auto it = buffer.find(i);it != buffer.find(++i);it++,i++) {
                _output.write(((*it).second).first);
                buffer.erase(index);
                if (((*it).second).second) {
                    return;
                } 
            }
            next_index = i;
        } else {
            buffer.emplace(index,make_pair(data,eof));
        }
    }
  }

  size_t StreamReassembler::unassembled_bytes() const { return buffer.size(); }

  bool StreamReassembler::empty() const { return buffer.empty(); }
  ```

- 16项用例通过了8项，未通过的用例为
  ```
  15 - t_strm_reassem_single (Failed)
  16 - t_strm_reassem_seq (Failed)
  17 - t_strm_reassem_dup (Failed)
  18 - t_strm_reassem_holes (Failed)
  19 - t_strm_reassem_many (Failed)
  20 - t_strm_reassem_overlapping (Failed)
  21 - t_strm_reassem_win (Failed)
  22 - t_strm_reassem_cap (Failed)
  ```
- 其中错误有
	- The reassembler was expected to be at EOF, but was not
	- The reassembler was expected to have `8` total bytes assembled, but there were `4`
	- The reassembler was expected to have `4` total bytes assembled, but there were `8`
	- The reassembler was expected to be at EOF, but was not
	- The reassembler was expected to have `2` total bytes assembled, but there were `3`
	- The reassembler was expected to have `4` total bytes assembled, but there were `2`
  详情见[test-results/test-lab1-1](../sponge/build/test-results/test-lab1-1)
- 针对
  The reassembler was expected to be at EOF, but was not
  
  根据
  List of steps that executed successfully:
	Initialized (capacity = 65000)
	  Action:      substring submitted with data "a", index `0`, eof `1`
	  Expectation: net bytes assembled = 1
	  Expectation: stream_out().buffer_size() returned 1, and stream_out().read(1) returned the string "a"
- 在void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) 函数中增加_output.end_input();语句

## 第二次测试
- 16项用例通过了8项，未通过的用例为
  ```
  15 - t_strm_reassem_single (Failed)
  16 - t_strm_reassem_seq (Failed)
  17 - t_strm_reassem_dup (Failed)
  18 - t_strm_reassem_holes (Failed)
  19 - t_strm_reassem_many (Failed)
  20 - t_strm_reassem_overlapping (Failed)
  21 - t_strm_reassem_win (Failed)
  22 - t_strm_reassem_cap (Failed)
  ```
- 其中错误有
	- The reassembler was expected to have `8` total bytes assembled, but there were `3`
	- The reassembler was expected to have `8` total bytes assembled, but there were `4`
	- The reassembler was expected to have `8` total bytes assembled, but there were `4`
	- The reassembler was expected to have `2` total bytes assembled, but there were `3`
  - Exception: test 1 - number of bytes RX is incorrect
  - Exception: test 2 - number of RX bytes is incorrect
  - Exception: The reassembler was expected to have `2` total bytes assembled, but there were `1`

  详情见[test-results/test-lab1-2](../sponge/build/test-results/test-lab1-2)
- 针对
	The reassembler was expected to have `8` total bytes assembled, but there were `3`
  
  根据
  List of steps that executed successfully:
	Initialized (capacity = 8)
  	Action:      substring submitted with data "abc", index `0`, eof `0`
	  Expectation: net bytes assembled = 3
	  Action:      substring submitted with data "bcdefgh", index `1`, eof `1`

- 将void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) 函数修正为    
```c++
{
  if (buffer.size() < _output.remaining_capacity()) {
        if (index == next_index) {
            if (!_output.error() && !_output.input_ended()) {
                _output.write(data);
                buffer.erase(index);
                if (eof) {
                    _output.end_input();
                    return;
                }
            }
            size_t i = index + 1; 
            for (auto it = buffer.find(i);it != buffer.find(++i);it++,i++) {
                _output.write(((*it).second).first);
                buffer.erase(index);
                if (((*it).second).second) {
                    _output.end_input();
                    return;
                } 
            }
            next_index = --i;
        } else {
            buffer.emplace(index,make_pair(data,eof));
        }
    }
}
```

## 第三次测试
- 16项用例通过了8项，未通过的用例为
  ```
  15 - t_strm_reassem_single (Failed)
  16 - t_strm_reassem_seq (Failed)
  17 - t_strm_reassem_dup (Failed)
  18 - t_strm_reassem_holes (Failed)
  19 - t_strm_reassem_many (Failed)
  20 - t_strm_reassem_overlapping (Failed)
  21 - t_strm_reassem_win (Failed)
  22 - t_strm_reassem_cap (Failed)
  ```
- 其中错误有
	- The reassembler was expected to have  bytes "abcdefgh", but there were "abcbcdef"
	- The reassembler was expected to have `8` total bytes assembled, but there were `4`
	- The reassembler was expected to have `8` total bytes assembled, but there were `4`
	- The reassembler was expected to have `2` total bytes assembled, but there were `3`
	- The reassembler was expected to have `2` total bytes assembled, but there were `1`
  - Exception: test 2 - number of RX bytes is incorrect
	- The reassembler was expected to have `4` total bytes assembled, but there were `2`

  详情见[test-results/test-lab1-3](../sponge/build/test-results/test-lab1-3)
- 针对
  The reassembler was expected to have  bytes "abcdefgh", but there were "abcbcdef"
  
  根据
  List of steps that executed successfully:
	Initialized (capacity = 8)
  	Action:      substring submitted with data "abc", index `0`, eof `0`
	  Expectation: net bytes assembled = 3
	  Action:      substring submitted with data "bcdefgh", index `1`, eof `1`
	  Expectation: net bytes assembled = 8

- 将void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) 函数修正为    
```c++
{
  if (buffer.size() < _output.remaining_capacity()) {
        if (index == next_index) {
            if (!_output.error() && !_output.input_ended()) {
                _output.write(data);
                buffer.erase(index);
                if (eof) {
                    _output.end_input();
                    return;
                }
            }
            size_t i = index + 1; 
            for (auto it = buffer.find(i);it != buffer.find(++i);it++,i++) {
                _output.write(((*it).second).first);
                buffer.erase(index);
                if (((*it).second).second) {
                    _output.end_input();
                    return;
                } 
            }
            next_index = --i;
        } else {
            buffer.emplace(index,make_pair(data,eof));
        }
    }
}
```