#ifdef _MSC_VER
#pragma once
#endif

#ifndef _LOCATION_HPP
#define _LOCATION_HPP

#define USE_LANDSCAPE_LOCATIONS 1

#include <Es/Types/lLinks.hpp>
#include <Es/Strings/rString.hpp>
#include <Es/Containers/bankArray.hpp>
#include <Es/Containers/quadtreeCont.hpp>

#include <El/Evaluator/expressImpl.hpp>

#include "object.hpp"
#include "objLink.hpp"

class CStaticMap;

#define LOCATION_DRAW_STYLE_ENUM(type, prefix, XX) \
  XX(type, prefix, Icon) \
  XX(type, prefix, Area) \
  XX(type, prefix, Mount) \
  XX(type, prefix, Name) \

#ifndef DECL_ENUM_LOCATION_DRAW_STYLE
#define DECL_ENUM_LOCATION_DRAW_STYLE
DECL_ENUM(LocationDrawStyle)
#endif
DECLARE_ENUM(LocationDrawStyle, LDS, LOCATION_DRAW_STYLE_ENUM)

/// type of location - defines appearance of the location
struct LocationType : public RemoveLLinks
{
  /// type name
  RString _name;

  /// how to draw
  LocationDrawStyle _drawStyle;
  /// picture shape
  Ref<Texture> _texture;
  /// picture color
  PackedColor _color;
  /// picture size
  float _size;
  /// text font
  Ref<Font> _font;
  /// text size
  float _textSize;
  /// base importance
  float _importance;
  /// draw shadow
  int _shadow;

  LocationType(RString name);
  RString GetName() const {return _name;}
};

template <>
struct BankTraits<LocationType>: DefLLinkBankTraits<LocationType>
{
};

typedef BankArray<LocationType> LocationTypeBank;

extern LocationTypeBank LocationTypes;

#if _ENABLE_CONVERSATION
/// support for more variants of location name dubbing (different languages, grammar categories etc.)
struct LocationSpeechVariant
{
  /// name of the variant
  RString _variant;
  /// list of words
  AutoArray<RString> _speech;
};
TypeIsMovable(LocationSpeechVariant)

template <>
struct FindArrayKeyTraits<LocationSpeechVariant>
{
  typedef const RString &KeyType;
  static bool IsEqual(const RString &a, const RString &b)
  {
    return stricmp(a, b) == 0;
  }
  static const RString &GetKey(const LocationSpeechVariant &a) {return a._variant;}
};

/// set of speech variants
typedef FindArrayKey<LocationSpeechVariant> LocationSpeechVariants;
#endif

struct Location : public RemoveLLinks
{
  // type
  Ref<LocationType> _type;

  // placement and shape
  Vector3 _position;
  OLinkObject _attached;
  float _a;
  float _b;
  float _angle;
  bool _rectangular;

  // additional info
  float _importance;
  float _nearestMoreImportant;
  /// text of the location (can be localized)
  RString _text;
  /// name of location used for scripting
  RString _className;
  /// list of radio protocol words used as a dubbing
  AutoArray<RString> _speech;
#if _ENABLE_CONVERSATION
  LocationSpeechVariants _speechVariants;

  const LocationSpeechVariant *GetSpeechVariant(const RString &variant) const
  {
    int index = _speechVariants.FindKey(variant);
    if (index < 0) return NULL;
    return &_speechVariants[index];
  }
#endif

  TargetSide _side;

  // global variable containing location
  RString _varName;

  // identifier, used for serialization of reference
  // negative - in dynamic list
  // positive - index in leaf of quad tree
  int _id;

  Location();
  Location(RString typeName, Vector3Par position, float a, float b);
  Location(LocationType *type, Vector3Par position, float a, float b);
  Location(LocationType *type, Vector3Par position, float a, float b, float angle);

  Vector3Val GetPosition() const;
  int GetX() const {return toLargeInt(_position.X());}
  int GetY() const {return toLargeInt(_position.Z());}

  bool IsInside(Vector3Par pos) const;

  bool IsStatic() const {return _id >= 0;}

  virtual GameVarSpace *GetVars() {return NULL;}
  virtual const GameVarSpace * GetVars() const {return NULL;}

  // editor=false
  void Draw(CStaticMap *map, bool editor, float elevationOffset) const;

  virtual LSError Serialize(ParamArchive &ar);
  static Location *CreateObject(ParamArchive &ar) {return new Location();}
  static Location *LoadRef(ParamArchive &ar);
  virtual LSError SaveRef(ParamArchive &ar);

  static LSError LoadLLinkRef(ParamArchive &ar, LLink<Location> &link)
  {
    link = Location::LoadRef(ar);
    return LSOK;
  }
  static LSError SaveLLinkRef(ParamArchive &ar, LLink<Location> &link)
  {
    return link->SaveRef(ar);
  }
};

struct DynamicLocation : public Location
{
  GameVarSpace _vars;

  DynamicLocation() : _vars(true) {}
  DynamicLocation(RString typeName, Vector3Par position, float a, float b)
    : Location(typeName, position, a, b), _vars(true) {}
  DynamicLocation(LocationType *type, Vector3Par position, float a, float b)
    : Location(type, position, a, b), _vars(true) {}

  virtual GameVarSpace *GetVars() override {return &_vars;}
  virtual const GameVarSpace * GetVars() const override {return &_vars;}

  virtual LSError Serialize(ParamArchive &ar);
  static DynamicLocation *CreateObject(ParamArchive &ar) {return new DynamicLocation();}
};

struct LocationsItem : public RefCount
{
  int _x;
  int _y;
  RefArray<Location> _locations;

  LocationsItem(int x, int y) {_x = x; _y = y;}
};

struct LocationsItemRef : public Ref<LocationsItem>
{
  typedef Ref<LocationsItem> base;

  int GetX() const {return GetRef()->_x;}
  int GetY() const {return GetRef()->_y;}

  LocationsItemRef(LocationsItem *item) : base(item) {}
};

class Locations : public QuadTreeCont<LocationsItemRef>
{
  typedef QuadTreeCont<LocationsItemRef> base;

public:
  void AddLocation(Location *location);
  void RemoveLocation(Location *location);

  Location *Get(float x, float y, int id);
  Location *FindNearest(Vector3Par position, LocationType *type, float limit2);
  /// find the nearest location with "speech" given
  Location *FindNearestWithDubbing(Vector3Par position, float limit2);
  RefArray<Location> FindAllInArea(Vector3Par position, const RefArray<LocationType> &types, float limit2);
};

// Global functions
Location *FindNearestLocationWithDubbing(Vector3Par position, float limit2);

#endif

