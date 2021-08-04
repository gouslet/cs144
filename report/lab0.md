# 调试环境构建

1. cmake时出现未定义的符号错误
   ```
   cmake: symbol lookup error: cmake: undefined symbol: archive_write_add_filter_zstd
   ```
   解决：
   执行 `dnf update`
2. make时出现
    ```shell
        /c/Users/Elon/Desktop/cs144/sponge/libsponge/util/buffer.cc:7:15: 错误：‘out_of_range’在此作用域中尚未声明
            7 |         throw out_of_range("Buffer::remove_prefix");
    ```
    解决：在buffer.cc文件中添加`#include <stdexcept>`

# 任务：可靠的内存字节流
- 从input一侧写入字节，并可按相同的顺序从output一侧读取
- 字节流是有限的：writer可以终结输入，接着就不能写入了。当reader读到流的末尾时，会得到EOF，就无法读取更多字节了。
- 任何时候字节流将可以通过拥塞控制限制内存消耗
- 该对象初始化一个特殊的容量：在任何点上可以在内存中存储的最大字节数
- 任何时候字节流可以限制writer可以写的数量，以确保流未超出其存储容量
- reader从流中读取字节，同时writer可以写入，字节流仅仅在单线程中使用，不必担心并发读写、锁和竞争
- 字节流是有限的，但是在writer结束输入并终止流之前可以是无限长，必须能够处理比容量大得多的流 
- 容量限制可以保存在内存中的字节数，但是不限制流的长度
- 一个容量为1字节的对象可以持有以TB为单位的字节流，只要writer持续每次写1个字节，reader在writer写入下一个字节之前读取每个字节

## 第一次测试
- 9项用例通过了4项，未通过的用例为
 	 ```
     23 - t_byte_stream_construction (Failed)
	 24 - t_byte_stream_one_write (Failed)
	 25 - t_byte_stream_two_writes (Failed)
	 26 - t_byte_stream_capacity (Failed)
	 27 - t_byte_stream_many_writes (Failed)
     ```
- 其中未通过的方法有
    - eof
    - buffer_empty
    - input_ended
- 错误有
  - Exception: The test "overwrite" caused your implementation to throw an exception!
  - Expected "cat" at the front of the stream, but found "tac"
  - Exception:basic_string::_M_construct null not valid
    
    详情见[test_lab0-1](../sponge/build/test-results/test_lab0-1)
- 针对buffer_empty方法，在`byte_stream.hh`文件中可见其声明和说明为
    ```c
    //! \returns `true` if the buffer is empty
    bool buffer_empty() const;
    ```
  将其实现为
    ```c++
    return cap > 0 && size == 0;
    ```

## 第二次测试
- 9项用例通过了5项，未通过的用例为
     
   ```
    23 - t_byte_stream_construction (Failed)
    24 - t_byte_stream_one_write (Failed)
    25 - t_byte_stream_two_writes (Failed)
    26 - t_byte_stream_capacity (Failed)
  ```
- 其中未通过的方法有
  - eof
- 错误有
  - Exception: The test "overwrite" caused your implementation to throw an exception!
  - Expected "cat" at the front of the stream, but found "tac"
  - Exception:basic_string::_M_construct null not valid
    
    详情见[test_lab0-2](../sponge/build/test-results/test_lab0-2)
- 针对Expected "cat" at the front of the stream, but found "tac"，根据
  List of steps that executed successfully:
	Initialized with (capacity=15)
	     Action: write "cat" to the stream
  可知write写入string和peek_output显示front端string的顺序不一致

  将size_t ByteStream::write(const string &data)函数实现为
  ```c++
  {
    size_t i = 0;
    for (auto it = data.rbegin();it != data.rend();++it) {
        if (!_error && !i_ended && size < cap) {
            bas.push_back(*it);
            i++;
            size++;
        } else {
            set_error();
            break;
        }
    }
    b_written += i;
    return i;
  }
  ```
  将string ByteStream::peek_output(const size_t len) const 函数实现为
  ```c++
  {
    string s;
    if (!_error && size >= len) {
        for (size_t i = 0;i < len;i++) {
            s.push_back(bas.at(size - i - 1));
        }
        return s;
    }
    return nullptr;
  }
  ```

## 第三次测试
- 9项用例通过了5项，未通过的用例为
     
   ```
   23 - t_byte_stream_construction (Failed)
   24 - t_byte_stream_one_write (Failed)
   25 - t_byte_stream_two_writes (Failed)
   26 - t_byte_stream_capacity (Failed)
  ```
- 其中未通过的方法有
  - eof
- 错误有
  - Exception: The test "construction" failed
  - Exception: The test "overwrite" caused your implementation to throw an exception!
	- Expected "cattac" at the front of the stream, but found "taccat"
  - Exception:basic_string::_M_construct null not valid
    
  详情见[test_lab0-3](../sponge/build/test-results/test_lab0-3)
- 针对Expected "cattac" at the front of the stream, but found "taccat"
  
  根据
  List of steps that executed successfully:
	Initialized with (capacity=15)
	     Action: write "cat" to the stream
	Expectation: "cat" at the front of the stream
	     Action: write "tac" to the stream
  可知write写入string和peek_output显示front端string的顺序不一致

  将size_t ByteStream::write(const string &data)函数实现为
  ```c++
  {
    size_t i = 0;
    for (auto it = data.begin();it != data.end();++it) {
        if (!_error && !i_ended && size < cap) {
            bas.push_back(*it);
            i++;
            size++;
        } else {
            set_error();
            break;
        }
    }
    b_written += i;
    return i;
  }
  ```
  将string ByteStream::peek_output(const size_t len) const 函数实现为
  ```c++
  {
    string s;
    if (!_error && size >= len) {
        for (size_t i = 0;i < len;i++) {
            s.push_back(bas.at(i));
        }
        return s;
    }
    return nullptr;
  }
  ```

## 第四次测试
- 9项用例通过了5项，未通过的用例为
     
   ```
   23 - t_byte_stream_construction (Failed)
   24 - t_byte_stream_one_write (Failed)
   25 - t_byte_stream_two_writes (Failed)
   26 - t_byte_stream_capacity (Failed)
  ```
- 其中未通过的方法有
  - eof
- 错误有
  - The ByteStream should have had eof equal to 0 but instead it was 1
  - Exception: The test "overwrite" caused your implementation to throw an exception!
	- The ByteStream should have had bytes_read equal to 2 but instead it was 0
  - Exception:basic_string::_M_construct null not valid
    
  详情见[test_lab0-4](../sponge/build/test-results/test_lab0-4)
- 针对
  The ByteStream should have had bytes_read equal to 2 but instead it was 0
  
  根据
  List of steps that executed successfully:
	Initialized with (capacity=15)
	     Action: write "cat" to the stream
	     Action: pop 2

  将size_t ByteStream::write(const string &data)函数实现为
  ```c++
  {
    size_t i = 0;
    for (auto it = data.begin();it != data.end();++it) {
        if (!_error && !i_ended && size < cap) {
            bas.push_back(*it);
            i++;
            size++;
        } else {
            set_error();
            break;
        }
    }
    b_written += i;
    return i;
  }
  ```
  将void ByteStream::pop_output(const size_t len) 函数修正为
  ```c++
  {
    if (_error) {
        return;
    }
    if (len > size) {
        set_error();
        return;
    }
    for (size_t i = 0; i < len; i++) {
        bas.pop_front();
    }
    b_read += len;
    size -= len;
  }
  ```
## 第五次测试
- 9项用例通过了6项，未通过的用例为
     
   ```
   23 - t_byte_stream_construction (Failed)
   24 - t_byte_stream_one_write (Failed)
   26 - t_byte_stream_capacity (Failed)
  ```
- 其中未通过的方法有
  - eof
- 错误有
  - The ByteStream should have had eof equal to 0 but instead it was 1
	- basic_string::_M_construct null not valid
  - Exception: The test "overwrite" caused your implementation to throw an exception!
  - Exception:basic_string::_M_construct null not valid
    
  详情见[test_lab0-5](../sponge/build/test-results/test_lab0-5)
- 针对
  - The ByteStream should have had eof equal to 0 but instead it was 1
  
  根据
  List of steps that executed successfully:
    Initialized with (capacity=15)
    Expectation: input_ended: 0
    Expectation: buffer_empty: 1

  将bool ByteStream::eof() const函数修正为
  ```c++
  { return size <= 0 && i_ended; }
  ```

## 第六次测试
- 9项用例通过了8项，未通过的用例为
     
   ```
	 26 - t_byte_stream_capacity (Failed)
  ```
- 其中错误有
  - Expectation: "ca" at the front of the stream
	- basic_string::_M_construct null not valid
  - Exception: The test "overwrite" caused your implementation to throw an exception!
    
  详情见[test_lab0-6](../sponge/build/test-results/test_lab0-6)
- 针对
	- basic_string::_M_construct null not valid
  
  根据
  List of steps that executed successfully:
    Initialized with (capacity=2)
        Action: write "cat" to the stream
    Expectation: input_ended: 0
    Expectation: buffer_empty: 0
    Expectation: eof: 0
    Expectation: bytes_read: 0
    Expectation: bytes_written: 2
    Expectation: remaining_capacity: 0
    Expectation: buffer_size: 2

  将std::string ByteStream::read(const size_t len)函数修正为 
  ```c++
  {
    string s;
    for (size_t i = 0; i < len; i++) {
        if (!_error && !o_ended && size > 0) {
            s.push_back(move(bas.front()));
            bas.pop_front();
            b_read++;
            size--;
        } else {
            break;
        }
    }
    return s;
  }
  ```

## 第七次测试
- 9项用例通过了8项，未通过的用例为
     
   ```
	 26 - t_byte_stream_capacity (Failed)
  ```
- 其中错误有
  - Failure message:
      The ByteStream should have had bytes_written equal to 2 but instead it was 0
    
  详情见[test_lab0-7](../sponge/build/test-results/test_lab0-7)
- 针对
  - Failure message:
	    The ByteStream should have had bytes_written equal to 2 but instead it was 0
  
  根据
  List of steps that executed successfully:
    Initialized with (capacity=2)
        Action: write "cat" to the stream
        Action: pop 2

  将size_t ByteStream::write(const string &data)函数修正为
  ```c
  {
    size_t i = 0;
    for (auto it = data.begin(); it != data.end();++it) {
        if (!_error && !i_ended && size < cap) {
            bas.push_back(*it);
            i++;
            size++;
        } else {
            break;
        }
    }
    b_written += i;
    return i;
  }
  ```

## 第八次测试
- 本次测试全部9个用例均通过
  详情见[test_lab0-8](../sponge/build/test-results/test_lab0-8)
- 详情见