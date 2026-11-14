//-----------------------------------------------------------------------------
// File: enf_materials.h
//
// Desc: Custom materials definition
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_MATERIALS_H
#define ENF_MATERIALS_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "materials/enf_material.h"
#include "render/enf_rendmain.h"
#include "materials/enf_materialclass.h"
#include "render/enf_supershader.h"
//#include "../shaders/registers.h"
#include "pipeline/enf_ilight.h"
#include "Render/enf_renderresources.h"

namespace enf
{
struct ILight;
const int constantsSize = 65536;

#define SFS_SHADOWMAP_FILTER	SFS_POINT_POINT_NONE

/*!
8bit format of shadow map with packed 16bit values into RG and BA pair of channels
*/
#ifdef ENF_PS3
	#define SHADOW_MAP_8BIT
#endif

/*!
override of sort
*/
const int SORTBIAS_OCCLUDER		= -16;


/*!
rendered in SORT_DECAL group on the end
*/
const int SORTBIAS_SKY_LIGHTNING	= 15;
const int SORTBIAS_WATER_DECAL	= 14;
const int SORTBIAS_WATER			= 13;
const int SORTBIAS_PP_SSAO			= 12;	
const int SORTBIAS_GRASS			= 11;
const int SORTBIAS_SKY_LAYERS		= 10;
const int SORTBIAS_SKY_SUN			=  9;
const int SORTBIAS_SKY_PLANETS	=  8;
const int SORTBIAS_SKY_COLOR		=  7;

/*!
renderer in SORT_OVERLAY pass
*/
const int SORTBIAS_UNDERWATER		= 15;


#define SPECULAR_FROM_NMAPALPHA 1.0f
#define REFLECTION_FROM_NMAPALPHA -1.0f

FColor DecalColor(const class Decal* dec);
float DecalFadeOut(const class Decal* dec);
float PrepareSpecularPower(float power);

struct FogParamsVS
{
	Vector4	m_ColorSun;
	Vector4	m_ColorSunBack;
	Vector4	m_LerpParams;
	Vector4	m_MainDir;
};

struct LightParamsPS
{
	FColor	m_Diffuse;
	FColor	m_Ambient;
	FColor	m_Emissive;
};
struct SpecularParamsPS
{
	FColor	m_Specular;
	FColor	m_SpecularBias;
};

struct AmbLightParamsPS: public LightParamsPS, SpecularParamsPS
{
	Vector4	m_User;
};

//POINT=0,SPOT=1,DIRECT=2
ENF_INLINE uint LightType2Index(LightType ltype)
{
	switch(ltype)
	{
	case LT_POINT:
		return 0;
	case LT_SPOT:
		return 1;
	case LT_DIRECTIONAL:
		return 2;
	}
	return 0;
}

//-----------------------------------------------------------------------------
BEGIN_DEFINE_ABSTRACT_MCLASS(MatGeneric, "Generic", MaterialClass)

	static const uint MAX_INSTANCES = 54;
	struct InstanceInfo
	{
		Matrix34		m_mWorld;
		Vector4		m_vInfo;
	};
	DEFINE_PARAM(MV_ALPHA_TEST);
	DEFINE_PARAM(MV_BLEND_MODE);
	DEFINE_PARAM(MV_FLAGS2);
	DEFINE_PARAM(MV_COLOR);
	DEFINE_PARAM(MV_AMBIENT);
	DEFINE_PARAM(MV_EMISSIVE);
	DEFINE_PARAM(MV_NMAP_ATTENUATION);
	DEFINE_PARAM(MV_SPECULAR_POWER);
	DEFINE_PARAM(MV_SPECULAR_EXP);
	DEFINE_PARAM(MV_SPECULAR);
	DEFINE_PARAM(MV_SPECULAR_TINT);
	DEFINE_PARAM(MV_SPECULAR_COMBINED);
	DEFINE_PARAM(MV_SPECULAR_BIAS);
	DEFINE_PARAM(MV_REFLECTION);
	DEFINE_PARAM(MV_REFLECTION_TINT);
	DEFINE_PARAM(MV_FRESNEL);
	DEFINE_PARAM(MV_PARALLAX_SCALE);
	DEFINE_PARAM(MV_DIFFUSE_MAP);
	DEFINE_PARAM(MV_DIFFUSE_MAP2);
	DEFINE_PARAM(MV_NORMAL_MAP);
	DEFINE_PARAM(MV_AS_MAP);
	DEFINE_PARAM(MV_OVERLAY_MAP);
	DEFINE_PARAM(MV_ENVIRONMENT_MAP);
	DEFINE_PARAM(MV_ZENABLE);
	DEFINE_PARAM(MV_ZWRITE);
	DEFINE_PARAM(MV_CROSSFADE);
	DEFINE_PARAM(MV_LERP_MCMAP);
	DEFINE_PARAM(MV_SHADOW_POWER);
	DEFINE_PARAM(MV_OVERLAY_BIAS);

	virtual int CompareMaterials(const Material* mat1, const Material* mat2) const;

	virtual void OnMaterialFinalize(Material* mat);

	IVertexDeclaration	m_pVDTang[5];
	ComPtr<VertexBuffer> m_pInstanceVB;

	void CreateVertexDeclarations(bool skinned);
	virtual bool TouchTextures(Material* mat) const;

	//ps_color.hlsl -> color
	ComPtr<PixelShader> m_pPColor;
	ComPtr<PixelShader> m_pPColor30;
	ENF_INLINE PixelShader* GetColorPS()
	{
		return m_pPColor;
	}

	ENF_INLINE void SetSpecularLightPSConstants(SpecularParamsPS* params, const Material* mat, const FColor& specColor)
	{
		params->m_Specular = specColor * mat->GetColor(MV_SPECULAR_COMBINED);
		params->m_Specular.a = mat->GetScalarConst(MV_SPECULAR_EXP);
		params->m_SpecularBias = FColor1(mat->GetScalarDef(MV_SPECULAR_BIAS, 0));
	}

	ENF_INLINE void SetSpecularLightPSConstants(const Material* mat, const FColor& specColor)
	{
		FColor speccol = specColor * mat->GetColor(MV_SPECULAR_COMBINED);
		speccol.a = mat->GetScalarConst(MV_SPECULAR_EXP);

#ifdef ENF_DONE
		SET_PSH_CONST(CP_MAT_SPECULAR_COLOR, speccol);
		SET_PSH_CONST(CP_MAT_SPECULAR_BIAS, Vector4(mat->GetScalarDef(MV_SPECULAR_BIAS, 0)));
#endif
	}

	ENF_INLINE void ClearSpecularLightPSConstants()
	{
#ifdef ENF_DONE
		SET_PSH_CONST(CP_MAT_SPECULAR_COLOR, Vector4(0.0f));
		SET_PSH_CONST(CP_MAT_SPECULAR_BIAS, Vector4(0.0f));
#endif
	}

	void SetLightVSConstants(const ModelContext* mc, const ILight* light);
	virtual void OnSet(Pipeline* pipeline, PassType pt);
	virtual uint GetMaxInstances()
	{
		return MAX_INSTANCES;
	}
	virtual void BeginInstanced(const Pipeline* pipeline, PassType passtype, ModelContext* mc, Material* mat, const IMesh* mesh, const void* data);
	virtual bool IsCompatible(uint creationFlags) const;

	virtual const Texture* GetDiffuseTexture(const Material* mat) const;

	DEFINE_MCLASS_METHODS
END_DEFINE_MCLASS


//-----------------------------------------------------------------------------
BEGIN_DEFINE_ABSTRACT_MCLASS(MatParticle, "Particle", MaterialClass)
	DEFINE_PARAM(MV_ALPHA_TEST);
	DEFINE_PARAM(MV_BLEND_MODE);
	DEFINE_PARAM(MV_COLOR);
	DEFINE_PARAM(MV_AMBIENT);
	DEFINE_PARAM(MV_EMISSIVE);
	DEFINE_PARAM(MV_DIFFUSE_MAP);
	DEFINE_PARAM(MV_NORMAL_MAP);
	DEFINE_PARAM(MV_HEIGHT_MAP);
	DEFINE_PARAM(MV_ZENABLE);
	DEFINE_PARAM(MV_TILE_U);
	DEFINE_PARAM(MV_TILE_V);
	DEFINE_PARAM(MV_SOFTNESS);
	DEFINE_PARAM(MV_DEPTH_SCALE);
	DEFINE_PARAM(MV_LIGHT_SCALE);
	DEFINE_PARAM(MV_NORMAL_LIGHT_SCALE);
	DEFINE_PARAM(MV_CAMERA_BLEND0);
	DEFINE_PARAM(MV_CAMERA_BLEND1);
	DEFINE_PARAM(MV_CENTERX);
	DEFINE_PARAM(MV_CENTERY);
	DEFINE_PARAM(MV_SCALEX);
	DEFINE_PARAM(MV_SCALEY);

	IVertexDeclaration  m_pNewVD;

	ComPtr<PixelShader> m_pPColor;
	ComPtr<SuperShader> m_pAmbientNewVS;
	ComPtr<SuperShader> m_pAmbientPS;

	virtual bool TouchTextures(Material* mat) const;
	virtual int CompareMaterials(const Material* mat1, const Material* mat2) const;
	virtual void OnMaterialFinalize(Material* mat);
	virtual void OnSet(Pipeline* pipeline, PassType pt);
	virtual bool IsCompatible(uint creationFlags) const;
	virtual void GetTilesCount(const Material* mat, uint& tx, uint& ty) const 
	{
		tx = mat->GetInteger(MatParticle::MV_TILE_U);
		ty = mat->GetInteger(MatParticle::MV_TILE_V);
	}

	DEFINE_MCLASS_METHODS
END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_MCLASS(MatParticleSprite, "ParticleSprite", MatParticle)
	ComPtr<VertexBuffer> m_pSpriteVB;
	ComPtr<IndexBuffer> m_pSpriteIB;

	virtual void OnSet(Pipeline* pipeline, PassType pt);
	DEFINE_MCLASS_METHODS
END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_MCLASS(MatParticlePoint, "ParticlePoint", MatParticle)
	virtual void OnSet(Pipeline* pipeline, PassType pt);
	DEFINE_MCLASS_METHODS
END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_MCLASS(MatParticleStreak, "ParticleStreak", MatParticleSprite)
	ComPtr<VertexBuffer> m_pStreakVB;
	ComPtr<IndexBuffer> m_pStreakIB;

	virtual void OnSet(Pipeline* pipeline, PassType pt);
	DEFINE_MCLASS_METHODS
END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_EXT_MCLASS(MatDecal, "Decal", MaterialClass)
	DEFINE_PARAM(MV_USE_VERTEX_ALPHA);
	DEFINE_PARAM(MV_ALPHA_TEST);
	DEFINE_PARAM(MV_BLEND_MODE);
	DEFINE_PARAM(MV_COLOR);
	DEFINE_PARAM(MV_AMBIENT);
	DEFINE_PARAM(MV_EMISSIVE);
	DEFINE_PARAM(MV_SPECULAR_POWER);
	DEFINE_PARAM(MV_SPECULAR_EXP);
	DEFINE_PARAM(MV_SPECULAR_TINT);
	DEFINE_PARAM(MV_SPECULAR_BIAS);
	DEFINE_PARAM(MV_PARALLAX_SCALE);
	DEFINE_PARAM(MV_PARALLAX_OFFSET);
	DEFINE_PARAM(MV_DIFFUSE_MAP);
	DEFINE_PARAM(MV_NORMAL_MAP);
	DEFINE_PARAM(MV_AHS_MAP);
	DEFINE_PARAM(MV_ZENABLE);
	DEFINE_PARAM(MV_ZWRITE);
	DEFINE_PARAM(MV_SHADOW_POWER);

	virtual int CompareMaterials(const Material* mat1, const Material* mat2) const;

	virtual bool TouchTextures(Material* mat) const;
	virtual void OnMaterialFinalize(Material* mat);

	IVertexDeclaration	m_pVDTang[5];
	void CreateVertexDeclarations(bool skinned);

	//ps_color.hlsl -> color
	ComPtr<PixelShader> m_pPColor30;
	ENF_INLINE PixelShader* GetColorPS()
	{
		return m_pPColor30;
	}

	ComPtr<SuperShader> m_pAmbientVS;
	ComPtr<SuperShader> m_pLightVS;
	ComPtr<SuperShader> m_pDepthVS;
	ComPtr<SuperShader> m_pDepthPS;
	ComPtr<SuperShader> m_pAmbientPS;
	ComPtr<SuperShader> m_pLightPS;

	#ifdef SHADOW_MAP_8BIT
		ComPtr<SuperShader> 	m_pDepth8BitPS;
	#endif


	ENF_INLINE void SetSpecularLightPSConstants(SpecularParamsPS* params, const Material* mat, const FColor& specColor)
	{
		params->m_Specular = specColor * mat->GetColor(MV_SPECULAR_TINT);
		params->m_Specular.a = mat->GetScalarConst(MV_SPECULAR_EXP);
		params->m_SpecularBias = FColor1(mat->GetScalarDef(MV_SPECULAR_BIAS, 0));
	}

	ENF_INLINE void SetSpecularLightPSConstants(const Material* mat, const FColor& specColor)
	{
		FColor speccol = specColor * mat->GetColor(MV_SPECULAR_TINT);
		speccol.a = mat->GetScalarConst(MV_SPECULAR_EXP);
#ifdef ENF_DONE
		SET_PSH_CONST(CP_MAT_SPECULAR_COLOR, speccol);
		SET_PSH_CONST(CP_MAT_SPECULAR_BIAS, Vector4(mat->GetScalarDef(MV_SPECULAR_BIAS, 0)));
#endif
	}

	ENF_INLINE void ClearSpecularLightPSConstants()
	{
#ifdef ENF_DONE
		SET_PSH_CONST(CP_MAT_SPECULAR_COLOR, Vector4(0.0f));
		SET_PSH_CONST(CP_MAT_SPECULAR_BIAS, Vector4(0.0f));
#endif
	}

	void SetLightVSConstants(const ModelContext* mc, const ILight* light);
	virtual void OnSet(Pipeline* pipeline, PassType pt);
	virtual bool IsCompatible(uint creationFlags) const;

	virtual const Texture* GetDiffuseTexture(const Material* mat) const;

	DEFINE_MCLASS_METHODS
END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_MCLASS(MatDecalTree, "DecalTree", MatDecal)
	virtual ERESULT OnInit(TiXmlElement* root);
	virtual bool IsCompatible(const MaterialDescription& md);
	virtual void SetModel(const Pipeline* pipeline, PassType passtype, ModelContext* mc);
END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_MCLASS(MatVertexLitPerpixDX9, "GenericVertexlit", MatGeneric)

	ComPtr<SuperShader> m_pAmbientVS;
	ComPtr<SuperShader> m_pLightVS;
	ComPtr<SuperShader> m_pDepthVS;
	ComPtr<SuperShader> m_pShadowVS;
	ComPtr<SuperShader> m_pShadowPS;
	ComPtr<SuperShader> m_pDepthPS;
	ComPtr<SuperShader> m_pAmbientPS;
	ComPtr<SuperShader> m_pLightPS;
	ComPtr<SuperShader> m_pEnvMapPS;
	ComPtr<SuperShader>m_pEnvMapVS;
	
	#ifdef SHADOW_MAP_8BIT
		ComPtr<SuperShader> m_pDepth8BitPS;
	#endif

	virtual void OnMaterialFinalize(Material* mat);

	virtual void OnSet(Pipeline* pipeline, PassType pt);
	virtual void BeginInstanced(const Pipeline* pipeline, PassType passtype, ModelContext* mc, Material* mat, const IMesh* mesh, const void* data);
	virtual void RenderInstances(const Pipeline* pipeline, PassType passtype, const IMesh* mesh, const ModelContext** mc, uint numinstances);

	DEFINE_MCLASS_METHODS
END_DEFINE_MCLASS


//-----------------------------------------------------------------------------
BEGIN_DEFINE_EXT_MCLASS(MatVolLight, "VolumetricLight", MaterialClass)

	DEFINE_PARAM(MV_LIGHT_MAP)
	DEFINE_PARAM(MV_EMISSIVE)
	DEFINE_PARAM(MV_INTENSITY)
	DEFINE_PARAM(MV_FOV)
	DEFINE_PARAM(MV_DIST_ATT)
	DEFINE_PARAM(MV_DIST_BIAS)
	DEFINE_PARAM(MV_DIST_BIAS_SOFT)
	DEFINE_PARAM(MV_ANGLE_ATT)
	DEFINE_PARAM(MV_SOFT_DIST)
	DEFINE_PARAM(MV_SOFT_BIAS)
	DEFINE_PARAM(MV_SOFTSLICE)

	/*!
	hack, to get the model context data into BeginPass function, it comes out
	from the fact, that SetModelContext is called before BeginPass and in the case
	the model context is changed, BeginPass is always called
	*/
	struct GeometryData
	{
		Matrix34 m_mModelMatrix;
		Matrix34 m_mMatrix;
		Vector3	m_vLocPosition;
		Vector3	m_vDirection;
		float		m_fRadius;
		float		m_fFOV;
		float		m_fBIAS;
		float		m_fInvBIASSoft;
		int		m_iSoftBias;
		int		m_iSoft;
		bool		m_bDisable;
	};
	GeometryData m_GeometryData;

	/*!
	geometry data, return true if is something to render
	*/
	bool SetGeometryData();

	/*!
	compute slices direction in local coordinates camera system and compute world size of billboard
	*/
	float CreateSlicesDirLCS(Vector3& startpos, Vector3& endpos, Vector4& centerplane, bool& changepos, Vector3& billpos, AAB& viewAAB);


	IVertexDeclaration	m_pVertDecl;

	ComPtr<SuperShader> m_pAmbientVS;
	ComPtr<SuperShader> m_pAmbientPS;

	#ifdef ENF_STEREO_VISION
		ComPtr<SuperShader> m_pAmbientStereoPS;
	#endif

	ComPtr<SuperShader> m_pDebugPS;

	virtual bool TouchTextures(Material* mat) const;
	virtual void OnMaterialFinalize(Material* mat);
	virtual void OnSet(Pipeline* pipeline, PassType pt);
	bool IsCompatible(uint creationFlags) const;

	DEFINE_MCLASS_METHODS
END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_EXT_MCLASS(MatEngineCommon, "__EngineCommon__", MaterialClass)
	IVertexDeclaration	m_pShapesDeclaration;
	ComPtr<SuperShader>	m_pVShape;
	ComPtr<VertexShader>	m_pVOcclusion;
	ComPtr<VertexShader>	m_pVOcclusionFar;
	ComPtr<PixelShader>	m_pPColor;
	IVertexDeclaration	m_pOcclusionDeclaration;

	bool IsCompatible(uint creationFlags) const {return false;}
END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_MCLASS(MatSkinnedPerpixDX9, "GenericSkinned", MatVertexLitPerpixDX9)
	virtual ERESULT OnInit(TiXmlElement* root);
	virtual void SetModel(const Pipeline* pipeline, PassType passtype, ModelContext* mc);
	virtual bool IsCompatible(const MaterialDescription& md);
	virtual void RenderPass(const Pipeline* pipeline, PassType passtype, Material* mat, const IMesh* mesh, const void* data);
	virtual void BeginInstanced(const Pipeline* pipeline, PassType passtype, ModelContext* mc, Material* mat, const IMesh* mesh, const void* data);
	virtual void RenderInstances(const Pipeline* pipeline, PassType passtype, const IMesh* mesh, const ModelContext** mc, uint numinstances);
END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_EXT_MCLASS(MatDoubleSideSkinnedDX9, "DoubleSideSkinned", MatSkinnedPerpixDX9)
	DEFINE_PARAM(MV_BACKFACE_ATTENUATION);

	virtual void OnMaterialFinalize(Material* mat);
	virtual bool IsCompatible(const MaterialDescription& md);
	virtual ERESULT OnInit(TiXmlElement* root);
	virtual uint BeginPass(const Pipeline* pipeline, PassType passtype, Material* mat, const void* data);
END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_MCLASS(MatTreeDX9, "Tree", MatVertexLitPerpixDX9)
	virtual ERESULT OnInit(TiXmlElement* root);
	virtual bool IsCompatible(const MaterialDescription& md);
	virtual void RenderInstances(const Pipeline* pipeline, PassType passtype, const IMesh* mesh, const ModelContext** mc, uint numinstances);
	virtual void SetModel(const Pipeline* pipeline, PassType passtype, ModelContext* mc);
END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
//! test of plants geometry lighting in VS, enable in ps_plantsambient and vs_plantsambient too
//#define GEOMETRY_LIGHTING
BEGIN_DEFINE_EXT_MCLASS(MatPlantsDX9, "Plants", MatTreeDX9)
	DEFINE_PARAM(MV_BACKFACE_ATTENUATION);

	#ifdef GEOMETRY_LIGHTING
		DEFINE_PARAM(MV_CAPS_START_OFFS);
		DEFINE_PARAM(MV_CAPS_END_OFFS);
		DEFINE_PARAM(MV_FAKE_AO_MAX);
		DEFINE_PARAM(MV_FAKE_AO_MIN);
		DEFINE_PARAM(MV_FAKE_AO_HEIGHT);
		DEFINE_PARAM(MV_FAKE_AO_OFFS);
		DEFINE_PARAM(MV_DEBUG);

		ComPtr<PixelShader> m_pDebugPS;
	#endif

	DepthStencilStateHandle m_dssLightHandle;

	virtual void OnMaterialFinalize(Material* mat);
	virtual void OnUnset(Pipeline* pipeline, PassType passtype);
	virtual void OnSet(Pipeline* pipeline, PassType pt);
	virtual ERESULT OnInit(TiXmlElement* root);
	virtual uint BeginPass(const Pipeline* pipeline, PassType passtype, Material* mat, const void* data);
	virtual bool IsCompatible(const MaterialDescription& md);
	virtual void RenderInstances(const Pipeline* pipeline, PassType passtype, const IMesh* mesh, const ModelContext** mc, uint numinstances);
	virtual void SetModel(const Pipeline* pipeline, PassType passtype, ModelContext* mc);
END_DEFINE_MCLASS


//-----------------------------------------------------------------------------
//! test of plants geometry lighting in VS, enable in ps_plantsambient and vs_plantsambient too
//#define GEOMETRY_LIGHTING
BEGIN_DEFINE_EXT_MCLASS(MatDoubleSideDX9, "DoubleSide", MatVertexLitPerpixDX9)
	DEFINE_PARAM(MV_BACKFACE_ATTENUATION);

	virtual void OnMaterialFinalize(Material* mat);
	virtual ERESULT OnInit(TiXmlElement* root);
	virtual uint BeginPass(const Pipeline* pipeline, PassType passtype, Material* mat, const void* data);
	virtual bool IsCompatible(const MaterialDescription& md);
	virtual void RenderInstances(const Pipeline* pipeline, PassType passtype, const IMesh* mesh, const ModelContext** mc, uint numinstances);
END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_MCLASS(MatVertexLitPerpix2DDX9, "Generic2D", MatGeneric)
	ComPtr<SuperShader> m_pAmbientVS;
	ComPtr<SuperShader> m_pDepthVS;
	ComPtr<SuperShader> m_pDepthPS;
	ComPtr<SuperShader> m_pAmbientPS;

	virtual void OnMaterialFinalize(Material* mat);
	void CreateVertexDeclarations();

	virtual void OnSet(Pipeline* pipeline, PassType pt);
	virtual void BeginInstanced(const Pipeline* pipeline, PassType passtype, ModelContext* mc, Material* mat, const IMesh* mesh, const void* data);
	virtual void RenderInstances(const Pipeline* pipeline, PassType passtype, const IMesh* mesh, const ModelContext** mc, uint numinstances);
	DEFINE_MCLASS_METHODS
END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_MCLASS(MatFresnelDX9, "GenericFresnel", MatGeneric)
	ComPtr<SuperShader> m_pAmbientVS;
	ComPtr<SuperShader> m_pAmbientPS;
	ComPtr<SuperShader> m_pDepthVS;
	ComPtr<SuperShader> m_pDepthPS;

	#ifdef SHADOW_MAP_8BIT
		ComPtr<SuperShader> 	m_pDepth8BitPS;
	#endif

	virtual void OnMaterialFinalize(Material* mat);

	virtual void OnSet(Pipeline* pipeline, PassType pt);
	virtual void BeginInstanced(const Pipeline* pipeline, PassType passtype, ModelContext* mc, Material* mat, const IMesh* mesh, const void* data);
	virtual void RenderInstances(const Pipeline* pipeline, PassType passtype, const IMesh* mesh, const ModelContext** mc, uint numinstances);
	DEFINE_MCLASS_METHODS
END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_EXT_MCLASS(MatRefract, "Refract", MatGeneric)
	IVertexDeclaration	m_pVDTang[5];
	ComPtr<SuperShader> m_pAmbientVS;
	ComPtr<SuperShader> m_pAmbientPS;

	#ifdef ENF_STEREO_VISION
		ComPtr<SuperShader> m_pAmbientStereoPS;
	#endif

	DEFINE_PARAM(MV_REFRACT_MAP);
	DEFINE_PARAM(MV_REFRACT_MATRIX0);
	DEFINE_PARAM(MV_REFRACT_MATRIX1);
	DEFINE_PARAM(MV_REFRACT_SCALE);
	DEFINE_PARAM(MV_FORCE_REPAINT);
	
	virtual bool TouchTextures(Material* mat) const;
	virtual void OnMaterialFinalize(Material* mat);
	virtual void OnSet(Pipeline* pipeline, PassType pt);
	virtual void OnUnset(Pipeline* pipeline, PassType passtype);
	DEFINE_MCLASS_METHODS
END_DEFINE_MCLASS


//-----------------------------------------------------------------------------
BEGIN_DEFINE_ABSTRACT_MCLASS(MatGenericSprite, "GenericSprite", MaterialClass)

	struct InstanceInfo
	{
		Matrix34		m_mWorld;
		Vector4		m_vInfo;
	};

	#ifdef ENF_DONE
		static const uint MAX_INSTANCES = MAX_SPRITE_INSTANCES;
	#else
		static const uint MAX_INSTANCES = 1;
	#endif

	enum SpriteMode
	{
		SM_NONE		= 0,
		SM_Z_UP		= 1,
		SM_GROUND	= 2,
		SM_USER		= 3,
		SM_PROJ		= 4
	};

	DEFINE_PARAM(MV_SPRITE_MODE);
	DEFINE_PARAM(MV_ALPHA_TEST);
	DEFINE_PARAM(MV_ALPHA_SOFTTEST);
	DEFINE_PARAM(MV_BLEND_MODE);
	DEFINE_PARAM(MV_COLOR);
	DEFINE_PARAM(MV_EMISSIVE);
	DEFINE_PARAM(MV_ALPHA_BIAS);
	DEFINE_PARAM(MV_ZENABLE);
	DEFINE_PARAM(MV_ZWRITE);
	DEFINE_PARAM(MV_SOFT_DIST);
	DEFINE_PARAM(MV_SOFT_BIAS);

	int m_iAlphaTest;

	virtual int CompareMaterials(const Material* mat1, const Material* mat2) const;
	virtual void OnMaterialFinalize(Material* mat);

	virtual void OnSet(Pipeline* pipeline, PassType pt);
	virtual uint GetMaxInstances()
	{
		return MAX_INSTANCES;
	}

	virtual void BeginInstanced(const Pipeline* pipeline, PassType passtype, ModelContext* mc, Material* mat, const IMesh* mesh, const void* data);
	virtual void RenderInstances(const Pipeline* pipeline, PassType passtype, const IMesh* mesh, const ModelContext** mc, uint numinstances);
	virtual bool IsCompatible(uint creationFlags) const;

	DEFINE_MCLASS_METHODS

protected:
	IVertexDeclaration	m_pVD;
	ComPtr<VertexBuffer> m_pInstanceVB;
	ComPtr<VertexBuffer> m_pVBSprite;
	ComPtr<IndexBuffer>	m_pIBSprite;
	ComPtr<SuperShader>	m_pVertexShader;
	ComPtr<SuperShader>	m_pPixelShader;
	PixelShader*			m_pPColor30;
	bool						m_bSetDirty;

	bool SetSoft(const Pipeline* pipeline, Material* mat);

	void CreateInstanceBuffer();
	void CreateBuffers();


END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_EXT_MCLASS(MatSprite, "Sprite", MatGenericSprite)
	DEFINE_PARAM(MV_DIFFUSE_MAP);
	DEFINE_PARAM(MV_LIGHT);

	virtual bool TouchTextures(Material* mat) const;
	virtual void OnMaterialFinalize(Material* mat);
	void OnSet(Pipeline* pipeline, PassType pt);
	ERESULT OnInit(enf::TiXmlElement* root);
	uint BeginPass(const enf::Pipeline* pipeline, enf::PassType passtype, enf::Material* mat, const void* data);
END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_EXT_MCLASS(MatSimpleRefract, "SimpleRefract", MatGenericSprite)
	DEFINE_PARAM(MV_REFRACT_MAP);
	DEFINE_PARAM(MV_REFRACT_MAP2);
	DEFINE_PARAM(MV_DIFFUSE_MAP);
	DEFINE_PARAM(MV_PIXEL_DIST);
	DEFINE_PARAM(MV_REFRACT_SCALE);
	DEFINE_PARAM(MV_REFRACT_MAP2_SCALE);
	DEFINE_PARAM(MV_CHROM_ABER);

	virtual bool TouchTextures(Material* mat) const;
	virtual void OnMaterialFinalize(Material* mat);
	virtual void OnSet(Pipeline* pipeline, PassType pt);
	virtual void OnUnset(Pipeline* pipeline, PassType pt);
	bool IsCompatible(const MaterialDescription& md);
	ERESULT OnInit(enf::TiXmlElement* root);
	uint BeginPass(const enf::Pipeline* pipeline, enf::PassType passtype, enf::Material* mat, const void* data);
	
END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_MCLASS(MatHumanSkin, "HumanSkin", MatSkinnedPerpixDX9)
	DEFINE_PARAM(MV_RIM_POWER)
	DEFINE_PARAM(MV_RIM_BIAS)
	DEFINE_PARAM(MV_RIM_WEIGHT)
	DEFINE_PARAM(MV_LIGHT_TRANS)
	DEFINE_PARAM(MV_LIGHT_TRANS_WEIGHT)
	DEFINE_PARAM(MV_LAMBERT_BIAS)
	DEFINE_PARAM(MV_DEBUG_MODE)
	DEFINE_PARAM(MV_SUBDERMAL_MASK)

	ComPtr<SuperShader> m_pAmbientDebugPS1;
	ComPtr<SuperShader> m_pAmbientDebugPS2;

	virtual bool TouchTextures(Material* mat) const;
	virtual ERESULT OnInit(TiXmlElement* root);
	virtual void OnSet(Pipeline* pipeline, PassType pt);
	virtual void OnMaterialFinalize(Material* mat);
	virtual void BeginInstanced(const Pipeline* pipeline, PassType passtype, ModelContext* mc, Material* mat, const IMesh* mesh, const void* data);
	virtual void RenderInstances(const Pipeline* pipeline, PassType passtype, const IMesh* mesh, const ModelContext** mc, uint numinstances);
	virtual uint BeginPass(const Pipeline* pipeline, PassType passtype, Material* mat, const void* data);

	bool IsCompatible(const MaterialDescription& md);

END_DEFINE_MCLASS

//-----------------------------------------------------------------------------
BEGIN_DEFINE_MCLASS(OccluderMat, "OccluderMat", MaterialClass)

	VertexShader* m_pVS;
	PixelShader*  m_pPS;

	VertexBuffer*			m_pOccluderVB;
	IVertexDeclaration	m_pVD;

	~OccluderMat();

	virtual void OnMaterialFinalize(Material* mat);
	virtual void OnSet(Pipeline* pipeline, PassType pt);

	DEFINE_MCLASS_METHODS

END_DEFINE_MCLASS


}
#endif //ENF_MATERIALS_H
