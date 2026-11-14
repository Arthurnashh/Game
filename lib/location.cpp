#include "wpch.hpp"
#include "location.hpp"

#include "El/Stringtable/stringtable.hpp"
#include <El/Evaluator/express.hpp>

#include "fileLocator.hpp"
#include "paramArchiveExt.hpp"
#include "landscape.hpp"
#include "world.hpp"
#include "UI/uiMap.hpp"
#include "mbcs.hpp"

#include <Es/Algorithms/qsort.hpp>

/*!
\patch 2.92 Date 03/02/2006 by Pete
- Added: function nearestLocations
*/

LocationTypeBank LocationTypes;

DEFINE_ENUM(LocationDrawStyle, LDS, LOCATION_DRAW_STYLE_ENUM)

LocationType::LocationType(RString name)
{
  _name = name;

  ParamEntryVal cls = Pars >> "CfgLocationTypes" >> name;
  
  RStringB style = cls >> "drawStyle";
  _drawStyle = GetEnumValue<LocationDrawStyle>(style);

  RString texture = cls >> "texture";
  _texture = GlobLoadTexture(GetPictureName(texture));
  _color = GetPackedColor(cls >> "color");
  if (_drawStyle == LDSArea) _size = 1;
  else _size = cls >> "size";
  _shadow = cls >> "shadow";

  _font = GEngine->LoadFont(GetFontID(cls >> "font"));
  _textSize = cls >> "textSize";

  _importance = 0;
  ConstParamEntryPtr ptr = cls.FindEntry("importance");
  if (ptr) _importance = *ptr;
}

Location::Location()
{
  _position = VZero;
  _a = 0;
  _b = 0;
  _angle = 0;
  _rectangular = false;

  _importance = 0;
  _nearestMoreImportant = 0;
  _side = TSideUnknown;

  _id = -1;
}

Location::Location(RString typeName, Vector3Par position, float a, float b)
{
  _type = LocationTypes.New(typeName);
  
  _position = position;
  _a = a;
  _b = b;
  _angle = 0;
  _rectangular = false;

  _importance = _type ? _type->_importance : 0;
  _nearestMoreImportant = 0;
  _side = TSideUnknown;

  _id = -1;
}

Location::Location(LocationType *type, Vector3Par position, float a, float b)
{
  _type = type;

  _position = position;
  _a = a;
  _b = b;
  _angle = 0;
  _rectangular = false;

  _importance = _type ? _type->_importance : 0;
  _nearestMoreImportant = 0;
  _side = TSideUnknown;

  _id = -1;
}

Location::Location(LocationType *type, Vector3Par position, float a, float b, float angle)
{
  _type = type;

  _position = position;
  _a = a;
  _b = b;
  _angle = angle;
  _rectangular = false;

  _importance = _type ? _type->_importance : 0;
  _nearestMoreImportant = 0;
  _side = TSideUnknown;

  _id = -1;
}

Vector3Val Location::GetPosition() const
{
  if (_attached) return _attached->FutureVisualState().Position();
  return _position;
}

bool Location::IsInside(Vector3Par pos) const
{
  Vector3Val locPos = GetPosition();

  float aa = _a;
  float bb = _b;
  float angle = _angle;
  if (aa < bb)
  {
    Swap(aa, bb); 
    angle += 0.5 * H_PI;
  }

  float cosAngle = cos(angle);
  float sinAngle = sin(angle);

  if (_rectangular)
  {
    Vector3 f1 = locPos + Vector3(+cosAngle * aa, 0, -sinAngle * aa);
    Vector3 f2 = locPos + Vector3(+sinAngle * bb, 0, +cosAngle * bb);

    Vector3 b = locPos;
    Vector3 e = f1 - b;
    Vector3 p = pos - b;
    float eSize2 = e.SquareSize();
    if (eSize2 == 0) return false;
    float t = (e * p) / eSize2;
    if (t < -1.0 || t > 1.0) return false;

    e = f2 - b;
    eSize2 = e.SquareSize();
    if (eSize2==0) return false;
    t = (e * p) / eSize2;
    if (t < -1.0 || t > 1.0) return false;

    return true;
  }
  else
  {
    float e = sqrt(Square(aa) - Square(bb));
    Vector3 diff(+cosAngle * e, 0, -sinAngle * e);
    Vector3 f1 = locPos - diff;
    Vector3 f2 = locPos + diff;
    float p = (pos - f1).SizeXZ() + (pos - f2).SizeXZ();
    return p <= 2.0 * aa;
  }

}

//-----------------------------------------------------------------------------
// 19.4. 2010 CardA - [ADDED] elevationOffset value(will be added to position.Y)
void Location::Draw(CStaticMap *map, bool editor,float elevationOffset) const
{
  Vector3 position = editor ? _position : GetPosition();

  // adding preset elevation from map config
  position[1] = position[1] + elevationOffset;

  switch (_type->_drawStyle)
  {
  case LDSIcon:
    map->DrawSign(
      _type->_texture, _type->_color,
      position,
      _type->_size, _type->_size,
      _angle, Localize(_text),
      _type->_font, _type->_textSize, _type->_shadow
      );
    break;
  case LDSArea:
    if (_rectangular)
    {
      map->FillRectangle(
        position, _a, _b, _angle,
        _type->_color, _type->_texture
        );
    }
    else
    {
      map->FillEllipse(
        position, _a, _b, _angle,
        _type->_color, _type->_texture
        );
    }
    if (_text.GetLength() > 0)
    {
      float offset = fabs(cos(_angle) * _a) + fabs(sin(_angle) * _b);
      map->DrawSign(
        NULL, _type->_color,
        Vector3(position.X() + offset, position.Y(), position.Z()),
        0, 0, 0, Localize(_text),
        _type->_font, _type->_textSize, _type->_shadow
        );
    }
    break;
  case LDSMount:
    map->DrawMount(position);
    break;
  case LDSName:
    map->DrawName(position, _text, _type->_font, _type->_textSize, _type->_color, _type->_shadow);
    break;
  }
}

LSError Location::Serialize(ParamArchive &ar)
{
  if (ar.IsSaving())
  {
    RString name;
    if (_type) name = _type->_name;
    CHECK(ar.Serialize("type", name, 1))
  }
  else if (ar.GetPass() == ParamArchive::PassFirst)
  {
    RString name;
    CHECK(ar.Serialize("type", name, 1))
    if (name.GetLength() > 0) _type = LocationTypes.New(name);
  }

  CHECK(::Serialize(ar, "position", _position, 1))
  CHECK(ar.SerializeRef("attached", _attached, 1))
  CHECK(ar.Serialize("a", _a, 1))
  CHECK(ar.Serialize("b", _b, 1))
  CHECK(ar.Serialize("angle", _angle, 1))
  CHECK(ar.Serialize("rectangular", _rectangular, 1))

  CHECK(ar.Serialize("importance", _importance, 1))
  CHECK(ar.Serialize("text", _text, 1))
  CHECK(ar.SerializeEnum("side", _side, 1, TSideUnknown))

  CHECK(ar.Serialize("varName", _varName, 1))
  CHECK(ar.Serialize("className",_className, 1))

  CHECK(ar.Serialize("id", _id, 1))

  return LSOK;
}

Location *Location::LoadRef(ParamArchive &ar)
{
  int id;
  if (ar.Serialize("id", id, 1) != LSOK) return NULL;
  if (id >= 0)
  {
#if USE_LANDSCAPE_LOCATIONS
    // static location
    float x, y;
    if (ar.Serialize("x", x, 1) != LSOK) return NULL;
    if (ar.Serialize("y", y, 1) != LSOK) return NULL;
    return GLandscape->GetLocations().Get(x, y, id);
#else
    return NULL;
#endif
  }
  else
  {
    // dynamic location
    return GWorld->GetLocations()[-id - 1];
  }
}

LSError Location::SaveRef(ParamArchive &ar)
{
  if (IsStatic())
  {
    CHECK(ar.Serialize("x", _position[0], 1))
    CHECK(ar.Serialize("y", _position[2], 1))
    CHECK(ar.Serialize("id", _id, 1))
  }
  else
  {
    CHECK(ar.Serialize("id", _id, 1))
  }
  return LSOK;
}

LSError DynamicLocation::Serialize(ParamArchive &ar)
{
  CHECK(Location::Serialize(ar))

  // variables
  void *old = ar.GetParams();
  ar.SetParams(&GGameState);
  DoAssert(_vars.IsSerializationEnabled());
  LSError result = ar.Serialize("Variables", _vars._vars, 1);
  ar.SetParams(old);
  
  return result;
}

struct LocationPosition
{
  int _x, _y;

  LocationPosition(int x, int y)
  {
    _x = x; _y = y;
  }

  bool operator () (int x, int y, int size) const
  {
    return _x >= x && _y >= y && _x < x + size && _y < y + size;
  }
};

struct LocationAdd
{
  Location *_location;

  LocationAdd(Location *location)
  {
    _location = location;
  }

  bool operator () (const Ref<LocationsItem> &item) const
  {
    if (item->_x == _location->GetX() && item->_y == _location->GetY())
    {
      int id = item->_locations.AddUnique(_location);
      _location->_id = id;
      item->_locations.Compact();
      return true;
    }
    return false;
  }
};

struct LocationRemove
{
  Location *_location;

  LocationRemove(Location *location)
  {
    _location = location;
  }

  bool operator () (const Ref<LocationsItem> &item) const
  {
    if (item->_x == _location->GetX() && item->_y == _location->GetY())
    {
      item->_locations.Delete(_location);
      // fix location ids
      for (int id=0; id<item->_locations.Size(); id++)
        item->_locations[id]->_id = id;
      return true;
    }
    return false;
  }
};

void Locations::AddLocation(Location *location)
{
  int x = location->GetX();
  int y = location->GetY();
  if (!ForEachInRegion(LocationPosition(x, y), LocationAdd(location)))
  {
    // add a new item
    LocationsItemRef item(new LocationsItem(x, y));
    item->_locations.Realloc(1);
    item->_locations.Resize(1);
    item->_locations[0] = location;
    location->_id = 0;
    Set(x, y, item);
  }
}

void Locations::RemoveLocation(Location *location)
{
  int x = location->GetX();
  int y = location->GetY();
  ForEachInRegion(LocationPosition(x, y), LocationRemove(location));
}

struct LocationGet
{
  // in
  int _x;
  int _y;
  int _id;

  // out
  Location *_location;

  LocationGet(int x, int y, int id)
  {
    _x = x; _y = y; _id = id;
    _location = NULL;
  }

  bool operator () (const Ref<LocationsItem> &item)
  {
    if (item->_x == _x && item->_y == _y)
    {
      _location = item->_locations[_id];
      return true;
    }
    return false;
  }
};

Location *Locations::Get(float x, float y, int id)
{
  int xx = toLargeInt(x);
  int yy = toLargeInt(y);

  LocationGet action(xx, yy, id);
  ForEachInRegion(LocationPosition(xx, yy), action);
  return action._location;
}

struct LocationNearest
{
  Vector3 _position;
  float _limit;

  LocationNearest(Vector3Par position, float limit2)
    : _position (position) 
  {
    _limit = sqrt(limit2); //_limit2 * InvSqrt(_limit2); not used as limit2 can be zero
  }

  bool operator () (int x, int y, int size) const
  {
    // x, y are rounded to int, so ensure we will not skip something
    if (x - size - 1 > _position.X() + _limit) return false;
    if (x + size + 1 < _position.X() - _limit) return false;
    if (y - size - 1 > _position.Z() + _limit) return false;
    if (y + size + 1 < _position.Z() - _limit) return false;
    return true;
  }
};

struct LocationGetAllInArea
{
  Vector3 _position;
  const RefArray<LocationType> &_types;
  float _limit2;
  RefArray<Location> _closestLocs;

  LocationGetAllInArea(Vector3Par position, const RefArray<LocationType> &types, float limit2)
    : _limit2(limit2), _position (position), _types(types) {}

  bool operator () (const Ref<LocationsItem> &item)
  {
    Location *location = item->_locations[0];

    // check if type matches
    bool b = true;
    for (int j=0; j<_types.Size(); j++)
    {
      if (_types[j] && location->_type == _types[j]) 
      {
        b = false;
        break;
      }
    }
    if (!b)
    {
      float dist2 = location->_position.DistanceXZ2(_position);
      if (dist2 < _limit2)
      {
        _closestLocs.Add(location);
      }
    }
    return false; // continue with enumeration
  }
};

struct LocationUpdateNearest
{
  Vector3 _position;
  Ref<LocationType> _type;
  float _limit2;
  Location *_location;

  LocationUpdateNearest(Vector3Par position, LocationType *type, float limit2)
    : _limit2(limit2), _position (position), _type(type)
  {
    _location = NULL;
  }

  bool operator () (const Ref<LocationsItem> &item)
  {
    for (int i=0; i<item->_locations.Size(); i++)
    {
      Location *location = item->_locations[i];
      if (_type && location->_type != _type) continue;
      float dist2 = location->_position.DistanceXZ2(_position);
      if (dist2 >= _limit2) continue;
      // better candidate found
      _location = location;
      _limit2 = dist2;
    }
    return false; // continue with enumeration
  }
};

Location *Locations::FindNearest(Vector3Par position, LocationType *type, float limit2)
{
  LocationNearest detect(position, limit2);
  LocationUpdateNearest action(position, type, limit2);
  ForEachInRegion(detect, action);
  return action._location;
}

RefArray<Location> Locations::FindAllInArea(Vector3Par position, const RefArray<LocationType> &types, float limit2)
{
  LocationNearest detect(position, limit2);
  LocationGetAllInArea action(position, types, limit2);
  ForEachInRegion(detect, action);
  return action._closestLocs;
}

struct LocationUpdateNearestWithDubbing
{
  Vector3 _position;
  float _limit2;
  Location *_location;

  LocationUpdateNearestWithDubbing(Vector3Par position, float limit2)
    : _limit2(limit2), _position (position)
  {
    _location = NULL;
  }

  bool operator () (const Ref<LocationsItem> &item)
  {
    for (int i=0; i<item->_locations.Size(); i++)
    {
      Location *location = item->_locations[i];
      if (location->_speech.Size() == 0) continue;
      float dist2 = location->_position.DistanceXZ2(_position);
      if (dist2 >= _limit2) continue;
      // better candidate found
      _location = location;
      _limit2 = dist2;
    }
    return false; // continue with enumeration
  }
};

Location *Locations::FindNearestWithDubbing(Vector3Par position, float limit2)
{
  LocationNearest detect(position, limit2);
  LocationUpdateNearestWithDubbing action(position, limit2);
  ForEachInRegion(detect, action);
  return action._location;
}

Location *FindNearestLocationWithDubbing(Vector3Par position, float limit2)
{
  // search for dynamic location
  Location *bestDynamic = NULL;
  for (int i=0; i<GWorld->GetLocations().Size(); i++)
  {
    Location *location = GWorld->GetLocations()[i];
    if (location->_speech.Size() == 0) continue;

    float dist2 = position.DistanceXZ2(location->GetPosition());
    if (dist2 < limit2)
    {
      bestDynamic = location;
      limit2 = dist2;
    }
  }

# if USE_LANDSCAPE_LOCATIONS
  // search for static location (nearer than the nearest dynamic)
  Location *bestStatic = GLandscape->GetLocations().FindNearestWithDubbing(position, limit2);
  return (bestStatic ? bestStatic : bestDynamic);
# else
  return bestDynamic;
# endif
}

//////////////////////////////////////////////////////////////////////////
//
// Scripting functions interface
//

#include "gameStateExt.hpp"

#define TYPES_LOCATION(XX, Category) \
  XX("LOCATION", GameLocation, CreateGameDataLocation, "Location", "Location", "Location.", Category, "GameLocation", "Lcom/bistudio/JNIScripting/RVEngine$GameLocation;")

TYPES_LOCATION(DECLARE_TYPE, "Location")

typedef LLink<Location> GameLocationType;

#include <Es/Memory/normalNew.hpp>

class GameDataLocation : public GameData
{
  typedef GameData base;

  GameLocationType _value;

public:
  GameDataLocation() {}
  GameDataLocation(const GameLocationType &value) : _value(value) {}
  ~GameDataLocation() {}

  const GameType &GetType() const {return GameLocation;}
  const GameLocationType &GetLocation() const {return _value;}

  RString GetText() const;
  RString GetClassName() const;
  bool IsEqualTo(const GameData *data) const;
  const char *GetTypeName() const {return "location";}
  GameData *Clone() const {return new GameDataLocation(*this);}

  jobject ToJObject(JNIEnv *env, GameValuePar from) const;

  LSError Serialize(ParamArchive &ar);

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

GameData *CreateGameDataLocation(ParamArchive *ar) {return new GameDataLocation();}
GameValue CreateGameLocation(Location *location) {return GameValue(new GameDataLocation(location));}

TYPES_LOCATION(DEFINE_TYPE, "Location")

template<> CachedJavaClass ConvertTypes<&GTypeGameLocation>::_cache("com/bistudio/JNIScripting/RVEngine$GameLocation");

//////////////////////////////////////////////////////////////////////////
//
// Scripting functions implementation
//

DEFINE_FAST_ALLOCATOR(GameDataLocation)

RString GameDataLocation::GetText() const
{
  if (!_value) return "No location";
  if (_value->_varName.GetLength() > 0) return _value->_varName;
  return Format("Location %s at %d, %d", cc_cast(_value->_type->GetName()), _value->GetX(), _value->GetY());
}

RString GameDataLocation::GetClassName() const
{
  if (!_value) return "No location";
  return _value->_className;
}

bool GameDataLocation::IsEqualTo(const GameData *data) const
{
  const GameLocationType &val1 = GetLocation();
  const GameLocationType &val2 = static_cast<const GameDataLocation *>(data)->GetLocation();

  return val1 == val2;
}

jobject GameDataLocation::ToJObject(JNIEnv *env, GameValuePar from) const
{
  return ConvertTypes<&GTypeGameLocation>::FromGameValue(env, from);
}

LSError GameDataLocation::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar));
  CHECK(ar.SerializeRef("value", _value, 1));
  return LSOK;
}

Location *GetLocation(GameValuePar oper)
{
  if (oper.GetType() == GameLocation)
    return static_cast<GameDataLocation *>(oper.GetData())->GetLocation();
  return NULL;
}

bool GetPos(const GameState *state, Vector3 &ret, GameValuePar oper);
Object *GetObject(GameValuePar oper);
IControl *GetControl(GameValuePar oper);

GameValue LocationCreate(const GameState *state, GameValuePar oper1)
{
  const GameArrayType &array = oper1;
  if (array.Size() != 4)
  {
    state->SetError(EvalDim, array.Size(), 4);
    return CreateGameLocation(NULL);
  }
  // type
  if (array[0].GetType() != GameString)
  {
    state->TypeError(GameString, array[0].GetType());
    return CreateGameLocation(NULL);
  }
  RString type = array[0];
  // position
  Vector3 position;
  if (!GetPos(state, position, array[1]))
  {
    return CreateGameLocation(NULL);
  }
  // sizeX
  if (array[2].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[2].GetType());
    return CreateGameLocation(NULL);
  }
  float sizeX = array[2];
  // sizeZ
  if (array[3].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[3].GetType());
    return CreateGameLocation(NULL);
  }
  float sizeZ = array[3];

  DynamicLocation *location = new DynamicLocation(type, position, sizeX, sizeZ);
  GWorld->GetLocations().AddUnique(location);

  return CreateGameLocation(location);
}

GameValue LocationDelete(const GameState *state, GameValuePar oper1)
{
  Location *location = GetLocation(oper1);
  if (location && !location->IsStatic())
  {
    DynamicLocation *loc = static_cast<DynamicLocation *>(location);
    GWorld->GetLocations().Delete(loc);
  }
  return GameValue();
}

static int CmpLocations(const Ref<Location> *_a, const Ref<Location> *_b, const Vector3 &position)
{
  const Location *a = *_a;
  const Location *b = *_b;

  float diff = position.DistanceXZ2(a->GetPosition()) - position.DistanceXZ2(b->GetPosition());
  return sign(diff);
}
GameValue LocationsFindNearest(const GameState *state, GameValuePar oper1)
{
  // return array
  GameValue value = state->CreateGameValue(GameArray);
  GameArrayType &locations = value;

  const GameArrayType &array = oper1;
  if (array.Size() < 3 || array.Size() > 4)
  {
    state->SetError(EvalDim, array.Size(), 3);
    return value;
  }
  // position
  Vector3 position;
  if (!GetPos(state, position, array[0]))
  {
    return value;
  }
  // type
  if (array[1].GetType() != GameArray)
  {
    state->TypeError(GameArray, array[1].GetType());
    return value;
  }

  const GameArrayType &tArray = array[1];
  RefArray<LocationType> types;
  for (int i=0; i<tArray.Size(); i++)
  {
    RString typeName = tArray[i];
    if (typeName.GetLength() > 0) types.Add(LocationTypes.New(typeName));
  }

  // distance
  if (array[2].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[2].GetType());
    return value;
  }
  float distance2 = array[2]; distance2 *= distance2;

  // sort from...
  Vector3 sortPosition = position;
  if (array.Size() == 4)
  {
    if (!GetPos(state, sortPosition, array[3]))
    {
      return value;
    }
  }

  // get all locations within the specified distance
  RefArray<Location> closestLocs;
  for (int i=0; i<GWorld->GetLocations().Size(); i++)
  {
    Location *location = GWorld->GetLocations()[i];

    // check if type matches
    bool b = true;
    for (int j=0; j<types.Size(); j++)
    {
      if (types[j] && location->_type == types[j]) 
      {
        b = false;
        break;
      }
    }
    if (b) continue;

    float dist2 = position.DistanceXZ2(location->GetPosition());
    if (dist2 < distance2)
    {
      closestLocs.Add(location);
    }
  }
#if USE_LANDSCAPE_LOCATIONS
  RefArray<Location> staticLocs = GLandscape->GetLocations().FindAllInArea(position, types, distance2);
  for (int i=0; i<staticLocs.Size(); i++)
    closestLocs.Add(staticLocs[i]);
#endif
  // sort locations by distance from position
  QSort(closestLocs.Data(), closestLocs.Size(), sortPosition, CmpLocations);
  for (int i=0; i<closestLocs.Size(); i++)
    locations.Add(CreateGameLocation(closestLocs[i]));
  locations.Compact();
  return value;
}

GameValue LocationFindNearest(const GameState *state, GameValuePar oper1)
{
  const GameArrayType &array = oper1;
  if (array.Size() != 2)
  {
    state->SetError(EvalDim, array.Size(), 2);
    return CreateGameLocation(NULL);
  }
  // position
  Vector3 position;
  if (!GetPos(state, position, array[0]))
  {
    return CreateGameLocation(NULL);
  }
  // type
  if (array[1].GetType() != GameString)
  {
    state->TypeError(GameString, array[1].GetType());
    return CreateGameLocation(NULL);
  }
  RString typeName = array[1];

  Ref<LocationType> type;
  if (typeName.GetLength() > 0) type = LocationTypes.New(typeName);

  // search for dynamic location
  Location *bestDynamic = NULL;
  float limit2 = 1e30;
  for (int i=0; i<GWorld->GetLocations().Size(); i++)
  {
    Location *location = GWorld->GetLocations()[i];
    if (type && location->_type != type) continue;
    
    float dist2 = position.DistanceXZ2(location->GetPosition());
    if (dist2 < limit2)
    {
      bestDynamic = location;
      limit2 = dist2;
    }
  }

#if USE_LANDSCAPE_LOCATIONS
  // search for static location (nearer than the nearest dynamic)
  Location *bestStatic = GLandscape->GetLocations().FindNearest(position, type, limit2);
  return CreateGameLocation(bestStatic ? bestStatic : bestDynamic);
#else
  return CreateGameLocation(bestDynamic);
#endif
}

GameValue LocationFindNearestWithDubbing(const GameState *state, GameValuePar oper1)
{
#if _ENABLE_CONVERSATION
  // position
  Vector3 position;
  if (!GetPos(state, position, oper1))
  {
    return CreateGameLocation(NULL);
  }

  // search for dynamic location
  Location *bestDynamic = NULL;
  float limit2 = 1e30;
  for (int i=0; i<GWorld->GetLocations().Size(); i++)
  {
    Location *location = GWorld->GetLocations()[i];
    if (location->_speech.Size() == 0) continue;

    float dist2 = position.DistanceXZ2(location->GetPosition());
    if (dist2 < limit2)
    {
      bestDynamic = location;
      limit2 = dist2;
    }
  }

# if USE_LANDSCAPE_LOCATIONS
    // search for static location (nearer than the nearest dynamic)
    Location *bestStatic = GLandscape->GetLocations().FindNearestWithDubbing(position, limit2);
    return CreateGameLocation(bestStatic ? bestStatic : bestDynamic);
# else
    return CreateGameLocation(bestDynamic);
# endif
#else
  return CreateGameLocation(NULL);
#endif
}

GameValue LocationGetPosition(const GameState *state, GameValuePar oper1)
{
  Location *location = GetLocation(oper1);
  GameValue value = state->CreateGameValue(GameArray);
  GameArrayType &array = value;
  array.Resize(3);
  if (location)
  {
    Vector3Val pos = location->_position;
    array[0] = pos.X();
    array[1] = pos.Z();
    array[2] = pos.Y() - GLandscape->SurfaceYAboveWater(pos.X(), pos.Z());
  }
  else
  {
    array[0] = 0.0f; array[1] = 0.0f; array[2] = 0.0f;
  }
  return value;
}

GameValue LocationGetLocationPosition(const GameState *state, GameValuePar oper1)
{
  Location *location = GetLocation(oper1);
  GameValue value = state->CreateGameValue(GameArray);
  GameArrayType &array = value;
  array.Resize(3);
  if (location)
  {
    Vector3Val pos = location->GetPosition();
    array[0] = pos.X();
    array[1] = pos.Z();
    array[2] = pos.Y() - GLandscape->SurfaceYAboveWater(pos.X(), pos.Z());
  }
  else
  {
    array[0] = 0.0f; array[1] = 0.0f; array[2] = 0.0f;
  }
  return value;
}
static inline GameValue LocationGetLocationPosition2(const GameState *state, GameValuePar oper1)
{
  return LocationGetLocationPosition(state, oper1);
}

GameValue LocationGetDirection(const GameState *state, GameValuePar oper1)
{
  Location *location = GetLocation(oper1);
  if (location)
    return location->_angle * (180.0f / H_PI);
  else
    return 0.0f;
}

GameValue LocationGetSize(const GameState *state, GameValuePar oper1)
{
  Location *location = GetLocation(oper1);
  GameValue value = state->CreateGameValue(GameArray);
  GameArrayType &array = value;
  array.Resize(2);
  if (location)
  {
    array[0] = location->_a;
    array[1] = location->_b;
  }
  else
  {
    array[0] = 0.0f; array[1] = 0.0f;
  }
  return value;
}

GameValue LocationGetType(const GameState *state, GameValuePar oper1)
{
  Location *location = GetLocation(oper1);
  if (location && location->_type)
    return location->_type->GetName();
  else
    return RString();
}

GameValue LocationGetImportance(const GameState *state, GameValuePar oper1)
{
  Location *location = GetLocation(oper1);
  if (location)
    return location->_importance;
  else
    return 0.0f;
}

GameValue LocationGetName(const GameState *state, GameValuePar oper1)
{
  Location *location = GetLocation(oper1);
  if (location)
    return location->_varName;
  else
    return RString();
}

GameValue LocationGetClassName(const GameState *state, GameValuePar oper1)
{
  Location *location = GetLocation(oper1);
  if (location)
    return location->_className;
  else
    return RString();
}


GameValue LocationGetText(const GameState *state, GameValuePar oper1)
{
  Location *location = GetLocation(oper1);
  if (location)
    return location->_text;
  else
    return RString();
}

GameValue LocationGetRectangular(const GameState *state, GameValuePar oper1)
{
  Location *location = GetLocation(oper1);
  if (location)
    return location->_rectangular;
  else
    return false;
}

GameValue LocationGetAttachedObject(const GameState *state, GameValuePar oper1)
{
  Location *location = GetLocation(oper1);
  if (location)
    return GameValueExt(location->_attached,GameValExtObject);
  else
    return GameValueExt((Object *)NULL,GameValExtObject);
}

GameValue LocationGetSide(const GameState *state, GameValuePar oper1)
{
  Location *location = GetLocation(oper1);
  if (location)
    return GameValueExt(location->_side);
  else
    return GameValueExt(TSideUnknown);
}

GameValue LocationSetPosition(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Ref<Location> location = GetLocation(oper1);
  if (location && !location->IsStatic())
  {
    Vector3 position;
    if (!GetPos(state, position, oper2))
      return GameValue();
    DynamicLocation *loc = static_cast<DynamicLocation *>(location.GetRef());
    // remove from world
    GWorld->GetLocations().Delete(loc);
    // update position
    loc->_position = position;
    // add to world again
    GWorld->GetLocations().AddUnique(loc);
  }
  return GameValue();
}

GameValue LocationSetDirection(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Location *location = GetLocation(oper1);
  if (location && !location->IsStatic())
  {
    float angle = oper2;
    location->_angle = angle * (H_PI / 180.0f);
  }
  return GameValue();
}

GameValue LocationSetSize(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Location *location = GetLocation(oper1);
  if (location && !location->IsStatic())
  {
    const GameArrayType &array = oper2;
    if (array.Size() != 2)
    {
      state->SetError(EvalDim, array.Size(), 2);
      return GameValue();
    }
    // sizeX
    if (array[0].GetType() != GameScalar)
    {
      state->TypeError(GameScalar, array[0].GetType());
      return GameValue();
    }
    // sizeY
    if (array[1].GetType() != GameScalar)
    {
      state->TypeError(GameScalar, array[1].GetType());
      return GameValue();
    }
    location->_a = array[0];
    location->_b = array[1];
  }
  return GameValue();
}

GameValue LocationSetType(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Location *location = GetLocation(oper1);
  if (location && !location->IsStatic())
  {
    RString type = oper2;
    location->_type = LocationTypes.New(type);
  }
  return GameValue();
}

GameValue LocationSetImportance(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Location *location = GetLocation(oper1);
  if (location && !location->IsStatic())
  {
    location->_importance = oper2;
  }
  return GameValue();
}

GameValue LocationSetName(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Location *location = GetLocation(oper1);
  if (location && !location->IsStatic())
  {
    location->_varName = oper2;
  }
  return GameValue();
}

GameValue LocationSetText(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Location *location = GetLocation(oper1);
  if (location && !location->IsStatic())
  {
    location->_text = oper2;
  }
  return GameValue();
}

GameValue LocationSetRectangular(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Location *location = GetLocation(oper1);
  if (location && !location->IsStatic())
  {
    location->_rectangular = oper2;
  }
  return GameValue();
}

GameValue LocationSetAttachedObject(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Location *location = GetLocation(oper1);
  if (location && !location->IsStatic())
  {
    Object *object = GetObject(oper2);
    location->_attached = object;
  }
  return GameValue();
}

GameValue LocationSetSide(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Location *location = GetLocation(oper1);
  if (location)
  {
    location->_side = GetSide(oper2);
  }
  return GameValue();
}

GameValue LocationDraw(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  // drawing from mission editor
  CStaticMap *map = dynamic_cast<CStaticMap *>(GetControl(oper1));
  if (!map) return GameValue();
  Location *location = GetLocation(oper2);
  if (!location) return GameValue();
  const float elevationOffset = GLandscape->GetLandElevationOffset();
  location->Draw(map, true,elevationOffset);
  return GameValue();
}

GameValue LocationSetVariable(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Location *location = GetLocation(oper1);
  if (!location) return GameValue();

  const GameArrayType &array = oper2;
  if (array.Size() != 2)
  {
    state->SetError(EvalDim, array.Size(), 2);
    return GameValue();
  }
  if (array[0].GetType() != GameString)
  {
    state->TypeError(GameString, array[0].GetType());
    return GameValue();
  }

  RString name = array[0];
  name.Lower();
  GameVarSpace *vars = location->GetVars();
  if (vars) vars->VarSet(name, array[1]);

  return GameValue();
}

GameValue LocationGetVariable(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Location *location = GetLocation(oper1);
  if (!location) return GameValue();

  RString name = oper2;
  GameVarSpace *vars = location->GetVars();
  if (vars)
  {
    name.Lower();
    GameValue var;
    if (vars->VarGet(name, var)) return var;
  }
  return GameValue();
}

GameValue LocationIsInside(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Vector3 position;
  if (!GetPos(state, position, oper1)) return false;

  Location *location = GetLocation(oper2);
  if (!location) return false;

  return location->IsInside(position);
}

GameValue LocationIsNull(const GameState *state, GameValuePar oper1)
{
  if (oper1.GetType() == GameLocation)
  {
    Location *location = GetLocation(oper1);
    return !location;
  }
  else return true;
}

GameValue LocationDistance( const GameState *state, GameValuePar oper1, GameValuePar oper2 )
{
  Vector3 pos1;
  if (oper1.GetType() == GameLocation)
  {
    Location *location = GetLocation(oper1);
    if (!location) return 1e10f;
    pos1 = location->GetPosition();
  }
  else
  {
    if (!GetPos(state, pos1, oper1)) return 1e10f;
  }

  Vector3 pos2;
  if (oper2.GetType() == GameLocation)
  {
    Location *location = GetLocation(oper2);
    if (!location) return 1e10f;
    pos2 = location->GetPosition();
  }
  else
  {
    if (!GetPos(state, pos2, oper2)) return 1e10f;
  }

  return pos1.Distance(pos2);
}
static inline GameValue LocationDistance2( const GameState *state, GameValuePar oper1, GameValuePar oper2 )
{
  return LocationDistance(state, oper1, oper2);
}
static inline GameValue LocationDistance3( const GameState *state, GameValuePar oper1, GameValuePar oper2 )
{
  return LocationDistance(state, oper1, oper2);
}

GameValue LocationNull(const GameState *state)
{
  return CreateGameLocation(NULL);
}

GameValue LocationCmpE(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Location *location1 = GetLocation(oper1);
  Location *location2 = GetLocation(oper2);
  if (!location1 || !location2) return false;
  return location1 == location2;
}

GameValue LocationCmpNE(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Location *location1 = GetLocation(oper1);
  Location *location2 = GetLocation(oper2);
  if (!location1 || !location2) return true;
  return location1 != location2;
}


//////////////////////////////////////////////////////////////////////////
//
// Scripting functions registration
//

#if DOCUMENT_COMREF
static ComRefType ExtComRefType[] =
{
  TYPES_LOCATION(COMREF_TYPE, "Location")
};
#endif

// Nulars registrations
#define NULARS_LOCATION(XX, Category) \
  XX(GameLocation, "locationNull", LocationNull, "A non-existing location. This value is not equal to anything, including itself.", "", "", "5501", "", Category) \

NULARS_LOCATION(JNI_NULAR, "Location")

static const GameNular ExtNular[]=
{
  NULARS_LOCATION(REGISTER_NULAR, "Location")
};

#if DOCUMENT_COMREF
static ComRefFunc ExtComRefFunc0[] =
{
  NULARS_LOCATION(COMREF_NULAR, "Location")
};
#endif

// Functions registrations
#define FUNCTIONS_LOCATION(XX, Category) \
  XX(GameLocation, "createLocation", LocationCreate, GameArray, "[type, position, sizeX, sizeZ]", "Create location of given type with given size at given position.", "", "", "2.90", "", Category) \
  XX(GameNothing, "deleteLocation", LocationDelete, GameLocation, "location", "Delete the given location.", "", "", "2.90", "", Category) \
  XX(GameArray, "nearestLocations", LocationsFindNearest, GameArray, "[position, [types], distance, <position to sort from>]", "Find the nearest locations (from the given position) of certain types, within the specified distance. If &lt;position to sort from&gt; is provided, locations will be ordered by distance from this point.", "", "", "2.92", "", Category) \
  XX(GameLocation, "nearestLocation", LocationFindNearest, GameArray, "[position, type]", "Find the nearest location (to the given position) of given type.", "", "", "2.90", "", Category) \
  XX(GameArray, "position", LocationGetPosition, GameLocation, "location", "Return (raw) position of given location.", "", "", "2.90", "", Category) \
  XX(GameArray, "getPos", LocationGetLocationPosition, GameLocation, "location", "Return (raw) position of given location.", "", "", "5501", "", Category) \
  XX(GameArray, "locationPosition", LocationGetLocationPosition2, GameLocation, "location", "Return position of given location.", "", "", "2.90", "", Category) \
  XX(GameScalar, "direction", LocationGetDirection, GameLocation, "location", "Return direction (angle) of given location.", "", "", "2.90", "", Category) \
  XX(GameArray, "size", LocationGetSize, GameLocation, "location", "Return size of given location (width, height).", "", "", "2.90", "", Category) \
  XX(GameString, "type", LocationGetType, GameLocation, "location", "Return type of given location.", "", "", "2.90", "", Category) \
  XX(GameScalar, "importance", LocationGetImportance, GameLocation, "location", "Return importance of given location.", "", "", "2.90", "", Category) \
  XX(GameString, "name", LocationGetName, GameLocation, "location", "Return name of global vatiable containing given location.", "", "", "2.90", "", Category) \
  XX(GameString, "className", LocationGetClassName, GameLocation, "location", "Return short name of location. (used for scripting)", "", "", "80971 TOH", "", Category) \
  XX(GameString, "text", LocationGetText, GameLocation, "location", "Return text attached to given location.", "", "", "2.90", "", Category) \
  XX(GameBool, "rectangular", LocationGetRectangular, GameLocation, "location", "Check if given location has rectangular shape.", "", "", "2.90", "", Category) \
  XX(GameObject, "attachedObject", LocationGetAttachedObject, GameLocation, "location", "Return object attached to given location.", "", "", "2.90", "", Category) \
  XX(GameSide, "side", LocationGetSide, GameLocation, "location", "Return target side of given location.", "", "", "2.90", "", Category) \
  XX(GameBool, "isNull", LocationIsNull, GameLocation, "location", "Check whether the value is null.", "", "", "5501", "", Category) \
  XX(GameLocation, "nearestLocationWithDubbing", LocationFindNearestWithDubbing, GameArray, "position", "Find the nearest location (to the given position) having it speech non-empty.", "", "", "5501", "", Category) \

FUNCTIONS_LOCATION(JNI_FUNCTION, "Location")

static const GameFunction ExtUnary[]=
{
  FUNCTIONS_LOCATION(REGISTER_FUNCTION, "Location")
};

#if DOCUMENT_COMREF
static ComRefFunc ExtComRefFunc1[] =
{
  FUNCTIONS_LOCATION(COMREF_FUNCTION, "Location")
};
#endif

// Operators registration
#define OPERATORS_LOCATION(XX, Category) \
  XX(GameNothing, "setPosition", function, LocationSetPosition, GameLocation, GameArray, "location", "position", "Set a new position of given location.", "", "", "2.90", "", Category) \
  XX(GameNothing, "setDirection", function, LocationSetDirection, GameLocation, GameScalar, "location", "direction", "Set a new direction (angle) of given location.", "", "", "2.90", "", Category) \
  XX(GameNothing, "setSize", function, LocationSetSize, GameLocation, GameArray, "location", "[sizeX, sizeZ]", "Set a new size (width, height) of given location.", "", "", "2.90", "", Category) \
  XX(GameNothing, "setType", function, LocationSetType, GameLocation, GameString, "location", "type", "Set a new type of given location.", "", "", "2.90", "", Category) \
  XX(GameNothing, "setImportance", function, LocationSetImportance, GameLocation, GameScalar, "location", "importance", "Set a new importance of given location.", "", "", "2.90", "", Category) \
  XX(GameNothing, "setName", function, LocationSetName, GameLocation, GameString, "location", "name", "Set which global variable will contain given location.", "", "", "2.90", "", Category) \
  XX(GameNothing, "setText", function, LocationSetText, GameLocation, GameString, "location", "text", "Set a new text attached to given location.", "", "", "2.90", "", Category) \
  XX(GameNothing, "setRectangular", function, LocationSetRectangular, GameLocation, GameBool, "location", "rectangular", "Set if given location has rectangular shape.", "", "", "2.90", "", Category) \
  XX(GameNothing, "attachObject", function, LocationSetAttachedObject, GameLocation, GameObject, "location", "object", "Attach a object to given location.", "", "", "2.90", "", Category) \
  XX(GameNothing, "setSide", function, LocationSetSide, GameLocation, GameSide, "location", "side", "Set a new target side of given location.", "", "", "2.90", "", Category) \
  XX(GameNothing, "setVariable", function, LocationSetVariable, GameLocation, GameArray, "location", "[name, value]", "Set variable to given value in the variable space of given location.", "", "", "2.92", "", Category) \
  XX(GameVoid, "getVariable", function, LocationGetVariable, GameLocation, GameString, "location", "name", "Return the value of variable in the variable space of given location.", "", "", "2.92", "", Category) \
  XX(GameBool, "in", function, LocationIsInside, GameArray, GameLocation, "position", "location", "Check if the position is inside area defined by the location.", "", "", "2.92", "", Category) \
  XX(GameScalar, "distance", function, LocationDistance, GameLocation, GameLocation, "location1", "location2", "Computes the distance between two locations or positions.", "", "", "5501", "", Category) \
  XX(GameScalar, "distance", function, LocationDistance2, GameLocation, GameArray, "location1", "pos2", "Computes the distance between two locations or positions.", "", "", "5501", "", Category) \
  XX(GameScalar, "distance", function, LocationDistance3, GameArray, GameLocation, "pos1", "location2", "Computes the distance between two locations or positions.", "", "", "5501", "", Category) \
  XX(GameBool, "==", function, LocationCmpE, GameLocation, GameLocation, "location1", "location2", "Checks whether two locations are the same. If either of them is null, false is returned.", "", "", "", "5501", Category) \
  XX(GameBool, "!=", function, LocationCmpNE, GameLocation, GameLocation, "location1", "location2", "Checks whether two locations are different. If either of them is null, true is returned.", "", "", "", "5501", Category) \
  
#define OPERATORS_LOCATION_PC(XX, Category) \
  XX(GameNothing, "drawLocation", function, LocationDraw, GameControl, GameLocation, "map", "location", "Draw location in the map.", "", "", "2.90", "", Category) \

OPERATORS_LOCATION(JNI_OPERATOR, "Location")
OPERATORS_LOCATION_PC(JNI_OPERATOR, "Location PC")

static const GameOperator ExtBinary[]=
{
  OPERATORS_LOCATION(REGISTER_OPERATOR, "Location")
  OPERATORS_LOCATION_PC(REGISTER_OPERATOR, "Location PC")
};

#if DOCUMENT_COMREF
static ComRefFunc ExtComRefFunc2[] =
{
  OPERATORS_LOCATION(COMREF_OPERATOR, "Location")
  OPERATORS_LOCATION_PC(COMREF_OPERATOR, "Location PC")
};
#endif

#include <El/Modules/modules.hpp>

INIT_MODULE(Location, 2)
{
  GameState &state = GGameState; // helper to make macro works
  TYPES_LOCATION(REGISTER_TYPE, "Location")

  for (int i=0; i<lenof(ExtNular); i++)
  {
    GGameState.NewNularOp(ExtNular[i]);
  }
  //for (int i=0; i<lenof(ExtUnary); i++)
  //{
  //  GGameState.NewFunction(ExtUnary[i]);
  //}
  for (int i=0; i<lenof(ExtUnary); i++)
  {
    GGameState.NewFunction(ExtUnary[i]);
  }
  for (int i=0; i<lenof(ExtBinary); i++)
  {
    GGameState.NewOperator(ExtBinary[i]);
  }

#if DOCUMENT_COMREF
  for (int i=0; i<lenof(ExtComRefType); i++)
  {
    GGameState.AddComRefType(ExtComRefType[i]);
  }
  for (int i=0; i<lenof(ExtComRefFunc0); i++)
  {
    GGameState.AddComRefFunction(ExtComRefFunc0[i]);
  }
  for (int i=0; i<lenof(ExtComRefFunc1); i++)
  {
    GGameState.AddComRefFunction(ExtComRefFunc1[i]);
  }
  for (int i=0; i<lenof(ExtComRefFunc2); i++)
  {
    GGameState.AddComRefFunction(ExtComRefFunc2[i]);
  }
#endif
}
