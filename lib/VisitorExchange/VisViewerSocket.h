#pragma once

#include <El/TCPIPBasics/Socket.h>
#include <Es/Containers/array.hpp>
#include "messagedll.h"
#include <Es/Types/pointers.hpp>
#include <assert.h>

#include "VisViewerExchange.h"


///Implements exchange using the sockets
class VisViewerSocket: public VisViewerExchange
{
public:

private:
  Socket _sock;
  unsigned int _defaultTimeout;
protected:

  ///Sends a message
  /**
  @return status
  */
  Status SendMessage(const SPosMessage& msg);
  ///Receives a message
  /**
  @return status
  */
  Status ReceiveMessage(PSPosMessage& msg);


public:
  Status WaitMessage(const Array<int> &msgIds, PSPosMessage& msg, unsigned long timeout=0xFFFFFFFF);

public:
  VisViewerSocket(IVisViewerExchangeDocView &event):VisViewerExchange(event) {}  

  void Create(Socket s);
  void Destroy();

  Status CheckInterfaceStatus();


};
