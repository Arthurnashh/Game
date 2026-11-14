//-----------------------------------------------------------------------------
// File: enf_basevariable.h
//
// Desc: Base for definition of data descriptors and containers
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_BASEVARIABLE_H
#define ENF_BASEVARIABLE_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_color.h"
#include "common/enf_mathlib.h"
#include "common/enf_mathlib2d.h"
#include "common/enf_strings.h"

namespace enf
{
class BaseSource;

//-----------------------------------------------------------------------
/*!
Type of variable. Used by enf::MaterialClass, enf::Material,
enf::MaterialSource, enf::MaterialSystem, enf::WidgetProperties,
enf::SoundShaderDef etc
*/
enum DataVarType
{
	DVT_ARRAY = 0x80000000,
	//! Unknown type
	DVT_NULL = 0,
	//! Float type
	DVT_SCALAR,
	//! Vector2 type
	DVT_VECTOR2,
	//! Vector3 type
	DVT_VECTOR3,
	//! Vector4 type
	DVT_VECTOR,
	//! Matrix44 type
	DVT_MATRIX,
	//! Matrix34 type
	DVT_MATRIX34,
	//! FColor type
	DVT_COLOR,
	//! Int type
	DVT_INTEGER,
	//! TextureMap type
	DVT_TEXTURE,
	//! Boolean type
	DVT_BOOLEAN,
	//! String type.
	DVT_STRING,
	//! Pointer type.
	DVT_POINTER,
	//! Flag-set type. Used only for MaterialSource ATM
	DVT_FLAGS,
	//! Object, derived from BaseSource
	DVT_OBJECT,

	DVT_SCALAR_ARRAY = DVT_SCALAR|DVT_ARRAY,
	DVT_VECTOR2_ARRAY = DVT_VECTOR2|DVT_ARRAY,
	DVT_VECTOR3_ARRAY = DVT_VECTOR3|DVT_ARRAY,
	DVT_VECTOR_ARRAY = DVT_VECTOR|DVT_ARRAY,
	DVT_MATRIX_ARRAY = DVT_MATRIX|DVT_ARRAY,
	DVT_MATRIX34_ARRAY = DVT_MATRIX34|DVT_ARRAY,
	DVT_COLOR_ARRAY = DVT_COLOR|DVT_ARRAY,
	DVT_INTEGER_ARRAY = DVT_INTEGER|DVT_ARRAY,
	DVT_TEXTURE_ARRAY = DVT_TEXTURE|DVT_ARRAY,
	DVT_BOOLEAN_ARRAY = DVT_BOOLEAN|DVT_ARRAY,
	DVT_STRING_ARRAY = DVT_STRING|DVT_ARRAY,
	DVT_POINTER_ARRAY = DVT_POINTER|DVT_ARRAY,
	DVT_FLAGS_ARRAY = DVT_FLAGS|DVT_ARRAY,
	DVT_OBJECT_ARRAY = DVT_OBJECT|DVT_ARRAY

};

struct BaseVariable
{
	ENF_DECLARE_SMALLOBJECT(BaseVariable);
	ENF_INLINE static void Delete(DataVarType type, BaseVariable* tvar);
};

struct BaseVariableArray: public BaseVariable
{
	ENF_DECLARE_SMALLOBJECT(BaseVariableArray);
	ENF_INLINE static void Delete(DataVarType type, BaseVariable* tvar);
};

//!Value of float type variable
struct BaseVariableScalar: public BaseVariable
{
	float		Value;
	BaseVariableScalar(): Value(0.0f) {}
	BaseVariableScalar(float value ): Value(value) {}
};

//!Value of int type variable
struct BaseVariableInteger: public BaseVariable
{
	int		Value;
	BaseVariableInteger(): Value(0) {}
	BaseVariableInteger(int value ): Value(value) {}
};

//!Value of Vector2 type variable
struct BaseVariableVector2: public BaseVariable
{
	Vector2		Value;
	BaseVariableVector2(): Value(0.0f, 0.0f) {}
};

//!Value of Vector3 type variable
struct BaseVariableVector3: public BaseVariable
{
	Vector3		Value;
	BaseVariableVector3(): Value(0.0f, 0.0f, 0.0f) {}
};

//!Value of Vector4 type variable
struct BaseVariableVector: public BaseVariable
{
	Vector4		Value;
	BaseVariableVector(): Value(0.0f, 0.0f, 0.0f, 0.0f) {}
};

//!Value of FColor type variable
struct BaseVariableColor: public BaseVariable
{
	FColor		Value;
	BaseVariableColor(): Value(0.0f, 0.0f, 0.0f, 0.0f) {}
};

//!Value of Matrix44 type variable
struct BaseVariableMatrix: public BaseVariable
{
	Matrix44		Value;
	BaseVariableMatrix(): Value(true) {}
};

//!Value of Matrix34 type variable
struct BaseVariableMatrix34: public BaseVariable
{
	Matrix34		Value;
	BaseVariableMatrix34(): Value(true) {}
};

//!Value of bool type variable
struct BaseVariableBoolean: public BaseVariable
{
	ebool		Value;
	BaseVariableBoolean(): Value(false) {}
	BaseVariableBoolean(bool value ): Value(value) {}
};

//!Value of string type variable
struct BaseVariableString: public BaseVariable
{
	DString Value;
	BaseVariableString() {}
	BaseVariableString(CStr value ): Value(value) {}
};

//!Value of pointer type variable
struct BaseVariablePointer: public BaseVariable
{
	void* Value;
	BaseVariablePointer(): Value(NULL) {}
};

//!Value of object type variable
struct BaseVariableObject: public BaseVariable
{
	BaseSource* Value;
	BaseVariableObject(): Value(NULL) {}
};


//!Value of float-array type variable
struct BaseVariableScalarArray: public BaseVariableArray
{
	Array<BaseVariableScalar>	Values;
};

//!Value of int-array type variable
struct BaseVariableIntegerArray: public BaseVariableArray
{
	Array<BaseVariableInteger>	Values;
};

//!Value of Vector2-array type variable
struct BaseVariableVector2Array: public BaseVariableArray
{
	Array<BaseVariableVector2>	Values;
};

//!Value of Vector3-array type variable
struct BaseVariableVector3Array: public BaseVariableArray
{
	Array<BaseVariableVector3>	Values;
};

//!Value of Vector4-array type variable
struct BaseVariableVectorArray: public BaseVariableArray
{
	Array<BaseVariableVector>	Values;
};

//!Value of FColor-array type variable
struct BaseVariableColorArray: public BaseVariableArray
{
	Array<BaseVariableColor>	Values;
};

//!Value of Matrix44-array type variable
struct BaseVariableMatrixArray: public BaseVariableArray
{
	Array<BaseVariableMatrix> Values;
};

//!Value of Matrix34-array type variable
struct BaseVariableMatrix34Array: public BaseVariableArray
{
	Array<BaseVariableMatrix34> Values;
};

//!Value of bool-array type variable
struct BaseVariableBooleanArray: public BaseVariableArray
{
	Array<BaseVariableBoolean> Values;
};

//!Value of string-array type variable
struct BaseVariableStringArray: public BaseVariableArray
{
	Array<BaseVariableString> Values;
};

//!Value of pointer-array type variable
struct BaseVariablePointerArray: public BaseVariableArray
{
	Array<BaseVariablePointer> Values;
};

//!Value of object-array type variable
struct BaseVariableObjectArray: public BaseVariableArray
{
	Array<BaseVariableObject> Values;
};


/*!
Enumeration of predefined parameter values.
Must be retyped according to enf::DataVarType
*/
struct ParmEnum
{
	CStr	Name;
	CStr	Description;

	ParmEnum(CStr name): Name(name) {}
	ParmEnum(CStr name, CStr desc): Name(name), Description(desc) {}
};

//!Enumerated value of integer parameter
struct ParmEnumInteger: ParmEnum
{
	int		Value;
	ParmEnumInteger(CStr name, int value): ParmEnum(name), Value(value) {}
	ParmEnumInteger(CStr name, int value, CStr desc): ParmEnum(name, desc), Value(value) {}
	ParmEnumInteger(): ParmEnum(NULL, NULL), Value(0) {}
};

//!Enumerated value of scalar parameter
struct ParmEnumScalar: ParmEnum
{
	float		Value;
	ParmEnumScalar(CStr name, float value): ParmEnum(name), Value(value) {}
	ParmEnumScalar(CStr name, float value, CStr desc): ParmEnum(name, desc), Value(value) {}
};

//!Enumerated value of color parameter
struct ParmEnumColor: ParmEnum
{
	FColor	Value;
	ParmEnumColor(CStr name, const FColor& value): ParmEnum(name), Value(value) {}
	ParmEnumColor(CStr name, const FColor& value, CStr desc): ParmEnum(name, desc), Value(value) {}
};

/*!
Variable definition for BaseVariable containers. Its purpose is to be
public even for material tools thus some additional informations are there
(UserInterface type, Description etc). All strings and enums must be allocated
and kept by user!
*/
struct PUBLIC_API BaseVariableDef
{
	//! Type of variable. ie DVT_COLOR
	DataVarType	m_Type;

	//! Index of variable. Can be used for quick referencing variable
	uint			m_iIndex;

	//! The name of variable. ie "speculartint"
	CStr			m_strName;

	//!Default value, visible in tools
	CStr			m_strDefault;

	//!Type of UI element. ie "texture", "color", "combobox", "editbox", "checkbox", "slider", "flags"
	CStr			m_strUIWidget;

	//! Range and scale of slider element. Defined as "min max scale"
	CStr			m_strRangeScale;

	//! Short documentation, used for tools
	CStr			m_strDescription;

	/*!
	Array of enum values. Used by tools as source for combo-boxes or
	collections of flags. It's also used for conversions. This pointer
	must be retyped to correct	type(ParmEnumInt/ParmEnumFloat...)
	End or array is marked by NULL pointer.
	*/
	const ParmEnum**	m_pEnums;

	BaseVariableDef(DataVarType type, CStr name, CStr defvalue, CStr widget, CStr rangescale = NULL, CStr desc = NULL, const ParmEnum** enums = NULL):
		m_Type(type),
		m_strName(name),
		m_strDefault(defvalue),
		m_strUIWidget(widget),
		m_strRangeScale(rangescale),
		m_strDescription(desc),
		m_iIndex(0),
		m_pEnums(enums)
	{
	}
	bool FindEnumInteger(CStr value, int& ret) const;
	const char* FindEnumInteger(int value) const;
	bool FindEnumScalar(CStr value, float& ret) const;
	const char* FindEnumScalar(float value) const;
	bool FindEnumColor(CStr value, FColor& ret) const;
	const ParmEnum* FindEnum(CStr value) const;

	bool IsArray() const
	{
		return (m_Type & DVT_ARRAY) != 0;
	}

	DataVarType Type() const
	{
		return (DataVarType)(m_Type & ~DVT_ARRAY);
	}

	BaseVariableDef():
		m_Type(DVT_NULL),
		m_strName(NULL),
		m_strUIWidget(NULL),
		m_strDescription(NULL),
		m_iIndex(0),
		m_pEnums(NULL)
	{
	}
	void CopyStatic(BaseVariableDef& target) const;
};

PUBLIC_API const char* VariableTypeToString(DataVarType type);
#define DEFINE_PARAM(var)					static enf::uint var;
#define DECLARE_PARAM(cls, var)			enf::uint cls :: var;
/*
class DataDescription
{
protected:

	HashMap<
public:
};
*/
ENF_INLINE void BaseVariable::Delete(DataVarType type, BaseVariable* tvar)
{
	switch(type)
	{
		//	DVT_TEXTURE,
		//	DVT_FLAGS,
	case DVT_SCALAR:
		delete (BaseVariableScalar*)tvar;
		break;
	case DVT_INTEGER:
		delete (BaseVariableInteger*)tvar;
		break;
	case DVT_VECTOR:
		delete (BaseVariableVector*)tvar;
		break;
	case DVT_VECTOR2:
		delete (BaseVariableVector2*)tvar;
		break;
	case DVT_VECTOR3:
		delete (BaseVariableVector3*)tvar;
		break;
	case DVT_MATRIX:
		delete (BaseVariableMatrix*)tvar;
		break;
	case DVT_MATRIX34:
		delete (BaseVariableMatrix34*)tvar;
		break;
	case DVT_STRING:
		delete (BaseVariableString*)tvar;
		break;
	case DVT_COLOR:
		delete (BaseVariableColor*)tvar;
		break;
	case DVT_BOOLEAN:
		delete (BaseVariableBoolean*)tvar;
		break;
	case DVT_POINTER:
		delete (BaseVariablePointer*)tvar;
		break;
	case DVT_OBJECT:
		delete (BaseVariableObject*)tvar;
		break;
	}
}

ENF_INLINE void BaseVariableArray::Delete(DataVarType type, BaseVariable* tvar)
{
	switch(type)
	{
		//	DVT_TEXTURE,
		//	DVT_FLAGS,
	case DVT_SCALAR_ARRAY:
		delete (BaseVariableScalarArray*)tvar;
		break;
	case DVT_INTEGER_ARRAY:
		delete (BaseVariableIntegerArray*)tvar;
		break;
	case DVT_VECTOR_ARRAY:
		delete (BaseVariableVectorArray*)tvar;
		break;
	case DVT_VECTOR2_ARRAY:
		delete (BaseVariableVector2Array*)tvar;
		break;
	case DVT_VECTOR3_ARRAY:
		delete (BaseVariableVector3Array*)tvar;
		break;
	case DVT_MATRIX_ARRAY:
		delete (BaseVariableMatrixArray*)tvar;
		break;
	case DVT_MATRIX34_ARRAY:
		delete (BaseVariableMatrix34Array*)tvar;
		break;
	case DVT_STRING_ARRAY:
		delete (BaseVariableStringArray*)tvar;
		break;
	case DVT_COLOR_ARRAY:
		delete (BaseVariableColorArray*)tvar;
		break;
	case DVT_BOOLEAN_ARRAY:
		delete (BaseVariableBooleanArray*)tvar;
		break;
	case DVT_POINTER_ARRAY:
		delete (BaseVariablePointerArray*)tvar;
		break;
	case DVT_OBJECT_ARRAY:
		delete (BaseVariableObjectArray*)tvar;
		break;
	}
}


#define BASE_DECLARE_INTEGER(name, defvalue, uiwidget, rangescale, desc, enums) \
	enf::BaseVariableDef(enf::DVT_INTEGER, name, defvalue, uiwidget, rangescale, desc, enums)

#define BASE_DECLARE_SCALAR(name, defvalue, uiwidget, rangescale, desc, enums) \
	enf::BaseVariableDef(enf::DVT_SCALAR, name, defvalue, uiwidget, rangescale, desc, enums)

#define BASE_DECLARE_STRING(name, defvalue, desc, enums) \
	enf::BaseVariableDef(enf::DVT_STRING, name, defvalue, "editbox", NULL, desc, enums)

#define BASE_DECLARE_STRING2(name, defvalue, uiwidget, desc, enums) \
	enf::BaseVariableDef(enf::DVT_STRING, name, defvalue, uiwidget, NULL, desc, enums)

#define BASE_DECLARE_LOCALE_STRING(name, defvalue, desc, enums) \
	enf::BaseVariableDef(enf::DVT_STRING, name, defvalue, "localeeditbox", NULL, desc, enums)

#define BASE_DECLARE_VECTOR(name, defvalue, uiwidget, desc, enums) \
	enf::BaseVariableDef(enf::DVT_VECTOR, name, defvalue, uiwidget, NULL, desc, enums)

#define BASE_DECLARE_VECTOR2(name, defvalue, uiwidget, desc, enums) \
	enf::BaseVariableDef(enf::DVT_VECTOR2, name, defvalue, uiwidget, NULL, desc, enums)

#define BASE_DECLARE_COLOR(name, defvalue, desc, enums) \
	enf::BaseVariableDef(enf::DVT_COLOR, name, defvalue, "color", NULL, desc, enums)

#define BASE_DECLARE_BOOLEAN(name, defvalue, desc) \
	enf::BaseVariableDef(enf::DVT_BOOLEAN, name, defvalue, "checkbox", NULL, desc)

#define BASE_DECLARE_OBJECT(name, desc) \
	enf::BaseVariableDef(enf::DVT_OBJECT, name, NULL, "object", NULL, desc)

#define BASE_DECLARE_FILE_PATH(name, desc) \
	enf::BaseVariableDef(enf::DVT_STRING, name, NULL, "fileeditbox", NULL, desc)
}

#endif //ENF_BASEVARIABLE_H