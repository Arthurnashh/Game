#ifdef _MSC_VER
#pragma once
#endif

#ifndef _I_EVAL_HPP
#define _I_EVAL_HPP

#include <El/QStream/serializeBin.hpp>

//! interface for passing variables between various function 
/*!
this is an empty interface - it is used only for type safety.
Functions may pass this safely, but implementation is completely hidden.
*/
class IEvaluatorVariables
{
};

//! interface for passing global namespace between various function 
class IEvaluatorNamespace
{
};

//! default evaluator
class Evaluator
{
public:
	//! virtual destructor
	virtual ~Evaluator() {}

	//! callback function to initialize expression evaluator
	virtual int BeginContext() { return 0; }
	//! callback function to deinitialize expression evaluator
	virtual void EndContext(int gshandle) {}
	//! callback function to load state from binary stream
	virtual void LoadVariables(SerializeBinStream &f)
	{
		int n;
		f.Transfer(n);
		Assert(n == 0);
	}
	//! callback function to save variables into binary stream
	virtual void SaveVariables(SerializeBinStream &f)
	{
		int n = 0;
		f.Transfer(n);
	}
	//! callback function to delete all variables
	virtual void DeleteVariables()
	{
	}

	// External functions (used outside BeginContext ... EndContext)
	//! callback function to evaluate float expression
	virtual float EvaluateFloat(const char *expr) {return 0;}

	// Internal functions (used inside BeginContext ... EndContext)
	//! callback function to evaluate float expression without game state initialization and deinitialization
	virtual float EvaluateFloatInternal(const char *expr) {return 0;}
	//! callback function to evaluate string expression without game state initialization and deinitialization
	virtual RString EvaluateStringInternal(const char *expr) {return expr;}
	//! callback function to execute expression without game state initialization and deinitialization
	virtual void ExecuteInternal(const char *expr) {}
	//! callback function to set value of float variable without game state initialization and deinitialization
	virtual void VarSetFloatInternal(const char *name, float value, bool readOnly, bool forceLocal) {}
};

//! class of callback functions
class EvaluatorFunctions
{
public:
	//! virtual destructor
	virtual ~EvaluatorFunctions() {}

	//! callback function to create evaluator
	virtual Evaluator *CreateEvaluator(IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables, bool enableSerialization) {return new Evaluator;}
};

#endif
