#include <El/elementpch.hpp>
#include "paramArchive.hpp"
#include <El/Enum/enumNames.hpp>

#if _ENABLE_REPORT
void TraceError(const char *command)
{
	RptF("Error in statement: %s", command);
	//_asm int 3;
}
#endif

#define CHECK_INTERNAL(command) \
	{LSError err = command; if (err != LSError(0)) return err;}

ParamArchive::ParamArchive()
{
	_version = -1; // invalid
	_pass = PassUndefined;
	_params = NULL;
}
void ParamArchive::Close()
{
	_entry.Free();
}
ParamArchive::~ParamArchive()
{
}


RString ParamArchive::_errorContext;

const AutoArray<void *> ParamArchive::noParams;

void *ParamArchive::SetParams(void *params, int slot)
{
  _params.Access(slot);
  void *oldParams=_params[slot];
  _params[slot] = params;
  return oldParams;
}
void *ParamArchive::GetParams(int slot)
{
  if (_params.Size()<=slot) return NULL;
  return _params[slot];
}

void ParamArchive::OnError(LSError err, RString context)
{
	Assert(_entry);
	const char *member = context.GetLength() == 0 ? RString() : context;
	_errorContext = _entry->GetContext(member);
}

void ParamArchive::CancelError()
{
	_errorContext = "";
}

bool ParamArchive::IsSubclass(const RStringB &name)
{
	if (_saving) return true;
	else
	{
		SRef<ClassEntry> cls = _entry->FindEntry(name);
		return cls != NULL;
	}
}

int ParamArchive::GetEntryCount() const
{
	if (_saving)
	{
		Fail("No entry count available when saving");
		return 0;
	}
	return _entry->GetEntryCount();
}

void ParamArchive::OpenSubclass(SRef<ClassEntry> entry, ParamArchive &ar)
{
	ar._version = _version;
	ar._saving = _saving;
	ar._pass = _pass;
	ar._params = _params;

	ar._entry = entry;
}

bool ParamArchive::OpenSubclass(const RStringB &name, ParamArchive &ar, bool guaranteedUnique)
{
	ar._version = _version;
	ar._saving = _saving;
	ar._pass = _pass;
	ar._params = _params;

	if (_saving)
		ar._entry = _entry->AddClass(name,guaranteedUnique);
	else
		ar._entry = _entry->FindEntry(name);
	return ar._entry != NULL;
}

/*!
\patch_internal 1.24 Date 09/26/2001 by Ondra
- Optimized: memory usage during Serialize to ParamArchive.
*/

void ParamArchive::CloseSubclass(ParamArchive &ar)
{
	if (_saving)
	{
		ar._entry->Compact();
		ar._entry = NULL;
	}
}

ClassEntry *ParamArchive::OpenArray(const RStringB &name)
{
	if (_saving)
		return _entry->AddArray(name);
	else
		return _entry->FindEntry(name);
}

void ParamArchive::CloseArray(ClassEntry *arr)
{
	if (_saving)
	{
		arr->Compact();
	}
}

template <class Type>
void LogDifferentValues(const char *name, const Type &val1, const Type &val2)
{
  LogF("%s: 2nd pass value different",name);
}

template <>
void LogDifferentValues(const char *name, const int &val1, const int &val2)
{
  LogF("%s: 2nd pass value different (%d!=%d)",name,val1,val2);
}

template <>
void LogDifferentValues(const char *name, const bool &val1, const bool &val2)
{
  LogDifferentValues(name,int(val1),int(val2));
}

template <>
void LogDifferentValues(const char *name, const RString &val1, const RString &val2)
{
  LogF("%s: 2nd pass value different (%s!=%s)",name,cc_cast(val1),cc_cast(val2));
}

const bool VerifySecondPass = false;
//const bool VerifySecondPass = true;

/**
@param LoadType type used for an intermediate copy while loading
@param Type type of the value loaded
*/
template <class LoadType, class Type>
LSError ParamArchive::LoadOrVerifyEx(const RStringB &name, Type &value)
{
  if (!VerifySecondPass && _pass != PassFirst) return LSError(0);
  SRef<ClassEntry> entry = _entry->FindEntry(name);
  if (!entry) ON_ERROR(ErrorNoEntry(), name);
  if (_pass != PassFirst)
  {
    LoadType test=*entry;
    Type fileValue = safe_cast<Type>(test);
    if (fileValue!=value)
    {
      //LSError err = ErrorStructure();
      //OnError(err, name);
      // we would like to display value as well
      // template may specialize this for various types
      LogDifferentValues(name,fileValue,value);
      //return err;
    }
  }
  else
  {
    LoadType temp = *entry;
    value = safe_cast<Type>(temp);
  }
  return LSError(0);
}

template <class Type>
LSError ParamArchive::SerializeSimple(const RStringB &name, Type &value, int minVersion)
{
	if (_version < minVersion) return LSError(0);
	if (_saving)
	{
		_entry->Add(name, value);
	}
	else
	{
    return LoadOrVerify(name,value);
	}
	return LSError(0);
}

template <class LoadType, class Type>
LSError ParamArchive::SerializeSimpleEx(const RStringB &name, Type &value, int minVersion)
{
	if (_version < minVersion) return LSError(0);
	if (_saving)
	{
		_entry->Add(name, (LoadType)value);
	}
	else
	{
	  // no direct representation for bool - stored as int
    return LoadOrVerifyEx<LoadType>(name,value);
	}
	return LSError(0);
}


LSError ParamArchive::Serialize(const RStringB &name, bool &value, int minVersion)
{
  return SerializeSimpleEx<int>(name,value,minVersion);
}

LSError ParamArchive::Serialize(const RStringB &name, int &value, int minVersion)
{
  return SerializeSimple(name,value,minVersion);
}

LSError ParamArchive::Serialize(const RStringB &name, unsigned int &value, int minVersion)
{
  return SerializeSimpleEx<int>(name,value,minVersion);
}

LSError ParamArchive::Serialize(const RStringB &name, signed char &value, int minVersion)
{
  return SerializeSimpleEx<int>(name,value,minVersion);
}

LSError ParamArchive::Serialize(const RStringB &name, unsigned char &value, int minVersion)
{
  return SerializeSimpleEx<int>(name,value,minVersion);
}

LSError ParamArchive::Serialize(const RStringB &name, float &value, int minVersion)
{
  return SerializeSimple(name,value,minVersion);
}

LSError ParamArchive::Serialize(const RStringB &name, RString &value, int minVersion)
{
  return SerializeSimple(name,value,minVersion);
}

LSError ParamArchive::SerializeDef(const RStringB &name, SerializeClass &value, int minVersion)
{
	if (_version < minVersion) return LSError(0);
	// call Serialize for both passes
	if (IsSaving() && value.IsDefaultValue(*this))
	{
		return LSError(0);
	}
	ParamArchive arSubcls;
	if (!OpenSubclass(name, arSubcls))
	{
		if (IsLoading())
		{
			value.LoadDefaultValues(*this);
			return LSError(0);
		}
		ON_ERROR(ErrorNoEntry(), name);
	}
	arSubcls.SetParams(GetParams());
	LSError err = value.Serialize(arSubcls);
	CloseSubclass(arSubcls);
	return err;
}


LSError ParamArchive::Serialize(const RStringB &name, SerializeClass &value, int minVersion)
{
  return SerializeValue(name,value,minVersion);
}

//#if _HELISIM_LARGE_OBJECTID
LSError ParamArchive::Serialize(const RStringB &name, int64 &value, int minVersion)
{
  return SerializeSimple(name,value,minVersion);
  /*
#ifdef _VBS3_MULTIPLAYER_SERIALIZATION
  return SerializeSimpleEx<int64>(name,value,minVersion);
#else
  if (_version < minVersion) return LSError(0);
  if (_saving)
  {
    _entry->Add(name, (int64)value);
  }
  else
  {
    if (_pass != PassFirst) return LSError(0);
    SRef<ClassEntry> entry = _entry->FindEntry(name);
    if (!entry) ON_ERROR(ErrorNoEntry(), name);
    value = safe_cast<int64>(*entry);
  }
  return LSError(0);*/
//#endif
}
//#endif _HELISIM_LARGE_OBJECTID

LSError ParamArchive::SerializeEnumValue(
	const RStringB &name, int &value, int minVersion, const EnumName *names
)
{
	if (GetArVersion() < minVersion) return LSError(0);
	if (IsSaving())
	{
		bool found = false;
		RString str = "ERROR";
		int i;
		for (i=0; names[i].IsValid(); i++)
		{
			if( names[i].value==value ) {str=names[i].name; found = true; break;}
		}
		if (!found)
		{
			int val = value;
			RptF("Saving undefined enum value %d / %d, context %s", val, i, (const char *)_entry->GetContext(name));
		}
		CHECK_INTERNAL(Serialize(name, str, minVersion))
		return LSError(0);
	}
	else
	{
		if (GetPass() != ParamArchive::PassFirst) return LSError(0);
		RString str;
		CHECK_INTERNAL(Serialize(name, str, minVersion))

		for( int i=0; names[i].IsValid(); i++ )
		{
			if( stricmp(names[i].name,str)==0 ) {value=names[i].value;return LSError(0);}
		}
    OnError(LSStructure, name);
    RptF(
      "%s: Unknown enum value %s",
      cc_cast(GetErrorContext()),
      cc_cast(str)
      );
    // recover from the internal error, message in the report is enough
    value = INT_MIN;
    return LSOK;
	}
}

