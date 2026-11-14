#ifdef _MSC_VER
#pragma once
#endif

#ifndef _EL_PARAM_ARCHIVE_DB_HPP
#define _EL_PARAM_ARCHIVE_DB_HPP

#include "paramArchive.hpp"

class ParamArchiveSave : public ParamArchive
{
protected:
	bool _saved;
	Ref<ClassDb> _file;

public:
	ParamArchiveSave(int version, const Array<void *> &params = noParams);
	~ParamArchiveSave();

  bool Parse(const char *name, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL)
  {
    return DoParse(name, false, false, parentVariables, globalVariables);
  }
	bool Save(const char *name)
  {
    return DoSave(name, false, false);
  }
  bool Save(QOStream &out)
  {
    return DoSave(out, false, false);
  }
  bool ParseBin(const char *name, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL)
  {
    return DoParse(name, true, false, parentVariables, globalVariables);
  }
  bool ParseBin(QIStream &in, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL)
  {
    return DoParse(in, true, false, parentVariables, globalVariables);
  }
	bool SaveBin(const char *name)
  {
    return DoSave(name, true, false);
  }
  bool SaveBin(QOStream &out)
  {
    return DoSave(out, true, false);
  }
  bool ParseSigned(const char *name, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL)
  {
    return DoParse(name, true, true, parentVariables, globalVariables);
  }
  bool SaveSigned(const char *name)
  {
    return DoSave(name, true, true);
  }
  Ref<ClassDb> &GetClassDbFile() {return _file;}

protected:
  bool DoParse(const char *name, bool binary, bool signature, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables);
  bool DoParse(QIStream &in, bool binary, bool signature, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables);
  bool DoSave(const char *name, bool binary, bool signature);
  bool DoSave(QOStream &out, bool binary, bool signature);
};

class ParamArchiveLoad : public ParamArchive
{
protected:
  bool _open;
  Ref<ClassDb> _file;

public:

  ParamArchiveLoad();
  ParamArchiveLoad(ParamArchiveSave &arch);
  ParamArchiveLoad(const char *name, const Array<void *> &params=noParams, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL);
  ParamArchiveLoad(QIStream &in, const Array<void *> &params=noParams, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL);
  ~ParamArchiveLoad();

  bool Load(const char *name, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL)
  {
    return DoLoad(name, false, false, parentVariables, globalVariables);
  }
  bool Load(QIStream &in, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL)
  {
    return DoLoad(in, false, false, parentVariables, globalVariables);
  }
  bool LoadBin(const char *name, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL)
  {
    return DoLoad(name, true, false, parentVariables, globalVariables);
  }
  bool LoadBin(QIStream &in, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL)
  {
    return DoLoad(in, true, false, parentVariables, globalVariables);
  }
  bool LoadSigned(const char *name, IEvaluatorVariables *parentVariables = NULL, IEvaluatorNamespace *globalVariables = NULL)
  {
    return DoLoad(name, true, true, parentVariables, globalVariables);
  }
  void Close();
  void CleanUp();

protected:
  bool DoLoad(const char *name, bool binary, bool signature, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables);
  bool DoLoad(QIStream &in, bool binary, bool signature, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables);
};

class ParamArchiveLoadEntry : public ParamArchive
{
protected:
	bool _open;

public:
	ParamArchiveLoadEntry(const ClassEntry *cfg, const Array<void *> &params=ParamArchiveLoad::noParams);
	~ParamArchiveLoadEntry();

	bool Load(const ClassEntry *cfg);
	void Close();

	void SetArVersion(int version) {_version = version;}
};

#endif

