# $Id: makefile,v 1.6 2007/12/14 14:52:16 stuerze Exp $
# probably works not with all compilers. Thought this should be easy
# fixable. There is nothing special at this source.

ifdef windir
CC   = gcc
OPTS = -Wall -W -O3 -DWINDOWSVERSION 
LIBS = -lwsock32
else
OPTS = -Wall -W -O3 
endif

ntripclient: ntripclient.c serial.c
	$(CC) $(OPTS) ntripclient.c -o $@ $(LIBS)

clean:
	$(RM) ntripclient core*


archive:
	tar -czf ntripclient.tgz ntripclient.c makefile README startntripclient.sh

