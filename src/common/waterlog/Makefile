CC		= gcc
CXX      =  g++
CFLAGS  = -Wall -pipe -D_NEW_LIC -g -D_GNU_SOURCE -DUSE_TBDEF
INCLUDE =  -I. -I../ -I../log 

WRITEMASTER	= test_waterlog_writemaster
WRITESLAVE = test_waterlog_writeslave
LOGSERVER = test_waterlog_logserver
READSTORE = test_waterlog_read
TSWATERLOG = ts_waterlog


TARGETS = $(WRITEMASTER) $(WRITESLAVE) $(LOGSERVER) $(READSTORE) $(TSWATERLOG)

all: $(TARGETS)

$(WRITEMASTER): test_waterlog_writemaster.o waterlog.o log.o
	$(CXX) $(CFLAGS) -o $@ $^ $(INCLUDE)

$(WRITESLAVE): test_waterlog_writeslave.o waterlog.o log.o
	$(CXX) $(CFLAGS) -o $@ $^ $(INCLUDE)

$(LOGSERVER): test_waterlog_logserver.o waterlog.o log.o
	$(CXX) $(CFLAGS) -o $@ $^ $(INCLUDE)

$(READSTORE): test_waterlog_read.o waterlog.o log.o
	$(CXX) $(CFLAGS) -o $@ $^ $(INCLUDE)
	
$(TSWATERLOG): ts_waterlog.o
	$(CXX) $(CFLAGS) -o $@ $^ $(INCLUDE)


log.o:
	$(CC) $(CFLAGS) -c $(INCLUDE) ../log/log.c

.cpp.o:
	$(CXX) $(CFLAGS) -c $(INCLUDE) $<

.c.o:
	$(CC) $(CFLAGS) -c $(INCLUDE) $<
	
clean:
	rm -f *.o
	rm -f $(TARGETS)
