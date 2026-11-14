//-----------------------------------------------------------------------------
// File: vobject/enf_vobject.h
//
// Desc: Visual object superclass
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_VOBJECT_H
#define ENF_VOBJECT_H

#include "enf_config.h"
#include "common/enf_mathlib.h"
#include "common/enf_color.h"
#include "resource/enf_objectresource.h"

namespace enf
{
/** @defgroup VObjects Visual object implementations
@{*/

class Entity;
class Pipeline;
class VObject;
class ModelContext;
struct MEnvMap;
class Material;
class VertexBuffer;
class TraceContact;
struct BaseTrace;
struct VCacheEntry;
struct DecalBase;
class Decal;
struct ILight;
struct DynamicLight;
class VObjectInstance;
struct LightInteraction;
class IFFInput;
struct TVisibleEntity;
struct VisibleGroup;
class ObjectGeometry;
class RTTextureWidget;
class Texture;

//-----------------------------------------------------------------------------
/*!
temporally from BaseWorld
*/
//-----------------------------------------------------------------------------
struct TVisibleEntity
{
	Entity*	EntityPtr;
	ushort	ClipFlags;
	union
	{
		struct 
		{
			ubyte		SourceLOD;
			ubyte		TargetLOD;
		};
		ushort	LODs;
	};
	float		LODFade;
	int		Distance;

	TVisibleEntity(): EntityPtr(NULL), ClipFlags(0), LODs(0), LODFade(0.0f), Distance(0) {}
};


//-----------------------------------------------------------------------------
const uint VObjectInstanceID = 1000;

enum VObjectInstanceTypes
{
	VSpriteInstanceTypeID = VObjectInstanceID + 1,
	VObjectInstanceTypeID,
	MeshObjectInstanceTypeID,
	MeshObjectAnimatedInstanceTypeID,
	ParticleEffectInstanceTypeID,
	TerrainInstanceTypeID
};

typedef List<VObjectInstance> TVObjectInstanceList;
typedef ListIterator<VObjectInstance> TVObjectInstanceListIterator;

const uint NO_FORCE_LOD		= 0xff;

//-----------------------------------------------------------------------------
struct AnimMovement
{
	Vector3 Linear;
	Vector3 Angular;
	AnimMovement(): Linear(0.0f), Angular(0.0f) {}
};

//-----------------------------------------------------------------------------
//!VObjectInstance flags
enum VIFlags
{
	//!Don't filter too small objects
	VIF_NOFILTER						= BITF(0), 
	//!This instance do not cast shadows.
	VIF_NOSHADOW						= BITF(1),
	//!Do not render until all textures are loaded
	VIF_DONTRENDERUNTILLOADED		= BITF(2),
	//!This instance ignore light interactions.
	VIF_NOLIGHT							= BITF(3),
	//!Dynamic bounding box
	VIF_DYNAMICBBOX					= BITF(4),
	//!Prefers tracing to model, instead of physics geometry
	VIF_TRACE_MODEL					= BITF(5),
	//!Prefers tracing to model-bones, instead of physics geometry
	VIF_TRACE_BONES					= BITF(6),
	//!Simple box. AAB is big enough so it contain transforms. No need to compute AAB from OBB
	VIF_SIMPLEBBOX						= BITF(7),
	//!Receives wind emiters
	VIF_WINDRECEIVER					= BITF(8),
	//!Has user instance info
	VIF_USERINSTANCEINFO				= BITF(9),
	//!Internal flags follows, do not change directly!
	VIF_PRIVATE							= BITF(16),
	VIF_LIGHTINTERACTIONS_DIRTY	= BITF(17),
	VIF_SHADOWCASTER					= BITF(19),
	VIF_SHADOWRECEIVER				= BITF(20),
	VIF_NEED_ENVMAP					= BITF(21),
	VIF_ACCEPTS_LIGHT					= BITF(22),
	VIF_NEEDS_AMBIENT_LIGHT			= BITF(23),
	VIF_LOD_CROSSFADE					= BITF(24),
	//!This instance supports instancing
	VIF_INSTANCING						= BITF(25),

	VIF_MATERIAL_LOADED				= BITF(31)
};

//-----------------------------------------------------------------------------
class PUBLIC_API VObjectSkinDef
{
private:
	struct MaterialDef
	{
		CStr		Name;
		uint		CreationFlags;
		MaterialDef(): CreationFlags(0) {}
		MaterialDef(CStr name, uint cflags): Name(name.cstr()), CreationFlags(cflags) {}
	};

	Array<MaterialDef>	Defs;

public:
	ENF_DECLARE_SMALLOBJECT(VObjectSkinDef);

	bool operator==(const VObjectSkinDef& def) const;
	VObjectSkinDef& operator=(const VObjectSkinDef& def);

	ENF_INLINE uint GetNumMaterials() const
	{
		return Defs.GetCardinality();
	}

	ENF_INLINE uint GetMaterialFlags(uint n) const
	{
		return Defs[n].CreationFlags;
	}

	ENF_INLINE CStr GetMaterialName(uint n) const
	{
		return Defs[n].Name.cstr();
	}

	uint AddMaterial(CStr name, uint creationflags);

	ebool RemapMaterial(CStr matname, CStr newname);

	VObjectSkinDef() {}
	~VObjectSkinDef() {}
};

//-----------------------------------------------------------------------------
class PUBLIC_API VObjectSkin: public VObjectSkinDef, public RefCountObject
{
protected:
	friend class VObject;

	CArray<Material*>	m_Materials;
	VObject*				m_pObject;
	uint					m_iSkinIndex;
	uint					m_iLoadCount;

	void ReleaseMaterials();
public:
	ENF_DECLARE_SMALLOBJECT(VObjectSkin);

	//because of hack in LoadMeshObjectResource :(
	void SetObject(VObject* obj)
	{
		m_pObject = obj;
	}
	ENF_INLINE Material*const* GetMaterials() const
	{
		return m_Materials.GetArray();
	}
	ENF_INLINE Material* GetMaterial(uint n) const
	{
		return m_Materials[n];
	}
	bool CreateMaterials(CStr ownerName);
	uint Load();
	uint Unload();
	bool IsLoaded() const { return m_iLoadCount > 0; }
	VObjectSkin(): VObjectSkinDef(), m_pObject(NULL), m_iSkinIndex(INDEX_NOT_FOUND), m_iLoadCount(0) {}
	VObjectSkin(VObject* obj): VObjectSkinDef(), m_pObject(obj), m_iSkinIndex(INDEX_NOT_FOUND), m_iLoadCount(0) {}
	VObjectSkin(const VObjectSkinDef& def): VObjectSkinDef(def), m_pObject(NULL), m_iSkinIndex(INDEX_NOT_FOUND), m_iLoadCount(0) {}
	~VObjectSkin();
};

const uint CLIP_DRAWSHADOW = 0x8000;
const uint CLIP_DRAWMODEL = 0x4000;
const uint CLIP_DRAW_MASK = 0xc000;
const ushort NO_SKIN_INDEX = -1;

//-----------------------------------------------------------------------------
/*!
Instance class. Representing one instance of shared model. It's in
1:1 relation with entity.
*/
//-----------------------------------------------------------------------------
class PUBLIC_API VObjectInstance: public BaseItem
{
protected:
	friend class VObject;
	//friend class Entity;

	VObject*					m_pObject;
	
	uint						m_iFlags;
	uint						m_iCameraMask;
	ushort					m_iSkin;
	ubyte						m_iForceLOD;
	ubyte						m_iCameraIndex;

	//TODO: move this to ModelContext when there will be some persistency
	ushort					m_iLODTime;			///<normalized time in range 0...65535 used for lerp
	ushort					m_iLastLODChange;	///<last LOD change timestamp in milliseconds
	ubyte						m_iCurrentLOD;		///<current LOD
	ubyte						m_iTargetLOD;		///<target LOD
	ushort					m_iPad0;

	//!List of all light interactions
	LightInteraction*		m_pInteractions;
	
	//! dynamic lightning.
	uint						m_iDLights;
	uint						m_iDLightFrame;

	//!Env-map interaction
	MEnvMap*					m_pEnvMap;

	friend class List<VObjectInstance>;
	friend class ListIterator<VObjectInstance>;

	VObjectInstance*		ENF_LIST_PREV;
	VObjectInstance*		ENF_LIST_NEXT;

	virtual bool Init()
	{
		return true;
	}

	virtual void Unload()
	{
	}

	void UpdateFlags();
	~VObjectInstance();

public:
	ENF_DECLARE_ITEM(VObjectInstance, BaseItem);
	ENF_DECLARE_SMALLOBJECT(VObjectInstance);

	virtual void Serialize(SerializeInputAdapter* adapter);
	virtual void Serialize(SerializeOutputAdapter* adapter);

	//!Empty constructor for de-serialization.
	VObjectInstance():
		m_pObject(NULL),
		m_iFlags(0),
		m_iCameraMask(0xffffffff),
		m_iSkin(NO_SKIN_INDEX),
		m_iForceLOD(NO_FORCE_LOD),
		m_iCameraIndex(0),
		m_pInteractions(NULL),
		m_iDLights(0),
		m_iDLightFrame(0),
		m_pEnvMap(NULL)
	{
	}

	VObjectInstance(VObject* obj, Entity* owner);
	virtual void Destroy()
	{
		delete this;
	}

	virtual void SetVisible(bool visible) {}
	/*!
	Test if one or more of specified flags are set
	\param flags
	One or more flags. See: enf::VIFlags
	\return
	True if is set, false otherwise.
	*/
	bool IsFlagSet(uint flags) const
	{
		return ((m_iFlags & flags) != 0);
	}

	//!Returns all flags
	uint GetFlags() const
	{
		return m_iFlags;
	}

	/*!
	Sets one or more of specified flags
	\param flags
	One or more flags to be set. See enf::VIFlags
	\return
	Current state of bitmask. See enf::VIFlags
	*/
	uint SetFlags(uint flags)
	{
		m_iFlags |= flags;
		return m_iFlags;
	}

	/*!
	Clears one or more of specified flags
	\param flags
	One or more flags to be cleared. See: enf::VIFlags
	\return
	These bits that were set before and now are cleared.
	*/
	uint ClearFlags(uint flags)
	{
		uint prev = m_iFlags & flags;
		m_iFlags &= ~flags;
		return prev;
	}

	//for animated objects. (MeshObject)
	virtual void Update(Entity* ent)
	{
	}

	//!Returns index of current skin
	ENF_INLINE uint GetSkinIndex() const
	{
		return m_iSkin;
	}

	/*!
	You can lock certain LOD
	\param lod
	Number of LOD to be forced or NO_FORCE_LOD (default)
	*/
	void SetForceLOD(uint lod)
	{
		m_iForceLOD = lod;
	}

	ENF_INLINE uint GetForceLOD() const {return m_iForceLOD;}

	void SetLOD(uint lod, TVisibleEntity& vent, uint timeStamp);
	void SetLOD(TVisibleEntity& vent, uint timeStamp);
	
	ENF_INLINE uint GetLOD() const
	{
		return m_iTargetLOD;
	}

	ENF_INLINE uint GetCurrentLOD() const
	{
		return m_iCurrentLOD;
	}

	ENF_INLINE uint GetCameraIndex() const
	{
		return m_iCameraIndex;
	}

	/*!
	Asks whether is this instance visible in certain camera view
	\param camera
	camera index.
	\return
	True when visible, false otherwise.
	*/
	ENF_INLINE bool IsVisibleInCamera(uint camera) const
	{
		uint mask = 1 << camera;
		return ((m_iCameraMask & mask) != 0);
	}

	/*!
	Gets attached environment map. Usually set by Pipeline::PushModel if model
	needs cubemap.
	\return 
	Pointer to cubic texture.
	*/
	ENF_INLINE MEnvMap* GetEnvMap() const
	{
		return m_pEnvMap;
	}

	ENF_INLINE void SetEnvMap(MEnvMap* env)
	{
		m_pEnvMap = env;
	}
	virtual Texture* GetRTTexture() const { return NULL; }
	virtual void SetRTTexture(RTTextureWidget* rtwidget) {}

	ENF_INLINE const Material* GetMaterial(uint matid) const;
	ENF_INLINE Material* GetMaterial(uint matid);

	/*!
	Should be called after you change some Material used by this model
	ATM it's used from material editor, to force update instance
	flags after external Material changes
	*/
	void UpdateMaterials()
	{
		UpdateFlags();
	}

	/*!
	Asks whether this model needs cube-maps. Usually set by VObject because he knows
	if he has materials which needs cube-maps.
	\return
	True, when instance needs cube-maps, False otherwise.
	*/
	ENF_INLINE bool NeedEnvMap() const
	{
		return IsFlagSet(VIF_NEED_ENVMAP);
	}

	ENF_INLINE bool AcceptsLight() const
	{
		return (m_iFlags & (VIF_ACCEPTS_LIGHT|VIF_NOLIGHT)) == VIF_ACCEPTS_LIGHT;
	}

	ENF_INLINE bool NeedsAmbientLight() const
	{
		return (m_iFlags & (VIF_NEEDS_AMBIENT_LIGHT|VIF_NOLIGHT)) == VIF_NEEDS_AMBIENT_LIGHT;
	}

	ENF_INLINE bool IsShadowCaster() const
	{
		return IsFlagSet(VIF_SHADOWCASTER);
	}

	ENF_INLINE bool IsShadowReceiver() const
	{
		return IsFlagSet(VIF_SHADOWRECEIVER);
	}

	/*!
	Returns true if this instance has some decals on
	*/
	virtual ebool HasDecals() const
	{
		return false;
	}

	/** \name Light interaction
	Interactions with lights
	*/
	//@{
	ENF_INLINE void MakeInteractionsDirty()
	{
		SetFlags(VIF_LIGHTINTERACTIONS_DIRTY);
		SetEnvMap(NULL);
	}

	ENF_INLINE const LightInteraction* GetInteractions() const
	{
		return m_pInteractions;
	}
	void AddInteraction(LightInteraction* li);
	ENF_INLINE ebool InteractionsAreDirty() const
	{
		return IsFlagSet(VIF_LIGHTINTERACTIONS_DIRTY);
	}

	void RemoveAllInteractions(bool dirty = false);
	//@}


	/*!
	Returns pointer to owner object
	*/
	ENF_INLINE VObject* GetVObject() const
	{
		return m_pObject;
	}

	//!Returns actual estimated area
	ENF_INLINE float GetEstimateArea() const;

	/*!
	Returns per instance world bound box, containing transformed object.
	It is provided by instance, because there are objects that can provide
	real dynamic boundbox (particles, skeletal objects) and others that
	cannot. Depends on implementation.
	\param mat
	World transformation (enf::Entity::IGetMatrix())
	\param flags
	Flags affecting bounding box computation, see enf::VIFlags
	\return
	Bounding box in world coordinates
	*/
	virtual AAB ComputeWorldBoundBox(const Matrix43& mat, uint flags) const;
	ENF_INLINE AAB ComputeWorldBoundBox(const Matrix43& mat) const
	{
		return ComputeWorldBoundBox(mat, m_iFlags);
	}

	/*!
	Returns per instance local bound box, containing transformed object.
	It is provided by instance, because there are objects that can provide
	real dynamic boundbox (particles, skeletal objects) and others that
	cannot. Depends on implementation.
	\param mat
	World transformation (enf::Entity::IGetMatrix())
	\param flags
	Flags affecting bounding box computation, see enf::VIFlags
	\return
	Bounding box in local coordinates
	*/
	virtual AAB ComputeLocalBoundBox(const Matrix43& mat, uint flags) const;
	ENF_INLINE AAB ComputeLocalBoundBox(const Matrix43& mat) const
	{
		return ComputeLocalBoundBox(mat, m_iFlags);
	}

	uint SetSkin(const VObjectSkinDef& skin);
	uint SetSkin(uint skinIndex);

	void SetRenderView(Entity* ent, uint camindex, uint width, uint height);
	uint GetRenderView() const
	{
		return m_iCameraIndex;
	}

	/*!
	Changes bit mask of cameras, where is instance visible. There
	is 32 cameras ATM. Set bits represent cameras, where is instance
	visible.
	\param mask
	The mask of cameras where we are visible
	\return
	Previous mask
	*/
	uint SetCameraMask(uint mask);
	ENF_INLINE uint GetCameraMask() const
	{
		return m_iCameraMask;
	}

	/*!
	Changes bit mask of light interactions.
	\param mask
	The mask of bits which are tested against lights
	\return
	Previous mask
	*/
	uint SetLightMask(uint mask);

	ENF_INLINE bool IsDLightSet(uint framecount, uint dlmask) const
	{
		return (m_iDLightFrame == framecount && (m_iDLights & dlmask));
	}

	ENF_INLINE void SetDLight(uint framecount, uint dlmask)
	{
		if(m_iDLightFrame == framecount)
			m_iDLights |= dlmask;
		else
		{
			m_iDLights = dlmask;
			m_iDLightFrame = framecount;
		}
	}

	ENF_INLINE uint GetDLights(uint framecount) const
	{
		return (m_iDLightFrame == framecount) ? m_iDLights : 0;
	}

	/*!
	Updates animation of object.
	\param ent
	The attached entity
	\param time
	Time-slice of the frame
	\param flags
	Various flags. See implementation of VObject::UpdateAnim of each object type
	\param movement
	Optional structure, which holds result of angular and linear movement during
	animation
	\return
	Various results. See implementation of VObject::UpdateAnim of each object type
	*/
	ENF_INLINE int UpdateAnim(Entity* ent, float time, uint flags, AnimMovement* movement = NULL);
};

//-----------------------------------------------------------------------------
/*!
Helper structure for billboarded meshes passed to IMesh::m_pUserData
*/
//-----------------------------------------------------------------------------
struct MeshBillboard
{
	ENF_DECLARE_SMALLOBJECT(MeshBillboard);
	Matrix43	m_Matrix; ///<Plane normal is X axis
	float		m_fPlaneDistance;
};

//---------------------------------------------------------------------------
class PUBLIC_API VObject: public enf::ObjectResource
{
protected:
	friend class VObjectInstance;
	friend class IResourceManager;

	AAB					m_BBox;
	float				m_fEstimateArea;
	float				m_fRadius, m_fCRadius;
	int					m_iMaxDistance;

  TVObjectInstanceList  m_Instances;
	typedef float TLodFactors[6];

	TLodFactors		m_fLODFactors; //temporary, do it better
	uint					m_iNumLods;
	ushort				m_iMinLOD;
	ushort				m_iMaxLOD;
	static uint		m_iMinLoadedLOD;		///<Level of detail ;)

	/*!
	Method responsible for complete initialization of VObject variables
	*/
	virtual void Init();

	VObject(): ObjectResource(), m_BBox(true), m_fEstimateArea(0.0f), m_fRadius(0.0f), m_iMaxDistance(0x7fffffff), m_iNumLods(0), m_iMinLOD(0), m_iMaxLOD(0)
	{
	}

	void AreaFromBoundingBox();

	virtual int UpdateAnim(Entity* ent, VObjectInstance* vi, float time, uint flags, AnimMovement* movement)
	{
		if(movement)
		{
			movement->Angular.Zero();
			movement->Linear.Zero();
		}
      return 1;
	}

	Array<VObjectSkin*>	m_Skins;
	
	virtual uint AddSkin(VObjectSkin* skin);
	uint FindSkin(const VObjectSkinDef& skin) const;

friend VObjectSkin::~VObjectSkin();
	void RemoveSkin(VObjectSkin* skin);

public:
	ENF_DECLARE_ITEM(VObject, ObjectResource);

	void Swap(VObject& other)
	{
		enf_swap(m_BBox, other.m_BBox);
		enf_swap(m_fEstimateArea, other.m_fEstimateArea);
		enf_swap(m_fRadius, other.m_fRadius);
		enf_swap(m_fCRadius, other.m_fCRadius);
		enf_swap(m_iMaxDistance, other.m_iMaxDistance);
		enf_swap(m_Instances, other.m_Instances);
		enf_swap(m_Skins, other.m_Skins);
	}


	enum ClipResult {CR_UNSUPPORTED, CR_CLIPPED, CR_PASSED};

	virtual ObjectGeometry* GetGeometry() const
	{
		return NULL;
	}

	/*!
	Returns number of Level Of Details
	\return
	Levels Of Detail
	*/
	uint GetNumLODs() const
	{
		return m_iNumLods;
	}

	ENF_INLINE uint GetMinLoadedLOD() const
	{
		return m_iMinLOD;
	}
	ENF_INLINE uint GetMaxLoadedLOD() const
	{
		return m_iMaxLOD;
	}

	float GetLODFactor(uint lod) const
	{
		enf_assert(lod < m_iNumLods);
		return m_fLODFactors[lod];
	}

	void SetLODFactor(uint lod, float factor)
	{
		enf_assert(lod < m_iNumLods);
		m_fLODFactors[lod] = factor;
	}
	uint CalculateLOD(float aspect) const;


	void SetMaxDistance(int maxdistance);
	ENF_INLINE const VObjectSkin* GetSkin(uint skin) const { return m_Skins[skin]; }
	ENF_INLINE VObjectSkin* GetSkin(uint skin) { return m_Skins[skin]; }
	const VObjectSkinDef* GetDefaultSkin() const;

	void SetBoundingBox(const AAB& bbox, float radius)
	{
		m_BBox = bbox;
		m_fCRadius = m_fRadius = radius;
	}

	ENF_INLINE const AAB& GetBoundingBox() const
	{
		return m_BBox;
	}

	//!Returns original bounding box, if implementation supports both
	virtual const AAB& GetOriginalBoundingBox() const
	{
		return m_BBox;
	}

	ENF_INLINE float GetBoundingSphere() const
	{
		return m_fRadius;
	}

	ENF_INLINE float GetCBoundingSphere() const
	{
		return m_fCRadius;
	}

	ENF_INLINE int GetMaxDistance() const
	{
		return m_iMaxDistance;
	}

	ENF_INLINE float GetEstimateArea() const
	{
		return m_fEstimateArea;
	}

	virtual VObjectInstance* AllocInstance(Entity* ent, CStr options = NULL)
	{
		VObjectInstance* mi = ENF_NEW VObjectInstance(this, ent);
		return mi;
	}

	virtual bool MarkDLight(Entity* ent, DynamicLight* dl, uint framecount, uint tracecount)
	{
		return true;
	}

	/*!
	Method responsible for rendering of VObject.
	\param vent
	Rendered visible entity
	\param pipeline
	Rendering pipeline
	\return
	True when object was rendered
	*/
   virtual bool Render(const TVisibleEntity& vent, Pipeline* pipeline)
	{
		return true;
	}

#ifdef ENF_DONE
	virtual bool RenderGroup(const TVisibleEntity* vent, uint from, const VisibleGroup& group, Pipeline* pipeline);
#endif
	virtual bool MultiplyPivot(const Entity* ent, uint n, Matrix43& p) const
	{
		return false;
	}

	virtual ClipResult ClipMovement(Entity* ent, BaseTrace* trace)
	{
		return CR_UNSUPPORTED;
	}

	virtual ClipResult ClipBox(Entity* ent, BaseTrace* trace)
	{
		return CR_UNSUPPORTED;
	}

	virtual bool TraceLine(const Entity* ent, const Vector3& from, const Vector3& to, TraceContact* contact)
	{
		return false;
	}
	virtual void DrawOccluders(Entity* ent)
	{
	}

	virtual float TraceOccluders(Entity* ent, const Vector3& from, const Vector3& to, float traceLength)
	{
		return 0.0f;
	}

	//TODO: this is CRAP!
	virtual bool LoadTriangleMeshGeometryFromMaterial(uint materialindex, void* geom)
	{
		return true;
	}

	/*!
	Returns number of material used by this VObject
	\return
	Number of materials
	*/
	virtual uint GetNumMaterials() const { return m_Skins.IsEmpty() ? 0 : m_Skins[0]->GetNumMaterials(); }

	/*!
	Returns material used by this VObject
	\param skin
	Index of skin (0 is default)
	\param material
	Index of material
	\return
	Pointer to Material
	*/
	virtual const Material* GetMaterial(uint skin, uint material) const { return m_Skins.IsEmpty() ? 0 : m_Skins[skin]->GetMaterial(material); }
	virtual Material* GetMaterial(uint skin, uint material) { return m_Skins.IsEmpty() ? 0 : m_Skins[skin]->GetMaterial(material); }
	/*!
	Returns material from default skin used by this VObject
	\param material
	Index of material
	\return
	Pointer to Material
	*/
	virtual const Material* GetMaterial(uint material) const { return m_Skins.IsEmpty() ? 0 : m_Skins[0]->GetMaterial(material); }
	virtual Material* GetMaterial(uint material) { return m_Skins.IsEmpty() ? 0 : m_Skins[0]->GetMaterial(material); }

	//TODO: make it better?
	virtual const ushort* GetShadowVolumeAdjacency(void* mesh) const
	{
		return NULL;
	}
	virtual const ushort* GetShadowIndices(void* mesh) const
	{
		return NULL;
	}

	//!Tells whether the implementation supports decals
	virtual ebool DecalsSupport() const
	{
		return false;
	}

	virtual Decal* AddDecal(DecalBase& base, Entity* ent)
	{
		return NULL;
	}

	virtual bool Reload(CInputStream* instream = NULL);
};

ENF_INLINE int VObjectInstance::UpdateAnim(Entity* ent, float time, uint flags, AnimMovement* movement)
{
	return m_pObject->UpdateAnim(ent, this, time, flags, movement);
}

ENF_INLINE const Material* VObjectInstance::GetMaterial(uint matid) const
{
	return m_pObject->GetMaterial(m_iSkin, matid);
}
ENF_INLINE Material* VObjectInstance::GetMaterial(uint matid)
{
	return m_pObject->GetMaterial(m_iSkin, matid);
}

ENF_INLINE float VObjectInstance::GetEstimateArea() const
{
	return m_pObject->GetEstimateArea();
}

//@}
}
#endif //ENF_VOBJECT_H