/*
  Serial port access for NTRIP client for POSIX.
  $Id$
  Copyright (C) 2008 by Dirk Stöcker <soft@dstoecker.de>

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

/* system includes */
#include <stdio.h>
#include <string.h>

#ifndef WINDOWSVERSION
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#define SERIALDEFAULTDEVICE "/dev/ttyS0"
enum SerialBaud {
  SPABAUD_50 = B50, SPABAUD_110 = B110, SPABAUD_300 = B300, SPABAUD_600 = B600,
  SPABAUD_1200 = B1200, SPABAUD_2400 = B2400, SPABAUD_4800 = B4800,
  SPABAUD_9600 = B9600, SPABAUD_19200 = B19200,
  SPABAUD_38400 = B38400, SPABAUD_57600 = B57600, SPABAUD_115200 = B115200 };
enum SerialDatabits {
  SPADATABITS_5 = CS5, SPADATABITS_6 = CS6, SPADATABITS_7 = CS7, SPADATABITS_8 = CS8 };
enum SerialStopbits {
  SPASTOPBITS_1 = 0, SPASTOPBITS_2 = CSTOPB };
enum SerialParity {
  SPAPARITY_NONE = 0, SPAPARITY_ODD = PARODD | PARENB, SPAPARITY_EVEN = PARENB };
enum SerialProtocol {
  SPAPROTOCOL_NONE = 0, SPAPROTOCOL_RTS_CTS = 9999,
  SPAPROTOCOL_XON_XOFF = IXOFF | IXON };

struct serial
{
  struct termios Termios;
  int            Stream;
};
#else /* WINDOWSVERSION */
#include <windows.h>
#define SERIALDEFAULTDEVICE "COM1"
enum SerialBaud {
  SPABAUD_50 = 50, SPABAUD_110 = 110, SPABAUD_300 = 300, SPABAUD_600 = 600,
  SPABAUD_1200 = 1200, SPABAUD_2400 = 2400, SPABAUD_4800 = 4800,
  SPABAUD_9600 = 9600, SPABAUD_19200 = 19200,
  SPABAUD_38400 = 38400, SPABAUD_57600 = 57600, SPABAUD_115200 = 115200 };
enum SerialDatabits {
  SPADATABITS_5 = 5, SPADATABITS_6 = 6, SPADATABITS_7 = 7, SPADATABITS_8 = 8 };
enum SerialStopbits {
  SPASTOPBITS_1 = 1, SPASTOPBITS_2 = 2 };
enum SerialParity {
  SPAPARITY_NONE = 'N', SPAPARITY_ODD = 'O', SPAPARITY_EVEN = 'E'};
enum SerialProtocol {
  SPAPROTOCOL_NONE, SPAPROTOCOL_RTS_CTS, SPAPROTOCOL_XON_XOFF};

struct serial
{
  DCB    Termios;
  HANDLE Stream;
};
#if !defined(__GNUC__)
int strncasecmp(const char *a, const char *b, int len)
{
  while(len-- && *a && tolower(*a) == tolower(*b))
  {
    ++a; ++b;
  }
  return len ? (tolower(*a) - tolower(*b)) : 0;
}
#endif /* !__GNUC__ */

#endif /* WINDOWSVERSION */

static enum SerialParity SerialGetParity(const char *buf, int *ressize)
{
  int r = 0;
  enum SerialParity p = SPAPARITY_NONE;
  if(!strncasecmp(buf, "none", 4))
  { r = 4; p = SPAPARITY_NONE; }
  else if(!strncasecmp(buf, "no", 2))
  { r = 2; p = SPAPARITY_NONE; }
  else if(!strncasecmp(buf, "odd", 3))
  { r = 3; p = SPAPARITY_ODD; }
  else if(!strncasecmp(buf, "even", 4))
  { r = 4; p = SPAPARITY_EVEN; }
  else if(*buf == 'N' || *buf == 'n')
  { r = 1; p = SPAPARITY_NONE; }
  else if(*buf == 'O' || *buf == 'o')
  { r = 1; p = SPAPARITY_ODD; }
  else if(*buf == 'E' || *buf == 'e')
  { r = 1; p = SPAPARITY_EVEN; }
  if(ressize) *ressize = r;
  return p;
}

static enum SerialProtocol SerialGetProtocol(const char *buf, int *ressize)
{
  int r = 0;
  enum SerialProtocol Protocol = SPAPROTOCOL_NONE;
  /* try some possible forms for input, be as gentle as possible */
  if(!strncasecmp("xonxoff",buf,7)){r = 7; Protocol=SPAPROTOCOL_XON_XOFF;}
  else if(!strncasecmp("xon_xoff",buf,8)){r = 8; Protocol=SPAPROTOCOL_XON_XOFF;}
  else if(!strncasecmp("xon-xoff",buf,8)){r = 8; Protocol=SPAPROTOCOL_XON_XOFF;}
  else if(!strncasecmp("xon xoff",buf,8)){r = 8; Protocol=SPAPROTOCOL_XON_XOFF;}
  else if(!strncasecmp("xoff",buf,4)){r = 4; Protocol=SPAPROTOCOL_XON_XOFF;}
  else if(!strncasecmp("xon",buf,3)){r = 3; Protocol=SPAPROTOCOL_XON_XOFF;}
  else if(*buf == 'x' || *buf == 'X'){r = 1; Protocol=SPAPROTOCOL_XON_XOFF;}
  else if(!strncasecmp("rtscts",buf,6)){r = 6; Protocol=SPAPROTOCOL_RTS_CTS;}
  else if(!strncasecmp("rts_cts",buf,7)){r = 7; Protocol=SPAPROTOCOL_RTS_CTS;}
  else if(!strncasecmp("rts-cts",buf,7)){r = 7; Protocol=SPAPROTOCOL_RTS_CTS;}
  else if(!strncasecmp("rts cts",buf,7)){r = 7; Protocol=SPAPROTOCOL_RTS_CTS;}
  else if(!strncasecmp("rts",buf,3)){r = 3; Protocol=SPAPROTOCOL_RTS_CTS;}
  else if(!strncasecmp("cts",buf,3)){r = 3; Protocol=SPAPROTOCOL_RTS_CTS;}
  else if(*buf == 'r' || *buf == 'R' || *buf == 'c' || *buf == 'C')
  {r = 1; Protocol=SPAPROTOCOL_RTS_CTS;}
  else if(!strncasecmp("none",buf,4)){r = 4; Protocol=SPAPROTOCOL_NONE;}
  else if(!strncasecmp("no",buf,2)){r = 2; Protocol=SPAPROTOCOL_NONE;}
  else if(*buf == 'n' || *buf == 'N'){r = 1; Protocol=SPAPROTOCOL_NONE;}
  if(ressize) *ressize = r;
  return Protocol;
}

#ifndef WINDOWSVERSION
static void SerialFree(struct serial *sn)
{
  if(sn->Stream)
  {
    /* reset old settings */
    tcsetattr(sn->Stream, TCSANOW, &sn->Termios);
    close(sn->Stream);
    sn->Stream = 0;
  }
}

static const char * SerialInit(struct serial *sn,
const char *Device, enum SerialBaud Baud, enum SerialStopbits StopBits,
enum SerialProtocol Protocol, enum SerialParity Parity,
enum SerialDatabits DataBits, int dowrite
#ifdef __GNUC__
__attribute__((__unused__))
#endif /* __GNUC__ */
)
{
  struct termios newtermios;

  if((sn->Stream = open(Device, O_RDWR | O_NOCTTY | O_NONBLOCK)) <= 0)
    return "could not open serial port";
  tcgetattr(sn->Stream, &sn->Termios);

  memset(&newtermios, 0, sizeof(struct termios));
  newtermios.c_cflag = Baud | StopBits | Parity | DataBits
  | CLOCAL | CREAD;
  if(Protocol == SPAPROTOCOL_RTS_CTS)
    newtermios.c_cflag |= CRTSCTS;
  else
    newtermios.c_cflag |= Protocol;
  newtermios.c_cc[VMIN] = 1;
  tcflush(sn->Stream, TCIOFLUSH);
  tcsetattr(sn->Stream, TCSANOW, &newtermios);
  tcflush(sn->Stream, TCIOFLUSH);
  fcntl(sn->Stream, F_SETFL, O_NONBLOCK);
  return 0;
}

static int SerialRead(struct serial *sn, char *buffer, size_t size)
{
  int j = read(sn->Stream, buffer, size);
  if(j < 0)
  {
    if(errno == EAGAIN)
      return 0;
    else
      return j;
  }
  return j;
}

static int SerialWrite(struct serial *sn, const char *buffer, size_t size)
{
  int j = write(sn->Stream, buffer, size);
  if(j < 0)
  {
    if(errno == EAGAIN)
      return 0;
    else
      return j;
  }
  return j;
}

#else /* WINDOWSVERSION */
static void SerialFree(struct serial *sn)
{
  if(sn->Stream)
  {
    SetCommState(sn->Stream, &sn->Termios);
    /* reset old settings */
    CloseHandle(sn->Stream);
    sn->Stream = 0;
  }
}

static const char * SerialInit(struct serial *sn, const char *Device,
enum SerialBaud Baud, enum SerialStopbits StopBits,
enum SerialProtocol Protocol, enum SerialParity Parity,
enum SerialDatabits DataBits, int dowrite)
{
  char mydevice[50];
  if(Device[0] != '\\')
    snprintf(mydevice, sizeof(mydevice), "\\\\.\\%s", Device);
  else
    mydevice[0] = 0;

  if((sn->Stream = CreateFile(mydevice[0] ? mydevice : Device,
  dowrite ? GENERIC_WRITE|GENERIC_READ : GENERIC_READ, 0, 0, OPEN_EXISTING,
  0, 0)) == INVALID_HANDLE_VALUE)
    return "could not create file";

  memset(&sn->Termios, 0, sizeof(sn->Termios));
  GetCommState(sn->Stream, &sn->Termios);

  DCB dcb;
  memset(&dcb, 0, sizeof(dcb));
  char str[100];
  snprintf(str,sizeof(str),
  "baud=%d parity=%c data=%d stop=%d xon=%s octs=%s rts=%s",
  Baud, Parity, DataBits, StopBits,
  Protocol == SPAPROTOCOL_XON_XOFF ? "on" : "off",
  Protocol == SPAPROTOCOL_RTS_CTS ? "on" : "off",
  Protocol == SPAPROTOCOL_RTS_CTS ? "on" : "off");
#ifdef DEBUG
  fprintf(stderr, "COM Settings: %s\n", str);
#endif

  COMMTIMEOUTS ct = {MAXDWORD, 0, 0, 0, 0};

  if(!BuildCommDCB(str, &dcb))
  {
    CloseHandle(sn->Stream);
    return "creating device parameters failed";
  }
  else if(!SetCommState(sn->Stream, &dcb))
  {
    CloseHandle(sn->Stream);
    return "setting device parameters failed";
  }
  else if(!SetCommTimeouts(sn->Stream, &ct))
  {
    CloseHandle(sn->Stream);
    return "setting timeouts failed";
  }

  return 0;
}

static int SerialRead(struct serial *sn, char *buffer, size_t size)
{
  DWORD j = 0;
  if(!ReadFile(sn->Stream, buffer, size, &j, 0))
    return -1;
  return j;
}

static int SerialWrite(struct serial *sn, const char *buffer, size_t size)
{
  DWORD j = 0;
  if(!WriteFile(sn->Stream, buffer, size, &j, 0))
    return -1;
  return j;
}
#endif /* WINDOWSVERSION */
