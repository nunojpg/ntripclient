# $Id: makefile,v 1.5 2007/12/12 09:45:23 stoecker Exp $
# probably works not with all compilers. Thought this should be easy
# fixable. There is nothing special at this source.

ifdef windir
CC   = gcc
OPTS = -Wall -W -O3 -DWINDOWSVERSION 
LIBS = -lwsock32
else
OPTS = -Wall -W -O3 
endif

ntripclient: ntripclient.c
	$(CC) $(OPTS) $? -o $@ $(LIBS)

clean:
	$(RM) ntripclient core*


archive:
	tar -czf ntripclient.tgz ntripclient.c makefile README startntripclient.sh

