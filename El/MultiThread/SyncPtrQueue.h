#ifndef _SYNCPTRQUEUE_BREDYLIBS_MUTLITHREADS_
#define _SYNCPTRQUEUE_BREDYLIBS_MUTLITHREADS_
#pragma once

#include "Synchronized.h"


namespace MultiThread
{

  ///Default traits for SyncPtrQueue - Empty implementation
  template<class T>
  class SyncPtrQueueDefaultTraits
  {
  public:
    static void AddRef(T *item) {}
    static void Release(T *item) {}
  };

  ///RefCount traits for SyncPtrQueue - Calls AddRef and Release functions
  template<class T>
  class SyncPtrQueue_RefCountTraits
  {
  public:
    static void AddRef(T *item) {item->AddRef();}
    static void Release(T *item) {item->Release();}
  };

  ///Provides interlocked queue.
  /**
  Interlocked queue allows you to implement pointer queue for multiple threads without locking
  All operations in queue are provided using interlocked operations
  */
  template<class T, class PointerTraits=SyncPtrQueueDefaultTraits<T> >
  class SyncPtrQueue: public PointerTraits
  {
  public:
    typedef SyncPtr<T> Item;  ///<defines iteme type. Each item is synchronized pointer

  private:
    int _count; ///<Count of available items in queue.
    SyncInt _bottom; ///<Index of item on bottom of queue.
    SyncInt _top;	///<First free item on top of queue. If this item is not 0, queue is full.
    Item *_queue; ///<Array of items

  public:

    ///Constructs the queue. You must specify queue length
    /**
    @param queueLen Length of queue in items. Should be positive number above 2
    */
    SyncPtrQueue(int queueLen):_count(queueLen),_bottom(0),_top(0),_queue(new Item[queueLen])
    {
      for (int i=0;i<_count;i++) _queue[i]=0;
    }

    ///Destructs the queue
    /**
    @note Destroy queue after all threads leave. 
    @note All items are released.
    */
    ~SyncPtrQueue()
    {
      for (int i=0;i<_count;i++) 
      {
        T *x=_queue[i].Exchange(0);
        if (x) Release(x);
      }
      delete [] _queue;
    }

    ///Provieds synchronized push operation
    /**
    Push is synchronized.
    @param item pointer to item to push
    @return true, if item was added, or false, if queue is full
    @note AddRef is called. Item is copied into queue.
    */
    bool Push(T *item)
    {
      if (item==0) return false;
      AddRef(item);
      long newtop;
      long oldtop;
      do 
      {
        oldtop=_top;   //vyzvedni top
        newtop=(oldtop+1)%_count; //vypocti novy top
        if (_queue[newtop]!=0 && oldtop==_top) return false; //full queue
      } 
      // uloz novy top, pokud je tam stale stary, a zaroven pokud na pozici queue je null
      while(!_top.SetValueWhen(oldtop,newtop) || !_queue[oldtop].SetValueWhen(0,item));
      return true;
    }

    ///Provieds synchronized pop operation
    /**
    Pop is synchronized.
    @return pointer to popped item, or zero, if queue is empty
    @note Item is not released. Pointer is moved.
    */
    T *Pop()
    {
      if (_bottom==_top) return 0;
      long newbot;
      long oldbot;
      do  //pokus se zvednout bottom ukazatel
      {
        oldbot=_bottom;
        newbot=(oldbot+1)%_count;	  
      }
      while(!_bottom.SetValueWhen(oldbot,newbot));  //pokud se to podari
      return _queue[oldbot].Exchange(0); //vyzvedni hodnotu a dej misto ni NULL
    }
  };

};
#endif