#ifdef _MSC_VER
#pragma once
#endif

#ifndef _BANK_ARRAY_HPP
#define _BANK_ARRAY_HPP

#include <string.h>
#include <Es/Framework/debugLog.hpp>
#include <Es/Types/pointers.hpp>
#include <Es/Types/removeLinks.hpp>
#include <Es/Types/lLinks.hpp>
#include <Es/Containers/array.hpp>

// default traits (used for most types)
template <class TType>
struct DefBankTraits
{
  typedef TType Type;

  //@{ declarations based on Type
	static void ReuseCachedItem(const Type *item){}
	/// default container is RefArray
	typedef RefArray<Type> ContainerType;
	//@}

	// default name is character
	typedef const char *NameType;
	// default name comparison
	static int CompareNames( NameType n1, NameType n2 )
	{
		return strcmpi(n1,n2);
	}
};

template <class TTraits>
struct DefBankTraitsExt
{
  typedef TTraits Traits;
  typedef typename Traits::Type Type;
  typedef typename Traits::NameType NameType;
  typedef typename Traits::ContainerType ContainerType;
	/// create an object based on the name
  static Type *Create(NameType name) {return new Type(name);}
  /// get a name
	static NameType GetName(const Type *item) {return item->GetName();}
	/// perform a search in the container
  static Type *Find(const ContainerType &container, NameType name)
  {
	  for( int i=0; i<container.Size(); i++ )
	  {
		  Type *ii = container.Get(i);
		  if( ii && !Traits::CompareNames(GetName(ii),name) ) return ii;
	  }
	  return NULL;
  }

	/// add an item into the container
  static void AddItem(ContainerType &container, Type *item)
  {
	  container.AddReusingNull(item);
	}
	/// delete an item with given name
	/** @return false if not found */
	static bool DeleteItem(ContainerType &container, NameType name)
	{
	  for( int i=0; i<container.Size(); i++ )
	  {
		  Type *ii = container.Get(i);
		  if( ii && !Traits::CompareNames(GetName(ii),name) )
		  {
		    container.Delete(i);
		    return true;
		  }
	  }
	  return false;
	}
};

/// similar to DefBankTraits, but using Link instead of Ref
template <class Type>
struct DefLinkBankTraits: public DefBankTraits<Type>
{
	/// default container is RefArray
	typedef LinkArray<Type> ContainerType;
};

/// similar to DefBankTraits, but using LLink instead of Ref
template <class Type>
struct DefLLinkBankTraits: public DefBankTraits<Type>
{
	/// default container is RefArray
	typedef LLinkArray<Type> ContainerType;
};

/// typical implementation inherits from default traits
/** When creating specialization, inheritance from DefBankTraits<Type> may be used
if overriding only some members
*/
template <class Type>
struct BankTraits: public DefBankTraits<Type>
{
};

/// typical ext. implementation inherits from default traits
/** this avoids necessity to redefine dependent declarations when changing types in BankTraits
*/
template <class Type>
struct BankTraitsExt: public DefBankTraitsExt<Type>
{
};


//template <class Type,class ExtTraits=BankTraitsExt< BankTraits<Type> > >

template <class Type, class Traits=BankTraits<Type>, class ExtTraits=BankTraitsExt<Traits> >
class BankArray: public Traits::ContainerType, protected ExtTraits
{
	typedef typename Traits::NameType TypeNameType;
	typedef typename Traits::ContainerType base;

	protected:
  typedef Traits BankArrayTraits;
  typedef ExtTraits BankArrayTraitsExt;
  
	Type *Find( TypeNameType name ) const;
	Type *Load( TypeNameType name );
	
	public:
	Type *New( TypeNameType name );
	bool Delete(Type *type)
	{
	  TypeNameType name = ExtTraits::GetName(type);
	  return ExtTraits::DeleteItem(*this,name);
	}
};

template <class Type,class Traits,class ExtTraits>
Type *BankArray<Type,Traits,ExtTraits>::Find( TypeNameType name ) const
{
  return ExtTraits::Find(*this,name);
}

template <class Type,class Traits,class ExtTraits>
Type *BankArray<Type,Traits,ExtTraits>::Load( TypeNameType name )
{
  Type *item = ExtTraits::Create(name);
  if (item)
  {
    ExtTraits::AddItem(*this,item);
  }
  return item;
}

template <class Type,class Traits, class ExtTraits>
Type *BankArray<Type,Traits,ExtTraits>::New( TypeNameType name )
{
	Type *item = Find(name);
	if(item)
	{
  	return item;
	}
	return Load(name);
}

#endif
