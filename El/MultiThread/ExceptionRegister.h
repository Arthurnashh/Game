#pragma once

namespace ExceptReg
{


class IException
{
public:
  ///Returns exception code
  virtual unsigned int GetCode() const=0;
  ///Returns exception description (as english text)
  virtual const _TCHAR *GetDesc() const=0;
  ///Returns exception module source name
  virtual const _TCHAR *GetModule() const=0;
  ///Returns exceotion type
  /**
   * Function returns name of exception class. It
   * can be anything, but returned pointer should be always the same. To compare objects 
   * compare pointers should be enough.
  */
  virtual const _TCHAR *GetType() const=0;
  ///Returns count of parameters associated with this exception
  virtual unsigned int GetParamCount() const {return 0;}
  ///Returns parameter at position
  /**
   @param pos position of parameter
   @param param reference to variable that will receive the parameter
   @retval true parameter received
   @retval false cannot receive the parameter
  */
  virtual bool GetParam(unsigned int pos, int &param) const {return false;}
  ///Returns parameter at position  
  /**
   @param pos position of parameter
   @param param reference to variable that will receive the parameter
   @retval true parameter received
   @retval false cannot receive the parameter
  */
  virtual bool GetParam(unsigned int pos, unsigned int &param) const {return false;}
  ///Returns parameter at position
  /**
   @param pos position of parameter
   @param param reference to variable that will receive the parameter
   @retval true parameter received
   @retval false cannot receive the parameter
  */
  virtual bool GetParam(unsigned int pos, float &param) const {return false;}
  ///Returns parameter at position
  /**
   @param pos position of parameter
   @param param reference to variable that will receive the parameter
   @retval true parameter received
   @retval false cannot receive the parameter
  */
  virtual bool GetParam(unsigned int pos, double &param) const {return false;}
  ///Returns parameter at position
  /**
   @param pos position of parameter
   @param param reference to variable that will receive the parameter
   @retval true parameter received
   @retval false cannot receive the parameter
  */
  virtual bool GetParam(unsigned int pos, const char * &param) const {return false;}
  ///Returns parameter at position
  /**
   @param pos position of parameter
   @param param reference to variable that will receive the parameter
   @retval true parameter received
   @retval false cannot receive the parameter
  */
  virtual bool GetParam(unsigned int pos, void * &param) const {return false;}

  ///Creates copy of itself
  /**
  It creates copy of itself. It is not necesery to create right copy, it can be different class, but this
  class must represent the same type of exception.
  @retval pointer to copy. Pointer needn't to be allocated in heap. If there is another memory allocatar,
  it can be used. To release memory, DestroyCopy will be used.
  */
  virtual IException *MakeCopy() const {return 0;}

  ///Destroyes copy of the object.
  /**
  Destroyes this object, if it is a copy. Default implementation is using 'delete' operator. But it can be
  overwritten to use appropriate method of destruction.

  Don't call this function to original exception (this is error, and because exceptions are processed as const
  objects, you should not be able to call this function at original exception).
  */
  virtual void DestroyCopy() {delete this;}
};

class ExceptionRegister
{
  ExceptionRegister *_nextRegister;
public:
  ExceptionRegister(void);
  virtual ~ExceptionRegister(void);

  ///defines feedback for exception source
  enum StandardFeedback
  {
    fbAbort=0,    ///<abort operation and start controlled return (mostly default)
    fbRetry=1,    ///<retry failed operation
    fbIgnore=2,   ///<ignore reported error and try to continue
  };

  virtual int Catch(const IException &exception)=0;
  int CatchNext(const IException &exception)
  {
    if (_nextRegister) return _nextRegister->Catch(exception);
    else return DefaultHandler(exception);
  }

  static int RegExcpt(const IException &exception);
  static int DefaultHandler(const IException &exception);
  static ExceptReg::ExceptionRegister *GetCurrent();


  static va_list CreateListOfParams(const IException &exception, void *buffer);
};

static inline int RegExcpt(const IException &exception) {return ExceptReg::ExceptionRegister::RegExcpt(exception);}

typedef int (*RegExcpt_DefaultHandler_Type)(const IException &exception);
extern RegExcpt_DefaultHandler_Type RegExcpt_DefaultHandler;

}