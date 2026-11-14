#include "El/elementpch.hpp"
#include "cutMaterial.hpp"
#include "cutParamFileExt.hpp"
#include "engine.hpp"
#include <El/ParamArchive/paramArchiveDb.hpp>
#include "paramArchiveExt.hpp"
#include <El/Color/colors.hpp>
#include <El/ParamFile/classDbParamFile.hpp>

DEFINE_ENUM(UVSource,UV,UV_ENUM)

DEFINE_ENUM(TexType,TT,TEXTYPE_ENUM)

TexMaterial::TexMaterial()
:_emmisive(HBlack),
_ambient(HWhite),
_diffuse(HWhite),
_forcedDiffuse(HBlack),
_specular(HBlack),
_specularPower(0),
_secTexTransform(MIdentity)
{
}

template <class Type>
void LoadValue
(
	Type &dst, const ParamEntry &cfg, const char *name, const Type &defVal
)
{
	const ParamEntry *entry = cfg.FindEntry(name);
	if (!entry)
	{
		dst = defVal;
	}
	else
	{
		GetValue(dst,*entry);
	}
}

void TexMaterial::Load(const ParamEntry &cfg)
{
	ClassEntry *cls = new ParamClassEntry(const_cast<ParamEntry &>(cfg));
	ParamArchiveLoadEntry ar(cls);
	Serialize(ar);

}

LSError TexMaterial::Serialize(ParamArchive &ar)
{
	CHECK( ::Serialize(ar,"emmisive",_emmisive,0,HBlack) )
	CHECK( ::Serialize(ar,"ambient",_ambient,0,HWhite) )
	CHECK( ::Serialize(ar,"diffuse",_diffuse,0,HWhite) )
	CHECK( ::Serialize(ar,"forcedDiffuse",_forcedDiffuse,0,HBlack) )
	CHECK( ::Serialize(ar,"specular",_specular,0,HBlack) )
	CHECK( ar.Serialize("specularPower",_specularPower,0,0.0f) )

	//CHECK( ar.SerializeEnum("secTexType",_secTexType,0,TTColor) )
	CHECK( ar.SerializeEnum("secTexUVSource",_secTexUVSource,0,UVTex) )
	CHECK( ar.SerializeEnum("pixelShaderID",_pixelShaderID,0,PSNormal) )
	CHECK( ::Serialize(ar,"secTexTransform",_secTexTransform,0,MIdentity) )
	if (ar.IsLoading())
	{
		RString name;
		ar.Serialize("secTex",name,0,"");
		_secTex = GlobLoadTexture(name);
	}
	else
	{
		Fail("TexMaterial save not implemented");
	}
	/*
	// load properties from cfg file
	_specularPower = cfg.ReadValue("specularPower",0.0f);

	/*
	Ref<Texture> _secTex;
	*/

	return LSOK;
}

void TexMaterial::Init()
{
	_emmisive = HBlack;
	_ambient = HWhite;
	_diffuse = HWhite;
	_forcedDiffuse = HBlack;
	_specular = HBlack;
	_specularPower=0;
}


TexMaterial::TexMaterial(const ParamEntry &cfg)
{
	Load(cfg);
}
TexMaterial::TexMaterial(const char *name)
{
	_name = name;
	// load material from given file
	if (*name=='#')
	{
		// if first character is #, load from global material table
		ParamEntry *entry = (Pars>>"CfgMaterials").FindEntry(name+1);
		if (entry)
		{
			Load(*entry);
			return;
		}
		RptF("Material %s not found in CfgMaterials",name);
	}
	else
	{
		ParamFile cfg;
		if (cfg.ParseBinOrTxt(name) && cfg.GetEntryCount()>0)
		{
			Load(cfg);
			return;
		}
		RptF("Material %s load failed",name);
	}
	Init();
}


void TexMaterial::Combine(TLMaterial &dst, const TLMaterial &src)
{
	dst.ambient = src.ambient*_ambient;
	dst.diffuse = src.diffuse*_diffuse;
	dst.forcedDiffuse = src.forcedDiffuse*_forcedDiffuse;
	// note: combination of some properties is addition
	// those properties are zero for default material
	// and therefore it makes no sense multiplying them
	// note: src material has already acoomodation calculated in
	// this does not
	ColorVal accom = GEngine->GetAccomodateEye();
	dst.specular = src.specular+_specular*accom;
	dst.specularPower = (int)(src.specularPower+_specularPower);
	dst.emmisive = src.emmisive+_emmisive*accom;
	dst.specFlags = src.specFlags;
}

TexMaterialBank::TexMaterialBank()
{
}

TexMaterialBank::~TexMaterialBank()
{
}

TexMaterial *TexMaterialBank::TextureToMaterial(Texture *tex)
{
	if (!tex) return NULL;
	// check if given texture already exists
	int index = Find(tex->GetName());
	if (index>=0)
	{
		return Get(index);
	}
	// if it does not exist, try to find definition and load it
	if (!Pars.FindEntry("CfgMaterials"))
	{
		return NULL;
	}
	const ParamEntry &texmat = Pars>>"CfgTextureToMaterial";
	for (int i=0; i<texmat.GetEntryCount(); i++)
	{
		const ParamEntry &entry = texmat.GetEntry(i);
		if (!entry.IsClass()) continue;
		// check texture name
		//const RStringB &texname = entry>>"texture";
		const ParamEntry &texnames = entry>>"textures";
		const RStringB &matname = entry>>"material";

		for (int t=0; t<texnames.GetSize(); t++)
		{
			const RStringB &texname = texnames[t];
			if (!strcmpi(texname,tex->GetName()))
			{
				return New(matname);
			}
		}
	}

	return NULL;
}

//template Link<TexMaterial>;

TexMaterialBank GTexMaterialBank;

