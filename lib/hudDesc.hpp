#ifdef _MSC_VER
#pragma once
#endif

#ifndef _HUD_DESC_HPP
#define _HUD_DESC_HPP

/**
@file Vehicle HUD
HUD is organized in two layers - bones and elements.
Bones determine location on the HUD.
Elements are "skinned" to bones.
*/

#include <El/ParamFile/paramFileDecl.hpp>
#include "engine.hpp"
#include "UI/uiViewport.hpp"


class EntityAIFull;

/// 3x2 matrix for Point2DFloat tranformation
class Transform2DFloat
{
  public:  
  Point2DFloat _right;
  Point2DFloat _down;
  Point2DFloat _pos;

  Point2DFloat operator *(const Point2DFloat &pos) const;
};



/// attributes used during rendering
struct HUDDrawContext
{
  bool enabled;
  /// list of conditions which can be on
  const char **conditions;
  /// bitmask telling for each item in conditions if it is on or off
  int conditionMask;
  PackedColor color;
  EntityAIFull *vehicle;
  Ref<Font> font;

  /// original viewport
  UIViewport *_viewport;
  /// child viewport with clipping
  UIViewport *_viewportClipped;
  /// coeficient for y axis values recalculation (size of viewport in y axis)
  float _yCoef;
  float _offsetX, _offsetY;

  HUDDrawContext();
  ~HUDDrawContext();
  
  bool CheckCondition(const char *cnd) const;
};


/// HUD bone interface
class HUDBone: public RefCount
{
  RStringB _name;
  
  public:
  HUDBone(RStringB name):_name(name){}
  const RStringB &GetName() const {return _name;}
  
  /// load bone from the config
  virtual void Load(ParamEntryPar entry) = 0;
  /// get bone position
  virtual Point2DFloat Transform(const HUDDrawContext &ctx, const Point2DFloat &pos) const = 0;

  static void LoadTrans(Transform2DFloat &pos, ParamEntryPar entry);
  static void LoadPos(Point2DFloat &pos, ParamEntryPar entry);
  
};

/// list of bones
class HUDBoneList: public RefArray<HUDBone>
{
  public:
  HUDBone *FindBone(RStringB bone) const;
  void Load(ParamEntryPar entry);
};

/// HUD element interface
class HUDElement: public RefCount
{
  public:
  virtual void Load(const HUDBoneList &bones, ParamEntryPar entry) = 0;
  virtual void Draw(const HUDDrawContext &ctx) const = 0;
};

/// list of elements
class HUDElementList: public RefArray<HUDElement>
{
  public:
  void Draw(const HUDDrawContext &ctx) const;
  void Load(const HUDBoneList &bones, ParamEntryPar entry);
};

/// fixed bone

class HUDBoneFixed: public HUDBone
{
  typedef HUDBone base;
  
  Transform2DFloat _pos;
  
  public:
  //@{ HUDBone implementation
  explicit HUDBoneFixed(RStringB name)
  :base(name){}
  virtual void Load(ParamEntryPar entry);
  virtual Point2DFloat Transform(const HUDDrawContext &ctx, const Point2DFloat &pos) const
  {
    return _pos*pos;
  }
  //@}
};


/// binding element to single bone
struct HUDBindSingleBone
{
  float _weight;
  Ref<HUDBone> _bone;
  Point2DFloat _offset;
};

TypeIsMovable(HUDBindSingleBone)

/// element binded (skinned) to multiple bones
class HUDBindBones: public AutoArray<HUDBindSingleBone>
{
  public:
  /// evaluate position in given context
  Point2DFloat Position(const HUDDrawContext &ctx) const;
  /// load binding from a config (array)
  void Load(const HUDBoneList &bones, const IParamArrayValue &entry);
  /// load binding from a config (entry)
  void Load(const HUDBoneList &bones, ParamEntryPar entry);
};

/// helper for attribute stack
class HUDAttribute: public RefCount
{
  public:
  virtual void Apply(HUDDrawContext &ctx) = 0;
};

/// hierarchy encapsulation
class HUDElementGroup: public HUDElement
{
  HUDElementList _items;
  /// list of overriddden attributes
  RefArray<HUDAttribute> _override;

  public:  
  //@{ HUDElement implementation
  virtual void Load(const HUDBoneList &bones,ParamEntryPar entry);
  virtual void Draw(const HUDDrawContext &ctx) const;
  //@}
  
};

class EntityAIFull;
class ObjectVisualState;

/// description of HUD or MFD display
struct MFDDesc
{
  //@{ names of HUD positions in the memory lod
	RStringB _posName, _rightName, _downName;
	//@}
  //@{ indices of HUD positions in the memory lod
	int _position, _right, _down;
	//@}
  //@{ adjust position inside of memory points lod
  float _borderLeft,_borderRight;
  float _borderTop,_borderBottom;
  //@}
	HUDBoneList _bones;
	HUDElementGroup _elements;
  Ref<Font> _font;
  PackedColor _color;
  
  bool _helmetMountedDisplay;
  Vector3 _helmetPosition, _helmetRight, _helmetDown;

  bool _enableParallax;

  /// load from config
  void Load(ParamEntryPar entry);
  /// bind to shape
  void InitShape(LODShape *shape);
  /// draw while drawing a container
  void Draw(int cb, Transport *container, const PositionRender &pos) const;
};

TypeIsMovable(MFDDesc)

#endif

