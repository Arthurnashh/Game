//-----------------------------------------------------------------------------
// File: enf_pipeline.h
//
// Desc: Base class for rendering pipeline. Gathers models, surfaces,
//       do some sorting and sends result to MaterialClass
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_PIPELINE_H
#define ENF_PIPELINE_H

#include "enf_config.h"
#include "common/enf_mathlib.h"
#include "common/enf_viewfrustum.h"
#include "render/enf_vertexcache.h"
#include "render/enf_renderenums.h"
#include "enf_ilight.h"
#include "engine/enf_module.h"
#include "materials/enf_material.h"
#include "materials/enf_materialsystemenums.h"

namespace enf
{
/** @defgroup Pipeline Pipeline
@{*/

class Entity;
class BaseWorld;
class Texture;
struct MEnvMap;
class Material;
class Renderer;
class Vector4;
class Matrix44;
struct RRect;
class VObjectInstance;

//-----------------------------------------------------------------------
struct PipelineDescription
{
	uint	m_iMaxModels;
	uint	m_iMaxSurfaces;
	uint	m_iMaxMatrices;
	uint	m_iMaxInstancedMeshes;

	uint	m_iDynamicCacheVSize;
	uint	m_iDynamicCacheISize;
	uint	m_iDynamicCacheBuffers;
	
	uint	m_iStaticCacheVSize;
	uint	m_iStaticCacheISize;
	uint	m_iStaticCacheBuffers;

	uint	m_iShadowMapSize;

	int	m_iMaxDecalDistance;
	int	m_iDepthRange;
	uint	m_iLightingLevel;
	uint	m_iReflections;

	PipelineDescription():
		m_iMaxModels(0),
		m_iMaxSurfaces(0),
		m_iMaxMatrices(0),
		m_iMaxInstancedMeshes(1024),
		m_iDynamicCacheVSize(1024 * 1024),
		m_iDynamicCacheISize(256 * 1024),
		m_iDynamicCacheBuffers(1),
		m_iStaticCacheVSize(1024 * 1024),
		m_iStaticCacheISize(256 * 1024),
		m_iStaticCacheBuffers(1),
		m_iShadowMapSize(0),
		m_iMaxDecalDistance(3072),
		m_iDepthRange(6400),
		m_iLightingLevel(0),
		m_iReflections(0)
	{
	}
};


/*!
Helper struct for instanced meshes from VObjects
*/
struct InstancedMesh
{
	uint	m_iFrameCount;
	uint	m_iMeshIndex;
	InstancedMesh(): m_iFrameCount(0), m_iMeshIndex(0) {}
};


//-----------------------------------------------------------------------
enum MeshItemFlags
{
	//!Shadows from static geometry
	RF_STATICSHADOW = BITF(4),
	RF_DECAL				= BITF(5),

	//!Custom bits above this. Declare them as (RF_USER << 0...n)
	RF_USER					=	BITF(16),
};

//-----------------------------------------------------------------------
class IMesh
{
public:
	ushort				m_Type;
	union
	{
		ubyte				m_iStride[2]; //TODO: replace it by VertexDeclaration
		ushort			m_iStrides;
	};
	IndexBuffer*	m_pIB;
	VertexBuffer*	m_pVB[2]; //+optional 2nd stream (ie. static vertexlights)
	uint					m_iOffset;

	uint					m_iFirstVertex;
	uint					m_iFirstIndex;

	uint					m_iNumVertices;
	uint					m_iPrimitiveCount;

	//for skinning and instancing only
	union
	{
		struct 
		{
			ushort			m_iMatrixOffset; //+ModelContext::m_iFirstMatrix
			ubyte				m_iNumMatrices;
			ubyte				m_iMaxInfluence;
		};
		uint	m_iSkinInfo;
		uint  m_iNumInstances;
	};
	uint				m_iFlags;	//<See enf::MeshItemFlags
	void*				m_pUserData;

	ENF_INLINE bool IsFlagSet(uint flag) const { return ((m_iFlags & flag) != 0); }
	ENF_INLINE bool IsAllFlagsSet(uint flag) const { return ((m_iFlags & flag) == flag); }
	ENF_INLINE uint SetFlags(uint flags)
	{
		m_iFlags |= flags;
		return m_iFlags;
	}

	ENF_INLINE uint GetFlags() const { return m_iFlags; }

	IMesh():
		m_Type(PT_BAD),
		m_pIB(NULL),
		m_iFirstVertex(0),
		m_iFirstIndex(0),
		m_iNumVertices(0),
		m_iPrimitiveCount(0),
		m_iSkinInfo(0),
		m_pUserData(NULL),
		m_iFlags(0)
	{
	}

private:
	friend class Pipeline;
};


const uint MAX_PIPELINE_INSTANCES = 1024;

//-----------------------------------------------------------------------
/*!
Mesh context data in pipeline stack
*/
//-----------------------------------------------------------------------
class MeshItem: public IMesh
{
public:
	MeshItem(): IMesh(),
		m_pMaterial(NULL),
		m_iSort(0),
		m_iMaxInstances(MAX_PIPELINE_INSTANCES)
	{}

	Material* GetMaterial() const
	{
		return m_pMaterial;
	}

	int GetSort() const
	{
		return m_iSort;
	}

	uint GetMaxInstances() const
	{
		return m_iMaxInstances;
	}

	void SetMaxInstances(uint maxInstances)
	{
		m_iMaxInstances = maxInstances;
	}

protected:
	friend class Pipeline;

	Material*		m_pMaterial;
	int				m_iSort;
	uint				m_iMaxInstances;
};


//-----------------------------------------------------------------------
struct AmbientLight
{
	uint				m_iFlags;			///<See LightFlags
	Vector3			m_Direction;	///<Light direction
	FColor			m_Diffuse;		///<Diffuse color of light
	FColor			m_Specular;		///<Specular color of light
	FColor			m_Ambient;		///<Sum of ambient colors
	float				m_fHDRBrightness;

	void Clear()
	{
		m_iFlags = 0;
		m_Direction.Zero();
		m_Diffuse.Zero();
		m_Specular.Zero();
		m_Ambient.Zero();
		m_fHDRBrightness = 0;
	}
};

//-----------------------------------------------------------------------
//!Model context flags. Only 16bit!!!!
enum ModelContextFlags
{
	MCF_RECEIVESHADOW		= BITF(0), ///<Model receives shadows from sky

	MCF_LOD_0						= BITF(1),
	MCF_LOD_1						= BITF(2),
	MCF_IDENTWORLD			= BITF(3), ///<Model to World matrix is identity
	
	MCF_CASTSHADOW			= BITF(5), ///<Model casts shadows from sky
	MCF_HAS_ENVMAP			= BITF(6)
};

//-----------------------------------------------------------------------
class PUBLIC_API ModelContext
{
protected:

	friend class Pipeline;

	Matrix34	m_mMatrix;
	Vector4		m_vInstanceInfo;

	uint			m_iFlags;					///< See enf::ModelContextFlags
	ushort		m_iFirstMatrix;		///< Begin of matrices
	ubyte			m_iAmbientLight;	///< Ambient light
	ubyte			m_iEnvMap;				///< Environment cube map that is in interaction with this ModelContext.

public:
	ENF_DECLARE_MMOBJECT(ModelContext);

	Entity*		m_pEntity;
	VObjectInstance*	m_pInstance;
	int				m_iDistance;			///< Distance of nearest corner to camera
	                                                                                    
	float			m_fLODTime;				///< 0...1 normalized time
	uint			m_Pad[2];					///< pad to multiply of 16

	ModelContext();

	ENF_INLINE VObjectInstance* GetInstance() const
	{
		return m_pInstance;
	}

	ENF_INLINE void SetAmbientLight(uint amblight)
	{
		m_iAmbientLight = (ushort)amblight;
	}

	ENF_INLINE const Matrix34& GetWorldMatrix() const
	{
		return m_mMatrix;
	}

	ENF_INLINE Vector4& SetInstanceInfo()
	{
		return m_vInstanceInfo;
	}

	ENF_INLINE const Vector4& GetInstanceInfo() const
	{
		return m_vInstanceInfo;
	}

	ENF_INLINE uint GetFlags() const
	{
		return m_iFlags;
	}

	ENF_INLINE bool IsFlagSet(uint flag) const
	{
		return ((m_iFlags & flag) != 0);
	}

	ENF_INLINE bool IsAllFlagsSet(uint flag) const
	{
		return ((m_iFlags & flag) == flag);
	}

	ENF_INLINE uint SetFlags(uint flags)
	{
		m_iFlags |= flags;
		return (uint)m_iFlags;
	}

	ENF_INLINE uint ClearFlags(uint flags)
	{
		uint ret = m_iFlags & flags;
		m_iFlags &= ~flags;
		return ret;
	}
};

//-----------------------------------------------------------------------
/*!
Temporary struct created when rendering mirrors. It holds
mirrored matrix and clipping frustum.
*/
//-----------------------------------------------------------------------
struct Mirror
{
	Vector3			m_vPVSPosition;
	MPlane			m_Plane;
	ViewFrustum	m_Frustum;
	Matrix43		m_mMirror;

	bool Build(const MPlane& plane, const AAB& bbox, const Vector3& from);
	bool GetPlaneFromMesh(MPlane& plane, const float* vec, uint stride, uint num, const ushort* ind, uint numfaces);
	AAB GetBBoxFromMesh(const float* vec, uint stride, uint num);
};


//-----------------------------------------------------------------------
class TBaseItem
{
public:
	MeshItem*			Mesh;
	ModelContext*	Model;

	bool CanInstance(const TBaseItem& other) const
	{
		if(Mesh != other.Mesh) return false;
		return Model->GetFlags() == other.Model->GetFlags();
	}

	TBaseItem() {}
};

//-----------------------------------------------------------------------
class TItem: public TBaseItem
{
public:
	int	Sort;
	TItem() {}
};

//-----------------------------------------------------------------------
/*!
Basic set of MeshItems in Pipeline. There are all meshes,
that will be rendered.
*/
//-----------------------------------------------------------------------
class MeshItemSet: public Array<TItem>
{
public:
	TJobHandle m_JobHandle;

	ENF_INLINE void Insert(int sort, MeshItem* mesh, ModelContext* mc)
	{
		TItem& item = Array<TItem>::Append();
		item.Mesh = mesh;
		item.Model = mc;
		item.Sort = sort;
	}

	void Sort();
	MeshItemSet(): Array<TItem>(), m_JobHandle(BAD_JOBHANDLE)
	{
	}
};


typedef SArray<ILight*,32> DLightsArray; //32 because of 32bit mask

//-----------------------------------------------------------------------
//need it away for deffered light render
struct LightRef
{
	AAB				m_ShadowCastersBounds;
	const ILight*	m_pLight;
	LightRef(const ILight* light): m_ShadowCastersBounds(true), m_pLight(light)
	{
	}
	LightRef(): m_ShadowCastersBounds(true), m_pLight(NULL)
	{
	}
};


const int TRANS_SORT_GRANULARITY = 32 * 3;		//granularity for quantization
const int TRANS_MAX_QUANT_DISTANCE = (1 << 11) - 1;	//maximum distance after quantization -> = 2048*3 = 6km max resolution


//-----------------------------------------------------------------------
class PUBLIC_API Pipeline: public Module
{
public:
	//Opaque
	//
	//translucent
	//0...11		Material SortID
	//12..15		Material Sort Bias
	//16..30		Quantized distance
	//32			Sign
	static const uint TRANSP_SORTID_SHIFT				= 0;
	static const uint TRANSP_BIAS_SHIFT					= 12;
	static const uint TRANSP_CUSTOM_SORT_SHIFT	= 12;
	static const uint TRANSP_DISTANCE_SHIFT			= 16;

	static const uint OPAQUE_SORTID_SHIFT				= 8;
	static const uint OPAQUE_BIAS_SHIFT					= 27;
	
	static const uint MAX_SORT_NUM							= 8;

	ENF_DECLARE_ABSTRACT_ITEM(Pipeline, Module);

	ENF_INLINE uint GetMaxObjects() const
	{
		return m_Description.m_iMaxModels;
	}
	void UpdateCurrentTargetExtents();

	ENF_INLINE const PipelineDescription& GetDescription() const
	{
		return m_Description;
	}

	void PrintStats();

	/** \name Runtime methods
	These are valid/usable between BeginFrame and EndFrame call.
	*/
	//@{

	/*!
	Must be called as the first in frame
	\param world
	The BaseWorld we are going ot render
	\param type
	Type of render. See enf::RenderType
	\param camera
	Index of camera we are rendering
	\param frustum
	View frustum we are rendering from
	*/
	virtual void BeginFrame(BaseWorld* world, RenderType type, uint camera, const ViewFrustum& frustum);

	/*!
	Must be called as the last in frame
	*/
	virtual void EndFrame();

	//!Returns mask of camera-views that needs to be updated
	ENF_INLINE uint CameraMask() const
	{
		return m_iUpdateCameraMask;
	}

	//!Tells what type of render we are doing
	ENF_INLINE RenderType GetRenderType() const
	{
		return m_Type;
	}

	//!Tells what BaseWorld we are rendering
	ENF_INLINE BaseWorld* GetWorld() const
	{
		return m_pWorld;
	}

	//!Tells what camera we are rendering
	ENF_INLINE uint GetCamera() const
	{
		return m_iCamera;
	}

	ENF_INLINE Vector3 GetQuantizedCameraPos() const
	{
		return m_vQCamera;
	}
	ENF_INLINE const Matrix43& GetCameraMatrix() const
	{
		return m_mCamera;
	}

	ENF_INLINE const ViewFrustum& GetViewFrustum() const
	{
		return m_ViewFrustum;
	}

	ENF_INLINE const ViewFrustum& GetShadowViewFrustum() const
	{
		return m_ViewFrustumWithShadow;
	}
	ENF_INLINE float GetMaxShadowCasterDistance() const
	{
		return m_fMaxShadowCasterDistance;
	}

	ENF_INLINE uint GetShadowPlanes() const
	{
		return m_iShadowExtrudedFrustumPlanes;
	}

	//!Tells number of mirrors visible in the scene
	ENF_INLINE uint GetNumMirrors() const
	{
		return m_Mirrors.GetCardinality();
	}

	Mirror* AddMirror();

	ENF_INLINE const Mirror* GetMirror(uint num) const
	{
		return &m_Mirrors[num];
	}

	//!Adds visible dynamic light
	bool AddDLight(ILight* light)
	{
		if(m_pActiveDynamicLights.IsFull())
			return false;

		m_pActiveDynamicLights.Insert(light);
		return true;
	}

	//!Gets number of visible dynamic lights
	ENF_INLINE uint GetNumDLights() const
	{
		return m_pActiveDynamicLights.GetCardinality();
	}

	//!Gets visible dynamic light
	ENF_INLINE const ILight* GetDLight(uint index) const
	{
		return m_pActiveDynamicLights[index];
	}

	//!Gets number of lights
	ENF_INLINE uint GetNumLights() const
	{
		return m_ActiveLights.GetCardinality();
	}

	ENF_INLINE const ILight* GetLight(uint index) const
	{
		return m_ActiveLights[index].m_pLight;
	}

	/*!
	Sets current ModelContext. By default, this is called by BeginObject, but
	when we are going back to Object we already ended by EndObject, because there
	are pending some meshes, SetObject must be called
	*/
	ENF_INLINE void SetObject(ModelContext *mc)
	{
		enf_assert(mc != NULL);
		m_pLastModelContext = mc;
	}

	ENF_INLINE InstancedMesh* GetInstancedMeshes(uint offset) const
	{
		enf_assert(offset < m_iInstancedMeshOffset);
		return &m_pInstancedMeshes[offset];
	}

	ENF_INLINE uint AllocInstancedMeshes(uint numMeshes)
	{
		uint off = m_iInstancedMeshOffset;

		while(true)
		{
			if((off + numMeshes) <= m_iMaxInstancedMeshes)
			{
				m_iInstancedMeshOffset = off + numMeshes;
				return off;
			}
			GrowInstancedMeshes();
		}
	}

	//-----------------------------------------------------------------------

	//!Allocate object in advance
	uint AllocObject();

	/*!
	Called upon beginning of object rendering, before BeginMesh
	\param ent
	Entity we are going to render
	\param distance
	Distance-to-camera of nearest corner of entity boundbox.
	\param lights
	Array of lights affecting the model
	\param flags
	Optional flags. See ModelContext::Flags
	\param nummatrices
	Number of matrices needed for the object
	*/
	ModelContext* BeginObject(Entity *ent, int distance, TModelLights* lights, uint flags, uint nummatrices);

	/*!
	The same as BeginObject but it makes clone of existing context. Usable when you need
	to render meshes with different context parameters. For example, decals with dynamic light
	on object with static light, needs to be handled this way.
	\param clone
	Context we are going to clone
	\param flags
	Optional flags. See ModelContext::Flags
	*/
	ModelContext* BeginObject(ModelContext* clone, uint flags, uint nummatrices);

	//!The same as BeginObject but it uses previously allocated context.
	ModelContext* BeginObject(uint allocIndex, Entity *ent, int distance, TModelLights* lights, uint flags, uint nummatrices);

	//!Must be called after BeginObject and filling by new meshes
	void EndObject();

	/*!
	Allocate new MeshItem. Must be terminated by EndMesh call.
	\param pt
	Primitive type we are rendering
	\param sortval
	Sorting value
	\param mat
	Material
	\param offsetMatrices
	Offset in per-object matrix array
	\param numMatrices
	Number of matrices for skinning
	\param matrices
	When nummatrices!=0, this must be valid pointer to receive array of 3x4 skinning matrices
	\return
	Pointer to new MeshItem, or NULL
	*/
	MeshItem* BeginMesh(PrimitiveType pt, int sortval, Material* mat, uint offsetMatrices = 0, uint numMatrices = 0, Matrix34** matrices = NULL);

	/*!
	Must be called after filling MeshItem created by BeginMesh.
	*/
	ENF_INLINE void EndMesh()
	{
		MeshItem* mesh = &m_pStack[m_iStack - 1];
		enf_assert(mesh->m_iStride[0]);
	}


	//!Returns pointer to skinning matrices. See Pipeline::BeginMesh
	ENF_INLINE const Matrix34* GetMatrix(const ModelContext* mc, const IMesh* mesh) const
	{
		return &m_pMatrices[mc->m_iFirstMatrix + mesh->m_iMatrixOffset];
	}

	/*!
	Inserts mesh into environment receivers.
	\param sort
	Sorting value
	\param mesh
	Mesh to be inserted
	\param envmap
	Environment map
	*/
	void InsertToEnvironmentSet(uint sort, MeshItem* mesh, MEnvMap* envmap);

	//!Inserts visible static light to the scene.
	uint InsertLight(const ILight* light);

	/*!
	Inserts mesh into diffuse&specular light receivers.
	\param sort
	Sorting value
	\param mesh
	Mesh to be inserted
	\param light
	Light emitor
	*/
	void InsertToLightSet(uint sort, MeshItem* mesh, const ILight* light);

	/*!
	Inserts mesh into shadow casters. This do not add shadow caster bounds.
	Use AddShadowCasterBounds at the end!
	\param mesh
	Mesh to be inserted
	\param light
	Light emitor
	*/
	void InsertToShadowSet(MeshItem* mesh, const ILight* light);

	ENF_INLINE void AddShadowCasterBounds(const ILight* light, const AAB& wbbox)
	{
		m_ActiveLights[light->m_iNum].m_ShadowCastersBounds.Add(wbbox);
	}

	/*!
	Inserts mesh into shadow casters, with own world-space bounding box
	\param mesh
	Mesh to be inserted
	\param light
	Light emitor
	\param wbbox
	World-space bounding box
	*/
	void InsertToShadowSet(MeshItem* mesh, const ILight* light, const AAB& wbbox);

	//!Inserts mesh into depth-writers
	ENF_INLINE void InsertToAmbientDepthSet(MeshItem* mesh)
	{
		const Material* mat = mesh->GetMaterial();
		uint sorttype = mat->GetSortType();
		m_AmbientDepthSet[sorttype].Insert(mesh->GetSort(), mesh, m_pLastModelContext);
	}

	ENF_INLINE void InsertToAmbientDepthSet(int sortValue, MeshItem* mesh)
	{
		const Material* mat = mesh->GetMaterial();
		uint sorttype = mat->GetSortType();
		m_AmbientDepthSet[sorttype].Insert(sortValue, mesh, m_pLastModelContext);
	}

	ENF_INLINE void InsertToAmbientSet(MeshItem* mesh)
	{
		enf_assert(mesh->m_iStride[0]);

		Material* mat = mesh->GetMaterial();
		uint sorttype = mat->GetSortType();

		m_MeshItemSet[sorttype].Insert(mesh->GetSort(), mesh, m_pLastModelContext); //put it into sorted set
	}

	ENF_INLINE void InsertToAmbientSet(int sortValue, MeshItem* mesh)
	{
		enf_assert(mesh->m_iStride[0]);

		Material* mat = mesh->GetMaterial();
		uint sorttype = mat->GetSortType();

		m_MeshItemSet[sorttype].Insert(sortValue, mesh, m_pLastModelContext); //put it into sorted set
	}

	//@}

	//!Renders all meshes in stack. Must be called before EndFrame
	virtual void Render();

	//!Gets Renderer pointer
	ENF_INLINE Renderer* GetRender() const
	{
		return m_pRender;
	}


	ENF_INLINE VCache* GetStaticCache()
	{
		return &m_StaticCache;
	}

	ENF_INLINE VCache* GetDynamicCache()
	{
		return &m_DynamicCache;
	}

	//!Constructor
	Pipeline(Renderer* rend, const PipelineDescription& description);

	ENF_INLINE uint GetNumMeshes() const
	{
		return m_iStack;
	}
	
	MeshItem* GetMesh(uint mesh)
	{
		return &m_pStack[mesh];
	}

	uint GetFrameCount() const
	{
		return m_iFrameCount;
	}

	ENF_INLINE MEnvMap* GetEnvMap(const ModelContext* mc) const
	{
		if(mc->m_iEnvMap >= m_pActiveEnvMaps.GetCardinality())
			return NULL;
		return m_pActiveEnvMaps[mc->m_iEnvMap];
	}

	ENF_INLINE const AmbientLight& GetAmbientLight() const
	{
		return m_AmbientLights[1];
	}

	ENF_INLINE const AmbientLight& GetAmbientLight(const ModelContext* mc) const
	{
		return m_AmbientLights[mc->m_iAmbientLight];
	}

	ENF_INLINE AmbientLight& GetAmbientLight(const ModelContext* mc)
	{
		return m_AmbientLights[mc->m_iAmbientLight];
	}

	//!Returns new ambient light definition. Must be called between BeginObject/EndObject!
	ENF_INLINE AmbientLight& SetAmbientLight()
	{
		enf_assert(m_pLastModelContext != NULL);
		AmbientLight& l = m_AmbientLights.Append();
		m_pLastModelContext->m_iAmbientLight = m_AmbientLights.GetCardinality() - 1;
		return l;
	}

	bool IsShadowCaster(uint lightIndex) const
	{
		return !m_ShadowSet[lightIndex].IsEmpty();
	}

	virtual void UtilVisualizeOverdraw()
	{}

	const Array<MEnvMap*>& GetActiveEnvMaps() const
	{
		return m_pActiveEnvMaps;
	}

	float GetTargetWidth() const
	{
		return m_fTargetWidth;
	}

	float GetTargetHeight() const
	{
		return m_fTargetHeight;
	}

	/*!
	Utility for debug purpose. It renders content of texture to render target
	\param rgbabias	Bias of each color channel
	\param rgbamask	Mask of each color channel
	\param tex			The texture to be rendered
	\param rect			Destination rectangle
	\param cube			Render cube map if cube >= 0, cube is than the side of cube map
	*/
	virtual void UtilRenderTexture(const Vector4& rgbabias, const Matrix44& rgbamask, Texture* tex, const RRect& rect, bool filter, int cube = -1) const
	{}

	virtual void UtilRenderTexture(Texture* tex, const RRect& rect, bool filter, int cube = -1) const 
	{}

protected:

	friend class ModelContext;

	const Entity*		m_pLastEntity;
	const Material*	m_pLastMaterial;

	Renderer*				m_pRender;
	ViewFrustum			m_ViewFrustum;
	ViewFrustum			m_ViewFrustumWithShadow;
	uint						m_iShadowExtrudedFrustumPlanes;
	float						m_fMaxShadowCasterDistance;

	RenderType			m_Type;

	BaseWorld*			m_pWorld;
	uint						m_iCamera;
	Matrix43				m_mCamera;
	Vector3					m_vQCamera;

	PipelineDescription	m_Description;

	ModelContext*		m_pLastModelContext;
	ModelContext*		m_pObjectStack;
	uint						m_iObjectStackPos;

	Array<AmbientLight>	m_AmbientLights;

	uint						m_iStack;
	MeshItem*				m_pStack;
	uint						m_iFrameCount;
	Matrix34*				m_pMatrices;

	uint						m_iLastMatrix;

	InstancedMesh*	m_pInstancedMeshes;
	uint						m_iInstancedMeshOffset;
	uint						m_iMaxInstancedMeshes;

	uint						m_iCount;
	uint						m_iUpdateCameraMask;

	float						m_fTargetWidth;
	float						m_fTargetHeight;

	MeshItemSet			m_MeshItemSet[MAX_SORT_NUM];

	static const uint MAX_LIGHTS = 64;
	static const uint MAX_ENVMAPS = 64;

	MeshItemSet			m_ShadowSet[MAX_LIGHTS];
	MeshItemSet			m_AmbientDepthSet[MAX_SORT_NUM];

	MeshItemSet			m_LightSet[MAX_SORT_NUM][MAX_LIGHTS];
	MeshItemSet			m_EnvironmentSet[MAX_SORT_NUM][MAX_ENVMAPS];
	MeshItemSet			m_EnvironmentSet2[MAX_SORT_NUM];

	uint						m_IsSetUsed;

	VCache					m_StaticCache;
	VCache					m_DynamicCache;

	DLightsArray		m_pActiveDynamicLights; //in frame visible lights.

	Array<LightRef>	m_ActiveLights;			//in frame, all visible lights
	Array<MEnvMap*>	m_pActiveEnvMaps;			//in frame, all visible envmaps

	SArray<Mirror, 32>m_Mirrors;

	void GrowInstancedMeshes();

//!Implements allocation of MeshItem
	MeshItem* AllocMeshItem()
	{
		if(m_iStack >= m_Description.m_iMaxSurfaces)
			return NULL;

		return &m_pStack[m_iStack++];
	}


	ENF_INLINE ModelContext* BeginObjectInternal(uint allocIndex, Entity* ent, int distance, TModelLights* lights, uint flags, uint nummatrices);

	//!Destroy method.
	virtual ~Pipeline();

	//!Inserts visible environment map to the scene.
	uint InsertEnvMap(MEnvMap* envmap);

	virtual void OnEvent(const Event& event) {};
	virtual bool OnChangeSettings(TiXmlElement* xmlnode) {return true;};
};

//-----------------------------------------------------------------------

//@}
}
#endif //ENF_PIPELINE_H
