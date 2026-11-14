#ifndef _MESSAGETHREAD_BREDYLIBS_MUTLITHREADS_
#define _MESSAGETHREAD_BREDYLIBS_MUTLITHREADS_


#include "threadBase.h"
#include "CriticalSection.h"
#include "BlockerSimpleBase.h"
#include "Synchronized.h"
#include <queue>

namespace MultiThread
{

  ///Defines simple thread message
  /**
    This is a base class for all messages post able into the message-thread. Each message is also derived 
    from IRunnable, so it must define a function that will be processed in context of the thread. Message should
    be allocated dynamically (most of cases). When message is processed, calling thread calls Release function,
    that causes deletion by default. You can sometime use statically allocated messages, these messages
     should define own version of Release().

     @note there is no reference counting. Message pointer is moved through queue into calling thread, and
     after processing, it is released.
  */
  class IThreadMessage: public IRunnable
  {
  public:    
        
    ///Message function implementation
    /**
    @return function result, mostly ignored. Use 0 as default return
    @note This function is always processed in context of target thread
    */
    virtual unsigned long Run()=0;
    ///Some messages can except reply. Function notifies message that thread stored the reply into the message
    /**
    It useful to use message for output a reply. After posting, caller can stop and wait until calling thread
    replies the message. Calling thread can reply anytime. It is not necessary to send reply at the end of the
    function. When reply posted, caller is released and continues on his for (it can read result).

    @retval true Reply successfully posted
    @retval false No success or function is not supported

    @note This function should be always processed in context of target thread. But derived
    class can call this function from Release when it needs process some reply when message is
    destroyed without reply
    */
    virtual bool PostReply() {return false;}

    /**
    Stops the calling threads until reply arrives.
    @see PostReply
    @param timeout timeout in milliseconds. Use default value to wait infinite.
    @retval true reply arrived
    @retval false error or not supported.

    @note Function should be always processed outside of target process. Invoking this
    function in target process causes a deadlock. 
    */
    virtual bool WaitForReply(unsigned long timeout=-1) {return false;}

    ///Notifies message, that work is done, message should be destroyed
    /**
      Function is called after Run processed. Function should delete the occupied memory and
      free any associated resources. Base implementation call delete operator directly on this.

      @param succ true, when release is called after message was successfully processed. false,
      when message was not processed because queue was flushed.

      @note Release can be called even if Run was not processed. This situation means "rejected".
      Messages are mostly rejected by the destructor of the target class. Instance of that class
      can be dropped, and during  cleaning, queue is flushed and all queued messages are marked as "rejected".

      @note During Release, check whether the message has been replied. If there a thread, that still can 
      wait for the reply, destroying message can cause deadlock. If message is not replied, and reply is
      excepted, you should reply message before deleting.

      @note Beware of context of this function. It can be sometime processed in the
      target thread, sometime outside of target thread. You should use proper synchronization
      when you accessing data from this function and message is referenced by other threads.
      @note Function in called in target thread after message is processed or when thread
        closed down the queue that causes flushing the queue. Function is called in other thread
        when the thread is destroying the object, or when the thread closed down the queue thta
        causes flushing the queue
      
    */
    virtual void Release(bool succ) {delete this;}
    virtual ~IThreadMessage() {}
  };

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

    ///Whwn true, thread should exit soon
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
    ~MessageTarget() 
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
        for (IThreadMessage *m=_queue.front();m;m=_queue.front())
        {
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
    int CountMessages() const
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
      while (WaitForMessage(messageWaitTimeout))
      {
        PumpAllMessages();
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
  protected:
    MessageThread(unsigned long timeout=-1):timeout(timeout),_threadInExit(false) {}
    MessageThread(ThreadBase::Priority priority, size_t initStack=0, unsigned long timeout=-1):Thr(priority,initStack),timeout(timeout),_threadInExit(false) {}
    virtual void NotifyNewMsg()
    {
      //This function is called with locked object, no locking is necessary
      
      if (_threadInExit) 
      {
        //message was posted when thread is in exit state
        Join();  //wait for finish the thread        
        //we can wait, because in _threadInExit, lock will not be holded in future, so
        //no deadlock can happen
      }
      if (!IsRunning())  //if not thread is running, it must be started
      {           
        _threadInExit=false; //reset _threadInExit state
        _msgWait.Block();  //_msgWait is not in use now. We will use to wait thread start
        Start();           //start the thread
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
        state=MessageThreadClass::PumpAllMessages(timeout); //process message
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
        Join();      
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