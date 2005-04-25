/*
  Easy example NTRIP client for Linux/Unix.
  $Id: NtripLinuxClient.c,v 1.11 2005/04/19 11:28:10 stoecker Exp $
  Copyright (C) 2003-2005 by Dirk Stoecker <soft@dstoecker.de>
    
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  or read http://www.gnu.org/licenses/gpl.txt
*/

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

/* The string, which is send as agent in HTTP request */
#define AGENTSTRING "NTRIP NtripLinuxClient"

#define MAXDATASIZE 1000 /* max number of bytes we can get at once */

/* CVS revision and version */
static char revisionstr[] = "$Revision: 1.11 $";
static char datestr[]     = "$Date: 2005/04/19 11:28:10 $";

struct Args
{
  const char *server;
  int         port;
  const char *user;
  const char *password;
  const char *data;
};

/* option parsing */
#ifdef NO_LONG_OPTS
#define LONG_OPT(a)
#else
#define LONG_OPT(a) a
static struct option opts[] = {
{ "data",       required_argument, 0, 'd'},
{ "server",     required_argument, 0, 's'},
{ "password",   required_argument, 0, 'p'},
{ "port",       required_argument, 0, 'r'},
{ "user",       required_argument, 0, 'u'},
{ "help",       no_argument,       0, 'h'},
{0,0,0,0}};
#endif
#define ARGOPT "d:hp:r:s:u:"

static int getargs(int argc, char **argv, struct Args *args)
{
  int res = 1;
  int getoptr;
  char *a;
  int i = 0, help = 0;
  char *t;

  args->server = "www.euref-ip.net";
  args->port = 80;
  args->user = "";
  args->password = "";
  args->data = 0;
  help = 0;

  do
  {
#ifdef NO_LONG_OPTS
    switch((getoptr = getopt(argc, argv, ARGOPT)))
#else
    switch((getoptr = getopt_long(argc, argv, ARGOPT, opts, 0)))
#endif
    {
    case 's': args->server = optarg; break;
    case 'u': args->user = optarg; break;
    case 'p': args->password = optarg; break;
    case 'd': args->data = optarg; break;
    case 'h': help=1; break;
    case 'r': 
      args->port = strtoul(optarg, &t, 10);
      if((t && *t) || args->port < 1 || args->port > 65535)
        res = 0;
      break;
    case -1: break;
    }
  } while(getoptr != -1 || !res);

  for(a = revisionstr+11; *a && *a != ' '; ++a)
    revisionstr[i++] = *a;
  revisionstr[i] = 0;
  datestr[0] = datestr[7];
  datestr[1] = datestr[8];
  datestr[2] = datestr[9];
  datestr[3] = datestr[10];
  datestr[5] = datestr[12];
  datestr[6] = datestr[13];
  datestr[8] = datestr[15];
  datestr[9] = datestr[16];
  datestr[4] = datestr[7] = '-';
  datestr[10] = 0;

  if(!res || help)
  {
    fprintf(stderr, "Version %s (%s) GPL\nUsage: %s -s server -u user ...\n"
    " -d " LONG_OPT("--data     ") "the requested data set\n"
    " -s " LONG_OPT("--server   ") "the server name or address\n"
    " -p " LONG_OPT("--password ") "the login password\n"
    " -r " LONG_OPT("--port     ") "the server port number (default 80)\n"
    " -u " LONG_OPT("--user     ") "the user name\n"
    , revisionstr, datestr, argv[0]);
    exit(1);
  }
  return res;
}

static const char encodingTable [64] = {
  'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
  'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
  'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
  'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
};

/* does not buffer overrun, but breaks directly after an error */
/* returns the number of required bytes */
static int encode(char *buf, int size, const char *user, const char *pwd)
{
  unsigned char inbuf[3];
  char *out = buf;
  int i, sep = 0, fill = 0, bytes = 0;

  while(*user || *pwd)
  {
    i = 0;
    while(i < 3 && *user) inbuf[i++] = *(user++);
    if(i < 3 && !sep)    {inbuf[i++] = ':'; ++sep; }
    while(i < 3 && *pwd)  inbuf[i++] = *(pwd++);
    while(i < 3)         {inbuf[i++] = 0; ++fill; }
    if(out-buf < size-1)
      *(out++) = encodingTable[(inbuf [0] & 0xFC) >> 2];
    if(out-buf < size-1)
      *(out++) = encodingTable[((inbuf [0] & 0x03) << 4)
               | ((inbuf [1] & 0xF0) >> 4)];
    if(out-buf < size-1)
    {
      if(fill == 2)
        *(out++) = '=';
      else
        *(out++) = encodingTable[((inbuf [1] & 0x0F) << 2)
                 | ((inbuf [2] & 0xC0) >> 6)];
    }
    if(out-buf < size-1)
    {
      if(fill >= 1)
        *(out++) = '=';
      else
        *(out++) = encodingTable[inbuf [2] & 0x3F];
    }
    bytes += 4;
  }
  if(out-buf < size)
    *out = 0;
  return bytes;
}

int main(int argc, char **argv)
{
  struct Args args;

  if(getargs(argc, argv, &args))
  {
    int i, sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in their_addr; /* connector's address information */

    if(!(he=gethostbyname(args.server)))
    {
      perror("gethostbyname");
      exit(1);
    }
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror("socket");
      exit(1);
    }
    their_addr.sin_family = AF_INET;    /* host byte order */
    their_addr.sin_port = htons(args.port);  /* short, network byte order */
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(their_addr.sin_zero), '\0', 8);
    if(connect(sockfd, (struct sockaddr *)&their_addr,
    sizeof(struct sockaddr)) == -1)
    {
      perror("connect");
      exit(1);
    }

    if(!args.data)
    {
      i = snprintf(buf, MAXDATASIZE,
      "GET / HTTP/1.0\r\n"
      "User-Agent: %s/%s\r\n"
#ifdef UNUSED
      "Accept: */*\r\n"
      "Connection: close\r\n"
#endif
      "\r\n"
      , AGENTSTRING, revisionstr);
    }
    else
    {
      i=snprintf(buf, MAXDATASIZE-40, /* leave some space for login */
      "GET /%s HTTP/1.0\r\n"
      "User-Agent: %s/%s\r\n"
#ifdef UNUSED
      "Accept: */*\r\n"
      "Connection: close\r\n"
#endif
      "Authorization: Basic "
      , args.data, AGENTSTRING, revisionstr);
      if(i > MAXDATASIZE-40 && i < 0) /* second check for old glibc */
      {
        fprintf(stderr, "Requested data too long\n");
        exit(1);
      }
      i += encode(buf+i, MAXDATASIZE-i-5, args.user, args.password);
      if(i > MAXDATASIZE-5)
      {
        fprintf(stderr, "Username and/or password too long\n");
        exit(1);
      }
      snprintf(buf+i, 5, "\r\n\r\n");
      i += 5;
    }
    if(send(sockfd, buf, i, 0) != i)
    {
      perror("send");
      exit(1);
    }
    if(args.data)
    {
      int k = 0;
      while((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) != -1)
      {
        if(!k)
        {
          if(numbytes < 12 || strncmp("ICY 200 OK\r\n", buf, 12))
          {
            fprintf(stderr, "Could not get the requested data: ");
            for(k = 0; k < numbytes && buf[k] != '\n' && buf[k] != '\r'; ++k)
            {
              fprintf(stderr, "%c", isprint(buf[k]) ? buf[k] : '.');
            }
            fprintf(stderr, "\n");
            exit(1);
          }
          ++k;
        }
        else
        {
          fwrite(buf, numbytes, 1, stdout);
          fflush(stdout);
        }
      }
    }
    else
    {
      while((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) > 0)
      {
        fwrite(buf, numbytes, 1, stdout);
      }
    }

    close(sockfd);
  }
  return 0;
}
