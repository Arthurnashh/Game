#ifdef _MSC_VER
#  pragma once
#endif

/**
@file Delegate implementation
*/

#ifndef _ES_DELEGATE_HPP
#define _ES_DELEGATE_HPP

#include <stdio.h>

/// default base for non-virtual delegates
class DefDelegate0
{
};

/// base for virtual delegates
template <class RetType>
class VirtDelegate0
{
  public:
  // when delegate is remembered as VirtDelegate0 *, call appropriate destructor
  virtual ~VirtDelegate0() {}
  virtual RetType operator () () const = 0;
};

/// delegate - bind a member function (0 arguments) with an object
template <class Type, class RetType, class Base = DefDelegate0>
class Delegate0: public Base
{
  Type &_type;
  RetType (Type::*_member)() const;
  
  public:
  Delegate0(Type &type, RetType (Type::*member)() const):_type(type),_member(member){}
  
  RetType operator () () const
  {
    return (_type.*_member)();
  }
};

/// creative function for a non-virtual delegate with 0 arguments
template <class Type, class RetType>
Delegate0<Type,RetType> CreateDelegate0(Type *type, RetType (Type::*member)() const)
{
  return Delegate0<Type,RetType>(*type,member);
};

/// creative function for a non-virtual delegate with 0 arguments
template <class Type, class RetType>
Delegate0< Type,RetType,VirtDelegate0<RetType> > *CreateVirtDelegate0(Type *type, RetType (Type::*member)() const)
{
  return new Delegate0< Type,RetType,VirtDelegate0<RetType> >(*type,member);
};

class FooA
{
  public:
  float GetValue1() const {return 1;}
  float GetValue2() const {return 2;}
};

class FooB
{
  public:
  virtual float GetValue2() const {return 2;}
  virtual float GetValue3() const {return 3;}
};

class FooC: public FooB
{
  public:
  virtual float GetValue2() const {return 20;}
  virtual float GetValue3() const {return 30;}
  float GetValueC1() const {return 100;}
};

/// default base for non-virtual member function pointers
class DefMembFunc0
{
};

/// base for virtual delegates
template <class RetType, class Type>
class VirtMembFunc0
{
  public:
  virtual RetType operator () (Type *type) const = 0;
};

/// member function pointer, call via a base pointer to an object
template <class Type, class RetType, class BaseType=Type, class Base = DefMembFunc0>
class MembFunc0: public Base
{
  RetType (Type::*_member)() const;
  
  public:
  MembFunc0(RetType (Type::*member)() const):_member(member){}
  
  RetType operator () (const BaseType *type) const
  {
    Assert(dynamic_cast<const Type *>(type));
    return (static_cast<const Type *>(type)->*_member)();
  }
};


/// member function pointer, call via a base pointer to an object. It also contains configuration data for function. 
template <class Type, class RetType, class ConfigType, class BaseType=Type, class Base = DefMembFunc0>
class MembFunc0WithConfig: public Base
{ 
  const ConfigType _config; 
  RetType (Type::*_member)(ConfigType) const;

public:
  MembFunc0WithConfig(RetType (Type::*member)(ConfigType) const, ConfigType config):_member(member), _config(config) {};

  RetType operator () (const BaseType *type) const
  {
    Assert(dynamic_cast<const Type *>(type));
    return (static_cast<const Type *>(type)->*_member)(_config);
  }
};

/// creative function for a non-virtual member function pointer with 0 arguments
template <class BaseType, class Type, class RetType>
MembFunc0< Type,RetType,BaseType, VirtMembFunc0<RetType,BaseType> > *
CreateVirtMembFunc0(RetType (Type::*member)() const)
{
  return new MembFunc0< Type,RetType,BaseType, VirtMembFunc0<RetType,BaseType> >(member);
};



/// default base for non-virtual member function pointers
class DefConstMembFunc1
{
};

/// member function pointer, call via a base pointer to an object
template <class Type, class RetType, class Arg1Type, class BaseType=Type, class Base = DefConstMembFunc1>
class ConstMembFunc1: public Base
{
  RetType (Type::*_member)(Arg1Type) const;
  typedef RetType (BaseType::*BaseTypeMemberFunc)(Arg1Type) const;

public:
  ConstMembFunc1(RetType (Type::*member)(Arg1Type) const):_member(member){}

  RetType operator () (const BaseType *type, Arg1Type arg1) const
  {
    Assert(dynamic_cast<const Type *>(type));
    return (static_cast<const Type *>(type)->*_member)(arg1);
  }

  bool operator != (RetType (BaseType::*member)(Arg1Type) const) const {return (BaseTypeMemberFunc)_member!=member;}
};


/// default base for non-virtual member function pointers
class DefMembFunc1
{
};

/// member function pointer, call via a base pointer to an object
template <class Type, class RetType, class Arg1Type, class BaseType=Type, class Base = DefMembFunc1>
class MembFunc1: public Base
{
  RetType (Type::*_member)(Arg1Type);
  typedef RetType (BaseType::*BaseTypeMemberFunc)(Arg1Type);

public:
  MembFunc1(RetType (Type::*member)(Arg1Type)):_member(member){}

  RetType operator () (BaseType *type, Arg1Type arg1)
  {
    Assert(dynamic_cast<Type *>(type));
    return (static_cast<Type *>(type)->*_member)(arg1);
  }

  bool operator != (RetType (BaseType::*member)(Arg1Type)) const {return (BaseTypeMemberFunc)_member!=member;}
};


/// default base for non-virtual member function pointers
class DefConstMembFunc2
{
};

/// member function pointer, call via a base pointer to an object
template <class Type, class RetType, class Arg1Type, class Arg2Type, class BaseType=Type, class Base = DefConstMembFunc2>
class ConstMembFunc2: public Base
{
  RetType (Type::*_member)(Arg1Type, Arg2Type) const;
  typedef RetType (BaseType::*BaseTypeMemberFunc)(Arg1Type, Arg2Type) const;

public:
  ConstMembFunc2(RetType (Type::*member)(Arg1Type, Arg2Type) const):_member(member){}

  RetType operator () (const BaseType *type, Arg1Type arg1, Arg2Type arg2) const
  {
    Assert(dynamic_cast<const Type *>(type));
    return (static_cast<const Type *>(type)->*_member)(arg1, arg2);
  }

  bool operator != (RetType (BaseType::*member)(Arg1Type, Arg2Type) const) const {return (BaseTypeMemberFunc)_member!=member;}
};

/// default base for non-virtual member function pointers
class DefMembFunc2
{
};

/// member function pointer, call via a base pointer to an object
template <class Type, class RetType, class Arg1Type, class Arg2Type, class BaseType=Type, class Base = DefMembFunc2>
class MembFunc2: public Base
{
  RetType (Type::*_member)(Arg1Type, Arg2Type);
  typedef RetType (BaseType::*BaseTypeMemberFunc)(Arg1Type, Arg2Type);

public:
  MembFunc2(RetType (Type::*member)(Arg1Type, Arg2Type)):_member(member){}

  RetType operator () (BaseType *type, Arg1Type arg1, Arg2Type arg2)
  {
    Assert(dynamic_cast<Type *>(type));
    return (static_cast<Type *>(type)->*_member)(arg1, arg2);
  }

  bool operator != (RetType (BaseType::*member)(Arg1Type, Arg2Type)) const {return (BaseTypeMemberFunc)_member!=member;}
};

/// default base for non-virtual member function pointers
class DefConstMembFunc3
{
};

/// member function pointer, call via a base pointer to an object
template <class Type, class RetType, class Arg1Type, class Arg2Type, class Arg3Type, class BaseType=Type, class Base = DefConstMembFunc3>
class ConstMembFunc3: public Base
{
  RetType (Type::*_member)(Arg1Type, Arg2Type, Arg3Type) const;
  typedef RetType (BaseType::*BaseTypeMemberFunc)(Arg1Type, Arg2Type, Arg3Type) const;

public:
  ConstMembFunc3(RetType (Type::*member)(Arg1Type, Arg2Type, Arg3Type) const):_member(member){}

  RetType operator () (const BaseType *type, Arg1Type arg1, Arg2Type arg2, Arg3Type arg3) const
  {
    Assert(dynamic_cast<const Type *>(type));
    return (static_cast<const Type *>(type)->*_member)(arg1, arg2, arg3);
  }
  
  bool operator != (RetType (BaseType::*member)(Arg1Type, Arg2Type, Arg3Type) const) const {return (BaseTypeMemberFunc)_member!=member;}
};

/// default base for non-virtual member function pointers
class DefMembFunc3
{
};

/// member function pointer, call via a base pointer to an object
template <class Type, class RetType, class Arg1Type, class Arg2Type, class Arg3Type, class BaseType=Type, class Base = DefMembFunc3>
class MembFunc3: public Base
{
  RetType (Type::*_member)(Arg1Type, Arg2Type, Arg3Type);
  typedef RetType (BaseType::*BaseTypeMemberFunc)(Arg1Type, Arg2Type, Arg3Type);

public:
  MembFunc3(RetType (Type::*member)(Arg1Type, Arg2Type, Arg3Type)):_member(member){}

  RetType operator () (BaseType *type, Arg1Type arg1, Arg2Type arg2, Arg3Type arg3)
  {
    Assert(dynamic_cast<Type *>(type));
    return (static_cast<Type *>(type)->*_member)(arg1, arg2, arg3);
  }

  bool operator != (RetType (BaseType::*member)(Arg1Type, Arg2Type, Arg3Type)) const {return (BaseTypeMemberFunc)_member!=member;}
};

/* Example
int main()
{
  FooA *a = new FooA;
  FooB *b = new FooB;
  FooB *c = new FooC;

  VirtDelegate0<float> *ad2 = CreateVirtDelegate0(a,FooA::GetValue2);
  Delegate0<FooB,float> bd2 = CreateDelegate0(b,FooB::GetValue2);
  Delegate0<FooB,float> cd2 = CreateDelegate0(c,FooB::GetValue2);
  Delegate0<FooB,float> cd3 = CreateDelegate0(c,FooB::GetValue3);

  MembFunc0<FooC,float> cm1(FooC::GetValueC1);
  
  printf("ad2 %g\n",(*ad2)());
  printf("bd2 %g\n",bd2());
  printf("cd2 %g\n",cd2());
  printf("cd3 %g\n",cd3());
  printf("cd3 %g\n",cm1(*c));
  getchar();
  return 0;
}
*/

#endif
