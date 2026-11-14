#include <Es/essencepch.hpp>
#include <Es/Common/win.h>
#include "VisViewerSocket.h"

void VisViewerSocket::Create(Socket s)
{
  _sock=s;
}

void VisViewerSocket::Destroy()
{
  _sock=Socket();
}

#if ALLOC_DEBUGGER
#define AllocatorAlloc(type,sz,a) reinterpret_cast<type *>(a.Alloc(sz,__FILE__,__LINE__,""))
#else
#define AllocatorAlloc(type,sz,a) reinterpret_cast<type *>(a.Alloc(sz))
#endif

VisViewerExchange::Status VisViewerSocket::SendMessage(const SPosMessage &sMsg)
{
  MemAllocLocal<char,256> allocator;

  size_t msgSize = sizeof(sMsg._iMsgType) + sMsg.HeaderSize() + sMsg.DataSize();
  size_t tmpSize=msgSize;
  unsigned char * pMsg = AllocatorAlloc(unsigned char,tmpSize,allocator);

  *(int *) pMsg = sMsg._iMsgType;
  sMsg.WriteHeader(pMsg + sizeof(sMsg._iMsgType));
  sMsg.WriteData(pMsg + sizeof(sMsg._iMsgType) + sMsg.HeaderSize());

  Socket::BlockOpRet result2 = _sock.BlockWrite(pMsg, msgSize);  
  allocator.Free(pMsg,msgSize);

  if (result2==Socket::BlockLost) return statLost;
  if (result2==Socket::BlockError) return statError;
  return statOk;
}

VisViewerExchange::Status VisViewerSocket::ReceiveMessage(PSPosMessage &pMsg)
{
  //Test if some message arrives
    int waitres=_sock.Wait(0,Socket::WaitRead|Socket::WaitExcept);
    if (waitres==Socket::WaitError) return statError;
    if (waitres & Socket::WaitExcept) return statError;
    if (waitres==Socket::WaitTimeout) return statOk;

  //Read message type
    int msgType; 
    Socket::BlockOpRet ret=_sock.BlockRead(&msgType,sizeof(msgType));
    if (ret==Socket::BlockLost) return statLost;
    if (ret==Socket::BlockError) return statError;

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
    Socket::BlockOpRet ret=_sock.BlockRead(header,pMsg->HeaderSize());
    if (ret==Socket::BlockLost) return statLost;
    if (ret==Socket::BlockError) return statError;
  } 

  //calculate data size
  int msgSize = pMsg->DataSize(header);
  //create buffer for message data
  unsigned char *msgBody = msgSize>65535?(new unsigned char [msgSize]):((unsigned char *)alloca(msgSize));

  //read message data
  ret=_sock.BlockRead(msgBody, msgSize);  
  if (ret==Socket::BlockOk)
  {
    //save message data
    pMsg->SetData(msgBody, header );
  }
  if (msgSize>65535) delete [] msgBody;

  //handle errors
  if (ret==Socket::BlockLost) return statLost;
  if (ret==Socket::BlockError) return statError;

  return statOk;
}

VisViewerExchange::Status VisViewerSocket::CheckInterfaceStatus()
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

VisViewerExchange::Status VisViewerSocket::WaitMessage(const Array<int> &msgIds, PSPosMessage& msg, unsigned long timeout)
{
  unsigned int remainTimeout=timeout;
  while (true)
  {
    msg=0;
    Status st=ReceiveMessage(msg);
    if (st!=statOk) return st;
    if (msg.IsNull())
    {
      //neni zadny timeout
      if (remainTimeout==0) return statTimeout;
      //kolik je hodin
      unsigned int t1=GetTickCount();
      //cekej na zpravu
      int res=_sock.Wait(remainTimeout,_sock.WaitRead|_sock.WaitExcept);
      //kolik je hodin
      unsigned int t2=GetTickCount();
      //vyhodnot vysledek cekani
      if (res==_sock.WaitError) return statError;
      if (res & _sock.WaitExcept) return statError;
      if (res==0) return statTimeout;
      //spocitej jak dlouho se cekalo
      unsigned int tt=t2-t1;
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


