#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MAP_TYPES_HPP
#define _MAP_TYPES_HPP

// map types factory
// enum value, "map" property in the model

// all types - the order must be kept (is stored as integer in binarized map)
#define MAP_TYPES_ALL(XX) \
  XX(Tree, "TREE") \
  XX(SmallTree, "SMALL TREE") \
  XX(Bush, "BUSH") \
  XX(Building, "BUILDING") \
  XX(House, "HOUSE") \
  XX(ForestBorder, "FOREST BORDER") \
  XX(ForestTriangle, "FOREST TRIANGLE") \
  XX(ForestSquare, "FOREST SQUARE") \
  XX(Church, "CHURCH") \
  XX(Chapel, "CHAPEL") \
  XX(Cross, "CROSS") \
  XX(Rock, "ROCK") \
  XX(Bunker, "BUNKER") \
  XX(Fortress, "FORTRESS") \
  XX(Fountain, "FOUNTAIN") \
  XX(ViewTower, "VIEW-TOWER") \
  XX(Lighthouse, "LIGHTHOUSE") \
  XX(Quay, "QUAY") \
  XX(Fuelstation, "FUELSTATION") \
  XX(Hospital, "HOSPITAL") \
  XX(Fence, "FENCE") \
  XX(Wall, "WALL") \
  XX(Hide, "HIDE") \
  XX(BusStop, "BUSSTOP") \
  XX(Road, "ROAD") \
  XX(Forest, "FOREST") \
  XX(Transmitter, "TRANSMITTER") \
  XX(Stack, "STACK") \
  XX(Ruin, "RUIN") \
  XX(Tourism, "TOURISM") \
  XX(Watertower, "WATERTOWER") \
  XX(Track, "TRACK") \
  XX(MainRoad, "MAIN ROAD") \
  XX(Rocks, "ROCKS") \
  XX(PowerLines, "POWER LINES") \
  XX(RailWay, "RAILWAY") \

// types drawn as an icon only
#define MAP_TYPES_ICONS_1(XX) \
  XX(Tree, "TREE") \
  XX(SmallTree, "SMALL TREE") \
  XX(Bush, "BUSH") \
  XX(Cross, "CROSS") \
  XX(Rock, "ROCK") \
  XX(Bunker, "BUNKER") \
  XX(Fortress, "FORTRESS") \
  XX(Fountain, "FOUNTAIN") \
  XX(ViewTower, "VIEW-TOWER") \
  XX(Lighthouse, "LIGHTHOUSE") \
  XX(Quay, "QUAY") \
  XX(BusStop, "BUSSTOP") \
  XX(Transmitter, "TRANSMITTER") \
  XX(Stack, "STACK") \
  XX(Watertower, "WATERTOWER") \

#define MAP_TYPES_ICONS_2(XX) \
  XX(Church, "CHURCH") \
  XX(Chapel, "CHAPEL") \
  XX(Fuelstation, "FUELSTATION") \
  XX(Hospital, "HOSPITAL") \
  XX(Ruin, "RUIN") \
  XX(Tourism, "TOURISM") \

// factory usage

#define MAP_TYPE_ENUM(value, name) \
  Map##value,

#define MAP_TYPE_READ(value, name) \
  else if (stricmp(mapType, name) == 0) _mapType = Map##value;

#define MAP_TYPE_INFO_DECL(value, name) \
  MapTypeInfo _info##value;

#define MAP_TYPE_CASE(value, name) \
  case Map##value:

#define MAP_TYPE_DRAW_ICON(value, name) \
  case Map##value: \
    if (props._info##value.importance >= _scaleX) \
    { \
      DrawSign(props._info##value, obj->GetPosition(defaultPos)); \
    } \
    break;

#define MAP_TYPE_DRAW_BUILDING_AND_ICON(value, name) \
  case Map##value: \
    DrawBuilding(obj, obj->GetColor()); \
    if (props._info##value.importance >= _scaleX) \
    { \
      DrawSign(props._info##value, obj->GetPosition(defaultPos)); \
    } \
    break;

#define MAP_TYPE_INFO_LOAD(value, name) \
  _info##value.Load(cls >> #value);

#define MAP_TYPE_INFO_DECL_STATIC(value, name) \
  static MapTypeInfo info##value;

#define MAP_TYPE_MASK_DECL_STATIC(value, name) \
  static HBITMAP mask##value = NULL;

#define MAP_TYPE_BITMAP_DECL_STATIC(value, name) \
  static HBITMAP bmp##value = NULL;

#define MAP_TYPE_BITMAP_DELETE(value, name) \
  static HBITMAP bmp##value = NULL;

#define MAP_TYPE_EXPORT_LOAD(value, name) \
  info##value.Load(cls >> #value);

#define MAP_TYPE_EXPORT_UNLOAD(value, name) \
  info##value.icon = NULL;

#define MAP_TYPE_EXPORT_LOAD_AND_CREATE(value, name) \
  info##value.Load(cls >> #value); \
  CreateBitmap(hDC, info##value, bmp##value, mask##value);

#define MAP_TYPE_EXPORT_DRAW_SIGN(value, name) \
  case Map##value: \
    DrawSign(hDC, bmp##value, mask##value, obj->GetPosition(VZero), toInt(info##value.size)); \
    break;

#define MAP_TYPE_EXPORT_DRAW_SIGN_AND_BUILDING(value, name) \
  case Map##value: \
    DrawBuilding(hDC, obj); \
    DrawSign(hDC, bmp##value, mask##value, obj->GetPosition(VZero), toInt(info##value.size)); \
    break;

// MapType enum

#ifndef DECL_ENUM_MAP_TYPE
#define DECL_ENUM_MAP_TYPE
DECL_ENUM(MapType)
#endif
DEFINE_ENUM_BEG(MapType)
  MAP_TYPES_ALL(MAP_TYPE_ENUM)
	NMapTypes
DEFINE_ENUM_END(MapType)

#endif
