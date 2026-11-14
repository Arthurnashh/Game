//-----------------------------------------------------------------------------
// File: enf_scriptmain.h
//
// Desc: Main script context, holds and manage compiled image
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_SCRIPTTYPE_H
#define ENF_SCRIPTTYPE_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "script/enf_scriptvariable.h"
#include "common/enf_hashmap.h"
#include "common/enf_strings.h"

namespace enf
{
/** @defgroup EnforceScript Enforce script
@{*/

class Funct;
class Variable;
class ScriptModule;

//-----------------------------------------------------------------------
//!Script Variable type
//-----------------------------------------------------------------------
class PUBLIC_API VarType
{
	friend class ScriptContext;
	friend class ScriptModule;
	friend class CCompiler;

public:
	ENF_DECLARE_SMALLOBJECT(VarType);

	//!Position in array of variable types
	int				Index;
	//!Basic type
	VType				Type;
	//!Array dimensions if any
	ushort			Dim[2];
	//!Size of the variable type
	int				Sizeof;

	//!Gets owner module
	ScriptModule* GetModule() const
	{
		return Module;
	}

	//!Gets variable type name
	ENF_INLINE const char* GetName() const
	{
		return Name;
	}

	VarType(ScriptModule* mod, VType type, const char* name);
	virtual ~VarType() {}

protected:
	const char*		Name;
	ScriptModule*	Module;
};

enum ClassFlags
{
	CF_FORWARD_DECLARATION	= BITF(0),
	CF_BOUND						= BITF(1),
	CF_NATIVE					= BITF(2),
};

//-----------------------------------------------------------------------
//!Script class definition
//-----------------------------------------------------------------------
class PUBLIC_API Class: public VarType
{
protected:
	friend class CCompiler;
	friend class ScriptContext;
	friend class ScriptModule;
	friend class Instance;

	HashMap<CStr, uint, CStrHashFuncCS>	m_MethodsHash;
	HashMap<CStr, uint, CStrHashFuncCS>	m_VariablesHash;

	Class*		TopSuper;				///<Topmost super class
	Class*		Super;					///<Super class
	Array<Variable*>	m_Variables;
	Array<Funct*>		m_Functions;
	uint			ISizeof;					///<sizeof(Instance) + native extensions. enf::Entity for example
	uint			VSizeof;					///<sizeof all IVariables. They are just behind ISizeof offset
	ushort		Constructor;			///<Constructor method
	ushort		Destructor;				///<Destructor methos
	uint			m_iFlags;
	mutable Instance*	Cached;					///<Runtime cache of instances
	mutable ushort		NumCached;				///<Number of cached instances
	mutable ushort		MaxCached;				///<Max allowed number of cached instances

public:
#ifdef ENF_DEVELOPER
	mutable TimeStamp	Time;						///<Profiler related
#endif
	ENF_DECLARE_SMALLOBJECT(Class);

	ENF_INLINE Class* GetSuper() const
	{
		return Super;
	}

	ENF_INLINE Class* GetTopSuper() const
	{
		return TopSuper;
	}

	ENF_INLINE ushort GetConstructor() const
	{
		return Constructor;
	}

	void AddVariable(Variable* variable);
	void AddMethod(Funct* method);

	Variable* GetVariable(const char* name) const;
	const Variable* GetVariable(uint index) const
	{
		return m_Variables[index];
	}
	Variable* GetVariable(uint index)
	{
		return m_Variables[index];
	}
	ENF_INLINE uint GetNumVariables() const
	{
		return m_Variables.GetCardinality();
	}
	ENF_INLINE uint GetVariableIndex(const char* name) const
	{
		Variable* var = GetVariable(name);
		return (var == NULL) ? SCRIPT_INDEX_NOT_FOUND : var->Pos;
	}

	uint GetMethodIndex(const char *name) const;

	ENF_INLINE Funct* GetMethod(uint index) const
	{
		return m_Functions[index];
	}

	ENF_INLINE Funct* GetMethod(const char *name) const
	{
		uint index = GetMethodIndex(name);
		return (index == SCRIPT_INDEX_NOT_FOUND) ? NULL : m_Functions[index];
	}
	ENF_INLINE uint GetNumMethods() const
	{
		return m_Functions.GetCardinality();
	}

	bool IsInherited(const VarType* vt) const;	///<Returns true if type is our parent
	bool IsInherited(const char* cname) const;	///<Returns true if type is our parent

	virtual Instance* New(const void* init) const;
	virtual void Initialize(Instance* inst, const void* init) const;
	virtual void Delete(Instance* inst) const;
	virtual ~Class() {}
	/*!
	Creates new instance of Class. It's necessary for in-script defined
	classes that extends native class. Usually it creates the same class
	as the native one.
	\param mod
	Module
	\param name
	Name of Class
	\return
	New instance
	*/
	virtual Class* NewInheritedClass(ScriptModule* mod, const char* name);
	Class(ScriptModule* mod, const char* name, uint instancesize, Class* super);

	uint GetSizeof() const
	{
		return ISizeof + VSizeof;
	}
};

class PUBLIC_API TemplatedClass: public Class
{
protected:
	Array<Variable> m_Parameters;

public:
	ENF_DECLARE_SMALLOBJECT(TemplatedClass);
};

//@}

//!Helper macro for class definition
#define ENF_DECLARE_CLASS(classname, objectname, parentclass)\
	virtual Class* NewInheritedClass(enf::ScriptModule* mod, const char* name) {\
	return ENF_NEW classname(mod, name, sizeof(objectname), this);}\
	classname(enf::ScriptModule* mod, const char* name, enf::uint instanceSize, enf::Class* parent); \
	ENF_DECLARE_MMOBJECT(classname)

//!Helper macro for class definition, with implemented constructor
#define ENF_DECLARE_CLASS_IMP(classname, objectname, parentclass)\
	virtual Class* NewInheritedClass(enf::ScriptModule* mod, const char* name) {\
	return ENF_NEW classname(mod, name, sizeof(objectname), this);}\
	classname(enf::ScriptModule* mod, const char* name, enf::uint instanceSize, enf::Class* parent): parentclass(mod, name, instanceSize, parent) {} \
	ENF_DECLARE_MMOBJECT(classname)

}
#endif //ENF_SCRIPTTYPE_H
