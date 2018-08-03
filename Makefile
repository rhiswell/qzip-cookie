
CC=gcc
QAT_INCLUDE		= -I$(ICP_ROOT)/quickassist/include 		\
				  -I$(ICP_ROOT)/quickassist/include/dc 		\
				  -I$(ICP_ROOT)/quickassist/lookaside/access_layer/include
USDM_INCLUDE	= -I$(ICP_ROOT)/quickassit/utilities/libusdm_drv
QATZIP_INCLUDE 	= -I$(QATZIP_ROOT)/include
CFLAGS 			= $(QAT_INCLUDE) $(USDM_INCLUDE) $(QATZIP_INCLUDE)
LDLIBS 			= -lz -lqatzip

all: test qzpipe

test: test.c qzip_cookie.c
	$(CC) $^ -o $@ $(CFLAGS) $(LDLIBS)

qzpipe: qzpipe.c qzip_cookie.c
	$(CC) $^ -o $@ $(CFLAGS) $(LDLIBS)

clean:
	rm -f *.o test qzpipe

.PHONY: all test clean
