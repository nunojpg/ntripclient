# $Id: makefile,v 1.4 2007/10/05 15:32:13 stuerze Exp $
# probably works not with all compilers. Thought this should be easy
# fixable. There is nothing special at this source.

ifdef windir
OPTS = -Wall -W -O3 -DWINDOWSVERSION -lwsock32
else
OPTS = -Wall -W -O3 
endif

ntripclient: ntripclient.c
	$(CC) $(OPTS) $? -o $@

clean:
	$(RM) ntripclient core*


archive:
	tar -czf ntripclient.tgz ntripclient.c makefile README startntripclient.sh
