#ifdef _MSC_VER
#pragma once
#endif

#ifndef _I_EXT_PARSER_HPP
#define _I_EXT_PARSER_HPP

#include <Es/Strings/rString.hpp>

class StringTableDynamic;

class QIStream;
class QIFStream;
class QIFStreamB;

class QFBank;

/// Class of callback functions
class StringtableExtParserFunctions
{
public:
  virtual ~StringtableExtParserFunctions() {}

  /// Open the stream for the extended parser if possible
  virtual bool Open(const char *filename, QIFStreamB &in) {return false;}
  /// Open the stream for the extended parser if possible
  virtual bool Open(QFBank &bank, const char *filename, QIFStreamB &in) {return false;}

  /// Parse the stream, store results to stringtable
  virtual bool Parse(QIStream &in, StringTableDynamic &stringtable, RString language) {return false;}
};

extern StringtableExtParserFunctions *GStringtableExtParserFunctions;

#endif
