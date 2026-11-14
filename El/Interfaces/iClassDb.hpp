#ifdef _MSC_VER
#pragma once
#endif

#ifndef _I_CLASS_DB_HPP
#define _I_CLASS_DB_HPP

#include "iEval.hpp"

#include <Es/Strings/rString.hpp>
#include <El/ParamArchive/serializeClass.hpp>

//! default array item
class ClassArrayItem
{
public:
	//! virtual destructor
	virtual ~ClassArrayItem() {}

	virtual operator float() const {return 0;}
	virtual operator int() const {return 0;}
	virtual operator RStringB() const {return RStringB();}
  virtual operator RString() const {return RString();}
	virtual operator bool() const {return false;}
};

//! default class entry
class ClassEntry
{
public:
	//! virtual destructor
	virtual ~ClassEntry() {}

	// generic entry
	virtual int GetEntryCount() const {return 0;}
	virtual ClassEntry *GetEntry(int i) const {return NULL;}

	virtual ClassEntry *FindEntry(const RStringB &name) const {return NULL;}
	virtual operator float() const {return 0;}
	virtual operator int() const {return 0;}
//#if _HELISIM_LARGE_OBJECTID
  virtual operator int64() const {return 0;}
//#endif // _HELISIM_LARGE_OBJECTID
	virtual operator RStringB() const {return RStringB();}
  virtual operator RString() const {return RString();}
  virtual operator bool() const {return false;}
	virtual RString GetContext(const char *member = NULL) const {return RString();}

	// array
	virtual void ReserveArrayElements(int count) {}
	virtual void AddValue(float val) {}
	virtual void AddValue(int val) {}
//#if _HELISIM_LARGE_OBJECTID
  virtual void AddValue(int64 val) {}
//#endif
	virtual void AddValue(const RStringB &val) {}
	virtual int GetSize() const {return 0;}
	virtual ClassArrayItem *operator [] (int i) const {return new ClassArrayItem();}

	// class
	virtual ClassEntry *AddArray(const RStringB &name) {return new ClassEntry;}
	virtual ClassEntry *AddClass(const RStringB &name, bool guaranteedUnique = false) {return new ClassEntry;}
	virtual void Add(const RStringB &name, const RStringB &val) {}
	virtual void Add(const RStringB &name, float val) {}
	virtual void Add(const RStringB &name, int val) {}
//#if _HELISIM_LARGE_OBJECTID
  virtual void Add(const RStringB &name, int64 val) {}
//#endif // _HELISIM_LARGE_OBJECTID
	virtual void Compact() {}

  //! Delete the entry. Note: it could be used in rare cases only!
  virtual void Delete(const RStringB &name) {}
};

class QOStream;
class QIStream;
//! default class database
class ClassDb : public RefCount
{
public:
	//! virtual destructor
	virtual ~ClassDb() {}

	virtual ClassEntry *GetEntry() {return new ClassEntry;}
  virtual bool Load(const char *name, bool binary, bool signature, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables = NULL) {return true;}
  virtual bool Load(QIStream &in, bool binary, bool signature, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables = NULL) {return true;}
	virtual bool Save(const char *name, bool binary, bool signature) {return true;}
  virtual bool Save(QOStream &out, bool binary, bool signature) {return true;}
	virtual void Clear() {}
};

//! class of callback functions
class ClassDbFunctions
{
public:
	//! virtual destructor
	virtual ~ClassDbFunctions() {}

	//! callback function to create class database
	virtual ClassDb *CreateDatabase() {return new ClassDb;}

	virtual LSError ErrorNoEntry() {return LSError(0);}
	virtual LSError ErrorStructure() {return LSError(0);}
};

#endif
