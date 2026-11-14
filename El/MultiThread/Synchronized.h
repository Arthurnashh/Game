#ifndef _BREDY_LIBS_TYPES_SYNCHRONIZED_
#define _BREDY_LIBS_TYPES_SYNCHRONIZED_

#include "Assert.h"

#include "Interlocked.h"

#ifdef _MT
#include "CriticalSection.h"
#endif

namespace MultiThread
{


  using namespace MultiThreadSafe;


  ///Implements basic synchronized type - signed int (long)
  /**Synchronized type ensures, that all operation are carried out atomically
  This is promised on all "get modify and store" functions, such as operators ++,--, +=, -= etc.
  There are some special functions, that provides atomic exchange, or test and store operations
  @see SyncInt::Exchange, SyncInt::setValueWhen
  @note SyncInt type must be aligned to 32-bit address
  */
  class SyncInt
  {
    long _value; ///<Variable that stores value
  public:
    SyncInt() {} ///<Default constructor (constructs uninicialized value)
    SyncInt(long value):_value(value) {} ///<Constructor (constructs inicialized value)
    SyncInt(const SyncInt &other):_value(other._value) {} ///<Copy constructior

    ///Provides atomic assignment
    long operator=(long value) {_value=value;return value;}
    ///Provides atomic assignment
    SyncInt &operator=(const SyncInt &other) {_value=other._value;return *this;}

    ///Provides atomic increment. 
    /**No other processors or threads can change this value during this operation*/
    long operator++() {return MTIncrement(&_value);}

    ///Provides atomic decrement. 
    /**No other processors or threads can change this value during this operation*/
    long operator--() {return MTDecrement(&_value);}

    ///Provides atomic left increment. 
    /**No other processors or threads can change this value during this operation*/
    long operator++(int z) {return MTExchangeAdd(&_value,1);}

    ///Provides atomic left decrement. 
    /**No other processors or threads can change this value during this operation*/
    long operator--(int z) {return MTExchangeAdd(&_value,-1);}

    ///Returns value. 
    /**
    @note Result shouldn't reflects current value, because, it can be changed before result is returned
    */
    operator long () {return _value;}


    ///Provides atomic add. 
    /**No other processors or threads can change this value during this operation*/
    long operator+=(long add) {return MTExchangeAdd(&_value,add)+add;}

    ///Provides atomic substract 
    /**No other processors or threads can change this value during this operation*/
    long operator-=(long add) {return MTExchangeAdd(&_value,-add)-add;}

    ///Provides atomic multiply
    /**  
    @note Implementation can process operation multiple times, until it ensures, that no other
    threads did not change value during operation
    */
    long operator*=(long mm)
    {
      long mval;
      long mres;
      do {mval=_value;mres=mval*mm;} while (MTCompareExchange(&_value,mres,mval)!=mval);
      return mres;
    }

    ///Provides atomic divide
    /**  
    @note Implementation can process operation multiple times, until it ensures, that no other
    threads did not change value during operation
    */
    long operator/=(long mm)
    {
      long mval;
      long mres;
      do {mval=_value;mres=mval/mm;} while (MTCompareExchange(&_value,mres,mval)!=mval);
      return mres;
    }

    ///Provides atomic shift right
    /**  
    @note Implementation can process operation multiple times, until it ensures, that no other
    threads did not change value during operation
    */
    long operator>>(long mm)
    {
      long mval;
      long mres;
      do {mval=_value;mres=mval>>mm;} while (MTCompareExchange(&_value,mres,mval)!=mval);
      return mres;
    }

    ///Provides atomic shift left
    /**  
    @note Implementation can process operation multiple times, until it ensures, that no other
    threads did not change value during operation
    */
    long operator<<(long mm)
    {
      long mval;
      long mres;
      do {mval=_value;mres=mval<<mm;} while (MTCompareExchange(&_value,mres,mval)!=mval);
      return mres;
    }

    ///Provides atomic exchange
    /**  
    gets value and stores another. Operation is interlocked
    */
    long Exchange(long with)
    {
      return MTExchange(&_value,with);
    }

    ///Provides atomic test with set value
    /**
    Function tests value for oldvalue. When value is equal to parameter, sets value to newvalue and returns
    true. Otherwise, value remains unchanged and returns false
    @param oldvalue value to compare
    @param newvalue new value, that will be set to object, when test passed
    @return true, if value has been changed (test passed), false if test failed.
    @note don't use "if (syncInt==0) then {syncIntr=1;...}". Instead use "if (syncInt.SetValueWhen(0,1)) then {...}"
    */
    bool SetValueWhen(long oldvalue, long newvalue)
    {
      return MTCompareExchange(&_value,newvalue,oldvalue)==oldvalue;
    }


  };
  ///Implements basic synchronized type - typed pointer -
  /** This is promised on all "get modify and store" functions, such as operators ++,--, +=, -= etc.
  There are some special functions, that provides atomic exchange, or test and store operations
  @see SyncInt::Exchange, SyncInt::setValueWhen
  @note SyncInt type must be aligned to 32-bit address
  */
  template<class Type>
  class SyncPtr
  { 
  protected:
    Type *_ptr;
  public:
    SyncPtr():_ptr(0) {}
    SyncPtr(Type *ptr):_ptr(ptr) {}
    SyncPtr(const SyncPtr &other):_ptr(other._ptr) {}

    SyncPtr &operator=(const SyncPtr &other) {_ptr=other._ptr;return *this;}
    Type *operator=(Type *other) {_ptr=other;return _ptr;}

    operator Type *() {return _ptr;}
    operator Type *() const {return _ptr;}

    Type *operator->() {return _ptr;}
    Type *operator->() const {return _ptr;}

    Type *operator++() {return (Type *)MTCompareExchange((long *)&_ptr,sizeof(Type));}
    Type *operator--() {return (Type *)MTCompareExchange((long *)&_ptr,-sizeof(Type));}
    Type *operator++(int) {return (Type *)MTCompareExchange((long *)&_ptr,sizeof(Type))-1;}
    Type *operator--(int) {return (Type *)MTCompareExchange((long *)&_ptr,-sizeof(Type))+1;}

    Type *Exchange(Type *with) {return (Type *)MTExchange((long *)&_ptr,(long)with);}
    bool SetValueWhen(Type *oldvalue, Type *newvalue)
    {
      return MTCompareExchange((long *)&_ptr,(long)newvalue,(long)oldvalue)==(long)oldvalue;
    }

    bool operator! () const {return _ptr==0;}
    bool IsNull() const {return _ptr==0;}
    bool NotNull() const {return _ptr!=0;}

    Type *Ptr() {return _ptr;}
    const Type *Ptr() const {return _ptr;}

  };


  class Synchronized
  {
  public:
    class SynchronizedSection;
  protected:

    friend class SynchronizedSection;

#ifdef _MT
    MultiThread::CriticalSection _lock;
#endif

#ifdef _MT
    MultiThread::CriticalSection &GetLocker() {return _lock;}
#endif
  public:

    class SynchronizedSection
    {
#ifdef _MT
      MultiThread::CriticalSection &_locker;
    public:
      SynchronizedSection(MultiThread::CriticalSection &lk):_locker(lk)
      {
        _locker.Lock();
      }

      SynchronizedSection(MultiThread::CriticalSection &lk, unsigned long timeout):_locker(lk)
      {
        _locker.Lock(timeout);
      }

      bool IsSynchronized() {return _locker.IamOwner();}

      ~SynchronizedSection()
      {
        _locker.Unlock();
      }
#else
    public:
      SynchronizedSection(Synchronized *lk) {}
#endif
    };
  };

#ifdef _MT
#define SYNCHRONIZED() SynchronizedSection __synchronized(GetLocker())
#define SYNCHRONIZED_TIMEOUT(x) SynchronizedSection __synchronized(GetLocker(),x)
#define IS_SYNCHRONIZED() (__synchronized.IsSynchronized())
#else
#define SYNCHRONIZED()
#define SYNCHRONIZED_TIMEOUT(x)
#define IS_SYNCHRONIZED()
#endif

  class SynchronizeBase
  {
  public:
    static bool LockObjects(const void **arr, int count, unsigned long timeout=0xFFFFFFFF);
    static void UnlockObjects(const void **arr, int count);
  };

  template<int nObjects=1>
  class Synchronize: protected SynchronizeBase
  {
    const void *_objects[nObjects];
    bool _locked;


  public:
    Synchronize(const void *obj1);
    Synchronize(const void *obj1,const void *obj2);
    Synchronize(const void *obj1,const void *obj2, const void *obj3);
    Synchronize(const void *obj1,const void *obj2, const void *obj3, const void *obj4);
    Synchronize(const void *obj1,const void *obj2, const void *obj3, const void *obj4, const void *obj5, ...);

    Synchronize(unsigned long timeout,const void *obj1);
    Synchronize(unsigned long timeout,const void *obj1,const void *obj2);
    Synchronize(unsigned long timeout,const void *obj1,const void *obj2, const void *obj3);
    Synchronize(unsigned long timeout,const void *obj1,const void *obj2, const void *obj3, const void *obj4);
    Synchronize(unsigned long timeout,const void *obj1,const void *obj2, const void *obj3, const void *obj4, const void *obj5, ...);

    ~Synchronize()
    {
      if (_locked) UnlockObjects(_objects,nObjects);
    }

    bool operator!() const {return _locked;}



  };

  template<int nObjects> Synchronize<nObjects>::Synchronize(const void *obj1) {constructor_argument_count_mistmatch;}
  template<int nObjects> Synchronize<nObjects>::Synchronize(const void *obj1,const void *obj2) {constructor_argument_count_mistmatch;}
  template<int nObjects> Synchronize<nObjects>::Synchronize(const void *obj1,const void *obj2, const void *obj3) {constructor_argument_count_mistmatch;}
  template<int nObjects> Synchronize<nObjects>::Synchronize(const void *obj1,const void *obj2, const void *obj3, const void *obj4) {constructor_argument_count_mistmatch;}
  template<int nObjects> Synchronize<nObjects>::Synchronize(unsigned long timeout,const void *obj1) {constructor_argument_count_mistmatch;}
  template<int nObjects> Synchronize<nObjects>::Synchronize(unsigned long timeout,const void *obj1,const void *obj2) {constructor_argument_count_mistmatch;}
  template<int nObjects> Synchronize<nObjects>::Synchronize(unsigned long timeout,const void *obj1,const void *obj2, const void *obj3) {constructor_argument_count_mistmatch;}
  template<int nObjects> Synchronize<nObjects>::Synchronize(unsigned long timeout,const void *obj1,const void *obj2, const void *obj3, const void *obj4) {constructor_argument_count_mistmatch;}


  template<int nObjects> Synchronize<nObjects>::Synchronize(const void *obj1,const void *obj2, const void *obj3, const void *obj4, const void *obj5,...) 
  {
    const void **arr=&obj1;
    for (int i=0;i<nObjects;i++) _objects[i]=arr[i];
    _locked=LockObjects(_objects,nObjects,timeout);
  }

  template<int nObjects> Synchronize<nObjects>::Synchronize(unsigned long timeout,const void *obj1,const void *obj2, const void *obj3, const void *obj4, const void *obj5, ...)
  {
    const void **arr=&obj1;
    for (int i=0;i<nObjects;i++) _objects[i]=arr[i];
    _locked=LockObjects(_objects,nObjects,timeout);
  }


  template<> inline Synchronize<1>::Synchronize(const void *obj1) 
  {
    _objects[0]=obj1;
    _locked=LockObjects(_objects,1);
  }
  template<> inline Synchronize<2>::Synchronize(const void *obj1,const void *obj2) 
  {
    _objects[0]=obj1;
    _objects[1]=obj2;
    _locked=LockObjects(_objects,2);
  }
  template<> inline Synchronize<3>::Synchronize(const void *obj1,const void *obj2, const void *obj3) 
  {
    _objects[0]=obj1;
    _objects[1]=obj2;
    _objects[2]=obj3;
    _locked=LockObjects(_objects,3);
  }
  template<> inline Synchronize<4>::Synchronize(const void *obj1,const void *obj2, const void *obj3, const void *obj4) 
  {
    _objects[0]=obj1;
    _objects[1]=obj2;
    _objects[2]=obj3;
    _objects[3]=obj4;
    _locked=LockObjects(_objects,4);
  }

  template<> inline Synchronize<1>::Synchronize(unsigned long timeout, const void *obj1) 
  {
    _objects[0]=obj1;
    _locked=LockObjects(_objects,1,timeout);
  }
  template<> inline Synchronize<2>::Synchronize(unsigned long timeout, const void *obj1,const void *obj2) 
  {
    _objects[0]=obj1;
    _objects[1]=obj2;
    _locked=LockObjects(_objects,2,timeout);
  }
  template<> inline Synchronize<3>::Synchronize(unsigned long timeout, const void *obj1,const void *obj2, const void *obj3) 
  {
    _objects[0]=obj1;
    _objects[1]=obj2;
    _objects[2]=obj3;
    _locked=LockObjects(_objects,3,timeout);
  }
  template<> inline Synchronize<4>::Synchronize(unsigned long timeout, const void *obj1,const void *obj2, const void *obj3, const void *obj4) 
  {
    _objects[0]=obj1;
    _objects[1]=obj2;
    _objects[2]=obj3;
    _objects[3]=obj4;
    _locked=LockObjects(_objects,4,timeout);
  }

};

#endif