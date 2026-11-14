#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MBCS_HPP
#define _MBCS_HPP

#include "languages.hpp"
#include <Es/Strings/rString.hpp>

int FindLangID(const char *language);
const char *GetLangName(int language);
int GetLangID();
void SetLangID(const char *language);

void MultiByteStringToPseudoCodeString(const char *in, char *out);
bool PseudoCodeToPictureCode(const char *in, short *out);

struct FontID
{
	RString name;
	int langID;

	FontID(RString n, int lID) {name = n; langID = lID;}
};

#endif
