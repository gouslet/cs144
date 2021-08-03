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

# 可靠的内存字节流
- 从input一侧写入字节，并可按相同的顺序从output一侧读取
- 字节流是有限的：writer可以终结输入，接着就不能写入了。当reader读到流的末尾时，会得到EOF，就无法读取更多字节了。
- 任何时候字节流将可以通过拥塞控制限制内存消耗
- 该对象初始化一个特殊的容量：在任何点上可以在内存中存储的最大字节数
- 任何时候字节流可以限制writer可以写的数量，以确保流未超出其存储容量
- reader从流中读取字节，同时writer可以写入，字节流仅仅在单线程中使用，不必担心并发读写、锁和竞争
- 字节流是有限的，但是在writer结束输入并终止流之前可以是无限长，必须能够处理比容量大得多的流 
- 容量限制可以保存在内存中的字节数，但是不限制流的长度
- 一个容量为1字节的对象可以持有以TB为单位的字节流，只要writer持续每次写1个字节，reader在writer写入下一个字节之前读取每个字节