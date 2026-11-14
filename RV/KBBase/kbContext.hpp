#ifdef _MSC_VER
#pragma once
#endif

#ifndef _KB_CONTEXT_HPP
#define _KB_CONTEXT_HPP

#include <Es/Strings/rString.hpp>
#include <El/Evaluator/expressImpl.hpp>

class KBContext : public RefCount
{
public:
  // check if condition is satisfy in conversation context
  virtual bool CheckCondition(RString condition) const = NULL;

  // evaluate fuzzy condition in conversation context
  virtual float CheckFuzzyCondition(RString condition) const = NULL;

  // evaluate text in conversation context
  virtual RString GetText(RString expression) const = NULL;

  // evaluate speech in conversation context
  virtual void GetSpeech(AutoArray<RString> &speech, RString expression) const = NULL;

  // initialize context by assigning value to named slot
  virtual void InitContext(RString name, GameValuePar value) = NULL;

  // extract value from named slot
  virtual void GetContext(RString name, GameValue &result) = NULL;

  // update conversation context
  virtual void UpdateContext(RString statement) = NULL;
};

#endif

