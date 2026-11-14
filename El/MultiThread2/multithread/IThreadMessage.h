#ifndef IMESSAGETARGET_H__BREDYLIBS_MUTLITHREADS_
#define IMESSAGETARGET_H__BREDYLIBS_MUTLITHREADS_

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
      when message was not processed because queue has been flushed.

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
}

#endif /*IMESSAGETARGET_H__BREDYLIBS_MUTLITHREADS_*/
