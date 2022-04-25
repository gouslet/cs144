# 简介
- TCPConnection负责接收和发送帧，确保发送方和接收方都及时接收到各自关心的字段的信息
# 职责
- TCPConnection必须遵守以下规则：
## 接收帧
- 当TCPConnection调用segment_received方法时，其从互联网接收TCPSegments，此时，TCPConnection查看该帧，
    - 如果设置了rst (reset)标志，将读入和写出的流都设置为 error状态，永远终止该连接
    - 否则将帧传递给TCPReceiver以获取其感兴趣的字段：seqno, syn, payload和fin
    - 如果设置了ack标志，通知TCPSender其关心的字段：ackno 和window size.
    - 如果进来的帧占据了sequence numbers，TCPConnection确保至少一个帧会收到回复，以反映ackno和window size的更新
## 发送帧
- 每当TCPSender将一个帧推入其发送队列，都要先设置其字段(seqno, syn , payload, 和fin).
- 在发送帧之前，TCPConnection将会向TCPReceiver请求其负责的字段：ackno和window size. 如果有ackno, 它将会设置ack标志和TCPSegment中的ackno字段.
## 控制时间流逝
- TCPConnection有一个会周期性被操作系统调用的tick方法，当被调用时
    - 通知TCPSender过去了多长时间
    - 如果consecutive_retransmissions的值大于TCPConfig::MAX RETX ATTEMPTS，则中止连接，发送一个重置帧(一个设置了rst标志的空帧)到对等方。
    - 结束连接并清理现场

# 实现要点
## How should I get started?
- 实现“writer”类方法:connect(), write()和end_input_stream() 
- 其中一些方法可能需要操作对外的ByteStream(在TCPSender中)

## How does the TCPConnection actually send a segment?
- 和TCPSender一样————推入segments_out队列中

## How does the TCPConnection learn about the passage of time?
- 和TCPSender一样——tick()方法会被周期性调用

## What does the TCPConnection do if an incoming segment has the rst flag set?
- “reset”标志意味着连接需要立刻终结
- 如果接收到一个带有rst标志的帧，设置对外和对内ByteStream的error标志，随后调用TCPConnection::active()应该返回false

## When should I send a segment with the rst flag set?
- 两种情况下应该完全终结连接
    1. 发送方多次重发失败(重发次数超过TCPConfig::MAX RETX ATTEMPTS)
    2. 当连接活跃（active()方法返回true）时调用了TCPConnection析构函数
- 发送一个带有rst标志的帧和接受一个这样的帧有相同的效果：连接终结，active()返回false,ByteStream设置为error状态

## Wait, but how do I even make a segment that I can set the rst flag on? What’s the sequence number?
- 任何发出的帧都应该有相应的sequence number。

## What’s the purpose of the ack flag? Isn’t there always an ackno?
- 几乎每个TCPSegment都有ackno，也都设置了ack标志，除了最开始发出的帧
- 对于发出的帧，只要TCPReceiver的ackno()方法返回一个非空的std::optional<WrappingInt32>（可以通过has_value()方法测试），就可以设置ackno和ack标志
- 对于收到的帧，仅当设置了ack字段才查看ackno，然后将ackno和window size发给TCPSender

## What window size should I send if the TCPReceiver wants to advertise a window size that’s bigger than will fit in the TCPSegment::header().win field?
- 发送最大的那个值，std::numeric_limits类可能有用

## When is the TCP connection finally “done”? When can active() return false?

# The end of a TCP connection: consensus takes work
- TCPConnection的一个重要功能是决定何时连接完全结束。此时释放其占用的本地端口，并停止向接收到的帧发送确认，active()方法返回false.
- 连接可能以两种方式结束. 
    ## 以一种不干净的方式结束, TCPConnection要么发送，要么接收一个带有rst标志的帧
    - 这种情况下，输入和输出ByteStream应该都处于error状态，active()方法立即返回false
    ## 干净的方式是以非error状态结束（active() = false）
    - 这更复杂，也更优雅，因为这更能确保两个ByteStream都完整地将每个字节可靠地交付到了接收方

- 由于两将军问题，无法保证双方都能以干净的方式结束，但是TCP尽力而为
- 从本地方的视角，以干净的方式结束同远程对等方的连接有四个先决条件
    ## 条件#1 输入流已重组完成，且已结束
    ## 条件#2 输出流被本地应用程序终结，且已全部发送给对等方(包括fin帧)
    ## 条件#3 已收到对等方对输出流的全部确认帧
    ## 条件#4 本地TCPConnection确信远程对等方可以满足条件#3
    - There are two alternative ways this can happen:
        ### Option A: 在两个流都终结后再延缓一会儿
        - 条件#1到#3为真，远程对等方似乎已经收到本地方对整个流的确认 
        - 本地方并不一定知道此事——TCP并不一定可靠交付ack帧（没有ack帧确认帧） 
        - 但是本地方非常确信远程对等方已经收到了确认，因为其没有重传任何帧，本地方也等待了一定时间
        - 确切地说，当条件#1到条件#3都满足，自本地方从远程对等方收到任何帧开始，经过了10倍初始重传时间时，一个连接就完成了
        - 这称为在两个流都结束后延缓一会儿，以确定远程对等方并非在重传本地需要确认的帧 
        - 这意味着，即便TCPSender和TCPReceiver都已经完成了各自的任务，两个流也都终结了，TCPConnection仍要再生存一段时间，继续占用本地端口，并可能对收到的帧发送确认
        ### Option B: 被动关闭 
        - 条件#1到条件#3为真，本地方100%确定远程对等方满足条件#3. 
        - 如果TCP并不对ack帧发送确认，这怎么可能呢？因为远程对等方先终结流

# 为什么这个规则能生效? 
在收到并重组远程对等方的fin后(条件#1)，本地方发送了一个具有比之前发过的所有sequence number还要大的sequence number，(至少，得要发送它自己的fin帧以满足条件#2), 该帧同时携带有对对等方进行确认的fin位。远程对等方确认该帧，以满足条件#3，此时远程对等方已经收到了本地对其发送的fin帧的确认。这保证了远程对等方自己满足条件#3。所有这些加在一起意味着本地方可以满足条件#4，而不用延迟

底线是如果TCPConnection的输入流在TCPConnection发送一个fin帧之前终结了，那么TCPConnection就不需要在流结束时延迟

- 实际上，在流结束后，TCPConnection通过state()方法暴露一个成员变量_linger_after_streams_finish
- 该变量初始值为true，如果输入流在输出流之前结束，则设置为false
- 如果在流结束时linger为false，无论何时，只要条件 #1到#3满足了，连接就完成了，active()就应该返回false
- 否则需要延迟: 连接在自收到最后一个帧之后经过足够时间才能完成(10 × cfg.rt timeout)