CC := g++
LIBSPONGE := ../../libsponge
CXXFLAGS := -I$(LIBSPONGE) -g
PROGS := uint_example
# stream_reassembler_example\
	# strm_reassem_win_example

all:$(PROGS) 

# $(PROGS): $(PROGS).cc $(LIBSPONGE)/byte_stream.cc $(LIBSPONGE)/stream_reassembler.cc
# $(CC) $(CXXFLAGS) $(PROGS).cc $(LIBSPONGE)/byte_stream.cc $(LIBSPONGE)/stream_reassembler.cc -o $@
# $(LIBSPONGE)/stream_reassembler.o: $(LIBSPONGE)/byte_stream.o

# $(LIBSPONGE)/byte_stream.o: $(LIBSPONGE)/byte_stream.cc
$(PROGS): $(PROGS).cc 


clean:
	rm -rf $(PROGS) $(LIBSPONGE)/*.o *.out