#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MATERIAL_HPP
#define _MATERIAL_HPP

#include "cutTextbank.hpp"
#include "cutTlVertex.hpp"
//#include "engine.hpp"
#include <El/Enum/enumNames.hpp>

//! how UV coordinates should be generated

#define UV_ENUM(type,prefix,XX) \
	XX(type, prefix, Tex) \
	XX(type, prefix, Pos) \
	XX(type, prefix, Norm)

//! source of texture coordinates
DECLARE_ENUM(UVSource,UV,UV_ENUM)

#define TEXTYPE_ENUM(type,prefix,XX) \
	XX(type, prefix, Color) \
	XX(type, prefix, Normal) \
	XX(type, prefix, Specular) \
	XX(type, prefix, Detail) 

//! type of texture
DECLARE_ENUM(TexType,TT,TEXTYPE_ENUM)

//! general surface material properties
/*!
contains texture and other material properties.
Primary use for this structure is in ShapeSection
*/
class ParamEntry;
class ParamArchive;

class TexMaterial: public RefCountWithLinks
{
	public:
	//! material name
	RStringB _name;

	//@{
	/*!
	\name secondary texture information
	*/
	//! secondary texture map
	Ref<Texture> _secTex;
	//! secondary texture coordinate transform
	Matrix4 _secTexTransform;
	//! secondary texture coord source
	UVSource _secTexUVSource;
	//! engine specific pixel shader ID
	//PixelShaderID _pixelShaderID;
	//@}


	//@{
	/*!
	\name material lighting properties
	*/
	//! emmisive color - used for shining polygons
	Color _emmisive;
	//! ambient color factor
	Color _ambient;
	//! diffuse color factor
	Color _diffuse;
	//! forced diffuse color factor - used to simulate half or fully lighted polys
	Color _forcedDiffuse;
	//! specular color factor
	Color _specular;
	// specular power - higher value means sharper highlight
	// value zero means no highlight
	float _specularPower;
	//@}

	//! create default material
	void Init();
	//! constructor
	TexMaterial();
	//! create material from CfgMaterials
	explicit TexMaterial(const char *name);
	//! create material from a config entry
	explicit TexMaterial(const ParamEntry &cfg);
	//! load material from a config entry
	void Load(const ParamEntry &cfg);
	//! combine this material with TLMaterial
	void Combine(TLMaterial &dst, const TLMaterial &src);
	//! get name (necessary for BankArray)
	const RStringB &GetName() const {return _name;}

	protected:
	//! load material from a config entry
	LSError Serialize(ParamArchive &ar);
};

#include <Es/Containers/bankArray.hpp>

// material bank

template <>
struct BankTraits<TexMaterial>
{
	typedef const char *NameType;
	// default name comparison
	static int CompareNames( NameType n1, NameType n2 )
	{
		return strcmpi(n1,n2);
	}
	typedef LinkArray<TexMaterial> ContainerType;
};

//! material bank

/*!
Result of TexMaterialBank::New or TextureToMaterial should be assigned
to Ref<TexMaterialBank>.
*/

class TexMaterialBank: public BankArray<TexMaterial>
{
	public:
	TexMaterialBank();
	~TexMaterialBank();

	//! handling of old-style texture-driven materials
	TexMaterial *TextureToMaterial(Texture *tex);
};

// TLMaterial

extern TexMaterialBank GTexMaterialBank;

#endif
