#include <Es/essencepch.hpp>
#include "StdAfx.h"
#include ".\shapedatabase.h"

namespace ShapeFiles
{
	RString ShapeDatabase::ShareString(const RString& text)
	{
		RString* nw = _stringTable.Find(text);
		if(nw)
		{
			return *nw;
		}
		else
		{
			_stringTable.Add(text);
			return text;
		}
	}

	RString ShapeDatabase::ShareString(const RString& text) const
	{
		RString* nw = _stringTable.Find(text);
		if (nw) 
		{
			return *nw;
		}
		else 
		{
			return text;
		}
	}

	bool ShapeDatabase::SetField(unsigned int shapeId, const char* fieldName, const char* value)
	{
		RString field = fieldName;
		field.Upper();
		field = ShareString(field);
		RString v = value;
		v = ShareString(v);

		DBItem* i = _database.Find(DBItem(shapeId, field));
		if(i) 
		{
			i->_value = v;
			return true;
		}
		else
		{
			return _database.Add(DBItem(shapeId, field, v));
		}
	}

	const char* ShapeDatabase::GetField(unsigned int shapeId, const char* fieldName) const
	{
		RString field = fieldName;
		field.Upper();
		field = ShareString(field);

		const DBItem* found = _database.Find(DBItem(shapeId, field));
		if (found) 
		{
			return found->_value;
		}
		else 
		{
			return 0;
		}
	}

	bool ShapeDatabase::DeleteShape(unsigned int shapeId)
	{
		bool res = false;
		BTreeIterator<DBItem> iter(_database);
		DBItem* found;
		iter.BeginFrom(DBItem(shapeId));
		while ((found = iter.Next()) != 0 && found->_shapeId == shapeId)
		{
			_database.Remove(*found);
			iter.BeginFrom(DBItem(shapeId));
			res = true;
		}
		return res;
	}

	bool ShapeDatabase::DeleteField(unsigned int shapeId, const char* fieldName)
	{
		RString field=fieldName;
		field.Upper();
		field = const_cast<const ShapeDatabase*>(this)->ShareString(field);

		_database.Remove(DBItem(shapeId, field));
		return true;
	}

	void ShapeDatabase::Clear()
	{
		_database.Clear();
		_stringTable.Clear();
	}

	unsigned int ShapeDatabase::GetNextUsedShapeID(int currentID) const
	{
		currentID++;
		BTreeIterator<DBItem> iter(_database);
		DBItem* found;
		iter.BeginFrom(DBItem(currentID));
		found = iter.Next();
		if (found == 0) return -1;
		return found->_shapeId;
	}
}