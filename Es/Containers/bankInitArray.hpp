#ifdef _MSC_VER
#pragma once
#endif

#ifndef _BANK_INIT_ARRAY_HPP
#define _BANK_INIT_ARRAY_HPP

#include <string.h>
#include <Es/Framework/debugLog.hpp>
#include <Es/Containers/array.hpp>
#include <Es/Containers/bankArray.hpp>

template <class TTraits>
struct DefBankInitTraitsExt: public DefBankTraitsExt<TTraits>
{
  typedef typename TTraits::Type Type;
  typedef typename TTraits::NameType NameType;
  /// create an object based on the name
  static Type *Create(NameType name)
  {
    Type *item = new Type;
    item->Init(name);
    return item;
  }
};

template <class TTraits>
struct BankInitTraitsExt: public DefBankInitTraitsExt<TTraits>
{
};

template <class Type,class Traits=BankTraits<Type>, class ExtTraits=BankInitTraitsExt<Traits> >
class BankInitArray: public BankArray<Type,Traits,ExtTraits>
{
};

#endif
