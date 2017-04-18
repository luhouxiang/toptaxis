#!/bin/sh

export IR_LIB=../lib
export LD_LIBRARY_PATH=$IR_LIB

_BIN=toptaxisservice

# 102400 blocks£¨4k/per blocks£© = 400M
ulimit -c 102400

if [ "$1" = "stop" ] ; then
	killall -TERM ${_BIN}
	sleep 2
elif [ "$1" = "restart" ]; then
	killall -TERM ${_BIN}
	sleep 2
	./${_BIN} 
elif [ "$1" = "start" ]; then
	./${_BIN}
else 
	echo "top.sh start|stop|restart"
fi
