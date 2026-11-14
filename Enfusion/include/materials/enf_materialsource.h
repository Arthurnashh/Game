//-----------------------------------------------------------------------------
// File: enf_materialsource.h
//
// Desc: Material source definition
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_MATERIALSOURCE_H
#define ENF_MATERIALSOURCE_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_set.h"
#include "common/enf_strings.h"
#include "common/enf_hashmap.h"
#include "materials/enf_materialsystem.h"

namespace enf
{
/** @addtogroup MaterialSystem
@{*/

class Variable;
class Class;
struct ParmEnum;
struct ParmEnumMaterialDeform;

enum MatSrcSaveFlags
{
	MSF_ALL = 0,	///<All MaterialSource are saved
	MSF_ONLYCHANGED = 1	///<Only those MaterialSource::IsChanged() == true are saved
};

//-----------------------------------------------------------------------
/*!
Collection of parsed materials.
*/
class PUBLIC_API MaterialSourceCollection
{
protected:
	friend class MaterialSource;

	Set<MaterialSource*>	m_Materials;
	HashMap<const char*, MaterialSource*, StringHashFunc>	m_MaterialsHash;
	MaterialSystem*	m_pMaterialSystem;
	struct File 
	{
		DString	m_strName;
		bool	m_bChanged;
		File(): m_bChanged(false) {}
		File(CStr name): m_strName(name), m_bChanged(false) {}
	};

	Array<File>			m_Files;

	uint FindBestFile(CStr name);

public:
	ENF_DECLARE_MMOBJECT(MaterialSourceCollection);

	MaterialSourceCollection(MaterialSystem* ms);
	~MaterialSourceCollection();
	
	/*!
	Gets set of all MaterialSourcees in the collection.
	\return
	Set of all MaterialSources.
	*/
	const Set<MaterialSource*>& GetMaterials() const
	{
		return m_Materials;
	}

	/*!
	Finds MaterialSource by its name.
	\param name
	Name of MaterialSource we are looking for
	\return
	Pointer to MaterialSource if it exists. NULL otherwise
	*/
	MaterialSource* FindMaterial(CStr name) const;

	/*!
	Removes MaterialSource from internal lists and it's destroyed there too.
	\param material
	MaterialSource to be removed and destroyed.
	*/
	void DestroyMaterial(MaterialSource* material);

	/*!
	Adds existing MaterialSource to internal lists and hash-tables. The collection
	is responsible for it and it's destroyed when collection is destroyed too.
	\param material
	MaterialSource to be added.
	\param file
	Index of file to add in
	*/
	void AddMaterial(MaterialSource* material, int file = -1, bool markFileAsChanged = false);
	
	/*!
	Removes MaterialSource form internal lists and hash-tables. MaterialSource is not
	destroyed and caller is responsible for him then.
	\param material
	MaterialSource we are going to remove
	*/
	void RemoveMaterial(MaterialSource* material);

	//!Returns number of files we are loaded from
	uint GetNumFiles() const { return m_Files.GetCardinality(); }
	//!Returns name of the file
	CStr GetFile(uint index) const { return m_Files[index].m_strName.cstr(); }

	/*!
	Load all material from legacy material scripts.
	\param name
	Name of master script (ie materials/materials.h)
	\param keepsource
	When true, all parsed MaterialSourcees remains in the collection. Usable for tools
	which will work with them later. Otherwise all MaterialSources are deleted after
	Material is created and initialized to save memory (usable for runtime)
	*/
	bool LoadMaterialsFromScript(CStr name, bool keepsource);

	//bool LoadMaterialsFromBIN(CStr name, bool keepsource);

	/*!
	Saves all materials to their files. In future there may be option to save only changed files.
	\param flags
	See enf::MatSrcSaveFlags
	\param errors
	Buffer which will receive errors, separated by \n
	\param maxsize
	Maximum size of buffer
	\return
	True when save was successful. False otherwise (for example, some files are read-only)
	*/
	bool SaveMaterialsToScript(uint flags, char* errors, size_t maxsize);
};

//-----------------------------------------------------------------------
/*!
Source data for material creation. It's just collection of variables of
various types. Most variable types uses basic MatSrcVar. Some special
types (textures, flags collections...) uses their inherited implementations.
*/
class PUBLIC_API MaterialSource
{
public:
	ENF_DECLARE_SMALLOBJECT(MaterialSource);

	/*!
	Structure which holds reference to script Variable
	*/
	struct VariableRef
	{
		Variable*	VariablePtr;
		Class*		ClassPtr;

		void Clear()
		{
			VariablePtr = NULL;
			ClassPtr = NULL;
		}

		bool IsSet() const
		{
			return (VariablePtr != NULL);
		}

		//!Default constructor
		VariableRef(): VariablePtr(NULL), ClassPtr(NULL)
		{
		}
	};

	/*!
	Common type, used for scalars, integers, vectors, colors and
	other types which can be expressed by one string value.
	*/
	struct PUBLIC_API MatSrcVar
	{
		ENF_DECLARE_SMALLOBJECT(MatSrcVar);

		DataVarType			Type;
		VariableRef			VarRef;
		CStr					Name;
		DString				Value;
		const ParmEnum*	Enum;
		MaterialSource*	MatSource;

		//so we don't need to care about type when deleting
		virtual ~MatSrcVar() {}

		bool IsVariableReference() const
		{
			return VarRef.IsSet();
		}

		MatSrcVar& operator=(const MatSrcVar& v);

		MatSrcVar(DataVarType type, CStr name): Type(type), Name(name), Enum(NULL), MatSource(NULL) {}
		MatSrcVar(): Type(DVT_NULL), Name(NULL), Enum(NULL), MatSource(NULL) {}

		/*!
		Converts Value to bool representation
		Validation is not performed. You should know what to call!
		\return
		Boolean value of variable
		*/
		bool ToBool() const;

		/*!
		Converts Value to float representation
		Validation is not performed. You should know what to call!
		\return
		Float value of variable
		*/
		float ToFloat() const;

		/*!
		Converts Value to integer representation
		Validation is not performed. You should know what to call!
		\return
		Integer value of variable
		*/
		int ToInt() const;

		/*!
		Converts Value to FColor representation
		Validation is not performed. You should know what to call!
		\return
		FColor value of variable
		*/
		FColor ToColor() const;

		/*!
		Converts Value to Vector4 representation
		Validation is not performed. You should know what to call!
		\return
		Vector4 value of variable
		*/
		Vector4 ToVector() const;

		/*!
		Converts Value to Matrix44 representation
		Validation is not performed. You should know what to call!
		\return
		Matrix44 value of variable
		*/
		Matrix44 ToMatrix() const;
	};

	/*!
	Extended type, which can hold list of flags
	*/
	struct PUBLIC_API MatSrcVarFlags: public MatSrcVar
	{
		ENF_DECLARE_SMALLOBJECT(MatSrcVarFlags);

		SArray<CStr, 32>	Flags;

		/*!
		Tells whether is certain flag set (included in array)
		\param flag
		Name of flag
		\return
		True when it is set, false otherwise
		*/
		bool IsFlagSet(CStr flag);

		/*!
		Sets certain flag (include it in array)
		\param flag
		Name of flag to be set
		*/
		void SetFlag(CStr flag);

		/*!
		Clears certain flag (remove it from array)
		\param flag
		Name of flag to be cleared
		*/
		void ClearFlag(CStr flag);

		//!Clears all flags
		void ClearFlags();

		MatSrcVarFlags& operator=(const MatSrcVarFlags& v);

		MatSrcVarFlags(CStr name):
			MatSrcVar(DVT_FLAGS, name)
		{
		}

		MatSrcVarFlags(DataVarType type, CStr name):
			MatSrcVar(type, name)
		{
		}
	};

	//!Extended type, which can hold list of textures.
	struct PUBLIC_API MatSrcVarTexture: public MatSrcVarFlags
	{
		ENF_DECLARE_SMALLOBJECT(MatSrcVarTexture);

		Array<const char*>	Textures;
		float				FPS;

		MatSrcVarTexture& operator=(const MatSrcVarTexture& v);

		/*!
		Adds one texture to array
		\param name
		Name of texture
		*/
		void AddTexture(CStr name);

		//!Clears array of textures
		void ClearTextures();

		MatSrcVarTexture(CStr name): MatSrcVarFlags(DVT_TEXTURE, name), FPS(0) {}
	};

	struct PUBLIC_API DeformFunc
	{
		int	m_Type;
		SArray<MatSrcVar, Material::CFunc::MAX_PARMS>	m_Variables;
		DeformFunc& operator=(const DeformFunc& f);
	};

private:

	friend class MaterialSourceCollection;
	MaterialSourceCollection* m_pCollection;

	uint						m_iFile;
	DString					m_strName;
	Array<MatSrcVar*>		m_Variables;
	Material*				m_pMaterial;
	MaterialClass*			m_pMClass;
	DString					m_strDescription;
	bool						m_bChanged;
	Array<DeformFunc>		m_DeformFuncs;

	void FreeVariables();
	MatSrcVar* AddVariable(const MaterialVarDef& desc);

	MatSrcVar* FindVariable(CStr name);
	uint GetVariableIndex(CStr name) const;

public:

	ENF_INLINE uint GetFile(){return m_iFile;}

	void SetFile(uint file)
	{
		m_iFile = file;
	}

	MaterialClass* GetMaterialClass() const
	{
		return m_pMClass;
	}

/** \name DeformFunc methods
	Deform function management, add/remove/update/get
*/
//@{
	/*!
	Adds deform fuction (See enf::MaterialDeform)
	\param def Enum definition of function (taken from enf::MaterialSystem::GetDeformFuncs )
	\return Number of functions
	*/
	uint AddDeformFunc(const ParmEnumMaterialDeform& def);

	/*!
	Inserts function at certain position
	\param def Enum definition of function (taken from enf::MaterialSystem::GetDeformFuncs )
	\param index Position at which is function inserted.
	*/
	uint InsertDeformFunc(const ParmEnumMaterialDeform& def, uint index);

	/*!
	Removes function
	\param index Index of function to be removed
	*/
	void RemoveDeformFunc(uint index)
	{
		m_DeformFuncs.RemoveElementOrdered(index);
		SetChanged();
	}

	/*!
	Updates function
	\param index Index of function to be updated
	\param func New function definition
	*/
	void UpdateDeformFunc(uint index, const DeformFunc& func)
	{
		m_DeformFuncs[index] = func;
		SetChanged();
	}

	/*!
	Changes variable of function to script variable reference
	\param index Index of function
	\param variable Index of variable to be set
	\param ref New value
	\return Pointer to updated variable
	*/
	MatSrcVar* SetDeformFuncVariable(uint index, uint variable, const VariableRef& ref);

	/*!
	Changes variable of function to constant value
	\param index Index of function
	\param variable Index of variable to be set
	\param value New value
	\return Pointer to updated variable
	*/
	MatSrcVar* SetDeformFuncVariable(uint index, uint variable, CStr value);

	/*!
	Gets one function
	\param index Index of function
	*/
	const DeformFunc& GetDeformFuncs(uint index) const
	{
		return m_DeformFuncs[index];
	}

	/*!
	Gets array of all functions
	\return Number of functions
	*/
	const Array<DeformFunc>& GetDeformFuncs() const
	{
		return m_DeformFuncs;
	}

//@}

/** \name Variables methods
	Variables management
*/
//@{

	/*!
	Gets number of variables
	\return Number of variables
	*/
	uint GetNumVariables() const
	{
		return m_Variables.GetCardinality();
	}

	/*!
	Gets variable by index
	\param variable Index of variable. Must be between 0 and GetNumVariables()
	\return Pointer to variable
	*/
	const MatSrcVar* GetVariable(uint variable) const
	{
		return m_Variables[variable];
	}

	/*!
	Finds variable by name in MaterialVarDef
	\param def Definition of variable
	\return Pointer to variable, or NULL
	*/
	MatSrcVar* GetVariable(const MaterialVarDef& def);

	/*!
	Deletes variable from this MaterialSource
	\param name Name of variable
	*/
	void DeleteVariable(CStr name);

	/*!
	Tells whether is the variable already allocated and set.
	\param name Name of variable
	\return True when it is, false otherwise
	*/
	bool IsVariableSet(CStr name) const
	{
		return (GetVariableIndex(name) != INDEX_NOT_FOUND);
	}

	/*!
	Sets variable to reference to script variable. If variable do not exists in
	MaterialSource, it's allocated.
	\param desc
	Description of variable
	\param ref
	Reference to variable
	\return
	Pointer to set MatSrcVar
	*/
	MatSrcVar* SetVariable(const MaterialVarDef& desc, const VariableRef& ref);

	/*!
	Sets variable to constant value. If variable do not exists in
	MaterialSource, it's allocated.
	\param desc
	Description of variable
	\param value
	New value of variable
	\return
	Pointer to set MatSrcVar
	*/
	MatSrcVar* SetVariable(const MaterialVarDef& desc, CStr value);

	/*!
	Sets variable to constant value. If variable do not exists in
	MaterialSource, it's searched in MaterialClass and allocated.
	\param name
	Name of variable
	\param value
	New value of variable
	\return
	Pointer to set MatSrcVar, or NULL when variable do not exists
	*/
	MatSrcVar* SetVariable(CStr name, CStr value);

	/*!
	Returns pointer to MatSrcVarTexture. If variable do not exists in
	MaterialSource, it's allocated.
	\param desc
	Description of variable
	\return
	Pointer to set MatSrcVarTexture
	*/
	MatSrcVarTexture* SetVariableTexture(const MaterialVarDef& desc);

	/*!
	Quickly sets one texture and returns pointer to MatSrcVarTexture.
	If variable do not exists in MaterialSource, it's searched in
	MaterialClass and allocated.
	\param name
	Name of variable
	\param value
	Name of texture
	\return
	Pointer to set MatSrcVarTexture
	*/
	MatSrcVarTexture* SetVariableTexture(CStr name, CStr value = NULL);

	/*!
	Returns pointer to MatSrcVarFlags. If variable do not exists in
	MaterialSource, it's allocated.
	\param desc
	Description of variable
	\return
	Pointer to set MatSrcVarTexture
	*/
	MatSrcVarFlags* SetVariableFlags(const MaterialVarDef& desc);
//@}

	/*!
	Changes MaterialClass attached to Material
	\param mc
	Pointer to MaterialClass
	*/
	void ChangeMaterialClass(MaterialClass* mc);

	/*!
	Changes name of this MaterialSource
	\param name
	New name of MaterialSource
	*/
	void SetName(CStr name);

	/*!
	Gets name of this MaterialSource
	\return
	The name
	*/
	CStr GetName() const
	{
		return m_strName.cstr();
	}

	/*!
	Gets associated Material
	\return
	Pointer to Material
	*/
	Material* GetMaterial() const
	{
		return m_pMaterial;
	}

	/*!
	Does almost the same as ::MaterialSource::operator= but it do
	not overwrite Name, Material and MaterialClass pointers. Usable
	for new materials, with cloned data from another one
	\param ms
	Reference to source
	\return 
	Self
	*/
	MaterialSource& CopyDefinitions(const MaterialSource& ms);

	/*!
	Sets description, visible in material editor
	\param desc
	Description. Line break is supplied as '|' character
	*/
	void SetDescription(CStr desc)
	{
		m_strDescription = desc.cstr();
		SetChanged();
	}

	/*!
	Gets description, visible in material editor
	\return
	Description. Line break is supplied as '|' character
	*/
	CStr GetDescription() const
	{
		return m_strDescription.c_str();
	}

	/*!
	Updates data of Material
	*/
	void UpdateMaterial();

	/*!
	Constructor. Constructs empty MaterialSource with defined name
	and attached to certain Material&MaterialClass pair.
	\param name
	Name of MaterialSource. Have to be the same as Material's name!
	\param mat
	Material we are attached to. Will be removed in future and Material
	will be created _after_ initializing MaterialSource
	\param matclass
	MaterialClass pointer. Must be specified as it is source of variable
	definitions!
	*/
	MaterialSource(CStr name, Material* mat, MaterialClass* matclass);

	/*!
	Tells whether something has changed since last ClearChanged call
	\return
	True when changed, false otherwise
	*/
	ENF_INLINE bool IsChanged() const
	{
		return m_bChanged;
	}

	/*!
	Sets changed status
	*/
	ENF_INLINE void SetChanged()
	{
		m_bChanged = true;
	}

	/*!
	Clears changed status.
	*/
	ENF_INLINE void ClearChanged()
	{
		m_bChanged = false;
	}

	/*!
	Re-initialize content.
	*/
	void Init(CStr name, Material* mat, MaterialClass* mc);

	/*!
	Empty constructor. Usable for clones of MaterialSources
	*/
	MaterialSource();

	/*!
	Destructor
	*/
	~MaterialSource();
};
//@}
}
#endif //ENF_MATERIALSOURCE_H