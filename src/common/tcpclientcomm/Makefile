CC      = gcc
CXX     = g++
INC_HOME= ./
LIB_HOME= ./lib
INC     = -I. -I$(INC_HOME)  
CFLAGS = -g  -fPIC -DNDEBUG 

LIB_SO = -L./ -ltcptxtclient
LIB     = libtcptxtclient.a   
LIB_OBJS    = tcp_client.o
DEMO_OBJS = test_demo.o
BIN = demo

SERVER_OBJS  = test_server.o
SERVER = server

TEST_EPOLL_OBJS  = test_epoll.o
TEST_EPOLL = test_epoll

all: $(LIB) $(BIN) $(SERVER) $(TEST_EPOLL)

$(LIB): $(LIB_OBJS) 
	$(AR) rc $@ $(LIB_OBJS)  
$(BIN): $(DEMO_OBJS) 
	$(CXX) $(CFLAGS)  -o $@ $^ ${LIB_SO}
$(SERVER): $(SERVER_OBJS) 
	$(CXX) $(CFLAGS)  -o $@ $^ ${LIB_SO}
$(TEST_EPOLL): $(TEST_EPOLL_OBJS) 
	$(CXX) $(CFLAGS)  -o $@ $^ ${LIB_SO}
.c.o:
	$(CC) $(CFLAGS)  $(INC) -c  $<
.cpp.o:
	$(CXX) $(CFLAGS) $(INC) -c  $<

clean:
	rm -f $(LIB_OBJS) $(LIB) $(BIN)  ${DEMO_OBJS} ${SERVER} ${SERVER_OBJS} ${TEST_EPOLL} ${TEST_EPOLL_OBJS}
