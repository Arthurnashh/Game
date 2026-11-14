// IPA.cpp: implementation of the CIPA class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "IPA.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define COLON ':'

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static ADRTYPE GetHostByName(const char *adrc)
{
  ADRTYPE adr;
  adr=inet_addr(adrc);
  if (adr==INADDR_NONE)
  {
    HOSTENT *phe=gethostbyname(adrc);
    if (phe==NULL) return INADDR_NONE;
    memcpy(&adr,phe->h_addr_list[0],sizeof(long));
  }
  return adr;
}

static ADRTYPE DecodeAdr(const char *adr)
{
//  char *c=strchr(adr,COLON);
  const char* c = const_cast<char* >(strchr(adr,COLON));
  if (c==NULL) return GetHostByName(adr);
  char *d=(char *)alloca(c-adr+1);
  memcpy(d,adr,c-adr);
  d[c-adr]=0;
  return GetHostByName(d);
}

static PORTTYPE DecodePort(const char *adr, PORTTYPE defport)
{
//  char *c=strchr(adr,COLON);
  const char *c = const_cast<char* >(strchr(adr,COLON));
  if (c==NULL) return defport;
  c++;
  PORTTYPE res=defport;
  sscanf(c,"%hd",&res);
  return res;
}

CIPA::CIPA(const char *adr, PORTTYPE p): addr(DecodeAdr(adr)), port(DecodePort(adr,p))
{}

bool CIPA::GetHostName(char *buffer, int size)const
{
  if (!IsAddrValid())
  {
    strncpy(buffer,"???.???.???.???",size);
    buffer[size-1]=0;
    return true;
  }
  HOSTENT *phe=gethostbyaddr((char *)&addr,sizeof(addr),AF_INET);
  char *c;
  if (phe==NULL) 
  {	
    struct in_addr a;
    a.S_un.S_addr=addr;
    c=inet_ntoa(a);
  }
  else c=phe->h_name;
  if ((int)strlen(c)+1>size) return false;
  strcpy(buffer,c);
  return true;
}

CIPA::operator LPSOCKADDR() const
{
  static SOCKADDR_IN sin;
  memset(&sin,0,sizeof(sin));
  sin.sin_family=AF_INET;
  sin.sin_port=htons(port);
  sin.sin_addr.s_addr=addr;
  return (SOCKADDR *)&sin;
}

void CIPA::GetSockAddres(SOCKADDR_IN &sin) const
{
  memset(&sin,0,sizeof(sin));
  sin.sin_family=AF_INET;
  sin.sin_port=htons(port);
  sin.sin_addr.s_addr=addr;
}

void CIPA::GetSockAddres(SOCKADDR& si) const
{
  SOCKADDR_IN *sin=(SOCKADDR_IN *)(&si);
  memset(sin,0,sizeof(*sin));
  sin->sin_family=AF_INET;
  sin->sin_port=htons(port);
  sin->sin_addr.s_addr=addr;
}

bool CIPA::operator== (const CIPA& other) const
{
  return other.addr==addr && other.port==port;
}

CIPA& CIPA::operator= (const CIPA& other)
{
  addr=other.addr;
  port=other.port;
  return *this;
}

bool CIPA::GetFullAdrSpec(char *buffer, int size)const
{
  char text[10];
  if (GetHostName(buffer,size)==false) return false;
  size-=strlen(buffer);
  sprintf(text,":%d",port);
  if ((int)strlen(text)+1>size) return false;
  strcat(buffer,text);
  return true;
}

