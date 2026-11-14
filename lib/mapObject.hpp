#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MAP_OBJECT_HPP
#define _MAP_OBJECT_HPP

#include <El/Math/math3d.hpp>
#include <El/Color/colors.hpp>
#include "objectId.hpp"

#ifndef DECL_ENUM_MAP_TYPE
#define DECL_ENUM_MAP_TYPE
DECL_ENUM(MapType)
#endif

class SerializeBinStream;

class MapObject
{
protected:
	MapType _type;
	VisitorObjId _id;

public:
	//MapObject() {} // used for serialization
	MapObject(MapType type):_type(type) {} // used for serialization
	MapObject(MapType type, const VisitorObjId &id) : _type(type), _id(id) {}
	virtual ~MapObject() {}

	MapType GetType() const {return _type;}
	const VisitorObjId &GetId() const {return _id;}

	virtual Vector3 GetPosition(Vector3Par def) const {return def;}
	
	virtual Vector3 GetTLPosition() const {return VZero;}
	virtual Vector3 GetTRPosition() const {return VZero;}
	virtual Vector3 GetBLPosition() const {return VZero;}
	virtual Vector3 GetBRPosition() const {return VZero;}

  virtual bool HasMoreParts() const {return false;}

	virtual PackedColor GetColor() const {return PackedBlack;}

	virtual float GetAngle() const {return 0;}
  virtual void SetAngle(float angle) {}

	virtual void SerializeBin(SerializeBinStream &f) = 0;
	virtual size_t GetMemoryControlled() const = 0;

  // used for diagnostics
  //void SetType(MapType type) {_type = type;}
};

// handle directly (too much specific parameters)
class MapObjectForest : public MapObject
{
  typedef MapObject base;

public:
  unsigned char _row, _col;
  unsigned char _rows, _cols;

  float _tl, _tr, _bl, _br;

public:
  // need not initialize - will follow
  explicit MapObjectForest(MapType type) : base(type) {}

  virtual void SerializeBin(SerializeBinStream &f);
  virtual size_t GetMemoryControlled() const {return sizeof(*this);}
};

//! used when converting objects to map
MapObject *CreateMapObject(Object *obj);
//! used when loading map
MapObject *CreateMapObject(MapType type);

#endif
