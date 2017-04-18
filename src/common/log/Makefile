CC = gcc
CXX= g++
#添加编译选项，编译宏
CFLGS = -Wall -g -DDEBUG -DTRACE

CFLAGS   = $(CFLGS) 
CPPFLAGS = $(CFLGS)
#添加源文件
OBJS = log.o test_log.o

BINS = test 

OBJ =$(OBJS)
#添加所依赖的库
LIB=-L ../lib
#需要包含的头文件路径
INCLUDE=-I./ -I../lib/ -I../include/

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

.c.o:
	$(CC) $(INCLUDE) -c $(CFLAGS) $<
.cpp.o:
	$(CXX) $(INCLUDE) -c $(CPPFLAGS) $<

