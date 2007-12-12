/*
  Easy example NTRIP client for POSIX.
  $Id: ntripclient.c,v 1.36 2007/11/20 12:54:05 stoecker Exp $
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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#ifdef WINDOWSVERSION
  #include <winsock.h>
  typedef SOCKET sockettype;
  typedef u_long in_addr_t;
  typedef size_t socklen_t;
#else
  typedef int sockettype;
  #include <signal.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netdb.h>

  #define closesocket(sock)       close(sock)
  #define ALARMTIME   (2*60)
#endif

#ifndef COMPILEDATE
#define COMPILEDATE " built " __DATE__
#endif

/* The string, which is send as agent in HTTP request */
#define AGENTSTRING "NTRIP NtripClientPOSIX"

#define MAXDATASIZE 1000 /* max number of bytes we can get at once */

/* CVS revision and version */
static char revisionstr[] = "$Revision: 1.36 $";
static char datestr[]     = "$Date: 2007/11/20 12:54:05 $";

enum MODE { HTTP = 1, RTSP = 2, NTRIP1 = 3, AUTO = 4, END };

struct Args
{
  const char *server;
  const char *port;
  const char *user;
  const char *proxyhost;
  const char *proxyport;
  const char *password;
  const char *nmea;
  const char *data;
  int         bitrate;
  int         mode;

  int         udpport;
  int         initudp;
};

/* option parsing */
#ifdef NO_LONG_OPTS
#define LONG_OPT(a)
#else
#define LONG_OPT(a) a
static struct option opts[] = {
{ "bitrate",    no_argument,       0, 'b'},
{ "data",       required_argument, 0, 'd'}, /* compatibility */
{ "mountpoint", required_argument, 0, 'm'},
{ "initudp",    no_argument,       0, 'I'},
{ "udpport",    required_argument, 0, 'P'},
{ "server",     required_argument, 0, 's'},
{ "password",   required_argument, 0, 'p'},
{ "port",       required_argument, 0, 'r'},
{ "proxyport",  required_argument, 0, 'R'},
{ "proxyhost",  required_argument, 0, 'S'},
{ "user",       required_argument, 0, 'u'},
{ "nmea",       required_argument, 0, 'n'},
{ "mode",       required_argument, 0, 'M'},
{ "help",       no_argument,       0, 'h'},
{0,0,0,0}};
#endif
#define ARGOPT "-d:m:bhp:r:s:u:n:S:R:M:IP:"

int stop = 0;
#ifndef WINDOWSVERSION
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

#ifdef __GNUC__
static void sighandler_int(int sig __attribute__((__unused__)))
#else /* __GNUC__ */
static void sighandler_alarm(int sig)
#endif /* __GNUC__ */
{
  alarm(2);
  stop = 1;
}
#endif /* WINDOWSVERSION */

static const char *encodeurl(const char *req)
{
  char *h = "0123456789abcdef";
  static char buf[128];
  char *urlenc = buf;
  char *bufend = buf + sizeof(buf) - 3;

  while(*req && urlenc < bufend)
  {
    if(isalnum(*req) 
    || *req == '-' || *req == '_' || *req == '.')
      *urlenc++ = *req++;
    else
    {
      *urlenc++ = '%';
      *urlenc++ = h[*req >> 4];
      *urlenc++ = h[*req & 0x0f];
      req++;
    }
  }
  *urlenc = 0;
  return buf;
}

static const char *geturl(const char *url, struct Args *args)
{
  static char buf[1000];
  static char *Buffer = buf;
  static char *Bufend = buf+sizeof(buf);
  char *h = "0123456789abcdef";

  if(strncmp("ntrip:", url, 6))
    return "URL must start with 'ntrip:'.";
  url += 6; /* skip ntrip: */

  if(*url != '@' && *url != '/')
  {
    /* scan for mountpoint */
    args->data = Buffer;
    if(*url != '?')
    {
       while(*url && *url != '@' &&  *url != ';' && *url != '/' && Buffer != Bufend)
         *(Buffer++) = *(url++);
    }
    else
    {
       while(*url && *url != '@' &&  *url != '/' && Buffer != Bufend) 
       {
          if(isalnum(*url) || *url == '-' || *url == '_' || *url == '.')
            *Buffer++ = *url++;
          else
          {
            *Buffer++ = '%';
            *Buffer++ = h[*url >> 4];
            *Buffer++ = h[*url & 0x0f];
            url++;
          }      
       }
    }
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
    if(*url != '@' && *url != ':')
    {
      args->server = Buffer;
      while(*url && *url != '@' && *url != ':' && *url != ';' && Buffer != Bufend)
        *(Buffer++) = *(url++);
      if(Buffer == args->server)
        return "Servername cannot be empty.";
      else if(Buffer >= Bufend-1)
        return "Parsing buffer too short.";
      *(Buffer++) = 0;
    }

    if(*url == ':')
    {
      ++url;
      args->port = Buffer;
      while(*url && *url != '@' && *url != ';' && Buffer != Bufend)
        *(Buffer++) = *(url++);
      if(Buffer == args->port)
        return "Port cannot be empty.";
      else if(Buffer >= Bufend-1)
        return "Parsing buffer too short.";
      *(Buffer++) = 0;
    }

    if(*url == '@') /* proxy */
    {
      ++url;
      args->proxyhost = Buffer;
      while(*url && *url != ':' && *url != ';' && Buffer != Bufend)
        *(Buffer++) = *(url++);
      if(Buffer == args->proxyhost)
        return "Proxy servername cannot be empty.";
      else if(Buffer >= Bufend-1)
        return "Parsing buffer too short.";
      *(Buffer++) = 0;

      if(*url == ':')
      {
        ++url;
        args->proxyport = Buffer;
        while(*url && *url != ';' && Buffer != Bufend)
          *(Buffer++) = *(url++);
        if(Buffer == args->proxyport)
          return "Proxy port cannot be empty.";
        else if(Buffer >= Bufend-1)
          return "Parsing buffer too short.";
        *(Buffer++) = 0;
      }
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

  args->server = "www.euref-ip.net";
  args->port = "2101";
  args->user = "";
  args->password = "";
  args->nmea = 0;
  args->data = 0;
  args->bitrate = 0;
  args->proxyhost = 0;
  args->proxyport = "2101";
  args->mode = AUTO;
  args->initudp = 0;
  args->udpport = 0;
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
    case 'd':
    case 'm':
      if(optarg && *optarg == '?') 
        args->data = encodeurl(optarg);
      else 
        args->data = optarg; 
      break;
    case 'I': args->initudp = 1; break;
    case 'P': args->udpport = strtol(optarg, 0, 10); break;
    case 'n': args->nmea = optarg; break;
    case 'b': args->bitrate = 1; break;
    case 'h': help=1; break;
    case 'r': args->port = optarg; break;
    case 'S': args->proxyhost = optarg; break;
    case 'R': args->proxyport = optarg; break;
    case 'M':
      args->mode = 0;
      if (!strcmp(optarg,"n") || !strcmp(optarg,"ntrip1"))
        args->mode = NTRIP1;
      else if(!strcmp(optarg,"h") || !strcmp(optarg,"http"))
        args->mode = HTTP;
      else if(!strcmp(optarg,"r") || !strcmp(optarg,"rtsp"))
        args->mode = RTSP;
      else if(!strcmp(optarg,"a") || !strcmp(optarg,"auto"))
        args->mode = AUTO;
      else args->mode = atoi(optarg);
      if((args->mode == 0) || (args->mode >= END))
      {
        fprintf(stderr, "Mode %s unknown\n", optarg);
        res = 0;
      }
      break;
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
    fprintf(stderr, "Version %s (%s) GPL" COMPILEDATE "\nUsage:\n%s -s server -u user ...\n"
    " -m " LONG_OPT("--mountpoint ") "the requested data set or sourcetable filtering criteria\n"
    " -s " LONG_OPT("--server     ") "the server name or address\n"
    " -p " LONG_OPT("--password   ") "the login password\n"
    " -r " LONG_OPT("--port       ") "the server port number (default 2101)\n"
    " -u " LONG_OPT("--user       ") "the user name\n"
    " -n " LONG_OPT("--nmea       ") "NMEA string for sending to server\n"
    " -b " LONG_OPT("--bitrate    ") "output bitrate\n"
    " -S " LONG_OPT("--proxyhost  ") "proxy name or address\n"
    " -R " LONG_OPT("--proxyport  ") "proxy port, optional (default 2101)\n"
    " -M " LONG_OPT("--mode       ") "mode for data request\n"
    "     Valid modes are:\n"
    "     1, h, http     NTRIP Version 2.0 Caster in TCP/IP mode\n"
    "     2, r, rtsp     NTRIP Version 2.0 Caster in RTSP/RTP mode\n"
    "     3, n, ntrip1   NTRIP Version 1.0 Caster\n"
    "     4, a, auto     automatic detection (default)\n"
    "or using an URL:\n%s ntrip:mountpoint[/user[:password]][@[server][:port][@proxyhost[:proxyport]]][;nmea]\n"
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
#ifndef WINDOWSVERSION
  signal(SIGALRM,sighandler_alarm);
  signal(SIGINT,sighandler_int);
  alarm(ALARMTIME);
#else
  WSADATA wsaData;
  if(WSAStartup(MAKEWORD(1,1),&wsaData))
  {
    fprintf(stderr, "Could not init network access.\n");
    return 20;
  }
#endif

  if(getargs(argc, argv, &args))
  {
    int sleeptime = 0;
    do
    {
      sockettype sockfd;
      int numbytes;  
      char buf[MAXDATASIZE];
      struct sockaddr_in their_addr; /* connector's address information */
      struct hostent *he;
      struct servent *se;
      const char *server, *port, *proxyserver = 0;
      char proxyport[6];
      char *b;
      long i;
      if(sleeptime)
      {
#ifdef WINDOWSVERSION
        Sleep(sleeptime*1000);
#else
        sleep(sleeptime);
#endif
        sleeptime += 2;
      }
      else
      {
        sleeptime = 1;
      }
#ifndef WINDOWSVERSION
      alarm(ALARMTIME);
#endif
      if(args.proxyhost)
      {
        int p;
        if((i = strtol(args.port, &b, 10)) && (!b || !*b))
          p = i;
        else if(!(se = getservbyname(args.port, 0)))
        {
          fprintf(stderr, "Can't resolve port %s.", args.port);
          exit(1);
        }
        else
        {
          p = ntohs(se->s_port);
        }
        snprintf(proxyport, sizeof(proxyport), "%d", p);
        port = args.proxyport;
        proxyserver = args.server;
        server = args.proxyhost;
      }
      else
      {
        server = args.server;
        port = args.port;
      }
      memset(&their_addr, 0, sizeof(struct sockaddr_in));
      if((i = strtol(port, &b, 10)) && (!b || !*b))
        their_addr.sin_port = htons(i);
      else if(!(se = getservbyname(port, 0)))
      {
        fprintf(stderr, "Can't resolve port %s.", port);
        exit(1);
      }
      else
      {
        their_addr.sin_port = se->s_port;
      }
      if(!(he=gethostbyname(server)))
      {
        fprintf(stderr, "Server name lookup failed for '%s'.\n", server);
        exit(1);
      }
      if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
      {
        perror("socket");
        exit(1);
      }
      their_addr.sin_family = AF_INET;
      their_addr.sin_addr = *((struct in_addr *)he->h_addr);
      
      if(args.data && *args.data != '%' && args.mode == RTSP)
      {
        struct sockaddr_in local;
        int sockudp, localport;
        int cseq = 1;
        socklen_t len;

        if((sockudp = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        {
          perror("socket");
          exit(1);
        }
        /* fill structure with local address information for UDP */
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_port = htons(args.udpport);
        local.sin_addr.s_addr = htonl(INADDR_ANY); 
        len = sizeof(local);
        /* bind() in order to get a random RTP client_port */ 
        if((bind(sockudp, (struct sockaddr *)&local, len)) < 0)
        {
          perror("bind");
          exit(1);
        }
        if((getsockname(sockudp, (struct sockaddr*)&local, &len)) != -1) 
        {
          localport = ntohs(local.sin_port); 
        }
        else
        {
          perror("local access failed"); 
          exit(1);
        }
        if(connect(sockfd, (struct sockaddr *)&their_addr,
        sizeof(struct sockaddr)) == -1)
        {
          perror("connect");
          exit(1);
        }
        i=snprintf(buf, MAXDATASIZE-40, /* leave some space for login */
        "SETUP rtsp://%s%s%s/%s RTSP/1.0\r\n" 	        
        "CSeq: %d\r\n"		
        "Ntrip-Version: Ntrip/2.0\r\n"
        "Ntrip-Component: Ntripclient\r\n"
        "User-Agent: %s/%s\r\n"
        "Transport: RTP/GNSS;unicast;client_port=%u\r\n"
        "Authorization: Basic ",
        args.server, proxyserver ? ":" : "", proxyserver ? args.port : "",
        args.data, cseq++, AGENTSTRING, revisionstr, localport);
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
          if(j >= 0 && j < MAXDATASIZE-i)
            i += j;
          else
          {
            fprintf(stderr, "NMEA string too long\n");
            exit(1);
          }
        }
        if(send(sockfd, buf, (size_t)i, 0) != i)
        {
          perror("send");
          exit(1);
        }
        if((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) != -1)
        {
          if(numbytes >= 17 && !strncmp(buf, "RTSP/1.0 200 OK\r\n", 17))
	  {
            int serverport = 0, session = 0;
            const char *portcheck = "server_port=";
            const char *sessioncheck = "session: ";
            int l = strlen(portcheck)-1;
            int j=0;
            for(i = 0; j != l && i < numbytes-l; ++i)
            {
              for(j = 0; j < l && tolower(buf[i+j]) == portcheck[j]; ++j)
                ;
            }
            if(i == numbytes-l)
            {
              fprintf(stderr, "No server port number found\n");
              exit(1);
            }
            else
            {
              i+=l;
              while(i < numbytes && buf[i] >= '0' && buf[i] <= '9')
                serverport = serverport * 10 + buf[i++]-'0';
              if(buf[i] != '\r' && buf[i] != ';')
              {
                fprintf(stderr, "Could not extract server port\n");
                exit(1);
              }
            }
            l = strlen(sessioncheck)-1;
            j=0;
            for(i = 0; j != l && i < numbytes-l; ++i)
            {
              for(j = 0; j < l && tolower(buf[i+j]) == sessioncheck[j]; ++j)
                ;
            }
            if(i == numbytes-l)
            {
              fprintf(stderr, "No session number found\n");
              exit(1);
            }
            else
            {
              i+=l;
              while(i < numbytes && buf[i] >= '0' && buf[i] <= '9')
                session = session * 10 + buf[i++]-'0';
              if(buf[i] != '\r')
              {
                fprintf(stderr, "Could not extract session number\n");
                exit(1);
              }
            }
            if(args.initudp)
            {
              printf("Sending initial UDP packet\n");
              struct sockaddr_in casterRTP;
              char rtpbuffer[12];
              int i;
              rtpbuffer[0] = (2<<6);
              /* padding, extension, csrc are empty */
              rtpbuffer[1] = 96;
              /* marker is empty */
              rtpbuffer[2] = 0;
              rtpbuffer[3] = 0;
              rtpbuffer[4] = 0;
              rtpbuffer[5] = 0;
              rtpbuffer[6] = 0;
              rtpbuffer[7] = 0;
              rtpbuffer[8] = 0;
              rtpbuffer[9] = 0;
              rtpbuffer[10] = 0;
              rtpbuffer[11] = 0;
              /* fill structure with caster address information for UDP */
              memset(&casterRTP, 0, sizeof(casterRTP));
              casterRTP.sin_family = AF_INET;
              casterRTP.sin_port   = htons(((uint16_t)serverport));
              casterRTP.sin_addr   = *((struct in_addr *)he->h_addr);

              if((i = sendto(sockudp, rtpbuffer, 12, 0,
              (struct sockaddr *) &casterRTP, sizeof(casterRTP))) != 12)
                perror("WARNING: could not send initial UDP packet");
            }

            i = snprintf(buf, MAXDATASIZE,
            "PLAY rtsp://%s%s%s/%s RTSP/1.0\r\n"	        
            "CSeq: %d\r\n"
            "Session: %d\r\n"
            "\r\n", 
            args.server, proxyserver ? ":" : "", proxyserver ? args.port : "",
            args.data, cseq++, session);

            if(i > MAXDATASIZE || i < 0) /* second check for old glibc */
            {
              fprintf(stderr, "Requested data too long\n");
              exit(1);
            }
            if(send(sockfd, buf, (size_t)i, 0) != i)
            {
              perror("send");
              exit(1);
            }
            if((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) != -1)
            {
              if(numbytes >= 17 && !strncmp(buf, "RTSP/1.0 200 OK\r\n", 17))
              {
                struct sockaddr_in addrRTP;
                /* fill structure with caster address information for UDP */
                memset(&addrRTP, 0, sizeof(addrRTP));
                addrRTP.sin_family = AF_INET;  
                addrRTP.sin_port   = htons(serverport);
                their_addr.sin_addr = *((struct in_addr *)he->h_addr);
                len = sizeof(addrRTP);
                int ts = 0;
                int sn = 0;
                int ssrc = 0;
                int init = 0;
                int u, v, w;
                while(!stop && (i = recvfrom(sockudp, buf, 1526, 0,
                (struct sockaddr*) &addrRTP, &len)) > 0)
                {
#ifndef WINDOWSVERSION
                  alarm(ALARMTIME);
#endif
                  if(i >= 12+1 && (unsigned char)buf[0] == (2 << 6) && buf[1] == 0x60)
                  {
                    u= ((unsigned char)buf[2]<<8)+(unsigned char)buf[3];
                    v = ((unsigned char)buf[4]<<24)+((unsigned char)buf[5]<<16)
                    +((unsigned char)buf[6]<<8)+(unsigned char)buf[7];
                    w = ((unsigned char)buf[8]<<24)+((unsigned char)buf[9]<<16)
                    +((unsigned char)buf[10]<<8)+(unsigned char)buf[11];

                    if(init)
                    {
                      if(u < -30000 && sn > 30000) sn -= 0xFFFF;
                      if(ssrc != w || ts > v)
                      {
                        fprintf(stderr, "Illegal UDP data received.\n");
                        exit(1);
                      }
                      if(u > sn) /* don't show out-of-order packets */
                        fwrite(buf+12, (size_t)i-12, 1, stdout);
                    }
                    sn = u; ts = v; ssrc = w; init = 1;
                  }
                  else
                  {
                    fprintf(stderr, "Illegal UDP header.\n");
                    exit(1);
                  }
                }
              }
              i = snprintf(buf, MAXDATASIZE,
              "TEARDOWN rtsp://%s%s%s/%s RTSP/1.0\r\n"	        
              "CSeq: %d\r\n"
              "Session: %d\r\n"
              "\r\n", 
              args.server, proxyserver ? ":" : "", proxyserver ? args.port : "",
              args.data, cseq++, session);

              if(i > MAXDATASIZE || i < 0) /* second check for old glibc */
              {
                fprintf(stderr, "Requested data too long\n");
                exit(1);
              }
              if(send(sockfd, buf, (size_t)i, 0) != i)
              {
                perror("send");
                exit(1);
              }
            }
            else
            {
              fprintf(stderr, "Could not start data stream.\n");
              exit(1);
            }
          }
          else
          {
            fprintf(stderr, "Could not setup initial control connection.\n");
            exit(1);
          }
        }
        else
        {
          perror("recv");
          exit(1);
        }
      }
      else
      {
        if(connect(sockfd, (struct sockaddr *)&their_addr,
        sizeof(struct sockaddr)) == -1)
        {
          perror("connect");
          exit(1);
        }
        if(!args.data)
        {
          i = snprintf(buf, MAXDATASIZE,
          "GET %s%s%s%s/ HTTP/1.0\r\n"
          "Host: %s\r\n%s"
          "User-Agent: %s/%s\r\n"
          "\r\n"
          , proxyserver ? "http://" : "", proxyserver ? proxyserver : "",
          proxyserver ? ":" : "", proxyserver ? proxyport : "",
          args.server, args.mode == NTRIP1 ? "" : "Ntrip-Version: Ntrip/2.0\r\n",
          AGENTSTRING, revisionstr);
        }
        else
        {
          i=snprintf(buf, MAXDATASIZE-40, /* leave some space for login */
          "GET %s%s%s%s/%s HTTP/1.0\r\n"
          "Host: %s\r\n%s"
          "User-Agent: %s/%s\r\n"
          "Authorization: Basic "
          , proxyserver ? "http://" : "", proxyserver ? proxyserver : "",
          proxyserver ? ":" : "", proxyserver ? proxyport : "",
          args.data, args.server,
          args.mode == NTRIP1 ? "" : "Ntrip-Version: Ntrip/2.0\r\n",
          AGENTSTRING, revisionstr);
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
            if(j >= 0 && j < MAXDATASIZE-i)
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
        if(args.data && *args.data != '%')
        {
          int k = 0;
          int chunkymode = 0;
          int starttime = time(0);
          int lastout = starttime;
          int totalbytes = 0;
          int chunksize = 0;

          while(!stop && (numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) > 0)
          {
#ifndef WINDOWSVERSION
            alarm(ALARMTIME);
#endif
            if(!k)
            {
              if(numbytes > 17 && (!strncmp(buf, "HTTP/1.1 200 OK\r\n", 17)
              || !strncmp(buf, "HTTP/1.0 200 OK\r\n", 17)))
              {
                const char *datacheck = "Content-Type: gnss/data\r\n";
                const char *chunkycheck = "Transfer-Encoding: chunked\r\n";
                int l = strlen(datacheck)-1;
                int j=0;
                for(i = 0; j != l && i < numbytes-l; ++i)
                {
                  for(j = 0; j < l && buf[i+j] == datacheck[j]; ++j)
                    ;
                }
                if(i == numbytes-l)
                {
                  fprintf(stderr, "No 'Content-Type: gnss/data' found\n");
                  exit(1);
                }
                l = strlen(chunkycheck)-1;
                j=0;
                for(i = 0; j != l && i < numbytes-l; ++i)
                {
                  for(j = 0; j < l && buf[i+j] == chunkycheck[j]; ++j)
                    ;
                }
                if(i < numbytes-l)
                  chunkymode = 1;
	      }
              else if(numbytes < 12 || strncmp("ICY 200 OK\r\n", buf, 12))
              {
                fprintf(stderr, "Could not get the requested data: ");
                for(k = 0; k < numbytes && buf[k] != '\n' && buf[k] != '\r'; ++k)
                {
                  fprintf(stderr, "%c", isprint(buf[k]) ? buf[k] : '.');
                }
                fprintf(stderr, "\n");
                exit(1);
              }
              else if(args.mode != NTRIP1)
              {
                fprintf(stderr, "NTRIP version 2 HTTP connection failed%s.\n",
                args.mode == AUTO ? ", falling back to NTRIP1" : "");
                if(args.mode == HTTP)
                  exit(1);
              }
              ++k;
            }
            else
            {
              sleeptime = 0;
              if(chunkymode)
              {
                int stop = 0;
                int pos = 0;
                while(!stop && pos < numbytes)
                {
                  switch(chunkymode)
                  {
                  case 1: /* reading number starts */
                    chunksize = 0;
                    ++chunkymode; /* no break */
                  case 2: /* during reading number */
                    i = buf[pos++];
                    if(i >= '0' && i <= '9') chunksize = chunksize*16+i-'0';
                    else if(i >= 'a' && i <= 'f') chunksize = chunksize*16+i-'a'+10;
                    else if(i >= 'A' && i <= 'F') chunksize = chunksize*16+i-'A'+10;
                    else if(i == '\r') ++chunkymode;
                    else stop = 1;
                    break;
                  case 3: /* scanning for return */
                    if(buf[pos++] == '\n') chunkymode = chunksize ? 4 : 1;
                    else stop = 1;
                    break;
                  case 4: /* output data */
                    i = numbytes-pos;
                    if(i > chunksize) i = chunksize;
                    fwrite(buf+pos, (size_t)i, 1, stdout);
                    totalbytes += i;
                    chunksize -= i;
                    pos += i;
                    if(!chunksize)
                      chunkymode = 1;
                    break;
                  }
                }
                if(stop)
                {
                  fprintf(stderr, "Error in chunky transfer encoding\n");
                  break;
                }
              }
              else
              {
                totalbytes += numbytes;
                fwrite(buf, (size_t)numbytes, 1, stdout);
              }
              fflush(stdout);
              if(totalbytes < 0) /* overflow */
              {
                totalbytes = 0;
                starttime = time(0);
                lastout = starttime;
              }
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
          sleeptime = 0;
          while(!stop && (numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) > 0)
          {
#ifndef WINDOWSVERSION
            alarm(ALARMTIME);
#endif
            fwrite(buf, (size_t)numbytes, 1, stdout);
          }
        }
        closesocket(sockfd);
      }
    } while(args.data && *args.data != '%' && !stop);
  }
  return 0;
}
