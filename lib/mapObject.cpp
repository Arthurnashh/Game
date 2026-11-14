#include "wpch.hpp"
#include "mapObject.hpp"
#include "Shape/mapTypes.hpp"
#include "object.hpp"
#include <El/QStream/serializeBin.hpp>
#include "serializeBinExt.hpp"

class MapObjectWithPos : public MapObject
{
	typedef MapObject base;
protected:
	float _x;
	float _z;

public:
	explicit MapObjectWithPos(MapType type) : base(type) {}
	MapObjectWithPos
	(
		MapType type, const VisitorObjId &id, float x, float z
	) : base(type, id) {_x = x; _z = z;}
	virtual Vector3 GetPosition(Vector3Par def) const {return Vector3(_x, 0, _z);}
	virtual void SerializeBin(SerializeBinStream &f);
	virtual size_t GetMemoryControlled() const {return sizeof(*this);}
};

class MapObjectWithLine : public MapObject
{
  typedef MapObject base;
protected:
  float _x1;
  float _z1;
  float _x2;
  float _z2;

public:
  explicit MapObjectWithLine(MapType type) : base(type) {}
  MapObjectWithLine(MapType type, const VisitorObjId &id, float x1, float z1, float x2, float z2)
    : base(type, id) {_x1 = x1; _z1 = z1; _x2 = x2; _z2 = z2;}
  virtual Vector3 GetTLPosition() const {return Vector3(_x1, 0, _z1);}
  virtual Vector3 GetTRPosition() const {return Vector3(_x2, 0, _z2);}
  virtual Vector3 GetBLPosition() const {return Vector3(_x1, 0, _z1);}
  virtual Vector3 GetBRPosition() const {return Vector3(_x2, 0, _z2);}
  virtual Vector3 GetPosition(Vector3Par def) const
  {
    return Vector3(0.5 * (_x1 + _x2), 0, 0.5 * (_z1 + _z2));
  }
  virtual void SerializeBin(SerializeBinStream &f);
  virtual size_t GetMemoryControlled() const {return sizeof(*this);}
};

class MapObjectRailWay : public MapObject
{
  typedef MapObject base;
protected:
  float _x1;
  float _z1;
  float _x2;
  float _z2;
  float _x3;
  float _z3;
 
  bool _moreparts;

public:
  explicit MapObjectRailWay(MapType type) : base(type) {};
  MapObjectRailWay(MapType type, const VisitorObjId &id, float x1, float z1, float x2, float z2, float x3, float z3, bool moreparts)
    : base(type, id) {_x1 = x1; _z1 = z1; _x2 = x2; _z2 = z2; _x3 = x3; _z3 = z3; _moreparts = moreparts;}
  virtual Vector3 GetTLPosition() const {return Vector3(_x1, 0, _z1);}
  virtual Vector3 GetTRPosition() const {return Vector3(_x2, 0, _z2);}
  virtual Vector3 GetBLPosition() const {return Vector3(_x3, 0, _z3);} //returns optional vertex (second part of dividing railway)
  //virtual Vector3 GetBRPosition() const {return Vector3(_x2, 0, _z2);}

  virtual bool HasMoreParts() const {return _moreparts;}

  virtual Vector3 GetPosition(Vector3Par def) const
  {
    return Vector3(0.5 * (_x1 + _x2), 0, 0.5 * (_z1 + _z2));
  }

  virtual void SetTwoLines() {_moreparts = true;};
  virtual void SerializeBin(SerializeBinStream &f);
  virtual size_t GetMemoryControlled() const {return sizeof(*this);}
};


class MapObjectWithAngle : public MapObject
{
	typedef MapObject base;
protected:
	float _angle;

public:
	explicit MapObjectWithAngle(MapType type) : base(type) {}
	MapObjectWithAngle
	(
		MapType type, const VisitorObjId &id, float angle
	) : base(type, id) {_angle = angle;}
	virtual float GetAngle() const {return _angle;}
  virtual void SetAngle(float angle) {_angle = angle;}
	virtual void SerializeBin(SerializeBinStream &f);
	virtual size_t GetMemoryControlled() const {return sizeof(*this);}
};

class MapObjectWithRect : public MapObject
{
	typedef MapObject base;
protected:
	float _xTL, _zTL;
	float _xTR, _zTR;
	float _xBL, _zBL;
	float _xBR, _zBR;

public:
	explicit MapObjectWithRect(MapType type) : base(type) {}
	MapObjectWithRect
	(
		MapType type, const VisitorObjId &id,
		float xTL, float zTL, float xTR, float zTR,
		float xBL, float zBL, float xBR, float zBR
	) : base(type, id)
	{
		_xTL = xTL; _zTL = zTL;
		_xTR = xTR; _zTR = zTR;
		_xBL = xBL; _zBL = zBL;
		_xBR = xBR; _zBR = zBR;
	}
	virtual Vector3 GetTLPosition() const {return Vector3(_xTL, 0, _zTL);}
	virtual Vector3 GetTRPosition() const {return Vector3(_xTR, 0, _zTR);}
	virtual Vector3 GetBLPosition() const {return Vector3(_xBL, 0, _zBL);}
	virtual Vector3 GetBRPosition() const {return Vector3(_xBR, 0, _zBR);}
	virtual Vector3 GetPosition(Vector3Par def) const
	{
		return Vector3(0.25 * (_xTL + _xTR + _xBL + _xBR), 0, 0.25 * (_zTL + _zTR + _zBL + _zBR));
	}
	virtual void SerializeBin(SerializeBinStream &f);
	virtual size_t GetMemoryControlled() const {return sizeof(*this);}
};

class MapObjectWithRectColored : public MapObjectWithRect
{
	typedef MapObjectWithRect base;
protected:
	PackedColor _color;

public:
	explicit MapObjectWithRectColored(MapType type) : base(type) {}
	MapObjectWithRectColored
	(
		MapType type, const VisitorObjId &id,
		float xTL, float zTL, float xTR, float zTR,
		float xBL, float zBL, float xBR, float zBR,
		PackedColor color
	) : base(type, id, xTL, zTL, xTR, zTR, xBL, zBL, xBR, zBR), _color(color)
	{
	}
	virtual PackedColor GetColor() const {return _color;}
	virtual void SerializeBin(SerializeBinStream &f);
	virtual size_t GetMemoryControlled() const {return sizeof(*this);}
};

class MapObjectPlain: public MapObject
{
	typedef MapObject base;

	public:
	explicit MapObjectPlain(MapType type) : base(type) {}
	MapObjectPlain
	(
		MapType type, const VisitorObjId &id
	) : base(type, id) {}
	void SerializeBin(SerializeBinStream &f);
	virtual size_t GetMemoryControlled() const {return sizeof(*this);}
};

void operator << (SerializeBinStream &f, MapType &type)
{
	f.TransferBinary(&type,sizeof(type));
}

void operator << (SerializeBinStream &f, VisitorObjId &id)
{
	f.TransferLittleEndian(id);
}

void MapObject::SerializeBin(SerializeBinStream &f)
{
	// provide base implementation
	// when loading, _type and _id is loaded before creation
	if (f.IsSaving())
	{
		f.Transfer(_type);
	}
	f.TransferLittleEndian(_id);
}

void MapObjectForest::SerializeBin(SerializeBinStream &f)
{
  base::SerializeBin(f);
  f.Transfer(_row);
  f.Transfer(_col);
  f.Transfer(_rows);
  f.Transfer(_cols);
  f.Transfer(_tl);
  f.Transfer(_tr);
  f.Transfer(_bl);
  f.Transfer(_br);
}

void MapObjectPlain::SerializeBin(SerializeBinStream &f)
{
	base::SerializeBin(f);
}

void MapObjectWithPos::SerializeBin(SerializeBinStream &f)
{
	base::SerializeBin(f);
	f.Transfer(_x);
	f.Transfer(_z);
}

void MapObjectWithLine::SerializeBin(SerializeBinStream &f)
{
  base::SerializeBin(f);
  f.Transfer(_x1);
  f.Transfer(_z1);
  f.Transfer(_x2);
  f.Transfer(_z2);
}

void MapObjectRailWay::SerializeBin(SerializeBinStream &f)
{
  base::SerializeBin(f);
  f.Transfer(_x1);
  f.Transfer(_z1);
  f.Transfer(_x2);
  f.Transfer(_z2);
  f.Transfer(_x3);
  f.Transfer(_z3);
  f.Transfer(_moreparts);
}

void MapObjectWithRect::SerializeBin(SerializeBinStream &f)
{
	base::SerializeBin(f);
	f.Transfer(_xTL);
	f.Transfer(_zTL);
	f.Transfer(_xTR);
	f.Transfer(_zTR);
	f.Transfer(_xBL);
	f.Transfer(_zBL);
	f.Transfer(_xBR);
	f.Transfer(_zBR);
}
void MapObjectWithRectColored::SerializeBin(SerializeBinStream &f)
{
	base::SerializeBin(f);
	f.Transfer(_color);
	if (f.IsLoading())
	{
	  _color.SetA8(0xff);
	  int maxC = intMax(_color.R8(),_color.G8(),_color.B8());
	  const int maxBright = 0x80;
	  if (maxC>maxBright)
	  {
	    float darken = float(maxBright)/maxC;
	    _color = PackedColor(
	      toInt(_color.R8()*darken),
	      toInt(_color.G8()*darken),
	      toInt(_color.B8()*darken),
	      _color.A8()
	    );
	  }
	}
}
void MapObjectWithAngle::SerializeBin(SerializeBinStream &f)
{
	base::SerializeBin(f);
	f.Transfer(_angle);
}

MapObject *CreateMapObject(MapType type)
{
	switch (type)
	{
    MAP_TYPES_ICONS_1(MAP_TYPE_CASE)
		case MapHide:
			return new MapObjectWithPos(type);
    case MapTrack:
		case MapRoad:
    case MapMainRoad:
			return new MapObjectWithRect(type);
    MAP_TYPES_ICONS_2(MAP_TYPE_CASE)
		case MapBuilding:
		case MapHouse:
		case MapFence:
		case MapWall:
			return new MapObjectWithRectColored(type);
    case MapPowerLines:
      return new MapObjectWithLine(type);
    case MapRailWay:
      return new MapObjectRailWay(type);
		case MapForestTriangle:
			return new MapObjectWithAngle(type);
		case MapForestSquare:
		case MapForestBorder:
			return new MapObjectPlain(type);
    case MapForest:
    case MapRocks:
      return new MapObjectForest(type);
	}
	Fail("Unknown map type loaded");
	return NULL;
}

MapObject *CreateMapObject(Object *obj)
{
	if (!obj) return NULL;
	LODShape *shape = obj->GetShape();
	if (!shape) return NULL;

  Object::ProtectedVisualState<const ObjectVisualState> vs = obj->FutureVisualStateScope();

	if (obj->GetType() == Network)
	{
		Vector3 ptTL(VZero);
		Vector3 ptTR(VZero);
		Vector3 ptBL(VZero);
		Vector3 ptBR(VZero);
		
		if (shape->MemoryLevel())
		{
			ptTL = shape->MemoryPoint("LB");
			ptTR = shape->MemoryPoint("PB");
			ptBL = shape->MemoryPoint("LE");
			ptBR = shape->MemoryPoint("PE");
		}
		else
		{
			// no memory  - synthetic points required
			const Shape *geom = shape->GeometryLevel();
			if (geom)
			{
				Vector3Val min=geom->Min();
				Vector3Val max=geom->Max();
				ptTL = Vector3(min.X(),max.Y(),min.Z());
				ptTR = Vector3(max.X(),max.Y(),min.Z());
				ptBL = Vector3(min.X(),max.Y(),max.Z());
				ptBR = Vector3(max.X(),max.Y(),max.Z());
			}
		}

		ptTL = vs->PositionModelToWorld(ptTL);
		ptTR = vs->PositionModelToWorld(ptTR);
		ptBL = vs->PositionModelToWorld(ptBL);
		ptBR = vs->PositionModelToWorld(ptBR);

    MapType type = shape->GetMapType();
    if (type == MapHide)
    {
      Vector3Val pos = vs->Position();
      return new MapObjectWithPos(type, obj->ID(), pos.X(), pos.Z());
    }

    if (type != MapRoad && type != MapTrack && type != MapMainRoad) type = MapRoad;
		return new MapObjectWithRect
		(
			type, obj->ID(),
			ptTL.X(), ptTL.Z(),
			ptTR.X(), ptTR.Z(),
			ptBL.X(), ptBL.Z(),
			ptBR.X(), ptBR.Z()
		);
	}
	else if (obj->GetType() == Primary
#if _ARMA3_TERRAIN_DECALS
    || obj->GetType() == LandDecal 
#endif                               
    )
	{
		MapType type = shape->GetMapType();
		switch (type)
		{
    MAP_TYPES_ICONS_2(MAP_TYPE_CASE)
		case MapBuilding:
		case MapHouse:
		case MapFence:
		case MapWall:
			{
				const Vector3 *minmax = shape->MinMax();
				Vector3 ptTL = vs->PositionModelToWorld(Vector3(minmax[0].X(), 0, minmax[0].Z()));
				Vector3 ptTR = vs->PositionModelToWorld(Vector3(minmax[1].X(), 0, minmax[0].Z()));
				Vector3 ptBL = vs->PositionModelToWorld(Vector3(minmax[0].X(), 0, minmax[1].Z()));
				Vector3 ptBR = vs->PositionModelToWorld(Vector3(minmax[1].X(), 0, minmax[1].Z()));
				return new MapObjectWithRectColored
				(
					type, obj->ID(),
					ptTL.X(), ptTL.Z(),
					ptTR.X(), ptTR.Z(),
					ptBL.X(), ptBL.Z(),
					ptBR.X(), ptBR.Z(),
					shape->Color()
				);
			}
    case MapPowerLines:
      {
        Vector3 pt1 = vs->PositionModelToWorld(shape->MemoryPoint("map1"));
        Vector3 pt2 = vs->PositionModelToWorld(shape->MemoryPoint("map2"));
        return new MapObjectWithLine(type, obj->ID(), pt1.X(), pt1.Z(), pt2.X(), pt2.Z());
      }

    case MapRailWay:
      {
       
        Vector3 pt1 = vs->PositionModelToWorld(shape->MemoryPoint("map1"));
        Vector3 pt2 = vs->PositionModelToWorld(shape->MemoryPoint("map2"));

        Vector3 mpt = shape->MemoryPoint("map3");
        Vector3 pt3 = vs->PositionModelToWorld(mpt);
   
        if (mpt!=VZero) //is there 3rd point (map3)
          return new MapObjectRailWay(type, obj->ID(), pt1.X(), pt1.Z(), pt2.X(), pt2.Z(), pt3.X(), pt3.Z(),true);  
        else
          return new MapObjectRailWay(type, obj->ID(), pt1.X(), pt1.Z(), pt2.X(), pt2.Z(), pt3.X(), pt3.Z(),false);      
      }



    MAP_TYPES_ICONS_1(MAP_TYPE_CASE)
		case MapHide:
			{
				Vector3Val pos = vs->Position();
				return new MapObjectWithPos(type, obj->ID(), pos.X(), pos.Z());
			}
		case MapForestTriangle:
			{
				Vector3Val dir = vs->Direction();
				return new MapObjectWithAngle(type, obj->ID(), atan2(dir.X(), dir.Z()));
			}
		default:
			Fail("Unknown map type");
			break;
		case MapForestSquare:
		case MapForestBorder:
			return new MapObjectPlain(type, obj->ID());
		}
	}
	return NULL;
}

