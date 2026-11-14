#if _MSC_VER>=1100
#pragma once
#endif

#ifndef _ES_ERROR_PROP_HPP
#define _ES_ERROR_PROP_HPP

#include <Es/Containers/listBidir.hpp>
#include <Es/Strings/rString.hpp>

/// information about an error
/**
Each info is a root of a double linked list
*/
class ErrorInfo: public TLinkBidir<-1>
{
  public:
  ErrorInfo(){}
  virtual ~ErrorInfo(){}
};

/// constant string passing - string object lifetime must be guaranteed
class ErrorInfoText: public ErrorInfo
{
  const char *_str;
  public:
  ErrorInfoText():_str(NULL){}
  ErrorInfoText(const char *str):_str(str){}
  const char *GetText() const {return _str;}
};

/// string passing
class ErrorInfoRString: public ErrorInfo
{
  RString _str;
  public:
  ErrorInfoRString():_str(){}
  ErrorInfoRString(RString str):_str(str){}
  RString GetText() const {return _str;}
};

/// helper for automatic deallocation
template <class Type>
class ErrorInfoRet
{
  friend class ErrorTry;

  mutable Type *_info;
  ErrorInfoRet(Type *info){_info = info;}
  void Unlink() const {_info = NULL;}
  Type *GetPointer() const {return _info;}


  public:
  ~ErrorInfoRet()
  {
    if (_info)
    {
      delete _info;
      _info = NULL;
    }
  }
  operator bool() const {return _info!=NULL;}
  void operator = (ErrorInfoRet &src)
  {
    // pass ownership
    _info = src._info;
    src._info = NULL;
  }
  const Type *operator ->() const {return _info;}
  Type *operator ->() {return _info;}
  const Type &operator *() const {return *_info;}
  Type &operator *() {return *_info;}
};
/// error propagation class
/**
Allows for low overhead error propagation between functions.
Errors may be propagated to any encapsulating levels when not handled.
*/

class ErrorTry
{
  TListBidir<ErrorInfo, TLinkBidir<-1> > _error;
  bool _propagateUp;

  ErrorTry *_parent;

  public:
  ErrorTry(bool propagateUp=true);
  ~ErrorTry();

  operator bool() const {return _error.First()!=NULL;}
  /// call this when all errors are handled and need to be propagated
  void Throw(ErrorInfo *error=new ErrorInfo());
  template <class Type>
  void Throw(const ErrorInfoRet<Type> &error)
  {
    Throw(error.GetPointer());
    error.Unlink();
  }
  /**
  MSVC 6 is not able to instantiate function templates explicitely
  */
  template <class Type>
  ErrorInfoRet<Type> Catch(
  #if _MSC_VER &&  _MSC_VER<1300 
    const Type &dummy
  #endif
  )
  {
    for (ErrorInfo *info = _error.Start(); _error.NotEnd(info); info=_error.Advance(info))
    {
      Type *exc = dynamic_cast<Type *>(info);
      if (exc==NULL) continue;
      ErrorHandled(exc);
      return ErrorInfoRet<Type>(exc);
    }
    return ErrorInfoRet<Type>(NULL);
  }
  void ErrorHandled(ErrorInfo *error);

  private:
  ErrorTry(const ErrorTry &src);
  void operator =(const ErrorTry &src);
};

extern ErrorTry *GErrorTry;

//@{ exception like usage macros

#define ERROR_TRY() {ErrorTry err;

#define ERROR_THROW(Type,value) if (GErrorTry) GErrorTry->Throw(new Type(value));
#define ERROR_THROW_NOVAL(Type) if (GErrorTry) GErrorTry->Throw(new Type());

// while is used, because we want to catch all error of the type we are able to process
#if _MSC_VER>=1300 || !_MSC_VER
  #define ERROR_CATCH(Type,exc) while (const ErrorInfoRet<Type> &exc = err.Catch<Type>())
#else
  #define ERROR_CATCH(Type,exc) while (const ErrorInfoRet<Type> &exc = err.Catch(Type()))
#endif
#define ERROR_CATCH_ANY(exc) ERROR_CATCH(ErrorInfo,exc)

#define ERROR_RETHROW(exc) GErrorTry->Throw(exc);

#define ERROR_END() }
//@}

#endif
