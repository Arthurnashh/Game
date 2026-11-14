#ifdef _MSC_VER
#pragma once
#endif

#ifndef _STRINGTABLE_IMPL_HPP
#define _STRINGTABLE_IMPL_HPP

// Basic internal structures for stringtable implementation

#include <Es/Containers/hashMap.hpp>
#include <El/QStream/serializeBin.hpp>

/// one item in the stringtable - id + value
struct StringTableItem
{
  RString name;
  RString value;

  StringTableItem() {}
  StringTableItem(RString n, RString v) {name = n; value = v;}
  const char *GetKey() const {return name;}
};
TypeIsMovableZeroed(StringTableItem)

/// type used for the hashtable storage
typedef MapStringToClass<
StringTableItem, AutoArray<StringTableItem>, MapClassTraitsNoCase<StringTableItem>
> StringTableMap;

/// dynamically loaded localization

class StringTableDynamic : public StringTableMap
{
  typedef StringTableMap base;
protected:
  RString _name;
  float _priority;

  void ParseCSV(QIStream &f);
  void LoadBin(QIStream &f);

public:
  enum ParserType
  {
    PTCSV, ///< CSV parser
    PTXML, ///< XML parser
    PTBIN, ///< BIN loader
  };

  void Create(RString name, float priority);
  void Init();
  void Add(RString name, RString value);
  void Load(QIStream &f, ParserType parserType);
  RString GetName() const {return _name;}
  float GetPriority() const {return _priority;}

  /// function transfers (saves or loads) version to stream (used for stringtable binarization)
  static bool TransferBinVersion(SerializeBinStream& f);
};
TypeIsMovableZeroed(StringTableDynamic)

#endif
