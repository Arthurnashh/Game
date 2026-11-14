#pragma once

#include "El\BTree\BTree.h"
namespace ShapeFiles
{
	class ShapeDatabase
	{
		struct DBItem
		{
			unsigned int _shapeId;
			RStringI _paramName;
			RStringI _value;

			DBItem(unsigned int shapeId = 0, const RStringI& paramName = RStringI(), const RStringI& value = RStringI()):
			_shapeId(shapeId),
			_paramName(paramName),
			_value(value)
			{
			}

			int CompareWith(const DBItem& other) const
			{
				if (_shapeId == other._shapeId)
				{
					return _stricmp(_paramName, other._paramName);
				}
				return (_shapeId > other._shapeId) - (_shapeId < other._shapeId);
			}

			bool operator == (const DBItem& other) const 
			{
				return CompareWith(other) == 0;
			}

			bool operator >= (const DBItem& other) const 
			{
				return CompareWith(other) >= 0;
			}

			bool operator <= (const DBItem& other) const 
			{
				return CompareWith(other) <= 0;
			}

			bool operator != (const DBItem& other) const 
			{
				return CompareWith(other) != 0;
			}

			bool operator > (const DBItem& other) const 
			{
				return CompareWith(other) > 0;
			}

			bool operator < (const DBItem& other) const 
			{
				return CompareWith(other) < 0;
			}
		};

		BTree<DBItem>   _database;
		BTree<RStringS> _stringTable;

		RString ShareString(const RString& text);
		RString ShareString(const RString& text) const;

	public:
		bool SetField(unsigned int shapeId, const char* fieldName, const char* value);
		const char* GetField(unsigned int shapeId, const char* fieldName) const;

		template<class T>
		bool ForEachField(const T& functor, unsigned int shapeId = -1) const
		{
			BTreeIterator<DBItem> iter(_database);
			iter.BeginFrom(DBItem(shapeId == -1 ? 0 : shapeId));
			DBItem* found;
			while ((found = iter.Next()) != 0 && (shapeId == -1 || shapeId == found->_shapeId))
			{
				if (functor(found->_shapeId, found->_paramName, found->_value)) return true;
			}
			return false;
		}
		bool DeleteShape(unsigned int shapeId);
		bool DeleteField(unsigned int shapeId, const char* fieldName);
		void Clear();
		unsigned int GetNextUsedShapeID(int currentID) const;
	};
}
