//-----------------------------------------------------------------------------
// File: enf_baseclass.cpp
//
// Desc: 
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "common/enf_baseclass.h"

namespace enf
{
//-----------------------------------------------------------------------
uint BaseClass::AddVariableDef(const BaseVariableDef& def)
{
	uint index = m_VariableDefs.GetCardinality();
	BaseVariableDef& ndef = m_VariableDefs.Append();
	def.CopyStatic(ndef);
	ndef.m_iIndex = index;
	m_VariableHash.Insert(ndef.m_strName, index);

	return index;
}

//-----------------------------------------------------------------------
void BaseVariableDef::CopyStatic(BaseVariableDef& target) const
{
	target.m_Type = m_Type;
	target.m_strName = m_strName.Static();
	target.m_strDefault = m_strDefault.Static();
	target.m_strUIWidget = m_strUIWidget.Static();
	target.m_strRangeScale = m_strRangeScale.Static();
	target.m_strDescription = m_strDescription.Static();
	target.m_pEnums = m_pEnums;
}


//-----------------------------------------------------------------------
uint BaseClass::GetVariableIndex(CStr name) const
{
	uint index;
	if(!m_VariableHash.Find(name, index))
		return INDEX_NOT_FOUND;
	return index;
}


//-----------------------------------------------------------------------
void BaseSource::CopyData(BaseSource& target) const
{
	target.m_strName = m_strName;
	target.FreeVariables();
	target.m_pClass = m_pClass;
	target.m_iNumVariables = m_iNumVariables;
	target.m_pVariables = ENF_NEW BaseVariable*[m_iNumVariables];
	ZeroMem(target.m_pVariables, m_iNumVariables * sizeof(BaseVariable*));

	for(uint n = 0; n < m_iNumVariables; n++)
	{
		if(m_pVariables[n] == NULL)
		{
			target.m_pVariables[n] = NULL;
			continue;
		}
		const BaseVariableDef& vdef = m_pClass->GetVariableDef(n);
		BaseVariable* tvar = target.AllocVariable(n);
		switch(vdef.m_Type)
		{
		case DVT_SCALAR:
			((BaseVariableScalar*)tvar)->Value = GetScalar(n);
			break;
		case DVT_INTEGER:
			((BaseVariableInteger*)tvar)->Value = GetInteger(n);
			break;
		case DVT_VECTOR:
			((BaseVariableVector*)tvar)->Value = GetVector(n);
			break;
		case DVT_VECTOR2:
			((BaseVariableVector2*)tvar)->Value = GetVector2(n);
			break;
		case DVT_STRING:
			((BaseVariableString*)tvar)->Value = GetString(n);
			break;
		case DVT_COLOR:
			((BaseVariableColor*)tvar)->Value = GetColor(n);
			break;
		case DVT_BOOLEAN:
			((BaseVariableBoolean*)tvar)->Value = GetBoolean(n);
			break;
		}
	}
}

//-----------------------------------------------------------------------
void BaseSource::SetName(CStr name)
{
#ifdef _DEBUG
	enf_assert2(m_pCollection == NULL || !m_pCollection->m_SourcesHash.Contains(m_strName.cstr()), "Do not call SetName, when in collection.Call BaseSourceCollection::RenameSource instead!");
#endif
	m_strName = name;
}

//-----------------------------------------------------------------------
void BaseSource::FreeVariables()
{
	if(m_pClass == NULL)
		return;

	for(uint n = 0; n < m_iNumVariables; n++)
	{
		BaseVariable* tvar = m_pVariables[n];
		if(tvar == NULL)
			continue;

		BaseVariable::Delete(m_pClass->GetVariableType(n), tvar);
	}
	SAFE_DELETE_ARRAY(m_pVariables);
}

//-----------------------------------------------------------------------
void BaseSource::DeleteVariable(uint variable)
{
	if(m_pClass == NULL || m_pVariables == NULL)
		return;

	enf_assert(variable < GetNumVariables());

	BaseVariable::Delete(m_pClass->GetVariableType(variable), m_pVariables[variable]);
	m_pVariables[variable] = NULL;
}

//-----------------------------------------------------------------------
BaseSource::~BaseSource()
{
/*
#ifdef _DEBUG
	if(m_pCollection)
	{
		enf_assert2(!m_pCollection->m_Sources.Contains(this), "Do not delete BaseSource when in collection. Call BaseSourceCollection::RemoveSource before!");
	}
#endif
*/
	FreeVariables();
}

//-----------------------------------------------------------------------
BaseSource::BaseSource(BaseClass* baseClass): m_pClass(baseClass), m_pVariables(NULL), m_iNumVariables(0)
#ifdef _DEBUG
	, m_pCollection(NULL)
#endif
{
	enf_assert(baseClass);
	m_iNumVariables = baseClass->GetNumVariables();
	m_pVariables = ENF_NEW BaseVariable*[baseClass->GetNumVariables()];
	ZeroMem(m_pVariables, sizeof(BaseVariable*) * baseClass->GetNumVariables());
}

//-----------------------------------------------------------------------
BaseSource::BaseSource(BaseClass* baseClass, CStr name): m_pClass(baseClass), m_pVariables(NULL), m_iNumVariables(0)
#ifdef _DEBUG
	, m_pCollection(NULL)
#endif
{
	enf_assert(baseClass);
	m_iNumVariables = baseClass->GetNumVariables();
	m_pVariables = ENF_NEW BaseVariable*[baseClass->GetNumVariables()];
	ZeroMem(m_pVariables, sizeof(BaseVariable*) * baseClass->GetNumVariables());
	SetName(name);
}

//-----------------------------------------------------------------------
BaseVariable* BaseSource::AllocVariable(uint variable)
{
	BaseVariable* var = m_pVariables[variable];
	if(!IsVariableSet(variable))
	{
		switch(m_pClass->GetVariableType(variable))
		{
		case DVT_SCALAR:
			var = ENF_NEW BaseVariableScalar();
			break;
		case DVT_INTEGER:
			var = ENF_NEW BaseVariableInteger();
			break;
		case DVT_VECTOR:
			var = ENF_NEW BaseVariableVector();
			break;
		case DVT_VECTOR2:
			var = ENF_NEW BaseVariableVector2();
			break;
		case DVT_STRING:
			var = ENF_NEW BaseVariableString();
			break;
		case DVT_COLOR:
			var = ENF_NEW BaseVariableColor();
			break;
		case DVT_BOOLEAN:
			var = ENF_NEW BaseVariableBoolean();
			break;
		default:
			enf_assert(!"Unsupported");
			return NULL;
		}
		m_pVariables[variable] = var;
	}
	return var;
}

//-----------------------------------------------------------------------
bool BaseSource::SetVariable(uint variable, CStr value)
{
	enf_assert(variable < m_iNumVariables);
	const BaseVariableDef& vdef = m_pClass->GetVariableDef(variable);

	BaseVariable* var = AllocVariable(variable);
	if(var == NULL)
		return false;

	switch(vdef.m_Type)
	{
	case DVT_SCALAR:
		if(!vdef.FindEnumScalar(value, ((BaseVariableScalar*)m_pVariables[variable])->Value))
			((BaseVariableScalar*)var)->Value = value.ToFloat();
		break;
	case DVT_INTEGER:
		if(!vdef.FindEnumInteger(value, ((BaseVariableInteger*)m_pVariables[variable])->Value))
			((BaseVariableInteger*)var)->Value = value.ToInt();
		break;
	case DVT_VECTOR:
		{
			Vector4 vec;
			sscanf(value.cstr(), "%f %f %f %f", &vec.x, &vec.y, &vec.z, &vec.w);
			((BaseVariableVector*)var)->Value = vec;
		}
		break;
	case DVT_VECTOR2:
		{
			Vector2 vec;
			sscanf(value.cstr(), "%f %f", &vec.x, &vec.y);
			((BaseVariableVector2*)var)->Value = vec;
		}
		break;
	case DVT_STRING:
		((BaseVariableString*)var)->Value = value.cstr();
		break;
	case DVT_COLOR:
		{
			FColor col;
			sscanf(value.cstr(), "%f %f %f %f", &col.r, &col.g, &col.b, &col.a);
			col.Clamp(0.0f, 1.0f);
			((BaseVariableColor*)var)->Value = col;
		}
		break;
	case DVT_BOOLEAN:
		((BaseVariableBoolean*)var)->Value = value.cmpi("true") == 0;
		break;
	default:
		enf_assert(!"Unsupported");
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------
BaseSourceCollection::BaseSourceCollection()
{
}

//-----------------------------------------------------------------------
BaseSourceCollection::~BaseSourceCollection()
{
	ForEach<DeleteItem>(m_Sources);
	m_SourcesHash.Clear();
	m_Sources.Clear();
}

//-----------------------------------------------------------------------
bool BaseSourceCollection::AddSource(BaseSource* source)
{
	if(FindSource(source->GetName()))
		return false;
	enf_assert(!m_Sources.Contains(source));
	m_Sources.Insert(source);
	m_SourcesHash.Insert(source->GetName(), source);
#ifdef _DEBUG
	source->m_pCollection = this;
#endif
	ChangeSource(source);
	return true;
}

//-----------------------------------------------------------------------
bool BaseSourceCollection::RemoveSource(BaseSource* source)
{
#ifdef _DEBUG
	enf_assert(source->m_pCollection == this);
#endif

	if(m_SourcesHash.Remove(source->GetName()))
	{
		ChangeSource(source);
		return m_Sources.Remove(source);
	}
	return false;
}

//-----------------------------------------------------------------------
bool BaseSourceCollection::DestroySource(BaseSource* source)
{
	bool res = RemoveSource(source);
	delete source;
	return res;
}

//-----------------------------------------------------------------------
bool BaseSourceCollection::RenameSource(BaseSource* source, CStr name)
{
	if(FindSource(name))
		return false;
#ifdef _DEBUG
	enf_assert(source->m_pCollection == this);
#endif
	RemoveSource(source);
	source->SetName(name);
	AddSource(source);
	return true;
}

//-----------------------------------------------------------------------
BaseSource* BaseSourceCollection::FindSource(CStr name) const
{
	BaseSource* src;
	if(m_SourcesHash.Find(name, src))
		return src;

	return NULL;
}

//-----------------------------------------------------------------------
bool BaseVariableDef::FindEnumInteger(CStr value, int& ret) const
{
	enf_assert(m_Type == DVT_INTEGER || m_Type == DVT_FLAGS);

	const ParmEnumInteger** enums = (const ParmEnumInteger**)m_pEnums;

	while(enums && *enums)
	{
		if(value == (*enums)->Name)
		{
			ret = (*enums)->Value;
			return true;
		}
		enums++;
	}

	return false;
}

//-----------------------------------------------------------------------
const char* BaseVariableDef::FindEnumInteger(int value) const
{
	enf_assert(m_Type == DVT_INTEGER || m_Type == DVT_FLAGS);

	const ParmEnumInteger** enums = (const ParmEnumInteger**)m_pEnums;

	while(enums && *enums)
	{
		if(value == (*enums)->Value)
		{
			return (*enums)->Name.cstr();
		}
		enums++;
	}

	return NULL;
}

//-----------------------------------------------------------------------
bool BaseVariableDef::FindEnumScalar(CStr value, float& ret) const
{
	enf_assert(m_Type == DVT_SCALAR);

	const ParmEnumScalar** enums = (const ParmEnumScalar**)m_pEnums;

	while(enums && *enums)
	{
		if(value == (*enums)->Name)
		{
			ret = (*enums)->Value;
			return true;
		}
		enums++;
	}

	return false;
}

//-----------------------------------------------------------------------
const char* BaseVariableDef::FindEnumScalar(float value) const
{
	enf_assert(m_Type == DVT_SCALAR);

	const ParmEnumScalar** enums = (const ParmEnumScalar**)m_pEnums;

	while(enums && *enums)
	{
		if(value == (*enums)->Value)
		{
			return (*enums)->Name.cstr();
		}
		enums++;
	}

	return NULL;
}


//-----------------------------------------------------------------------
bool BaseVariableDef::FindEnumColor(CStr value, FColor& ret) const
{
	enf_assert(m_Type == DVT_COLOR);

	const ParmEnumColor** enums = (const ParmEnumColor**)m_pEnums;

	while(enums && *enums)
	{
		if(value == (*enums)->Name)
		{
			ret = (*enums)->Value;
			return true;
		}
		enums++;
	}

	return false;
}

//-----------------------------------------------------------------------
const ParmEnum* BaseVariableDef::FindEnum(CStr value) const
{
	if(m_pEnums == NULL)
		return NULL;

	const ParmEnum** enums = m_pEnums;

	while(*enums)
	{
		if(value == (*enums)->Name)
		{
			return *enums;
		}
		enums++;
	}
	return false;
}

//----------------------------------------------------------------------------------------------------------
const char* VariableTypeToString(DataVarType type)
{
	switch(type)
	{
	case DVT_NULL:
		return "Unknown";

	case DVT_SCALAR:
		return "Float";

	case DVT_VECTOR2:
		return "Vector2";

	case DVT_VECTOR3:
		return "Vector3";

	case DVT_VECTOR:
		return "Vector";

	case DVT_MATRIX:
		return "Matrix";

	case DVT_COLOR:
		return "Color";

	case DVT_INTEGER:
		return "Integer";

	case DVT_STRING:
		return "String";

	case DVT_TEXTURE:
		return "Texture";

	case DVT_BOOLEAN:
		return "Boolean";

	case DVT_FLAGS:
		return "Flags";
	};
	return "";
}

}
