#pragma once

#pragma warning( disable : 4290 )

namespace MultiThread
{


  enum TlsBaseExceptionEnum {TlsOutOfIndexes};

  class TlsBase
  {
    unsigned long _index;
  public:
    TlsBase(void) throw (TlsBaseExceptionEnum);
    ~TlsBase(void);
    void SetValue(void *ptr);
    void *GetValue();
  };

template<class T>
class Tls:public TlsBase
{
public:
  void SetValue(T *ptr)
  {
    TlsBase::SetValue(reinterpret_cast<void *>(ptr));
  }

  T *GetValue()
  {
    return reinterpret_cast<T *>(TlsBase::GetValue());
  }

  T *operator=(T *val) {SetValue(val);return val;}
  operator T *() {return GetValue();}
  T *operator->() {return GetValue();}
};

};

#if _MSC_VER>1000 && !defined(_DLL_BUILD)
#define TLS(type) _declspec(thread) type *
#else
#define TLS(type) MultiThread::Tls<type>
#endif

