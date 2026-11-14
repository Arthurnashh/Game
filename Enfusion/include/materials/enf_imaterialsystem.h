//-----------------------------------------------------------------------------
// File: enf_imaterialsystem.cpp
//
// Desc: Internal class for material system
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_IMATERIALSYSTEM_H
#define ENF_IMATERIALSYSTEM_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_set.h"
#include "common/enf_color.h"
#include "common/enf_hashmap.h"
#include "render/enf_rendmain.h"

#include "materials/enf_materialsystem.h"
#include "materials/enf_materialclass.h"
#include "render/enf_texture.h"
#include "render/enf_supershader.h"

namespace enf
{
/** @defgroup MaterialSystem Material system
@{*/

class Entity;


//-----------------------------------------------------------------------
struct ProjectionStack
{
	int				Left;
	int				Top;
	int				Width;
	int				Height;
	Matrix44	ProjMatrix;
	float			NearPlane;
	float			FarPlane;
	float			Aspect;

	ProjectionStack(): Left(0),Top(0), Width(0), Height(0), ProjMatrix(true), NearPlane(0), FarPlane(0), Aspect(0)
	{}
};

//-----------------------------------------------------------------------
struct SuperShaderHashFunc
{
	unsigned int operator()(const SuperShader* sh) const
	{
		return sh->GetHashValue();
	}

	int compare(const SuperShader* sh1, const SuperShader* sh2) const
	{
		return sh1->Compare(sh2);
	}
};

//-----------------------------------------------------------------------
class IMaterialSystem: public MaterialSystem, public FlushableObject
{
private:
	friend class Material;
	friend class ISuperShader;

	static const uint MAX_CAMERA_STACK = 16;

	Renderer*				m_pDevice;
	SamplerFilterStateHandle m_Filters[FILTER_MAX_VALUE];

	const char*			m_pRenderViewName;
	const char*			m_pGUIName[4];
	const char*			m_pNearestCubeName;
	Matrix43				m_CameraStack[MAX_CAMERA_STACK][2];
	uint						m_iCameraStackPos;
	ProjectionStack	m_ProjStack[MAX_CAMERA_STACK];
	uint						m_iProjStackPos;

	uint64	m_iTime;
	uint		m_iTimeStamp;

	Texture*	m_pIdentityNormalMap;
	Texture*	m_pIdentityNormalMapDXT;
	Texture*	m_pIdentityWhiteMap;
	Texture*	m_pIdentityBlackMap;

	float	m_fLastBias;
	bool	m_bEditorMode;
	bool	m_bInMirror;

	Array<MaterialClass*>m_MatClasses;

	HashMap<const char*, Material*, StringHashFunc> m_MaterialsHash;
	Set<Material*>	m_Materials;
	bool						m_bMaterialsSorted;

	CMutex m_Mutex;

	Set<SuperShader*>		m_SuperShaders;
	HashMap<SuperShader*, SuperShader*, SuperShaderHashFunc> m_SuperShadersHash;


	Array<MaterialVarDef>m_MaterialVarDefs;
	HashMap<CStr, uint, CStrHashFuncCS> m_MaterialVarHash;

	const ParmEnumMaterialDeform** m_pDeformFuncs;
	ScriptContext*		m_pScript;


	struct RTTCache
	{
		uint64					m_iTimeStamp;
		ComPtr<Texture>	m_pTexture;

		RTTCache():	m_iTimeStamp(0) 
		{}
	};

	SArray<RTTCache, 6>	m_CurrentRenderTextures;

	struct RTCache
	{
		uint64								m_iTimeStamp;
		int										m_iRef;
		ComPtr<RenderTarget>	m_pRT;

		RTCache(): m_iTimeStamp(0), m_iRef(0) 
		{}
	};

	struct RTCacheSort
	{
		uint Width, Height;
		TextureFormat RTFormat;
		TextureFormat DepthFormat;
		FSAAType FSAA;
		uint Usage;
		uint EDRAMOffset;
		uint Handle;

		int compare(const RTCacheSort& other) const
		{
			if(Width < other.Width) return -1;
			if(Width > other.Width) return 1;
			if(Height < other.Height) return -1;
			if(Height > other.Height) return 1;
			if(RTFormat < other.RTFormat) return -1;
			if(RTFormat > other.RTFormat) return 1;
			if(DepthFormat < other.DepthFormat) return -1;
			if(DepthFormat > other.DepthFormat) return 1;
			if(FSAA < other.FSAA) return -1;
			if(FSAA > other.FSAA) return 1;
			if(Usage < other.Usage) return -1;
			if(Usage > other.Usage) return 1;
			if(EDRAMOffset < other.EDRAMOffset) return -1;
			if(EDRAMOffset > other.EDRAMOffset) return 1;
			if(Handle < other.Handle) return -1;
			if(Handle > other.Handle) return 1;
			return 0;
		}

		int compareNH(const RTCacheSort& other) const
		{
			if(Width < other.Width) return -1;
			if(Width > other.Width) return 1;
			if(Height < other.Height) return -1;
			if(Height > other.Height) return 1;
			if(RTFormat < other.RTFormat) return -1;
			if(RTFormat > other.RTFormat) return 1;
			if(DepthFormat < other.DepthFormat) return -1;
			if(DepthFormat > other.DepthFormat) return 1;
			if(FSAA < other.FSAA) return -1;
			if(FSAA > other.FSAA) return 1;
			if(Usage < other.Usage) return -1;
			if(Usage > other.Usage) return 1;
			if(EDRAMOffset < other.EDRAMOffset) return -1;
			if(EDRAMOffset > other.EDRAMOffset) return 1;
			return 0;
		}

		bool operator<(const RTCacheSort& other) const { return compare(other) < 0; }
		bool operator>(const RTCacheSort& other) const { return compare(other) > 0; }
		bool operator==(const RTCacheSort& other) const { return compare(other) == 0; }
	};

	SArray<RTCache, 64>	m_RenderTargets;
	Set<RTCacheSort> m_RenderTargetsSet;
	ENF_INLINE int FindGE(const Set<RTCacheSort>& set, const RTCacheSort &a);

	struct IdentityTextureItem
	{
		Texture*		m_pTexture;
		uint			m_iFlags;
		FColor		m_Color;
		int compare(const IdentityTextureItem& other) const
		{
			if(m_iFlags < other.m_iFlags) return -1;
			if(m_iFlags > other.m_iFlags) return 1;
			if(m_Color.r < other.m_Color.r) return -1;
			if(m_Color.r > other.m_Color.r) return 1;
			if(m_Color.g < other.m_Color.g) return -1;
			if(m_Color.g > other.m_Color.g) return 1;
			if(m_Color.b < other.m_Color.b) return -1;
			if(m_Color.b > other.m_Color.b) return 1;
			if(m_Color.a < other.m_Color.a) return -1;
			if(m_Color.a > other.m_Color.a) return 1;
			return 0;
		}
		bool operator<(const IdentityTextureItem& other) const
		{
			return compare(other) < 0;
		}
		bool operator>(const IdentityTextureItem& other) const
		{
			return compare(other) > 0;
		}
		bool operator==(const IdentityTextureItem& other) const
		{
			return compare(other) == 0;
		}
		IdentityTextureItem(Texture* tex, const FColor& col, uint flags): m_pTexture(tex), m_Color(col), m_iFlags(flags) {}
		IdentityTextureItem() {}
	};
	Set<IdentityTextureItem>	m_IdentityTextures;

	~IMaterialSystem();
	
	virtual void OnBeginFrame();

	virtual void Flush(unsigned int level);
public:
	ENF_DECLARE_MMOBJECT(IMaterialSystem);

	IMaterialSystem(Renderer* device, ScriptContext* script, bool editormode);


	bool IsEditorMode() const
	{
		return m_bEditorMode;
	}

	ENF_INLINE void SetFilterMode(uint tmu, uint filter)
	{
		SET_SFS(tmu, m_Filters[filter]);
	}

	MaterialClass* FindMaterialClass(const char* name);
	MaterialClass* FindMaterialClass(const MaterialDescription& mdesc, CStr inherited);
	bool AddMaterialClass(MaterialClass* mclass);

	SuperShader* CreateSuperShader(CStr name, ProfileType prof, const char* const* options, const char* const* defines = NULL);

	Texture* GetIdentityColorMap(const FColor& color, uint flags);

	const Set<Material*>& GetMaterials() const
	{
		return m_Materials;
	}

	Material* GetMaterial(CStr matname);
	Material* GetMaterial(CStr matname, uint flags);
	Material* CreateMaterial(CStr matname);
	virtual void RemoveMaterial(Material* material);
	bool MaterialExists(CStr matname) const;
	
	Texture* GetCurrentRender();
	RenderTarget* AllocRenderTarget(uint width, uint height, TextureFormat rtfmt, TextureFormat depthfmt, FSAAType fsaa, uint usage, uint edramoffset, uint& handle, uint texFlags = 0);
	void FreeRenderTarget(uint handle);

	void OnInsert(TiXmlElement* xmlnode);
	bool OnChangeSettings(TiXmlElement* xmlnode);
	void OnEvent(const Event& event);

	void SetInMirror(bool inmirror)
	{
		m_bInMirror = inmirror;
	}

	void ToggleInMirror()
	{
		m_bInMirror = !m_bInMirror;
	}

	//!Sets cullmode according flags
	void SetCull(const Material* mat) const;
	void SetCull(bool doCull) const;

	uint GetNumVariableDefs() const;
	uint AddVariableDef(const MaterialVarDef& mvdef);
	const MaterialVarDef* GetVariableDef(uint index) const;
	const MaterialVarDef* GetVariableDef(CStr name) const;
	uint FindVariableDef(CStr name) const;
	
	const Array<MaterialClass*>& GetMaterialClasses() const
	{
		return m_MatClasses;
	}

	void BeginPass()
	{
	}

	void BeginFrame(const Matrix43& viewmatrix, RenderType type);
	void EndFrame();

	void SetProjection(int left, int top, int width, int height, const Matrix44& projmatrix, float nearplane, float farplane);

	void SetupCamera(const Matrix43& viewmatrix, const Matrix43& invviewmatrix);

	bool PushProjection(int left, int top, int width, int height, const Matrix44& projmatrix, float nearplane, float farplane);
	void PopProjection();
	bool PushCamera(const Matrix43& viewmatrix, const Matrix43& invviewmatrix);
	void PopCamera();

	void SetBias(float bias);

	const ParmEnumMaterialDeform** GetDeformFuncs() const
	{
		return m_pDeformFuncs;
	}

};

//@}

}
#endif //ENF_IMATERIALSYSTEM_H