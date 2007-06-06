----------------------------------------------------------------------
                NTRIP Client for Linux/Unix Version
----------------------------------------------------------------------

Easy example NTRIP client for Linux/Unix.
Copyright (C) 2003-2007 by Dirk Stoecker <soft@dstoecker.de>
    
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

Files in NtripLinuxClient.zip
-----------------------------
NtripLinuxClient.c:          Ntrip Linux/UNIX client source code
ReadmeLinuxClient.txt:       Readme file for 'NtripLinuxClient'
StartNtripLinuxClient:       Shell script to start 'NtripLinuxClient'
makefile:                    Easy makefile to build source

Ntrip
-----
The NtripLinuxClient is an HTTP client based on 'Networked Transport
of RTCM via Internet Protocol' (Ntrip). This is an application-level 
protocol streaming Global Navigation Satellite System (GNSS) data over 
the Internet. Ntrip is a generic, stateless protocol based on the 
Hypertext Transfer Protocol HTTP/1.1. The HTTP objects are enhanced 
to GNSS data streams.

Ntrip is designed for disseminating differential correction data 
(e.g in the RTCM-104 format) or other kinds of GNSS streaming data to
stationary or mobile users over the Internet, allowing simultaneous PC,
Laptop, PDA, or receiver connections to a broadcasting host. Ntrip 
supports Wireless Internet access through Mobile IP Networks like GSM, 
GPRS, EDGE, or UMTS.

Ntrip is implemented in three system software components: NtripClients, 
NtripServers and NtripCasters. The NtripCaster is the actual HTTP 
server program whereas NtripClient and NtripServer are acting as HTTP 
clients.

NtripLinuxClient
----------------
This Linux/UNIX NtripClient program is written under GNU General Public 
License in C programming language. The program reads data from an Ntrip 
Broadcaster and writes on standard output for further redirection 
of data to a file or COM-port. PLEASE NOTE THAT THIS PROGRAM VERSION
DOES NOT HANDLE POTENTIALLY OCCURRING INTERRUPTIONS OF COMMUNICATION
OR NETWORK CONGESTION SITUATIONS. Its distribution may stimulate
those intending to write their own client program.

Call the program with following arguments:

NtripLinuxClient -d mountpoint
                 -s Ntrip Broadcaster IP adress         
                 -p password
                 -r Ntrip Broadcaster Port number
                 -u username
		 -n NMEA string for sending to server
		 -b output bitrate

or using an URL:
./NtripLinuxClient ntrip:mountpoint[/username[:password]][@server[:port]][;nmea]
		 

The argument '-h' will cause a HELP on the screen.
Without any argument NtripLinuxClient will provide the a table of
available resources (source table).

Compilation/Installation
------------------------
Please unzip the archive and copy its contents into an appropriate
directory. Compile the source code under Linux through entering e.g.
the command 'cc -o NtripLinuxClient NtripLinuxClient.c'.

Registration
------------
Some of the data streams (mountpoints) from an NtripCaster may be
available for test, demonstration, and evaluation purposes and
accessible without authentication/authorization. For accessing other
data streams (mountpoints) the user needs a user-ID and a
user password. Authorization can be provided for a single stream,
for a group of streams (network) or for all available streams.
Currently, registration can be requested via the registration form
on http://igs.ifag.de/ntrip/ntrip_register.htm.

Ntrip Broadaster Address and Port
---------------------------------
The current Internet address of the Ntrip Broadcaster is
www.euref-ip.net. The port number is 80.

Disclaimer
----------
Note that this NtripLinuxClient program is for experimental use
only. The BKG disclaims any liability nor responsibility to any 
person or entity with respect to any loss or damage caused, or alleged 
to be caused, directly or indirectly by the use and application of the 
Ntrip technology.

Further Information
-------------------
http://igs.ifag.de/index_ntrip.htm
euref-ip@bkg.bund.de
