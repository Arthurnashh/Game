#ifdef _MSC_VER
#pragma once
#endif

#ifndef _I_DEBUG_ENGINE_INTERFACE_HPP
#define _I_DEBUG_ENGINE_INTERFACE_HPP

#include <Es/Types/pointers.hpp>
#include <Es/Strings/rString.hpp>

template<class Type>
struct IRefTraits
{
  static __forceinline int AddRef(Type *ptr) {return ptr->IAddRef();}
  static __forceinline int Release(Type *ptr) {return ptr->IRelease();}
};

class IDebugValue;
typedef RefR<IDebugValue, IDebugValue, IRefTraits<IDebugValue> > IDebugValueRef;

class IDebugValue
{
public:
  IDebugValue() {}
  virtual ~IDebugValue() {}

  // reference counting
  virtual int IAddRef() = 0;
  virtual int IRelease() = 0;

  virtual void GetValueType(char *buffer, int len) const = 0;
  virtual void GetValueValue(unsigned int radix, char *buffer, int len) const = 0;
  virtual bool IsExpandable() const = 0;
  virtual int NItems() const = 0;
  virtual IDebugValueRef GetItem(int i) const = 0;
};

class IDebugVariable
{
public:
  virtual void GetVarName(char *buffer, int len) const = 0;
  virtual IDebugValueRef GetVarValue() const = 0;
};

class IDebugScope
{
public:
  virtual const char *GetName() const = 0;
  virtual int NVariables() const = 0;
  virtual int GetVariables(const IDebugVariable **storage, int count) const = 0;
  virtual IDebugValueRef EvaluateExpression(const char *code, unsigned int radix) = 0;
  virtual void GetDocumentPos(char *file, int fileSize, int &line) = 0;
  virtual IDebugScope *GetParentScope() = 0;
};

enum StepKind
{
  SKInto,
  SKOver,
  SKOut
};

enum StepUnit
{
  SUStatement,
  SULine,
  SUInstruction
};

class IDebugScript
{
public:
  IDebugScript() {}
  virtual ~IDebugScript() {}

  // reference counting
  virtual int IAddRef() = 0;
  virtual int IRelease() = 0;

  virtual bool IsAttached() const = 0;
  virtual bool IsEntered() const = 0;
  virtual bool IsRunning() const = 0;

  virtual void AttachScript() = 0;
  virtual void EnterScript() = 0;
  virtual void RunScript(bool run) = 0;
  virtual void Step(StepKind kind, StepUnit unit) = 0;

  virtual void GetScriptContext(IDebugScope * &scope) = 0;
  virtual void SetBreakpoint(const char *file, int line, unsigned long bp, bool enable) = 0;
  virtual void RemoveBreakpoint(unsigned long bp) = 0;
  virtual void EnableBreakpoint(unsigned long bp, bool enable) = 0;
};

typedef RefR<IDebugScript, IDebugScript, IRefTraits<IDebugScript> > IDebugScriptRef;


#endif
