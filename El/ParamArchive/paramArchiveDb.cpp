#include <El/elementpch.hpp>
#include "paramArchiveDb.hpp"
#include <El/Enum/enumNames.hpp>

ParamArchiveLoad::ParamArchiveLoad()
{
	_open = false;
	_saving = false;
	_pass = PassFirst;
	_params = NULL;
	_file = _defaultClassDbFunctions->CreateDatabase();
	_entry = _file->GetEntry();
}

ParamArchiveLoad::ParamArchiveLoad(const char *name, const Array<void *> &params, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
	_open = false;
	_saving = false;
	_pass = PassFirst;
	_params = params;
	_file = _defaultClassDbFunctions->CreateDatabase();
	_entry = _file->GetEntry();
	Load(name, parentVariables, globalVariables);
}

ParamArchiveLoad::ParamArchiveLoad(QIStream &in, const Array<void *> &params, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  _open = false;
  _saving = false;
  _pass = PassFirst;
  _params = params;
  _file = _defaultClassDbFunctions->CreateDatabase();
  _entry = _file->GetEntry();
  Load(in, parentVariables, globalVariables);
}

ParamArchiveLoad::ParamArchiveLoad(ParamArchiveSave &arch)
{
  _open = true;
  _saving = false;
  _pass = PassFirst;
  _params = NULL;
  //_file = _defaultClassDbFunctions->CreateDatabase();
  _file = arch.GetClassDbFile();
  _version = arch.GetArVersion();
  _entry = _file->GetEntry();
}

ParamArchiveLoad::~ParamArchiveLoad()
{
	CleanUp();
}

bool ParamArchiveLoad::DoLoad(const char *name, bool binary, bool signature, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
	if (_open) Close();
	if (!_file->Load(name, binary, signature, parentVariables, globalVariables)) return false;
	SRef<ClassEntry> entry = _entry->FindEntry("version");
	if (!entry)
	{
		_version = 0;
	}
	else
	{
	  _version = *entry;
	}
	_open = true;
	return true;
}

bool ParamArchiveLoad::DoLoad(QIStream &in, bool binary, bool signature, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  if (_open) Close();
  if (!_file->Load(in, binary, signature, parentVariables, globalVariables)) return false;
  SRef<ClassEntry> entry = _entry->FindEntry("version");
  if (!entry)
  {
    _version = 0;
  }
  else
  {
    _version = *entry;
  }
  _open = true;
  return true;
}

void ParamArchiveLoad::CleanUp()
{
	ParamArchive::Close();
	//_file->Clear();
  _file=NULL; //Ref
	_open = false;
}

void ParamArchiveLoad::Close()
{
	if (!_open) return;
	CleanUp();
	_open = false;
}

ParamArchiveLoadEntry::ParamArchiveLoadEntry(const ClassEntry *cfg, const Array<void *> &params)
{
	_open = false;
	_saving = false;
	_pass = PassFirst;
	_params = params;
	Load(cfg);
}

ParamArchiveLoadEntry::~ParamArchiveLoadEntry()
{
	Close();
}

bool ParamArchiveLoadEntry::Load(const ClassEntry *cfg)
{
	if (_open) Close();
	_entry = const_cast<ClassEntry *>(cfg);
	SRef<ClassEntry> entry = _entry->FindEntry("version");
	if (!entry)
	{
		_version = 0;
	}
	else
	{
		_version = *entry;
	}
	_open = true;
	return true;
}

void ParamArchiveLoadEntry::Close()
{
	if (!_open) return;
	ParamArchive::Close();
	_entry.Free();
	_open = false;
}

ParamArchiveSave::ParamArchiveSave(int version, const Array<void *> &params)
{
	_saved = false;
	_saving = true;
	_version = version;
	_params = params;
	_file = _defaultClassDbFunctions->CreateDatabase();
	_entry = _file->GetEntry();
	if (version>0)
	{
	  _entry->Add("version", version);
	}
}

ParamArchiveSave::~ParamArchiveSave()
{
	ParamArchive::Close();

	if (!_saved)
		LogF("Warning: Archive not saved");
}

bool ParamArchiveSave::DoParse(const char *name, bool binary, bool signature, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
	// used for config files - not all values will be serialized
	bool result = _file->Load(name, binary, signature, parentVariables, globalVariables);
	if (_version>0)
	{
	  _entry->Add("version", _version);
	}
	return result;
}

bool ParamArchiveSave::DoParse(QIStream &in, bool binary, bool signature, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  // used for config files - not all values will be serialized
  bool result = _file->Load(in, binary, signature, parentVariables, globalVariables);
  if (_version>0)
  {
    _entry->Add("version", _version);
  }
  return result;
}

bool ParamArchiveSave::DoSave(const char *name, bool binary, bool signature)
{
	if (_saved)
		LogF("Warning: Archive saved twice");

	_saved = _file->Save(name, binary, signature);
	return _saved;
}

bool ParamArchiveSave::DoSave(QOStream &out, bool binary, bool signature)
{
  if (_saved)
    LogF("Warning: Archive saved twice");

  _saved = _file->Save(out, binary, signature);
  return _saved;
}
