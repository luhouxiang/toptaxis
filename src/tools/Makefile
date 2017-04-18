CC      = gcc
CXX     = g++

INC     = -I.
CFLAGS = -Wall -pipe -g -D_GNU_SOURCE -fPIC
LIB_SO = -lc #-Wl,-rpath=./


DEMO_OBJS  = main.o tcp_client.o
BIN = demo

all: $(BIN)


	
$(BIN): $(DEMO_OBJS) 
	$(CXX) $(CFLAGS)  -o $@ $^ ${LIB_SO}
.c.o:
	$(CC) $(CFLAGS)  $(INC) -c  $<
.cpp.o:
	$(CXX) $(CFLAGS) $(INC) -c  $<

clean:
	rm -f $(BIN) ${DEMO_OBJS}

cleanobj:
	rm -f *.o


	
