//-----------------------------------------------------------------------------
// File: enf_materialsource.cpp
//
// Desc: Implementation of MaterialSource
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include <stdio.h>
#include "common/enf_staticbuffer.h"
#include "materials/enf_materialsource.h"
#include "materials/enf_materialclass.h"
#include "script/enf_scriptapi.h"
#include "common/enf_parser.h"
#include "engine/enf_engine.h"
#include "system/enf_vfilesystem.h"
#include "common/enf_streams.h"

namespace enf
{
char IncParmError[] = "Incompatible parameter '%s'";
char CFuncNumError[] = "Too much functions";
char FramesError[] = "Layer without textures";
char EffectParmError[] = "Too much parms on material";
char PassParmError[] = "Too much passes on material";

//-----------------------------------------------------------------------
MaterialSource* MaterialSourceCollection::FindMaterial(CStr name) const
{
	return m_MaterialsHash.Find(name.cstr());
}

//-----------------------------------------------------------------------
MaterialSource::MatSrcVar* MaterialSource::SetDeformFuncVariable(uint func, uint variable, const VariableRef& ref)
{
	m_DeformFuncs[func].m_Variables[variable].VarRef = ref;
	m_DeformFuncs[func].m_Variables[variable].Value = "";
	SetChanged();
	return &m_DeformFuncs[func].m_Variables[variable];
}

//-----------------------------------------------------------------------
MaterialSource::MatSrcVar* MaterialSource::SetDeformFuncVariable(uint func, uint variable, CStr value)
{
	m_DeformFuncs[func].m_Variables[variable].VarRef.Clear();
	m_DeformFuncs[func].m_Variables[variable].Value = value.cstr();
	SetChanged();
	return &m_DeformFuncs[func].m_Variables[variable];
}

//-----------------------------------------------------------------------
uint MaterialSource::AddDeformFunc(const ParmEnumMaterialDeform& def)
{
	DeformFunc func;

	func.m_Type = (Material::CFuncType)def.Type;
	for(uint n = 0; n < Material::CFunc::MAX_PARMS; n++)
	{
		if(def.Parms[n] != DVT_NULL)
			func.m_Variables.Insert(MatSrcVar(def.Parms[n], NULL));
	}

	SetChanged();
	return m_DeformFuncs.Insert(func);
}

//-----------------------------------------------------------------------
uint MaterialSource::InsertDeformFunc(const ParmEnumMaterialDeform& def, uint index)
{
	DeformFunc func;

	func.m_Type = (Material::CFuncType)def.Type;
	for(uint n = 0; n < Material::CFunc::MAX_PARMS; n++)
	{
		if(def.Parms[n] != DVT_NULL)
			func.m_Variables.Insert(MatSrcVar(def.Parms[n], NULL));
	}

	SetChanged();

	return m_DeformFuncs.InsertAt(func, index);
}

//-----------------------------------------------------------------------
void MaterialSource::ChangeMaterialClass(MaterialClass* mc)
{
	m_pMClass = mc;
	m_pMaterial->ChangeMaterialClass(mc);
	m_pMaterial->Update(this);
	SetChanged();
}

//-----------------------------------------------------------------------
bool MaterialSource::MatSrcVar::ToBool() const
{
	if(Value == "true")
	{
		return true;
	}
	if(Value == "false")
	{
		return false;
	}
	enf_assert(!"You should not get here!");
	return false;
}

//-----------------------------------------------------------------------
float MaterialSource::MatSrcVar::ToFloat() const
{
	return CStr(Value.c_str()).ToFloat();
}

//-----------------------------------------------------------------------
int MaterialSource::MatSrcVar::ToInt() const
{
	return CStr(Value.c_str()).ToInt();
}

//-----------------------------------------------------------------------
FColor MaterialSource::MatSrcVar::ToColor() const
{
	FColor color(0.0f, 0.0f, 0.0f, 1.0f);

	sscanf(Value.c_str(), "%f %f %f %f", &color.r, &color.g, &color.b, &color.a);

	return color;
}

//-----------------------------------------------------------------------
Vector4 MaterialSource::MatSrcVar::ToVector() const
{
	Vector4 vec(0.0f, 0.0f, 0.0f, 1.0f);

	sscanf(Value.c_str(), "%f %f %f %f", &vec.x, &vec.y, &vec.z, &vec.w);

	return vec;
}

//-----------------------------------------------------------------------
Matrix44 MaterialSource::MatSrcVar::ToMatrix() const
{
	Matrix44 m(true);

	sscanf(Value.c_str(), "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
		&m._11, &m._12, &m._13, &m._14, 
		&m._21, &m._22, &m._23, &m._24, 
		&m._31, &m._32, &m._33, &m._34, 
		&m._41, &m._42, &m._43, &m._44);

	return m;
}

//-----------------------------------------------------------------------
void MaterialSource::MatSrcVarFlags::SetFlag(CStr flag)
{
	for(uint n = 0; n < Flags.GetCardinality(); n++)
	{
		if(Flags[n] == flag)
			return;
	}

	Flags.Insert(g_pStatic->AddString(flag.cstr()));
	if(MatSource)
		MatSource->SetChanged();
}

//-----------------------------------------------------------------------
void MaterialSource::MatSrcVarFlags::ClearFlag(CStr flag)
{
	for(uint n = 0; n < Flags.GetCardinality(); n++)
	{
		if(Flags[n] == flag)
		{
			Flags.RemoveElement(n);
			if(MatSource)
				MatSource->SetChanged();
			return;
		}
	}
}

//-----------------------------------------------------------------------
void MaterialSource::MatSrcVarFlags::ClearFlags()
{
	if(!Flags.IsEmpty())
	{
		Flags.Clear();
		if(MatSource)
			MatSource->SetChanged();
	}
}

//-----------------------------------------------------------------------
bool MaterialSource::MatSrcVarFlags::IsFlagSet(CStr flag)
{
	for(uint n = 0; n < Flags.GetCardinality(); n++)
	{
		if(Flags[n] == flag)
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------
uint MaterialSource::GetVariableIndex(CStr name) const
{
	for(uint n = 0; n < m_Variables.GetCardinality(); n++)
	{
		if(m_Variables[n]->Name == name)
			return n;
	}

	return INDEX_NOT_FOUND;
}

//-----------------------------------------------------------------------
MaterialSource::MatSrcVar* MaterialSource::FindVariable(CStr name)
{
	uint index = GetVariableIndex(name);

	if(index != INDEX_NOT_FOUND)
	{
		return m_Variables[index];
	}
	return NULL;
}

//-----------------------------------------------------------------------
void MaterialSource::DeleteVariable(CStr name)
{
	uint index = GetVariableIndex(name);

	if(index != INDEX_NOT_FOUND)
	{
		delete m_Variables[index];
		m_Variables.RemoveElement(index);
	}
	SetChanged();
}

//-----------------------------------------------------------------------
void MaterialSource::FreeVariables()
{
	for(uint n = 0; n < m_Variables.GetCardinality(); n++)
	{
		MatSrcVar* var = m_Variables[n];
		delete var;
	}

	m_Variables.Clear();
	SetChanged();
}

//-----------------------------------------------------------------------
MaterialSource::~MaterialSource()
{
	FreeVariables();
}

//-----------------------------------------------------------------------
void MaterialSource::UpdateMaterial()
{
	if(m_pMaterial)
	{
		m_pMaterial->Update(this);
	}
}

//-----------------------------------------------------------------------
MaterialSource::MatSrcVar& MaterialSource::MatSrcVar::operator=(const MaterialSource::MatSrcVar& v)
{
	Name = v.Name;
	Type = v.Type;
	Value = v.Value;
	VarRef = v.VarRef;
	Enum = v.Enum;
	return *this;
}

//-----------------------------------------------------------------------
MaterialSource::MatSrcVarFlags& MaterialSource::MatSrcVarFlags::operator=(const MaterialSource::MatSrcVarFlags& v)
{
	MatSrcVar::operator=(v);

	Flags.Clear();
	for(uint n = 0; n < v.Flags.GetCardinality(); n++)
	{
		Flags.Insert(v.Flags[n]);
	}
	return *this;
}

//-----------------------------------------------------------------------
MaterialSource::MatSrcVarTexture& MaterialSource::MatSrcVarTexture::operator=(const MaterialSource::MatSrcVarTexture& v)
{
	MatSrcVarFlags::operator=(v);

	FPS = v.FPS;

	ClearTextures();
	Textures.Insert(v.Textures);

	return *this;
}

//-----------------------------------------------------------------------
void MaterialSource::MatSrcVarTexture::AddTexture(CStr name)
{
	Textures.Insert(g_pStatic->AddString(name.cstr()));
	if(MatSource)
		MatSource->SetChanged();
}

//-----------------------------------------------------------------------
void MaterialSource::MatSrcVarTexture::ClearTextures()
{
	if(!Textures.IsEmpty())
	{
		Textures.Clear();
		if(MatSource)
			MatSource->SetChanged();
	}
}

//-----------------------------------------------------------------------
MaterialSource::DeformFunc& MaterialSource::DeformFunc::operator=(const MaterialSource::DeformFunc& f)
{
	m_Type = f.m_Type;

	uint count = f.m_Variables.GetCardinality();

	m_Variables.Clear();
	for(uint v = 0; v < count; v++)
	{
		m_Variables.Insert(f.m_Variables[v]);
	}
	return *this;
}

//-----------------------------------------------------------------------
MaterialSource& MaterialSource::CopyDefinitions(const MaterialSource& mc)
{
	m_iFile = mc.m_iFile;

	for(uint f = 0; f < mc.m_DeformFuncs.GetCardinality(); f++)
	{
		DeformFunc df = mc.m_DeformFuncs[f];
		m_DeformFuncs.Insert(df);
	}

	FreeVariables();

	for(uint n = 0; n < mc.m_Variables.GetCardinality(); n++)
	{
		const MatSrcVar* sv = mc.m_Variables[n];
		MatSrcVar* dv;
		
		switch(sv->Type)
		{
		case DVT_TEXTURE:
			dv = ENF_NEW MatSrcVarTexture(sv->Name);
			*(MatSrcVarTexture*)dv = *(MatSrcVarTexture*)sv;
			break;

		case DVT_FLAGS:
			dv = ENF_NEW MatSrcVarFlags(sv->Name);
			*(MatSrcVarFlags*)dv = *(MatSrcVarFlags*)sv;
			break;

		default:
			dv = ENF_NEW MatSrcVar(sv->Type, sv->Name);
			*dv = *sv;
			break;
		}

		m_Variables.Insert(dv);
	}
	SetChanged();
	return *this;
}

//-----------------------------------------------------------------------
MaterialSource::MaterialSource()
{
	m_pCollection = NULL;
	m_pMaterial = NULL;
	m_pMClass = NULL;
	m_iFile = INDEX_NOT_FOUND;
	m_bChanged = false;
}

//-----------------------------------------------------------------------
void MaterialSource::Init(CStr name, Material* mat, MaterialClass* mc)
{
	FreeVariables();
	m_strName = name.cstr();
	m_pMaterial = mat;
	m_pMClass = mc;
	m_bChanged = true;
}

//-----------------------------------------------------------------------
MaterialSource::MaterialSource(CStr name, Material* mat, MaterialClass* mc):
	m_pCollection(NULL),
	m_strName(name.cstr()),
	m_pMaterial(mat),
	m_pMClass(mc),
	m_iFile(INDEX_NOT_FOUND),
	m_bChanged(false)
{
}

//-----------------------------------------------------------------------
MaterialSource::MatSrcVar* MaterialSource::GetVariable(const MaterialVarDef& def)
{
	return FindVariable(def.m_strName);
}

//-----------------------------------------------------------------------
void MaterialSource::SetName(CStr name)
{
	if(m_pCollection && m_pCollection->m_MaterialsHash.Find(name.cstr()))
	{
		m_pCollection->m_MaterialsHash.Remove(name.cstr());
		m_strName = name;
		m_pCollection->m_MaterialsHash.Insert(m_strName.cstr(), this);
	}
	else
	{
		m_strName = name;
	}
	SetChanged();
}

//-----------------------------------------------------------------------
MaterialSource::MatSrcVar* MaterialSource::AddVariable(const MaterialVarDef& desc)
{
	MatSrcVar* pvar = NULL;
	const char* name = g_pStatic->AddString(desc.m_strName.cstr());

	switch(desc.m_Type)
	{
	case DVT_TEXTURE:
		pvar = ENF_NEW MatSrcVarTexture(name);
		break;

	case DVT_FLAGS:
		pvar = ENF_NEW MatSrcVarFlags(name);
		break;

	default:
		pvar = ENF_NEW MatSrcVar(desc.m_Type, name);
	}

	pvar->MatSource = this;
	m_Variables.Insert(pvar);
	SetChanged();
	return pvar;
}

//-----------------------------------------------------------------------
MaterialSource::MatSrcVar* MaterialSource::SetVariable(CStr name, CStr value)
{
	//I cannot set variable without definitions in MaterialClass!
	if(m_pMClass == NULL || m_pMaterial == NULL)
		return NULL;

	const MaterialVarDef* def = m_pMClass->GetVariableDef(name);
	if(def == NULL)
		return NULL;

	return SetVariable(*def, value);
}

//-----------------------------------------------------------------------
MaterialSource::MatSrcVar* MaterialSource::SetVariable(const MaterialVarDef& desc, CStr value)
{
	uint index = GetVariableIndex(desc.m_strName);
	MatSrcVar* pvar;
	
	if(index == INDEX_NOT_FOUND)
	{
		pvar = AddVariable(desc);
	}
	else
	{
		pvar = m_Variables[index];
	}

	pvar->VarRef.Clear();

	if(!value.IsEmpty())
	{
		pvar->Value = value.cstr();
	}

	pvar->Enum = desc.FindEnum(value);

	SetChanged();
	return pvar;
}

//-----------------------------------------------------------------------
MaterialSource::MatSrcVar* MaterialSource::SetVariable(const MaterialVarDef& desc, const VariableRef& ref)
{
	uint index = GetVariableIndex(desc.m_strName);
	MatSrcVar* pvar;

	if(index == INDEX_NOT_FOUND)
	{
		pvar = AddVariable(desc);
	}
	else
	{
		pvar = m_Variables[index];
	}

	pvar->VarRef = ref;
	SetChanged();
	return pvar;
}

//-----------------------------------------------------------------------
MaterialSource::MatSrcVarTexture* MaterialSource::SetVariableTexture(const MaterialVarDef& desc)
{
	enf_assert(desc.m_Type == DVT_TEXTURE);
	uint index = GetVariableIndex(desc.m_strName);
	MatSrcVar* pvar;

	if(index == INDEX_NOT_FOUND)
	{
		pvar = AddVariable(desc);
	}
	else
	{
		pvar = m_Variables[index];
	}

	SetChanged();
	return (MatSrcVarTexture*)pvar;
}

//-----------------------------------------------------------------------
MaterialSource::MatSrcVarTexture* MaterialSource::SetVariableTexture(CStr name, CStr value)
{
	//I cannot set variable without definitions in MaterialClass!
	if(m_pMClass == NULL || m_pMaterial == NULL)
		return NULL;

	const MaterialVarDef* def;
	if((def = m_pMClass->GetVariableDef(name)) == NULL)
		return NULL;

	MatSrcVarTexture* tex = SetVariableTexture(*def);
	if(tex && !value.IsEmpty())
		tex->AddTexture(value);
	return tex;
}

//-----------------------------------------------------------------------
MaterialSource::MatSrcVarFlags* MaterialSource::SetVariableFlags(const MaterialVarDef& desc)
{
	enf_assert(desc.m_Type == DVT_FLAGS || desc.m_Type == DVT_TEXTURE);

	uint index = GetVariableIndex(desc.m_strName);
	MatSrcVar* pvar;

	if(index == INDEX_NOT_FOUND)
	{
		pvar = AddVariable(desc);
	}
	else
	{
		pvar = m_Variables[index];
	}

	pvar->VarRef.Clear();

	SetChanged();
	return (MatSrcVarFlags*)pvar;
}

//-----------------------------------------------------------------------
MaterialSourceCollection::MaterialSourceCollection(MaterialSystem* ms)
{
	m_pMaterialSystem = ms;
}

//-----------------------------------------------------------------------
MaterialSourceCollection::~MaterialSourceCollection()
{
	for(uint n = 0; n < m_Materials.GetCardinality(); n++)
	{
		delete m_Materials[n];
	}
}

//-----------------------------------------------------------------------
bool ParseVariableRef(ScriptContext* script, CParser& tp, MaterialSource::VariableRef& ref)
{
	Variable *var = NULL;
	bool hasbracket = false;

	if(TTDefEqual(tp.CToken, CParser::TLBracket))
	{
		hasbracket = true;
		if(!tp.NextToken(true))
		{
         tp.LError(CParser::ParmParseError);
			return false;
		}
	}

	Class *cl = NULL;

	if(tp.CToken->Type != TT_STRING)
	{
		tp.LError(CParser::ParmParseError);
		return false;
	}

#ifdef ENF_DONE
	cl = script->FindClass(tp.CToken->Text);
#endif

	if(cl != NULL)
	{
		if(!tp.NextToken(true))
		{
         tp.LError(CParser::ParmParseError);
			return false;
		}
		
		if(tp.CToken->Type != TT_STRING)
		{
         tp.LError(CParser::ParmParseError);
			return false;
		}

#ifdef ENF_DONE
		var = cl->GetVariable(tp.CToken->Text);
#endif
	}
#ifdef ENF_DONE
	else
		var = script->GetVariable(tp.CToken->Text);
#endif

	if(var == NULL)
	{
		enf::LogF(LL_ERROR, "No variable or constant '%s'", tp.CToken->Text);
		tp.NextToken(true);
	}

	if(hasbracket)
	{
		if(!tp.NextToken(true))
		{
			return false;
		}
	
		if(!tp.AssertToken(tp.TRBracket))
		{
			return false;
		}
	}
	else
	{
		tp.NextToken(false);
	}

	ref.VariablePtr = var;
	ref.ClassPtr = cl;

	return true;
}

//-----------------------------------------------------------------------
bool _CheckCompatibility(DataVarType vtype, const Variable* pvar)
{
	if(pvar == NULL)
		return true;

	switch(vtype)
	{
	case DVT_SCALAR:
		if(pvar->GetVType() != VARTYPE_FLOAT)
			return false;
		break;

	case DVT_VECTOR:
		//allow just vectors, or pointers to >=3 array of floats
		if(pvar->GetVType() != VARTYPE_VECTOR)
		{
			if(pvar->GetVType() != VARTYPE_FLOAT || !pvar->IsPtr() || pvar->ArraySize() < 3)
				return false;
		}
		break;

	case DVT_MATRIX:
		if(pvar->GetVType() != VARTYPE_VECTOR || !pvar->IsPtr())
			return false;
		break;

	case DVT_COLOR:
		//allow just vectors, or pointers to >=4 array of floats, or integer
		if(pvar->GetVType() != VARTYPE_VECTOR)
		{
			if(pvar->GetVType() != VARTYPE_FLOAT || !pvar->IsPtr() || pvar->ArraySize() < 4)
			{
				if(pvar->GetVType() != VARTYPE_INT || pvar->IsPtr())
					return false;
			}
		}
		break;

	case DVT_INTEGER:
		if(pvar->GetVType() != VARTYPE_INT)
			return false;
		break;

	case DVT_TEXTURE:
		//textures cannot be provided by script
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------
MaterialSource::MatSrcVarTexture* ParseTextureVariable(ScriptContext* script, CParser& tp, MaterialSource* mat, const MaterialVarDef& vardef)
{
	SArray<const char*, 128> texturenames;
	SArray<const char*, 128> flags;

	uint fps = 0;
	MaterialSource::MatSrcVarTexture* tex;
	MaterialSource::VariableRef ref;
	bool specialtex = false;

	while(!tp.EndOfLine())
	{
		if(tp.CToken->Type == TT_QUOTE)
      {
			texturenames.Insert(tp.CToken->Text);
		}
		else
		if(tp.CToken->Text[0] == '$')
		{
			CStr parm = tp.CToken->Text;

			if(!vardef.FindEnum(parm))
			{
				tp.LError("Unknown special texture name %s", parm.cstr());
				return NULL;
			}
			//these must be always alone!
			texturenames.Clear();
			texturenames.Insert(tp.CToken->Text);
			specialtex = true;
		}
		else
			break;

		tp.NextToken(false);
		
		//skip comma if any
		if(!tp.EndOfLine() && TTDefEqual(tp.CToken, CParser::TComma))
		{
			if(!tp.NextToken(true))
			{
				return NULL;
			}
		}
	}

	while(!tp.EndOfLine())
	{
		if(!tp.AssertType(TT_STRING))
		{
			return NULL;
		}

		flags.Insert(g_pStatic->AddString(tp.CToken->Text));

		//anim keyword
		if(TTEqual(*tp.CToken, "anim"))
		{
			if(specialtex)
			{
				tp.LError("Can't animate special texture '%s'", texturenames[0]);
				return NULL;
			}

			if(!tp.NextToken(true))
			{
				return NULL;
			}

			if(!tp.AssertToken(CParser::TLBracket))
			{
				return NULL;
			}

			if(tp.CToken->Type != TT_NUMBER)
			{
				if(!ParseVariableRef(script, tp, ref))
				{
					return NULL;
				}
				fps = 0;
			}
			else
			{
				fps = atof(tp.CToken->Text);

				if(!tp.NextToken(true))
					return NULL;
			}

			if(!tp.AssertToken(CParser::TRBracket))
			{
				return NULL;
			}
		   continue;
		}

		tp.NextToken();
	}

	if(!texturenames.IsEmpty())
	{
		tex = mat->SetVariableTexture(vardef);
		if(tex == NULL)
			return NULL;

		tex->ClearTextures();
		tex->ClearFlags();

		for(uint n = 0; n < texturenames.GetCardinality(); n++)
			tex->AddTexture(texturenames[n]);

		for(uint n = 0; n < flags.GetCardinality(); n++)
			tex->SetFlag(flags[n]);

		tex->FPS = fps;
		tex->VarRef = ref;
		return tex;
	}

	return NULL;
}

//-----------------------------------------------------------------------
uint MaterialSourceCollection::FindBestFile(CStr name)
{
	SString<> temp;
	temp = name.cstr();
	temp.lower();

	for(uint f = 0; f < m_Files.GetCardinality(); f++)
	{
		SString<> filename (m_Files[f].m_strName.c_str());
		filename.StripExt();
		int slash = filename.FindR('/');
		if(slash >= 0)
			filename = filename.cstr() + slash + 1;

		if(CStr(temp.cstr()).strstr(filename.cstr()))
			return f;
	}

	return 0;
}

//-----------------------------------------------------------------------
void MaterialSourceCollection::AddMaterial(MaterialSource* material, int index, bool markFileAsChanged)
{
	m_Materials.Insert(material);
	m_MaterialsHash.Insert(material->m_strName.cstr(), material);
	material->m_pCollection = this;
	
	if(index >= 0 && index < m_Files.GetCardinality())
		material->m_iFile = index;
	else if(material->m_iFile == INDEX_NOT_FOUND)
		material->m_iFile = FindBestFile(material->m_strName.cstr());

	if(markFileAsChanged)
	{
		if(material->m_iFile < m_Files.GetCardinality())
			m_Files[material->m_iFile].m_bChanged = true;
	}
}

//-----------------------------------------------------------------------
void MaterialSourceCollection::RemoveMaterial(MaterialSource* material)
{
	//force the file to be resaved
	if(material->m_iFile < m_Files.GetCardinality())
		m_Files[material->m_iFile].m_bChanged = true;

	m_Materials.Remove(material);
	m_MaterialsHash.Remove(material->m_strName.cstr());
}

//-----------------------------------------------------------------------
void MaterialSourceCollection::DestroyMaterial(MaterialSource* material)
{
	RemoveMaterial(material);
	delete material;
}

//-----------------------------------------------------------------------
void StoreMatSrcVar(COutputStream& ostr, const MaterialSource::MatSrcVar* var)
{
	if(var->Type == DVT_FLAGS)
	{
		MaterialSource::MatSrcVarFlags* flags = (MaterialSource::MatSrcVarFlags*)var;
		if(flags->Flags.IsEmpty())
			return;
	}

	ostr << '\t';

	if(var->Name == "physmaterial")
		ostr << "surfparm ";
	else
	{
		ostr << var->Name;
		ostr << ' ';
	}

	if(var->Type != DVT_TEXTURE && var->IsVariableReference())
	{
		ostr << '(';
		if(var->VarRef.ClassPtr)
		{
			ostr << var->VarRef.ClassPtr->GetName();
			ostr << ' ';
		}

		ostr << var->VarRef.VariablePtr->GetName();
		ostr << ')';
	}
	else
	{
		switch(var->Type)
		{
		case DVT_INTEGER:
		case DVT_SCALAR:
		case DVT_BOOLEAN:
			ostr << var->Value;
			break;

		case DVT_STRING:
		case DVT_VECTOR:
		case DVT_MATRIX:
		case DVT_COLOR:
			ostr << '\"';
			ostr << var->Value;
			ostr << '\"';
			break;

		case DVT_FLAGS:
			MaterialSource::MatSrcVarFlags* flags;
			flags = (MaterialSource::MatSrcVarFlags*)var;

			if(var->Name == "surfparm")
			{
				for(uint f = 0; f < flags->Flags.GetCardinality(); f++)
				{
					ostr << flags->Flags[f];

					if(f < (flags->Flags.GetCardinality() - 1))
						ostr << "\n\tsurfparm ";
				}
			}
			else
			{
				if(var->Name == "passmask" &&
					flags->Flags.GetCardinality() >= 4 &&
					flags->IsFlagSet("noambient") &&
					flags->IsFlagSet("nospecular") &&
					flags->IsFlagSet("nodiffuse") &&
					flags->IsFlagSet("noenvironment"))
				{
					ostr << "nolight";
				}
				else
				{
					for(uint f = 0; f < flags->Flags.GetCardinality(); f++)
					{
						ostr << flags->Flags[f];
						if(f < (flags->Flags.GetCardinality() - 1))
							ostr << ' ';
					}
				}
			}
			break;

		case DVT_TEXTURE:
			MaterialSource::MatSrcVarTexture* tex;
			tex = (MaterialSource::MatSrcVarTexture*)var;

			for(uint t = 0; t < tex->Textures.GetCardinality(); t++)
			{
				if(tex->Textures[t][0] == '$')
				{
					ostr << tex->Textures[t];
				}
				else
				{
					ostr << '\"';
					ostr << tex->Textures[t];
					ostr << '\"';
				}
				if(t < (tex->Textures.GetCardinality() - 1))
					ostr << ',';
			}

			if(!tex->Flags.IsEmpty())
			{
				for(uint f = 0; f < tex->Flags.GetCardinality(); f++)
				{
					ostr << ' ';
					if(tex->Flags[f] == "anim")
					{
						ostr << "anim(";
						if(tex->VarRef.IsSet())
						{
							ostr << tex->VarRef.VariablePtr->GetName();
						}
						else
						{
							SString<> str;
							ostr << str.format("%.f", tex->FPS);
						}
						ostr << ')';
					}
					else
					{
						ostr << tex->Flags[f];
					}
				}
			}
			break;
		}
	}
	ostr << '\n';
}

//-----------------------------------------------------------------------
bool MaterialSourceCollection::SaveMaterialsToScript(uint flags, char* errors, size_t maxsize)
{
	VFile* files[128];

	SString<256> lerror;
	size_t pos = 0;
	bool res = true;

	if(errors == NULL)
		maxsize = 0;

	ZeroMem(files, sizeof(files));

	Set<const MaterialSource::MatSrcVar*> storedvars;

	Set<Pair<CStr, MaterialSource*> > sortedmaterials;

	//mark changed files by changed materials
	for(uint m = 0; m < m_Materials.GetCardinality(); m++)
	{
		m_Files[m_Materials[m]->m_iFile].m_bChanged |= m_Materials[m]->IsChanged();
		m_Materials[m]->ClearChanged();
	}

	for(uint m = 0; m < m_Materials.GetCardinality(); m++)
	{
		sortedmaterials.Insert(MakePair(m_Materials[m]->GetName(), m_Materials[m]));
	}

	for(uint m = 0; m < sortedmaterials.GetCardinality(); m++)
	{
		MaterialSource* ms = sortedmaterials[m].Data;
		MaterialClass* mc = ms->GetMaterialClass();

		if(flags & MSF_ONLYCHANGED && !m_Files[ms->m_iFile].m_bChanged)
		{
			continue;
		}

		VFile* vf = files[ms->m_iFile];

		//do we have opened file?
		if(vf == NULL)
		{
			vf = GetEngine()->GetFileSystem()->Open(m_Files[ms->m_iFile].m_strName, FILEMODE_WRITE);

			if(vf == NULL)
			{
				lerror.format("File '%s' cannot be opened.\n", m_Files[ms->m_iFile].m_strName.cstr());
				size_t len = lerror.Length();
				if((len + pos + 1) < maxsize)
				{
					memcpy(errors + pos, lerror.cstr(), len);
					pos += len;
					errors[pos] = 0;
				}

				res = false;
				vf = (VFile*)0xffffffff;
			}

			files[ms->m_iFile] = vf;
		}

		if(vf == (VFile*)0xffffffff)
		{
			//couldn't open this file
			continue;
		}

		CFileOutputStream ostr(vf, true);

		storedvars.Clear();

		ostr << "material \"";
		//output header
		if(mc != NULL)
		{
			ostr << ms->m_strName;
			ostr << "\": ";
			ostr << mc->GetName();
		}
		else
		{
			ostr << ms->m_strName;
			ostr << "\": Null";
		}

		ostr << "\n{\n";

		const MaterialVarDef* def;
		const MaterialSource::MatSrcVar* var;

		if(!ms->m_strDescription.IsEmpty())
		{
			ostr << "\tdescription \"";
			ostr << ms->m_strDescription;
			ostr << "\"\n";
		}

		//first output common variables
		for(uint n = 0; n < m_pMaterialSystem->GetNumVariableDefs(); n++)
		{
			def = m_pMaterialSystem->GetVariableDef(n);

			var = ms->GetVariable(*def);
			if(var == NULL)
				continue;

			StoreMatSrcVar(ostr, var);

			storedvars.Insert(var);
		}

		if(mc)
		{
			//store material class variables
			for(uint n = 0; n < mc->GetNumVariables(); n++)
			{
				def = mc->GetVariableDef(n);

				var = ms->GetVariable(*def);
				if(var == NULL)
					continue;

				StoreMatSrcVar(ostr, var);

				storedvars.Insert(var);
			}
		}

		//store the rest
		for(uint n = 0; n < ms->GetNumVariables(); n++)
		{
			var = ms->GetVariable(n);

			if(storedvars.Contains(var))
				continue;

			StoreMatSrcVar(ostr, var);
		}

		//deform functions
		for(uint n = 0; n < ms->m_DeformFuncs.GetCardinality(); n++)
		{
			MaterialSource::DeformFunc& func = ms->m_DeformFuncs[n];

			const ParmEnumMaterialDeform** deff = m_pMaterialSystem->GetDeformFuncs();

			while(*deff)
			{
				if((*deff)->Type == func.m_Type)
					break;
				deff++;
			}
			
			if(*deff)
			{
				ostr << '\t';
				ostr << (*deff)->Name;
				ostr << ' ';
				for(uint v = 0; v < func.m_Variables.GetCardinality(); v++)
				{
					MaterialSource::MatSrcVar& var = func.m_Variables[v];

					if(var.IsVariableReference())
					{
						ostr << '(';
						if(var.VarRef.ClassPtr)
						{
							ostr << var.VarRef.ClassPtr->GetName();
							ostr << ' ';
						}
						ostr << var.VarRef.VariablePtr->GetName();
						ostr << ')';
					}
					else
					{
						switch(var.Type)
						{
						case DVT_INTEGER:
						case DVT_SCALAR:
						case DVT_STRING:
						case DVT_BOOLEAN:
							ostr << var.Value;
							break;

						case DVT_VECTOR:
						case DVT_MATRIX:
						case DVT_COLOR:
							ostr << '\"';
							ostr << var.Value;
							ostr << '\"';
							break;
						}
					}
					//if((n + 1) < func.m_Variables.GetCardinality())
						ostr << ' ';
				}
				ostr << "\n";
			}
		}

		ostr << "}\n\n";
	}

	for(uint f = 0; f < m_Files.GetCardinality(); f++)
	{
		if(files[f] != NULL && files[f] != (VFile*)0xffffffff)
			files[f]->Close();
		m_Files[f].m_bChanged = false;
	}

	return res;
}

//-----------------------------------------------------------------------
bool MaterialSourceCollection::LoadMaterialsFromScript(CStr name, bool keepsource)
{
	enf_assert(!name.IsNull());

	CParser tp;
	MaterialSource* material;
	Material* pmaterial;
	bool res = true;
	int ivalue;
	bool isOk = false;
	const MaterialVarDef* physmatdef;

	if(tp.Parse(name) != PE_OK)
   	return false;

   material = NULL;
	uint fileindex = INDEX_NOT_FOUND;

	physmatdef = m_pMaterialSystem->GetVariableDef("physmaterial");

	TModule* lastmodule = NULL;
	for(uint l = 0; l < tp.GetNumLines(); l++)
   {
		tp.BeginLine(l);

		if(TTDefEqual(tp.CToken, CParser::TLCurly))
		{
      	continue;
		}
//closing '}'
		CStr cmd, parm;
		
		cmd = tp.CToken[0].Text;

		if(tp.CLine->m_iNumTokens > 1)
			parm = tp.CToken[1].Text;
		else
			parm = NULL;

		if(TTDefEqual(&tp.CToken[0], CParser::TRCurly))
      {
         if(!material)
         {
				tp.LError(CParser::SequenceError);
				res=false;
				break;
         }

			material->UpdateMaterial();
			material->ClearChanged();

			if(keepsource)
				AddMaterial(material);
			else
				delete material;

			pmaterial = NULL;
			material = NULL;
			continue;
		}

		if(material && !isOk)
		{
			//silently ignore ill materials
			continue;
		}

//material definition
		if(cmd == "material")
      {
			if(material)
			{
				tp.LError(CParser::SequenceError);
				res = false;
				break;
			}

			if(!tp.NextToken(true) || !tp.AssertType(TT_QUOTE))
         {
            res = false;
            break;
         }

			CStr matname = tp.CToken->Text;
			
			if((!keepsource && m_pMaterialSystem->MaterialExists(matname)) || FindMaterial(matname))
			{
            tp.LError("Material '%s' already exists!", matname.cstr());
				res = false;
				break;
			}
			
			pmaterial = m_pMaterialSystem->CreateMaterial(matname);
			isOk = true;

			if(tp.NextToken())
			{
				if(TTDefEqual(tp.CToken, CParser::TDoubleDot))
				{
					if(!tp.NextToken(true))
					{
						res = false;
						break;
					}

					if(CStr(tp.CToken->Text).cmpi("Null") != 0)
					{
						CStr ename = NULL;

						MaterialClass* mclass = m_pMaterialSystem->FindMaterialClass(tp.CToken->Text);

						if(mclass == NULL)
						{
							tp.LError("Unknown material class %s", tp.CToken->Text);
							isOk = false;
							//res = false;
							//continue;
						}

						if(tp.NextToken())
						{
							if(!tp.AssertToken(CParser::TLBracket))
							{
								res = false;
								break;
							}

							ename = tp.CToken->Text;

							if(!tp.NextToken(true) || !tp.AssertToken(CParser::TRBracket))
							{
								res = false;
								break;
							}
						}

						if(mclass)
						{
							if(!pmaterial->SetMaterialClass(mclass))
							{
								tp.LError("Can't set material class %s", mclass->GetName().cstr());
								res = false;
								continue;
							}
						}
					}
				}
			}

			material = ENF_NEW MaterialSource(matname, pmaterial, pmaterial->GetMaterialClass());

			if(tp.GetModule(tp.CLine->m_iModule) != lastmodule)
			{
				lastmodule = tp.GetModule(tp.CLine->m_iModule);
				uint n;
				for(n = 0; n < m_Files.GetCardinality(); n++)
				{
					if(m_Files[n].m_strName == lastmodule->GetName())
						break;
				}

				if(n == m_Files.GetCardinality())
				{
					m_Files.Insert(File(lastmodule->GetName()));
				}
				fileindex = n;
			}
			material->m_iFile = fileindex;
			continue;
      }

		if(material == NULL)
		{
			tp.LError(CParser::SequenceError);
			res = false;
			continue;
		}

		if(cmd == "description")
		{
			material->SetDescription(parm);
			continue;
		}

//coordinate modifying function
		if(cmd == "vcmod" || cmd == "tcmod")
      {
			const ParmEnumMaterialDeform** pdef = m_pMaterialSystem->GetDeformFuncs();
			const ParmEnumMaterialDeform* def = NULL;

			while(*pdef)
			{
				def = *pdef;

				if(def->Name.ncmp(cmd, 5) == 0)
				{
					
					if(parm.cmpi(def->Name.cstr() + 6) == 0)
						break;
				}
				pdef++;
			}

			//TODO: error? or just skip not-implemented function?
			if(*pdef == NULL)
				continue;

			ivalue = def->Type;

			MaterialSource::DeformFunc deff;

			deff.m_Type = (Material::CFuncType)ivalue;
			tp.CurrentToken(2);

			while(!tp.EndOfLine())
			{
				if(deff.m_Variables.IsFull())
				{
					tp.LError("Too much values for vcmod/tcmod");	
					res = false;
					break;
				}

				MaterialSource::MatSrcVar& var = deff.m_Variables.Append();
				var.Type = def->Parms[deff.m_Variables.GetCardinality() - 1];
				if(tp.CToken->Type == TT_NUMBER)
				{
					var.Value = tp.CToken->Text;
					tp.NextToken();
				}
				else
				{
					if(!ParseVariableRef(g_pScript, tp, var.VarRef))
					{
						res = false;
						break;
					}
				}
			}
			material->m_DeformFuncs.Insert(deff);
         continue;
      }


		const MaterialVarDef* vardef;

		if(((material->m_pMClass != NULL && (vardef = material->m_pMClass->GetVariableDef(cmd)) != NULL)) ||
			(vardef = m_pMaterialSystem->GetVariableDef(cmd)) != NULL)
		{
			if(!tp.NextToken(true))
			{
				res = false;
				break;
			}

			MaterialSource::MatSrcVar* pvar = NULL;

			//if(cmd == "matrix0")
			//	cmd=cmd;
			//variable reference
			if(TTDefEqual(tp.CToken, CParser::TLBracket))
			{
				MaterialSource::VariableRef ref;

				//Some basic validation between variable type and parameter type
				if(!ParseVariableRef(g_pScript, tp, ref) || !_CheckCompatibility(vardef->m_Type, ref.VariablePtr))
				{
					if(ref.VariablePtr == NULL)
						tp.LError(IncParmError, parm.cstr());
					else
						tp.LError(IncParmError, ref.VariablePtr->GetName());
					res = false;
					continue;
				}
				pvar = material->SetVariable(*vardef, ref);
			}
			else
			{
				CStr value = tp.CToken->Text;
			
				switch(vardef->m_Type)
				{
				case DVT_SCALAR:
				case DVT_VECTOR:
				case DVT_MATRIX:
				case DVT_INTEGER:
				case DVT_STRING:
					pvar = material->SetVariable(*vardef, value);
					break;

				case DVT_FLAGS:
					if(vardef->m_strName == "surfparm")
					{
						SString<> varname;

						varname = "MATERIAL_";
						varname += parm.cstr();
						varname.upper();

#ifdef ENF_DONE
						Variable* var = g_pScript->GetVariable(varname.cstr());
#else
						Variable* var = NULL;
#endif
						if(var == NULL)
						{
							pvar = material->SetVariableFlags(*vardef);
							if(!vardef->FindEnum(parm))
							{
								tp.LError("Unknown flag switch '%s'", parm.cstr());
								res = false;
							}
							else
								((MaterialSource::MatSrcVarFlags*)pvar)->SetFlag(parm);
						}
						else
						{
							pvar = material->SetVariable(*physmatdef, parm);
						}
						break;
					}

					if(vardef->m_strName == "passmask")
					{
						uint passmask = 0xffffffff;

						while(true)
						{
							parm = tp.CToken->Text;
							int mask = 0;

							if(vardef->FindEnumInteger(parm, mask))
							{
								passmask &= ~mask;
							}
							else
							{
								tp.LError("Unknown passmask keyword '%s'", parm.cstr());
								res = false;
							}

							if(!tp.NextToken())
								break;
						}

						pvar = material->SetVariableFlags(*vardef);

						const ParmEnumInteger** pi = (const ParmEnumInteger**)vardef->m_pEnums;
						
						while(pi && *pi)
						{
							if((passmask & (*pi)->Value) == 0)
								((MaterialSource::MatSrcVarFlags*)pvar)->SetFlag((*pi)->Name);
							pi++;
						}
						break;
					}
					break;

				case DVT_BOOLEAN:
					if(value != "true" && value != "false")
					{
						tp.LError("Bad boolean value '%s'. Only true/false is allowed!", value.cstr());
						res = false;
					}
					pvar = material->SetVariable(*vardef, value);
					break;

				case DVT_COLOR:
					if(tp.CToken->Type == TT_HEXA)
					{
						SString<64> buff;

						//convert to integer and then to FColor
						int color;
						if(sscanf(value.cstr(), "%x", &color) == 1)
						{
							buff.format("%.4lf %.4lf %.4lf",
								((double)(color & 0xff) * FCOLOR_SCALE),
								((double)((color & 0xff00) >> 8) * FCOLOR_SCALE),
								((double)((color & 0xff0000) >> 16) * FCOLOR_SCALE),
								((double)((color & 0xff000000) >> 24) * FCOLOR_SCALE));
							
							pvar = material->SetVariable(*vardef, buff.cstr());
						}
					}
					else
					{
						pvar = material->SetVariable(*vardef, value);
					}
					break;

				case DVT_TEXTURE:
					pvar = ParseTextureVariable(g_pScript, tp, material, *vardef);
					break;
				
				default:
					enf_assert(!"Not implemented");
					break;
				}
			}

			if(pvar == NULL)
			{
				tp.LError("Can't set '%s' variable", cmd.cstr());
				res = false;
			}
			continue;
		}

		if(cmd.ncmp("emap_", 5) == 0 || cmd.ncmp("qer_", 4) == 0 || cmd == "fogparms")
		{
			//unparse it back to one string
			char temp[256] = "\0";
			
			if(tp.NextToken(false))
				tp.Unparse(tp.CToken, (tp.CLine->m_iNumTokens - tp.CParm), temp, sizeof(temp));

			//we probably don't have definitions for these. Add it as DVT_STRING
			MaterialVarDef def(DVT_STRING, 0, 0, g_pStatic->AddString(cmd.cstr()), "editbox", "", NULL);
			material->SetVariable(def, temp);
			continue;
		}

//or bad command
		tp.LError("Unknown material keyword %s", cmd.cstr());
      continue;
   }

	return res;
}

}