//-----------------------------------------------------------------------------
// File: enf_materials.cpp
//
// Desc: Material class
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "materials/enf_imaterialsystem.h"
#include "materials/enf_materialsource.h"
//#include "script/enf_scriptmain.h"
#include "script/enf_scriptvariable.h"
#include "render/enf_texture.h"
#include "resource/enf_resourcemanager.h"
#include "pipeline/enf_pipeline.h"
//#include "baseworld/enf_entity.h"
//#include "vobject/enf_vobject.h"
//#include "enf_thread.h"
#include "enf_main.h"
#include "common/enf_staticbuffer.h"

namespace enf
{

MaterialVar* Material::m_pEmptyVariables[];

//-----------------------------------------------------------------------
int Material::CompareVariable(const Material* mat2, uint mv) const
{
	enf_assert(GetMaterialClass() != NULL && mat2->GetMaterialClass() != NULL);
	enf_assert(GetMaterialClass()->GetVariableType(mv) == mat2->GetMaterialClass()->GetVariableType(mv));

	switch(GetMaterialClass()->GetVariableType(mv))
	{
	case DVT_INTEGER:
		if(!IsVariableSet(mv) && mat2->IsVariableSet(mv))
			return -1;

		if(IsVariableSet(mv) && !mat2->IsVariableSet(mv))
			return 1;

		if(!IsVariableSet(mv) && !mat2->IsVariableSet(mv))
			return 0;

		return GetInteger(mv) - mat2->GetInteger(mv);

	case DVT_SCALAR:
		if(!IsVariableSet(mv) && mat2->IsVariableSet(mv))
			return -1;

		if(IsVariableSet(mv) && !mat2->IsVariableSet(mv))
			return 1;

		if(!IsVariableSet(mv) && !mat2->IsVariableSet(mv))
			return 0;

		if(GetScalar(mv) < mat2->GetScalar(mv))
			return -1;

		if(GetScalar(mv) > mat2->GetScalar(mv))
			return 1;

		return 0;

	case DVT_BOOLEAN:
		if(!IsVariableSet(mv) && mat2->IsVariableSet(mv))
			return -1;

		if(IsVariableSet(mv) && !mat2->IsVariableSet(mv))
			return 1;

		if(!IsVariableSet(mv) && !mat2->IsVariableSet(mv))
			return 0;

		return (int)GetBoolean(mv) - (int)mat2->GetBoolean(mv);

	case DVT_TEXTURE:
		if(!IsVariableSet(mv) && mat2->IsVariableSet(mv))
			return -1;

		if(IsVariableSet(mv) && !mat2->IsVariableSet(mv))
			return 1;

		if(!IsVariableSet(mv) && !mat2->IsVariableSet(mv))
			return 0;

		const TextureMap* tmap1 = GetTextureMap(mv, NULL);
		const TextureMap* tmap2 = mat2->GetTextureMap(mv, NULL);

		return CStr(tmap1->m_strName).cmpi(CStr(tmap2->m_strName));
	}

	return 0;
}

//-----------------------------------------------------------------------
float Material::CFunc::GetScalar(uint var, const Material* mat) const
{
	if(m_Parms[var] == NULL)
		return 0.0;

	if(m_Parms[var]->Var)
	{
		const IVariable* iv = mat->GetIVariable(m_Parms[var]->Var);
		if(iv)
			return iv->Float;
	}

	return m_Parms[var]->Value;
}


//-----------------------------------------------------------------------
void Material::_GetTCMatrix(Vector4* mres) const
{
	const int MAX_TC = 2;

	Matrix34 result[MAX_TC][2];
	uint resnum[MAX_TC];
	for(int n = 0; n < MAX_TC; n++)
		resnum[n] = 0;

	const CFunc* cf = m_pDeformFuncs;

	for(uint n = 0; n < m_iNumDeformFuncs; n++, cf++)
	{
		CFuncType type = cf->m_Type;

		uint uvset = 0;
		if(type >= TCMOD_SCALE2)
		{
			uvset = 1;
			type = (CFuncType)((type - TCMOD_SCALE2) + TCMOD_SCALE);
		}

		switch(type)
		{
		case VCMOD_SCALE:
		case VCMOD_CLOTH:
		case VCMOD_LIQUID:
		case VCMOD_SIN:
			break;

		case TCMOD_SHIFT:
			{
				if(resnum[uvset] == 0)
					result[uvset][1] = Matrix34(Vector3(cf->GetScalar(0, this), cf->GetScalar(1, this), 0.0f));
				else
					result[uvset][(resnum[uvset] & 1) ^ 1] = result[uvset][resnum[uvset] & 1] * Matrix34(Vector3(cf->GetScalar(0, this), cf->GetScalar(1, this), 0.0f));
				resnum[uvset]++;
			}
			break;

		case TCMOD_SCALE:
			{
				float offu = cf->GetScalar(0, this);
				float offv = cf->GetScalar(1, this);
				float scaleu = cf->GetScalar(2, this);
				float scalev = cf->GetScalar(3, this);

				if(resnum[uvset] == 0)
				{
					result[uvset][1] = Matrix34(Vector3(offu, offv, 0.0f)) * Matrix34(
						scaleu, 0.0f, 0.0f,
						0.0f, scalev, 0.0f,
						0.0f, 0.0f, 1.0f,
						0.0f, 0.0f, 0.0f
						) * Matrix34(Vector3(-offu, -offv, 0.0f));
				}
				else
				{
					result[uvset][(resnum[uvset] & 1) ^ 1] = result[uvset][resnum[0] & 1] * Matrix34(Vector3(offu, offv, 0.0f)) * Matrix34(
						scaleu, 0.0f, 0.0f,
						0.0f, scalev, 0.0f,
						0.0f, 0.0f, 1.0f,
						0.0f, 0.0f, 0.0f
						) * Matrix34(Vector3(-offu, -offv, 0.0f));
				}
				resnum[uvset]++;
			}
			break;

		case TCMOD_SCALE_NO:
			{
				float scaleu = cf->GetScalar(2, this);
				float scalev = cf->GetScalar(3, this);

				if(resnum[uvset] == 0)
				{
					result[uvset][1] = Matrix34(
						scaleu, 0.0f, 0.0f,
						0.0f, scalev, 0.0f,
						0.0f, 0.0f, 1.0f,
						0.0f, 0.0f, 0.0f
						);
				}
				else
				{
					result[uvset][(resnum[uvset] & 1) ^ 1] = result[uvset][resnum[0] & 1] * Matrix34(
						scaleu, 0.0f, 0.0f,
						0.0f, scalev, 0.0f,
						0.0f, 0.0f, 1.0f,
						0.0f, 0.0f, 0.0f
						);
				}
				resnum[uvset]++;
			}
			break;

		case TCMOD_ROTATE:
			{
				Matrix34 rmat(Vector3(0.0f));
				rmat.Yaw(cf->GetScalar(0, this));

				float offu = cf->GetScalar(1, this);
				float offv = cf->GetScalar(2, this);

				if(resnum[uvset] == 0)
					result[uvset][1] = Matrix34(Vector3(offu, offv, 0.0f)) * rmat * Matrix34(Vector3(-offu, -offv, 0.0f));
				else
					result[uvset][(resnum[uvset] & 1) ^ 1] = result[uvset][resnum[uvset] & 1] * Matrix34(Vector3(offu, offv, 0.0f)) * rmat * Matrix34(Vector3(-offu, -offv, 0.0f));
				resnum[uvset]++;
			}
			break;
		}
	}

	for(int n = 0; n < MAX_TC; n++)
	{
		if(resnum[n] != 0)
		{
			mres[n * 2 + 0] = result[n][resnum[n] & 1].GetColumn(0);
			mres[n * 2 + 1] = result[n][resnum[n] & 1].GetColumn(1);
		}
		else
		{
			mres[n * 2 + 0].x = 1.0f; mres[n * 2 + 0].y = 0.0f; mres[n * 2 + 0].z = 0.0f; mres[n * 2 + 0].w = 0.0f;
			mres[n * 2 + 1].x = 0.0f; mres[n * 2 + 1].y = 1.0f; mres[n * 2 + 1].z = 0.0f; mres[n * 2 + 1].w = 0.0f;
		}
	}
/*
	if(this->GetMaterialName() == "c_display_3")
	{
		LogF(LL_NORMAL, "%f %f %f %f", mres[0].x, mres[0].y, mres[0].z, mres[0].w);
		LogF(LL_NORMAL, "%f %f %f %f", mres[1].x, mres[1].y, mres[1].z, mres[1].w);
	}*/
}

//-----------------------------------------------------------------------
bool Material::IsType(uint variable, DataVarType type) const
{
	if(m_pMClass == NULL)
		return false;
	
	const MaterialVarDef* def = m_pMClass->GetVariableDef(variable);
	if(def == NULL)
		return false;

	return (def->m_Type == type);
}

//-----------------------------------------------------------------------
MaterialVar* Material::SetDefaultValue(uint variable)
{
	if(m_pMClass == NULL)
		return NULL;

	MaterialVar* mv = NULL;
	const MaterialVarDef* def = m_pMClass->GetVariableDef(variable);

	if(def == NULL)
		return NULL;

	if(def->m_strDefault.IsEmpty())
		return NULL;

	switch(def->m_Type)
	{
	case DVT_STRING:
		mv = SetString(variable, def->m_strDefault);
		break;

	case DVT_INTEGER:
		mv = SetInteger(variable, def->m_strDefault.ToInt());
		break;

	case DVT_SCALAR:
		mv = SetScalar(variable, def->m_strDefault.ToFloat());
		break;

	case DVT_BOOLEAN:
		mv = SetBoolean(variable, def->m_strDefault == "true");
		break;

	case DVT_TEXTURE:
		{
			const char* texname = def->m_strDefault.cstr();
			mv = SetTexture(variable, &texname, 1, 0);
		}
		break;

	case DVT_COLOR:
		{
			FColor col;
			sscanf(def->m_strDefault.cstr(), "%f %f %f %f", &col.r, &col.g, &col.b, &col.a);
			mv = SetColor(variable, col);
		}
		break;

	case DVT_VECTOR:
		{
			Vector4 vec;
			sscanf(def->m_strDefault.cstr(), "%f %f %f %f", &vec.x, &vec.y, &vec.z, &vec.w);
			mv = SetVector(variable, vec);
		}
		break;
	
	default:
		return false;
	}
	return mv;
}

//-----------------------------------------------------------------------
const IVariable* Material::GetIVariable(const Variable* var) const
{
	if(var->Pos == -1)
		return var;

	enf_assert(m_pMClass != NULL);
	
	if(m_pMClass->m_pContext == NULL)
		return NULL;

#ifdef ENF_DONE
	const Instance* inst = m_pMClass->m_pContext->m_pEntity;

	//it's bad context.
	if(inst->GetClass()->GetNumVariables() <= (uint)var->Pos || inst->GetClass()->GetVariable(var->Pos)->GetName() != var->GetName())
		return NULL;

	return inst->GetIVariable(var->Pos);
#endif
	return NULL;
}

//-----------------------------------------------------------------------
uint Material::RenderTargetIndex(const char* texName)
{
	const char** names = ((IMaterialSystem*)g_pMaterialSystem)->m_pGUIName;

	for(uint n = 0; n < 4; n++)
	{
		if(names[n] == texName)
			return n;
	}
	return INDEX_NOT_FOUND;
}

//-----------------------------------------------------------------------
const TextureMap*	Material::GetTextureMap(uint var, const Pipeline* pipeline, const void* extra) const
{
	enf_assert(IsType(var, DVT_TEXTURE));
	if(!IsVariableSet(var))
		return NULL;
	
	MaterialVarTexture* mm = (MaterialVarTexture*)m_pVariables[var];
	TextureMap* tmap = &mm->m_Texture;
	
	if(mm->m_iNumTextures > 1)
	{
		if(mm->Var != NULL)
		{
			uint n = m_pMClass->GetVariable(mm->Var)->Int;

			//avoid out of range
			n = enf_min(n, (uint)(mm->m_iNumTextures - 1));
			tmap = &mm->m_pTextures[n];
		}
		else
		{
#ifdef ENF_DONE
			if(pipeline)
			{
				float time = (float)(pipeline->GetWorld()->GetElapsedTime() * 0.01 * (float)mm->m_iFPS);
				tmap = &mm->m_pTextures[(int)time % mm->m_iNumTextures];
			}
			else
#endif
				tmap = &mm->m_pTextures[0];
		}
	}

	//this layer needs something special
	if(mm->m_iTexFlags & TEXTR_SPECIAL && tmap->m_strName)
	{
		Entity* ent = NULL;
		const ModelContext* mc = m_pMClass->GetContext();

		if(mc)
			ent = mc->m_pEntity;

		if(NeedsEnvMap() && tmap->m_strName == ((IMaterialSystem*)g_pMaterialSystem)->m_pNearestCubeName)
		{
			//we have per-face pointer
			if(extra != NULL)
			{
				tmap->m_pTexture = (Texture*)extra;
				return tmap;
			}

			if(mc && pipeline)
			{
				tmap->m_pTexture = mc->IsFlagSet(MCF_HAS_ENVMAP) ? pipeline->GetEnvMap(mc)->m_pTexture : NULL;
			}
			return tmap;
		}

		uint rtIndex;
		if(NeedsGUI() && (rtIndex = RenderTargetIndex(tmap->m_strName)) != INDEX_NOT_FOUND)
		{
			if(ent)
			{
#ifdef ENF_DONE
				Texture* rt = ent->GetRTTexture(rtIndex);
				if(rt)
				{
					tmap->m_pTexture = rt;
					return tmap;
				}
#endif
			}

			tmap->m_pTexture = g_pMaterialSystem->GetIdentityColorMap(FColor(0,0,0,0), 0);
			return tmap;
		}

		if(NeedsRenderView() && tmap->m_strName == ((IMaterialSystem*)g_pMaterialSystem)->m_pRenderViewName)
		{
#ifdef ENF_DONE
			if(ent && pipeline)
			{
				uint cameraIndex = ent->GetInstance()->GetRenderView();
				
				Texture* rt = pipeline->GetWorld()->GetCameraRenderTarget(cameraIndex);

				if(rt == NULL)
				{
					rt = g_pMaterialSystem->GetCurrentRender();
					if(rt == NULL)
						rt = g_pMaterialSystem->GetIdentityColorMap(FColor(0,0,0,0), 0);
				}

				tmap->m_pTexture = rt;
			}
			else
#endif
			{
				tmap->m_pTexture = g_pMaterialSystem->GetIdentityColorMap(FColor(0,0,0,0), 0);
			}
		}
	}
	return tmap;
}

//-----------------------------------------------------------------------
void Material::InitDefaults()
{
	m_iSortBias = 0;
	m_SortType = SORT_OPAQUE;
	m_iFlags = MF_CULL | MF_ZWRITE | MF_ZENABLE;
	m_SurfParms = 0;
	m_iMaxDistance = 0x7fffffff;
	m_pDeformFuncs = NULL;
	m_iNumDeformFuncs = 0;
	m_PassMask = 0xffff;
}

//-----------------------------------------------------------------------
void DeleteVariable(DataVarType type, MaterialVar* var, bool ownMemory)
{
	//we don't have virtual destructor (as want to keep it small)
	if(ownMemory)
	{
		switch(type)
		{
		case DVT_SCALAR:
			delete ((MaterialVarScalar*)var);
			break;
		case DVT_VECTOR:
			delete ((MaterialVarVector*)var);
			break;
		case DVT_MATRIX:
			delete ((MaterialVarMatrix*)var);
			break;
		case DVT_COLOR:
			delete ((MaterialVarColor*)var);
			break;
		case DVT_INTEGER:
			delete ((MaterialVarInteger*)var);
			break;
		case DVT_BOOLEAN:
			delete ((MaterialVarBoolean*)var);
			break;
		case DVT_TEXTURE:
			delete ((MaterialVarTexture*)var);
			break;
		case DVT_STRING:
			delete ((MaterialVarString*)var);
			break;

		case DVT_POINTER:
			delete ((MaterialVarPointer*)var);
			break;

		default:
			//DVT_NULL
			//DVT_FLAGS
			enf_assert(!"Not implemeted");
			break;
		}
	}
	else
	{
		switch(type)
		{
		case DVT_SCALAR:
			((MaterialVarScalar*)var)->~MaterialVarScalar();
			break;
		case DVT_VECTOR:
			((MaterialVarVector*)var)->~MaterialVarVector();
			break;
		case DVT_MATRIX:
			((MaterialVarMatrix*)var)->~MaterialVarMatrix();
			break;
		case DVT_COLOR:
			((MaterialVarColor*)var)->~MaterialVarColor();
			break;
		case DVT_INTEGER:
			((MaterialVarInteger*)var)->~MaterialVarInteger();
			break;
		case DVT_BOOLEAN:
			((MaterialVarBoolean*)var)->~MaterialVarBoolean();
			break;
		case DVT_TEXTURE:
			//do not call this. It tries to free m_pTextures array, which does not own!
			//((MaterialVarTexture*)var)->~MaterialVarTexture();
			break;
		case DVT_STRING:
			((MaterialVarString*)var)->~MaterialVarString();
			break;

		case DVT_POINTER:
			((MaterialVarPointer*)var)->~MaterialVarPointer();
			break;

		default:
			//DVT_NULL
			//DVT_FLAGS
			enf_assert(!"Not implemeted");
			break;
		}
	}
}

//-----------------------------------------------------------------------
void Material::UnsetVariable(uint variable)
{
	enf_assert(variable < m_iNumVariables);
	if(m_pVariables[variable] != NULL)
	{
		const ubyte* start = m_pVariableBuffer;
		const ubyte* end = start + m_iBufferSize;

		
		DataVarType vt = m_pMClass->GetVariableType(variable);
		bool ownMemory = ((ubyte*)m_pVariables[variable] < start || (ubyte*)m_pVariables[variable] > end);
		enf::DeleteVariable(vt, m_pVariables[variable], ownMemory);
		m_pVariables[variable] = NULL;
	}
}

//-----------------------------------------------------------------------
void Material::FreeVariables()
{
	const ubyte* start = m_pVariableBuffer;
	const ubyte* end = start + m_iBufferSize;

	MaterialVarDef def;
	def.m_Type = DVT_SCALAR;
	for(uint f = 0; f < m_iNumDeformFuncs; f++)
	{
		for(uint v = 0; v < CFunc::MAX_PARMS; v++)
		{
			CFunc& func = m_pDeformFuncs[f];

			if(func.m_Parms[v])
			{
				bool ownMemory = ((ubyte*)func.m_Parms[v] < start || (ubyte*)func.m_Parms[v] > end);
				enf::DeleteVariable(DVT_SCALAR, func.m_Parms[v], ownMemory);
				func.m_Parms[v] = NULL;
			}
		}
	}

	if(m_pVariables != m_pEmptyVariables)
	{
		for(uint n = 0; n < m_iNumVariables; n++)
		{
			if(!IsVariableSet(n))
				continue;

			DataVarType vt = m_pMClass->GetVariableType(n);
			bool ownMemory = ((ubyte*)m_pVariables[n] < start || (ubyte*)m_pVariables[n] > end);
			enf::DeleteVariable(vt, m_pVariables[n], ownMemory);

			m_pVariables[n] = NULL;
		}

		if(m_pVariableBuffer)
		{
			delete[] m_pVariableBuffer;
			m_pVariableBuffer = NULL;
		}
		m_pVariables = m_pEmptyVariables;
	}
	m_iNumVariables = 0;

	if(m_pDeformFuncs)
	{
		bool ownMemory = ((ubyte*)m_pDeformFuncs < start || (ubyte*)m_pDeformFuncs > end);
		if(ownMemory)
			delete[] m_pDeformFuncs;

		m_pDeformFuncs = NULL;
		m_iNumDeformFuncs = 0;
	}

}

Vector4 StrToVector4(CStr str)
{
	Vector4 v;
	sscanf(str.cstr(), "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
	return v;
}

FColor StrToColor(CStr str)
{
	FColor v;
	sscanf(str.cstr(), "%f %f %f %f", &v.r, &v.g, &v.b, &v.a);
	return v;
}

bool StrToBool(CStr str)
{
	return str == "true";
}

//-----------------------------------------------------------------------
void Material::CopyVariables(Material* mat) const
{
	enf_assert(m_pMClass == mat->GetMaterialClass());

	for(uint n = 0; n < m_pMClass->GetNumVariables(); n++)
	{
		if(!IsVariableSet(n))
			continue;

		const MaterialVarDef* def = m_pMClass->GetVariableDef(n);
		switch(def->m_Type)
		{
		case DVT_INTEGER:
			mat->SetInteger(n, GetInteger(n));
			break;
		case DVT_POINTER:
			mat->SetPointer(n, GetPointer(n));
			break;
		case DVT_BOOLEAN:
			mat->SetBoolean(n, GetBoolean(n));
			break;
		case DVT_SCALAR:
			mat->SetScalar(n, GetScalar(n));
			break;
		case DVT_VECTOR:
			mat->SetVector(n, GetVector(n));
			break;
		case DVT_COLOR:
			mat->SetColor(n, GetColor(n));
			break;
		case DVT_TEXTURE:
			{
				const MaterialVarTexture* tex = GetTexture(n);
				uint numTexMaps = tex->GetNumTextureMaps();
				const char** names = ALLOCA(const char*, numTexMaps);

				for(uint t = 0; t < numTexMaps; t++)
				{
					const TextureMap* tmap = tex->GetTextureMap(t);
					names[t] = tmap->m_strName;
				}
				mat->SetTexture(n, names, numTexMaps, tex->m_iTexFlags);
			}
		}
	}
}

//-----------------------------------------------------------------------
void Material::LerpVariables(Material* targetMat, const Material* otherMat, float lerp) const
{
	enf_assert(m_pMClass == otherMat->GetMaterialClass());
	enf_assert(m_pMClass == targetMat->GetMaterialClass());

	for(uint n = 0; n < m_pMClass->GetNumVariables(); n++)
	{
		if(!IsVariableSet(n))
			continue;
		if(!otherMat->IsVariableSet(n))
			continue;

		const MaterialVarDef* def = m_pMClass->GetVariableDef(n);
		switch(def->m_Type)
		{
		case DVT_INTEGER:
			targetMat->SetInteger(n, enf_lerp((float)GetInteger(n), (float)otherMat->GetInteger(n), lerp));
			break;
		case DVT_POINTER:
			targetMat->SetPointer(n, (lerp <= 0.5f) ? GetPointer(n) : otherMat->GetPointer(n));
			break;
		case DVT_BOOLEAN:
			targetMat->SetBoolean(n, (lerp <= 0.5f) ? GetBoolean(n) : otherMat->GetBoolean(n));
			break;
		case DVT_SCALAR:
			targetMat->SetScalar(n, enf_lerp(GetScalar(n), otherMat->GetScalar(n), lerp));
			break;
		case DVT_VECTOR:
			targetMat->SetVector(n, GetVector(n).Lerp(otherMat->GetVector(n), lerp));
			break;
		case DVT_COLOR:
			targetMat->SetColor(n, GetColor(n).Lerp(otherMat->GetColor(n), lerp));
			break;
/*		case DVT_TEXTURE:
			{
				const MaterialVarTexture* tex = GetTexture(n);
				const MaterialVarTexture* otherTex = otherMat->GetTexture(n);
				uint numTexMaps = enf_min(tex->GetNumTextureMaps(), otherTex->GetNumTextureMaps());

				const char** names = ALLOCA(const char*, numTexMaps);

				for(uint t = 0; t < numTexMaps; t++)
				{
					const TextureMap* tmap = tex->GetTextureMap(t);
					const TextureMap* otherTmap = otherTex->GetTextureMap(t);
					names[t] = (lerp <= 0.5f) ? tmap->m_strName : otherTmap->m_strName;
				}
				targetMat->SetTexture(n, names, numTexMaps, tex->m_iTexFlags);
			}*/
		}
	}
}


//-----------------------------------------------------------------------
Material::CFunc::CFunc(): m_Type(Material::VCMOD_NONE), m_iTC(0)
{
	for(uint n = 0; n < MAX_PARMS; n++)
		m_Parms[n] = NULL;
}

//-----------------------------------------------------------------------
bool Material::Update(const MaterialSource* ms)
{
	CMutexLock lock(m_pMaterialSystem->m_Mutex);
	enf_assert(ms != NULL);

	Material* rootmat = m_pParent ? m_pParent : this;

	int loadcount = rootmat->m_iLoadCount;

	if(loadcount)
	{
		rootmat->m_iLoadCount = 1;
		Unload();
	}

	rootmat->FreeVariables();
	rootmat->InitDefaults();
	rootmat->ClearFlags(MF_FINALIZED);
	MaterialClass* matClass = rootmat->m_pMClass;

	if(matClass)
		rootmat->AllocVariables(matClass->GetNumVariables());

	IMaterialSystem* sys = (IMaterialSystem*)g_pMaterialSystem;

	for(uint n = 0; n < ms->GetNumVariables(); n++)
	{
		const MaterialSource::MatSrcVar* mv = ms->GetVariable(n);
		
		const MaterialVarDef* def;

		if(m_pMClass && (def = matClass->GetVariableDef(mv->Name)) != NULL)
		{
			switch(def->m_Type)
			{
			case DVT_SCALAR:
				if(mv->VarRef.IsSet())
				{
					rootmat->SetVariable(def->m_iIndex, def->m_Type, mv->VarRef.VariablePtr);
					rootmat->SetScalar(def->m_iIndex, def->m_strDefault.ToFloat());
				}
				else
				{
					rootmat->SetScalar(def->m_iIndex, mv->Enum ? ((ParmEnumScalar*)mv->Enum)->Value : mv->ToFloat());
				}
				break;

			case DVT_VECTOR:
				if(mv->VarRef.IsSet())
				{
					rootmat->SetVariable(def->m_iIndex, def->m_Type, mv->VarRef.VariablePtr);
					rootmat->SetVector(def->m_iIndex, StrToVector4(def->m_strDefault));
				}
				else
					rootmat->SetVector(def->m_iIndex, mv->ToVector());
				break;

			case DVT_MATRIX:
				if(mv->VarRef.IsSet())
					rootmat->SetVariable(def->m_iIndex, def->m_Type, mv->VarRef.VariablePtr);
				else
					rootmat->SetMatrix(def->m_iIndex, mv->ToMatrix());
				break;

			case DVT_COLOR:
				if(mv->VarRef.IsSet())
				{
					rootmat->SetVariable(def->m_iIndex, def->m_Type, mv->VarRef.VariablePtr);
					rootmat->SetColor(def->m_iIndex, StrToColor(def->m_strDefault));
				}
				else
				{
					rootmat->SetColor(def->m_iIndex, mv->Enum ? ((ParmEnumColor*)mv->Enum)->Value : mv->ToColor());
				}
				break;

			case DVT_INTEGER:
				if(mv->VarRef.IsSet())
				{
					rootmat->SetVariable(def->m_iIndex, def->m_Type, mv->VarRef.VariablePtr);
					rootmat->SetInteger(def->m_iIndex, mv->ToInt());
				}
				else
				{
					rootmat->SetInteger(def->m_iIndex, mv->Enum ? ((ParmEnumInteger*)mv->Enum)->Value : mv->ToInt());
				}
				break;

			case DVT_POINTER:
				if(mv->VarRef.IsSet())
					rootmat->SetVariable(def->m_iIndex, def->m_Type, mv->VarRef.VariablePtr);
				break;

			case DVT_TEXTURE:
				{
					MaterialSource::MatSrcVarTexture* mvt = (MaterialSource::MatSrcVarTexture*)mv;
					
					if(!mvt->Textures.IsEmpty())
					{
						uint flags = 0;

						for(uint f = 0; f < mvt->Flags.GetCardinality(); f++)
						{
							if(mvt->Flags[f] == "alpha")
								flags |= TMAP_ALPHA;
							else if(mvt->Flags[f] == "cube")
								flags |= TMAP_CUBE;
							else if(mvt->Flags[f] == "nomipmap")
								flags |= TMAP_NOMIPMAP;
							else if(mvt->Flags[f] == "clamp")
								flags |= (TMAP_CLAMPU|TMAP_CLAMPV);
							else if(mvt->Flags[f] == "clampu")
								flags |= TMAP_CLAMPU;
							else if(mvt->Flags[f] == "clampv")
								flags |= TMAP_CLAMPV;
							else if(mvt->Flags[f] == "nofilter")
								flags |= TMAP_NOFILTER;
							else if(mvt->Flags[f] == "noresize")
								flags |= TMAP_NORESIZE;
						}
						
						MaterialVarTexture* vt;

						if(mvt->Textures.GetCardinality() > 1)
							vt = rootmat->SetTexture(def->m_iIndex, mvt->Textures.GetArray(), mvt->Textures.GetCardinality(), flags);
						else
							vt = rootmat->SetTexture(def->m_iIndex, mvt->Textures[0], flags);

						if(vt->m_iUsage != MAP_IMAGE && vt->m_iUsage != MAP_EDITOR_ONLY && def->m_iLoadPriority != RESPRI_IMMEDIATE)
							vt->m_iTexFlags |= TEXTR_MANAGED;

						vt->m_iFPS = (ushort)(mvt->FPS * 100.0f);
						vt->Var = mvt->VarRef.VariablePtr;
					}
				}
				break;

			case DVT_BOOLEAN:
				if(mv->VarRef.IsSet())
				{
					rootmat->SetVariable(def->m_iIndex, def->m_Type, mv->VarRef.VariablePtr);
					rootmat->SetBoolean(def->m_iIndex, StrToBool(def->m_strDefault));
				}
				else
					rootmat->SetBoolean(def->m_iIndex, mv->ToBool());
				break;

			case DVT_STRING:
				if(mv->VarRef.IsSet())
				{
					rootmat->SetVariable(def->m_iIndex, def->m_Type, mv->VarRef.VariablePtr);
					rootmat->SetString(def->m_iIndex, def->m_strDefault);
				}
				else
					rootmat->SetString(def->m_iIndex, mv->Value.c_str());
				break;
			}
		}
		else
		{
			if((def = sys->GetVariableDef(mv->Name)) == NULL)
			{
				continue;
			}
			if(mv->Type == DVT_BOOLEAN && mv->Name == "castshadow")
			{
				if(mv->ToBool())
					rootmat->SetFlags(MF_CASTSHADOW);
				else
					rootmat->ClearFlags(MF_CASTSHADOW);
				continue;
			}

			if(mv->Type == DVT_BOOLEAN && mv->Name == "receiveshadow")
			{
				if(mv->ToBool())
					rootmat->SetFlags(MF_RECEIVESHADOW);
				else
					rootmat->ClearFlags(MF_RECEIVESHADOW);
				continue;
			}

			if(mv->Type == DVT_BOOLEAN && mv->Name == "mirror")
			{
				if(mv->ToBool())
					rootmat->SetFlags(MF_MIRROR);
				else
					rootmat->ClearFlags(MF_MIRROR);
				continue;
			}

			if(mv->Type == DVT_SCALAR && mv->Name == "distance")
			{
				rootmat->m_iMaxDistance = mv->ToInt();
				continue;
			}

			if(mv->Type == DVT_INTEGER && mv->Name == "sort")
			{
				int st = SORT_OPAQUE;

				def->FindEnumInteger(mv->Value.c_str(), st);

				rootmat->m_SortType = (SortType)st;
				continue;
			}

			if(mv->Type == DVT_INTEGER && mv->Name == "sortbias")
			{
				rootmat->m_iSortBias = enf_limit(mv->ToInt(), -128, 127);
				continue;
			}

			if(mv->Type == DVT_INTEGER && mv->Name == "cull")
			{
				if(mv->Value == "none")
					rootmat->ClearFlags(MF_CULL);
				else if(mv->Value == "ccw")
					rootmat->SetFlags(MF_CULL);
				continue;
			}

			if(mv->Type == DVT_FLAGS && mv->Name == "passmask")
			{
				MaterialSource::MatSrcVarFlags* mvf = (MaterialSource::MatSrcVarFlags*)mv;

				if(mvf->IsFlagSet("noambient"))
					rootmat->DisablePass(PT_AMBIENT);
				else
					rootmat->EnablePass(PT_AMBIENT);
				
				if(mvf->IsFlagSet("nolight"))
					rootmat->DisablePass(PT_DIFFUSE_SPECULAR);
				else
					rootmat->EnablePass(PT_DIFFUSE_SPECULAR);
				
				if(mvf->IsFlagSet("noenvironment"))
					rootmat->DisablePass(PT_ENVIRONMENT);
				else
					rootmat->EnablePass(PT_ENVIRONMENT);
				continue;
			}

			if(mv->Type == DVT_FLAGS && mv->Name == "surfparm")
			{
				MaterialSource::MatSrcVarFlags* mvf = (MaterialSource::MatSrcVarFlags*)mv;
			
				for(uint f = 0; f < mvf->Flags.GetCardinality(); f++)
				{
					int mask = 0;
					if(def->FindEnumInteger(mvf->Flags[f], mask))
					{
						rootmat->m_SurfParms.SetFlags(mask);
					}
				}
			}

			if(mv->Type == DVT_INTEGER && mv->Name == "physmaterial")
			{
				int matindex = 0;

				if(def->FindEnumInteger(mv->Value.c_str(), matindex))
				{
					rootmat->m_SurfParms.SetMaterialIndex(matindex);
				}
				continue;
			}
		}
	}

	//coordinate modifying function
	uint numDeformFuncs = ms->GetDeformFuncs().GetCardinality();
	if(numDeformFuncs)
	{
		rootmat->m_pDeformFuncs = ENF_NEW CFunc[numDeformFuncs];
		rootmat->m_iNumDeformFuncs = numDeformFuncs;

		for(uint n = 0; n < numDeformFuncs; n++)
		{
			int type = ms->GetDeformFuncs()[n].m_Type;

			CFunc& func = m_pDeformFuncs[n];

			func.m_Type = (CFuncType)type;

			if(type == VCMOD_SCALE ||
				type == VCMOD_SIN ||
				type == VCMOD_XYSIN/* ||
				type == TCMOD_TURB*/)
				rootmat->SetFlags(MF_NEEDSVCMOD);
			else
				rootmat->SetFlags(MF_NEEDSTMAT);

			for(uint p = 0; p < ms->GetDeformFuncs()[n].m_Variables.GetCardinality(); p++)
			{
				MaterialVarScalar* mv = ENF_NEW MaterialVarScalar();

				if(ms->GetDeformFuncs()[n].m_Variables[p].VarRef.IsSet())
					mv->Var = ms->GetDeformFuncs()[n].m_Variables[p].VarRef.VariablePtr;
				else
					mv->Value = ms->GetDeformFuncs()[n].m_Variables[p].ToFloat();

				func.m_Parms[p] = mv;
			}


			//little optimization. When center is zero, replace it by simpler function
			//TODO: give this opportunity to some deform-func handler, instead of the hardcode
			if(type == TCMOD_SCALE || type == TCMOD_SCALE2)
			{
				if(func.m_Parms[0]->Var == NULL && func.m_Parms[0]->Value == 0.0f &&
					func.m_Parms[1]->Var == NULL && func.m_Parms[1]->Value == 0.0f)
					func.m_Type = (CFuncType)(type + 1);
			}

      }
	}

	rootmat->Finalize();

	if(loadcount)
	{
		Load();
		rootmat->m_iLoadCount = loadcount;
	}
	return true;
}

//-----------------------------------------------------------------------
uint VariableSizeof(const MaterialVar* var, const BaseVariableDef* def)
{
	switch(def->m_Type)
	{
	case DVT_SCALAR:
		return sizeof(MaterialVarScalar);
	case DVT_INTEGER:
		return sizeof(MaterialVarInteger);
	case DVT_VECTOR:
		return sizeof(MaterialVarVector);
	case DVT_STRING:
		return sizeof(MaterialVarString);
	case DVT_COLOR:
		return sizeof(MaterialVarColor);
	case DVT_BOOLEAN:
		return sizeof(MaterialVarBoolean);
	case DVT_MATRIX:
		return sizeof(MaterialVarMatrix);
	case DVT_TEXTURE:
		{
			const MaterialVarTexture* tvar = static_cast<const MaterialVarTexture*>(var);
			if(tvar->GetNumTextureMaps())
				return sizeof(MaterialVarTexture) + tvar->GetNumTextureMaps() * sizeof(TextureMap);
			else
				return sizeof(MaterialVarTexture);
		}
	case DVT_POINTER:
		return sizeof(MaterialVarPointer);
	default:
		enf_assert(!"Unsupported");
	};
	return 0;
}
//-----------------------------------------------------------------------
ubyte* Material::VariableCopy(ubyte* dst, const MaterialVar* var, const BaseVariableDef* def)
{
	switch(def->m_Type)
	{
	case DVT_SCALAR:
		PLACEMENT_NEW(dst) MaterialVarScalar;
		*(MaterialVarScalar*)dst = *(MaterialVarScalar*)var;
		break;
	case DVT_INTEGER:
		PLACEMENT_NEW(dst) MaterialVarInteger;
		*(MaterialVarInteger*)dst = *(MaterialVarInteger*)var;
		break;
	case DVT_VECTOR:
		PLACEMENT_NEW(dst) MaterialVarVector;
		*(MaterialVarVector*)dst = *(MaterialVarVector*)var;
		break;
	case DVT_STRING:
		PLACEMENT_NEW(dst) MaterialVarString;
		*(MaterialVarString*)dst = *(MaterialVarString*)var;
		break;
	case DVT_COLOR:
		PLACEMENT_NEW(dst) MaterialVarColor;
		*(MaterialVarColor*)dst = *(MaterialVarColor*)var;
		break;
	case DVT_BOOLEAN:
		PLACEMENT_NEW(dst) MaterialVarBoolean;
		*(MaterialVarBoolean*)dst = *(MaterialVarBoolean*)var;
		break;
	case DVT_MATRIX:
		PLACEMENT_NEW(dst) MaterialVarMatrix;
		*(MaterialVarMatrix*)dst = *(MaterialVarMatrix*)var;
		break;
	case DVT_TEXTURE:
		PLACEMENT_NEW(dst) MaterialVarTexture;
		*(MaterialVarTexture*)dst = *(MaterialVarTexture*)var;
		{
			uint numTexMaps = ((MaterialVarTexture*)var)->GetNumTextureMaps();
			if(numTexMaps > 1)
			{
				((MaterialVarTexture*)dst)->m_pTextures = (TextureMap*)(dst + sizeof(MaterialVarTexture));
				memcpy(((MaterialVarTexture*)dst)->m_pTextures, ((MaterialVarTexture*)var)->m_pTextures, sizeof(TextureMap) * numTexMaps);
			}
		}
		break;
	case DVT_POINTER:
		PLACEMENT_NEW(dst) MaterialVarPointer;
		*(MaterialVarPointer*)dst = *(MaterialVarPointer*)var;
		break;
	default:
		enf_assert(!"Unsupported");
	};
	return dst + VariableSizeof(var, def);
}

//-----------------------------------------------------------------------
void Material::Finalize()
{
	Material* rootmat = m_pParent ? m_pParent : this;
	
	rootmat->InternalFinalize();

	//update clones too!
	Material* clone = rootmat->m_pNextClone;

	while(clone)
	{
		CloneData(clone);
		clone->InternalFinalize();
		clone = clone->m_pNextClone;
	}

	if(rootmat->IsFlagSet(MF_FINALIZED))
		return;

	if(rootmat->m_pVariables != m_pEmptyVariables || GetNumDeformFuncs())
	{
		//compute memory size for all variables and put them into one buffer
		uint numVariables = rootmat->m_pMClass->GetNumVariables();
		uint varSizeof = 0;

		if(rootmat->m_pVariables != m_pEmptyVariables)
		{
			varSizeof += numVariables * sizeof(MaterialVar*);
			for(uint n = 0; n < rootmat->m_pMClass->GetNumVariables(); n++)
			{
				if(!rootmat->IsVariableSet(n))
					continue;
				varSizeof = varSizeof + VariableSizeof(rootmat->GetVariable(n), rootmat->m_pMClass->GetVariableDef(n));
			}
		}

		for(uint f = 0; f < rootmat->GetNumDeformFuncs(); f++)
		{
			varSizeof += sizeof(CFunc);
			for(uint p = 0; p < CFunc::MAX_PARMS; p++)
			{
				if(rootmat->m_pDeformFuncs[f].m_Parms[p])
				{
					varSizeof += sizeof(MaterialVarScalar);
				}
			}
		}

		ubyte* buffer = ENF_NEW ubyte[varSizeof];
		ubyte* ptr = buffer;
		MaterialVar** materialVars = NULL;

		if(rootmat->m_pVariables != m_pEmptyVariables)
		{
			materialVars = (MaterialVar**)ptr;
			ptr += sizeof(MaterialVar**) * numVariables;

			for(uint n = 0; n < numVariables; n++)
			{
				if(rootmat->IsVariableSet(n))
				{
					materialVars[n] = (MaterialVar*)ptr;
					ptr = VariableCopy(ptr, rootmat->GetVariable(n), rootmat->m_pMClass->GetVariableDef(n));
				}
				else
					materialVars[n] = NULL;
			}
		}
		MaterialVarDef scalarDef;
		scalarDef.m_Type = DVT_SCALAR;

		CFunc* deformFuncs = NULL;
		uint numDeformFuncs = rootmat->GetNumDeformFuncs();
		if(numDeformFuncs)
		{
			deformFuncs = (CFunc*)ptr;
			ptr += sizeof(CFunc) * numDeformFuncs;
			for(uint f = 0; f < numDeformFuncs; f++)
			{
				deformFuncs[f].m_Type = rootmat->m_pDeformFuncs[f].m_Type;
				deformFuncs[f].m_iTC = rootmat->m_pDeformFuncs[f].m_iTC;
				for(uint p = 0; p < CFunc::MAX_PARMS; p++)
				{
					if(rootmat->m_pDeformFuncs[f].m_Parms[p])
					{
						deformFuncs[f].m_Parms[p] = (MaterialVarScalar*)ptr;
						ptr = VariableCopy(ptr, rootmat->m_pDeformFuncs[f].m_Parms[p], &scalarDef);
					}
					else
						deformFuncs[f].m_Parms[p] = NULL;
				}
			}
		}
		enf_assert((ptr - buffer) == varSizeof);
		
		rootmat->FreeVariables();
		rootmat->m_iNumVariables = numVariables;
		rootmat->m_pVariableBuffer = buffer;
		rootmat->m_iBufferSize = varSizeof;
		rootmat->m_pDeformFuncs = deformFuncs;
		rootmat->m_iNumDeformFuncs = numDeformFuncs;

		Material* clone = rootmat->m_pNextClone;

		//refresh pointer copies
		while(clone)
		{
			clone->m_pVariableBuffer = rootmat->m_pVariableBuffer;
			clone->m_iBufferSize = rootmat->m_iBufferSize;
			clone->m_iNumVariables = rootmat->m_iNumVariables;
			clone->m_pDeformFuncs = rootmat->m_pDeformFuncs;
			clone->m_iNumDeformFuncs = rootmat->m_iNumDeformFuncs;
			clone = clone->m_pNextClone;
		}
	}
	SetFlags(MF_FINALIZED);
}

//-----------------------------------------------------------------------
void Material::CloneData(Material* mat)
{
	mat->m_iFlags = m_iFlags;
	mat->m_SurfParms = m_SurfParms;
	mat->m_iSortID = 	m_iSortID;
	mat->m_iMaxDistance = m_iMaxDistance;
	mat->m_iSortBias = m_iSortBias;
	mat->m_SortType = m_SortType;

	mat->m_pDeformFuncs = m_pDeformFuncs;
	mat->m_iNumDeformFuncs = m_iNumDeformFuncs;
	mat->m_pVariableBuffer = m_pVariableBuffer;
	mat->m_iBufferSize = m_iBufferSize;
	mat->m_iNumVariables = m_iNumVariables;
}

//-----------------------------------------------------------------------
Material* Material::CreateClone(uint creationflags)
{
	CMutexLock lock(m_pMaterialSystem->m_Mutex);
	//clones only from root material
	enf_assert(m_pParent == NULL);

	Material* mat = ENF_NEW Material(m_pMaterialSystem, GetName());

	CloneData(mat);
	
	mat->m_pMClass = m_pMClass;
	//add it to chain of clones
	mat->m_pParent = this;
	mat->m_pNextClone = m_pNextClone;
	mat->m_iCreationFlags = creationflags;
	m_pNextClone = mat;
	AddRef();
	return mat;
}

//-----------------------------------------------------------------------
float Material::_GetScalar(const Variable* var, float defValue) const
{
	const IVariable* iv = GetIVariable(var);
	return iv ? iv->Float : defValue;
}

//-----------------------------------------------------------------------
int Material::_GetInteger(const Variable* var, int defValue) const
{
	const IVariable* iv = GetIVariable(var);
	return iv ? iv->Int : defValue;
}

//-----------------------------------------------------------------------
FColor Material::_GetColor(const Variable* var, const FColor& defValue) const
{
	const IVariable* ivar = GetIVariable(var);
	if(ivar == NULL)
		return defValue;

	switch(var->GetVType())
	{
	case VARTYPE_VECTOR:
		const float* fptr;
		fptr = (const float*)ivar->Pointer;
		return FColor(fptr[0], fptr[1], fptr[2], 1.0f);

	case VARTYPE_INT:
		return FColor((float)(ivar->Int & 0xff) * FCOLOR_SCALE, (float)((ivar->Int & 0xff00) >> 8) * FCOLOR_SCALE, (float)((ivar->Int & 0xff0000) >> 16) * FCOLOR_SCALE, (float)((ivar->Int & 0xff000000) >> 24) * FCOLOR_SCALE);

	case VARTYPE_FLOAT:
		if(var->IsPtr())
			return *(FColor*)ivar->Pointer;
		else
			return FColor1(ivar->Float);
	}
	return defValue;
}


//-----------------------------------------------------------------------
CStr Material::GetString(uint var) const
{
	enf_assert(IsType(var, DVT_STRING));
	enf_assert(IsVariableSet(var));

	MaterialVarString* mv = (MaterialVarString*)m_pVariables[var];

	if(mv->Var)
	{
		const IVariable* iv = GetIVariable(mv->Var);
		if(iv)
			return iv->String;
	}

	return mv->Value.c_str();
}

//-----------------------------------------------------------------------
Matrix44 Material::GetMatrix(uint var) const
{
	enf_assert(IsType(var, DVT_MATRIX));
	enf_assert(IsVariableSet(var));

	MaterialVarMatrix* mv = (MaterialVarMatrix*)m_pVariables[var];

	if(mv->Var)
	{
		const IVariable* ivar = GetIVariable(mv->Var);

		if(ivar)
		{
#ifdef ENF_DONE
			if(mv->Var->GetVType() == VARTYPE_VECTOR)
			{
				//we suppose it's vector[4]
				enf_assert(mv->Var->ArraySize() >= 12);
				return *(Matrix43*)ivar->Pointer;
			}
			else if(mv->Var->GetVType() == VARTYPE_FLOAT && mv->Var->IsPtr())
			{
				enf_assert(mv->Var->ArraySize() >= 16);
				return *(Matrix44*)ivar->Pointer;

			}
			else
#endif
			{
				enf_assert(!"Not implemented");
			}
		}
	}

	return mv->Value;
}

//-----------------------------------------------------------------------
ebool Material::GetBoolean(uint var) const
{
	enf_assert(IsType(var, DVT_BOOLEAN));
	enf_assert(IsVariableSet(var));

	MaterialVarBoolean* mv = (MaterialVarBoolean*)m_pVariables[var];

	if(mv->Var)
	{
		const IVariable* ivar = GetIVariable(mv->Var);
		if(ivar)
			return ivar->Int != 0;
	}

	return mv->Value;
}

//-----------------------------------------------------------------------
void* Material::GetPointer(uint var) const
{
	enf_assert(IsType(var, DVT_POINTER));
	enf_assert(IsVariableSet(var));

	MaterialVarPointer* mv = (MaterialVarPointer*)m_pVariables[var];

	if(mv->Var)
	{
		const IVariable* ivar = GetIVariable(mv->Var);
		if(ivar)
			return GetIVariable(mv->Var)->Pointer;
	}

	return mv->Value;
}

//-----------------------------------------------------------------------
Vector4 Material::GetVector(uint var) const
{
	enf_assert(IsType(var, DVT_VECTOR));
	enf_assert(IsVariableSet(var));

	MaterialVarVector* mv = (MaterialVarVector*)m_pVariables[var];

	if(mv->Var)
	{
		const IVariable* ivar = GetIVariable(mv->Var);
		if(ivar)
		{
			if(mv->Var->GetVType() == VARTYPE_VECTOR)
			{
				const float* fptr = (const float*)ivar->Pointer;
				return Vector4(fptr[0], fptr[1], fptr[2], 1.0f);
			}
			else
				return *(Vector4*)ivar->Pointer;
		}
	}

	return mv->Value;
}

//-----------------------------------------------------------------------
/*
#define SET_VARIABLE(pvar, var, value, type) \
	type* pvar = static_cast<type*>(m_pVariables[var]); \
	if(pvar == NULL) \
	{ \
		ClearFlags(MF_FINALIZED); \
		pvar = ENF_NEW type; \
		m_pVariables[var] = pvar; \
	} else { \
		if(pvar->Value == value) \
			return pvar; \
	}\
	pvar->Value = value;
*/

template<class T, class V> T* Material::SET_VARIABLE(uint var, V value)
{
	T* pvar = static_cast<T*>(m_pVariables[var]);
	if(pvar == NULL)
	{
		if(m_pParent && m_pVariables == m_pParent->m_pVariables)
		{
			//do something :)
			m_pParent = m_pParent;
		}
		ClearFlags(MF_FINALIZED);
		pvar = ENF_NEW T;
		m_pVariables[var] = pvar;
	}
	else
	{
		if(pvar->Value == value)
			return pvar;
		if(m_pParent && m_pVariables == m_pParent->m_pVariables)
		{
			//do something :)
			m_pParent = m_pParent;
		}
	}
	pvar->Value = value;
	return pvar;
}

//-----------------------------------------------------------------------
MaterialVarScalar* Material::SetScalar(uint var, float value)
{
	enf_assert(IsType(var, DVT_SCALAR));
	return SET_VARIABLE<MaterialVarScalar, float>(var, value);
}

//-----------------------------------------------------------------------
MaterialVarString* Material::SetString(uint var, CStr value)
{
	enf_assert(IsType(var, DVT_STRING));
	return SET_VARIABLE<MaterialVarString, CStr>(var, value);
}

//-----------------------------------------------------------------------
MaterialVarMatrix* Material::SetMatrix(uint var, const Matrix44& value)
{
	enf_assert(IsType(var, DVT_MATRIX));
	return SET_VARIABLE<MaterialVarMatrix, Matrix44>(var, value);
}

//-----------------------------------------------------------------------
MaterialVarInteger* Material::SetInteger(uint var, int value)
{
	enf_assert(IsType(var, DVT_INTEGER));
	return SET_VARIABLE<MaterialVarInteger, int>(var, value);
}

//-----------------------------------------------------------------------
MaterialVarBoolean* Material::SetBoolean(uint var, ebool value)
{
	enf_assert(IsType(var, DVT_BOOLEAN));
	return SET_VARIABLE<MaterialVarBoolean, ebool>(var, value);
}

//-----------------------------------------------------------------------
MaterialVarPointer* Material::SetPointer(uint var, void* value)
{
	enf_assert(IsType(var, DVT_POINTER));
	return SET_VARIABLE<MaterialVarPointer, void*>(var, value);
}

//-----------------------------------------------------------------------
MaterialVarColor* Material::SetColor(uint var, const FColor& value)
{
	enf_assert(IsType(var, DVT_COLOR));
	return SET_VARIABLE<MaterialVarColor, const FColor&>(var, value);
}

//-----------------------------------------------------------------------
MaterialVarVector* Material::SetVector(uint var, const Vector4& value)
{
	enf_assert(IsType(var, DVT_VECTOR));
	return SET_VARIABLE<MaterialVarVector, const Vector4&>(var, value);
}

//-----------------------------------------------------------------------
void Material::DeleteVariable(uint variable)
{
	enf_assert(variable < m_iNumVariables);
	enf_assert(!IsFlagSet(MF_FINALIZED));

	MaterialVar* mvar = m_pVariables[variable];
	if(mvar)
	{
		enf::DeleteVariable(m_pMClass->GetVariableType(variable), mvar, true);
		m_pVariables[variable] = NULL;
	}
}

//-----------------------------------------------------------------------
MaterialVar* Material::SetVariable(uint var, DataVarType type, const Variable* pvar)
{
	enf_assert(IsType(var, type));
	MaterialVar* mvar = m_pVariables[var];

	if(mvar == NULL)
	{
		switch(type)
		{
		case DVT_VECTOR:
			mvar = ENF_NEW MaterialVarVector;
			break;

		case DVT_SCALAR:
			mvar = ENF_NEW MaterialVarScalar;
			break;

		case DVT_COLOR:
			mvar = ENF_NEW MaterialVarColor;
			break;

		case DVT_INTEGER:
			mvar = ENF_NEW MaterialVarInteger;
			break;

		case DVT_POINTER:
			mvar = ENF_NEW MaterialVarPointer;
			break;

		case DVT_BOOLEAN:
			mvar = ENF_NEW MaterialVarBoolean;
			break;

		default:
			enf_assert(!"Not implemented");
			break;
		}
		m_pVariables[var] = mvar;
	}

	if(mvar)
	{
		mvar->Var = pvar;
	}
	return mvar;
}

//-----------------------------------------------------------------------
void Material::InternalFinalize()
{
	if(m_pMClass)
	{
		if(IsFlagSet(MF_NOSELFSHADOW) && m_SortType == SORT_OPAQUE)
		{
			m_SortType = SORT_OPAQUENOSELFSHADOW;
		}

		if(IsFlagSet(MF_MIRROR))
		{
			m_SortType = SORT_NEAR;
		}
		m_pMClass->OnMaterialFinalize(this);
	}
	
	//when we have any per-instance variable, we cannot instance
	if(IsFlagSet(MF_INSTANCING))
	{
		for(uint n = 0; n < m_iNumVariables; n++)
		{
			const MaterialVar* mv = m_pVariables[n];

			if(mv && mv->Var && mv->Var->Pos >= 0)
			{
				ClearFlags(MF_INSTANCING);
				break;
			}
		}

		for(uint n = 0; n < m_iNumDeformFuncs; n++)
		{
			const CFunc& cf = m_pDeformFuncs[n];
			for(uint p = 0; p < CFunc::MAX_PARMS; p++)
			{
				if(cf.m_Parms[p] && cf.m_Parms[p]->Var && cf.m_Parms[p]->Var->Pos >= 0)
				{
					ClearFlags(MF_INSTANCING);
					break;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------
void Material::AllocVariables(uint numVariables)
{
	enf_assert(m_pVariables == m_pEmptyVariables);
	m_iNumVariables = numVariables;
	if(numVariables)
	{
		//initial size
		m_iBufferSize = sizeof(MaterialVar*) * numVariables;
		m_pVariableBuffer = ENF_NEW ubyte[m_iBufferSize];
		ZeroMem(m_pVariableBuffer, m_iBufferSize);
	}
}
//-----------------------------------------------------------------------
bool Material::SetMaterialClass(MaterialClass* mclass)
{
	bool firstclass = (m_pMClass == NULL);

	m_pMClass = mclass;
	if(mclass == NULL)
		return true;

	if(mclass->OnMaterialSet(this))
	{
		//if it is our first class, initialize variable array.
		//otherwise we suppose we already have one, that is created
		//according some superclass of this one
		if(firstclass || m_iNumVariables != mclass->GetNumVariables())
		{
			//do not release variables of cloned materials!
			enf_assert(m_pParent == NULL);
			FreeVariables();
			AllocVariables(mclass->GetNumVariables());
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------
bool Material::ChangeMaterialClass(MaterialClass* mclass)
{
	enf_assert(mclass != NULL);
	enf_assert(!IsClone());

	FreeVariables();

	m_pMClass = mclass;
	if(mclass == NULL)
		return true;

	if(mclass->OnMaterialSet(this))
	{
		AllocVariables(mclass->GetNumVariables());

		if(m_pNextClone)
		{
			//TODO:
			//1. all Materials has to keep creation flags
			//2. all clones must be traversed and checked against new MaterialClass for compatibility
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------
bool Material::TouchTexture(uint variable) const
{
	enf_assert(m_pMClass->GetVariableType(variable) == DVT_TEXTURE);

	MaterialVarTexture* mm = (MaterialVarTexture*)m_pVariables[variable];
	TextureMap* tmaps = (mm->m_iNumTextures > 1) ? mm->m_pTextures : &mm->m_Texture;

	bool loaded = true;
	for(int t = 0; t < mm->m_iNumTextures; t++)
	{
		if(tmaps[t].m_strName == NULL || tmaps[t].m_strName[0] == '$')
			continue;

		if(mm->m_iUsage == MAP_EDITOR_ONLY && !m_pMaterialSystem->IsEditorMode())
			continue;

		if(tmaps[t].m_pTexture != NULL)
		{
			tmaps[t].m_pTexture->Touch(0);
			if(!tmaps[t].m_pTexture->IsLoaded())
			{
				loaded = false;
			}
		}
	}

	return loaded;
}

//-----------------------------------------------------------------------
bool Material::Touch(float level)
{
	uint timeStamp = m_pMaterialSystem->m_iTimeStamp;
	Material* rootmat = m_pParent ? m_pParent : this;
	
	//enf_assert2(m_iLoadCount > 0, "Trying to touch&use unloaded material");
	if(IL_EXCHANGE(rootmat->m_iTimeStamp, timeStamp) == timeStamp)
	{
		return rootmat->IsFlagSet(MF_TEXLOADED);//????rootmat->SetFlags(MF_TEXLOADED);
	}

	bool loaded = m_pMClass->TouchTextures(this);

	if(!loaded)
		rootmat->ClearFlags(MF_TEXLOADED);
	else
		rootmat->SetFlags(MF_TEXLOADED);
	return loaded;
}

//-----------------------------------------------------------------------
uint Material::Load()
{
	//there is more than one Material sharing one variable set.
	//Therefore we work with m_iLoadCount of root material only!
	Material* rootmat = m_pParent ? m_pParent : this;

	uint loadCount = IL_INCREMENT(rootmat->m_iLoadCount);
	if(loadCount == 1)
	{
		for(uint n = 0; n < m_iNumVariables; n++)
		{
			if(!IsVariableSet(n))
				continue;

			const MaterialVarDef* vdef = m_pMClass->GetVariableDef(n);

			if(vdef->m_Type != DVT_TEXTURE)
				continue;

			MaterialVarTexture* mm = (MaterialVarTexture*)m_pVariables[n];
			TextureMap* tmaps = (mm->m_iNumTextures > 1) ? mm->m_pTextures : &mm->m_Texture;

			for(int t = 0; t < mm->m_iNumTextures; t++)
			{
				if(tmaps[t].m_strName == NULL || tmaps[t].m_strName[0] == '$')
					continue;

				if(mm->m_iUsage == MAP_EDITOR_ONLY && !m_pMaterialSystem->IsEditorMode())
					continue;

				uint flags = 0;
				if(mm->m_iUsage == MAP_ENVIRONMENT)
					flags |= TEXTR_CUBE;

				tmaps[t].m_pTexture = GetResources()->CreateTextureFromFile(tmaps[t].m_strName, mm->m_iTexFlags | flags, (TextureUsage)mm->m_iUsage, vdef->m_iLoadPriority);
				if(tmaps[t].m_pTexture == NULL)
				{
					enf::LogF(LL_ERROR, "Can't load '%s' '%s', in material '%s'",
						vdef->m_strName.cstr(),
						tmaps[t].m_strName,
						GetName().cstr());

					tmaps[t].m_pTexture = GetResources()->CreateTextureFromFile("textures/default", 0, (TextureUsage)mm->m_iUsage);
				}
			}
		}
		if(m_pMClass) m_pMClass->OnMaterialLoaded(this);
	}
	return loadCount;
}

//-----------------------------------------------------------------------
uint Material::Unload()
{
	//there is more than one Material sharing one variable set.
	//Therefore we work with m_iLoadCount of root material only!
	Material* rootmat = m_pParent ? m_pParent : this;
	uint loadCount = IL_DECREMENT(rootmat->m_iLoadCount);
	if(loadCount == 0)
	{
		rootmat->ClearFlags(MF_TEXLOADED);
		if(m_pMClass) m_pMClass->BeforeMaterialUnload(this);

		//unload textures
		for(uint n = 0; n < m_iNumVariables; n++)
		{
			if(m_pMClass->GetVariableType(n) != DVT_TEXTURE)
				continue;

			if(!IsVariableSet(n))
				continue;

			MaterialVarTexture* mm = (MaterialVarTexture*)m_pVariables[n];
			TextureMap* tmaps = (mm->m_iNumTextures > 1) ? mm->m_pTextures : &mm->m_Texture;

      	for(int t = 0; t < mm->m_iNumTextures; t++)
			{
				if(tmaps[t].m_strName == NULL || tmaps[t].m_strName[0] == '$')
					continue;

				if(tmaps[t].m_pTexture)
					tmaps[t].m_pTexture->Release();
			}
		}
	}

	return loadCount;
}

//-----------------------------------------------------------------------
Material::Material(MaterialSystem* msys, CStr name):
	m_pMaterialSystem((IMaterialSystem*)msys),
	m_iLoadCount(0),
	m_pMClass(NULL),
	m_pVariables(m_pEmptyVariables),
	m_iNumVariables(0),
	m_iBufferSize(0),
	m_pNextClone(NULL),
	m_pParent(NULL),
	m_iCreationFlags(0),
	m_iSortID(0),
	m_iTimeStamp(-1)
{
	m_strName = g_pStatic->AddString(name.cstr());
	const char* bname = m_strName.strrchr('/');
	if(bname == NULL)
		m_strMatName = m_strName;
	else
		m_strMatName = bname + 1;
	
	InitDefaults();
}

//-----------------------------------------------------------------------
Material::~Material()
{
	CMutexLock lock(m_pMaterialSystem->m_Mutex);
	if(IsClone())
	{
		//remove from parent's list
		Material** prev = &m_pParent->m_pNextClone;

		while(*prev)
		{
			if((*prev) == this)
			{
				*prev = this->m_pNextClone;
				break;
			}
			prev = &(*prev)->m_pNextClone;
		}
		m_pParent->Release();
	}
	else
	{
		//TODO: obcas se to stane pri uvolnovani leaknutych materialu. Proc???
//		enf_assert(m_pNextClone == NULL);

		enf_assert(m_iLoadCount == 0);

		FreeVariables();


		if(m_pMaterialSystem)
			m_pMaterialSystem->RemoveMaterial(this);
	}

	if(m_pMClass)
		m_pMClass->OnMaterialDelete(this);
}

//-----------------------------------------------------------------------
MaterialVarTexture* Material::SetTexture(uint var, Texture* texture)
{
	MaterialVarTexture* mm = (MaterialVarTexture*)m_pVariables[var];

	if(mm == NULL)
	{
		m_pVariables[var] = mm = ENF_NEW MaterialVarTexture;
		mm->m_iNumTextures = 1;

		const MaterialVarDef* def = m_pMClass->GetVariableDef(var);
		if(def)
		{
			//It's better to make copy of texture usage
			//instead of peeking there in runtime for each texture
			//furthermore, OnFinalize can change type to better one. MAP_NORMAL_DXT instead of MAP_NORMAL for example
			mm->m_iUsage = (TextureUsage)def->m_iUsage;
		}

		mm->m_Texture.m_pTexture = texture;
		mm->m_Texture.m_strName = NULL;
		//we don't need flags ATM, as we suppose texture is initialized in its best way
		mm->m_iTexFlags = 0;
	}
	else
	{
		enf_assert(mm->m_iNumTextures == 1);
		if(mm->m_Texture.m_pTexture != texture)
		{
			SAFE_RELEASE(mm->m_Texture.m_pTexture);
			mm->m_Texture.m_pTexture = texture;

			if (texture && texture->GetName().IsEmpty())
			{
				mm->m_Texture.m_strName = NULL;	
			}
		}
	}
	return mm;
}

//-----------------------------------------------------------------------
MaterialVarTexture* Material::SetTexture(uint var, const char **names, uint num, uint flags)
{
SString<> tname;

	enf_assert(m_pVariables[var] == NULL);

	TextureMap* tmaps;
	MaterialVarTexture* mm = ENF_NEW MaterialVarTexture;

	m_pVariables[var] = mm;

	mm->m_iNumTextures = num;

	const MaterialVarDef* def;
	if((def = m_pMClass->GetVariableDef(var)) != NULL)
	{
		//It's better to make copy of texture usage
		//instead of peeking there in runtime for each texture
		//furthermore, OnFinalize can change type to better one. MAP_NORMAL_DXT instead of MAP_NORMAL for example
		mm->m_iUsage = (TextureUsage)def->m_iUsage;

		if(def->m_iUsage == MAP_NORMAL)
			flags |= TMAP_NORMALIZE;
	}

	if(num > 1)
	{
		mm->m_pTextures = ENF_NEW TextureMap[num];
		tmaps = mm->m_pTextures;
	}
	else
	{
		tmaps = &mm->m_Texture;
	}

	for(uint n = 0; n < num; n++)
   {
		tmaps[n].m_pTexture = NULL;
		tmaps[n].m_strName = NULL;

   	if(names[n] == NULL)
      	continue;

		tname = names[n];
		
		if(tname[0] == '$')
		{
			if(tname == "$renderview")
				flags |= TMAP_RENDERVIEW;
			else if(tname == "$rendertarget")
				flags |= TMAP_GUI;
			else if(tname == "$rendertarget2")
				flags |= TMAP_GUI;
			else if(tname == "$rendertarget3")
				flags |= TMAP_GUI;
			else if(tname == "$rendertarget4")
				flags |= TMAP_GUI;
			else if(tname == "$nearestcube")
				flags |= TMAP_NEARESTCUBE;
			else if(tname == "$current")
				flags |= TMAP_CURRENTCUBE;
		}

		tmaps[n].m_strName = g_pStatic->AddString(tname.StripExt());
   }

	mm->m_iTexFlags = 0;

	if(flags & TMAP_CUBE)
		mm->m_iTexFlags = TEXTR_CUBE | TEXTR_NOMIPMAP;

	if(flags & TMAP_NOFILTER)
	{
		mm->m_iTexFlags |= TEXTR_NOFILTER;
	}

	if(flags & TMAP_CLAMPU)
	{
		mm->m_iTexFlags |= TEXTR_CLAMPU;
	}

	if(flags & TMAP_CLAMPV)
	{
		mm->m_iTexFlags |= TEXTR_CLAMPV;
	}

	if(flags & TMAP_NOMIPMAP)
	{
		mm->m_iTexFlags |= TEXTR_NOMIPMAP;
	}

	if(flags & TMAP_NORESIZE)
	{
		mm->m_iTexFlags |= TEXTR_NORESIZE;
	}

	if(flags & TMAP_NEARESTCUBE)
	{
		SetFlags(MF_NEEDSENVMAP);
		mm->m_iTexFlags |= TEXTR_SPECIAL;
	}

	if(flags & TMAP_RENDERVIEW)
	{
		SetFlags(MF_NEEDSRENDERVIEW);
		mm->m_iTexFlags |= TEXTR_SPECIAL;
	}

	if(flags & TMAP_GUI)
	{
		SetFlags(MF_NEEDSGUI);
		mm->m_iTexFlags |= TEXTR_SPECIAL;
	}

	return mm;
}

}