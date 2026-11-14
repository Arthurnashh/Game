//-----------------------------------------------------------------------------
// File: enf_scriptcompiler.h
//
// Desc: Script Compiler
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_SCRIPTCOMPILER_H
#define ENF_SCRIPTCOMPILER_H

#include "enf_config.h"
#include "script/enf_scriptinternal.h"

namespace enf
{
/** @addtogroup EnforceScript
@{*/

struct TLine;
class CParser;

//-----------------------------------------------------------------------
// Compiler class
//-----------------------------------------------------------------------
class PUBLIC_API CCompiler
{
	enum CommandOp
	{
		CMD_BAD = -1,
		CMD_IF = 1,
		CMD_ELSE,
		CMD_END,
		CMD_BREAK,
		CMD_CONTINUE,
		CMD_SWITCH,
		CMD_CASE,
		CMD_DEFAULT,
		CMD_GOTO,
		CMD_RETURN,
		CMD_PROTO,
		CMD_CLASS,
		CMD_DELETE,
		CMD_CLAMP,
		CMD_FOR,
		CMD_WHILE,
		CMD_DEBUG,
		CMD_TYPEDEF,
		CMD_CACHE,
		CMD_WAIT,
		CMD_SLEEP,
		CMD_FORCE_DWORD = 0x7fffffff
	};

friend class ScriptListing;
friend class CCompilerContext;

	ScriptContext*	m_pContext;
	ScriptModule* m_pModule;

	static const uint	MAX_STATS = 16384;

	HashMap<CStr, CommandOp, CStrHashFuncCS> m_Keywords;

	static CParser*			m_pParser;
	static bool					m_bRes;

	Stat*				m_psStats;
	Stat*				m_pLastRts;

	int				m_iNumStats;
	int				m_iStatsPerFunc;

	enum ScopeType
	{
		REF_NULL,
		REF_CLASS,
		REF_STRUCT,
		REF_FUNC,
		REF_IF,
		REF_ELSE,
		REF_WHILE,
		REF_SWITCH,
		REF_LABEL,
		REF_GOTO,
		REF_BLOCK,
		REF_FORCE_DWORD = 0x7fffffff
	};

	// References
	struct Scope
	{
		ScopeType		Type;

		//array of 'break' references
		Stat**		BRefs;
		uint			NumBRefs;

		Stat*			RStat[2][8];
		Stat*			Stat2;	//usefull for while/for loops
		Variable*	Var;		//usefull for switch
		int			Lines;
		const char*	Name;

		void AddStat(int t, Stat *st);

		void SolveStat(int t, Stat *st)
		{
			for(int n = 0; n < 8 && RStat[t][n] != NULL; n++)
			{
				RStat[t][n]->SetJmp(st);
				RStat[t][n] = NULL;
			}
		}
	};

	SArray<Scope, 256>	m_TRefs;
	SArray<Stat*, 256>	m_BRefs;
	Set<Variable*> m_UsedReturns;

	//temporary
	//Compiler globals

	enum post_e {PS_NONE, PS_INC, PS_DEC} ;

	class TVariable: public Variable
	{
	public:
		void**	IClass;
		Variable*OrgVar;
		TVariable(): Variable() {}
	};

	static const int MAX_REFS = 1024;

	TVariable	m_RefVars[MAX_REFS];
	int			m_iNumRefVars;

	uint			m_iFirstDebugLine;
	uint			m_iFunctionIndex;
	Funct*		m_pCFunct;
	Class*		m_pCClass;
	bool			m_bMethodOutsideClass;
	uint			m_iIndexCount;

	const TLine*m_pDLine;
	const char*	m_pCSymbol;
	Stat*			m_PrevCStat;
	Variable*	m_pObj;

	uint			m_iFuncFlags;
	short			m_iCVSpec;
	VarType*		m_pCVarType;

	static int	m_iSLen[];
	static bool	m_bInitialized;

	Stat			m_Stats[MAX_STATS];

	Scope			m_rRefs[256];
	int			m_iNumScopes;
	uint			m_iLastRef;

	uint			m_iVRegMask;
	uint			m_iSRegMask;
	uint			m_iCRegMask;
	uint			m_iFRegMask;
	uint			m_iRRegMask;
	size_t		m_iAllocated;

	char*			LastError;

	const static char	NotInBlockError[];
	const static char	BlockError[];
	const static char	ParmError[];
	const static char	TooMuchParmError[];
	const static char	BadVarError[];
	const static char	NotVarError[];
	const static char	AddVarError[];
	const static char	MultDecError[];
	const static char	BadStatError[];
	const static char	IncParmError[];
	const static char	OperatorError[];
	const static char	ElseError[];
	const static char	DupLabelError[];
	const static char	Reference2Error[];
	const static char	ProtoError[];
	const static char	ExtendsError[];
	const static char	ClassExistsError[];
	const static char	FuncNotCompError[];
	const static char	DestructorError[];
	const static char	BreakContError[];
	const static char	InitVarError[];
	const static char	TooManyValsError[];
	const static char	NotClassPtrError[];
	const static char	NotRetValueError[];
	const static char	ConstModifyError[];
	const static char	ThisError[];
	const static char	ThisStaticError[];
	const static char	LocalVarError[];
	const static char	FuncInStruct[];
	const static char	TooMuchVariables[];
	const static char MixedArithmetic[];
	const static char MethodIsPrivate[];
	const static char UndefinedFunction[];

	Variable*	GetRegister(VarType *vt);
	void			FreeAllRegisters();
	void			FreeRegister(Variable* reg);

	bool IsRegister(Variable* var)
	{
		if(var->GetName() && var->GetName()[0] == '@')
			return true;

		return false;
	}

	Stat*			AddStat(OpCode cmd, int size = 0);

	Class*		AddClass(const char *name);
	bool			ExtendClass(Class* cl, Class* parent);

	Variable*	ParseVariable(VarType *type, Variable **ivar, bool lside = true);
	Variable*	ParseRValue(Variable *var3, Variable *ivar3, bool justreturn = false);
	Funct*		FindFunction(const char *name, Class *cl, bool onlyclass = false);
	Variable*	FindVariable(const char *name,const Funct* func, const Class* cl, bool onlyclass = false);

	//scope
	Scope*		FindScope(ScopeType type);
	Scope*		BeginScope(ScopeType type);
	void			AddBreakRef(Scope* sc, Stat* st);
	Scope*		EndScope();
	ENF_INLINE bool IsScopeType(ScopeType type) const
	{
		return (m_rRefs[m_iNumScopes].Type == type);
	}
	//reference variables
	Variable*	AddRefVar(Variable *v, Instance **ptr);
	ENF_INLINE bool IsRefVar(const Variable *v)
	{
		return (v->Pos >= 0);
	}

	//stack
	void			Pop(Variable* var);
	void			Push(Variable* var);
	void			BeforeCall(Funct* fn);
	void			AfterCall(Funct* fn);
	
	bool			AreTypesRelated(const Variable* outVar, const Variable* inVar);
	bool			CheckClassCompatibility(Class* cl);
	bool			CheckFunctionCompatibility(Funct* fn1, Funct* fn2);
	Funct*		AddFunction(const char* name, uint type, Class* cl);
	Variable*	AddVariable(const char* name, const VarType* type, Funct* func, Class* cl, int dim, short spec);
	void*			EncodeParm(const Variable* var);
	OpCode		FindSign(const TToken* sign, VType type) const;
	OpCode		Negate(OpCode op);
	Variable*	DoConversion(Variable* out, Variable* oidx, Variable* in, Variable* iidx, int move, bool force = false);
	Variable*	CompileID(Variable* dest, Variable* src, post_e symb);

	int			IsType();
	int			NextCondition();
	Variable*	Compute(OpCode cmd, Variable* out, Variable* in1, Variable* in2);
	void			SetParm(Stat* st, int a, void* parm, bool immed = false);
	uint EncodeCallRef(Funct* fn, uint functionIndex, Variable* objptr);

	//parsers
	bool IsFunctionDef();
	bool IsConstructorOrDestructor(const Class* cl, const Funct* fn);

	Variable* ParseFunctionCall(Variable *optr, bool thread);
	Variable* ParseConstructorCall(Class *cl);
	bool ParseArraySize(int& dim);
	Variable* ParseRVariable(VarType *vt);
	bool ParseIndex(Variable *var, Variable **ivar);
	bool ParseFuncParm(Funct *f, Variable *var3,Variable **vp);
	bool ParseEquation();
	bool ParseClassDef();
	bool ParseConditions(CommandOp inst);
	bool ParseFor();
	bool ParseCase();
	bool ParseBreakContinue(CommandOp inst);
	bool ParseEndOfFunc();
	bool ParseProto();
	bool ParseDelete();
	bool ParseTypeDef();
	bool ParseClamp();

	bool ParseFunctionDef(const VarType* rtype, uint funcFlags);
	bool ParseFunctParms(const VarType* rt, const Funct* pf = NULL);
	bool ParseVarDef(int init, bool multiple, const VarType* vt, short vspec, int num = 0, Variable* avar = NULL);
	bool EndOfValue();

	bool IsExpression(bool left) const;

	const char* FindString(const char* name) const;
	const char* AddString(const char* name) const;

public:
	int			m_iOptimize;
	bool			m_bList;

	ScriptContext* GetContext() const
	{
		return m_pContext;
	}
	void Begin(ScriptContext* ctx, ScriptModule* mod, const char* listingName);
	void End();
	bool CompileFragment(CParser& tparse, Class* thisClass = NULL);
	bool Compile(CParser& tparse, ScriptContext* ctx, ScriptModule* mod, const char* listingName);

	CCompiler();
};

class PUBLIC_API ScriptListing
{
private:
	static void*						m_fFile;
	static const ScriptModule*		m_pModule;
	static const CCompiler*			m_pCompiler;
	static uint m_iDebugOffset;
	static const Variable* GetVar(const void *v);
	static void DumpFunctionCall(const Class* cl, const Funct* fn, const void** parms);

public:
	static bool Init(const CCompiler* compiler, const char* name);
	static void Close();
	static void Add(int num);
	static void Add(const Class* cl, const Funct* fn);
	static void Add(const Variable *var);
	static void Add(const Stat *st);
	static void AddSourceLine(uint line);
	static void Add(const char *str);
	static void Add(float num);
};
//@}
}

#endif //ENF_SCRIPTCOMPILER_H