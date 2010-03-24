# $Id: makefile,v 1.7 2008/04/15 13:27:49 stoecker Exp $
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
        zip -9 ntripclient.zip ntripclient.c makefile README serial.c

tgzarchive:
	tar -czf ntripclient.tgz ntripclient.c makefile README serial.c
