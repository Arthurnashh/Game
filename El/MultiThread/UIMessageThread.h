#pragma once

#include "messagethread.h"

namespace MultiThread
{

  ///Provides message target for any UI thread
  /**
    Implementation uses special UI messages to notify one thread by another. 
    UI thread processes these messages while it is waiting for message from user.
  */


  class UIMessageTarget :  public MultiThread::MessageTarget
  {
    char _instanceData[2*sizeof(void *)];
    friend char *UIMessageTarget_GetInstanceData(UIMessageTarget *a);
  public:
    ///Constructs message target
    /**
    There can be more instances of this class on one thread. Each instance may take a bit resource from OS pool.
    @param noInit true will cause, that message listener will not be installed. Construction such a instance is little bit faster, and it
      doesn't steal OS resource. But you cannot use this instance until message listener is installed. To install it later, call Install 
      function. You can send message to the message target even if no message listener is installed, but messages will not processed. After
      installation, all queued messages will be processed at the point when first message after installation will arrive.
    @note In case noInit false, class must be constructed in thread, that is target for messages. 
    */
    UIMessageTarget(bool noInit=false);
    virtual ~UIMessageTarget(void);

    ///Notifies thread, that there is new message in queue.
    /**
    You don't need to call this function explicit. This is done automatically by SendMsg function.
    */
    virtual void NotifyNewMsg();

    ///Installs message listener.
    /**
    Message listener is installed automatically by the constructor, exception that this feature is disabled by noInit parameter.
    @retval true listener has been installed or already has been installed.
    @retval false listener has not been instaled, because there were no more resource available.
    @note Call install in thread that will process the messages.
    */
    bool Install();

    ///Retrivies message listener install status.
    bool IsInstalled() const;

    ///Removes message listener
    bool Uninstall();
  };

}