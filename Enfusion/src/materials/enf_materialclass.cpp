//-----------------------------------------------------------------------------
// File: enf_materialclass.cpp
//
// Desc: Implementation of MaterialClass
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "engine/enf_engine.h"
#include "materials/enf_imaterialsystem.h"
#include "script/enf_scriptmain.h"
//#include "baseworld/enf_entity.h"
//#include "enf_pipeline.h"
//#include "vobject/enf_vobject.h"

namespace enf
{
//-----------------------------------------------------------------------
TexFilter MaterialClass::m_TextureUsage2Filter[] =
{
	FILTER_DIFFUSEMAP, //MAP_DIFFUSE
	FILTER_NONE, //MAP_LIGHTMAP
	FILTER_NORMALMAP, //MAP_NORMAL
	FILTER_NORMALMAP, //MAP_NORMAL_DXT
	FILTER_LOW, //MAP_REFRACT
	FILTER_CUBEENVMAP, //MAP_ENVIRONMENT
	FILTER_LOW, //MAP_HEIGHT
	FILTER_LOW, //MAP_EDITOR_ONLY
	FILTER_NONE, //MAP_IMAGE
};

SamplerAddressStateHandle MaterialClass::m_AddressStates[];

//-----------------------------------------------------------------------
void MaterialClass::BeginFrame()
{
	m_pContext = NULL;
}

//-----------------------------------------------------------------------
void MaterialClass::EndFrame()
{
	m_pContext = NULL;
}

//-----------------------------------------------------------------------
MaterialClass::MaterialClass(MaterialSystem* imat):
	m_pMaterialSystem((IMaterialSystem*)imat),
	m_pContext(NULL)
{
	m_AddressStates[0] = g_pRender->CreateSamplerAddressState(SamplerAddressState(TADDRESS_WRAP, TADDRESS_WRAP, 0));
	m_AddressStates[1] = g_pRender->CreateSamplerAddressState(SamplerAddressState(TADDRESS_CLAMP, TADDRESS_WRAP, 0));
	m_AddressStates[2] = g_pRender->CreateSamplerAddressState(SamplerAddressState(TADDRESS_WRAP, TADDRESS_CLAMP, 0));
	m_AddressStates[3] = g_pRender->CreateSamplerAddressState(SamplerAddressState(TADDRESS_CLAMP, TADDRESS_CLAMP, 0));
}


//-----------------------------------------------------------------------
void MaterialClass::SetAddressMode(uint tmu, const Material* mat, uint var)
{
	uint flags = 0;
	MaterialVarTexture* tex = mat->GetTexture(var);
	if(tex != NULL)
	{
		if(tex->m_iUsage == MAP_ENVIRONMENT)
		{
			//there is only one correct way
			SET_SAS(tmu, SAS_CUBE_MAP);
			return;
		}
		
		flags = tex->m_iTexFlags;
	}

	uint sasIndex = (((flags & TEXTR_CLAMPU) == 0) ? 0 : 1) + (((flags & TEXTR_CLAMPV) == 0) ? 0 : 2);
	SET_SAS(tmu, m_AddressStates[sasIndex]);
}

//-----------------------------------------------------------------------
void MaterialClass::SetFilterMode(uint tmu, TextureUsage textype)
{	
	enf_assert(textype < MAP_SIZEOF);
	((IMaterialSystem*)g_pMaterialSystem)->SetFilterMode(tmu, m_TextureUsage2Filter[textype]);
}

//-----------------------------------------------------------------------
void MaterialClass::SetFilterMode(uint tmu, const Material* mat, uint var)
{	
	TexFilter filter = FILTER_NONE;

	MaterialVarTexture* tex = mat->GetTexture(var);
	if(tex != NULL)
	{
		if(!(tex->m_iTexFlags & TEXTR_NOFILTER))
		{
			enf_assert(tex->m_iUsage < MAP_SIZEOF);
			filter = m_TextureUsage2Filter[tex->m_iUsage];
		}
	}
	((IMaterialSystem*)g_pMaterialSystem)->SetFilterMode(tmu, filter);
}

//-----------------------------------------------------------------------
const IVariable* MaterialClass::GetVariable(const Variable* var) const
{

#ifdef ENF_DONE

	static IVariable dummyptrvar;
	static IVariable dummyintvar;
	static IVariable dummyfloatvar;
	static IVariable dummyvectorvar;
	const static float zeroes[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	const static float ones[4] = {1.0f, 1.0f, 1.0f, 1.0f};

	if(var->Pos >= 0)
	{
		Instance* inst = m_pContext->m_pEntity;
		if(inst->GetClass()->GetNumVariables() <= (uint)var->Pos || inst->GetClass()->GetVariable(var->Pos)->GetName() != var->GetName())
		{
			if(var->IsPtr())
			{
				switch(var->GetVType())
				{
				case VARTYPE_FLOAT:
					dummyptrvar.Pointer = (void*)ones;
					return &dummyptrvar;
		
				case VARTYPE_VECTOR:
					dummyvectorvar.Pointer = (void*)zeroes;
					return &dummyvectorvar;

				default:
					dummyintvar.Pointer = (void*)zeroes;
					return &dummyvectorvar;
				}
			}
			else
			{
				switch(var->GetVType())
				{
				case VARTYPE_FLOAT:
					dummyfloatvar.Float = 1.0f;
					return &dummyfloatvar;
		
				case VARTYPE_VECTOR:
					dummyvectorvar.Pointer = (void*)zeroes;
					return &dummyvectorvar;

				default:
					dummyintvar.Int = 0xffffffff;
					return &dummyintvar;
				}
			}
		}

		return inst->GetIVariable(var->Pos);
	}
#endif
	return var;

}

//-----------------------------------------------------------------------
uint MaterialClass::AddVarDef(const MaterialVarDef& def)
{
	uint index = m_VarDefs.GetCardinality();
	MaterialVarDef& ndef = m_VarDefs.Append();
	def.CopyStatic(ndef);
	ndef.m_iUsage = def.m_iUsage;
	ndef.m_iIndex = index;
	ndef.m_iLoadPriority = def.m_iLoadPriority;
	m_VarHash.Insert(ndef.m_strName, index);
	return index;
}

//-----------------------------------------------------------------------
uint MaterialClass::GetVariableIndex(CStr name) const
{
	uint index;
	if(m_VarHash.Find(name, index))
	{
		return index;
	}
	return INDEX_NOT_FOUND;
}

//-----------------------------------------------------------------------
int MaterialClass::CompareMaterials(const Material* mat1, const Material* mat2) const
{
	if(mat1->GetCreationFlags() < mat2->GetCreationFlags())
		return -1;
	if(mat1->GetCreationFlags() > mat2->GetCreationFlags())
		return 1;
	return 0;
}

//-----------------------------------------------------------------------
bool MaterialClass::TouchTextures(Material* mat) const
{
	bool loaded = true;
	for(uint n = 0; n < m_VarDefs.GetCardinality(); n++)
	{
		if(!mat->IsVariableSet(n))
			continue;

		if(m_VarDefs[n].m_Type != DVT_TEXTURE)
			continue;

		loaded &= mat->TouchTexture(n);
	}
	return loaded;
}

}