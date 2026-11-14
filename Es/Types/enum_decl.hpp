#ifdef _MSC_VER
#pragma once
#endif

#ifndef __ENUM_DECL_HPP
#define __ENUM_DECL_HPP

/*!
\file
Portable enum forward declaration.
*/

#if _MSC_VER>=1700 || defined __GNUC__ && __VERSION__>=40600

#define DECL_ENUM(Name) enum Name: int;

#define DEFINE_ENUM_BEG(Name) enum Name: int {
#define DEFINE_ENUM_END(Name) };

#define CAST_TO_ENUM(Type,Val) Val

#elif _MSC_VER && !NO_ENUM_FORWARD_DECLARATION

// Microsoft compiler supports enum forward declaration
#define DECL_ENUM(Name) enum Name;

#define DEFINE_ENUM_BEG(Name) enum Name {
#define DEFINE_ENUM_END(Name) };

#define CAST_TO_ENUM(Type,Val) Val

#elif !NO_UNDEF_ENUM_REFERENCE

	#include <limits.h>

	// workaround - forward declare using class enum_base

	class enum_base
	{
		int _val;

		public:
		operator int() const {return _val;}
		explicit enum_base( int val ){_val=val;}
		enum_base(){}
	};

	#define DECL_ENUM(Name) \
		class ClassHelperEnum_##Name; \
		class Name: public enum_base \
		{ \
			public: \
			Name():enum_base(){} \
			explicit Name( int val ):enum_base(val){} \
			Name( const enum Name##_Enum &val ); \
		};

	#define DEFINE_ENUM_BEG(Name) \
		enum Name##_Enum \
		{

	#define DEFINE_ENUM_END(Name) \
		}; \
		class ClassHelperEnum_##Name: public enum_base \
		{ \
			public: \
			ClassHelperEnum_##Name(){} \
			ClassHelperEnum_##Name( Name##_Enum val ):enum_base(val){} \
		}; \
		inline Name::Name( const Name##_Enum &val ):enum_base(val){}

#define CAST_TO_ENUM(Type,Val) (Type)(Val)

#else

	#include <limits.h>

	// workaround - forward declare using class enum_base

	class enum_base
	{
		int _val;

		public:
		operator int() const {return _val;}
		explicit enum_base( int val ){_val=val;}
		enum_base(){}
	};

	// declare enum - may be used only once
	#define DECL_ENUM(Name) \
		class ClassHelperEnum_##Name; \
		class Name: public enum_base \
		{ \
			public: \
			typedef ClassHelperEnum_##Name Helper; \
			Name():enum_base(){} \
			explicit Name( int val ):enum_base(val){} \
			Name(const Name &a):enum_base(a){} \
			template <class Type> \
			Name(Type x) \
			:enum_base(ClassHelperEnum_##Name(x)) {} \
		};

	//! define enum and its values - DECLARE_ENUM must be used before
	#define DEFINE_ENUM_BEG(Name) \
		enum Name##_Enum \
		{

	#define DEFINE_ENUM_END(Name) \
		}; \
		class ClassHelperEnum_##Name: public enum_base \
		{ \
			public: \
			typedef enum Name##_Enum EnumType; \
			ClassHelperEnum_##Name(){} \
			ClassHelperEnum_##Name( Name##_Enum val ):enum_base(val){} \
		};

		#if REMARK_EXAMPLE
			// typical usage example
			DECL_ENUM(SomeEnum)
			DEFINE_ENUM_BEG(SomeEnum)
				SomeEnumValue0,
				SomeEnumValue1,
				SomeEnumValue2,
				SomeEnumValue3,
			DEFINE_ENUM_END(SomeEnum)
		#endif

#define CAST_TO_ENUM(Type,Val) (Type)(Val)

#endif

#endif
