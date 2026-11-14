#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PREPROC_C_HPP
#define _PREPROC_C_HPP

#include <El/Interfaces/iPreproc.hpp>
#include <El/QStream/qbStream.hpp>
#include "preproc.h"

struct IncludeItem
{
  RString filename;
  int line;
};
TypeIsMovableZeroed(IncludeItem)

class IncludeStack : public AutoArray<IncludeItem>
{
  typedef AutoArray<IncludeItem> base;

public:
  int Add(RString filename)
  {
    int index = base::Add();
    Set(index).filename = filename;
    Set(index).line = 0;
    return index;
  }
};

class StringPreprocessor : public Preproc
{
protected:
  SRef<QIStream> _stream;

  QIStream *OnEnterInclude(const char *str);
  void OnExitInclude(QIStream *stream);
  void AtBeginLine(QOStream &out);
};

class FilePreprocessor : public Preproc
{
protected:
  bool _lineNumbers;
	IncludeStack _stack;

private:
	QIStream *OnEnterInclude(const char *filename);
	void OnExitInclude(QIStream *stream);
  void AtBeginLine(QOStream &out);

public:
  FilePreprocessor(const PreprocessorFunctions::Params &params=PreprocessorFunctions::Params())
  : _lineNumbers(params._lineNumbers)
  {
    if (params._defines)
    {
      for (const PreprocessorFunctions::Define *def=params._defines; def->name; def++ )
      {
        DefineDefine(def->name,def->value ? def->value : "");
      }
    
    }
  
  }

  RString GetFilename() const;
  int GetLine() const;
};

class BankPreprocessor : public FilePreprocessor
{
private:
  QFBank &_bank;

  QIStream *OnEnterInclude(const char *filename);
  void OnExitInclude(QIStream *stream);
  void AtBeginLine(QOStream &out);

public:
  BankPreprocessor(QFBank &bank, const PreprocessorFunctions::Params &params) : FilePreprocessor(params),_bank(bank) {}

  RString GetFilename() const;
  int GetLine() const;
};

//! class of callback functions
class CPreprocessorFunctions : public PreprocessorFunctions
{
public:
	//! callback function to preprocess of stream content
	virtual bool Preprocess(QOStream &out, const char *name, const Params &params=Params());
  //! callback function to preprocess of stream content in bank
  virtual bool Preprocess(QOStream &out, QFBank &bank, const char *name, const Params &params=Params());
};

#endif

