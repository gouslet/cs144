# 任务
- TCPReceiver在传入的TCP帧(通过互联网传输的数据包的有效载荷)和传入的字节流之间进行转换
- TCPReceiver通过received()方法从互联网接收帧，转换为对StreamReassembler的调用，最终写入ByteStream
- TCPReceiver负责告诉发送方两件事
  - 首个unassembled字节的序号，称为"acknowledgment number"或者"ackno"，这是接收方希望从发送方接收的第一个字节
  - 首个unassembled序号和首个unacceptable序号之间的差值，称为"window size"
- ackno和window size共同描述了接收方的窗口：TCP发送方允许发送的序号范围
- 通过这个窗口，接收方可以实现流量控制，在接收方准备好接收之前限制发送方的发送

# TCP如何表示stream中的每个字节——sequence number
- 在TCP头部中，空间是稀缺的,每个字节在流中的序号并非表示为64位的，而是32位的。称为“sequence number”或“seqno”
- 这带来了了三个复杂性
  - 需要考虑封装32位整数
    - TCP中的流可以是任意长度，但是2^32字节只有4GiB，是远远不够的
    - 一旦一个32位的sequence number计数到2^32 − 1, 流中的下一个字节sequence number就会归零  
  - TCP sequence numbers起始于任意值
    -  为了提高安全性，避免和属于同一对节点之间之前的连接的segment混淆，TCP试图确保sequence number无法泄露，也无法重复，所以sequence number不从0开始
    -  首个sequenc number是一个32位的随机数，称作Initial Sequence Number(ISN). 该数代表SYN(流的起始)
    -  后续sequence number恢复正常: 数据的第一个byte的sequence number为ISN+1 (mod 2^32), 第二个字节的sequence number为ISN+2(mod 2^32)
  - 逻辑起始各占一个sequence number
    - 为了确保接收到数据的全部字节, TCP保证流的开头和结尾被可靠地接收
    - 因此, 在TCP中，SYN(beginning-of-stream)和FIN(end-of-stream) 控制标志也赋予了sequence number. 

- 这些sequence number(seqno)通过包含在TCP帧中进行传输
- 同样也有两个流——两个方向各一个。每个流都有各自的sequence number和一个 不同的随机ISN 
  
- absolute sequence number 
  - 通常起始于0，不需要封装, 和“stream index”有关：在StreamReassembler中已经使用: 流中各个字节的序号
- 为了使这些差异更具体，考虑一个仅仅包含三个字母的流
  - 如果SYN碰巧有seqno为2^32 − 2, 那么各个字节的seqnos, absolute seqnos和stream indices为  

    |element|SYN|c|a|t|FIN|
    |---|---|---|---|---|---|
    |seqno|$2^{32}-2$|$2^{32}-1$|0|1|2|
    |absolute seqno|0|1|2|3|4|
    |stream index||0|1|2|
- 下表显示了这三个不同序号的差异

    |Sequence Numbers|Absolute Sequence Numbers|Stream Indices|
    |---|---|---|
    |从ISN开始|从0开始|从0开始|
    |包含SYN/FIN|包含SYN/FIN|忽略SYN/FIN|
    |32位，封装|64位，不封装|64位，不封装|
    |"seqno"|"absolute seqno"|"stream index"|

- absolute sequence number和stream indices之间的转换很容易，只需要加1或者减1
- sequence number和absolute sequence number之间的转换可能会产生一些不容易发现的bug。为了避免这些bug，用一个自定义的WrappingInt32类型来代表sequence number
- 并实现一个它和absolute sequence number(用uint64_t表示)之间的转换
  - `WrappingInt32 wrap(uint64 t n, WrappingInt32 isn)`
    - 给定一个absolute sequence number值n，和一个Initial Sequence Number(isn)，生成n的sequence number
  - `uint64 t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64 t checkpoint)`
    - 给定一个sequence number值n,其Initial Sequence Number(isn), 和一个absolute checkpoint sequence number，计算对应到n的最接近checkpoint的absolute sequence number
- 注意：因为任何seqno都对应多个absolute seqnos，例如，对于ISN为0，seqno “17”对应的absolute seqno有17,2^32 + 17,和2^33 + 17, 以及2^34 + 17等等
- checkpoint用于确定精确度：是一个absolute seqno的大概值，即±2^31范围内的64位数
- 本TCP实现中用最后一个reassembled的字节的序号作为checkpoint

# TCPReceiver
- 从对等方接收帧
- 用StreamReassembler重组ByteStream
- 计算acknowledgment number (ackno)和window size

ackno和window size最终传输回对等方
- segment received()
  - 每次从对等方收到一个新的栈帧都会调用TCPReceiver::segment received()
  - 该方法需要
    - 如果有必要，则设置Initial Sequence Number——第一个到达的带有SYN标志的帧的sequence number
      - 为了在32位的seqnos/acknos和其绝对等价之间转换，可能需要持续跟踪 
      - 该帧可能同时携带数据并包含FIN标志
    - 将任何数据包或者流终结标志压入StreamReassembler
      - 如果在TCP帧头部中设置了FIN标志，就意味着负荷的最后一个字节是整个流的最后一个字节
      - StreamReassembler中的流序号从0开始，因此可能要对seqnos使用unwrap方法
- ackno()
  - 返回一个包含第一个未知sequence number字节的optional<WrappingInt32>  
  这是windows的左边缘：receiver待接收的第一个字节
  - 如果还未设置ISN，则返回空的optional.
- window size()
  - 返回“first unassembled”序号（对应于ackno）和“first unacceptable”序号之间的距离

# 重要的几个变量

|变量|含义|
|---|---|
|last_byte_reassembled|已整理的最后一个字节的序号
|first_unassembled=ackno|待整理的下一个字节序号|
|unassembled_bytes|已接收并存储，但未整理的字节的数量|

# 注意
- 直接写入带有SYN和FIN的帧，状态为FIN_RECV，即stream_out().input_ended()返回true
- 先写入一个带有SYN的空帧，则ackno+1，但是stream_index=0