#include <El/elementpch.hpp>
#include <El/QStream/qbStream.hpp>
#include <ctype.h>
#include "stringtable.hpp"
#include "stringtableImpl.hpp"

#include "iExtParser.hpp"
#include <El/Interfaces/iPreproc.hpp>
#include <El/FileServer/fileServerMT.hpp>
#include <El/QStream/serializeBin.hpp>

void StringTableDynamic::Create(RString name, float priority)
{
  _priority = priority;
  _name = name;
}

void StringTableDynamic::Init()
{
  base::Clear();
}

void StringTableDynamic::Add(RString name, RString value)
{
  // avoid duplicity in names
  const StringTableItem &check = Get(name);
  if (!IsNull(check))
  {
#if _VBS2
    // allow addons to overwrite registered strings
    StringTableItem &item = Set(name);
    item.value = value;
    return;
#else
    RptF("Item %s listed twice", (const char *)name);
    return;
#endif
  }

  base::Add(StringTableItem(name, value));
}


/* replaced by   QIStream::nextLine()
8/27/2002 Pepca

void NextLine(QIStream &f)
{
while (!f.eof() && f.get() != 0x0D);
if (!f.eof() && f.get() != 0x0A) f.unget();
}
*/

RString ReadColumn(QIStream &f)
{
  int c = f.get();
  if (c < 0) return "";
  // skip leading spaces:
  while (c != 0x0D && c != 0x0A && isspace(c))
  {
    c = f.get();
    if (c < 0) return "";
  }
  char buffer[4096];
  int i = 0;
  if (c == '\"')
  {
    c = f.get();
    if (c < 0) return "";
    while (true)
    {
      if (c == '\"')
      {
        c = f.get();
        if (c < 0)
        {
          buffer[i] = 0;
          return buffer;
        }
        if (c != '"')
        {
          while (c != 0x0D && c != 0x0A && c != ',')		
          {
            c = f.get();
            if (c < 0)
            {
              buffer[i] = 0;
              return buffer;
            }
          }
          if (c == 0x0D || c == 0x0A) f.unget();
          if (i == 0) return "";
          buffer[i] = 0;
          return buffer;
        }
      }
      if (i < sizeof(buffer) - 1) buffer[i++] = c;
      c = f.get();
      if (c < 0)
      {
        Assert(i > 0);
        buffer[i] = 0;
        return buffer;
      }
    }
    // Unaccessible
  }
  else
  {
    while (c != 0x0D && c != 0x0A && c != ',')		
    {
      if (i < sizeof(buffer) - 1) buffer[i++] = c;
      c = f.get();
      if (c < 0)
      {
        buffer[i] = 0;
        return buffer;
      }
    }
    if (c == 0x0D || c == 0x0A) f.unget();
    if (i == 0) return "";
    buffer[i] = 0;
    return buffer;
  }
}

void StringTableDynamic::ParseCSV(QIStream &f)
{
  int column = -1;
  while (true)
  {
    if (f.eof()) return;
    RString name = ReadColumn(f);
    if (name && stricmp(name, "LANGUAGE") == 0)
    {
      RString value = ReadColumn(f);
      int c = 0;
      while (value.GetLength() > 0)
      {
        if (stricmp(value, GLanguage) == 0)
        {
          column = c;
          break;
        }
        value = ReadColumn(f);
        c++;
      }
      f.nextLine();
      break;
    }
    else
      f.nextLine();
  }
  if (column < 0)
  {
    RptF("Unsupported language %s in stringtable",(const char *)GLanguage);
    column = 0;
  }

  while (!f.eof())
  {
    RString name = ReadColumn(f);
    if (name.GetLength()<=0 || stricmp(name, "COMMENT") == 0)
    {
      f.nextLine();
      continue;
    }
    for (int c=0; c<column; c++) ReadColumn(f);
    RString value = ReadColumn(f);
    if (value) Add(name, value);
    f.nextLine();
  }
}

void StringTableDynamic::LoadBin(QIStream &f)
{
  // note: binarization of stringtable.XML is done in XBOXPBOPacker utility (using StringtableXMLToBINParser class)

  SerializeBinStream bs(&f);

  // check version
  if (!TransferBinVersion(bs))
  {
    RptF("Invalid file type of binarized XML.");
    bs.SetError(bs.EBadFileType);
    return;
  }

  
  // languages
  AutoArray<RString> languages;
  bs.TransferBasicArray(languages);

  // offsets
  AutoArray<int> offsets;
  bs.TransferBasicArray(offsets);

  DoAssert(languages.Size() == offsets.Size());
  if (languages.Size() != offsets.Size())
  {
    RptF("Error in binarized stringtable: mismatch of number of languages and offsets.");
    return;
  }

  // find index of our language
  int languageIndex = -1;
  for (int i=0; i<languages.Size(); ++i)
  {
    if (stricmp(GLanguage, languages[i]) == 0)
    {
      languageIndex = i;
      break;
    }
  }

  DoAssert(languageIndex != -1);
  if (languageIndex == -1)
  {
    RptF("Cannot find requested language \"%s\" in binarized stringtable.", cc_cast(GLanguage));
    return;
  }

  // keys
  AutoArray<RString> keys;
  bs.TransferBasicArray(keys);

  // offset of translations for our language
  int offset = offsets[languageIndex];
  f.seekg(offset, QIOS::beg);

  // translations
  AutoArray<RString> translations;
  bs.TransferBasicArray(translations);

  DoAssert(translations.Size() == keys.Size());
  if (translations.Size() != keys.Size())
  {
    RptF("Error in binarized stringtable: mismatch of number of keys and translations.");
    return;
  }

  // add values to table
  for (int i=0; i<keys.Size(); ++i)
  {
    Add(keys[i], translations[i]);
  }
}

void StringTableDynamic::Load(QIStream &f, ParserType parserType)
{
  switch (parserType)
  {
  case PTCSV:
    f.PreReadSequential();
    ParseCSV(f);
    break;
  case PTXML:
    f.PreReadSequential();
    if (!GStringtableExtParserFunctions->Parse(f, *this, GLanguage))
    {
      RptF("Unsupported language %s in stringtable", cc_cast(GLanguage));
    }
    break;
  case PTBIN:
    LoadBin(f);
    break;
  }
}

bool StringTableDynamic::TransferBinVersion(SerializeBinStream& f)
{
#ifdef _MSC_VER
  return f.Version('XMLB');
#else
  return f.Version(StrToInt("BLMX"));
#endif
}

/// localization support
class StringTable
{
protected:
  AutoArray<StringTableDynamic> _tables;
  AutoArray<StringTableItem> _registered;

public:

  StringTable() {}
  void Clear();

  void Load(RString type, QIStream &f, float priority, bool init, StringTableDynamic::ParserType parserType);
  void Unload(RString type);

  int Register(RString name);

  /// localize based on numerical ID
  RString Localize(int ids);

  /// localize based on string ID (starting with STR_)
  RString Localize(const char *str);

  /// true when localize based on string ID is available
  bool IsLocalized(const char *str);

  /// check if localization is possible, if not, return empty string
  RString TryLocalize(const char *str);
  /// check if localization is possible, and by what stringtable
  RString TryLocalize(const char *str, RString &tableName);

protected:
  StringTableDynamic *FindStringtable(RString type);
};

void StringTable::Clear()
{
  _tables.Clear();
  _registered.Clear();
}

StringTableDynamic *StringTable::FindStringtable(RString type)
{
  for (int i=0; i<_tables.Size(); i++)
  {
    StringTableDynamic &table = _tables[i];
    if (stricmp(table.GetName(), type) == 0) return &table;
  }
  return NULL;
}

void StringTable::Load(RString type, QIStream &f, float priority, bool init, StringTableDynamic::ParserType parserType)
{
  for (int i=0; i<_tables.Size(); i++)
  {
    StringTableDynamic &table = _tables[i];
    if (stricmp(table.GetName(), type) == 0)
    {
      DoAssert(table.GetPriority() == priority);
      if (init) table.Init();
      table.Load(f, parserType);
      return;
    }
  }

  // Create new stringtable
  int index = -1;
  for (int i=0; i<_tables.Size(); i++)
  {
    if (priority > _tables[i].GetPriority())
    {
      _tables.Insert(i);
      index = i;
      break;
    }
  }

  if (index < 0) index = _tables.Add();

  StringTableDynamic &table = _tables[index];
  table.Create(type, priority);
  table.Load(f, parserType);
}

void StringTable::Unload(RString type)
{
  for (int i=0; i<_tables.Size(); i++)
  {
    if (stricmp(_tables[i].GetName(), type) == 0)
    {
      _tables.Delete(i);
      return;
    }
  }
}

int StringTable::Register(RString name)
{
  // only strings from global stringtable can be registered
  StringTableDynamic *table = FindStringtable("Global");
  if (!table)
  {
    RptF("Cannot register string %s - global stringtable not found", (const char *)name);
    return -1;
  }

  const StringTableItem &item = (*table)[name];
  if (table->IsNull(item))
  {
    RptF("Cannot register unknown string %s", (const char *)name);
    StringTableItem noValue(name, Format("Missing string %s", cc_cast(name)));
    return _registered.Add(noValue);
  }
  return _registered.Add(item);
}

RString StringTable::Localize(int ids)
{
  if (ids < 0 || ids >= _registered.Size())
  {
    RptF("String id %d is not registered", ids);
#if _ENABLE_REPORT
    return "!!! UNREGISTERED STRING";
#else
    return "";
#endif
  }
#if _VBS2
  // load string from hashmap, as it may have been overwritten by an addon
  return Localize(_registered[ids].name);
#else
  return _registered[ids].value;
#endif
}

RString StringTable::Localize(const char *str)
{
  for (int i=0; i<_tables.Size(); i++)
  {
    const StringTableDynamic &table = _tables[i];
    const StringTableItem &item1 = table[str];
    if (!table.IsNull(item1))
		return item1.value;
  }

  RptF("String %s not found", (const char *)str);
#if _ENABLE_REPORT
  return str;
#else
  return "";
#endif
}

RString StringTable::TryLocalize(const char *str)
{
  for (int i=0; i<_tables.Size(); i++)
  {
    const StringTableDynamic &table = _tables[i];
    const StringTableItem &item1 = table[str];
    if (!table.IsNull(item1)) return item1.value;
  }
  return RString();
}

RString StringTable::TryLocalize(const char *str, RString &tableName)
{
  tableName = RString();
  for (int i=0; i<_tables.Size(); i++)
  {
    const StringTableDynamic &table = _tables[i];
    const StringTableItem &item1 = table[str];
    if (!table.IsNull(item1))
    {
      tableName = table.GetName();
      return item1.value;
    }
  }
  return RString();
}

bool StringTable::IsLocalized(const char *str)
{
  for (int i=0; i<_tables.Size(); i++)
  {
    const StringTableDynamic &table = _tables[i];
    const StringTableItem &item1 = table[str];
    if (!table.IsNull(item1)) return true;
  }
  return false;
}

RString GLanguage;
StringTable GStringTable;

bool LoadStringtable(RString type, RString filename, float priority, bool init, const PreprocessorFunctions::Define *defs)
{
  // check for binarized table first
  const char *ext = strrchr(filename, '.');
  if (ext)
  {
    RString binName = RString(filename, ext - filename) + RString(".bin");
    if (!binName.IsEmpty() && QFBankQueryFunctions::FileExists(binName))
    {
      QIFStreamB in;
      in.AutoOpen(binName);
      if (!in.fail())
      {
        GStringTable.Load(type, in, priority, init, StringTableDynamic::PTBIN);
      }
      return true;
    }
  }

  // check if the extended parser can handle it first
  {
    QIFStreamB in;
    if (GStringtableExtParserFunctions->Open(filename, in))
    {
      GStringTable.Load(type, in, priority, init, StringTableDynamic::PTXML);
      return true;
    }
  }

  // process csv file now
  if (!QFBankQueryFunctions::FileExists(filename))
	  return false;

  QOStrStream out;
  if (!GStringtablePreprocFunctions->Preprocess(out, filename,PreprocessorFunctions::Params(false,defs)))
  {
    ErrF("Stringtable %s: preprocessor error", (const char *)filename);
    return false;
  }
  QIStrStream in;
  in.init(out.str(), out.pcount());
  GStringTable.Load(type, in, priority, init, StringTableDynamic::PTCSV);
  return true;
}

bool LoadStringtable(RString type, QFBank &bank, RString filename, float priority, bool init, const PreprocessorFunctions::Define *defs)
{
  // check for binarized table first
  const char *ext = strrchr(filename, '.');
  if (ext)
  {
    RString binName = RString(filename, ext - filename) + RString(".bin");
    if (!binName.IsEmpty() && bank.FileExists(binName))
    {
      QIFStreamB in;
      in.open(bank, binName);
      if (!in.fail())
      {
        GStringTable.Load(type, in, priority, init, StringTableDynamic::PTBIN);
      }
      return true;
    }
  }

  // check if the extended parser can handle it first
  {
    QIFStreamB in;
    if (GStringtableExtParserFunctions->Open(bank, filename, in))
    {
      GStringTable.Load(type, in, priority, init, StringTableDynamic::PTXML);
      return true;
    }
  }

  // process csv file now
  if (!bank.FileExists(filename)) return false;

  QOStrStream out;
  if (!GStringtablePreprocFunctions->Preprocess(out, bank, filename, PreprocessorFunctions::Params(false,defs)))
  {
    ErrF("Stringtable %s: preprocessor error", (const char *)filename);
    return false;
  }
  QIStrStream in;
  in.init(out.str(), out.pcount());
  GStringTable.Load(type, in, priority, init, StringTableDynamic::PTCSV);
  return true;
}

void UnloadStringtable(RString type)
{
  GStringTable.Unload(type);
}

int RegisterString(RString name)
{
  return GStringTable.Register(name);
}

RString LocalizeString(int ids)
{
  return GStringTable.Localize(ids);
}

RString LocalizeString(const char *str)
{
  return GStringTable.Localize(str);
}

bool IsLocalized(const char *str)
{
  return GStringTable.IsLocalized(str);
}

RString TryLocalizeString(const char *str)
{
  return GStringTable.TryLocalize(str);
}

RString TryLocalizeString(const char *str, RString &tableName)
{
  return GStringTable.TryLocalize(str,tableName);
}

RString Localize(RString str)
{
  if (str[0] == '@') return LocalizeString((const char *)str + 1);
  return str;
}

RString TryLocalize(RString str, RString &tableName)
{
  if (str[0] == '@')
  {
    return TryLocalizeString((const char *)str + 1,tableName);
  }
  else
  {
    tableName = RString();
    return str;
  }
}


RString LocalizedString::GetLocalizedValue() const
{
  switch (_type)
  {
  case Stringtable:
    return LocalizeString(_id);
  default: // case PlainText
    return _id;
  }
}

typedef AutoArray<RString, MemAllocLocal<RString,32> > ReplaceStringArgs;

/// functor will replace %s by the strings given in the input array
class ReplaceStringFunc
{
protected:
  int _index;
  ReplaceStringArgs &_args;

public:
  ReplaceStringFunc(ReplaceStringArgs &args) : _args(args) {_index = 0;}

  RString operator ()(const char *&ptr)
  {
    // only %s are recognized
    if (*ptr != 's') return RString();
    ptr++;
    if (_index >= _args.Size()) return RString();
    return _args[_index++];
  }
};

RString LocalizedFormatedStringTemp::GetLocalizedValue() const
{
  // first argument is format
  if (_args.Size()<=0)
  {
    Fail("No localization format string");
    return RString();
  }
  RString format = _args[0].GetLocalizedValue();
  // localize arguments
  ReplaceStringArgs args;
  args.Resize(_args.Size()-1);
  for (int i=0; i<args.Size(); i++)
  {
    const LocalizedString &arg = _args[i+1];
    args[i] = arg.GetLocalizedValue();
  }
  
  ReplaceStringFunc func(args);
  return format.ParseFormat(func);
}

/**
SetFormat can be used only once
*/

void LocalizedFormatedString::SetFormat(LocalizedString::Type type, RString id)
{
  Assert(_args.Size()==0);
  _args.Access(0);
  _args[0] = LocalizedString(type,id);
}
/**
SetFormat must be used before AddArg
*/
void LocalizedFormatedString::AddArg(LocalizedString::Type type, RString id)
{
  Assert(_args.Size()>0);
  _args.Add(LocalizedString(type,id));
}

RString LocalizedFormatedString::GetLocalizedValue() const
{
  return LocalizedFormatedStringTemp(_args).GetLocalizedValue();
}


void ClearStringtable()
{
  GStringTable.Clear();
}
