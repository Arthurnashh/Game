#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ENUM_NAMES_HPP
#define _ENUM_NAMES_HPP

//#include "wpch.hpp"
#include <Es/Strings/rString.hpp>
#include <Es/Types/enum_decl.hpp>

extern int DUPLICATE_IDS;

struct EnumName
{
	int value;
	RStringB name;
  int *ids;

	EnumName(int v, RStringB n, int *i = NULL) {value=v, name=n, ids=i;}
	EnumName() {value = -1; ids = NULL;}

	const char *GetKey() const {return name;}
	RStringB GetName() const {return name;}
	int GetValue() const {return value;}
  int GetIDS() const {return ids ? *ids : -1;}
	bool IsValid() const {return name.GetLength()>0;}
};
TypeIsMovableZeroed(EnumName)

int GetEnumValue( const EnumName *names, const RStringB &name );
int GetEnumValue( const EnumName *names, const char *name );
const RStringB &GetEnumName( const EnumName *names, int value );

template <class Type>
const EnumName *GetEnumNames(Type value);

template <class Type>
Type GetEnumCount(Type value);

template <class Type>
Type GetEnumValue(const RStringB &name)
{
	Type dummy=(Type)0;
	return (Type)GetEnumValue(GetEnumNames(dummy),name);
}

template <class Type>
Type GetEnumValue(const char *name)
{
	Type dummy=(Type)0;
	return (Type)GetEnumValue(GetEnumNames(dummy),name);
}

DECL_RSB_EXT(ERROR)

template <class Type>
const RStringB &FindEnumName(Type value)
{
	const EnumName *names = GetEnumNames(value);
	for (int i=0; names[i].IsValid(); i++)
	{
		if (names[i].value == value) return names[i].name;
	}
	return RSB(ERROR);
}

// enum factory
// enums with names can be created easily using this factory
/* example: 
// enum definiton macro:
#define ON_OFF_ENUM(type,prefix,XX) \
	XX(type, prefix, Off) \
	XX(type, prefix, On)

// call to enum factory using this macro:
DECLARE_DEFINE_ENUM(OnOff,O,ON_OFF_ENUM)
*/

#define ENUM_VALUE(type,typeprefix,name) \
	typeprefix##name,

#define ENUM_NAME(type,typeprefix,name) \
	EnumName(typeprefix##name,#name),

//! use enum-definition macro to declare enum names and define enum values
#define DECLARE_ENUM(type,prefix,ENUM_DEF) \
  DEFINE_ENUM_BEG(type) \
    ENUM_DEF(type,prefix,ENUM_VALUE) \
    N##type \
  DEFINE_ENUM_END(type) \
	template <> \
  const EnumName *GetEnumNames(type dummy); \

//! use enum-definition macro to declare enum names and define enum values - static version
#define DECLARE_ENUM_STATIC(type,prefix,ENUM_DEF) \
  DEFINE_ENUM_BEG(type) \
  ENUM_DEF(type,prefix,ENUM_VALUE) \
  N##type \
  DEFINE_ENUM_END(type) \
  template <> \
  static const EnumName *GetEnumNames(type dummy); \

//! use enum-definition macro to define enum names
#define DEFINE_ENUM(type,prefix,ENUM_DEF) \
	static const EnumName type##Names[]= \
	{ \
		ENUM_DEF(type,prefix,ENUM_NAME) \
		EnumName() \
	}; \
	template <> \
	const EnumName *GetEnumNames(type dummy) {return type##Names;}\
	\
	template <> \
	type GetEnumCount(type value) {return N##type;}

//! use enum-definition macro to define enum names - static version
#define DEFINE_ENUM_STATIC(type,prefix,ENUM_DEF) \
  static const EnumName type##Names[]= \
  { \
    ENUM_DEF(type,prefix,ENUM_NAME) \
    EnumName() \
  }; \
  template <> \
  static const EnumName *GetEnumNames(type dummy) {return type##Names;}\
  \
  template <> \
  static type GetEnumCount(type value) {return N##type;}

#define DECLARE_DEFINE_ENUM(type,prefix,def) \
	DECLARE_ENUM(type,prefix,def) \
	DEFINE_ENUM(type,prefix,def) \

#define DECLARE_DEFINE_ENUM_STATIC(type,prefix,def) \
  DECLARE_ENUM_STATIC(type,prefix,def) \
  DEFINE_ENUM_STATIC(type,prefix,def) \

#endif
