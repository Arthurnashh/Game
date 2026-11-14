#pragma once

#ifndef tupleToGameArray_h__
#define tupleToGameArray_h__

#include <El/Evaluator/expressImpl.hpp>
#include <tuple>

template <class Type> __forceinline GameValue ToGameValueFunc(const Type &x) {return ToGameValue<Type>::Do(x);}

template <class Type> struct ToGameValue {static inline GameValue Do(const Type &x) {return GameValue(x);}};

template <class Type> struct ToGameValue<Type *> {static inline GameValue Do(Type * const &x){return GameValueExt(x,GameValExtObject);}};

template <> struct ToGameValue<Vector3> {static inline GameValue Do(Vector3 const &x){GameArrayType arr;arr.Add(x[0]);arr.Add(x[2]);arr.Add(x[1]);return arr;}};

//template <> struct GameValue ToGameValue<EntityAI *>(EntityAI * const &x);
//template <> struct GameValue ToGameValue<Person *>(Person * const &x);

template <class Type> struct FromGameValue { static inline Type Do(const GameValue &x) {return safe_cast<Type>(x);}};

template <> struct FromGameValue<bool> { static inline bool Do(const GameValue &x) {return !x.GetNil() && x.GetType() == GameBool && x;}};

template <> struct FromGameValue<Vector3>
{
  static inline Vector3 Do(const GameValue &x)
  {
    if (x.GetNil() || x.GetType() != GameArray) return VZero;

    const GameArrayType &vecArray = x;
    if (vecArray.Size() != 3) return VZero;

    return Vector3(vecArray[0], vecArray[2], vecArray[1]);
  }
};

//@{ iterate through a tuple using meta programming
template <class InputTuple, int i>
struct TupleToGameArray
{
  static void __forceinline Do(const InputTuple &input, GameArrayType &arguments)
  {
    TupleToGameArray<InputTuple,i-1>::Do(input,arguments);
    arguments.Add(ToGameValueFunc(std::get<i-1>(input)));
  }
};

template <class InputTuple>
struct TupleToGameArray<InputTuple,0>
{
  static void __forceinline Do(const InputTuple &input, GameArrayType &arguments) {}
};
//@}



#endif // tupleToGameArray_h__
