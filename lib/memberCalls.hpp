#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MEMBER_CALLS_HPP
#define _MEMBER_CALLS_HPP

/*!
\file
Map calls to calls of a member of the class
*/

class CallVoidReturn {};

//! class representing empty argument list
class Arg0
{
  public:
  Arg0(){}
};

//! class representing 1 argument list

template <class A1>
class Arg1
{
  public:
  A1 a1;

  Arg1(A1 aa1):a1(aa1){}
};

//! class representing 2 arguments list

template <class A1, class A2>
class Arg2
{
  public:
  A1 a1;
  A2 a2;

  Arg2(A1 aa1, A2 aa2):a1(aa1),a2(aa2){}
};

//! class representing 3 arguments list

template <class A1, class A2, class A3>
class Arg3
{
  public:
  A1 a1;
  A2 a2;
  A3 a3;

  Arg3(A1 aa1, A2 aa2, A3 aa3):a1(aa1),a2(aa2),a3(aa3){}
};

//! function performing resolution of number of arguments

inline Arg0 CreateCallArgs() {return Arg0();}

//! function performing resolution of number of arguments
template <class A1>
inline Arg1<A1> CreateCallArgs(A1 a1) {return Arg1<A1>(a1);}

//! function performing resolution of number of arguments
template <class A1,class A2>
inline Arg2<A1,A2> CreateCallArgs(A1 a1,A2 a2) {return Arg2<A1,A2>(a1,a2);}

//! function performing resolution of number of arguments
template <class A1,class A2, class A3>
inline Arg3<A1,A2,A3> CreateCallArgs(A1 a1,A2 a2, A3 a3) {return Arg3<A1,A2,A3>(a1,a2,a3);}

//! conversion of return values, intended to convert from void to CallVoidReturn
/*!
needs specialization for void, and (probably due to VC 6 bug)
also identical specializations for all types (see macro RETVALUES_NO_SPEC)
*/
template <class RetType>
struct RetValues
{
  typedef RetType RType;
  typedef RetType RFType;

  static RType GetDefValue() {return RType(0);}

  template <class VehType, class MemberType>
  static RType CallF0CT(const VehType *v, const MemberType *s, RFType (MemberType::*f)(const VehType *v) const)
  {
    return (s->*f)(v);
  }
  template <class VehType, class MemberType>
  static RType CallF0T(VehType *v, MemberType *s, RFType (MemberType::*f)(VehType *v))
  {
    return (s->*f)(v);
  }

  template <class VehType, class MemberType, class A1>
  static RType CallF1CT(const VehType *v, const MemberType *s, RFType (MemberType::*f)(const VehType *v, A1 a1) const, A1 a1)
  {
    return (s->*f)(v,a1);
  }
  template <class VehType, class MemberType, class A1>
  static RType CallF1T(VehType *v, MemberType *s, RFType (MemberType::*f)(VehType *v, A1 a1), A1 a1)
  {
    return (s->*f)(v,a1);
  }

  template <class VehType, class MemberType, class A1, class A2>
  static RType CallF2CT(const VehType *v, const MemberType *s, RFType (MemberType::*f)(const VehType *v, A1 a1, A2 a2) const, A1 a1, A2 a2)
  {
    return (s->*f)(v,a1,a2);
  }
  template <class VehType, class MemberType, class A1, class A2>
  static RType CallF2T(VehType *v, MemberType *s, RFType (MemberType::*f)(VehType *v, A1 a1, A2 a2), A1 a1, A2 a2)
  {
    return (s->*f)(v,a1,a2);
  }

  /* calls that do not expect vehicle to be passed*/
  template <class MemberType>
  static RType CallF0C(const MemberType *s, RetType (MemberType::*f)() const)
  {
    return (s->*f)();
  }
  template <class MemberType>
  static RType CallF0(MemberType *s, RetType (MemberType::*f)())
  {
    return (s->*f)();
  }

  template <class MemberType, class A1>
  static RType CallF1C(const MemberType *s, RetType (MemberType::*f)(A1 a1) const, A1 a1)
  {
    return (s->*f)(a1);
  }
  template <class MemberType, class A1>
  static RType CallF1(MemberType *s, RetType (MemberType::*f)(A1 a1), A1 a1)
  {
    return (s->*f)(a1);
  }

  template <class MemberType, class A1, class A2>
  static RType CallF2C(const MemberType *s, RetType (MemberType::*f)(A1 a1, A2 a2) const, A1 a1, A2 a2)
  {
    return (s->*f)(a1,a2);
  }
  template <class MemberType, class A1, class A2>
  static RType CallF2(MemberType *s, RetType (MemberType::*f)(A1 a1, A2 a2), A1 a1, A2 a2)
  {
    return (s->*f)(a1,a2);
  }
};


template <>
struct RetValues<void>
{
  typedef void RFType;
  typedef CallVoidReturn RType;

  static RType GetDefValue() {return CallVoidReturn();}

  template <class VehType, class MemberType>
  static RType CallF0CT(const VehType *v, const MemberType *s, RFType (MemberType::*f)(const VehType *v) const)
  {
    (s->*f)(v);
    return CallVoidReturn();
  }
  template <class VehType, class MemberType>
  static RType CallF0T(VehType *v, MemberType *s, RFType (MemberType::*f)(VehType *v))
  {
    (s->*f)(v);
    return CallVoidReturn();
  }

  template <class VehType, class MemberType, class A1>
  static RType CallF1CT(const VehType *v, const MemberType *s, RFType (MemberType::*f)(const VehType *v, A1 a1) const, A1 a1)
  {
    (s->*f)(v,a1);
    return CallVoidReturn();
  }
  template <class VehType, class MemberType, class A1>
  static RType CallF1T(VehType *v, MemberType *s, RFType (MemberType::*f)(VehType *v, A1 a1), A1 a1)
  {
    (s->*f)(v,a1);
    return CallVoidReturn();
  }

  template <class VehType, class MemberType, class A1, class A2>
  static RType CallF2CT(const VehType *v, const MemberType *s, RFType (MemberType::*f)(const VehType *v, A1 a1, A2 a2) const, A1 a1, A2 a2)
  {
    (s->*f)(v,a1,a2);
    return CallVoidReturn();
  }
  template <class VehType, class MemberType, class A1, class A2>
  static RType CallF2T(VehType *v, MemberType *s, RFType (MemberType::*f)(VehType *v, A1 a1, A2 a2), A1 a1, A2 a2)
  {
    (s->*f)(v,a1,a2);
    return CallVoidReturn();
  }

   /* calls that do not expect vehicle to be passed*/
  template <class MemberType>
  static RType CallF0C(const MemberType *s, RFType (MemberType::*f)() const)
  {
    (s->*f)();
    return CallVoidReturn();
  }
  template <class MemberType>
  static RType CallF0(MemberType *s, RFType (MemberType::*f)())
  {
    (s->*f)();
    return CallVoidReturn();
  }

  template <class MemberType, class A1>
  static RType CallF1C(const MemberType *s, RFType (MemberType::*f)(A1 a1) const, A1 a1)
  {
    (s->*f)(a1);
    return CallVoidReturn();
  }
  template <class MemberType, class A1>
  static RType CallF1(MemberType *s, RFType (MemberType::*f)(A1 a1), A1 a1)
  {
    (s->*f)(a1);
    return CallVoidReturn();
  }

  template <class MemberType, class A1, class A2>
  static RType CallF2C(const MemberType *s, RFType (MemberType::*f)(A1 a1, A2 a2) const, A1 a1, A2 a2)
  {
    (s->*f)(a1,a2);
    return CallVoidReturn();
  }
  template <class MemberType, class A1, class A2>
  static RType CallF2(MemberType *s, RFType (MemberType::*f)(A1 a1, A2 a2), A1 a1, A2 a2)
  {
    (s->*f)(a1,a2);
    return CallVoidReturn();
  }
};

#define RETVALUES_NO_SPEC(type) \
template <> \
struct RetValues<type> \
{ \
  typedef type RetType; \
 \
  typedef RetType RType; \
 \
  static RType GetDefValue() {return RType(0);} \
 \
  template <class VehType, class MemberType> \
  static RType CallF0CT(const VehType *v, const MemberType *s, RetType (MemberType::*f)(const VehType *v) const) \
  { \
    return (s->*f)(v); \
  } \
  template <class VehType, class MemberType> \
  static RType CallF0T(VehType *v, MemberType *s, RetType (MemberType::*f)(VehType *v)) \
  { \
    return (s->*f)(v); \
  } \
 \
  template <class VehType, class MemberType, class A1> \
  static RType CallF1CT(const VehType *v, const MemberType *s, RetType (MemberType::*f)(const VehType *v, A1 a1) const, A1 a1) \
  { \
    return (s->*f)(v,a1); \
  } \
  template <class VehType, class MemberType, class A1> \
  static RType CallF1T(VehType *v, MemberType *s, RetType (MemberType::*f)(VehType *v, A1 a1), A1 a1) \
  { \
    return (s->*f)(v,a1); \
  } \
 \
  template <class VehType, class MemberType, class A1, class A2> \
  static RType CallF2CT(const VehType *v, const MemberType *s, RetType (MemberType::*f)(const VehType *v, A1 a1, A2 a2) const, A1 a1, A2 a2) \
  { \
    return (s->*f)(v,a1,a2); \
  } \
  template <class VehType, class MemberType, class A1, class A2> \
  static RType CallF2T(VehType *v, MemberType *s, RetType (MemberType::*f)(VehType *v, A1 a1, A2 a2), A1 a1, A2 a2) \
  { \
    return (s->*f)(v,a1,a2); \
  } \
 \
  template <class VehType, class MemberType, class A1, class A2, class A3> \
  static RType CallF3CT(const VehType *v, const MemberType *s, RetType (MemberType::*f)(const VehType *v, A1 a1, A2 a2, A3 a3) const, A1 a1, A2 a2, A3 a3) \
  { \
    return (s->*f)(v,a1,a2,a3); \
  } \
  template <class VehType, class MemberType, class A1, class A2, class A3> \
  static RType CallF3T(VehType *v, MemberType *s, RetType (MemberType::*f)(VehType *v, A1 a1, A2 a2, A3 a3), A1 a1, A2 a2, A3 a3) \
  { \
    return (s->*f)(v,a1,a2,a3); \
  } \
 \
 /* calls that do not expect vehicle to be passed*/ \
  template <class MemberType> \
  static RType CallF0C(const MemberType *s, RetType (MemberType::*f)() const) \
  { \
    return (s->*f)(); \
  } \
  template <class MemberType> \
  static RType CallF0(MemberType *s, RetType (MemberType::*f)()) \
  { \
    return (s->*f)(); \
  } \
 \
  template <class MemberType, class A1> \
  static RType CallF1C(const MemberType *s, RetType (MemberType::*f)(A1 a1) const, A1 a1) \
  { \
    return (s->*f)(a1); \
  } \
  template <class MemberType, class A1> \
  static RType CallF1(MemberType *s, RetType (MemberType::*f)(A1 a1), A1 a1) \
  { \
    return (s->*f)(a1); \
  } \
 \
  template <class MemberType, class A1, class A2> \
  static RType CallF2C(const MemberType *s, RetType (MemberType::*f)(A1 a1, A2 a2) const, A1 a1, A2 a2) \
  { \
    return (s->*f)(a1,a2); \
  } \
  template <class MemberType, class A1, class A2> \
  static RType CallF2(MemberType *s, RetType (MemberType::*f)(A1 a1, A2 a2), A1 a1, A2 a2) \
  { \
    return (s->*f)(a1,a2); \
  } \
 \
  template <class MemberType, class A1, class A2, class A3> \
  static RType CallF3C(const MemberType *s, RetType (MemberType::*f)(A1 a1, A2 a2) const, A1 a1, A2 a2, A3 a3) \
  { \
    return (s->*f)(a1,a2,a3); \
  } \
  template <class MemberType, class A1, class A2, class A3> \
  static RType CallF2(MemberType *s, RetType (MemberType::*f)(A1 a1, A2 a2), A1 a1, A2 a2, A3 a3) \
  { \
    return (s->*f)(a1,a2,a3); \
  } \
};

RETVALUES_NO_SPEC(float)
RETVALUES_NO_SPEC(int)
RETVALUES_NO_SPEC(double)
RETVALUES_NO_SPEC(bool)

//! mapp calls to calls to the member
#define CALL_ARGS(Type,VehType,member) \
  template <class RetType> \
  typename RetValues<RetType>::RType Call##Type \
  ( \
    const VehType *veh, \
    RetType (Type::*f)(const VehType *v) const, Arg0 a, \
    typename RetValues<RetType>::RType def=RetValues<RetType>::GetDefValue() \
  )\
  { \
    return (veh->member) ? RetValues<RetType>::CallF0CT(veh,(Type *)(veh->member),f) : def; \
  } \
  template <class RetType> \
  typename RetValues<RetType>::RType Call##Type \
  ( \
    VehType *veh, \
    RetType (Type::*f)(VehType *v), Arg0 a, \
    typename RetValues<RetType>::RType def=RetValues<RetType>::GetDefValue() \
  ) \
  { \
    return (veh->member) ? RetValues<RetType>::CallF0T(veh,(Type *)(veh->member),f) : def; \
  } \
 \
  template <class RetType,class A1> \
  typename RetValues<RetType>::RType Call##Type \
  ( \
    const VehType *veh, \
    RetType (Type::*f)(const VehType *v, A1 a1) const, Arg1<A1> a, \
    typename RetValues<RetType>::RType def=RetValues<RetType>::GetDefValue() \
  ) \
  { \
    return (veh->member) ? RetValues<RetType>::CallF1CT(veh,(Type *)(veh->member),f,a.a1) : def; \
  } \
 \
  template <class RetType,class A1> \
  typename RetValues<RetType>::RType Call##Type \
  ( \
    VehType *veh, \
    RetType (Type::*f)(VehType *v, A1 a1), Arg1<A1> a, \
    typename RetValues<RetType>::RType def=RetValues<RetType>::GetDefValue() \
  ) \
  { \
    return (veh->member) ? RetValues<RetType>::CallF1T(veh,(Type *)(veh->member),f,a.a1) : def; \
  } \
 \
  template <class RetType,class A1, class A2> \
  typename RetValues<RetType>::RType Call##Type \
  ( \
    const VehType *veh, \
    RetType (Type::*f)(const VehType *v, A1 a1,A2 a2) const, Arg2<A1,A2> a, \
    typename RetValues<RetType>::RType def=RetValues<RetType>::GetDefValue() \
  ) \
  { \
    return (veh->member) ? RetValues<RetType>::CallF2CT(veh,(Type *)(veh->member),f,a.a1,a.a2) : def; \
  } \
  template <class RetType,class A1, class A2> \
  typename RetValues<RetType>::RType Call##Type \
  ( \
    VehType *veh, \
    RetType (Type::*f)(VehType *v, A1 a1,A2 a2), Arg2<A1,A2> a, \
    typename RetValues<RetType>::RType def=RetValues<RetType>::GetDefValue() \
  ) \
  { \
    return (veh->member) ? RetValues<RetType>::CallF2T(veh,(Type *)(veh->member),f,a.a1,a.a2) : def; \
  } \
 \
  template <class RetType,class A1, class A2, class A3> \
  typename RetValues<RetType>::RType Call##Type \
  ( \
    const VehType *veh, \
    RetType (Type::*f)(const VehType *v, A1 a1,A2 a2, A3) const, Arg3<A1,A2,A3> a, \
    typename RetValues<RetType>::RType def=RetValues<RetType>::GetDefValue() \
  ) \
  { \
    return (veh->member) ? RetValues<RetType>::CallF3CT(veh,(Type *)(veh->member),f,a.a1,a.a2,a.a3) : def; \
  } \
  template <class RetType,class A1, class A2, class A3> \
  typename RetValues<RetType>::RType Call##Type \
  ( \
    VehType *veh, \
    RetType (Type::*f)(VehType *v, A1 a1,A2 a2, A3 a3), Arg3<A1,A2,A3> a, \
    typename RetValues<RetType>::RType def=RetValues<RetType>::GetDefValue() \
  ) \
  { \
    return (veh->member) ? RetValues<RetType>::CallF3T(veh,(Type *)(veh->member),f,a.a1,a.a2,a.a3) : def; \
  } \
  /* calls that do not need this to be passed */ \
  template <class RetType> \
  typename RetValues<RetType>::RType Call##Type \
  ( \
    const VehType *veh, \
    RetType (Type::*f)() const, Arg0 a, \
    typename RetValues<RetType>::RType def=RetValues<RetType>::GetDefValue() \
  ) \
  { \
    return (veh->member) ? RetValues<RetType>::CallF0C((Type *)(veh->member),f) : def; \
  } \
  template <class RetType> \
  typename RetValues<RetType>::RType Call##Type \
  ( \
    VehType *veh, \
    RetType (Type::*f)(), Arg0 a, \
    typename RetValues<RetType>::RType def=RetValues<RetType>::GetDefValue() \
  ) \
  { \
    return (veh->member) ? RetValues<RetType>::CallF0((Type *)(veh->member),f) : def; \
  } \
 \
  template <class RetType,class A1> \
  typename RetValues<RetType>::RType Call##Type \
  ( \
    const VehType *veh, \
    RetType (Type::*f)(A1 a1) const, Arg1<A1> a, \
    typename RetValues<RetType>::RType def=RetValues<RetType>::GetDefValue() \
  ) \
  { \
    return (veh->member) ? RetValues<RetType>::CallF1C((Type *)(veh->member),f,a.a1) : def; \
  } \
 \
  template <class RetType,class A1> \
  typename RetValues<RetType>::RType Call##Type \
  ( \
    VehType *veh, \
    RetType (Type::*f)(A1 a1), Arg1<A1> a, \
    typename RetValues<RetType>::RType def=RetValues<RetType>::GetDefValue() \
  ) \
  { \
    return (veh->member) ? RetValues<RetType>::CallF1((Type *)(veh->member),f,a.a1) : def; \
  } \
 \
  template <class RetType,class A1, class A2> \
  typename RetValues<RetType>::RType Call##Type \
  ( \
    const VehType *veh, \
    RetType (Type::*f)(A1 a1,A2 a2) const, Arg2<A1,A2> a, \
    typename RetValues<RetType>::RType def=RetValues<RetType>::GetDefValue() \
  ) \
  { \
    return (veh->member) ? RetValues<RetType>::CallF2C((Type *)(veh->member),f,a.a1,a.a2) : def; \
  } \
  template <class RetType,class A1, class A2> \
  typename RetValues<RetType>::RType Call##Type \
  ( \
    VehType *veh, \
    RetType (Type::*f)(A1 a1,A2 a2), Arg2<A1,A2> a, \
    typename RetValues<RetType>::RType def=RetValues<RetType>::GetDefValue() \
  ) \
  { \
    return (veh->member) ? RetValues<RetType>::CallF2((Type *)(veh->member),f,a.a1,a.a2) : def; \
  } \
 \
  template <class RetType,class A1, class A2, class A3> \
  typename RetValues<RetType>::RType Call##Type \
  ( \
    const VehType *veh, \
    RetType (Type::*f)(A1 a1,A2 a2, A3 a3) const, Arg3<A1,A2,A3> a, \
    typename RetValues<RetType>::RType def=RetValues<RetType>::GetDefValue() \
  ) \
  { \
    return (veh->member) ? RetValues<RetType>::CallF3C((Type *)(veh->member),f,a.a1,a.a2) : def; \
  } \
  template <class RetType,class A1, class A2, class A3> \
  typename RetValues<RetType>::RType Call##Type \
  ( \
    VehType *veh, \
    RetType (Type::*f)(A1 a1,A2 a2, A3 a3), Arg3<A1,A2,A3> a, \
    typename RetValues<RetType>::RType def=RetValues<RetType>::GetDefValue() \
  ) \
  { \
    return (veh->member) ? RetValues<RetType>::CallF3((Type *)(veh->member),f,a.a1,a.a2) : def; \
  }


#define CALLARGS(veh,Type,x,arg) Call##Type(veh,&Type::x,CreateCallArgs arg)
#define CALLARGS_DEF(veh,Type,x,arg,def) Call##Type(veh,&Type::x,CreateCallArgs arg,def)

#endif

