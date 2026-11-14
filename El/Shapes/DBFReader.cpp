#include <Es/essencepch.hpp>
#include "StdAfx.h"
#include "ShapeDatabase.h"
#include "dbfreader.h"
//#include <fstream>

namespace ShapeFiles
{
	using namespace std;

	DBFReader::ReaderError DBFReader::ReadDBF(QIStream& input, ShapeDatabase& database, const Array<unsigned int>& shapeIndexes, IDBColumnNameConvert* t)
	{
		input.read((char*)&_header, sizeof(_header));
		if (input.fail()) return errHeaderReadError;

		_fieldInfo.Clear();
		unsigned char mark;
		input.read((char*)&mark, 1);
    if (input.fail()) return errHeaderReadError;
		while (mark != 0x0D)
		{
			FieldInfo finfo;
			finfo.fieldName[0] = mark;
			input.read((char*)&finfo + 1, sizeof(finfo) - 1);
      if (input.fail()) return errHeaderReadError;
			input.read((char*)&mark, 1);
      if (input.fail()) return errHeaderReadError;
			finfo.fieldName[10] = 0;
			int id = _fieldInfo.Size();
			_fieldInfo.Add(finfo);
			if (t) 
			{
				_fieldTranslated.Add(t->TranslateColumnName(_fieldInfo[id].fieldName));
			}
			else 
			{
				_fieldTranslated.Add(_fieldInfo[id].fieldName);
			}
		}
		input.seekg(_header.headersz);
		int totalRecords = __min(_header.records, (unsigned)shapeIndexes.Size());
		char* buffer = (char*)alloca(__max(_header.reclen, 255) + 1);
		int indexpos = 0;
		for (int i = 0; i < totalRecords; i++)
		{
			char delmark;
			input.read(&delmark, 1);
			if (input.fail()) return errErrorReadingData;
			if (delmark == ' ')
			{      
				for (int j = 0; j < _fieldInfo.Size(); j++)
				{
					input.read(buffer, _fieldInfo[j].length);
					if (input.fail()) return errErrorReadingData;
					buffer[_fieldInfo[j].length] = 0;
					switch (_fieldInfo[j].fieldType)
					{
					case 'B':
					case 'M':
					case 'G': buffer[0] = 0; break; //NO VALUE, this type is not supported
					case '@': buffer[0] = 0; break; //NO VALUE, this type is not supported
					case '+':
					case '|':
						{
							long* l = reinterpret_cast<long*>(buffer);
							if (*l & 0x80000000) *l = -(*l & 0x7FFFFFFF);
							_ltoa(*l, buffer, 10);
						}
						break;
					case 'O':
						{
							double* l = reinterpret_cast<double*>(buffer);
							sprintf(buffer, "%g", *l);
						}
						break;
					}
					while (buffer[0] == ' ') strcpy(buffer, buffer + 1);
					int l = strlen(buffer);
					while (l > 0 && buffer[l - 1] == ' ') buffer[--l] = 0;

					unsigned int shapeId = shapeIndexes[indexpos];
					if (shapeId != -1)
					{
						database.SetField(shapeId, _fieldTranslated[j], buffer);
					}
				}
				indexpos++;
			}
		}
		return errOk;  
	}

	DBFReader::ReaderError DBFReader::ReadDBF(const char* fname, ShapeDatabase& database, const Array<unsigned int>& shapeIndexes, IDBColumnNameConvert* t)
	{
		QIFStream input;
    input.open(fname);
		if (input.fail()) return errFileOpenError;
		return ReadDBF(input, database, shapeIndexes, t);
	}
}