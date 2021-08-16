# 主流程
将string存入buffer中

从buffer中往stream中写入string

# 将string存入buffer中
```
if ds.empty || buffer.empty  
    buffer[ds.i] = ds  
    unasm-bytes += ds.length  
    return  

if buffer.len == 1   
    s = buffer.begin()  
    if ds.i < s.i   
        if ds.h < s.i  
            buffer[ds.i] = ds  
            unasm-bytes += ds.length  
        else  
            if ds.h >= s.h  
                buffer.remove[s.i]  
                buffer[ds.i] = ds  
                unasm-bytes += ds.length - s.length  
            else  
                ws = ds.substr(0,s.i - ds.i)  
                buffer[ws.i] = ws   
                unasm-bytes += ws.length    
    else   
        if ds.i > s.h  
            buffer[ds.i] = ds  
            unasm-bytes += ds.length    
        else    
            if ds.h <= s.h  
                return
            else   
                ws = ds.substr(s.h + 1 - ds.i,ds.h - s.h)  
                buffer[ws.i] = ws  
                unasm-bytes += ws.length  
    return  

// buffer.len >= 2 
s = buffer.begin()

while (ds != "" && s != buffer.end)  
    if ds.i < s.i   
        if ds.h < s.i  
            buffer[ds.i] = ds  
            unasm-bytes += ds.length  
            ds = ""  
        else   
            if ds.h <= s1.h  
                len = s.i - ds.i
                ws = ds.substr  (0,len)   
                ds = ""  
                buffer[ws.i] = ws  
                unasm-bytes += ws.length  
            else  
                len = s.i - ds.i  
                ws = ds.substr(0,len)  
                ds = ds.substr(s.h + 1 - ds.i,ds.h - s.h)  
                buffer[ws.i] = ws  
                unasm-bytes += ws.length  
    else  
        if ds.h <= s.h  
            ds = ""  
        else  
            ds = ds.substr(s.h + 1 - ds.i,ds.h - s.h)  
    s++  


if ds != ""  
    buffer[ds.i] = ds  
    unasm-bytes += ds.length  
```
# 从buffer中往stream中写入string
```
it = buffer.begin()

while it != buffer.end  
    if rc == 0  
        buffer.remove(it.i)  
        unasm-bytes -= it.length  
        it++  
    else  
        if it.i <= next_index  
            if it.h < next_index  
                buffer.remove(it.i)
                it++
            else
                len = it.h - next_index + 1
                ws = it.substr(next_index - it.i,len)
                stream.write(ws)
                next_index += len
                it++
        else  
            break
```
# 删除多余的string
```
it = buffer.end()  
while it != buffer.begin && rc < unassem_bytes  
    len= unassem_bytes - rc> it.length?it.length:unassem_bytes - rc   
    ws = it.substr(0,it.length - len)  
    buffer.remove(it.i)  
    if ws != ""  
        buffer[ws.i] = ws  
    unassem_bytes -= len  
```