// Implementation of control hierarchy

#include "wpch.hpp"
#include "autoComplete.hpp"
#include <Es/Containers/bankArray.hpp>
#include <ctype.h>
#include <El/Evaluator/expressImpl.hpp>
#include "drawText.hpp"

/// Type of callback function formatting the hint as a structured text
typedef Ref<INode> FormatHintFuncion(RString name);

/// Type of callback function formatting the HTML help for given text
typedef RString FormatHelpFuncion(RString name);

class AutoComplete: public IAutoComplete
{
	//! instance name (needed for BankArray)
	RString _name;
	//! list of known words (fixed)
	FindArray<RStringS> _fixedWords;
	//! list of words used by user
	FindArray<RStringS> _userWords;

  /// callback function formatting the hint as a structured text
  FormatHintFuncion *_formatHint;
  /// callback function formatting the HTML help for given text
  FormatHelpFuncion *_formatHelp;

	public:	
	AutoComplete(const RString &name);
	//! instance name (needed for BankArray)
	const RString &GetName() const {return _name;}

	// public interface
	RString Guess
	(
		RString text, int caret, bool &certain, RString &beg
	);
	void WordDone(RString word);
	void AfterChar(RString text, int caret);
  
  Ref<INode> ShowGuess(RString text);
  RString GetHelp(RString text);

	private:
	void AddWord(const char *word);
	RString ToComplete
	(
		RString text, int caret, bool &certain, RString *beg
	);
	void AddDictionary(GameState *state);
};

static BankArray<AutoComplete> AutoCompleteBank;

IAutoComplete *CreateAutoComplete(const char *type)
{
	// empty name means no autocomplete
	if (!*type) return NULL;
	return AutoCompleteBank.New(type);
}

static Ref<INode> FormatScriptingHint(RString name)
{
#if _ENABLE_COMREF
  RString lwName = name;
  lwName.Lower();
  const GameNular *nular = GGameState.FindNular(lwName);
  const GameFunctions *functions = GGameState.FindFunctions(lwName);
  const GameOperators *operators = GGameState.FindOperators(lwName);
  if (nular || functions || operators)
  {
    Ref<INode> root = CreateTextStructured(NULL);
    bool empty = true;
    if (nular)
    {
      // if (!empty) CreateTextBreak(root);
      CreateTextASCII(root, name);
      empty = false;
    }
    if (functions)
    {
      for (int i=0; i<functions->Size(); i++)
      {
        const GameFunction &function = (*functions)[i];
        if (!empty) CreateTextBreak(root);
        CreateTextStructured(root, Format("%s <t color=\"#ffff00\">%s</t>", cc_cast(name), cc_cast(function.GetOperName())));
        empty = false;
      }
    }
    if (operators)
    {
      for (int i=0; i<operators->Size(); i++)
      {
        const GameOperator &oper = (*operators)[i];
        if (!empty) CreateTextBreak(root);
        CreateTextStructured(root, Format("<t color=\"#ffff00\">%s</t> %s <t color=\"#ffff00\">%s</t>", cc_cast(oper.GetLOperName()), cc_cast(name), cc_cast(oper.GetROperName())));
        empty = false;
      }
    }
    return root;
  }
#endif
  return CreateTextASCII(NULL, name);
}

static RString FormatScriptingHelp(RString name)
{
#if _ENABLE_COMREF
  RString lwName = name;
  lwName.Lower();
  const GameNular *nular = GGameState.FindNular(lwName);
  if (nular) return Format("nular:%s", cc_cast(name));
  const GameFunctions *functions = GGameState.FindFunctions(lwName);
  if (functions && functions->Size() > 0) return Format("function:%s:0", cc_cast(name));
  const GameOperators *operators = GGameState.FindOperators(lwName);
  if (operators && operators->Size() > 0) return Format("operator:%s:0", cc_cast(name));
#endif
  return RString();
}

AutoComplete::AutoComplete(const RString &name)
{
	_name = name;
	if (!strcmpi(name,"scripting"))
	{
		AddDictionary(&GGameState);
    _formatHint = &FormatScriptingHint;
    _formatHelp = &FormatScriptingHelp;
	}
  else
  {
    _formatHint = NULL;
    _formatHelp = NULL;
  }
}

Ref<INode> AutoComplete::ShowGuess(RString text)
{
  if (_formatHint) return (*_formatHint)(text);
  // by default return the text itself
  return CreateTextASCII(NULL, text);
}

RString AutoComplete::GetHelp(RString text)
{
  if (_formatHelp) return (*_formatHelp)(text);
  // by default no help is available
  return RString();
}

void AutoComplete::AddWord(const char *word)
{
	_fixedWords.Add(word);
}

inline bool IsPartOfWord(char c)
{
	// quote marks supposed to be a part of word
	// this makes autocomplete more context sensitive, as it can distinguish
	// between language keywords and often used user strings
	return isalnum(c) || c=='_' || c=='"';
}
static bool WordFilter(const char *word)
{
	char c = *word;
	if (!isalnum(c) && c!='_') return false;
#if _VBS2 //ignore AAR commands
  if (!strnicmp(word, "AAR", 3)) return false;
#endif
	return true;
}

void AutoComplete::AddDictionary(GameState *state)
{
	state->AppendFunctionList(_fixedWords,WordFilter);
	state->AppendNularOpList(_fixedWords,WordFilter);
	state->AppendOperatorList(_fixedWords,WordFilter);
}


static RString GetTypedWord(RString text, int caret)
{
	// typed word ends at caret position
	const char *beg = text;
	const char *endWord = beg+caret;
	const char *begWord = endWord;
	while (begWord>beg)
	{
		char c = begWord[-1];
		if (!IsPartOfWord(c)) break;
		begWord--;
	}
	// begWord to beg is word part
	return RString(begWord,endWord-begWord);
}

static int CountMatch(const char *s1, const char *s2)
{
	int cnt = 0;
	for(;;)
	{
		char c1 = s1[cnt];
		char c2 = s2[cnt];
		if (c1==c2 || tolower(c1)==tolower(c2)) cnt++;
		else break;
	}
	if (s2[cnt]!=0) return 0;
	return cnt;
}

static RString CommonStart(const char *s1, const char *s2)
{
	int cnt = 0;
	for(;;)
	{
		char c1 = s1[cnt];
		char c2 = s2[cnt];
		if (c1==c2 || tolower(c1)==tolower(c2)) cnt++;
		else break;
	}
	return RString(s1,cnt);
}

/*!
\patch 1.53 Date 4/27/2002 by Ondra
- New: Autocomplete for mission editor fields and chat (use Tab key to complete).
*/

RString AutoComplete::Guess
(
	RString text, int caret, bool &certain, RString &beg
)
{
	certain = false;
	RString begWord = GetTypedWord(text,caret);
	beg = begWord;
	// search all dictionaries for any match
	// search user dictionary - use last match
	int bestLen = 0;
	RString bestWord;
	int matches = 0;
	RString longestCommon;
	for (int i=0; i<_userWords.Size(); i++)
	{
		int m = CountMatch(_userWords[i],begWord);
		if (m>=bestLen)
		{
			RString oldBestWord = bestWord;
			bestWord = _userWords[i];
			if (m>bestLen)
			{
				bestLen = m;
				matches = 0;
				longestCommon = bestWord;
			}
			else
			{
				longestCommon = CommonStart(longestCommon,_userWords[i]);
			}
			if (strcmpi(oldBestWord,_userWords[i])) matches++;
		}
	}
	// check if we could find better match in predefined directory
	for (int i=0; i<_fixedWords.Size(); i++)
	{
		int m = CountMatch(_fixedWords[i],begWord);
		if (m>=bestLen)
		{
			RString oldBestWord = bestWord;
			if (m>bestLen)
			{
				bestLen = m,bestWord = _fixedWords[i];
				matches = 0;
				longestCommon = bestWord;
			}
			else
			{
				longestCommon = CommonStart(longestCommon,_fixedWords[i]);
			}
			if (strcmpi(oldBestWord,_fixedWords[i])) matches++;
		}
	}

	if (bestLen<1) return RString();
	if (longestCommon.GetLength()>begWord.GetLength()+2)
	{
		certain = true;
		return longestCommon;
	}
	if (bestWord.GetLength()<begWord.GetLength()+2) return RString();
	
	// search this word in fixed dictionary
	bool found = false;
	for (int i=0; i<_fixedWords.Size(); i++)
	{
		if (!strcmpi(bestWord,_fixedWords[i]))
		{
			bestWord = _fixedWords[i];
			found = true;
			break;
		}
	}
	certain = found && matches<2;
	return bestWord;
}

void AutoComplete::WordDone(RString word)
{
	if (word.GetLength()<=0) return;
	// always add this word to the end of the list - to mark it as most recent
	int index = _userWords.Find(word);
	if (index>=0)
	{
		_userWords.DeleteAt(index);
	}
	_userWords.Add(word);
}

void AutoComplete::AfterChar(RString text, int caret)
{
	if (caret<0 || caret>text.GetLength()) return;
	// check what user typed
	// if we think he terminated word we can add it to temporary list
	if (caret<=0) return;
	char c = text[caret-1];
	if (IsPartOfWord(c)) return; // user typed normal char - continue word
	RString word = GetTypedWord(text,caret-1);
	if (word.GetLength()<=0) return;
	if (isdigit(word[0])) return;
	WordDone(word);
}

