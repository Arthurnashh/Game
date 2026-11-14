#ifdef _MSC_VER
#pragma once
#endif

#ifndef _STRINGTABLE_HPP
#define _STRINGTABLE_HPP

#include <Es/Strings/rString.hpp>

#include <El/Interfaces/iPreproc.hpp>
class QFBank;

extern RString GLanguage;
extern PreprocessorFunctions *GStringtablePreprocFunctions;

/// Load stringtable, return true if something was loaded
bool LoadStringtable(RString type, RString filename, float priority = 0, bool init = true, const PreprocessorFunctions::Define *defs=NULL);
/// Load stringtable, return true if something was loaded
bool LoadStringtable(RString type, QFBank &bank, RString filename, float priority = 0, bool init = true, const PreprocessorFunctions::Define *defs=NULL);

void UnloadStringtable(RString type);

int RegisterString(RString name);

RString LocalizeString(int ids);
RString LocalizeString(const char *str);
bool IsLocalized(const char *str);
RString TryLocalizeString(const char *str);
RString TryLocalizeString(const char *str, RString &tableName);

RString Localize(RString str);
/// try localization, string is localized only when starting with @
RString TryLocalize(RString str, RString &tableName);

void ClearStringtable();

#define DEFINE_STRING(name)			extern int IDS_##name;
#define DECLARE_STRING(name)		int IDS_##name;
#define REGISTER_STRING(name) \
	int RegisterString(RString name); \
	IDS_##name = RegisterString("STR_"#name);


/// simple string localized runtime
struct LocalizedString
{
  /// type of the source
  enum Type
  {
    /// text cannot be localized
    PlainText,
    /// use stringtable - global function Localize
    Stringtable,
  };
  RString _id;
  Type _type;
  
  /// by default create empty non-localized string
  LocalizedString():_id(),_type(PlainText){}
  LocalizedString(Type type,RString id)
  :_type(type),_id(id)
  {
  }
  /// get value suitable for the user
  RString GetLocalizedValue() const;
};

TypeIsMovableZeroed(LocalizedString)

/// compound (formatted) localized string message - temporary reference
struct LocalizedFormatedStringTemp
{
  const AutoArray<LocalizedString> &_args;

  LocalizedFormatedStringTemp(const AutoArray<LocalizedString> &args)
  :_args(args)
  {}
  /// get formatted value - suitable for the user
  RString GetLocalizedValue() const;
};

/// compound (formatted) localized run-time
struct LocalizedFormatedString
{
  AutoArray<LocalizedString> _args;

  LocalizedFormatedString()
  {}
  void SetFormat(LocalizedString::Type type, RString id);
  void AddArg(LocalizedString::Type type, RString id);
  
  /// get formatted value - suitable for the user
  RString GetLocalizedValue() const;
};

#endif
