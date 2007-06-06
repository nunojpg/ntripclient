
# probably works not with all compilers. Thought this should be easy
# fixable. There is nothing special at this source.

NtripLinuxClient: NtripLinuxClient.c
	$(CC) -Wall -W -O3 $? -o $@

archive:
	zip -9 NtripLinuxClient.zip NtripLinuxClient.c makefile ReadmeLinuxClient.txt StartNtripLinuxClient
