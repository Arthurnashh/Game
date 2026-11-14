#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MAP_PROPERTIES_HPP
#define _MAP_PROPERTIES_HPP

#include <El/ParamFile/paramFileDecl.hpp>
#include "../Shape/mapTypes.hpp"

/// Legend properties
struct LegendInfo
{
  /// size and position
  float _x;
  float _y;
  float _w;
  float _h;

  /// text font
  Ref<Font> _font;
  /// text size
  float _size;

  /// background color
  PackedColor _colorBackground;
  /// text color
  PackedColor _color;

  void Load(ParamEntryPar cls);
};

//! Describes map object type
struct MapTypeInfo
{
  //! picture
  Ref<Texture> icon;
  //! color
  PackedColor color;
  //! relative size
  float size;
  //! minimal size
  float coefMin;
  //! maximal size
  float coefMax;
  //! when to draw
  float importance;
  //! when to draw
  int shadow;

  //! reads description from resource
  void Load(ParamEntryPar cls);
};

struct MapProperties
{
  //! minimal map scale
  float _scaleMin;
  //! maximal map scale
  float _scaleMax;
  //! default (initial) map scale
  float _scaleDefault;

  // shown area
  float _xMin, _xMax, _zMin, _zMax;

  // visuals

  //@{ coefficients which determine rendering density / threshold
  float _ptsPerSquareSea;   // seas
  float _ptsPerSquareTxt;   // textures
  float _ptsPerSquareCLn;   // count-lines
  //@}

  //@{ coefficients which determine when rendering of given type is done
  float _ptsPerSquareFor;   // forests
  float _ptsPerSquareForEdge;   // forest edges
  float _ptsPerSquareRoad;  // roads
  float _ptsPerSquareObj;    // other objects
  //@}

  //! color of height levels
  PackedColor _colorLevels;
  //@{
  //! color of sea
  PackedColor _colorSeaPacked;
  Color _colorSea;
  //@}
  //! color of forest
  PackedColor _colorForest;
  //! color of rocky area
  PackedColor _colorRocks;

  //! color of countlines
  PackedColor _colorCountlines;
  //! color of countlines in sea
  PackedColor _colorCountlinesWater;
  //! color of thicker countlines
  PackedColor _colorMainCountlines;
  //! color of thicker countlines in sea
  PackedColor _colorMainCountlinesWater;
  //! color of borders of forest
  PackedColor _colorForestBorder;
  //! color of borders of rocky area
  PackedColor _colorRocksBorder;
  /// color of power lines
  PackedColor _colorPowerLines;
  /// color of railway
  PackedColor _colorRailWay;
  //! color of geographic names (towns, villages, mounts etc.)
  PackedColor _colorNames;

  /// color outside the map area
  PackedColor _colorOutside;

  //! color of tracks
  PackedColor _colorTracks;
  //! color of roads
  PackedColor _colorRoads;
  //! color of main roads
  PackedColor _colorMainRoads;
  //! color of tracks
  PackedColor _colorTracksFill;
  //! color of roads
  PackedColor _colorRoadsFill;
  //! color of main roads
  PackedColor _colorMainRoadsFill;

  //! color of grid texts
  PackedColor _colorGrid;
  //! color of grid lines
  PackedColor _colorGridMap;

  //! font used for geographic names
  Ref<Font> _fontNames;
  //! font size
  float _sizeNames;
  //! font used for natural ground levels
  Ref<Font> _fontLevels;
  //! font size
  float _sizeLevels;
  //! font used for grid texts
  Ref<Font> _fontGrid;
  //! font size
  float _sizeGrid;

  /// legend properties
  LegendInfo _legend;
  //! countour interval 
  bool _showCountourInterval;

  //@{
  //! map object type properties
  MAP_TYPES_ICONS_1(MAP_TYPE_INFO_DECL)
  MAP_TYPES_ICONS_2(MAP_TYPE_INFO_DECL)
  //@}

  /// background texture
  Ref<Texture> _texture;

  MapProperties();
  void Load(ParamEntryPar cls);

  bool DrawTerrainOutside() const;
};

#endif // _MAP_PROPERTIES_HPP
