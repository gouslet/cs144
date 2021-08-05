# 任务
实现一个负责重组流数据的数据结构——treamReassembler. 
  - 它接收一系列字节形成的子串和第一个字节在更大的流中的序号
  - 每个字节都有其自己的序号，从零往上增加
  - StreamReassembler拥有一个用于输出的ByteStream，每当reassembler了解到流的下一个字节，就将其写入ByteStream.
  - 属主可以随时访问和读取ByteStream