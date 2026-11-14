#include <malloc.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include ".\pipeserver.h"

#pragma warning (push)
#pragma warning (disable: 4355)


PipeServer::PipeServer(void):StreamParser<char,PipeServer>(this,false)
{
  if (GetStdHandle(STD_INPUT_HANDLE)==NULL)
  {
    AllocConsole();
    _console=true;
  }
  else
    _console=false;
  _input=GetStdHandle(STD_INPUT_HANDLE);
  _output=GetStdHandle(STD_OUTPUT_HANDLE);
  _error=false;
}

PipeServer::~PipeServer(void)
{
  if (_console) 
  {
    FreeConsole();
    CloseHandle(_input);
    CloseHandle(_output);
    SetStdHandle(STD_INPUT_HANDLE,NULL);
    SetStdHandle(STD_OUTPUT_HANDLE,NULL);
  }
}


bool PipeServer::ReadStream(char *array, size_t toread, size_t &wasread)
{
  if (_error) return false;
  wasread=0;  
  ReadFile(_input,array,toread,reinterpret_cast<DWORD *>(&wasread),NULL);
  return true;
}

void PipeServer::SendOK(const char *message)
{
  SendMessage("+Ok ",message?message:"","\r\n");
}
void PipeServer::SendError(const char *reason)
{
  SendMessage("-Err ",reason?reason:"","\r\n");
}

void PipeServer::SendMessage(const char *prefix, const char *message, const char *postfix)
{
  if (_error) return;
  size_t sz1=strlen(prefix);
  size_t sz2=strlen(message);
  size_t sz3=strlen(postfix);
  size_t total=sz1+sz2+sz3;
  char *buffer=(char *)alloca(total+1);
  memcpy(buffer,prefix,sz1);
  memcpy(buffer+sz1,message,sz2);
  memcpy(buffer+sz1+sz2,postfix,sz3);
  buffer[total]=0;
  DWORD processed;
  WriteFile(_output,buffer,total,&processed,NULL);
  _error=processed!=total;
}

void PipeServer::SendLine(const char *line)
{
  SendMessage("",line,"\r\n");
}

void PipeServer::WriteStream(const void *data, size_t size)
{
  if (_error) return;
  DWORD processed;
  WriteFile(_output,data,size,&processed,NULL);
  _error=processed!=size;
}

bool PipeServer::TestCommand(const char *command)
{
  char *buff=(char *)alloca(strlen(command)+20);
  sprintf(buff,"^%s\r\n",command);
  return TestI(buff);
}

void PipeServer::SendFormatted(const char *format, ...)
{
  va_list args;
  va_start(args,format);
  char smallBuff[256];
  if (_vsnprintf(smallBuff,256,format,args)==-1)
  {
    char *bigbuff=new char[65536];
    _vsnprintf(smallBuff,65536,format,args);
    bigbuff[65535]=0;
    WriteStream(bigbuff,strlen(bigbuff));
    delete bigbuff;
  } 
  else
    WriteStream(smallBuff,strlen(smallBuff));
}

#pragma warning (pop)
