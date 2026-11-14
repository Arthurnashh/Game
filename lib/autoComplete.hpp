
#ifdef _MSC_VER
#pragma once
#endif

#ifndef _AUTO_COMPLETE_HPP
#define _AUTO_COMPLETE_HPP

#include <Es/Strings/rString.hpp>
#include <El/HierWalker/hierWalker.hpp>
/*
\file
Autocomplete interface
*/

//! autocomplete interface class
class IAutoComplete: public RefCount
{
	public:
	//! guess how given text can continue
	/*!
	\param text source text
	\param caret current position where user is typing
	\param certain pointer to boolean returning if guess is certain or not
	*/
	virtual RString Guess
	(
		RString text, int caret, bool &certain, RString &beg
	) = 0;
	//! provide text that should be inserted to complete to Guess
	//! user typed some word - add it to temporary dictionary
	virtual void WordDone(RString word) = 0;
	//! user typed character - decide what to do now
	virtual void AfterChar(RString text, int caret) = 0;
  /// format the guess as a structured text
  virtual Ref<INode> ShowGuess(RString text) = 0;
  /// format the help for given text
  virtual RString GetHelp(RString text) = 0;
};

//! function to create autocomplete
IAutoComplete *CreateAutoComplete(const char *type);

#endif
