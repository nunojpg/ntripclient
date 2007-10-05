# $Id: makefile,v 1.3 2007/08/30 07:38:24 stoecker Exp $
# probably works not with all compilers. Thought this should be easy
# fixable. There is nothing special at this source.

ntripclient: ntripclient.c
	$(CC) -Wall -W -O3 $? -o $@


clean:
	$(RM) -f ntripclient core*


archive:
	tar -czf ntripclient.tgz ntripclient.c makefile README startntripclient.sh
