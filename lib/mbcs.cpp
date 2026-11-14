#include "wpch.hpp"
#include "mbcs.hpp"
#include <Es/Common/win.h>

static int LangID = English;

struct LangInfo
{
  const char *_name;
  int _id;

  LangInfo(const char *name, int id) : _name(name), _id(id) {}
};

static LangInfo LangTable[] =
{
  LangInfo("Chinese", Chinese),
  LangInfo("Czech", Czech),
  LangInfo("Danish", Danish),
  LangInfo("Dutch", Dutch),
  LangInfo("English", English),
  LangInfo("Finnish", Finnish),
  LangInfo("French", French),
  LangInfo("German", German),
  LangInfo("Greek", Greek),
  LangInfo("Hungarian", Hungarian),
  LangInfo("Icelandic", Icelandic),
  LangInfo("Italian", Italian),
  LangInfo("Japanese", Japanese),
  LangInfo("Korean", Korean),
  LangInfo("Norwegian", Norwegian),
  LangInfo("Polish", Polish),
  LangInfo("Portuguese", Portuguese),
  LangInfo("Russian", Russian),
  LangInfo("SerboCroatian", SerboCroatian),
  LangInfo("Slovak", Slovak),
  LangInfo("Spanish", Spanish),
  LangInfo("Swedish", Swedish),
  LangInfo("Turkish", Turkish)
};

int FindLangID(const char *language)
{
  for (int i=0; i<lenof(LangTable); i++)
  {
    if (stricmp(language, LangTable[i]._name) == 0) return LangTable[i]._id;
  }
  RptF("Unsupported language: %s", language);
	return English;
}

const char *GetLangName(int language)
{
  for (int i=0; i<lenof(LangTable); i++)
  {
    if (language == LangTable[i]._id) return LangTable[i]._name;
  }
  RptF("Unsupported language: %d", language);
  return "English";
}

int GetLangID()
{
	return LangID;
}

void SetLangID(const char *language)
{
	LangID = FindLangID(language);
}

#define CP_KOREAN				949
#define CP_KOREAN_JOHAB	1361

void MultiByteStringToPseudoCodeString(const char *in, char *out)
{
#ifdef _WIN32
	const int bufferSize = 1024;
	wchar_t buffer[1024]; buffer[0] = 0;
	int len = MultiByteToWideChar
	(
		CP_KOREAN,				 // code page
		0,				         // character-type options
		in,								 // address of string to map
		-1,						     // number of bytes in string
		buffer,						 // address of wide-character buffer
		bufferSize				 // size of buffer
	);
	if (len == 0) Fail("Cannot translate into unicode");

	len = WideCharToMultiByte
	(
		CP_KOREAN_JOHAB,	 // code page
		0,				         // character-type options
		buffer,						 // address of string to map
		-1,						     // number of bytes in string
		out,							 // address of wide-character buffer
		bufferSize,				 // size of buffer
		NULL, NULL				 // default chars
	);
	if (len == 0) Fail("Cannot translate from unicode");
#else
	Fail("MultiByteStringToPseudoCodeString is not implemented");
#endif
}

const byte table[3][32] = {
    { 0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,0,1,2,3,4,5,0,0,6,7,8,9,10,11,0,0,12,13,14,15,16,17,0,0,18,19,20,21,0,0 },
    { 0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,0,17,18,19,20,21,22,23,24,25,26,27,0,0 }
};

const byte fsttable[2][20] = {
    { 0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1 },
     { 0,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,3,3 }
};

const byte midtable[3][22] = {
    { 0,0,2,0,2,1,2,1,2,3,0,2,1,3,3,1,2,1,3,3,1,1 },
    { 0,0,0,0,0,0,0,0,0,1,3,3,3,1,2,4,4,4,2,1,3,0 },
    { 0,5,5,5,5,5,5,5,5,6,7,7,7,6,6,7,7,7,6,6,7,5 }
};

const int baseChoSung = 0x0080;
const int baseJoongSung = baseChoSung + 8 * 20;
const int baseJongSung = baseJoongSung + 4 * 22;

bool PseudoCodeToPictureCode(const char *in, short *out)
{
	DoAssert(*in & 0x80);
	int pseudocode = (*(unsigned char *)in << 8) | *((unsigned char *)in + 1);
	
	int first = table[0][(pseudocode & 0x7c00) >> 10];
	int mid = table[1][(pseudocode & 0x03e0) >> 5];
	int last = table[2][pseudocode & 0x001f];

	int b1, b2, b3;
	b3 = midtable[0][mid];					// Find index of Jong Sung Character for typography of Joong Sung in JoongSung Table
	if (last == 0)									// Is Jong sung not used
	{
		b2 = fsttable[0][first];			// Get one of Joong Sung Index
		b1 = midtable[1][mid];				// Get one of Cho Sung Index
	}
	else
	{
		b2 = fsttable[1][first];			// Get one of Joong Sung Index
    b1 = midtable[2][mid];				// Get one of Cho Sung Index
	}

	*(out++) = first == 0 ? baseChoSung : baseChoSung + b1 * 20 + first;
	*(out++) = mid == 0 ? baseChoSung : baseJoongSung + b2 * 22 + mid;
	*(out++) = last == 0 ? baseChoSung : baseJongSung + b3 * 28 + last;

	return true;
}
