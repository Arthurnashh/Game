#include <Es/essencepch.hpp>
#include <Es/Common/win.h>

#include "visviewerpipe.h"

VisViewerPipe::VisViewerPipe(IVisViewerExchangeDocView &event):_send(0),_receive(0),VisViewerExchange(event)
{
  _outbuff=0;  
  _inbufsz=0;
  _inbufptr=0;
  _inbuffer=0;
  _inbufmax=0;
}

VisViewerPipe::~VisViewerPipe(void)
{
  if (_outbuff) delete [] _outbuff;
  if (_inbuffer) delete [] _inbuffer;
}

void VisViewerPipe::Create(HANDLE send, HANDLE receive)
{
  _send=send;
  _receive=receive;
}

struct FindWindowByNameStruct
{
  HWND found;
  const char *name;
  FindWindowByNameStruct(const char *name):found(0),name(name) {}
};

static BOOL CALLBACK FindWindowByNameEnumChld(HWND hWnd, LPARAM context)
{
  FindWindowByNameStruct *str=(FindWindowByNameStruct *)context;
  char buffer[256];
  GetWindowText(hWnd,buffer,256);
  if (strcmp(buffer,str->name)==0)
  {
    GetClassName(hWnd,buffer,256);
    if (stricmp(buffer,"STATIC")==0)
    {
      str->found=hWnd;
      return FALSE;
    }
  }
  return TRUE;
}
static BOOL CALLBACK FindWindowByNameEnum(HWND hWnd, LPARAM context)
{
  FindWindowByNameStruct *str=(FindWindowByNameStruct *)context;
  EnumChildWindows(hWnd,FindWindowByNameEnumChld,context);
  return str->found==0;
}

static HWND FindWindowByName(const char *params)
{
  FindWindowByNameStruct fndInfo(params);
  EnumWindows(FindWindowByNameEnum,(LPARAM)&fndInfo);
  return fndInfo.found;
}

bool VisViewerPipe::Create(const char *params)
{
  HWND window;
  unsigned int message;
  if (sscanf(params,"%X,%X",&window,&message)!=2)
  {
    window=FindWindowByName(params);
    if (window==0) return false;
    // acquire message from the window
    // this is WM_INCOME_PIPECONNECTION for Visitor
    message=GetDlgCtrlID(window);
    if (message==0) return false;
    window=GetParent(window);
  }
  HANDLE sendLocal=0,sendRemote=0,receiveLocal=0,receiveRemote=0;
  ///Ask Visitor for buffer size (LPARAM and WPARAM==0)
  DWORD buffer=::SendMessage(window,message,0,0);
  if (CreatePipe(&sendRemote,&sendLocal,0,buffer) && CreatePipe(&receiveLocal,&receiveRemote,0,buffer))
  {
    DWORD procId;
    if (GetWindowThreadProcessId(window,&procId))
    {
      HANDLE proc=OpenProcess(PROCESS_DUP_HANDLE,FALSE,procId);
      if (proc!=0)
      {        
        if (DuplicateHandle(GetCurrentProcess(),sendRemote,proc,&sendRemote,0,FALSE,DUPLICATE_CLOSE_SOURCE|DUPLICATE_SAME_ACCESS))
        {
          if (DuplicateHandle(GetCurrentProcess(),receiveRemote,proc,&receiveRemote,0,FALSE,DUPLICATE_CLOSE_SOURCE|DUPLICATE_SAME_ACCESS))
          {
            ///send handles to Visitor
            if (::SendMessage(window,message,(WPARAM)sendRemote,(WPARAM)receiveRemote))
            {
              CloseHandle(proc);
              Create(sendLocal,receiveLocal);
              

              return TRUE;
            }
            DuplicateHandle(proc,sendRemote,GetCurrentProcess(),&sendRemote,0,FALSE,DUPLICATE_CLOSE_SOURCE|DUPLICATE_SAME_ACCESS);
            DuplicateHandle(proc,receiveRemote,GetCurrentProcess(),&receiveRemote,0,FALSE,DUPLICATE_CLOSE_SOURCE|DUPLICATE_SAME_ACCESS);            
          }
        }
      CloseHandle(proc);
      }
    }
  }
  if (sendRemote) CloseHandle(sendRemote);
  if (sendLocal) CloseHandle(sendLocal);
  if (receiveRemote) CloseHandle(receiveRemote);
  if (receiveLocal) CloseHandle(receiveLocal);
  return FALSE;
}

#if ALLOC_DEBUGGER
#define AllocatorAlloc(type,sz,a) reinterpret_cast<type *>(a.Alloc(sz,__FILE__,__LINE__,""))
#else
#define AllocatorAlloc(type,sz,a) reinterpret_cast<type *>(a.Alloc(sz))
#endif

VisViewerExchange::Status VisViewerPipe::SendMessage(const SPosMessage &sMsg)
{
  MemAllocLocal<char,256> Allocator;

  size_t msgSize = sizeof(sMsg._iMsgType) + sMsg.HeaderSize() + sMsg.DataSize();
  size_t tmpSize = msgSize;
  unsigned char * pMsg = AllocatorAlloc(unsigned char,tmpSize,Allocator);

  *(int *) pMsg = sMsg._iMsgType;
  sMsg.WriteHeader(pMsg + sizeof(sMsg._iMsgType));
  sMsg.WriteData(pMsg + sizeof(sMsg._iMsgType) + sMsg.HeaderSize());

  DWORD written;
  Status ret=statOk;
  if (_outbuff)
  {
    if (_bufused+msgSize>_bufsize) 
      ret=StartBuffer(_bufsize);
    if (ret!=statOk) return ret;
    if (_bufused+msgSize<=_bufsize)
    {
      memcpy(_outbuff+_bufused,pMsg,msgSize);
      _bufused+=msgSize;
      return statOk;
    }    
  }
  if (WriteFile(_send,pMsg,msgSize,&written,0)==FALSE)
  {
    if (GetLastError()==ERROR_BROKEN_PIPE) ret=statLost;
    else ret=statError;
  }
  if (written!=msgSize) ret=statError;
  Allocator.Free(pMsg,msgSize);

  return ret;
}

VisViewerExchange::Status VisViewerPipe::ReceiveMessage(PSPosMessage &pMsg)
{


  Status res=PeekPipe();
  if (res!=statOk) return res;



 //Read message type
  int msgType; 

  res=ReadPipe(&msgType,sizeof(msgType));
  if (res!=statOk) return res;

  //Allocate message by type
  pMsg = AllocPosMessage(msgType);
  if (pMsg == NULL) return statError;

  //Set Message type
  pMsg->_iMsgType = msgType;

  unsigned char * header = NULL;
  //read header, if there any
  if (pMsg->HeaderSize() > 0)
  {
    //allocate header
    header = (unsigned char *)alloca(pMsg->HeaderSize());

    //read header data
    res=ReadPipe(header,pMsg->HeaderSize());
    if (res!=statOk) return res;
  } 

  //calculate data size
  int msgSize = pMsg->DataSize(header);
  //create buffer for message data
  unsigned char *msgBody = msgSize>65535?(new unsigned char [msgSize]):((unsigned char *)alloca(msgSize));

  if (msgSize!=0)
    res=ReadPipe(msgBody,msgSize);    

  if (res==statOk)
     pMsg->SetData(msgBody,header);

  if (msgSize>65535) delete [] msgBody;

  return res;
}

VisViewerExchange::Status VisViewerPipe::BlockAndWait()
{
  if (_inbufptr<_inbufsz || _inbuffer==0) return statOk;
  DWORD rd;
  if (ReadFile(_receive,_inbuffer,1,&rd,0)==FALSE || rd!=1)
  {
    if (GetLastError()==ERROR_BROKEN_PIPE) return statLost;
    return statError;
  }
  _inbufsz=1;
  _inbufptr=0;
  return statOk;
}

VisViewerExchange::Status VisViewerPipe::WaitMessage(const Array<int> &msgIds, PSPosMessage& msg, unsigned long timeout)
{
  unsigned int remainTimeout=timeout;
  unsigned int t1=GetTickCount();
  while (true)
  {
    msg=0;
    Status st=ReceiveMessage(msg);
    if (st!=statOk && st!=statNoMessage) return st;
    if (msg.IsNull())
    {
      //neni zadny timeout
      if (remainTimeout==0) return statTimeout;
      //kolik je hodin      
      //cekej na zpravu
      Status res=BlockAndWait();
      //vyhodnot vysledek cekani
      if (res!=statOk) return res;
      //kolik je hodin
      unsigned int t2=GetTickCount();
      //spocitej jak dlouho se cekalo
      unsigned int tt=t2-t1;
      t1 = t2;
      //vypocti jeste zbyvajici cas
      if (tt>remainTimeout) remainTimeout=0;else remainTimeout-=tt;
    }
    else
    {
      //prisla zprava - je to nase
      for (int i=0;i<msgIds.Size();i++) 
        //ano, vrat ji, nezpracovavej ji
        if (msg->_iMsgType==msgIds[i]) return statOk; 
      //jinak zpracuj zpravu, kterou necekame
      DispatchMessage(*msg);      
    }
  }  
}

VisViewerExchange::Status VisViewerPipe::CheckInterfaceStatus()
{
  PSPosMessage msg;
  Status okStat=statNoMessage;
  while (true)
  {
    msg=0;
    Status st=ReceiveMessage(msg);
    if (st!=statOk) return st;
    if (msg.IsNull()) return okStat;
    okStat=DispatchMessage(*msg);
    if (okStat!=statOk) return okStat;
  }  
}

bool VisViewerPipe::Create(WPARAM wParam, LPARAM lParam)
{
  if (wParam==0 || lParam==0) return false;
  Create((HANDLE)lParam,(HANDLE)wParam);
  return true;
}

VisViewerPipe::Status VisViewerPipe::DispatchMessage(const SPosMessage &msg)
{
  if (msg._iMsgType==CURSOR_POSITION_SET)
  {  
    int msgid;
    DWORD rd;
    if (PeekNamedPipe(_receive,&msgid,sizeof(msgid),&rd,0,0)!=FALSE && rd==sizeof(msgid))
    {
      if (msgid==CURSOR_POSITION_SET) return statOk;
    }
  }
  return VisViewerExchange::DispatchMessage(msg);
}

VisViewerPipe::Status VisViewerPipe::StartBuffer(size_t bufferSize)
{
  Status res=statOk;
  if (_outbuff) 
  {
    res=FlushBuffer();
    if (res!=statOk) return res;
    if (bufferSize!=_bufsize)
    {
      delete [] _outbuff;
      _outbuff=new char[bufferSize];
    }
  }
  else
  {  
    _outbuff=new char[bufferSize];
  }
  _bufsize=bufferSize;
  _bufused=0;
  return res;
}

VisViewerPipe::Status VisViewerPipe::FlushBuffer()
{
  Status res=statOk;
  DWORD written=0;
  res=CheckInterfaceStatus();
  if (res!=statOk && res!=statNoMessage) return res;
  if (_bufused)
  {  
    if (WriteFile(_send,_outbuff,_bufused,&written,0)==FALSE || written!=_bufused)
    {
      if (GetLastError()==ERROR_BROKEN_PIPE) return statLost;
      return statError;
    }
  }
  _bufused=0;
  return statOk;
}

VisViewerPipe::Status VisViewerPipe::EndBuffer()
{
  Status res=FlushBuffer();
  delete [] _outbuff;
  _outbuff=0;
  return res;  
}

VisViewerPipe::Status VisViewerPipe::FillUpBuffer()
{
  if (_inbufptr<_inbufsz) return statOk;
  DWORD toread=0;
  if (PeekNamedPipe(_receive,0,0,0,&toread,0)==FALSE) 
    return statError;
  if (toread==0)
  {
    if (_inbufmax>65536)
      SetInBuffer(65536);
    return statNoMessage;
  }
  DWORD read;
  if (toread>(unsigned)_inbufmax) 
    SetInBuffer(toread);
  if (ReadFile(_receive,_inbuffer,toread,&read,0)==FALSE || toread!=read)
  {
    if (GetLastError()==ERROR_BROKEN_PIPE) return statLost;
    return statError;
  }
  _inbufptr=0;
  _inbufsz=toread;
  return statOk;
}

VisViewerPipe::Status VisViewerPipe::ReadPipe(void *ptr, int size)
{
  char *z=reinterpret_cast<char *>(ptr);
  int remain=_inbufsz-_inbufptr;
  int rd=__min(size,remain);
  memcpy(z,_inbuffer+_inbufptr,rd);
  size-=rd;
  _inbufptr+=rd;
  z+=rd;
  if (size)
  {
    DWORD read;
    if (ReadFile(_receive,z,size,&read,0)==FALSE || read!=size)
    {
      if (GetLastError()==ERROR_BROKEN_PIPE) return statLost;
      return statError;
    }
  }
  FillUpBuffer();
  return statOk;
}

VisViewerPipe::Status VisViewerPipe::PeekPipe()
{
  return FillUpBuffer();
}

void VisViewerPipe::SetInBuffer(int size)
{
  delete [] _inbuffer;

  _inbuffer=new char[size];
  _inbufmax=size;
  _inbufptr=0;
  _inbufsz=0;

}