#include "stream_reassembler.hh"
#include <iostream>

using namespace std;

void test1() {
    cout << "Test[1]:" << endl;
    StreamReassembler sr{8};

    string s1{"abc"};
    cout << "push_substring: " << s1 << endl;
    sr.push_substring(s1,0,0);
    cout << "Buffer: [";
    cout << sr.unassembled_bytes() << "]\n";
    cout << "ByteStream.stream_out.bytes_written = ";
    size_t bytes_in_stream(sr.stream_out().bytes_written());
    cout << bytes_in_stream << endl;
    cout << "bytes in bytestream:[";
    cout << sr.stream_out().read(bytes_in_stream) << "]\n";

    string s2{"bcdefgh"};
    cout << "push_substring: " << s2 << endl;
    sr.push_substring("bcdefgh",1,1);
    cout << "Buffer: [";
    cout << sr.unassembled_bytes() << "]\n";
    cout << "ByteStream.stream_out.bytes_written = ";
    bytes_in_stream = sr.stream_out().bytes_written();
    cout << bytes_in_stream << endl;
    cout << "bytes in bytestream:[";
    cout << sr.stream_out().read(bytes_in_stream) << "]\n";

    cout << "-----------------------------------------\n";
}

void test2() {
    cout << "Test[2]:" << endl;
    StreamReassembler sr{8};

    string s1{"abcd"};
    cout << "push_substring: " << s1 << endl;
    sr.push_substring(s1,0,0);
    cout << "Buffer: [";
    cout << sr.unassembled_bytes() << "]\n";
    cout << "ByteStream.stream_out.bytes_written = ";
    size_t bytes_in_stream(sr.stream_out().bytes_written());
    cout << bytes_in_stream << endl;
    cout << "bytes in bytestream:[";
    cout << sr.stream_out().read(bytes_in_stream) << "]\n";

    string s2{"efgh"};
    cout << "push_substring: " << s2 << endl;
    sr.push_substring("bcdefgh",1,1);
    cout << "Buffer: [";
    cout << sr.unassembled_bytes() << "]\n";
    cout << "ByteStream.stream_out.bytes_written = ";
    bytes_in_stream = sr.stream_out().bytes_written();
    cout << bytes_in_stream << endl;
    cout << "bytes in bytestream:[";
    cout << sr.stream_out().read(bytes_in_stream) << "]\n";

    cout << "-----------------------------------------\n";
}

void test3() {
    cout << "Test[3]:" << endl;
    StreamReassembler sr{1};

    string s1{"ab"};
    cout << "push_substring: " << s1 << endl;
    sr.push_substring(s1,0,0);
    cout << "Buffer: [";
    cout << sr.unassembled_bytes() << "]\n";
    cout << "ByteStream.stream_out.bytes_written = ";
    size_t bytes_in_stream(sr.stream_out().bytes_written());
    cout << bytes_in_stream << endl;

    string s2{"ab"};
    cout << "push_substring: " << s2 << endl;
    sr.push_substring(s2,1,0);
    cout << "Buffer: [";
    cout << sr.unassembled_bytes() << "]\n";
    cout << "ByteStream.stream_out.bytes_written = ";
    bytes_in_stream = sr.stream_out().bytes_written();
    cout << bytes_in_stream << endl;
    cout << "bytes in bytestream:[";
    cout << sr.stream_out().read(bytes_in_stream) << "]\n";

    string s3{"abc"};
    cout << "push_substring: " << s3 << endl;
    sr.push_substring(s3,1,0);
    cout << "Buffer: [";
    cout << sr.unassembled_bytes() << "]\n";
    cout << "ByteStream.stream_out.bytes_written = ";
    bytes_in_stream = sr.stream_out().bytes_written();
    cout << bytes_in_stream << endl;
    cout << "bytes in bytestream:[";
    cout << sr.stream_out().read(1) << "]\n";
    cout << "-----------------------------------------\n";
}

int main(void) {
    test1();    
    test2();    
    test3();    
}

