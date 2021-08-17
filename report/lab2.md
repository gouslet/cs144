# 任务
- TCPReceiver在传入的TCP帧(通过互联网传输的数据包的有效载荷)和传入的字节流之间进行转换
- TCPReceiver通过received()方法从互联网接收帧，转换为对StreamReassembler的调用，最终写入ByteStream
- TCPReceiver负责告诉发送方两件事
  - 首个unassembled字节的序号，称为"acknowledgment number"或者"ackno"，这是接收方希望从发送方接收的第一个字节
  - 首个unassembled序号和首个unacceptable序号之间的差值，称为"window size"
- ackno和window size共同描述了接收方的窗口：TCP发送方允许发送的序号范围
- 通过这个窗口，接收方可以实现流量控制，在接收方准备好接收之前限制发送方的发送

# TCP如何表示stream中的每个字节——sequence number
