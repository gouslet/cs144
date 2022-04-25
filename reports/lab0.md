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

# 调试环境
wsl2 ubuntu 18.04

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

# 疑难杂症
## 2021/06/25 
### 21:25
**问题**：进入sponge文件夹下，新建并进入build目录，cmake ..之后，make操作时出现如下错误

    root@kali:~/sponge/build# make
    Scanning dependencies of target sponge
    [  3%] Building CXX object libsponge/CMakeFiles/sponge.dir/byte_stream.cc.o
    [  6%] Building CXX object libsponge/CMakeFiles/sponge.dir/util/address.cc.o
    [ 10%] Building CXX object libsponge/CMakeFiles/sponge.dir/util/buffer.cc.o
    /root/sponge/libsponge/util/buffer.cc: In member function ‘void Buffer::remove_prefix(size_t)’:
    /root/sponge/libsponge/util/buffer.cc:7:15: error: ‘out_of_range’ was not declared in this scope
        7 |         throw out_of_range("Buffer::remove_prefix");
        |               ^~~~~~~~~~~~
    /root/sponge/libsponge/util/buffer.cc: In member function ‘BufferList::operator Buffer() const’:
    /root/sponge/libsponge/util/buffer.cc:28:19: error: ‘runtime_error’ was not declared in this scope
    28 |             throw runtime_error(
        |                   ^~~~~~~~~~~~~
    /root/sponge/libsponge/util/buffer.cc: In member function ‘void BufferList::remove_prefix(size_t)’:
    /root/sponge/libsponge/util/buffer.cc:54:24: error: ‘out_of_range’ is not a member of ‘std’
    54 |             throw std::out_of_range("BufferList::remove_prefix");
        |                        ^~~~~~~~~~~~
    /root/sponge/libsponge/util/buffer.cc: In member function ‘void BufferViewList::remove_prefix(size_t)’:
    /root/sponge/libsponge/util/buffer.cc:76:24: error: ‘out_of_range’ is not a member of ‘std’
    76 |             throw std::out_of_range("BufferListView::remove_prefix");
        |                        ^~~~~~~~~~~~
    /root/sponge/libsponge/util/buffer.cc: In member function ‘BufferList::operator Buffer() const’:
    /root/sponge/libsponge/util/buffer.cc:27:9: error: control reaches end of non-void function [-Werror=return-type]
    27 |         default: {
        |         ^~~~~~~
    cc1plus: all warnings being treated as errors
    make[2]: *** [libsponge/CMakeFiles/sponge.dir/build.make:108: libsponge/CMakeFiles/sponge.dir/util/buffer.cc.o] Error 1
    make[1]: *** [CMakeFiles/Makefile2:470: libsponge/CMakeFiles/sponge.dir/all] Error 2
    make: *** [Makefile:114: all] Error 2
**解决**：百度搜索关键词 cs144 libsponge out_of_range，在[《关于部分编译器out_of_range报错的解决》](https://blog.51cto.com/beyond316/1229777)中找到原因和解决方案，如下图所示
![alt](img/2021-6-25%2021-53-42.png)
根据错误信息/root/sponge/libsponge/util/buffer.cc:7:15: error: ‘out_of_range’ was not declared in this scope可知是sponge/libsponge/util/buffer.cc源代码中使用了**stdexcept**中声明的**out_of_range**，加入```#include<stdexcept>```即可顺利编译

## 2021/06/26 
### 19:30
**问题**：执行 ```make check webget ```，得到如下错误信息

    root@kali:~/sponge/build# make check webget 
    [100%] Testing libsponge...
    make[3]: ../tun.sh: Command not found
    make[3]: *** [CMakeFiles/check.dir/build.make:77: CMakeFiles/check] Error 127
    make[2]: *** [CMakeFiles/Makefile2:200: CMakeFiles/check.dir/all] Error 2
    make[1]: *** [CMakeFiles/Makefile2:207: CMakeFiles/check.dir/rule] Error 2
    make: *** [Makefile:148: check] Error 2

**解决**：根据```make[3]: ../tun.sh: Command not found```一句可知在sponge目录下缺少tun.sh文件，在github上搜索到https://github.com/carlclone/TCP-Labs中含有 tun.sh和txrx.sh两个文件，下载添加到本地sponge目录下后顺利通过测试

## 2022/04/08
### 23:04
**问题**：make check_webget结果如下
```
Scanning dependencies of target check_webget
[100%] Testing webget...
Test project /mnt/i/DIY/CS144 计算机网络/sponge/build
    Start 31: t_webget
1/1 Test #31: t_webget .........................***Not Run   0.00 sec

0% tests passed, 1 tests failed out of 1

Total Test time (real) =   0.09 sec

The following tests FAILED:
	 31 - t_webget (BAD_COMMAND)
Errors while running CTest
CMakeFiles/check_webget.dir/build.make:57: recipe for target 'CMakeFiles/check_webget' failed
make[3]: *** [CMakeFiles/check_webget] Error 8
CMakeFiles/Makefile2:195: recipe for target 'CMakeFiles/check_webget.dir/all' failed
make[2]: *** [CMakeFiles/check_webget.dir/all] Error 2
CMakeFiles/Makefile2:202: recipe for target 'CMakeFiles/check_webget.dir/rule' failed
make[1]: *** [CMakeFiles/check_webget.dir/rule] Error 2
Makefile:181: recipe for target 'check_webget' failed
make: *** [check_webget] Error 2

```
### 原因：
- 以t_webget为关键字搜索文件夹，发现在/sponge/etc/tests.cmake文件中存在
    ```
    add_test(NAME t_webget               COMMAND "${PROJECT_SOURCE_DIR}/tests/webget_t.sh")
    ```
    可知，该测试执行了`${PROJECT_SOURCE_DIR}/tests/webget_t.sh`文件
- 单独执行该文件，得到如下结果
    ```
    bash: /mnt/i/DIY/CS144/sponge/tests/webget_t.sh: /bin/bash^M: bad interpreter: No such file or directory
    ```
- 以`/bin/bash^M 坏的解释器`为关键字搜索，在[/bin/bash^M: 坏的解释器: 没有那个文件或目录 的解决方法](https://blog.csdn.net/qq_56870570/article/details/120182874)中找到原因
    - windows系统下换行符为 \r\n，linux下换行符为 \n，所以导致在windows下编写的文件会比linux下多回车符号 \r
### 解决：
使用vim编辑该文件，在底行模式下输入 set ff，回车可以看到文件格式为dos，接着在底行模式下输入 set ff=unix，将文件格式修改为unix，即将文件中所有的回车换行符由\r\n替换为\n