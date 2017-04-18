CC = gcc
CXX= g++
#添加编译选项，编译宏
CFLGS = -Wall -g -fPIC -DDEBUG -DTRACE

CFLAGS   = $(CFLGS) 
CPPFLAGS = $(CFLGS)
#添加源文件
OBJS = toptaxis.o msg_sync.o ts_waterlog.o toptaxis_main.o toptaxis_conf.o toptaxis_handle.o toptaxis_sync.o toptaxis_backup.o \
		cache_manage.o stringExt.o client_sync.o listener.o poll_thread.o poller.o timerlist.o xmlParser.o log.o tcp_client.o \
		dc_process.o comm_tools.o strutil.o comm_semp.o send_warn.o
       

BINS = toptaxisservice

OBJ =$(OBJS)
#添加所依赖的库
LIB= -ldl  -lz -lm -L./ -lbitservice_api -lqos_client64 \
	-L../common/warn/ -lattr_api
#需要包含的头文件路径
INCLUDE=-I./ -I../common/ -I../common/memory/ -I../common/decodefile/ -I../common/epollthread/ -I../common/log/ -I../common/tcpclientcomm/  -I../common/waterlog/ -I../common/commcontrol/  -I../common/nameservice/include/ -I../common/semp/ -I../common/stringext -I../common/vectorext\
	-I../common/warn/

all:  bin

bin: $(BINS)
obj: $(OBJ) 

clean: cleanobj  cleanbin 


cleanbin:
	rm -f $(BINS)
cleanobj: 
	rm -f $(OBJ) 

$(BINS): $(OBJS) 
	$(CXX) $(CPPFLAGS)  -o $@ $^ ${LIB}
	
log.o:
	$(CC) $(CFLAGS) -c $(INCLUDE) ../common/log/log.c
	
toptaxis.o:
	$(CXX) $(CFLAGS) -c $(INCLUDE) ../common/toptaxis.cpp

cache_manage.o:
	$(CXX) $(CFLAGS) -c $(INCLUDE) ../common/memory/cache_manage.cpp

xmlParser.o:
	$(CXX) $(CFLAGS) -c $(INCLUDE) ../common/decodefile/xmlParser.cpp
    
client_sync.o:
	$(CXX) $(CFLAGS) -c $(INCLUDE) ../common/epollthread/client_sync.cpp
    
listener.o:
	$(CXX) $(CFLAGS) -c $(INCLUDE) ../common/epollthread/listener.cpp
  
poll_thread.o:
	$(CXX) $(CFLAGS) -c $(INCLUDE) ../common/epollthread/poll_thread.cpp
    
poller.o:
	$(CXX) $(CFLAGS) -c $(INCLUDE) ../common/epollthread/poller.cpp
    
timerlist.o:
	$(CXX) $(CFLAGS) -c $(INCLUDE) ../common/epollthread/timerlist.cpp

tcp_client.o:
	$(CXX) $(CFLAGS) -c $(INCLUDE) ../common/tcpclientcomm/tcp_client.cpp
	
ts_waterlog.o:
	$(CXX) $(CFLAGS) -c $(INCLUDE) ../common/waterlog/ts_waterlog.cpp
	
send_warn.o:
	$(CXX) $(CFLAGS) -c $(INCLUDE) ../common/warn/send_warn.cpp	

comm_semp.o:
	$(CXX) $(CFLAGS) -c $(INCLUDE) ../common/semp/comm_semp.cpp	
	
stringExt.o:
	$(CXX) $(CFLAGS) -c $(INCLUDE) ../common/stringext/stringExt.cpp	
	
.c.o:
	$(CC) $(INCLUDE) -c $(CFLAGS) $<
.cpp.o:
	$(CXX) $(INCLUDE) -c $(CPPFLAGS) $<

