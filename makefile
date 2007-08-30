# $Id$
# probably works not with all compilers. Thought this should be easy
# fixable. There is nothing special at this source.

ntripclient: ntripclient.c
	$(CC) -Wall -W -O3 $? -o $@

archive:
	tar -czf ntripclient.tgz ntripclient.c makefile README startntripclient.sh
