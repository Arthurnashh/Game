//-----------------------------------------------------------------------------
// File: enf_material.h
//
// Desc: material definition
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_MATERIAL_H
#define ENF_MATERIAL_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_mathlib.h"
#include "common/enf_color.h"
#include "enf_materialvar.h"
#include "resource/enf_surfaceparams.h"

namespace enf
{
/** @addtogroup MaterialSystem
@{*/

enum DataVarType;

class Material;

class MaterialClass;
class MaterialSource;
class MaterialSystem;
class IMaterialSystem;

class Variable;
class IVariable;
class Pipeline;
class IMesh;
class ModelContext;
class Texture;
class Instance;

const uint MAX_MATERIAL_VARIABLES = 64;

//! Type of sort group
enum SortType
{
	//!Solid materials, writing to zbuffer
	SORT_OPAQUE = 0,
	//!Solid materials, rendered after shadow passes
	SORT_OPAQUENOSELFSHADOW,
	//!Static decal geometry (paintings, signs etc)
	SORT_MARK,
	//!Dynamic decal geometry (blood spurts etc)
	SORT_DECAL,
	//!Translucent geometry, not writing to zbuffer
	SORT_TRANSLUCENT,
	//!Near effects (blood in eyes etc)
	SORT_NEAR,
	//!Rendered after all other geometry. Used for refract effects using current rendered view
	SORT_OVERLAY,
	//!Rendered after clearing zbuffer. Used for 1st person objects
	SORT_OVERALL,
	SORT_COUNT,		//just sizeof constant

	//!for sorting of special types of geometry which depends on correct order (water, grass, sky...)
	SORT_SPECIAL = SORT_DECAL
};

//!Pass type.
enum PassType
{
	/*!
	No rendering to frame-buffer at all. It just writes depth to depth-buffer.
	Used for optimization (optionally) and for shadow mapping
	*/
	PT_DEPTH = 1,

	//! Almost the same as PT_DEPTH, but there are exceptions. I.E. plants and dissolve effect
	PT_SHADOW,

	//! Almost the same as PT_DEPTH, but there are exceptions. I.E. plants and dissolve effect
	PT_SHADOW_OMNIDIRECTIONAL,

	//! Basic ambient lighting, or light-map lighting
	PT_AMBIENT,

	//! The same as PT_AMBIENT, but no need to write depth, as it's done by PT_DEPTH
	PT_AMBIENTDEPTH,

	//! Complex light with diffuse and specular part.
	PT_DIFFUSE_SPECULAR,

	//! Environment reflection. Reflected image from static or dynamic cube-maps
	PT_ENVIRONMENT
};

//!Runtime material flags. Set directly by material compiler or by MaterialClass::OnMaterialFinalize
enum MaterialFlags
{
	//!Material is casting shadows
	MF_CASTSHADOW			=	BITF(0),

	//!Material is receiving shadows
	MF_RECEIVESHADOW		=	BITF(1),

	//!Material is decal and needs special handling
	MF_DECAL					=	BITF(2),
	
	/*!
	When this material needs current render view, it can ask for actual frame
	whenever new geometry is rendered. Used for correct handling overlapped
	refract effects.
	*/
	MF_ALWAYSCOPY			=	BITF(3),

	//!Material is mirroring. Scene render is instructed to create mirrors from these geometries.
	MF_MIRROR				=	BITF(4),

	//!Material needs lightmap. Must be supplied in IMesh.
	MF_NEEDSLIGHTMAP		=	BITF(6),

	//!Material needs external rendertarget. ID of camera is supplied by VObjectInstance
	MF_NEEDSRENDERVIEW	=	BITF(8),

	//!Material needs external RTTextureWidget. Must be supplied by VObjectInstance.
	MF_NEEDSGUI				=	BITF(9),

	//!Material needs dynamic or precomputed cubemap
	MF_NEEDSENVMAP			=	BITF(10),

	//!Material needs to prepare texture transformation matrix
	MF_NEEDSTMAT			=	BITF(11),

	//!Material needs software vertex processing
	MF_NEEDSVCMOD			=	BITF(12),

	//!Material needs to be sorted after shadow pass
	MF_NOSELFSHADOW		=	BITF(13),

	//!Material accepts reflections from cubemaps
	MF_ACCEPTREFLECTION	=	BITF(14),

	//!Material accepts specular lights
	MF_ACCEPTSPECULAR		=	BITF(15),

	//!Material is one-sided
	MF_CULL					=	BITF(16),

	//!Material writes to z-buffer
	MF_ZWRITE				=	BITF(17),

	//!Material compares with z-buffer
	MF_ZENABLE				=	BITF(18),

	//!Material clips-out pixels with alpha value above this value
	MF_ALPHATEST			=	BITF(19),

	/*!
	Material is using static cube-map. Tells that even when
	there is not dynamic cube-map, we still accepts
	environment pass
	*/
	MF_HASSTATICENVMAP	=	BITF(20),

	/*!
	Set by MaterialClass::OnMaterialFinalize, when
	MaterialClass::GetMaxInstances() > 1 and there are not
	extra per-instance parameters.
	*/
	MF_INSTANCING			= BITF(21),

	//!Material needs MeshBillboard in IMesh::m_pUserData
	MF_NEEDSBILLBOARD		= BITF(22),

	//!Material support crossfading of LODs
	MF_CANCROSSFADE		= BITF(23),

	//!Internal!
	MF_FINALIZED			= BITF(30),
	MF_TEXLOADED			= BITF(31)
};


//-----------------------------------------------------------------------
/*!
Material class is 'instance' representation, filled by data
whereas enf::MaterialClass is 'class' of the enf::Material
*/
class PUBLIC_API Material: public RefCountObject
{
public:
	ENF_DECLARE_SMALLOBJECT(Material);
	enum CFuncType
	{
		VCMOD_NONE,
		VCMOD_SCALE,
		VCMOD_SIN,
		VCMOD_XYSIN,
		VCMOD_CLOTH,
		VCMOD_LIQUID,

		TCMOD_SCALE,
		TCMOD_SCALE_NO,
		TCMOD_SHIFT,
		TCMOD_ROTATE,

		TCMOD_SCALE2,
		TCMOD_SCALE_NO2,
		TCMOD_SHIFT2,
		TCMOD_ROTATE2
	};

	struct CFunc
	{
		static const uint MAX_PARMS = 4;

		CFuncType	m_Type;
		uint			m_iTC;
		MaterialVarScalar*	m_Parms[MAX_PARMS];
		float GetScalar(uint var, const Material* mat) const;
		CFunc();
	};


	const CFunc& GetDeformFunc(uint n) const
	{
		enf_assert(n < m_iNumDeformFuncs);
		return m_pDeformFuncs[n];
	}
	uint GetNumDeformFuncs() const
	{
		return m_iNumDeformFuncs;
	}
	/*!
	Constructs texture coordinate transformation matrix
	according to defined tcmod deform functions, UV animation etc.
	\param mres
	Resulting float4x2[2] matrix
	\return
	True when functions were applied. False otherwise
	*/
	ENF_INLINE bool GetTCMatrix(Vector4* mres) const
	{
		if(m_iNumDeformFuncs == 0)
			return false;

		_GetTCMatrix(mres);
		return true;
	}

	/*!
	Sets MaterialClass used for rendering
	\param mclass
	MaterialClass
	\return
	True when MaterialClass is successfully set, false otherwise
	*/
	bool SetMaterialClass(MaterialClass* mclass);

	bool ChangeMaterialClass(MaterialClass* mclass);

	/*!
	Gets defined sort type.
	\return
	Defined enf::SortType
	*/
	ENF_INLINE SortType GetSortType() const
	{
		return m_SortType;
	}

	ENF_INLINE void SetSortType(SortType st)
	{
		m_SortType = st;
	}

	/*!
	Gets unique identifier of this material. MaterialSystem is responsible
	for assigning reasonable values that most common materials are in sequence
	\return
	Unique sorting number
	*/
	ENF_INLINE uint GetSortID() const
	{
		return m_iSortID;
	}

	/*!
	Gets small sorting bias. Used to distinguish between two materials from
	the same group.
	\return
	Small sort bias
	*/
	ENF_INLINE int GetSortBias() const
	{
		return m_iSortBias;
	}

	ENF_INLINE void SetSortBias(int bias)
	{
		m_iSortBias = enf_limit(bias, -16, 15);
	}

	/*!
	Gets optional distance which limits when is material rendered
	\return
	Maximum distance
	*/
	ENF_INLINE int GetMaxDistance() const
	{
		return m_iMaxDistance;
	}

	ENF_INLINE void SetMaxDistance(int maxdist)
	{
		m_iMaxDistance = maxdist;
	}

	/*!
	Must be called after enf::Material is filled with data. Internally it
	calls enf::MaterialClass::OnFinalize() which validates&corrects the data.
	*/
	void Finalize();

	/*!
	Gets pointer to enf::MaterialClass responsible for rendering
	this material instance
	\return
	Pointer to MaterialClass or NULL
	*/
	ENF_INLINE MaterialClass* GetMaterialClass() const
	{
		return m_pMClass;
	}

	/*!
	Gets number of variables. It's the same count as enf::MaterialClass::GetNumVariables()
	\return
	Number of variables
	*/
	ENF_INLINE uint GetNumVariables() const
	{
		return m_iNumVariables;
	}

	/*!
	Tells whether is this variable set. If isn't it must not be asked for value!!!
	\return 
	True, when variable is set, false otherwise.
	*/
	ENF_INLINE bool IsVariableSet(uint variable) const
	{
		enf_assert(variable < m_iNumVariables);
		return (m_pVariables[variable] != NULL);
	}

	void UnsetVariable(uint variable);

	//!Is the variable scripted? Thus can have any value in the future
	bool IsVariableFromScript(uint variable) const
	{
		enf_assert(variable < m_iNumVariables);
		return (m_pVariables[variable] != NULL && m_pVariables[variable]->Var);
	}


	/*!
	Gets variable as enf::MaterialVar type.
	\param variable
	Index of variable
	\return
	Pointer to variable, or NULL
	*/
	ENF_INLINE MaterialVar* GetVariable(uint variable) const
	{
		return m_pVariables[variable];
	}

	/*!
	Gets variable as TextureMap type.
	\param variable
	Index of variable
	\param pipeline
	Pipeline we are rendering from. We are getting current envmaps there
	\param extra
	When it is special texture-map (cube-environment, lightmap etc. here is passed pointer to current value
	\return
	Value of variable
	*/
	const TextureMap*	GetTextureMap(uint variable, const Pipeline* pipeline, const void* extra = NULL) const;
	
	//!Touches texture and tells whether is texture physically loaded
	bool TouchTexture(uint variable) const;

	/*!
	Gets variable as MaterialVarTexture type.
	\param variable
	Index of variable
	\return
	Pointer to variable
	*/
	MaterialVarTexture* GetTexture(uint variable) const
	{
		enf_assert(IsType(variable, DVT_TEXTURE));
		return (MaterialVarTexture*)GetVariable(variable);
	}

	/*!
	Gets variable as float type.
	\param variable
	Index of variable
	\return
	Value of variable
	*/
	ENF_INLINE float GetScalar(uint variable) const
	{
		enf_assert(IsType(variable, DVT_SCALAR));
		enf_assert(IsVariableSet(variable));

		MaterialVarScalar* mv = (MaterialVarScalar*)m_pVariables[variable];

		if(mv->Var)
			return _GetScalar(mv->Var, mv->Value);

		return mv->Value;
	}

	/*!
	Gets variable as float type. Accepts only constant, not reference
	\param variable
	Index of variable
	\return
	Value of variable
	*/
	ENF_INLINE float GetScalarConst(uint variable) const
	{
		enf_assert(IsType(variable, DVT_SCALAR));
		enf_assert(IsVariableSet(variable));
		return ((const MaterialVarScalar*)m_pVariables[variable])->Value;
	}


	/*!
	Gets variable as float type.
	\param variable Index of variable
	\param defvalue The returned value when variable not set
	\return Value of variable or default value
	*/
	float GetScalarDef(uint variable, float defvalue) const
	{
		enf_assert(IsType(variable, DVT_SCALAR));
		if(!IsVariableSet(variable)) return defvalue;

		const MaterialVarScalar* mv = (const MaterialVarScalar*)m_pVariables[variable];

		if(mv->Var)
			return _GetScalar(mv->Var, mv->Value);

		return mv->Value;
	}

	/*!
	Gets variable as color type.
	\param variable Index of variable
	\param defvalue The returned value when variable not set
	\return Value of variable or default value
	*/
	FColor GetColorDef(uint variable, const FColor& defvalue) const
	{
		enf_assert(IsType(variable, DVT_COLOR));
		if(!IsVariableSet(variable)) return defvalue;

		const MaterialVarColor* mv = (const MaterialVarColor*)m_pVariables[variable];

		if(mv->Var)
			return _GetColor(mv->Var, mv->Value);

		return mv->Value;
	}

	/*!
	Gets variable as string type.
	\param variable
	Index of variable
	\return
	Value of variable
	*/
	CStr GetString(uint variable) const;

	/*!
	Gets variable as int type.
	\param variable
	Index of variable
	\return
	Value of variable
	*/
	int GetInteger(uint variable) const
	{
		enf_assert(IsType(variable, DVT_INTEGER));
		enf_assert(IsVariableSet(variable));

		const MaterialVarInteger* mv = (const MaterialVarInteger*)m_pVariables[variable];

		if(mv->Var)
			return _GetInteger(mv->Var, mv->Value);

		return mv->Value;
	}

	/*!
	Gets variable as int type. Accepts only constant, not reference
	\param variable
	Index of variable
	\return
	Value of variable
	*/
	int GetIntegerConst(uint variable) const
	{
		enf_assert(IsType(variable, DVT_INTEGER));
		enf_assert(IsVariableSet(variable));

		return ((const MaterialVarInteger*)m_pVariables[variable])->Value;
	}

	/*!
	Gets variable as Vector4 type.
	\param variable
	Index of variable
	\return
	Value of variable
	*/
	Vector4 GetVector(uint variable) const;

	/*!
	Gets variable as FColor type.
	\param variable
	Index of variable
	\return
	Value of variable
	*/
	FColor GetColor(uint variable) const
	{
		enf_assert(IsType(variable, DVT_COLOR));
		enf_assert(IsVariableSet(variable));

		const MaterialVarColor* mv = (const MaterialVarColor*)m_pVariables[variable];

		if(mv->Var)
			return _GetColor(mv->Var, mv->Value);

		return mv->Value;
	}

	/*!
	Gets variable as Matrix44 type.
	\param variable
	Index of variable
	\return
	Value of variable
	*/
	Matrix44 GetMatrix(uint variable) const;

	/*!
	Gets variable as bool type.
	\param variable
	Index of variable
	\return
	Value of variable
	*/
	ebool GetBoolean(uint variable) const;

	void* GetPointer(uint variable) const;

	//!Deletes variable value
	void DeleteVariable(uint variable);

	/*!
	Sets variable of any type to enf::Variable reference
	\param variable
	Index of variable
	\param type
	Type of variable
	\param var
	::Variable reference
	\return
	Pointer to initialized variable
	*/
	MaterialVar* SetVariable(uint variable, DataVarType type, const Variable* var);

	/*!
	Sets variable of texture type. Used for array of texture maps
	\param variable
	Index of variable
	\param names
	Array of texture map names
	\param num
	Number of textures in array
	\param flags
	Various flags. See enf::TextureMapFlags
	\return
	Pointer to initialized variable
	*/
	MaterialVarTexture* SetTexture(uint variable, const char **names, uint num, uint flags);

	/*!
	Sets variable of texture type. Used for one texture map
	\param variable
	Index of variable
	\param name
	Name of texture map
	\param flags
	Various flags. See enf::TextureMapFlags
	\return
	Pointer to initialized variable
	*/
	MaterialVarTexture* SetTexture(uint variable, const char* name, uint flags)
	{
		enf_assert(IsType(variable, DVT_TEXTURE));
		return SetTexture(variable, &name, 1, flags);
	}

	/*!
	Sets variable of texture type. Used for one texture map
	\param variable
	Index of variable
	\param texture
	Pointer to texture map
	\return
	Pointer to initialized variable
	*/
	MaterialVarTexture* SetTexture(uint variable, Texture* texture);

	/*!
	Sets variable of float type to constant value.
	\param variable
	Index of variable
	\param value
	Constant value
	\return
	Pointer to initialized variable
	*/
	MaterialVarScalar* SetScalar(uint variable, float value);

	/*!
	Sets variable of string type to constant value.
	\param variable
	Index of variable
	\param value
	Constant value
	\return
	Pointer to initialized variable
	*/
	MaterialVarString* SetString(uint variable, CStr value);

	/*!
	Sets variable of Matrix44 type to constant value.
	\param variable
	Index of variable
	\param value
	Constant value
	\return
	Pointer to initialized variable
	*/
	MaterialVarMatrix* SetMatrix(uint variable, const Matrix44& value);

	/*!
	Sets variable of Vector4 type to constant value.
	\param variable
	Index of variable
	\param value
	Constant value
	\return
	Pointer to initialized variable
	*/
	MaterialVarVector* SetVector(uint variable, const Vector4& value);

	/*!
	Sets variable of int type to constant value.
	\param variable
	Index of variable
	\param value
	Constant value
	\return
	Pointer to initialized variable
	*/
	MaterialVarInteger* SetInteger(uint variable, int value);

	/*!
	Sets variable of FColor type to constant value.
	\param variable Index of variable
	\param value Constant value
	\return Pointer to initialized variable
	*/
	MaterialVarColor* SetColor(uint variable, const FColor& value);

	/*!
	Sets variable of bool type to constant value.
	\param variable Index of variable
	\param value Constant value
	\return Pointer to initialized variable
	*/
	MaterialVarBoolean* SetBoolean(uint variable, ebool value);

	/*!
	Sets variable of pointer type to constant value.
	\param variable Index of variable
	\param value Constant value
	\return Pointer to initialized variable
	*/
	MaterialVarPointer* SetPointer(uint variable, void* value);

	/*!
	Copies default value from MaterialVarDef from attached MaterialClass
	and sets the variable.
	\param variable
	Index of variable
	\return
	Pointer to initialized variable, or NULL, when either MaterialClass is not 
	attached, or there is no default value defined.
	*/
	MaterialVar* SetDefaultValue(uint variable);

	/*!
	Support method for MaterialClass::CompareMaterials
	\param mat2
	Second material to compare
	\param mv
	Index of variable to compare
	\return
	-1 when less, 1 when higher, 0 when equal
	*/
	int CompareVariable(const Material* mat2, uint mv) const;

	/*!
	Gets flags set by 'surfparm'. See enf::SurfFlags
	\return
	Surface flags
	*/
	ENF_INLINE SurfaceParms GetSurfParms() const
	{
		return m_SurfParms;
	}

	/*!
	Gets material flags. See enf::MaterialFlags
	\return
	Material flags
	*/
	ENF_INLINE uint GetFlags() const
	{
		return m_iFlags;
	}

	/*!
	Sets material flags
	\param flags
	Flags that has to be set
	\return
	New flags
	*/
	ENF_INLINE uint SetFlags(uint flags)
	{
		m_iFlags |= flags;
		return m_iFlags;
	}

	/*!
	Clears material flags
	\param flags
	Flags that has to be cleared
	\return
	Flags that has been cleared
	*/
	ENF_INLINE uint ClearFlags(uint flags)
	{
		uint ret = m_iFlags & flags;
		m_iFlags &= ~flags;
		return ret;
	}

	/*!
	Tells Material to load all resources (textures). Material is incrementing
	Load calls and only when the count is zero, resources are really loaded.
	\return
	Number of load counts
	*/
   uint Load();

	/*!
	Tells Material to unload all resources (textures). Material is decrementing
	Load calls and only when the count is zero, resources are really unloaded.
	\return
	Number of load counts
	*/
	uint Unload();

	/*!
	Gets name of this material.
	\return
	Name of material.
	*/
	ENF_INLINE CStr GetName() const
	{
		return m_strName;
	}
	ENF_INLINE CStr GetMaterialName() const
	{
		return m_strMatName;
	}

	/*!
	Tells whether the Material accepts reflection from environment.
	If yes, another render pass is issued
	*/
	ENF_INLINE bool AcceptReflection() const
	{
		return ((m_iFlags & MF_ACCEPTREFLECTION) != 0);
	}

	/*!
	Tells whether the Material accepts reflection from lights.
	If yes, another render pass is issued
	*/
	ENF_INLINE bool AcceptSpeculars() const
	{
		return ((m_iFlags & MF_ACCEPTSPECULAR) != 0);
	}

	/*!
	Checks if one or more flags are set. Condition is AND.
	i.e. IsFlagSet(FLAG1|FLAG2) returns true only when
	both flags are set
	\param flags
	one or more flags. See enum enf::MaterialFlags
	*/
	ENF_INLINE bool IsFlagSet(uint flags) const
	{
		return ((m_iFlags & flags) == flags);
	}

	ENF_INLINE bool IsCreationFlagSet(uint flags) const
	{
		return ((m_iCreationFlags & flags) == flags);
	}

	ENF_INLINE uint GetCreationFlags() const
	{
		return m_iCreationFlags;
	}

	/*!
	Tells whether is MaterialClass attached. If not, material is
	not renderable.
	*/
	ENF_INLINE bool IsRenderable() const
	{
		return (m_pMClass != NULL);
	}

	/*!
	Tells whether is Material accepting reflection from scene. If 
	If yes, mirrored scene is rendered later.
	*/
	ENF_INLINE bool IsMirror() const
	{
		return ((m_iFlags & MF_MIRROR) != 0);
	}

	/*!
	Tells whether is certain surface flag set. See enf::SurfFlags
	*/
	ENF_INLINE bool IsSurfParmSet(uint flag) const
	{
		return ((m_SurfParms.GetParms() & flag) == flag);
	}

	//!Tells whether this material casts shadows
	ENF_INLINE bool IsShadowCaster() const
	{
		return ((m_iFlags & MF_CASTSHADOW) != 0);
	}

	ENF_INLINE bool IsShadowReceiver() const
	{
		return ((m_iFlags & MF_RECEIVESHADOW) != 0);
	}

	ENF_INLINE bool NeedsRenderView() const
	{
		return ((m_iFlags & MF_NEEDSRENDERVIEW) != 0);
	}

	ENF_INLINE bool NeedsEnvMap() const
	{
		return ((m_iFlags & MF_NEEDSENVMAP) != 0);
	}

	ENF_INLINE bool NeedsGUI() const
	{
		return ((m_iFlags & MF_NEEDSGUI) != 0);
	}

	ENF_INLINE bool NeedsTMat() const
	{
		return ((m_iFlags & MF_NEEDSTMAT) != 0);
	}

	ENF_INLINE bool IsCulling() const
	{
		return ((m_iFlags & MF_CULL) != 0);
	}

	ENF_INLINE bool NeedsVertexProcessing() const
	{
		return ((m_iFlags & MF_NEEDSVCMOD) != 0);
	}

	ENF_INLINE bool IsClone() const
	{
		return (m_pParent != NULL);
	}

	ENF_INLINE bool IsPassEnabled(PassType pt) const
	{
		return (BITF(pt) & m_PassMask) != 0;
	}

	ENF_INLINE bool CheckPassMask(uint mask) const
	{
		return (mask & m_PassMask) != 0;
	}

	ENF_INLINE void EnablePass(PassType pt)
	{
		m_PassMask |= BITF(pt);
	}

	ENF_INLINE void DisablePass(PassType pt)
	{
		m_PassMask &= ~BITF(pt);
	}

	ENF_INLINE bool HasStaticEnvMap() const
	{
		return ((m_iFlags & MF_HASSTATICENVMAP) != 0);
	}

	bool Update(const MaterialSource* ms);

	ENF_INLINE bool IsLoaded() const
	{
		const Material* mat = this;
		//must check our parent. He is responsible for loadcount
		while(mat->m_pParent) mat = mat->m_pParent;
		return (mat->m_iLoadCount != 0);
	}

	void CopyVariables(Material* targetMat) const;
	void LerpVariables(Material* targetMat, const Material* other, float lerp) const;

	Material(MaterialSystem* msys, CStr name);
	
	//!touches all textures, initiates loading if necessary and returns load status
	bool Touch(float level);

private:
	friend class IMaterialSystem;
	~Material();

	template<class T, class V> T* SET_VARIABLE(uint var, V value);

	static uint RenderTargetIndex(const char* texName);

	void InternalFinalize();
	bool IsType(uint variable, DataVarType type) const;
	void CloneData(Material* clone);
	Material* CreateClone(uint creationflags);
	static ubyte* VariableCopy(ubyte* dst, const MaterialVar* var, const BaseVariableDef* def);

	ENF_INLINE const IVariable* GetIVariable(const Variable* var) const;
	void AllocVariables(uint numVariables);
	void FreeVariables();
	void InitDefaults();

	float _GetScalar(const Variable* ivar, float defValue) const;
	int _GetInteger(const Variable* ivar, int defValue) const;
	FColor _GetColor(const Variable* ivar, const FColor& defValue) const;
	void _GetTCMatrix(Vector4* matrix) const;

	IMaterialSystem* m_pMaterialSystem;
	MaterialClass*	m_pMClass;
	union
	{
		MaterialVar**	m_pVariables;
		ubyte*			m_pVariableBuffer;
	};
	uint				m_iBufferSize;
	ushort			m_iNumVariables;
	ushort			m_PassMask;
	uint				m_iFlags;
	SurfaceParms	m_SurfParms;
	IL_INT32			m_iTimeStamp;

	SortType			m_SortType;
	short				m_iSortBias;
	ushort			m_iSortID;

	uint				m_iNumDeformFuncs;
	IL_INT32			m_iLoadCount;
	
	//!Material creation flags. See enf::MaterialCreationFlags
	uint				m_iCreationFlags;
	CStr				m_strName;
	CStr				m_strMatName;
	int				m_iMaxDistance;

	CFunc*			m_pDeformFuncs;

	Material*		m_pNextClone;
	Material*		m_pParent;
	static MaterialVar* m_pEmptyVariables[MAX_MATERIAL_VARIABLES];
};
//@}

}
#endif //ENF_MATERIAL_H