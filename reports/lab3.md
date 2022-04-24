# TCP Sender
- TCP协议的发送方负责读取ByteStream(由发送方应用程序创建和写入)，然后转换为TCP帧序列
- 在远端，TCP接收器将这些帧转换回原先的字节流，然后发送acknowledgments和window信息回发送方
- TCP发送方和接收方各自负责TCP帧的一部分
- TCP发送方填写所有和TCP接收方相关的TCP帧字段，包括sequence number,SYN flag,payload,和FIN flag
- 但是,TCP发送方仅仅读取帧中被接收方写入的字段，ackno和window size

# TCPSender的职责
- 跟踪接收方的window (处理接收到的ackno和window size)
- 通过读取ByteStream和创建新的TCP帧并发送来填充window
    - 发送方应该保持发送帧，除非window满了或者ByteStream为空
- 持续跟踪已发送但未收到回应的帧，称为“outstanding”帧 
- 如果一段时间未收到回应，则重发这些outstanding帧 

# TCP Sender如何判断帧丢失？
- TCPSender将会持续跟踪其发出的帧，直到其接收到对应sequence number的回应帧
- TCP Sender周期性调用tick方法，代表时间的流逝
- TCPSender负责查找出发出时间最久的帧，并判断其是否超过回应等待时间。如果是，则它需要重发
- outstanding for too long意味着
    1. 每一毫秒将会调用一次TCPSender的tick方法，参数为从上次调用这个方法起已经过去的时间（单位为毫秒）。
        - 这用来记录TCPSender存活的时间（单位为毫秒） 
        - 请勿调用任何操作系统或CPU相关的“time”或“clock”函数，这有助于保持确定性和可测试性
    2. 创建TCPSender后，赋予其一个代表重传计时的初始值（RTO）
        - RTO代表重传一个已发出的TCP帧之前需要等待的时间- RTO的值会随着时间变化，但是初始值保持不变
        - RTO的初始值保存在_initial_retransmission_timeout成员变量中
    3. 需要实现一个重传计时器: 一个可以在某个确定时刻启动的闹钟，其会在经过了RTO时间之后失效
        - 时间以调用tick方法的方式来记录，而不是获取真实的时间
    4. 每次发送一个包含非零长度数据的帧时(无论是初次发送，还是重传), 如果计时器没有运行，则开始运行，这样其将会在RTO毫秒之后失效
        - 失效是指时间将会以毫秒为单位耗尽
    5. 当所有发出的数据都收到了回应，停止重传计时器
    6. 如果调用了tick方法，而且重传计时器也失效了
        - 重传最早发出的没有收到回应的帧（具有最小的sequence number）
            - 可能需要存储已经发出的帧
        - 如果window size非0
            - 记录连续重发的次数，TCPConnection将会使用这个信息来决定是否要终止该连接(连续多次重发意味着无望)
            - RTO翻倍，称为“指数回退”—它减少了在不良的网络状况下重传的次数，以避免堵塞网络
        - 重置重传计时器，并启动，这样它将会在RTO毫秒之后失效
    7. 当接收方传给发送方一个比之前的ackno大的ackno，表示接收到了数据
    - 将RTO设置为“initial value”
    - 如果sender有任何已发出的数据，则重启重传计时器
    - 连续重传计数清零
 
# 接口
TCP Sender有四个方法，每个方法都以发送一个TCP帧结束：
- void fill window()
    - TCPSender被要求填充window: 
        - 读取ByteStream，只要还有新的字节可以读取、window还有空间，就以TCP帧的形式发送尽可能多的字节
        - 单个TCPSegment大小不超过TCPConfig::MAX PAYLOAD SIZE (1452 bytes)
        - 可以使用TCPSegment::length_in_sequence_space()方法来计算一个帧占用sequence_number的总数
        - SYN和FIN标志也占各用一个sequence number,这意味着它们也占用window空间

- void ack_received(const WrappingInt32 ackno, const uint16 t window size)
    - 接收到来自receiver的帧传递的窗口的新的left (= ackno)和right (= ackno + window size)
    - TCPSender应该查找已发送帧的集合，移除所有sequence number小于ackno的帧
    - TCPSender应该再次填充window，如果window有了新的空间
    
- void tick( const size t_ms_since_last_tick)
    - Time has passed — a certain number of milliseconds since the last time this method was called. 
    - The sender may need to retransmit an outstanding segment.

- void send_empty_segment()
    -  TCPSender应该生成并发送一个占用sequence长度为0且sequence number正确的TCPSegment

# 实现要点
## 如何发送一个帧？
压入_segments_out栈中
## 如何在发送一个帧的同时记录已发送的帧？
将其大的备份保存在一个数据结构中。因为帧的负荷是以引用计数式的只读字符串保存的，所以不会浪费空间
## 在TCPSender收到ACK帧之前，window_size是多少？
1
## 如果一个ACK帧仅仅对一部分已发送帧作出了回应，是否需要剪掉这部分字节?
可以但没必要。这里仅仅在一个已发送帧的全部字节都收到回应后再删除
## 如果先发送了三个独立的帧，分别包含“a,” “b”,“c”三个字节，但是没有收到回应，之后重发时是否可以以一个包含“abc”三个字符的帧发送？
可以，但此处不要这样做
## 需要记录空的已发送帧吗？
不需要
## 如何构造一个TCP Segment