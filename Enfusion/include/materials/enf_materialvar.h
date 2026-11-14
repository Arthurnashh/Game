//-----------------------------------------------------------------------------
// File: enf_materialvar.h
//
// Desc: Material variables definition
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_MATERIALVAR_H
#define ENF_MATERIALVAR_H

#include "enf_config.h"
#include "common/enf_basevariable.h"

namespace enf
{

/** @addtogroup MaterialSystem
@{*/

class Variable;
class Texture;

//-----------------------------------------------------------------------
/*!
Basic structure defining value of variable in material. Value can
be supplied as constant (when enf::MaterialVar::Var == NULL) or by
reference to script variable. Constant value is supplied in inherited
structures (enf::MaterialVarColor, enf::MaterialVarScalar etc...), each one
for one type
*/
struct MaterialVar
{
	//ENF_DECLARE_SMALLOBJECT(MaterialVar);
	const Variable*	Var;
	MaterialVar(): Var(NULL) {}
};


struct MaterialVarScalar: public MaterialVar, public BaseVariableScalar
{
};

struct MaterialVarInteger: public MaterialVar, public BaseVariableInteger
{
};

struct MaterialVarVector: public MaterialVar, public BaseVariableVector
{
};

struct MaterialVarColor: public MaterialVar, public BaseVariableColor
{
};

struct MaterialVarMatrix: public MaterialVar, public BaseVariableMatrix
{
};

struct MaterialVarBoolean: public MaterialVar, public BaseVariableBoolean
{
};

struct MaterialVarString: public MaterialVar, public BaseVariableString
{
};

struct MaterialVarPointer: public MaterialVar, public BaseVariablePointer
{
};

struct TextureMap
{
	const char*	m_strName;
	Texture*		m_pTexture;
};

//!Value of TextureMap type variable
struct MaterialVarTexture: public MaterialVar
{
private:
friend class Material;

	//To avoid allocation of 8byte chunks for every non-animated texture variable,
	//we have union and usage depends on m_iNumTextures. When m_iNumTextures == 1,
	//we can read directly m_TextureMap, otherwise we have to access array m_pTextures
	union
	{
		TextureMap*			m_pTextures;
		TextureMap			m_Texture;
	};

	ushort			m_iNumTextures;

public:
	ushort			m_iFPS;
	uint				m_iTexFlags;
	uint				m_iUsage;

	ENF_DECLARE_SMALLOBJECT(MaterialVarTexture);

	ENF_INLINE uint GetNumTextureMaps() const
	{
		return m_iNumTextures;
	}

	ENF_INLINE const TextureMap* GetTextureMap(uint tex) const
	{
		enf_assert(tex < m_iNumTextures);
		if(m_iNumTextures == 1)
			return &m_Texture;
		
		return &m_pTextures[tex];
	}

	MaterialVarTexture(): m_pTextures(NULL), m_iNumTextures(0), m_iFPS(0), m_iTexFlags(0), m_iUsage(0) {}

	~MaterialVarTexture()
	{
		if(m_iNumTextures > 1)
		{
			SAFE_DELETE_ARRAY(m_pTextures);
		}
	}
};


//!Enumerated type of MaterialDeform functions
struct ParmEnumMaterialDeform: ParmEnum
{
	int			Type;
	DataVarType	Parms[4];

	ParmEnumMaterialDeform(CStr name, int ptype, DataVarType type0 = DVT_NULL, DataVarType type1 = DVT_NULL, DataVarType type2 = DVT_NULL, DataVarType type3 = DVT_NULL):
		ParmEnum(name),
		Type(ptype)
	{
		Parms[0] = type0; Parms[1] = type1; Parms[2] = type2; Parms[3] = type3;
	}

	ParmEnumMaterialDeform(CStr name, int ptype, CStr desc, DataVarType type0 = DVT_NULL, DataVarType type1 = DVT_NULL, DataVarType type2 = DVT_NULL, DataVarType type3 = DVT_NULL):
		ParmEnum(name, desc),
		Type(ptype)
	{
		Parms[0] = type0; Parms[1] = type1; Parms[2] = type2; Parms[3] = type3;
	}
};


/*!
Variable definition for MaterialClass implementation.
*/
struct MaterialVarDef: public BaseVariableDef
{
	//!Additional info, i.e. enf::TextureUsage for textures
	uint			m_iUsage;

	//!When referencing texture, this is loading priority
	int			m_iLoadPriority;

	MaterialVarDef& operator=(const MaterialVarDef& vdef)
	{
		BaseVariableDef::operator=(vdef);
		m_iUsage = vdef.m_iUsage;
		m_iLoadPriority = vdef.m_iLoadPriority;
		return *this;
	}

	MaterialVarDef(DataVarType type, uint usage, int loadPriority, CStr name, CStr defvalue, CStr widget, CStr rangescale = NULL, CStr desc = NULL, const ParmEnum** enums = NULL):
		BaseVariableDef(type, name, defvalue, widget, rangescale, desc, enums),
		m_iUsage(usage),
		m_iLoadPriority(loadPriority)
	{
	}

	MaterialVarDef(): BaseVariableDef(),
		m_iUsage(0),
		m_iLoadPriority(0)
	{
	}
};
//@}

}
#endif //ENF_MATERIALVAR_H