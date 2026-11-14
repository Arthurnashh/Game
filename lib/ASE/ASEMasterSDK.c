/* ASEMasterSDK - functions for reporting a server to the All-Seeing Eye */
/* master server                                                         */
/*                                                                       */
/* Requires standard C library and Berkeley sockets. Should compile on   */
/* Windows and most unixes as is.                                        */
/*                                                                       */
/* Copyright 2002 UDP Soft Ltd.   http://www.udpsoft.com/                */
/*                                                                       */
/* Not for public distribution!                                          */

#ifdef WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/ioctl.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif
#endif

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#define MASTERREPORTINTERVALMIN 5
#define MASTERREPORTINTERVALMAX 5 * 60

static struct sockaddr_in masteraddr;
static volatile int initialized = 0;
static sock = INVALID_SOCKET;
static int masterreportinterval = MASTERREPORTINTERVALMIN;

static unsigned long resolve(char *name)
{
	struct hostent *hep;
	if (hep = gethostbyname(name))
		return *(unsigned int *)*hep->h_addr_list;
	else return INADDR_NONE;
}

/* int ASEMaster_init(int createsocket)                           */
/*                                                                */
/* Initializes the master reporting functions. Does a DNS lookup, */
/* call only when net already up and running, preferably from an  */
/* asynchronous thread to avoid stalls.                           */
/*                                                                */
/* createsocket = flag indicating whether we should create a new  */
/*                socket for master packets (!=0) or if we can    */
/*                use one provided by the game (==0)              */
/*                                                                */
/* returns non-zero for success                                   */

int ASEMaster_init(int createsocket)
{
#ifdef WIN32
        WSADATA WSAData;
        WSAStartup(MAKEWORD(1,1), &WSAData);
#endif
        
	memset(&masteraddr, 0, sizeof(masteraddr));
	masteraddr.sin_family = AF_INET;
	masteraddr.sin_port = htons(27900);
	initialized = (masteraddr.sin_addr.s_addr = resolve("master.udpsoft.com")) != INADDR_NONE;

	if (!initialized) return 0;

	if (createsocket)
	{
		if (sock == INVALID_SOCKET)
			sock = socket(AF_INET, SOCK_DGRAM, 0);
	
		if (sock == INVALID_SOCKET) initialized = 0;
	}

	masterreportinterval = MASTERREPORTINTERVALMIN;

	return initialized;
}

static time_t nextmastertime = 0;

/* int ASEMaster_heartbeat(SOCKET s, int port, char *gamename)    */
/*                                                                */
/* Checks if it's time to send another heartbeat to the ASE       */
/* master server. Should be called at least once every few        */
/* minutes, can be called more often, for example every frame.    */
/*                                                                */
/* s = UDP socket to use when sending packets (if createsocket    */
/*     was 0 when calling init, otherwise ignored)                */
/*                                                                */
/* port = query port for the server                               */
/*                                                                */
/* gamename = unique identifier for the game                      */
/*                                                                */
/* return value:  >0 == heartbeat was sent                        */
/*               ==0 == too soon to send another heartbeat        */
/*                <0 == sendto error code                         */
/*              -999 == not initialized                           */
/*                                                                */

int ASEMaster_heartbeat(SOCKET s, int port, char *gamename)
{
	time_t currenttime;
	if (!initialized || (s == INVALID_SOCKET && sock == INVALID_SOCKET))
		return -999;

	currenttime = time(NULL);
	if (currenttime >= nextmastertime)
	{
		char buf[128];
		int len = sprintf(buf, "\\heartbeat\\%d\\gamename\\%s\\", port, gamename);
		
		if (sock != INVALID_SOCKET) s = sock;
		
		nextmastertime = currenttime + masterreportinterval;
		if (masterreportinterval < MASTERREPORTINTERVALMAX)
			masterreportinterval <<= 1;
		return sendto(s, buf, len, 0, (struct sockaddr *)&masteraddr, sizeof(masteraddr));
	}
	return 0;
}

/* test program
void main(void)
{
	ASEMaster_init(1);
        while (1)
        {
                Sleep(1000);
                ASEMaster_heartbeat(0, 23000, "bfield1942");
        }
}*/
