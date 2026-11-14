//-----------------------------------------------------------------------------
// File: enf_materialsystem.h
//
// Desc: External class for material system
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_MATERIALSYSTEM_H
#define ENF_MATERIALSYSTEM_H

#include "enf_config.h"
#include "common/enf_set.h"
#include "engine/enf_module.h"
#include "engine/enf_event.h"
#include "materials/enf_material.h"
#include "render/enf_rendertypes.h"
#include "render/enf_renderenums.h"
#include "enf_materialsystemenums.h"

namespace enf
{
/** @addtogroup MaterialSystem
@{*/

class MaterialClass;
class MaterialSystem;
class ScriptContext;
class ModelContext;
class Pipeline;
class IMesh;
class Variable;
class IVariable;
class FXEffect;
struct MatSysVar;
struct Define;
class RenderShader;
class SuperShader;

extern PUBLIC_API MaterialSystem* g_pMaterialSystem;

//-----------------------------------------------------------------------

/*!
Description of Material we are asking for
by MaterialSystem::GetMaterial

TODO: merge this with FXEffectDef so we can
customize criteria in any way. This will replace
hard-coded creation flags too!
*/
struct MaterialDescription
{
	//!Material class name
	CStr		m_strName;

	//!Various creation flags. See enf::MaterialCreationFlags
	uint		m_iFlags;

	//!Empty constructor
	MaterialDescription(): m_iFlags(0) {}

	/*!
	Constructor for material description. Used for searching
	the best material class for our needs.
	\param name
	If we want exactly certain material class, it's his name
	\param flags
	Various creation flags. See enf::MaterialCreationFlags
	*/
	MaterialDescription(CStr name, uint flags):
		m_strName(name),
		m_iFlags(flags)
	{
	}
};

//-----------------------------------------------------------------------
class PUBLIC_API MaterialSystem: public Module
{
protected:
	virtual ~MaterialSystem(){};

	//RenderType		m_eType;
	int						m_iLastValidRender;
	uint					m_iMaterialSortVersion;

	/*we know this is just interface, but variables will be implemented here, as we
	need it as fast as possible. So, no virtual methods!
	*/

	struct VarDef
	{
		DataVarType	Type;
		const char*	Name;
		uint			ArraySize;
	};

	Array<VarDef>			m_Variables;
	Array<uint>				m_VariableIndices;
	Array<float>			m_ScalarVariables;
	Array<int>				m_IntegerVariables;
	//TODO: optional allocator to guarantee 16byte alignment!
	Array<Matrix44>		m_Matrix44Variables;
	Array<Matrix34>		m_Matrix34Variables;
	Array<Vector4>		m_Vector4Variables;
	Array<Texture*>		m_TextureVariables;

public:
	ENF_DECLARE_ABSTRACT_ITEM(MaterialSystem, Module);

	MaterialSystem();

	void SetCurrent()
	{
		g_pMaterialSystem = this;
	}

	virtual void OnEvent(const Event& event)=0;
	virtual void OnInsert(TiXmlElement* xmlnode)=0;
	virtual bool OnChangeSettings(TiXmlElement* xmlnode)=0;

	virtual bool IsEditorMode() const=0;

	/*!
	Whenever you keep some information about material sort somewhere else,
	keep to be synced with this version number. When version differs, you
	must rearrange your order too.
	*/
	ENF_INLINE uint GetSortVersion() const
	{
		return m_iMaterialSortVersion;
	}

	//!Built-in variables
	enum BasicVariables
	{
		MV_PROJVIEW_MATRIX = 0,
		MV_PROJECT_MATRIX,
		MV_PROJECTTRANS_MATRIX,
		MV_VIEW_MATRIX,
		MV_VIEWTRANS_MATRIX,
		MV_BILLBOARD_MATRIX,
		MV_EYE_POS,
		MV_ASPECT,
		MV_NEARPLANE,
		MV_FARPLANE,
		MV_IDENTNORMAL_TEXTURE,				///< Returns texture with A8R8G8B8 = 0xff7f7fff (represents vector (0,0,1))
		MV_IDENTNORMALDXT_TEXTURE,		///< Returns texture with A8R8G8B8 = 0x7f007f00 (represents vector (0,0,1) for DXT compressed normal-maps)
		MV_BLACK_TEXTURE,							///< Returns texture with A8R8G8B8 = 0xff000000
		MV_WHITE_TEXTURE,							///< Returns texture with A8R8G8B8 = 0xffffffff
		MV_GRAY_TEXTURE,							///< Returns texture with A8R8G8B8 = 0xff808080
		MV_DEFAULT_SKY_TEXTURE,				///< returns cube-texture with A8R8G8B8 = 0x0022a4b3
		MV_WHITE_CUBE_TEXTURE,				///< white cube texture
		MV_TIME,
		MV_SHADOWVIEWPROJ_TRANS_MAT,
		MV_SHADOWVIEWPROJ_TERRAIN_TRANS_MAT,
		MV_SHADOW_MIN_DISTANCE,
		MV_SHADOW_MAX_DISTANCE,
		MV_SHADOWMAP_PARAMS,
		MV_CAMERA_MATRIX,
		MV_INVVIEW_MATRIX,
		MV_INVVIEWTRANS_MATRIX,
		MV_DEPTH_RANGE,
		MV_DEPTH_PARAMS,
		MV_TERRAIN_SHADOW,
		MV_TERRAIN_SHADOW_MAP,
		#ifdef ENF_PS3
			MV_TERRAIN_SHADOW_MAP_8BIT,
		#endif
		MV_OBJECT_LOD_SCALE,
	};

	virtual void SetInMirror(bool inmirror)=0;
	virtual void ToggleInMirror()=0;

	//!Must be called upon begin of batch
	virtual void BeginPass()=0;

	virtual void BeginFrame(const Matrix43& viewmatrix, RenderType type)=0;
	virtual void EndFrame()=0;

	virtual bool PushProjection(int left, int top, int width, int height, const Matrix44& projmatrix, float nearplane, float farplane)=0;
	virtual void PopProjection()=0;

	virtual bool PushCamera(const Matrix43& viewmatrix, const Matrix43& invviewmatrix)=0;
	virtual void PopCamera()=0;

	/*!
	Internally calls Renderer::DrawPrimitive.
	This wrapper must be used instead of direct call in order to make
	everything working
	\param mesh
	Mesh to be rendered
	\param dirty GetCurrentRender needs to be refreshed after this DP
	*/
	void DrawMesh(const IMesh* mesh, bool dirty = true);
	/*!
	Internally calls Renderer::DrawIndexedPrimitive.
	This wrapper must be used instead of direct call in order to make
	everything working
	\param mesh Mesh to be rendered
	\param dirty GetCurrentRender needs to be refreshed after this DP
	*/
	void DrawIndexedMesh(const IMesh* mesh, bool dirty = true);

	/*!
	Internally calls Renderer::DrawIndexedPrimitive/Renderer::DrawPrimitive.
	This wrapper must be used instead of direct call in order to make
	everything working
	\param mesh Mesh to be rendered
	\param numInstances Number of instances 
	\param dirty GetCurrentRender needs to be refreshed after this DP
	*/
	void DrawInstancedMesh(const IMesh* mesh, uint numInstances, bool dirty = true);
	
	/*!
	Internally calls Renderer::DrawIndexedPrimitive/Renderer::DrawPrimitive.
	This wrapper must be used instead of direct call in order to make
	everything working
	\param mesh Mesh to be rendered
	\param numInstances Number of instances 
	\param dirty GetCurrentRender needs to be refreshed after this DP
	*/
	void DrawIndexedInstancedMesh(const IMesh* mesh, uint numInstances, bool dirty = true);

	/*!
	Gets copy of current rendered frame
	\return
	Pointer to Texure with copy
	*/
	virtual Texture* GetCurrentRender()=0;
	virtual RenderTarget* AllocRenderTarget(uint width, uint height, TextureFormat rtfmt, TextureFormat depthfmt, FSAAType fsaa, uint usage, uint edramoffset, uint& handle, uint texFlags = 0)=0;
	virtual void FreeRenderTarget(uint handle)=0;

	//!When you change current render-target otherwise than DrawMesh, you must make him dirty manually
	void DirtyCurrentRender()
	{
		m_iLastValidRender = -1;
	}

	/*!
	Utility method, returns Texture with constant A8R8G8B8 color
	\return
	Texture pointer. Must not be released!
	*/
	virtual Texture* GetIdentityColorMap(const FColor& color, uint flags)=0;

	/*!
	Creates supershader from shader source and array of options.
	\param name
	Name of shader source
	\param profile
	Lowest profile for shader compilation
	\param options
	NULL terminated list of options. For bool options, just use option name, for enums/ints, use name[size] syntax
	\param defines
	NULL terminated list of additional defines used for compilation (not used as options)
	\return
	NULL or pointer to enf::SuperShader
	*/
	virtual SuperShader* CreateSuperShader(CStr name, ProfileType profile, const char* const* options, const char* const* defines = NULL)=0;
	
	/*!
	Helper method, sets Z-bias, where +1 or -1 means bias by one distinct level
	\param bias
	Bias factor
	*/
	virtual void SetBias(float bias)=0;

	/*!
	Gets array of available functions for material deformation. These
	are supported by Material,MaterialSource and tools (Material editor).
	To make them working, they must be either implemented by MaterialClass
	or by processing throught VertexProcessor. Anyway, all these functions
	are supported by current MaterialClass Generic.
	\return
	Array of enums. Terminated by NULL pointer
	*/
	virtual const ParmEnumMaterialDeform** GetDeformFuncs() const=0;

/** \name Material system variables
Global material variables. These are global to all MaterialClasses.
You can add your own if you need. All projection, world to camera etc.
matrices are defined here. See enf::MaterialSystem::AddVariable for details.
*/
//@{
	/*!
	Adds custom variable to MaterialSystem. These variables
	can be set upon begin of frame and asked during rendering
	from MaterialClass. This makes possible to add whatever
	transformation matrices, vectors, positions and custom
	data.
	\param name
	Name of variable. Used for dynamic binding. For example
	when you want to copy HLSL shared variables from here, this
	name could be used for matching these variables.
	\param type
	Variable type. You must use only these SetXXX GetXXX methods
	which match with this type!
	\param arraySize
	Size of array
	\return
	Index of variable. Variable can be addressed by name or much faster
	by index.
	*/
	uint AddVariable(CStr name, DataVarType type, uint arraySize = 1);

	/*!
	Sets value of matrix-variable. Type must match with
	type used for AddVariable!
	\param variable
	Index of variable
	\param value
	Matrix value
	\param index
	Index to array
	*/
	void SetMatrix(uint variable, const Matrix44& value, uint index = 0);
	void SetMatrix34(uint variable, const Matrix34& value, uint index = 0);
	void SetMatrix34Array(uint variable, const Matrix34* value, uint count);

	/*!
	Sets value of vector-variable. Type must match with
	type used for AddVariable!
	\param variable
	Index of variable
	\param value
	Vector4 value
	*/
	void SetVector(uint variable, const Vector4& value);

	/*!
	Sets value of float-variable. Type must match with
	type used for AddVariable!
	\param variable
	Index of variable
	\param value
	float value
	*/
	void SetScalar(uint variable, float value);

	/*!
	Sets value of Texture-variable. Type must match with
	type used for AddVariable!
	\param variable
	Index of variable
	\param value
	float value
	*/
	void SetTexture(uint variable, Texture* value);
	
	/*!
	Sets value of bool-variable. Type must match with
	type used for AddVariable!
	\param variable
	Index of variable
	\param value
	bool value
	*/
	void SetBoolean(uint variable, bool value);
	
	/*!
	Sets value of int-variable. Type must match with
	type used for AddVariable!
	\param variable
	Index of variable
	\param value
	bool value
	*/
	void SetInteger(uint variable, int value);
	
	/*!
	Gets index of variable definition.
	\param name
	Name of variable
	\return
	Index of variable or INDEX_NOT_FOUND
	*/
	uint GetVariableIndex(CStr name) const;

	/*!
	Gets value of matrix-variable. Type must match with
	type used for AddVariable!
	\return
	Matrix44 value
	*/
	const Matrix44& GetMatrix(uint variable) const
	{
		enf_assert(m_Variables[variable].Type == DVT_MATRIX);
		return m_Matrix44Variables[m_VariableIndices[variable]];
	}

	/*!
	Gets value of matrix-variable. Type must match with
	type used for AddVariable!
	\return
	Matrix34 value
	*/
	const Matrix34& GetMatrix34(uint variable) const
	{
		enf_assert(m_Variables[variable].Type == DVT_MATRIX34);
		return m_Matrix34Variables[m_VariableIndices[variable]];
	}
	const Matrix34* GetMatrix34Array(uint variable) const
	{
		enf_assert(m_Variables[variable].Type == DVT_MATRIX34);
		return &m_Matrix34Variables[m_VariableIndices[variable]];
	}

	/*!
	Gets value of vector-variable. Type must match with
	type used for AddVariable!
	\return
	Vector4 value
	*/
	const Vector4& GetVector(uint variable) const
	{
		enf_assert(m_Variables[variable].Type == DVT_VECTOR);
		return m_Vector4Variables[m_VariableIndices[variable]];
	}

	/*!
	Gets value of float-variable. Type must match with
	type used for AddVariable!
	\return
	float value
	*/
	float GetScalar(uint variable) const
	{
		enf_assert(m_Variables[variable].Type == DVT_SCALAR);
		return m_ScalarVariables[m_VariableIndices[variable]];
	}

	/*!
	Gets value of Texture-variable. Type must match with
	type used for AddVariable!
	\return
	float value
	*/
	Texture* GetTexture(uint variable) const
	{
		enf_assert(m_Variables[variable].Type == DVT_TEXTURE);
		return m_TextureVariables[m_VariableIndices[variable]];
	}

	/*!
	Gets value of bool-variable. Type must match with
	type used for AddVariable!
	\return
	bool value
	*/
	bool GetBoolean(uint variable) const
	{
		enf_assert(m_Variables[variable].Type == DVT_BOOLEAN);
		return m_IntegerVariables[m_VariableIndices[variable]] != 0;
	}

	/*!
	Gets value of int-variable. Type must match with
	type used for AddVariable!
	\return
	bool value
	*/
	int GetInteger(uint variable) const
	{
		enf_assert(m_Variables[variable].Type == DVT_INTEGER);
		return m_IntegerVariables[m_VariableIndices[variable]];
	}

//@}

/** \name Materials
Materials management
*/
//@{
	/*!
	Creates empty material
	\param matname
	Name
	\return
	Pointer to created material
	*/
	virtual Material* CreateMaterial(CStr matname)=0;

	virtual void RemoveMaterial(Material* material)=0;
	
	/*!
	Gets Material definition. You can specify flags with general switches which
	defines usage and purpose of Material and MaterialClass pair. If material
	is found, hit refcout is incremented. Call Material::Release when you don't
	need the Material anymore.
	\param matname
	Name of material definition
	\param flags
	Creation flags. See enf::MaterialCreationFlags
	*/
	virtual Material* GetMaterial(CStr matname, uint flags)=0;

	/*!
	Gets material definition. If material is found, pointer is returned and
	refcount is incremented. Call Material::Release when you don't need the
	Material anymore.
	\param matname
	Name of Material definition
	\return
	Material definition or NULL
	*/
	virtual Material* GetMaterial(CStr matname)=0;

	/*!
	Tells whether the Material exists.
	*/

	virtual bool MaterialExists(CStr matname) const=0;

	/*!
	Gets array of created materials.
	\return
	Array of Material definitions
	*/
	virtual const Set<Material*>& GetMaterials() const=0;
//@}

/** \name Material classes
MaterialClass management
*/
//@{
	/*!
	Finds MaterialClass definition
	\param name
	Name of material class
	\return
	Pointer to MaterialClass definition or NULL
	*/
	virtual MaterialClass* FindMaterialClass(const char* name)=0;

	/*!
	Finds MaterialClass definition by MaterialDescription
	\param mdesc
	MaterialDescription with criteria we want to meet
	\param inherited
	Filter only those MaterialClass inherited from this one
	\return
	Pointer to MaterialClass definition or NULL
	*/
	virtual MaterialClass* FindMaterialClass(const MaterialDescription& mdesc, CStr inherited = NULL)=0;

	/*!
	Adds MaterialClass definition. These definitions are managed by MaterialSystem then
	and must not be deleted
	\param mclass
	MaterialClass definition
	*/
	virtual bool AddMaterialClass(MaterialClass* mclass)=0;

	/*!
	Gets array of registered materialclasses
	\return
	Array of material classes
	*/
	virtual const Array<MaterialClass*>& GetMaterialClasses() const=0;
//@}

/** \name Common Material variable definitions
	These variables are included in all Materials
	nevertheless what MaterialClass they use. Physical
	properties, which are common to all materials are
	included here for example. You can add your own, if you need.
	It automaticaly extends loader, saver and material editor.
*/
//@{

	/*!
	Adds definition
	\param mvdef
	Definition to be added
	*/
	virtual uint AddVariableDef(const MaterialVarDef& mvdef)=0;

	/*!
	Gets definition by its index
	\param index
	Index of variable definition to be returned
	\return
	Pointer to definition or NULL
	*/
	virtual const MaterialVarDef* GetVariableDef(uint index) const=0;

	/*!
	Gets definition by its name
	\param name
	Name of variable definition to be returned
	\return
	Pointer to definition or NULL
	*/
	virtual const MaterialVarDef* GetVariableDef(CStr name) const=0;

	/*!
	Finds definition by its name
	\param name
	Name of variable definition to be found
	\return
	Index when definition returned, INDEX_NOT_FOUND when definition is not found
	*/
	virtual uint FindVariableDef(CStr name) const=0;

	/*!
	Gets number of definitions
	*/
	virtual uint GetNumVariableDefs() const=0;
//@}
};


PUBLIC_API ENF_INLINEFUNC MaterialSystem* GetMaterialSystem()
{
	enf_assert(g_pMaterialSystem);
	return g_pMaterialSystem;
}

PUBLIC_API MaterialSystem* CreateMaterialSystem(Renderer* device, ScriptContext* script, bool editormode);

//-----------------------------------------------------------------------
//@}
}
#endif //ENF_MATERIALSYSTEM_H