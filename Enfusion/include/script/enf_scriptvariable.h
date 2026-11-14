//-----------------------------------------------------------------------------
// File: enf_scriptvariable.cpp
//
// Desc: Script variable class
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_SCRIPTVARIABLE_H
#define ENF_SCRIPTVARIABLE_H

#include "enf_config.h"
#include "common/enf_types.h"

namespace enf
{
/** @addtogroup EnforceScript
@{*/

class Instance;
class VarType;
class ScriptContext;
struct TToken;
class Class;
class ScriptThread;
struct FuncPtrDef;
class SerializeOutputAdapter;
class SerializeInputAdapter;
class ScriptModule;

const uint SCRIPT_INDEX_NOT_FOUND = 0xffffffff;


#define SCAPI(a) void NMETHOD a(enf::IVariable **parm,enf::Variable *result) //makro definice ne-nativni fce

//!Basic variable types
enum VType
{
	VARTYPE_NONE = 0,
	VARTYPE_BOOL,		///<Boolean (not used by script ATM)
	VARTYPE_INT,		///<Integer
	VARTYPE_FLOAT,		///<Float
	VARTYPE_VECTOR,	///<Pointer to float[3]
	VARTYPE_STRING,	///<Pointer to char[]
	VARTYPE_CLASS,		///<Script Class
	VARTYPE_VOID,		///<void
	VARTYPE_FUNCPTR,	///<Pointer to method. Internaly it's pointer to struct {Variable* classptr,uint index}
	VARTYPE_CLASSREF,
	VARTYPE_ARRAY,
	VARTYPE_USER0,		///<User defined type 0
	VARTYPE_USER1,		///<User defined type 1
	VARTYPE_USER2,		///<User defined type 2
	VARTYPE_USER3,		///<User defined type 3
	VARTYPE_USER4,		///<User defined type 4
	VARTYPE_USER5,		///<User defined type 5
	VARTYPE_USER6,		///<User defined type 6
	VARTYPE_USER7		///<User defined type 7
};

//!Variable specification
enum VariableSpec
{
	VSPEC_CONST			= BITF(0),			///<can't be modified
	VSPEC_GLOBAL		= BITF(1),			///<global
	VSPEC_PTR			= BITF(2),			///<array pointer
	VSPEC_PROTECT		= BITF(3),			///<just 'this' variable
	VSPEC_LOCAL			= BITF(4),			///<function locals
	VSPEC_PRIVATE		= BITF(5),			///<private member of class
	VSPEC_DEFAULT		= BITF(6),			///<default parm
	VSPEC_CLASSMEMBER	= BITF(7),			///<member of class
	VSPEC_STATIC		= BITF(8),
	/*!
	Referenced outside script core. Used for identifying in tools,
	that it can be referenced from Material for example.
	*/
	VSPEC_REFERENCE	= BITF(9),
	VSPEC_OWNED			= BITF(10),
	VSPEC_OUT			= BITF(11), //output parameter of function
	VSPEC_INOUT			= BITF(12), //input/output parameter of function
};

//!Value of variable
class PUBLIC_API IVariable
{
public:
	union
	{
		int		Int;
		float 	Float;
		void*		Pointer;
		char*		String;
		Instance*InstancePtr;
		FuncPtrDef*	FuncPtr;
	};

	IVariable(): Int(0) {}
};

//-----------------------------------------------------------------------
//!const, local, static of class-member Variable
//-----------------------------------------------------------------------
class PUBLIC_API Variable: public IVariable
{
friend class ScriptContext;
friend class ScriptModule;
friend class CCompiler;

private:
	const char*		Name;			///<Variable Name
	ushort			TypeIndex;
	ushort			TypeType;

public:
	Variable(): Name(NULL), TypeIndex(0xffff), TypeType(VARTYPE_VOID), Pos(-1), Spec(0), Module(NULL)
	{
		Dim[0] = Dim[1] = 0;
	}

	short				Pos;			///<Position in Class instance table, or -1
	ushort			Spec;			///<Declaration specs See enf::VariableSpec
   ushort			Dim[2];		///<Array size
	ScriptModule*	Module;

	//!Gets variable name
	const char* GetName() const
	{
		return Name;
	}

	ENF_INLINE uint GetTypeIndex() const
	{
		return (uint)TypeIndex;
	}

	VarType* GetType() const;

	void SetType(const VarType* vt);

	//!Gets variable type
	ENF_INLINE VType GetVType() const
	{
		return (VType)TypeType;
	}

	//!Tells whether is this variable pointer to array
	ENF_INLINE bool IsPtr() const { return (Spec & VSPEC_PTR) != 0; }

	//!Tells whether is this variable constant
	ENF_INLINE bool IsConst() const { return (Spec & VSPEC_CONST) != 0; }

	//!Tells whether is this variable function local
	ENF_INLINE bool IsLocal() const { return (Spec & VSPEC_LOCAL) != 0; }

	bool Set(const TToken* value, IVariable *ivar, int x = 0) const;
	bool Set(const char* value, IVariable *ivar, int x = 0) const;

	//!clears variable, allocated buffers are left allocated, they are just cleared
	void Zero(IVariable *cvar) const;

	//!Initialize. Just for 1st time!
	void Clear(IVariable* ivar) const;

	//!Frees content of variable (strings)
	void Cleanup(IVariable* ivar);

	//!Gets size of array, if this variable is array
	ENF_INLINE uint ArraySize() const
	{
		return Dim[0] * (Dim[1] ? Dim[1] : 1);
	}

};

//-----------------------------------------------------------------------------
//!Base for all script class-instances
//-----------------------------------------------------------------------------
class PUBLIC_API Instance
{
protected:
friend class ScriptContext;
friend class ScriptDebugger;
friend class Class;

	//!Type of instance. Used for RTTI and so on.
	const Class*	m_pcClass;
	//!Size of Instance. Copied from Class::m_iISizeof
	uint		m_iISizeof;

	virtual ~Instance();

	ENF_INLINE const Variable* _CallMethod(uint index, va_list list);

public:
	//char tempName[32];
	union
	{
		ScriptThread*		m_psThread;	//thread chain
		Instance*	m_pNextCached; //instance cache
	};

	virtual void Serialize(SerializeOutputAdapter* );
	virtual void Serialize(SerializeInputAdapter* );

	//!Gets type of the Instance.
	ENF_INLINE const Class* GetClass() const
	{
		return m_pcClass;
	}
	ENF_INLINE uint GetISizeof() const
	{
		return m_iISizeof;
	}
	bool InstanceOf(const Class* clss) const;

	//!Instance got new reference - support for GC
	virtual int AddInstanceRef() { return 0; }
	//!Instance lost a reference - support for GC
	virtual int ReleaseInstance() { return 0; }

	//!Gets variable value by its name, or NULL when not found
	const IVariable* GetIVariable(const char* name) const;

	//!Gets variable value by its name, or NULL when not found
	IVariable* GetIVariable(const char* name);

	//!Gets variable value by its index, or NULL when not found
	ENF_INLINE const IVariable* GetIVariable(uint index) const
	{
		//enf_assert!
		return &((IVariable*)((char *)this + m_iISizeof))[index];
	}
	
	//!Gets variable value by its index, or NULL when not found
	ENF_INLINE IVariable* GetIVariable(uint index)
	{
		//enf_assert!
		return &((IVariable*)((char *)this + m_iISizeof))[index];
	}

	//!For custom allocated instances, it initializes RTTI info
	void Init(const Class* cl);

	/*!
	Calls scripted method by its name. Parameters can be passed here,
	but they must have correct types!
	\param name
	Name of method
	\return
	Pointer to return variable, or NULL, when method do not exists
	*/
	const Variable* CallMethod(const char* name, ...);

	/*!
	Calls scripted method by its index. Parameters can be passed here,
	but they must have correct types!
	\param methodIndex Index of method
	\return Pointer to return variable, or NULL, when method do not exists
	*/
	const Variable* CallMethod(uint methodIndex, ...);

	//!Dummy constructor for inherited classes with already initialized members
	Instance(const Class* cl);
	Instance();

	/*!
	Deletes instance. It passes the call to Class::Delete
	where you have to handle it
	*/
	virtual void Delete();
};

//helper macro
#ifdef _DEBUG
#define ENF_CALLMETHOD_INDEX(inst, name, var) static enf::uint var = enf::INDEX_NOT_FOUND; \
	if(var == enf::INDEX_NOT_FOUND) var = inst->GetClass()->GetMethodIndex(name); \
	enf_assert(var != enf::INDEX_NOT_FOUND);\
	enf_assert(enf::CStr(inst->GetClass()->GetMethod(var)->GetName()) == name);
#else
#define ENF_CALLMETHOD_INDEX(inst, name, var) static enf::uint var = enf::INDEX_NOT_FOUND; \
	if(var == enf::INDEX_NOT_FOUND) var = inst->GetClass()->GetMethodIndex(name); enf_assert(var != enf::INDEX_NOT_FOUND);
#endif

//@}
}
#endif //ENF_SCRIPTVARIABLE_H
