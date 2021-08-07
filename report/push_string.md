# 主流程
if remaining_capacity == 0  
    丢弃;  
if index <= next_index  
    string_2_bytestream(data,index)
else  
    将data中与buffer中不重叠的部分存入buffer  
    
如果buffer的size大于最新的remaining_capacity，则删除buffer中index偏大的部分  

void string_2_stream(str,index) {
    start = next_index - index  #data中可写入ByteStream的起始位置  
    slen = str.length - (next_index - index + 1);#str中index大于等于next_index的总长度  
    flag = remaining_capacity < slen   
    len = flag?remaining_capacity:slen #str中可写入ByteStream的长度  
    获取将data中index为[next_index,next_index + len + 1]的部分s  
        s = str.substr(start,len)  #待写入ByteStream的str子串  
    将s写入ByteStream;  
    更新next_index为next_index + len  
    if !flag  
        buffer_2_stream();  
    else   
        删除Buffer中的全部元素
}

# 将buffer中可写入ByteStream的部分写入ByteStream，同时删除Buffer中index小于next_index的部分;

void buffer_2_stream() {
    for (s:buffer) 
        if s.index <= next_index 
            if s.index + s.length < next_index
                delete s
            else
                string_2_stream(s,s.index)
        else
            break;
}

# 将data中与buffer中不重叠的部分存入buffer
for (s:buffer)
    if data == "" 
        break;
    if s.index <= index
        if s.index + s.length < index
            continue;
        else
            if (index + data.length <= s.index + s.length)
                continue;
            else
                start = s.index + s.length - index              
                len = index + data.length - s.index - s.length  
                data = data.substr(start,len)
                index += start
    else
        start = s.index + s.length - index             
        len = data.length + index - s.index - s.length
        if len < 0
            continue;
        data = data.substr(start,len)
        index += start
    
    if s是最后一块&&unassem_bytes < remaining_capacity
        wslen = data.length < remaining_capacity?data.length:remaining_capacity
        ws = data.substr(0,wslen)
        buffer[index] = ws
        index += wslen
        unas += wslen

    if sn_index = (s->next).index > index
        wslen = sn_index - index
        ws = data.substr(0,wslen)
        buffer[index] = ws
        index = sn_index
        data = data.substr(wslen-1,data.length - wslen)
        unas += wslen

# 如果buffer的size大于最新的remaining_capacity，则删除buffer中index偏大的部分  

delta = unass_bytes - remaining_capacity

for reverse s:buffer  
    slen = s.length  
    if slen > delta  
        s = s.substr(0,slen - delta)  
        buffer[s.index] = s
        unas -= delta  
    else  
        buffer.erase(s.index)  
        delta -= s.length  
        unass -= s.length  
    if delta == 0  
        break;  
