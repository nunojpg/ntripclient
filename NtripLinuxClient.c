/*
  Easy example NTRIP client for Linux/Unix.
  $Id: NtripLinuxClient.c,v 1.24 2007/01/09 08:16:33 stoecker Exp $
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

#include <ctype.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>

/* The string, which is send as agent in HTTP request */
#define AGENTSTRING "NTRIP NtripLinuxClient"

#define MAXDATASIZE 1000 /* max number of bytes we can get at once */
#define ALARMTIME   (2*60)

/* CVS revision and version */
static char revisionstr[] = "$Revision: 1.24 $";
static char datestr[]     = "$Date: 2007/01/09 08:16:33 $";

struct Args
{
  const char *server;
  int         port;
  const char *user;
  const char *password;
  const char *nmea;
  const char *data;
  int         bitrate;
};

/* option parsing */
#ifdef NO_LONG_OPTS
#define LONG_OPT(a)
#else
#define LONG_OPT(a) a
static struct option opts[] = {
{ "bitrate",    no_argument,       0, 'b'},
{ "data",       required_argument, 0, 'd'},
{ "server",     required_argument, 0, 's'},
{ "password",   required_argument, 0, 'p'},
{ "port",       required_argument, 0, 'r'},
{ "user",       required_argument, 0, 'u'},
{ "nmea",       required_argument, 0, 'n'},
{ "help",       no_argument,       0, 'h'},
{0,0,0,0}};
#endif
#define ARGOPT "-d:bhp:r:s:u:n:"

#ifdef __GNUC__
static __attribute__ ((noreturn)) void sighandler_alarm(
int sig __attribute__((__unused__)))
#else /* __GNUC__ */
static void sighandler_alarm(int sig)
#endif /* __GNUC__ */
{
  fprintf(stderr, "ERROR: more than %d seconds no activity\n", ALARMTIME);
  exit(1);
}

static const char *geturl(const char *url, struct Args *args)
{
  static char buf[1000];
  static char *Buffer = buf;
  static char *Bufend = buf+sizeof(buf);

  if(strncmp("ntrip:", url, 6))
    return "URL must start with 'ntrip:'.";
  url += 6; /* skip ntrip: */

  if(*url != '@' && *url != '/')
  {
    /* scan for mountpoint */
    args->data = Buffer;
    while(*url && *url != '@' &&  *url != ';' &&*url != '/' && Buffer != Bufend)
      *(Buffer++) = *(url++);
    if(Buffer == args->data)
      return "Mountpoint required.";
    else if(Buffer >= Bufend-1)
      return "Parsing buffer too short.";
    *(Buffer++) = 0;
  }

  if(*url == '/') /* username and password */
  {
    ++url;
    args->user = Buffer;
    while(*url && *url != '@' && *url != ';' && *url != ':' && Buffer != Bufend)
      *(Buffer++) = *(url++);
    if(Buffer == args->user)
      return "Username cannot be empty.";
    else if(Buffer >= Bufend-1)
      return "Parsing buffer too short.";
    *(Buffer++) = 0;

    if(*url == ':') ++url;

    args->password = Buffer;
    while(*url && *url != '@' && *url != ';' && Buffer != Bufend)
      *(Buffer++) = *(url++);
    if(Buffer == args->password)
      return "Password cannot be empty.";
    else if(Buffer >= Bufend-1)
      return "Parsing buffer too short.";
    *(Buffer++) = 0;
  }

  if(*url == '@') /* server */
  {
    ++url;
    args->server = Buffer;
    while(*url && *url != ':' && *url != ';' && Buffer != Bufend)
      *(Buffer++) = *(url++);
    if(Buffer == args->server)
      return "Servername cannot be empty.";
    else if(Buffer >= Bufend-1)
      return "Parsing buffer too short.";
    *(Buffer++) = 0;

    if(*url == ':')
    {
      char *s2 = 0;
      args->port = strtol(++url, &s2, 10);
      if((*s2 && *s2 != ';') || args->port <= 0 || args->port > 0xFFFF)
        return "Illegal port number.";
      url = s2;
    }
  }
  if(*url == ';') /* NMEA */
  {
    args->nmea = ++url;
    while(*url)
      ++url;
  }

  return *url ? "Garbage at end of server string." : 0;
}

static int getargs(int argc, char **argv, struct Args *args)
{
  int res = 1;
  int getoptr;
  char *a;
  int i = 0, help = 0;
  char *t;

  args->server = "www.euref-ip.net";
  args->port = 2101;
  args->user = "";
  args->password = "";
  args->nmea = 0;
  args->data = 0;
  args->bitrate = 0;
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
    case 'n': args->nmea = optarg; break;
    case 'b': args->bitrate = 1; break;
    case 'h': help=1; break;
    case 1:
      {
        const char *err;
        if((err = geturl(optarg, args)))
        {
          fprintf(stderr, "%s\n\n", err);
          res = 0;
        }
      }
      break;
    case 'r': 
      args->port = strtoul(optarg, &t, 10);
      if((t && *t) || args->port < 1 || args->port > 65535)
        res = 0;
      break;
    case -1: break;
    }
  } while(getoptr != -1 && res);

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
    fprintf(stderr, "Version %s (%s) GPL\nUsage:\n%s -s server -u user ...\n"
    " -d " LONG_OPT("--data     ") "the requested data set\n"
    " -s " LONG_OPT("--server   ") "the server name or address\n"
    " -p " LONG_OPT("--password ") "the login password\n"
    " -r " LONG_OPT("--port     ") "the server port number (default 2101)\n"
    " -u " LONG_OPT("--user     ") "the user name\n"
    " -b " LONG_OPT("--bitrate  ") "output bitrate\n"
    "or using an URL:\n%s ntrip:mountpoint[/username[:password]][@server[:port]]\n"
    , revisionstr, datestr, argv[0], argv[0]);
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

  setbuf(stdout, 0);
  setbuf(stdin, 0);
  setbuf(stderr, 0);
  signal(SIGALRM,sighandler_alarm);
  alarm(ALARMTIME);

  if(getargs(argc, argv, &args))
  {
    int i, sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in their_addr; /* connector's address information */

    if(!(he=gethostbyname(args.server)))
    {
      fprintf(stderr, "Server name lookup failed for '%s'.\n", args.server);
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
      if(i > MAXDATASIZE-40 || i < 0) /* second check for old glibc */
      {
        fprintf(stderr, "Requested data too long\n");
        exit(1);
      }
      i += encode(buf+i, MAXDATASIZE-i-4, args.user, args.password);
      if(i > MAXDATASIZE-4)
      {
        fprintf(stderr, "Username and/or password too long\n");
        exit(1);
      }
      buf[i++] = '\r';
      buf[i++] = '\n';
      buf[i++] = '\r';
      buf[i++] = '\n';
      if(args.nmea)
      {
        int j = snprintf(buf+i, MAXDATASIZE-i, "%s\r\n", args.nmea);
        if(j >= 0 && i < MAXDATASIZE-i)
          i += j;
        else
        {
          fprintf(stderr, "NMEA string too long\n");
          exit(1);
        }
      }
    }
    if(send(sockfd, buf, (size_t)i, 0) != i)
    {
      perror("send");
      exit(1);
    }
    if(args.data)
    {
      int k = 0;
      int starttime = time(0);
      int lastout = starttime;
      int totalbytes = 0;

      while((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) != -1)
      {
        alarm(ALARMTIME);
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
          totalbytes += numbytes;
          if(totalbytes < 0) /* overflow */
          {
            totalbytes = 0;
            starttime = time(0);
            lastout = starttime;
          }
          fwrite(buf, (size_t)numbytes, 1, stdout);
          fflush(stdout);
          if(args.bitrate)
          {
            int t = time(0);
            if(t > lastout + 60)
            {
              lastout = t;
              fprintf(stderr, "Bitrate is %dbyte/s (%d seconds accumulated).\n",
              totalbytes/(t-starttime), t-starttime);
            }
          }
        }
      }
    }
    else
    {
      while((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) > 0)
      {
        fwrite(buf, (size_t)numbytes, 1, stdout);
      }
    }

    close(sockfd);
  }
  return 0;
}
