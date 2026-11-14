//-----------------------------------------------------------------------------
// File: enf_materialclass.h
//
// Desc: Material render definition
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_MATERIALCLASS_H
#define ENF_MATERIALCLASS_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_hashmap.h"
#include "common/enf_strings.h"
#include "render/enf_rendertypes.h"
#include "render/enf_renderenums.h"
#include "enf_materialvar.h"
#include "enf_material.h"

namespace enf
{
/** @addtogroup MaterialSystem
@{*/

//-----------------------------------------------------------------------
class Variable;
class IVariable;

class Pipeline;
class ModelContext;
class IMesh;

class Material;
class MaterialSystem;
class IMaterialSystem;
struct MaterialDescription;

//!TexFilter is index to filter presets for each certain texture type
enum TexFilter
{
	FILTER_NONE,				///<Not filtering at all
	FILTER_DIFFUSEMAP,		///<Decent filtering, used for diffuse maps
	FILTER_NORMALMAP,			///<stronger filtering, for better looking normal maps
	FILTER_CUBEENVMAP,		///<Lousy filtering, for smoothed environment reflections
	FILTER_LOW,					///<General low quality filtering, for various purpose
	FILTER_MID,					///<General mid quality filtering, for various purpose
	FILTER_HIGH,				///<General high quality filtering, for various purpose
	FILTER_TERRAIN_LAYERMAP,	///<Filtering for alpha-layer-maps, used for detail terrain
	FILTER_TERRAIN_DIFFUSEMAP,	///<Filtering for diffuse-maps, used for detail terrain
	FILTER_TERRAIN_NORMALMAP,	///<Filtering for normal-maps, used for detail terrain
	FILTER_SUPER_DIFFUSEMAP,	///<Filtering for large diffuse-maps, used for distant terrain
	FILTER_SUPER_NORMALMAP,	///<Filtering for large normal-maps, used for distant terrain
	FILTER_LIGHTMAP,			///<Filtering for light-maps. Usually worse than diffuse-map, because of lower resolution
	FILTER_MAX_VALUE
};

class TiXmlElement;
//-----------------------------------------------------------------------
#define ENF_DEFINE_MCLASS_TYPE(name, parent, abstract)\
	virtual bool IsAbstract() const {return abstract;}\
	virtual CStr GetName() const {return name;}\
	virtual bool HasType(CStr ttype) const {if(ttype == name) return true; else return parent::HasType(ttype);}

#define BEGIN_DEFINE_MCLASS(classname, name, parent) class PUBLIC_API classname: public parent {\
public:\
	classname(MaterialSystem* imat): parent(imat){}\
	ENF_DEFINE_MCLASS_TYPE(name, parent, false)

#define BEGIN_DEFINE_ABSTRACT_MCLASS(classname, name, parent) class PUBLIC_API classname: public parent {\
public:\
	classname(MaterialSystem* imat);\
	ENF_DEFINE_MCLASS_TYPE(name, parent, true)

#define BEGIN_DEFINE_EXT_MCLASS(classname, name, parent) class PUBLIC_API classname: public parent {\
public:\
	classname(MaterialSystem* imat);\
	ENF_DEFINE_MCLASS_TYPE(name, parent, false)
	

#define END_DEFINE_MCLASS };

#define DEFINE_MCLASS_METHODS \
	virtual ERESULT OnInit(enf::TiXmlElement* root);\
	virtual void RenderPass(const enf::Pipeline* pipeline, enf::PassType passtype, enf::Material* mat, const enf::IMesh* mesh, const void* data);\
	virtual uint BeginPass(const enf::Pipeline* pipeline, enf::PassType passtype, enf::Material* mat, const void* data = NULL);\
	virtual void SetModel(const enf::Pipeline* pipeline, enf::PassType passtype, enf::ModelContext* mc);\
	virtual bool IsCompatible(const enf::MaterialDescription& md);


#define MCLASS_INIT(classname)			enf::ERESULT classname:: OnInit(enf::TiXmlElement* root)
#define MCLASS_ON_SET(classname)			void classname:: OnSet(enf::Pipeline* pipeline, enf::PassType pt)
#define MCLASS_ON_UNSET(classname)		void classname:: OnUnset(enf::Pipeline* pipeline, enf::PassType pt)
#define MCLASS_BEGIN_PASS(classname)	enf::uint classname:: BeginPass(const enf::Pipeline* pipeline, enf::PassType passtype, enf::Material* mat, const void* data)
#define MCLASS_SET_MODEL(classname)		void classname:: SetModel(const enf::Pipeline* pipeline, enf::PassType passtype, enf::ModelContext* mc)
#define MCLASS_PASS(classname)			void classname:: RenderPass(const enf::Pipeline* pipeline, enf::PassType passtype, enf::Material* mat, const enf::IMesh* mesh, const void* data)

//-----------------------------------------------------------------------
/*!
MaterialClass is 'class' representation of Material, whereas Material is
representing 'instance'. Here is interface for parameters description and
rendering functionality. you can re-implement this class in any way and
create either fixed function rendering, or HLSL or whatever you want.
MaterialClass expose set of user variables of DataVarType type. These
are allocated in every Material which is using this MaterialClass and
can be queried in runtime for value, or just asked if they are set.
*/
class PUBLIC_API MaterialClass
{
protected:
	friend class IMaterialSystem;
	friend class Material;

	IMaterialSystem*	m_pMaterialSystem;
	//current context
	ModelContext*	m_pContext;

	Array<MaterialVarDef>	m_VarDefs;
	HashMap<CStr, uint, CStrHashFuncCS> m_VarHash;

	static TexFilter m_TextureUsage2Filter[16];
	static SamplerAddressStateHandle m_AddressStates[4];

public:
	ENF_DECLARE_MMOBJECT(MaterialClass);

	/*!
	Gets pointer to owner MaterialSystem
	\return
	Pointer to MaterialSystem
	*/
	ENF_INLINE IMaterialSystem* GetMaterialSystem() const
	{
		return m_pMaterialSystem;
	}

	/*!
	Utility method, gets instance variable of current rendered Entity if exists. When not,
	some default value is returned.
	\param var
	Pointer to Variable. Global variable or class member.
	\return
	Pointer to IVariable
	*/
	const IVariable* GetVariable(const Variable* var) const;

	/*!
	Gets number of variables.
	\return
	Number of variables
	*/
	ENF_INLINE uint GetNumVariables() const
	{
		return m_VarDefs.GetCardinality();
	}

	/*!
	Gets description of variable or NULL, when variable do not exists
	\param index
	Index of variable.
	\return
	Pointer to definition or NULL
	*/
	ENF_INLINE const MaterialVarDef* GetVariableDef(uint index) const
	{
		enf_assert(m_VarDefs[index].m_iIndex == index);
		return &m_VarDefs[index];
	}


	/*!
	Gets description of variable or false, when variable do not exists
	\param name
	Name of variable
	\return
	True if variable exists, false otherwise
	*/
	const MaterialVarDef* GetVariableDef(CStr name) const
	{
		uint index = GetVariableIndex(name);
		if(index == INDEX_NOT_FOUND)
			return false;
		return GetVariableDef(index);
	}
	/*!
	Gets just type of variable instead of whole description. This method is used for
	quick lookup for type during runtime. Should be implemented as fast as possible.
	\param index
	Index of variable
	\return
	Type of variable
	*/
	ENF_INLINE DataVarType GetVariableType(uint index) const
	{
		return m_VarDefs[index].m_Type;
	}

	/*!
	Gets index of variable.
	\param name
	Name of variable
	\return
	Index of variables of INDEX_NOT_FOUND when variable do not exists
	*/
	uint GetVariableIndex(CStr name) const;

	/*!
	Adds variable definition
	\param def
	Definition structure
	\return
	Index of variable
	*/
	uint AddVarDef(const MaterialVarDef& def);

	/*!
	Gets current ModelContext. Valid only during rendering!
	\return
	Pointer to current ModelContext
	*/
	ENF_INLINE const ModelContext* GetContext() const
	{
		return m_pContext;
	}

	/*!
	Gets name of this MaterialClass
	\return
	The name
	*/
	virtual CStr GetName() const=0;

	/*!
	Asks whether is this MaterialClass usable for rendering or is it
	just superclass for another MaterialClass
	\return
	True when it is just superclass, false otherwise
	*/
	virtual bool IsAbstract() const
	{
		return false;
	}

	/*!
	Asks whether we are inherited from the typename
	*/
	virtual bool HasType(CStr type) const
	{
		return false;
	}
	

	virtual int CompareMaterials(const Material* mat1, const Material* mat2) const;

	/*!
	Constructor of MaterialClass
	\param imat
	MaterialSystem to which this MaterialClass belongs
	*/
	MaterialClass(MaterialSystem* imat);

	virtual ~MaterialClass(){}

	/*!
	Called when MaterialClass is inserted into MaterialSystem
	\param root
	Root element of settings XML DOM
	\return
	EE_OK when OK
	*/
	virtual ERESULT OnInit(TiXmlElement* root) { return EE_OK; }

	/*!
	Called when MaterialSystem gets event that all shaders, effects etc. has been reloaded.
	Then all MaterialClasses should reload all handles for example.
	*/
	virtual void OnReloadShaders()
	{
	}

	//!Should be re-implemented in most efficient way!
	virtual bool TouchTextures(Material* mat) const;

	/*!
	Called when MaterialSystem is notified that settings has changed.
	MaterialClass should look for its settings and change something if needed.
	\param root
	Root node of settings XML DOM
	\return
	True when accepted, false when change requires engine restart
	*/
	virtual bool OnChangeSettings(TiXmlElement* root) {return true;}

	/*!
	Called upon setting this MaterialClass to the Material. It happen just at begin of creation.
	\param mat
	Material we are set for.
	\return
	True when successfully set, false otherwise
	*/
	virtual bool OnMaterialSet(Material* mat){return true;}

	/*!
	Called when deleting Material. Here you can cleanup your per-instance data
	\param mat
	Material we are set for.
	*/
	virtual void OnMaterialDelete(Material* mat){}

	/*!
	Called after Material::Load(), so texture variables are already set
	\param mat
	Material we are set for.
	*/
	virtual void OnMaterialLoaded(Material* mat){}

	/*!
	Called before Material::Unload(), so all texture reference 
	can be released before
	\param mat
	Material we are set for.
	*/
	virtual void BeforeMaterialUnload(Material* mat){}
	/*!
	Called upon finalization of the Material. It happen just at end of creation. Here
	is usually done validating of parameters, setting up default values
	and setting MaterialFlags when necessary.
	\param mat
	Material we are set for.
	*/
	virtual void OnMaterialFinalize(Material* mat) {}


	/*!
	Called whenever MaterialClass is changed during rendering.
	Here you can set some common states
	\param pipeline Pipeline we are rendering
	\param passtype Type of pass we are rendering
	*/
	virtual void OnSet(Pipeline* pipeline, PassType passtype) {}

	/*!
	Called whenever MaterialClass is changed during rendering.
	Here you can unset some common states
	\param pipeline Pipeline we are rendering
	\param passtype Type of pass we are rendering
	*/
	virtual void OnUnset(Pipeline* pipeline, PassType passtype) {};

	/*!
	Must be called upon start of batch
	\param pipeline Pipeline we are rendering
	\param passtype General type of pass
	\param mat Material definition
	\param data user data for pass (ILight for light passes for example)
	\return number of necessary passes
	*/
	virtual uint BeginPass(const Pipeline* pipeline, PassType passtype, Material* mat, const void* data = NULL)
	{
		return 0;
	}


	/*!
	Must be called whenewer new Model is enumerated in batch
	\param pipeline Pipeline we are rendering
	\param passtype
	Type of pass we are rendering
	\param mc Context of new model
	*/
	virtual void SetModel(const Pipeline* pipeline, PassType passtype, ModelContext* mc) { m_pContext = mc; }

	/*!
	General drawing method.
	\param pipeline
	Pipeline used for the rendering
	\param passtype
	Type of pass we are rendering
	\param mat
	Material
	\param mesh
	Pointer to rendered mesh
	\param data
	Optional data which you can pass from Pipeline
	*/
	virtual void RenderPass(const Pipeline* pipeline, PassType passtype, Material* mat, const IMesh* mesh, const void* data = NULL) {}
	virtual void BeginInstanced(const Pipeline* pipeline, PassType passtype, ModelContext* mc, Material* mat, const IMesh* mesh, const void* data = NULL) {}
	virtual void RenderInstances(const enf::Pipeline* pipeline, enf::PassType passtype, const enf::IMesh* mesh, const ModelContext** mc, uint numinstances) {}

	/*!
	Selector.
	\param md Description of necessary features
	\return true when MaterialClass is compatible, false otherwise
	*/
	virtual bool IsCompatible(const MaterialDescription& md)
	{
		return false;
	}

	virtual bool IsCompatible(uint creationFlags) const
	{
		return false;
	}

	static void SetAddressMode(uint tmu, const Material* mat, uint var);
	static void SetFilterMode(uint tmu, const Material* mat, uint var);
	static void SetFilterMode(uint tmu, TextureUsage textype);

	/*!
	Tells how much instances can be rendered at once
	\param pt
	Type of pass, we want to render
	\return
	Maximum of instances
	*/
	virtual uint GetMaxInstances() { return 1; }

	virtual void BeginFrame();
	virtual void EndFrame();


	virtual void GetTilesCount(const Material* mat, uint& tx, uint& ty) const 
	{
		tx = ty = 0;
	}

	virtual const Texture* GetDiffuseTexture(const Material* mat) const 
	{
		return NULL;
	}
};
//@}

#define MAT_DECLARE_INTEGER(var, name, defvalue, uiwidget, rangescale, desc, enums) \
	var = AddVarDef(MaterialVarDef(DVT_INTEGER, 0, 0, name, defvalue, uiwidget, rangescale, desc, enums))

#define MAT_DECLARE_SCALAR(var, name, defvalue, uiwidget, rangescale, desc, enums) \
	var = AddVarDef(MaterialVarDef(DVT_SCALAR, 0, 0, name, defvalue, uiwidget, rangescale, desc, enums))

#define MAT_DECLARE_STRING(var, name, defvalue, desc, enums) \
	var = AddVarDef(MaterialVarDef(DVT_STRING, 0, 0, name, defvalue, "editbox", NULL, desc, enums))

#define MAT_DECLARE_VECTOR(var, name, defvalue, uiwidget, desc, enums) \
	var = AddVarDef(MaterialVarDef(DVT_VECTOR, 0, 0, name, defvalue, uiwidget, NULL, desc, enums))

#define MAT_DECLARE_COLOR(var, name, defvalue, desc, enums) \
	var = AddVarDef(MaterialVarDef(DVT_COLOR, 0, 0, name, defvalue, "color", NULL, desc, enums))

#define MAT_DECLARE_TEXTURE(var, subtype, name, defvalue, desc, enums) \
	var = AddVarDef(MaterialVarDef(DVT_TEXTURE, subtype, 0, name, defvalue, "texture", NULL, desc, enums))

#define MAT_DECLARE_TEXTURE_PRI(var, subtype, prio, name, defvalue, desc, enums) \
	var = AddVarDef(MaterialVarDef(DVT_TEXTURE, subtype, prio, name, defvalue, "texture", NULL, desc, enums))

#define MAT_DECLARE_BOOLEAN(var, name, defvalue, desc) \
	var = AddVarDef(MaterialVarDef(DVT_BOOLEAN, 0, 0, name, defvalue, "checkbox", NULL, desc))

#define MAT_DECLARE_POINTER(var, name, desc) \
	var = AddVarDef(MaterialVarDef(DVT_POINTER, 0, 0, name, 0, "editbox", NULL, desc, NULL))


}
#endif //ENF_MATERIALCLASS_H