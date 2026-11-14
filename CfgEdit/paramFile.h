#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PARAM_FILE_HPP
#define _PARAM_FILE_HPP

class CParamFile : public CMemFile
{
protected:
	int _line;
public:
	int GetLine() const {return _line;}
	void Rewind() {SeekToBegin(); _line = 1;}
	char GetChar();
	void UngetChar();
};

enum ParamType
{
	PTValue,
	PTClass,
	PTEnum,
};

class ParamClass
{
protected:
	CString _name;
	CString _base;
	int _line;
	ParamType _type;
	ParamClass *_parent;
	CArray<ParamClass *, ParamClass *> _entries;

public:
	ParamClass()
	{
		_name = ""; _base = "";
		_line = 0; _type = PTClass;
		_parent = NULL;
	}
	~ParamClass();

	void Clear();
	CString GetContext(const char *member = NULL) const;
	void Parse(CParamFile &f); // read until "}"

	CString GetName() const {return _name;}
	CString GetBase() const {return _base;}
	int GetLine() const {return _line;}
	ParamType GetType() const {return _type;}
	int GetEntryCount() const {return _entries.GetSize();}
	ParamClass *GetEntry(int i) const {return _entries[i];}
};

#endif
