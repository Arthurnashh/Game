#pragma once

#include "EL\qstream\qStream.hpp"

namespace ShapeFiles
{
	class ShapeDatabase;

	class IDBColumnNameConvert
	{
	public:
		virtual const char* TranslateColumnName(const char* name) const = 0;
	};

	class DBFReader
	{
	public:

#pragma pack(1)
		struct FieldInfo
		{
			char fieldName[11];
			char fieldType;
			unsigned long disp;
			unsigned char length;
			unsigned char decimals;
			unsigned char flags;
			unsigned long nextAutoincrement;
			unsigned char step;
			unsigned long res5;
			unsigned long res6;
			ClassIsBinaryZeroed(FieldInfo);
		};

		struct DBFHeader
		{
			unsigned char version;
			unsigned char lastupdate_yy;
			unsigned char lastupdate_mm;
			unsigned char lastupdate_dd;
			unsigned long records;
			unsigned short headersz;
			unsigned short reclen;
			unsigned short zeroes;
			char incomplette_transaction;
			char encription;
			char multiuser_reservation[12];
			char index;
			unsigned char language;
			char reserved[2];
		};

#pragma pack()
	protected:
		DBFHeader _header;
		AutoArray<FieldInfo, MemAllocLocal<FieldInfo, 32> > _fieldInfo;
		AutoArray<const char*, MemAllocLocal<const char*, 32> > _fieldTranslated;

	public:
		enum ReaderError
		{
			errOk = 0,
			errFileOpenError,
			errHeaderReadError,
			errErrorReadingData
		};

		ReaderError ReadDBF(QIStream& input, ShapeDatabase& database, const Array<unsigned int>& shapeIndexes, IDBColumnNameConvert* t = 0);    
		ReaderError ReadDBF(const char* fname, ShapeDatabase& database, const Array<unsigned int>& shapeIndexes, IDBColumnNameConvert* t = 0);
	};
}

TypeIsSimple(const char*);