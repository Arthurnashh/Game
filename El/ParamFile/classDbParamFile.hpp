#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CLASS_DB_PARAM_FILE_HPP
#define _CLASS_DB_PARAM_FILE_HPP

#include <El/Interfaces/iClassDb.hpp>
#include <El/Interfaces/iEval.hpp>

#include "paramFile.hpp"

#include <Es/Memory/normalNew.hpp>

//! default array item
class ParamClassArrayItem : public ClassArrayItem
{
protected:
	const IParamArrayValue &_item;

public:
	ParamClassArrayItem(const IParamArrayValue &item) : _item(item) {}

	virtual operator float() const {return _item;}
	virtual operator int() const {return _item;}
	virtual operator RStringB() const {return _item;}
  virtual operator RString() const {return _item;}
	virtual operator bool() const {return _item;}

	USE_FAST_ALLOCATOR
};

//! default class entry
class ParamClassEntry : public ClassEntry
{
protected:
	ParamEntryVal _entry;

public:
	ParamClassEntry(ParamEntryVal entry) : _entry(entry) {}
	ParamClassEntry(ParamClass &file) : _entry(ParamEntryVal(file)) {}

	// generic entry
	virtual ClassEntry *FindEntry(const RStringB &name) const
	{
		ConstParamEntryPtr entry = _entry.FindEntry(name);
		if (!entry) return NULL;
		return new ParamClassEntry(ParamEntryVal(*entry.GetClass(),*entry.GetPointer()));
	}
	virtual int GetEntryCount() const {return _entry.GetEntryCount();}
	virtual ClassEntry *GetEntry(int i) const
	{
		ParamEntryVal entry = _entry.GetEntry(i);
		return new ParamClassEntry(entry);
	}

	virtual operator float() const {return _entry;}
	virtual operator int() const {return _entry;}
//#if _HELISIM_LARGE_OBJECTID
  virtual operator int64() const {return _entry;}
//#endif
	virtual operator RStringB() const {return _entry;}
  virtual operator RString() const {return _entry;}
  virtual operator bool() const {return _entry;}
	virtual RString GetContext(const char *member = NULL) const {return _entry.GetContext(member);}

	// array
	virtual void ReserveArrayElements(int count) {_entry.GetModPointer()->ReserveArrayElements(count);}
	virtual void AddValue(float val) {_entry.GetModPointer()->AddValue(val);}
	virtual void AddValue(int val) {_entry.GetModPointer()->AddValue(val);}
//#if _HELISIM_LARGE_OBJECTID 
  virtual void AddValue(int64 val) {_entry.GetModPointer()->AddValue(val);}
//#endif
	virtual void AddValue(const RStringB &val) {_entry.GetModPointer()->AddValue(val);}
	virtual int GetSize() const {return _entry.GetSize();}
	virtual ClassArrayItem *operator [] (int i) const
	{
		return new ParamClassArrayItem(_entry[i]);
	}

	// class
	virtual ClassEntry *AddArray(const RStringB &name)
	{
		ParamEntryPtr entry = _entry.GetModPointer()->AddArray(name);
		if (!entry) return NULL;
		return new ParamClassEntry(ParamEntryVal(*entry.GetClass(),*entry.GetPointer()));
	}
	virtual ClassEntry *AddClass(const RStringB &name, bool guaranteedUnique = false)
	{
		ParamClassPtr entry = _entry.GetModPointer()->AddClass(name, guaranteedUnique);
		if (!entry) return NULL;
		return new ParamClassEntry(ParamEntryVal(*entry));
	}
	virtual void Add(const RStringB &name, const RStringB &val) {_entry.GetModPointer()->Add(name, val);}
	virtual void Add(const RStringB &name, float val) {_entry.GetModPointer()->Add(name, val);}
	virtual void Add(const RStringB &name, int val) {_entry.GetModPointer()->Add(name, val);}
//#if _HELISIM_LARGE_OBJECTID
  virtual void Add(const RStringB &name, int64 val) {_entry.GetModPointer()->Add(name, val);}
//#endif // _HELISIM_LARGE_OBJECTID
	virtual void Compact() {_entry.GetModPointer()->Compact();}
  //! Delete the entry. Note: it could be used in rare cases only!
  virtual void Delete(const RStringB &name) { _entry.GetModPointer()->Delete(name); }

	USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

//! default class database
class ParamClassDb : public ClassDb
{
protected:
	ParamFile _file;

public:
	ParamClassDb();
	~ParamClassDb();
	
	virtual ClassEntry *GetEntry() {return new ParamClassEntry(ParamEntryVal(_file));}
	virtual bool Load(const char *name, bool binary, bool signature, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables = NULL);
  virtual bool Load(QIStream &in, bool binary, bool signature, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables = NULL);
	virtual bool Save(const char *name, bool binary, bool signature);
  virtual bool Save(QOStream &out, bool binary, bool signature);
	virtual void Clear() {_file.Clear();}
};

//! class of callback functions
class ParamClassDbFunctions : public ClassDbFunctions
{
public:
	//! callback function to create class database
	virtual ClassDb *CreateDatabase() {return new ParamClassDb;}

	virtual LSError ErrorNoEntry() {return LSNoEntry;}
	virtual LSError ErrorStructure() {return LSStructure;}
};

#endif
