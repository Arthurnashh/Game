#ifdef _MSC_VER
#pragma once
#endif

#ifndef _I_PREPROC_HPP
#define _I_PREPROC_HPP

#include <El/QStream/qbStream.hpp>

//! class of callback functions
class PreprocessorFunctions
{
public:
  struct Define
  {
    const char *name;
    const char *value;
  };
  struct Params
  {
    // NULL terminated list
    const Define *_defines;
    
    bool _lineNumbers;
    
    Params(bool lineNumbers=false, const Define *defines=NULL)
    {
      _lineNumbers = lineNumbers;
      _defines = defines;
    }
    
    //static Params _default;
  };

	PreprocessorFunctions() {}
	virtual ~PreprocessorFunctions() {}

	//! callback function to preprocess of stream content
	virtual bool Preprocess(QOStream &out, const char *name, const Params &params=Params())
	{
		if (!QFBankQueryFunctions::FileExists(name))
			return false;
		
		QIFStreamB in;
		in.AutoOpen(name);
    in.copy(out);
		return true;
	}
  //! callback function to preprocess of stream content in bank
  virtual bool Preprocess(QOStream &out, QFBank &bank, const char *name, const Params &params=Params())
  {
    if (!bank.FileExists(name))
      return false;

    QIFStreamB in;
    in.open(bank, name);
    in.copy(out);
    return true;
  }
};

#endif
