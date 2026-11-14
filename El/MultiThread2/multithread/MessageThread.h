/*
Copyright (C)2006 bo Ondrej Novak
http://bredy.jinak.cz
novak.on@centrum.cz


This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _MESSAGETHREAD_BREDYLIBS_MUTLITHREADS_
#define _MESSAGETHREAD_BREDYLIBS_MUTLITHREADS_

#include "CriticalSection.h"
#include "BlockerSimpleBase.h"
#include "Synchronized.h"
#include <queue>
#include "IThreadMessage.h"

namespace MultiThread
{


  ///Base class for all message targets.
  /**
   This class (and all derived) can receive messages and store them in queue. Object is notified by the
   virtual function every time that message is received. There are function that allow to pump one message or 
   pump all queued messages
  */

  class MessageTarget
  {
  protected:
    ///Queue
    std::queue<IThreadMessage *> _queue;

    ///Critical section to keep class MT Safe
    mutable MiniCriticalSection _locker;

    ///When true, thread should exit soon
    bool _exitSignal;
    ///Called when new message arrived to the thread
    /**
     * @note This function is called in context of caller thread (the thread that 
     * posted some message). The Message Target should use this function to signal 
     * an event object that resumes waiting thread.
     */
    virtual void NotifyNewMsg() {}
  public:    

    ///Constructs message target
    MessageTarget() {_exitSignal=false;}
    ///Destructor
    /**
     * Destructor calls SignalExit. During destruction, no more messages can be posted.
     * 
     * @note It strongly recommended to avoid posting messages to the object when it starts destroying. 
     * If there can be such a danger, use proper synchronization.
     */
    virtual ~MessageTarget() 
    {
      AutoLockR<> lk(_locker);
        SignalExit();
    }

    ///Sends message to the thread represented by this instance.
    /**
     * @param msg pointer to the message. When message object is no longer needed, Release is called 
     * @note You cannot send message, when message target is in exit state. In this case, message is not queued
     *  and Release is called immediately
     */
    void SendMsg(IThreadMessage *msg)
    {
      AutoLockR<> lk(_locker);
      if (!_exitSignal) //we can enqueue message only when _exitSignal is false
      {
        _queue.push(msg); //enqueue message
        NotifyNewMsg();  //notify object, that new message arrived to the thread
      }
      else
      {
        msg->Release(false);   //_exitSignal is true, message cannot be enqueued. Release it
      }
    }


    ///Processes first message on the queue
    /**
       @retval true message was processed
       @retval false no message processed.
    */
    bool PumpMessage()
    {
      IThreadMessage *msg;
      {      
        AutoLockR<> lk(_locker);
        if (_queue.empty())   //if queue empty?
          return false;      //return fail
        msg=_queue.front();  //take first message from the queue
        _queue.pop();         //remove this message from tge queue
      }
      msg->Run();         //process the message
      msg->Release(true); //release the message
      return true;
    }

    ///Processes all message on the queue
    /**
       @retval true one or more messages processed
       @retval false no message processed.
    */
    bool PumpAllMessages()
    {
      bool pumped=false;
      while (!_exitSignal && PumpMessage()) {pumped=true;}
      return pumped;
    }

    ///Retrivies exit signal state
    bool IsExitSignaled() const {return _exitSignal;}

    ///Signals message target, that it should finish its work soon
    /**
     *  When exit signaled, current message is finished and function PumpAllMessages
     *  exits, event if there is more messages to process. During exit state, no more
     * messages can be posted. All messages in queue are removed
     *
     * @note Switching MessageTarget into exit state generates new message notify.
     *  This can be used to signal event object and release waiting thread and check queue status
     */
    void SignalExit()
    {
      AutoLockR<> lk(_locker);
      if (!_exitSignal)
      {
        _exitSignal=true;
        while (!_queue.empty())
        {
          IThreadMessage *m=_queue.front();
          _queue.pop();
          m->Release(false);
        }
        NotifyNewMsg();
      }
    }

    ///Rollbacks the exit signal
    void ResetExitSignal()
    {
      AutoLockR<> lk(_locker);
      _exitSignal=false;
    }

    ///Function tests, whether any message is waiting in the queue
    /**
     * @retval true message is ready.
     * @retval false queue is empty.
     */
    bool AnyMessage() const
    {
      AutoLockR<> lk(_locker);
      bool res=!_queue.empty();
      return res;
    }

    ///Returns count of messages in queue
    size_t CountMessages() const
    {
      AutoLockR<> lk(_locker);
      return _queue.size();
    }
    

  };

  ///Base class for messaging threads
  /**
   * Implements waiting for messages specified time
   */

  class MessageThreadBase: public MessageTarget
  {
  public: 
    ///Waits for message
    /**
     * @param timeout waiting timeout
     * @retval true message arrived
     * @retval false timeout elapsed
     */
    virtual bool WaitForMessage(unsigned long timeout)=0;

    ///Starts message loop
    /**
    * Function cycles and processes all posted message. 
    * @param messageWaitTimeout specified timeout for waiting to a message. If no message
    * arrives in specified time, function exits
    * @retval function exits after the timeout
    * @retval function exits after exit state was signaled     
    */
    bool PumpAllMessages(unsigned long messageWaitTimeout=-1)
    {
      if (IsExitSignaled()) {return false;}
      while (WaitForMessage(messageWaitTimeout))
      {
        MessageTarget::PumpAllMessages();
        if (IsExitSignaled()) {return false;}
      }
      return true;
    }

    ///Stops the message loop
    /**Alias to MessageTarget::SignalExit */
    void Stop()
    {
      SignalExit();
    }
    
    virtual ~MessageThreadBase() {}
    
  };

  ///Helps thread to process posted messages
  /**
   * Implements waiting for messages specified time
   */
  class MessageThreadClass: public MessageThreadBase
  {
  protected:
    ///Event object for waiting
    EventBlocker _msgWait;    
    ///Notifies event object that new message arrived
    virtual void NotifyNewMsg() 
    {      
      _msgWait.Unblock();
    }

    virtual bool WaitForMessage(unsigned long timeout)
    {
      return _msgWait.Acquire(timeout);
    }
    

  public:

    ///Constructor
    MessageThreadClass():_msgWait(EventBlocker::Blocked_AutoReset) {}
  };

  ///Implements message controlled thread
  /**
   * You can create this class from any extension of thread. Class excepts ThreadBase
   * by default, but any extension of this class is acceptable.
   * 
   * This thread can be used as it is. This thread will process any posted messages.
   * Thread doesn't need to be started. It is started when first message arrives.
   */
  template<class Thr=ThreadBase>
  class MessageThread: public Thr, public MessageThreadClass
  {    
    ///Time after which the thread exits if no message arrive
    unsigned long _timeout;
    ///If it is true, thread exiting
    bool _threadInExit;
  public:
    MessageThread(unsigned long timeout=-1):_timeout(timeout),_threadInExit(false) {}
    MessageThread(ThreadBase::Priority priority, size_t initStack=0, unsigned long timeout=-1):Thr(priority,initStack),_timeout(timeout),_threadInExit(false) {}
    virtual void NotifyNewMsg()
    {
      //This function is called with locked object, no locking is necessary
      
      if (_threadInExit) 
      {
        //message was posted when thread is in exit state
        Thr::Join();  //wait for finish the thread        
        //we can wait, because in _threadInExit, lock will not be holded in future, so
        //no deadlock can happen
      }
      if (!Thr::IsRunning() && !_exitSignal)  //if not thread is running, it must be started
      {           
        _threadInExit=false; //reset _threadInExit state
        _msgWait.Block();  //_msgWait is not in use now. We will use to wait thread start
        Thr::Start();           //start the thread
        _msgWait.Acquire();//wait for acknowledge that thread running
        _msgWait.Release();//release the event;
      }
      
      MessageThreadClass::NotifyNewMsg();
    }

    virtual unsigned long Run()
    {
      _msgWait.Unblock(); //unblock the starting thread
      bool state; //exist state of PumpAllMessages
      bool anymsg; //AnyMessage
      _locker.Lock(); //first lock the object (next cycle is started with locked object)      
      //the sideeffect of the lock is that it wait for _msgWait object will be free to use
      //for message loop (it has been used to acknowledge thread start)
      do 
      {
        _locker.Unlock(); //unlock object to allow posting during processing
        state=MessageThreadClass::PumpAllMessages(_timeout); //process message
        //* in this place, new message could arrived, 
        _locker.Lock(); //lock the object
        anymsg=MessageThreadClass::AnyMessage(); //check queue status, if there is message        
      } 
      while(anymsg && state); //cycle until all messages processed
      //Finally thread exit.
      _threadInExit=true; //mark thread exiting
      MessageThreadClass::_locker.Unlock(); //we can unlock object now
      return 0; //exit thread
    }
    
    ///Stops the thread and flushes all messages
    /**
     * This must be called in destructor (of final class, if this class is extended) 
     */
    void Stop()
    {
      if (!IsExitSignaled())
      {      
        MessageThreadClass::Stop();
        Thr::Join();      
      }
    }
    
    ~MessageThread()
    {
      Stop();
    }

    ///Sets threads timeout
    void SetTimeout(unsigned long timeout)
    {
      ///sets timeout
      _timeout=timeout;
      ///unblock the thread, that timeout takes an effect
      _msgWait.Unblock();
    }

  };
};

#endif
