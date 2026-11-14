//-----------------------------------------------------------------------------
// File: enf_baseclass.h
//
// Desc: Base class of BaseVariable definition
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_BASECLASS_H
#define ENF_BASECLASS_H

#include "enf_config.h"
#include "common/enf_basevariable.h"
#include "Common/enf_hashmap.h"
#include "Common/enf_set.h"
#include "engine/enf_item.h"

namespace enf
{

/** @addtogroup Common
@{*/

const uint BaseClassTypeID = 0;
const uint BaseSourceTypeID = 23700;
class BaseSource;
class BaseSourceCollection;

//-----------------------------------------------------------------------
class PUBLIC_API BaseClass: public BaseItem, public CDisableCopy
{
protected:
	Array<BaseVariableDef>	m_VariableDefs;
	HashMap<CStr, uint, CStrHashFuncCS> m_VariableHash;

public:
	ENF_DECLARE_ITEM(BaseClass, BaseItem);

	BaseClass() {}
	virtual ~BaseClass() {}

	ENF_INLINE uint GetNumVariables() const
	{
		return m_VariableDefs.GetCardinality();
	}

	ENF_INLINE const BaseVariableDef& GetVariableDef(uint index) const
	{
		return m_VariableDefs[index];
	}

	ENF_INLINE DataVarType GetVariableType(uint index) const
	{
		return m_VariableDefs[index].m_Type;
	}

	uint GetVariableIndex(CStr name) const;

	const BaseVariableDef* GetVariableDef(CStr name) const
	{
		uint index = GetVariableIndex(name);
		if(index == INDEX_NOT_FOUND)
			return false;
		return &m_VariableDefs[index];
	}
	
	/*!
	This gives opportunity to classes to disable properties, that are not meaningful
	for editing because of setting of other variables
	*/
	virtual bool IsVariableUsed(uint index, const BaseSource* src) const
	{
		return true;
	}

	uint AddVariableDef(const BaseVariableDef& def);
};

//-----------------------------------------------------------------------
class PUBLIC_API BaseSource: public Item
{
protected:
	friend class BaseSourceCollection;
#ifdef _DEBUG
	BaseSourceCollection*	m_pCollection;
#endif
	BaseClass*		m_pClass;
	BaseVariable**	m_pVariables;
	uint				m_iNumVariables;

	bool IsType(uint variable, DataVarType type) const
	{
		enf_assert(variable < m_iNumVariables);
		return (m_pClass->GetVariableType(variable) == type);
	}

	BaseVariable* AllocVariable(uint variable);
	void FreeVariables();

public:
	ENF_DECLARE_SMALLOBJECT(BaseSource);
	ENF_DECLARE_ITEM(BaseSource, Item);

	virtual void CopyData(BaseSource& target) const;

	BaseSource(BaseClass* wclass);
	BaseSource(BaseClass* wclass, CStr name);
	virtual ~BaseSource();

	void SetName(CStr name);

	const BaseClass* GetClass() const
	{
		return m_pClass;
	}

	/*!
	This gives opportunity to classes to disable properties, that are not meaningful
	for editing because of setting of other variables
	*/
	ENF_INLINE bool IsVariableUsed(uint index) const
	{
		return m_pClass != NULL && m_pClass->IsVariableUsed(index, this);
	}
	ENF_INLINE bool IsArray(uint index) const
	{
		return m_pClass != NULL && m_pClass->GetVariableDef(index).IsArray();
	}

	ENF_INLINE uint GetNumVariables() const
	{
		return m_iNumVariables;
	}

	ENF_INLINE bool IsVariableSet(uint variable) const
	{
		enf_assert(variable < m_iNumVariables);
		return (m_pVariables[variable] != NULL);
	}

	void DeleteVariable(uint variable);

	bool SetVariable(uint variable, CStr value);

#define GetXXX(nameSuffix, varType, type)\
	ENF_INLINE type Get ## nameSuffix(uint variable) const {\
		enf_assert(IsType(variable, varType));\
		enf_assert(IsVariableSet(variable));\
		return (type)((const BaseVariable ## nameSuffix*)m_pVariables[variable])->Value;\
	}\
	ENF_INLINE type Get ## nameSuffix ## Def(uint variable, type defvalue) const {\
		enf_assert(IsType(variable, varType));\
		if(!IsVariableSet(variable)) return defvalue;\
		return (type)((const BaseVariable ## nameSuffix*)m_pVariables[variable])->Value;\
	}\
	ENF_INLINE type Get ## nameSuffix(CStr variableName) const {\
		uint variable = GetClass()->GetVariableIndex(variableName); enf_assert(variable != INDEX_NOT_FOUND);\
		enf_assert(IsType(variable, varType));\
		enf_assert(IsVariableSet(variable));\
		return (type)((const BaseVariable ## nameSuffix*)m_pVariables[variable])->Value;\
	}\
	ENF_INLINE type Get ## nameSuffix ## Def(CStr variableName, type defvalue) const {\
		uint variable = GetClass()->GetVariableIndex(variableName); enf_assert(variable != INDEX_NOT_FOUND);\
		enf_assert(IsType(variable, varType));\
		if(!IsVariableSet(variable)) return defvalue;\
		return (type)((const BaseVariable ## nameSuffix*)m_pVariables[variable])->Value;\
	}

	GetXXX(Scalar, DVT_SCALAR, float);
	GetXXX(Integer, DVT_INTEGER, int);
	GetXXX(Boolean, DVT_BOOLEAN, ebool);
	GetXXX(Vector, DVT_VECTOR, Vector4);
	GetXXX(Vector2, DVT_VECTOR2, Vector2);
	GetXXX(Color, DVT_COLOR, FColor);
	GetXXX(String, DVT_STRING, CStr);
	GetXXX(Object, DVT_OBJECT, BaseSource*);

#define GetXXXArray(nameSuffix, varType)\
	ENF_INLINE Array<BaseVariable ## nameSuffix>& Get ## nameSuffix ## Array(uint variable) const {\
	enf_assert(IsType(variable, varType));\
	enf_assert(IsVariableSet(variable));\
	enf_assert(IsArray(variable));\
	return ((BaseVariable ## nameSuffix ## Array*)m_pVariables[variable])->Values;\
	}\
	ENF_INLINE Array<BaseVariable ## nameSuffix>& Get ## nameSuffix ## Array(CStr variableName) const {\
	uint variable = GetClass()->GetVariableIndex(variableName); enf_assert(variable != INDEX_NOT_FOUND);\
	enf_assert(IsType(variable, varType));\
	enf_assert(IsVariableSet(variable));\
	enf_assert(IsArray(variable));\
	return ((BaseVariable ## nameSuffix ## Array*)m_pVariables[variable])->Values;\
	}

	GetXXXArray(Scalar, DVT_SCALAR_ARRAY);
	GetXXXArray(Integer, DVT_INTEGER_ARRAY);
	GetXXXArray(Boolean, DVT_BOOLEAN_ARRAY);
	GetXXXArray(Vector, DVT_VECTOR_ARRAY);
	GetXXXArray(Vector2, DVT_VECTOR2_ARRAY);
	GetXXXArray(Color, DVT_COLOR_ARRAY);
	GetXXXArray(String, DVT_STRING_ARRAY);
	GetXXXArray(Object, DVT_OBJECT_ARRAY);
};



/*!
Collection of source containers
*/
class PUBLIC_API BaseSourceCollection
{
protected:
	friend class BaseSource;

	Set<BaseSource*>	m_Sources;
	HashMap<CStr, BaseSource*, CStrHashFuncCS>	m_SourcesHash;

public:
	ENF_DECLARE_MMOBJECT(BaseSourceCollection);

	BaseSourceCollection();
	virtual ~BaseSourceCollection();

	virtual void ChangeSource(BaseSource* src) {}

		//!Returns number of sources in collection
	uint GetNumSources() const
	{
		return m_Sources.GetCardinality();
	}

	//!Returns source by its index
	BaseSource* GetSource(uint index) const
	{
		return m_Sources[index];
	}

	/*!
	Gets set of all sources in the collection.
	\return
	Set of all MaterialSources.
	*/
	const Set<BaseSource*>& GetSources() const
	{
		return m_Sources;
	}

	/*!
	Finds source by its name.
	\param name
	Name of source we are looking for
	\return
	Pointer to source if it exists. NULL otherwise
	*/
	BaseSource* FindSource(CStr name) const;

	/*!
	Removes source from internal lists and it's destroyed there too.
	\param source
	Source to be removed and destroyed.
	*/
	bool DestroySource(BaseSource* source);

	/*!
	Adds existing source to internal lists and hash-tables. The collection
	is responsible for it and it's destroyed when collection is destroyed too.
	\param source Source to be added.
	*/
	virtual bool AddSource(BaseSource* source);

	/*!
	Removes source from internal lists and hash-tables. Source is not
	destroyed and caller is responsible for him then.
	\param source
	Source we are going to remove
	*/
	bool RemoveSource(BaseSource* source);

	bool RenameSource(BaseSource* source, CStr name);
};



//-----------------------------------------------------------------------
#define DECLARE_INTEGER(var, name, defvalue, uiwidget, rangescale, desc, enums) \
	var = AddVariableDef(BASE_DECLARE_INTEGER(name, defvalue, uiwidget, rangescale, desc, enums))

#define DECLARE_SCALAR(var, name, defvalue, uiwidget, rangescale, desc, enums) \
	var = AddVariableDef(BASE_DECLARE_SCALAR(name, defvalue, uiwidget, rangescale, desc, enums))

#define DECLARE_STRING(var, name, defvalue, desc, enums) \
	var = AddVariableDef(BASE_DECLARE_STRING(name, defvalue, desc, enums))

#define DECLARE_STRING2(var, name, defvalue, uiwidget, desc, enums) \
	var = AddVariableDef(BASE_DECLARE_STRING2(name, defvalue, uiwidget, desc, enums))

#define DECLARE_FONT(var, name, defvalue, desc, enums) \
   var = AddVariableDef(BASE_DECLARE_STRING2(name, defvalue, "font", desc, enums))

#define DECLARE_LOCALE_STRING(var, name, defvalue, desc, enums) \
	var = AddVariableDef(BASE_DECLARE_LOCALE_STRING(name, defvalue, desc, enums))

#define DECLARE_VECTOR(var, name, defvalue, uiwidget, desc, enums) \
	var = AddVariableDef(BASE_DECLARE_VECTOR(name, defvalue, uiwidget, desc, enums))

#define DECLARE_VECTOR2(var, name, defvalue, uiwidget, desc, enums) \
	var = AddVariableDef(BASE_DECLARE_VECTOR2(name, defvalue, uiwidget, desc, enums))

#define DECLARE_COLOR(var, name, defvalue, desc, enums) \
	var = AddVariableDef(BASE_DECLARE_COLOR(name, defvalue, desc, enums))

#define DECLARE_BOOLEAN(var, name, defvalue, desc) \
	var = AddVariableDef(BASE_DECLARE_BOOLEAN(name, defvalue, desc))

#define DECLARE_OBJECT(var, name, desc) \
	var = AddVariableDef(BASE_DECLARE_OBJECT(name, NULL, desc))

#define DECLARE_FILE_PATH(var, name, desc) \
	var = AddVariableDef(BASE_DECLARE_FILE_PATH(name, desc))

//@}

}
#endif //ENF_BASECLASS_H