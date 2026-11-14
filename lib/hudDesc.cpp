#include "wpch.hpp"
#include "hudDesc.hpp"

#include <El/ParamFile/paramFile.hpp>
#include "paramFileExt.hpp"
#include "engine.hpp"
#include "Shape/specLods.hpp"
#include "mbcs.hpp"
#include "vehicleAI.hpp"
#include "AI/ai.hpp"
#include "landscape.hpp"
#include "world.hpp"
#include "global.hpp"

void HUDBone::LoadTrans(Transform2DFloat &pos, ParamEntryPar entry)
{
  float x = entry[0];
  float y = entry[1];
  pos._pos = Point2DFloat(x,y);
  pos._right = Point2DFloat(1,0);
  pos._down = Point2DFloat(0,1);
}
void HUDBone::LoadPos(Point2DFloat &pos, ParamEntryPar entry)
{
  float x = entry[0];
  float y = entry[1];
  pos = Point2DFloat(x,y);
}

Point2DFloat Transform2DFloat::operator *(const Point2DFloat &pos) const
{
  return Point2DFloat(
    _right.x*pos.x + _down.x*pos.y + _pos.x,
    _right.y*pos.x + _down.y*pos.y + _pos.y
  );
}

/*!
\patch 5151 Date 3/30/2007 by Mark
- Added: Heading source to HUD's
*/

/// scalar value input selector
enum HUDValueSelect
{
  HUDValueAltAGL, // above ground level altitude
  HUDValueAltASL, // above sea level altitude
  HUDValueSpeed,
  HUDValueRPM,
  HUDValueVertSpeed,
  HUDValueFuel,
  HUDValueHeading, //return in degrees
  HUDValueHorizonBank,
  HUDValueHorizonDive,
  HUDValueWeapon, 
  HUDValueAmmo,
  NHUDValueSelect
};

static const EnumName ValueEnumName[]=
{
  EnumName(HUDValueAltAGL,"altitudeAGL"),
  EnumName(HUDValueAltASL,"altitudeASL"),
  EnumName(HUDValueSpeed,"speed"),
  EnumName(HUDValueRPM,"rpm"),
  EnumName(HUDValueVertSpeed,"vspeed"),
  EnumName(HUDValueFuel,"fuel"),
  EnumName(HUDValueHeading,"heading"),
  EnumName(HUDValueHorizonBank,"horizonBank"),
  EnumName(HUDValueHorizonDive,"horizonDive"), 
  EnumName(HUDValueWeapon,"weapon"),
  EnumName(HUDValueAmmo,"ammo"),
  EnumName()
};

template <>
const EnumName *GetEnumNames(HUDValueSelect dummy) {return ValueEnumName;}


/// read given value from the source
static float HUDGetSource(HUDValueSelect source, const HUDDrawContext &ctx)
{
  switch (source)
  {
    case HUDValueAltAGL:
    {
      Vector3 pos = ctx.vehicle->RenderVisualState().Position();
      return pos.Y()-GLandscape->RoadSurfaceYAboveWater(pos);
    } 
    case HUDValueAltASL:
      return ctx.vehicle->RenderVisualState().Position().Y();
    case HUDValueSpeed:
      return ctx.vehicle->RenderVisualState().ModelSpeed().Z();
    case HUDValueRPM:
      return ctx.vehicle->GetRenderVisualStateRPM();
    case HUDValueVertSpeed:
      return ctx.vehicle->RenderVisualState().Speed().Y();
    case HUDValueFuel:
      return ctx.vehicle->GetRenderVisualStateFuel() / ctx.vehicle->GetType()->GetFuelCapacity();
    case HUDValueHeading:
    {
      Vector3Val dir = ctx.vehicle->RenderVisualState().Direction();
      float heading = atan2(dir.X(), dir.Z()) * (180.0f / H_PI);
      if (heading < 0) heading += 360.0f;
      return heading;
    }
    case HUDValueHorizonDive:
    {
      Transport *vehicle = dyn_cast<Transport>(ctx.vehicle);
      if (!vehicle) return 0;
      return vehicle->RenderVisualState().GetHorizonDive();
  }
    case HUDValueHorizonBank:
    {
      Transport *vehicle = dyn_cast<Transport>(ctx.vehicle);
      if (!vehicle) return 0;
      return vehicle->RenderVisualState().GetHorizonBank();
    }
    case HUDValueWeapon:
    case HUDValueAmmo: return 0;
  }
  return 0;
}

/// vector source selection
enum HUDVectorSelect
{
  HUDVectorVelocity,
  HUDVectorWeapon,
  NHUDVectorSelect
};

static const EnumName VectorEnumName[]=
{
  EnumName(HUDVectorVelocity,"velocity"),
  EnumName(HUDVectorWeapon,"weapon"),
  EnumName()
};

template <>
const EnumName *GetEnumNames(HUDVectorSelect dummy) {return VectorEnumName;}



/// read given value from the source
static Vector3 HUDGetVector(HUDVectorSelect source, const HUDDrawContext &ctx)
{
  switch (source)
  {
    case HUDVectorVelocity:
    {
      Vector3Val speed = ctx.vehicle->RenderVisualState().ModelSpeed();
      if (speed.SquareSize()<1) return VZero;
      return speed;
    }
    case HUDVectorWeapon:
    {
      Vector3 weaponDir;
      TurretContext context; 
      if (ctx.vehicle->GetPrimaryGunnerTurret(context))
      {
        weaponDir = ctx.vehicle->GetWeaponDirection(ctx.vehicle->RenderVisualState(), context, context._weapons->ValidatedCurrentWeapon());
      }
      else
      {
        weaponDir = ctx.vehicle->RenderVisualState().Direction();
      }
      return ctx.vehicle->RenderVisualState().DirectionWorldToModel(weaponDir);
    }
  }
  return VZero;
}


void HUDBoneFixed::Load(ParamEntryPar entry)
{
  LoadTrans(_pos, entry>>"pos");
}

/// bone with source value attached
class HUDBoneSource: public HUDBone
{
  typedef HUDBone base;

  HUDValueSelect _source;
  float _min,_max;

  protected:
  float GetValue(const HUDDrawContext &ctx) const;
  
  public:
  //@{ HUDBone implementation
  explicit HUDBoneSource(RStringB name):base(name){}
  virtual void Load(ParamEntryPar entry);
  //@}
};

/// linear indicator
class HUDBoneLinear: public HUDBoneSource
{
  typedef HUDBoneSource base;
  Point2DFloat _minPos,_maxPos;
  
  public:
  //@{ HUDBoneSource implementation
  explicit HUDBoneLinear(RStringB name):base(name){}
  virtual void Load(ParamEntryPar entry);
  virtual Point2DFloat Transform(const HUDDrawContext &ctx, const Point2DFloat &pos) const;
  //@}
};

/// rotational indicator
class HUDBoneRotational: public HUDBoneSource
{
  typedef HUDBoneSource base;
  Point2DFloat _center;
  float _minAngle,_maxAngle,_aspectRatio;
  
  public:
  //@{ HUDBoneSource implementation
  explicit HUDBoneRotational(RStringB name):base(name),_aspectRatio(1){}
  virtual void Load(ParamEntryPar entry);
  virtual Point2DFloat Transform(const HUDDrawContext &ctx, const Point2DFloat &pos) const;
  //@}
};

/// special bone for artificial horizon construction
class HUDBoneHorizon: public HUDBone
{
  typedef HUDBone base;
  float _angle;
  /// position depending on dive and bank
  Point2DFloat _pos0,_pos10;
  
  public:
  //@{ HUDBone implementation
  explicit HUDBoneHorizon(RStringB name):base(name){}
  virtual void Load(ParamEntryPar entry);
  virtual Point2DFloat Transform(const HUDDrawContext &ctx, const Point2DFloat &pos) const;
  //@}
};


/// special bone for velocity vector
class HUDBoneVector: public HUDBone
{
  typedef HUDBone base;
  /// position depending on dive and bank
  Point2DFloat _pos0,_pos10;
  HUDVectorSelect _source;
  
  public:
  //@{ HUDBone implementation
  explicit HUDBoneVector(RStringB name):base(name){}
  virtual void Load(ParamEntryPar entry);
  virtual Point2DFloat Transform(const HUDDrawContext &ctx, const Point2DFloat &pos) const;
  //@}
};


/// special bone for ILS navigation system
class HUDBoneILS: public HUDBone
{
  typedef HUDBone base;
  
  Point2DFloat _pos0;
  Point2DFloat _pos3;

  public:
  //@{ HUDBone implementation
  explicit HUDBoneILS(RStringB name):base(name){}
  virtual void Load(ParamEntryPar entry);
  virtual Point2DFloat Transform(const HUDDrawContext &ctx, const Point2DFloat &pos) const;
  //@}
};

void HUDBoneSource::Load(ParamEntryPar entry)
{
  //base::Load(entry);
  RStringB source = entry>>"source";
  _source = GetEnumValue<HUDValueSelect>(source);
  if (_source<0 || _source>=NHUDValueSelect)
  {
    RptF("Value %s not recognized as a source",cc_cast(source));
  }
  _min = entry>>"min";
  _max = entry>>"max";
}

float HUDBoneSource::GetValue(const HUDDrawContext &ctx) const
{
  float value = HUDGetSource(_source,ctx);
  if (value<_min) return 0;
  if (value>_max) return 1;
  return (value-_min)/(_max-_min);
}

void HUDBoneLinear::Load(ParamEntryPar entry)
{
  base::Load(entry);
  LoadPos(_minPos, entry>>"minPos");
  LoadPos(_maxPos, entry>>"maxPos");
}

Point2DFloat HUDBoneLinear::Transform(const HUDDrawContext &ctx, const Point2DFloat &pos) const
{
  float value = GetValue(ctx);
  return _minPos*value + _maxPos*(1-value) + pos;
}

void HUDBoneRotational::Load(ParamEntryPar entry)
{
  base::Load(entry);

  LoadPos(_center, entry>>"center");
  _minAngle = HDegree(entry>>"minAngle");
  _maxAngle = HDegree(entry>>"maxAngle");
  if (entry.FindEntry("aspectRatio")) _aspectRatio = (float)(entry>>"aspectRatio");
}

Point2DFloat HUDBoneRotational::Transform(const HUDDrawContext &ctx, const Point2DFloat &pos) const
{
  float value = GetValue(ctx);
  // create rotated coord. space
  
  float angle = _minAngle*(1-value)+_maxAngle*value;
  Point2DFloat up(sin(angle),-cos(angle)*_aspectRatio);
  Point2DFloat right(cos(angle),sin(angle)*_aspectRatio);
  return _center+right*pos.x+up*pos.y;
}

void HUDBoneHorizon::Load(ParamEntryPar entry)
{
  _angle = entry>>"angle";
  _angle *= H_PI/180;
  LoadPos(_pos0,entry>>"pos0");
  LoadPos(_pos10,entry>>"pos10");
}
Point2DFloat HUDBoneHorizon::Transform(const HUDDrawContext &ctx, const Point2DFloat &pos) const
{
  // calculate orientation of the projected vector
  // project to screen space
  // we know what position is assigned to 0 degree direction
  // we know what position is assigned to 10 degree direction
  
  Matrix3Val orient = ctx.vehicle->RenderVisualState().Orientation();
  
  // based on the angle, place between 0 and 10 marks
  float pitch = atan2(orient.Direction().Y(),orient.Direction().SizeXZ())-_angle;

  // take a relative "horizontal" vector in the front of the plane
  // project it to model space
  // i.e. take DirectionAside()
  // project aside to the horizontal plane
  // this will give you aside vector offset
  Vector3 asideDir; //= orient.DirectionAside();
  asideDir.Init();
  asideDir[0] = +orient.Direction().Z();
  asideDir[1] = 0;
  asideDir[2] = -orient.Direction().X();

  Vector3 relAside = ctx.vehicle->RenderVisualState().DirectionWorldToModel(asideDir);
  relAside[2] = 0;
  relAside.Normalize();
  float asideX = relAside.X();
  float asideY = -relAside.Y();
  
  //float bank = atan2(orient.DirectionAside().Y(),orient.DirectionAside().SizeXZ());
//  float asideX = cos(bank);
//  float asideY = sin(bank);
  
  float factor10 = 1/(10*H_PI/180);
  // convert relative to 10 degree
  pitch *= factor10;
  //bank *= factor10;
  
  float x10 = _pos10.x-_pos0.x;
  float y10 = _pos10.y-_pos0.y;
  float normalize10 = InvSqrt(x10*x10+y10*y10);
  x10 *= normalize10;
  y10 *= normalize10;
  
  
  float down = pitch*y10*asideX;
  float aside = pitch*x10*asideY;
  //float down = (1-factor10)+_pos10*factor10;
  
  
  Point2DFloat aside10deg(_pos10.x-_pos0.x,0);
  Point2DFloat down10deg(0,_pos10.y-_pos0.y);
  
  // calculate what angle we have forward
  Transform2DFloat trans;
  trans._pos = _pos0 + down10deg*down - aside10deg*aside;
  trans._right = Point2DFloat(x10*asideX,y10*asideY);
  trans._down = Point2DFloat(-x10*asideY,y10*asideX);
  
  return trans*pos;
}

void HUDBoneVector::Load(ParamEntryPar entry)
{
  LoadPos(_pos0,entry>>"pos0");
  LoadPos(_pos10,entry>>"pos10");
  
  RStringB sourceText = entry>>"source";
  _source = GetEnumValue<HUDVectorSelect>(sourceText);
  if (_source<0 || _source>=NHUDVectorSelect)
  {
    RptF("Value %s not recognized as a source",cc_cast(sourceText));
  }
}
Point2DFloat HUDBoneVector::Transform(const HUDDrawContext &ctx, const Point2DFloat &pos) const
{
  // calculate orientation of the projected vector
  // project to screen space
  // we know what position is assigned to 0 degree direction
  // we know what position is assigned to 10 degree direction
  
  Vector3Val speed = HUDGetVector(_source,ctx);
  if (speed.Z()<=0)
  {
    /// vector is pointing back - do not render it
    return Point2DFloat(-1000,-1000);
  }
  if (speed.SquareSize()<Square(0.001))
  {
    // vector is off, do not render it
    return Point2DFloat(-1000,-1000);
  }
  // based on the angle, place between 0 and 10 marks
  float pitch = -atan2(speed.Y(),speed.SizeXZ());
  float aside = -asin(speed.X()*speed.InvSize());
  
  float factor10 = 1/(10*H_PI/180);
  // convert relative to 10 degree
  pitch *= factor10;
  aside *= factor10;
  
  Point2DFloat aside10deg(_pos10.x-_pos0.x,0);
  Point2DFloat down10deg(0,_pos10.y-_pos0.y);
  
  return _pos0 + down10deg*pitch - aside10deg*aside + pos;
}

void HUDBoneILS::Load(ParamEntryPar entry)
{
  LoadPos(_pos0,entry>>"pos0");
  LoadPos(_pos3,entry>>"pos3");
}
Point2DFloat HUDBoneILS::Transform(const HUDDrawContext &ctx, const Point2DFloat &pos) const
{
  // get ILS corridor
  Vector3 ilsPos = ctx.vehicle->RenderVisualState().Position(), ilsDir = ctx.vehicle->RenderVisualState().Direction();
  bool ilsSTOL;
  if (GWorld->GetILS(ilsPos,ilsDir,ilsSTOL,TSideUnknown,true)>=0)
  {
    const float maxDistance = 8000;
    const float max35DegDistance = 3000;
    float dist2 = ctx.vehicle->RenderVisualState().Position().Distance2(ilsPos);
    if (dist2<Square(maxDistance))
    {
      // calculate our position in the ILS cone
      Vector3 relDir = ctx.vehicle->RenderVisualState().Position()-ilsPos;
      float cosAngle = relDir.CosAngle(ilsDir);
      const float minCosAngle = 0.93969262079f; // cos 20 deg.
      const float cos35Angle = 0.819152044288f; // cos 35 deg.
      if (cosAngle>minCosAngle || cosAngle>cos35Angle && dist2<Square(max35DegDistance))
      {
        // calculate horizontal and vertical deviation
        Matrix3 ilsCoord(MDirection,ilsDir,VUp);
        
        // full deflection is set to 3 degrees. Cf: http://www.allstar.fiu.edu/aero/ils.htm
        // "ILS ... full-scale needle deflection occurs at approximately 2.5 deg"
        // direction in ILS local coordinates
        Vector3 ilsLocalDir = ilsCoord.InverseRotation() * relDir;
        const float factor = 1/(3*H_PI/180);
        float ilsHor = atan2(ilsLocalDir.X(),ilsLocalDir.Z())*factor;
        float ilsVer = atan2(ilsLocalDir.Y(),ilsLocalDir.Z())*factor;
        static float sat = 0.95f;
        saturate(ilsHor,-sat,+sat);
        saturate(ilsVer,-sat,+sat);
        
        Point2DFloat horD(_pos3.x-_pos0.x,0);
        Point2DFloat verD(0,_pos3.y-_pos0.y);
        
        return _pos0 + horD*ilsHor + verD*ilsVer + pos;
      }
    }
  }
  // invalid value - ILS will be clipped
  return Point2DFloat(-1,-1);
}

Point2DFloat HUDBindBones::Position(const HUDDrawContext &ctx) const
{
  Point2DFloat res(0,0);
  for (int i=0; i<Size(); i++)
  {
    const HUDBindSingleBone &bone = Get(i);
    if (bone._bone)
    {
      res = res + bone._bone->Transform(ctx,bone._offset)*bone._weight;
    }
    else
    {
      res = res + bone._offset*bone._weight;
    }
  }
  return res;
}

void HUDBindBones::Load(const HUDBoneList &bones, const IParamArrayValue &entry)
{
  // entry should be an array, like 
  // beg[]={AltBeg,1};
  float weigthSum = 0;
  Resize(0);
  Realloc(entry.GetItemCount()/2);
  for (int index=0; index<entry.GetItemCount(); )
  {
    // first is bone name (or direct fixed bone)
    const IParamArrayValue &boneName = entry[index];
    Ref<HUDBone> bone;
    Point2DFloat offset(0,0);
    if (!boneName.IsArrayValue())
    {
      bone = bones.FindBone(boneName);
      if (!bone)
      {
        ErrF("HUD: bone %s not found",cc_cast(boneName.operator RString()));
        Clear();
        break;
      }
      index++;
    }
    const IParamArrayValue &offsetEntry = entry[index];
    if (offsetEntry.IsArrayValue())
    {
      // offset only irect fixed bone
      float x = offsetEntry[0];
      float y = offsetEntry[1];
      offset = Point2DFloat(x,y);
      index++;
    }
    if (index==0)
    {
      ErrF("HUD: No bone or offset");
      Clear();
      break;
    }
    float weight = entry[index++];
    weigthSum += weight;
    HUDBindSingleBone &bind = Append();
    bind._weight = weight;
    bind._bone = bone;
    bind._offset = offset;
  }
  Compact();
  // do not normalize - we can use unnormalized values for bone addition/subtraction
}

void HUDBindBones::Load(const HUDBoneList &bones, ParamEntryPar entry)
{
  // entry should be an array, like 
  // beg[]={AltBeg,1};
  float weigthSum = 0;
  Resize(0);
  Realloc(entry.GetSize()/2);
  for (int index=0; index<entry.GetSize(); )
  {
    // first is bone name (or direct fixed bone)
    const IParamArrayValue &boneName = entry[index];
    Ref<HUDBone> bone;
    Point2DFloat offset(0,0);
    if (!boneName.IsArrayValue())
    {
      bone = bones.FindBone(boneName);
      if (!bone)
      {
        ErrF("HUD: bone %s not found",cc_cast(boneName.operator RString()));
        Clear();
        break;
      }
      index++;
    }
    const IParamArrayValue &offsetEntry = entry[index];
    if (offsetEntry.IsArrayValue())
    {
      // offset only irect fixed bone
      float x = offsetEntry[0];
      float y = offsetEntry[1];
      offset = Point2DFloat(x,y);
      index++;
    }
    if (index==0)
    {
      ErrF("HUD: No bone or offset");
      Clear();
      break;
    }
    float weight = entry[index++];
    weigthSum += weight;
    HUDBindSingleBone &bind = Append();
    bind._weight = weight;
    bind._bone = bone;
    bind._offset = offset;
  }
  Compact();
  // do not normalize - we can use unnormalized values for bone addition/subtraction
}

DEF_RSB(fixed)
DEF_RSB(linear)
DEF_RSB(rotational)
DEF_RSB(horizon)
DEF_RSB(ils)
DEF_RSB(vector)

void HUDBoneList::Load(ParamEntryPar entry)
{
  Resize(0);
  Realloc(entry.GetEntryCount());
  for (int i=0; i<entry.GetEntryCount(); i++)
  {
    ParamEntryVal item = entry.GetEntry(i);
    // ignore non-class entries
    if (!item.IsClass()) continue;
    // check type
    RStringB type = item>>"type";
    Ref<HUDBone> bone;
    if (type==RSB(fixed)) bone = new HUDBoneFixed(item.GetName());
    else if (type==RSB(linear)) bone = new HUDBoneLinear(item.GetName());
    else if (type==RSB(rotational)) bone = new HUDBoneRotational(item.GetName());
    else if (type==RSB(horizon)) bone = new HUDBoneHorizon(item.GetName());
    else if (type==RSB(ils)) bone = new HUDBoneILS(item.GetName());
    else if (type==RSB(vector)) bone = new HUDBoneVector(item.GetName());
    else
    {
      ErrF(
        "Unknown HUD bone %s in %s",
        cc_cast(type),cc_cast(entry.GetContext(item.GetName()))
      );
    }
    if (bone)
    {
      bone->Load(item);
      Add(bone);
    }
  }
  Compact();
}

HUDBone *HUDBoneList::FindBone(RStringB bone) const
{
  for (int i=0; i<Size(); i++)
  {
    if (!strcmpi(Get(i)->GetName(),bone))
    {
      return Get(i);
    }
  }
  return NULL;
}



HUDDrawContext::HUDDrawContext()
{
  color = PackedWhite;
  enabled = true;
  _offsetY =_offsetX= 0.0f;
}

HUDDrawContext::~HUDDrawContext()
{
  if (_viewportClipped!=_viewport)
  {
    _viewport->DestroyChild(_viewportClipped);
  }
}

bool HUDDrawContext::CheckCondition(const char *cnd) const
{
  if (!conditions) return false;
  int mask = 1;
  for (const char **c=conditions; *c; c++,mask<<=1)
  {
    if (!strcmpi(*c,cnd)) return (mask&conditionMask)!=0;
  }
  return false;
}

/// line, continuous between two or more points
class HUDLine: public AutoArray<HUDBindBones>
{
};

TypeIsMovable(HUDLine)

/// line drawing - basic element
/** consists of multiple lines
*/

class HUDElementLine: public HUDElement
{
  AutoArray<HUDLine> _lines;
  float _width;
  
  /// helper for drawing
  void Draw(
    const HUDDrawContext &ctx, Point2DFloat beg, Point2DFloat end
  ) const;

  public:
  HUDElementLine();
  //@{ HUDElement implementation
  virtual void Load(const HUDBoneList &bones,ParamEntryPar entry);
  virtual void Draw(const HUDDrawContext &ctx) const;
  //@}
};

HUDElementLine::HUDElementLine()
{
  _width = -8.0f*0.0008f;
}

void HUDElementLine::Load(const HUDBoneList &bones, ParamEntryPar entry)
{
  ParamEntryVal points = entry>>"points";
  // scan all points
  HUDLine line;
  for (int index=0; index<points.GetSize(); )
  {
    const IParamArrayValue &val = points[index];
    // assume val is array
    if (val.GetItemCount()==0)
    {
      // terminate current line
      if (line.Size()>=2)
      {
        line.Compact(),_lines.Add(line);
      }
      line.Clear();
      index++;
      continue;
    }
    // point expected
    HUDBindBones &bind = line.Append();
    bind.Load(bones,val);
    index++;
  }
  if (line.Size()>=2)
  {
    line.Compact(),_lines.Add(line);
  }
  _lines.Compact();

  ConstParamEntryPtr width = entry.FindEntry("width");
  if (width) _width = -(((float)*width)*0.0008);
}


/// polygon drawing 
/** draws a filled polygon, last point doesn't have to be specified
*/
#if _HUD_DRAW_POLYGON
class HUDElementPolygon: public HUDElement
{
protected:
  Texture* texture;
  AutoArray<HUDBindBones> _points;

public:
  //@{ HUDElement implementation
  virtual void Load(const HUDBoneList &bones,ParamEntryPar entry);
  virtual void Draw(const HUDDrawContext &ctx) const;

};

void HUDElementPolygon::Load(const HUDBoneList &bones, ParamEntryPar entry)
{
  ParamEntryVal points = entry>>"points";
  // scan all points
  for (int index=0; index<points.GetSize(); )
  {
    const IParamArrayValue &val = points[index];
    // assume val is array
    if (val.GetItemCount()==0)
      continue;

    HUDBindBones &bind = _points.Append();
    bind.Load(bones,val);
    // point expected
    index++;
  }
/* not used yet
  ConstParamEntryPtr texture = entry.FindEntry("texture");
  if (texture)
  {
    RString path = *texture;  
    const char *ptr = strchr(path, '.');
    if (!ptr)
      path = path +".paa";

    _texture = GlobLoadTexture(path.Lower());
  }
  else
    _texture = NULL;
*/
}

void HUDElementPolygon::Draw(const HUDDrawContext &ctx) const
{
  int nPoints = _points.Size();

  if(nPoints <= 2) return;

  Vertex2DFloat* vf = new Vertex2DFloat[nPoints];  
  
  for (int p=0; p < nPoints; ++p)
  {
    const HUDBindBones &bone = _points[p];
    Point2DFloat pt = bone.Position(ctx);
    vf[p].x = pt.x;
    vf[p].y = pt.y * ctx._yCoef;
    vf[p].color = ctx.color;
  }

  MipInfo mip = GEngine->TextBank()->UseMipmap(NULL, 0, 0);
//  ctx._viewportClipped->DrawPoly(mip, vf, nPoints, DstBlendOne, ClipAll | (MSShiningAdjustable * ClipUserStep));
  ctx._viewportClipped->DrawPoly(mip, vf, nPoints, IsColored, ClipAll | (MSShiningAdjustable * ClipUserStep));

  delete[] vf;
}
#endif
/// textual information on HUD
class HUDElementText: public HUDElement
{
  public:
  enum TextAlign {Left,Right,Center};
  
  private:
  //@{ location of the text
  HUDBindBones _pos,_right,_down;
  //@}
  
  //@{ text attributes
  TextAlign _align;
  float _scale;
  //@}
  
  //@{ source of the text 
  HUDValueSelect _source;
  float _sourceFactor;
  //@}
  //@{ the text is static
  bool _static;
#if _VBS2 //feel free to enable for Arma ;)
  bool _isTime;
#endif
  RString _text;
  //@}
  public:
  //@{ HUDElement
  virtual void Load(const HUDBoneList &bones, ParamEntryPar entry);
  virtual void Draw(const HUDDrawContext &ctx) const;
  //@}
};

static const EnumName TextAlignName[]=
{
  EnumName(HUDElementText::Left,"left"),
  EnumName(HUDElementText::Right,"right"),
  EnumName(HUDElementText::Center,"center"),
  EnumName()
};

template <>
const EnumName *GetEnumNames(HUDElementText::TextAlign dummy) {return TextAlignName;}

void HUDElementText::Load(const HUDBoneList &bones, ParamEntryPar entry)
{
  RStringB alignText = entry>>"align";
  RStringB sourceText = entry>>"source";
  _align = GetEnumValue<TextAlign>(alignText);
  _scale = entry>>"scale";

#if _VBS2
  _isTime = false;
#endif

  _static = false;
  if(strcmpi(cc_cast(sourceText), "static")==0)
  {
    _static = true;
    _text = entry >> "text";
  }
#if _VBS2
  else
  if(strcmpi(cc_cast(sourceText), "time")==0)
  {
    _isTime = true;
    _text = entry >> "text"; //stores the format of the time
  }
#endif

  else
  {
    _source = GetEnumValue<HUDValueSelect>(sourceText);
    if (_source<0 || _source>=NHUDValueSelect)
    {
      RptF("Value %s not recognized as a source",cc_cast(sourceText));
    }
    _sourceFactor = entry>>"sourceScale";
  }  
  _pos.Load(bones,entry>>"pos");
  _right.Load(bones,entry>>"right");
  _down.Load(bones,entry>>"down");
}

RString HUDGetTextSource(HUDValueSelect _source,const HUDDrawContext &ctx,float _sourceFactor)
{
  RString text;
  switch (_source)
  {
    case HUDValueWeapon:
    {
      // copy of UI\inGameUIDraw.cpp line 2784
      WeaponModePtr ptr;
      TurretContext context;
      Transport *transport = dyn_cast<Transport>(ctx.vehicle);
      AIBrain *agent = GWorld->FocusOn();
      if (!agent) return RString();
      if (transport && transport->IsManualFire() && agent == transport->CommanderUnit() && ctx.vehicle->GetPrimaryGunnerTurret(context))
      {
        // when manual fire is on, commander is controlling gunner's weapons (even when has own weapons)
        ptr = context._weapons->ValidatedCurrentWeapon();
      }
#if _VBS3 //Commander override
      else if (ctx.vehicle->FindOpticsTurret(agent->GetPerson(), context) && ctx.vehicle->CanFireUsing(context))
#else
      else if (ctx.vehicle->FindTurret(agent->GetPerson(), context) && ctx.vehicle->CanFireUsing(context))
#endif
      {
        ptr = context._weapons->ValidatedCurrentWeapon();
      }
      else if (ctx.vehicle->GetPrimaryGunnerTurret(context))
      {
        ptr = context._weapons->ValidatedCurrentWeapon();
      }
      if (ptr.IsValid())
      {
        const WeaponModeType *mode = ptr.GetWeaponModeType();
        if (mode) text = mode->_displayName;
      }
      break;
    }
    case HUDValueAmmo:
    {
      // copy of UI\inGameUIDraw.cpp line 2784
      WeaponModePtr ptr;
      TurretContext context;
      Transport *transport = dyn_cast<Transport>(ctx.vehicle);
      AIBrain *agent = GWorld->FocusOn();
      if (!agent) return RString();
      if (transport && transport->IsManualFire() && agent == transport->CommanderUnit() && ctx.vehicle->GetPrimaryGunnerTurret(context))
      {
        // when manual fire is on, commander is controlling gunner's weapons (even when has own weapons)
        ptr = context._weapons->ValidatedCurrentWeapon();
      }
#if _VBS3 //Commander override
      else if (ctx.vehicle->FindOpticsTurret(agent->GetPerson(), context) && ctx.vehicle->CanFireUsing(context))
#else
      else if (ctx.vehicle->FindTurret(agent->GetPerson(), context) && ctx.vehicle->CanFireUsing(context))
#endif
      {
        ptr = context._weapons->ValidatedCurrentWeapon();
      }
      else if (ctx.vehicle->GetPrimaryGunnerTurret(context))
      {
        ptr = context._weapons->ValidatedCurrentWeapon();
      }
      if (ptr.IsValid())
      {
        const MuzzleType *muzzle = ptr.GetMuzzleType();

        int ammoCur = 0;
        int maxAmmoCur = 0;
        int magazines = 0;
        const Magazine *currentMagazine = ptr.GetMagazine();
        const Magazine *magazine = currentMagazine;
        if (!magazine)
        {
          // find magazine of first compatible type
          for (int i=0; i<muzzle->_magazines.Size(); i++)
          {
            const MagazineType *type = muzzle->_magazines[i];
            context._weapons->ForEachMagazine([&magazine, &magazines, type](Magazine *mag) -> bool
            {
              if (mag->Type() != type) return false; // continue
              if (mag->GetAmmo() == 0) return false; // continue
              magazines++;
              if (!magazine)
              {
                magazine = mag;
              }
              return false; // continue
            });
            if (magazine) break;
          }

          if (magazine)
          {
            maxAmmoCur = magazine->Type()->_maxAmmo;
          }
          else if (muzzle->_magazines.Size() > 0)
          {
            maxAmmoCur = muzzle->_magazines[0]->_maxAmmo;
          }
        }
        else // FIX: wrong calculation of magazines
        {
          ammoCur = magazine->GetAmmo();
          maxAmmoCur = magazine->Type()->_maxAmmo;

          // reserve magazines
          context._weapons->ForEachMagazine([magazine, &magazines](Magazine *reserve) -> bool
          {
            if (reserve == magazine) return false; // continue
            if (reserve->Type() == magazine->Type())
            {
              if (reserve->GetAmmo() > 0) magazines++;
            }
            return false; // continue
          });
        }
        if (maxAmmoCur > 0)
        {
          if (magazine && magazine->Type()->_maxAmmo == 1)
          {
            ammoCur = magazines;
            if (currentMagazine && currentMagazine->GetAmmo() > 0) ammoCur++;
            magazines = 0;
          }
          char buffer[64];
          if (magazines > 0)
            sprintf(buffer, "%d | %d", ammoCur, magazines);
          else
            sprintf(buffer, "%d", ammoCur);
          text = RString(buffer);
        }
      }
      break;
    }
    default:
    {
      char buffer[64];
      float value = HUDGetSource(_source,ctx)*_sourceFactor;
      if (fabs(value)<0.5) value = 0.0f; // disable "-0"/"0" blinking
      sprintf(buffer, "%.0f", value); 
      text = RString(buffer);
    };
  };
  return text;
}

void HUDElementText::Draw(const HUDDrawContext &ctx) const
{
  if (!GEngine->IsFontReady3D(ctx.font)) return;
  
  Point2DFloat pos = _pos.Position(ctx);
  Point2DFloat right = _right.Position(ctx) - pos;
  Point2DFloat down = _down.Position(ctx) - pos;

  pos.y += ctx._offsetY;
  pos.x += ctx._offsetX;

  pos.y *= ctx._yCoef;
  right.y *= ctx._yCoef;
  down.y *= ctx._yCoef;
  
  BString<64> text;

  if(_static)
  {
    text = _text;
  }
#if _VBS2
  else
  if (_isTime)
  {
    char buffer[256];
    Glob.clock.FormatDate(cc_cast(_text), buffer);
    text = buffer;
  }
#endif
  else
  {
    text = HUDGetTextSource(_source,ctx,_sourceFactor);
  }
  switch (_align)
  {
    case Left:
 {
      float width = GEngine->GetTextWidth(1, ctx.font, text);
      pos = pos - right * width;
		  break;
    }
    case Center:
    {
      float width = GEngine->GetTextWidth(1, ctx.font, text);
      pos = pos - right * (0.5f * width);
		  break;
    }
  }
  TextDrawAttr attr(1, ctx.font, ctx.color, 0, false, TLMaterial::_shiningAdjustable, DstBlendOne);
  ctx._viewportClipped->DrawText(pos, right, down, attr, text);
}


void HUDElementLine::Draw(const HUDDrawContext &ctx) const
{
  for (int l=0; l<_lines.Size(); l++)
  {
    const HUDLine &line = _lines[l];
    for (int i=1; i<line.Size(); i++)
      Draw(ctx,line[i-1].Position(ctx),line[i].Position(ctx));
  }
}

static inline void Swap(Point2DFloat &p, Point2DFloat &q )
{
  Point2DFloat tmp=p;
  p=q;
  q=tmp;
}

void HUDElementLine::Draw(const HUDDrawContext &ctx, Point2DFloat beg, Point2DFloat end) const
{
  Line2DFloat line;
  line.beg.x = beg.x + ctx._offsetX;
  line.beg.y = ctx._yCoef * (beg.y + ctx._offsetY);
  line.end.x = end.x + ctx._offsetX;
  line.end.y = ctx._yCoef * (end.y + ctx._offsetY);
  ctx._viewportClipped->DrawLine(line, ctx.color, ctx.color, _width, DstBlendOne, TLMaterial::_shiningAdjustable);
}

/// define color (not including alpha)
class HUDAttributeColor: public HUDAttribute
{
  // 24b color - alpha ignored
  PackedColor _color;
public:
  HUDAttributeColor(ParamEntryVal entry) {GetValue(_color,entry);}
  //@{ HUDAttribute implementation
  virtual void Apply(HUDDrawContext &ctx) {ctx.color.SetRGB(_color);}
  //@}
};

/// define alpha (intensity)
class HUDAttributeAlpha: public HUDAttribute
{
  /// 8b alpha
  int _alpha;
  
public:
  HUDAttributeAlpha(ParamEntryVal entry)
  {
    float alpha = entry;
    _alpha = toInt(alpha*255);
    saturate(_alpha,0,255);
  }
  //@{ HUDAttribute implementation
  virtual void Apply(HUDDrawContext &ctx) {ctx.color.SetA8(_alpha);}
  //@}
};
/// limit clipping region (top left)
class HUDAttributeClipTL: public HUDAttribute
{
  Point2DFloat _topLeft;
public:
  HUDAttributeClipTL(ParamEntryVal entry) {HUDBone::LoadPos(_topLeft,entry);}
  //@{ HUDAttribute implementation
  virtual void Apply(HUDDrawContext &ctx);
  //@}
};

void HUDAttributeClipTL::Apply(HUDDrawContext &ctx)
{
  Rect2DFloat rect;
  ctx._viewportClipped->GetDrawArea(rect);
  saturateMax(rect.x, _topLeft.x);
  saturateMax(rect.y, ctx._yCoef * _topLeft.y);
  if(ctx._viewportClipped != ctx._viewport)
    ctx._viewport->DestroyChild(ctx._viewportClipped);
  ctx._viewportClipped = ctx._viewport->CreateChild(Point2DFloat(0, 0), rect);
}

/// limit clipping region (bottom right)
class HUDAttributeClipBR: public HUDAttribute
{
  Point2DFloat _bottomRight;
public:
  HUDAttributeClipBR(ParamEntryVal entry) {HUDBone::LoadPos(_bottomRight, entry);}
  //@{ HUDAttribute implementation
  virtual void Apply(HUDDrawContext &ctx);
  //@}
};

void HUDAttributeClipBR::Apply(HUDDrawContext &ctx)
{
  Rect2DFloat rect;
  ctx._viewportClipped->GetDrawArea(rect);
  float x = rect.x + rect.w;
  float y = rect.y + rect.h;
  saturateMin(x, _bottomRight.x);
  saturateMin(y, ctx._yCoef * _bottomRight.y);
  rect.w = x - rect.x;
  rect.h = y - rect.y;
  if(ctx._viewportClipped != ctx._viewport)
    ctx._viewport->DestroyChild(ctx._viewportClipped);
  ctx._viewportClipped = ctx._viewport->CreateChild(Point2DFloat(0, 0), rect);
}

/// skip group based on condition
class HUDAttributeCondition: public HUDAttribute
{
  RStringB _condition;
public:
  HUDAttributeCondition(ParamEntryVal entry)
  :_condition(entry.operator RString())
  {
  }
  //@{ HUDAttribute implementation
  virtual void Apply(HUDDrawContext &ctx)
  {
    if (!ctx.CheckCondition(_condition))
      ctx.enabled = false;
  }
  //@}
};

void HUDElementGroup::Load(const HUDBoneList &bones,ParamEntryPar entry)
{
  // load attributes
  // use a more standard way to enable inheritance
  ConstParamEntryPtr ptr = entry.FindEntry("color");
  if (ptr) _override.Add(new HUDAttributeColor(*ptr));
  ptr = entry.FindEntry("alpha");
  if (ptr) _override.Add(new HUDAttributeAlpha(*ptr));
  ptr = entry.FindEntry("clipTL");
  if (ptr) _override.Add(new HUDAttributeClipTL(*ptr));
  ptr = entry.FindEntry("clipBR");
  if (ptr) _override.Add(new HUDAttributeClipBR(*ptr));
  ptr = entry.FindEntry("condition");
  if (ptr) _override.Add(new HUDAttributeCondition(*ptr));

  _override.Compact();
  
  _items.Load(bones,entry);
}

void HUDElementGroup::Draw(const HUDDrawContext &ctx) const
{
  // context stack implemented here
  HUDDrawContext curr = ctx;

  // prevent destruction of inherited viewports
  curr._viewport = curr._viewportClipped;
  // override any attributes as needed
  for (int i=0; i<_override.Size(); i++)
  {
    _override[i]->Apply(curr);
  }
  // if rendering is disabled, there is nothing to draw - we can skip
  if (curr.enabled)
  {
    _items.Draw(curr);
  }
}

class HUDElementScale: public HUDElement
{
public:
  enum TextAlign {Left,Right,Center};
private:
  float _width;
  float _lineXleft,_lineYright;
  float _lineXleftMajor,_lineYrightMajor;
  bool _horizontal; 
  
  // border attributes
  float _top;
  float _bottom;
  float _center;

  float _step;
  float _stepSize;

  int _numberEach;
  
  // text attributes
  Point2DFloat _pos,_right,_down;
  TextAlign _align;
  float _scale;
  int _majorLineEach;


  // source attributes
  float _sourceFactor;
  float _min,_max;
  HUDValueSelect _source;

    void DrawLine(
    const HUDDrawContext &ctx, Point2DFloat beg, Point2DFloat end
  ) const;

  void LoadPos(Point2DFloat &pos, ParamEntryPar entry)
  {
    float x = entry[0];
    float y = entry[1];
    pos = Point2DFloat(x,y);
  }

public:
  /// helper for drawing
/*  void Draw(
    const HUDDrawContext &ctx, Point2DFloat beg, Point2DFloat end
  ) const;*/


  HUDElementScale();
  //@{ HUDElement implementation
  virtual void Load(const HUDBoneList &bones,ParamEntryPar entry);
  virtual void Draw(const HUDDrawContext &ctx) const;
  //@}
};

void HUDElementScale::DrawLine(const HUDDrawContext &ctx, Point2DFloat beg, Point2DFloat end) const
{
  Line2DFloat line;
  line.beg.x = beg.x + ctx._offsetX;
  line.beg.y = ctx._yCoef * (beg.y + ctx._offsetY);
  line.end.x = end.x + ctx._offsetX;
  line.end.y = ctx._yCoef * (end.y + ctx._offsetY);
  ctx._viewportClipped->DrawLine(line, ctx.color, ctx.color, _width, DstBlendOne, TLMaterial::_shiningAdjustable);
}

HUDElementScale::HUDElementScale()
{
  _width = -8.0f*0.0008f;
}


template <>
const EnumName *GetEnumNames(HUDElementScale::TextAlign dummy) {return TextAlignName;}

void HUDElementScale::Load(const HUDBoneList &bones, ParamEntryPar entry)
{
  RStringB alignText = entry>>"align";
  RStringB sourceText = entry>>"source";
  _align = GetEnumValue<TextAlign>(alignText);
  _scale = entry>>"scale";
  
  _source = GetEnumValue<HUDValueSelect>(sourceText);
  if (_source<0 || _source>=NHUDValueSelect)
  {
    RptF("Value %s not recognized as a source",cc_cast(sourceText));
  }
  _sourceFactor = entry>>"sourceScale";

  LoadPos(_pos,entry>>"pos");
  LoadPos(_right,entry>>"right");
  LoadPos(_down,entry>>"down");

  _step = entry>>"step";
  _stepSize = entry>>"stepSize";

  _horizontal = entry>>"horizontal";
  _lineXleft = entry>>"lineXleft";
  _lineYright = entry>>"lineYright";
  _lineXleftMajor = entry>>"lineXleftMajor";
  _lineYrightMajor = entry>>"lineYrightMajor";

  _top = entry>>"top";
  _bottom = entry>>"bottom";
  _center = entry>>"center";
  _majorLineEach = entry>>"majorLineEach";
  _numberEach = entry>>"numberEach";

  ConstParamEntryPtr width = entry.FindEntry("width");
  if (width) _width = -(((float)*width)*0.0008);
}

void HUDElementScale::Draw(const HUDDrawContext &ctx) const
{
  if (!GEngine->IsFontReady3D(ctx.font)) return;
  float value = HUDGetSource(_source,ctx)*_sourceFactor;
  

  float stepSize = _stepSize;
  if (_top>_bottom) stepSize = -stepSize;
  value -= (_center-_top)*_step/stepSize;

  float delta = fmod(value,_step);
  float factor = delta/_step;
  

  int valueScaled = value/_step;
  for (int i=1; i<100; ++i) // just for case limit all lines
  {
    ++valueScaled;
    float offset = stepSize*i - stepSize*factor;

    Point2DFloat offset2D;

    if (_top>_bottom) 
    {
      if (-offset>_top-_bottom) break;
    }
    else
    {
      if (offset>_bottom-_top) break;
    }

    if (_horizontal)
    {
      offset2D = Point2DFloat(offset,0.0f);
      if (valueScaled%_majorLineEach==0)
        DrawLine(ctx,Point2DFloat(_top,_lineYrightMajor)+offset2D,Point2DFloat(_top,_lineXleftMajor)+offset2D);
      else
        DrawLine(ctx,Point2DFloat(_top,_lineYright)+offset2D,Point2DFloat(_top,_lineXleft)+offset2D);
    }
    else
    {
      offset2D = Point2DFloat(0.0f,offset);
      if (valueScaled%_majorLineEach==0)
        DrawLine(ctx,Point2DFloat(_lineYrightMajor,_top)+offset2D,Point2DFloat(_lineXleftMajor,_top)+offset2D);
      else
        DrawLine(ctx,Point2DFloat(_lineYright,_top)+offset2D,Point2DFloat(_lineXleft,_top)+offset2D);
    }

    if (valueScaled%_numberEach==0)
    {
      Point2DFloat pos = _pos;
      Point2DFloat right = _right - pos;
      Point2DFloat down = _down - pos;

      pos = pos + offset2D;

      pos.x += ctx._offsetX;
      pos.y += ctx._offsetY;

      pos.y *= ctx._yCoef;
      right.y *= ctx._yCoef;
      down.y *= ctx._yCoef;

      BString<64> text;
      float textValue = value - delta;
      textValue += _step*i;
      if (_source == HUDValueHeading)
      {
        if (textValue < 0) textValue += 360.0f*_sourceFactor;
        if (textValue >= 360.0f*_sourceFactor) textValue -= 360.0f*_sourceFactor;
      };
      sprintf(text, "%.0f", textValue); 
      
      switch (_align)
      {
        case Left:
        {
          float width = GEngine->GetTextWidth(1, ctx.font, text);
          pos = pos - right * width;
		      break;
        }
        case Center:
        {
          float width = GEngine->GetTextWidth(1, ctx.font, text);
          pos = pos - right * (0.5f * width);
		      break;
        }
      }
      TextDrawAttr attr(1, ctx.font, ctx.color, 0, false, TLMaterial::_shiningAdjustable, DstBlendOne);
      ctx._viewportClipped->DrawText(pos, right, down, attr, text);
    }
  }
}

/*
void HUDElementScale::Draw(const HUDDrawContext &ctx) const
{

  if (!GEngine->IsFontReady3D(ctx.font)) return;
  float value = HUDGetSource(_source,ctx)*_sourceFactor;
  
  if (_top>_bottom) 
  {
    value += (_center-_top)*_step/_stepSize;
  }
  else
  {
    value -= (_center-_top)*_step/_stepSize;
  }

  float delta = fmod(value,_step);
  float factor = delta/_step;
  

  for (int i=1; i<100; ++i)
  {
    int valueScaled = toIntFloor(value/_step+i);
    float offset = _stepSize*i - _stepSize*factor;

    Point2DFloat offset2D;

    if (_top>_bottom) 
    {
      if (offset>_top-_bottom) break;
      offset = -offset; 
    }
    else
    {
      if (offset>_bottom-_top) break;
    }

    if (_horizontal)
    {
      offset2D = Point2DFloat(offset,0.0f);
      if (valueScaled%_majorLineEach==0)
        DrawLine(ctx,Point2DFloat(_top,_lineYrightMajor)+offset2D,Point2DFloat(_top,_lineXleftMajor)+offset2D);
      else
        DrawLine(ctx,Point2DFloat(_top,_lineYright)+offset2D,Point2DFloat(_top,_lineXleft)+offset2D);
    }
    else
    {
      offset2D = Point2DFloat(0.0f,offset);
      if (valueScaled%_majorLineEach==0)
        DrawLine(ctx,Point2DFloat(_lineYrightMajor,_top)+offset2D,Point2DFloat(_lineXleftMajor,_top)+offset2D);
      else
        DrawLine(ctx,Point2DFloat(_lineYright,_top)+offset2D,Point2DFloat(_lineXleft,_top)+offset2D);
    }

    if (valueScaled%_numberEach==0)
    {
      Point2DFloat pos = _pos;
      Point2DFloat right = _right - pos;
      Point2DFloat down = _down - pos;

      pos = pos + offset2D;

      pos.y *= ctx._yCoef;
      right.y *= ctx._yCoef;
      down.y *= ctx._yCoef;

      BString<64> text;
      float textValue = value - delta;
      textValue += _step*i;
      if (_source == HUDValueHeading)
      {
        if (textValue < 0) textValue += 360.0f*_sourceFactor;
        if (textValue >= 360.0f*_sourceFactor) textValue -= 360.0f*_sourceFactor;
      };
      sprintf(text, "%.0f", textValue); 
      
      switch (_align)
      {
        case Left:
        {
          float width = GEngine->GetTextWidth(1, ctx.font, text);
          pos = pos - right * width;
		      break;
        }
        case Center:
        {
          float width = GEngine->GetTextWidth(1, ctx.font, text);
          pos = pos - right * (0.5f * width);
		      break;
        }
      }
      TextDrawAttr attr(1, ctx.font, ctx.color, false, TLMaterial::_shiningAdjustable, DstBlendOne);
      ctx._viewportClipped->DrawText(pos, right, down, attr, text);
    }
  }
}*/


DEF_RSB(line)
DEF_RSB(group)
DEF_RSB(scale)
DEF_RSB(text)
#if _HUD_DRAW_POLYGON
DEF_RSB(polygon)
#endif

void HUDElementList::Load(const HUDBoneList &bones, ParamEntryPar entry)
{
  Resize(0);
  Realloc(entry.GetEntryCount());
  for (int i=0; i<entry.GetEntryCount(); i++)
  {
    ParamEntryVal item = entry.GetEntry(i);
    // ignore non-class entries
    if (!item.IsClass()) continue;
    // check type
    Ref<HUDElement> bone;
    // if there is no type, assume group
    RStringB type = item.ReadValue("type",RSB(group));
    if (type==RSB(line)) bone = new HUDElementLine;
    else if (type==RSB(group)) bone = new HUDElementGroup;
    else if (type==RSB(text)) bone = new HUDElementText;
    else if (type==RSB(scale)) bone = new HUDElementScale;
#if _HUD_DRAW_POLYGON
    else if (type==RSB(polygon))bone = new HUDElementPolygon;
#endif
    else
      ErrF("Unknown HUD element %s in %s",cc_cast(type),cc_cast(entry.GetContext(item.GetName())));
    if (bone)
    {
      bone->Load(bones,item);
      Add(bone);
    }
  }
  Compact();
}

void HUDElementList::Draw(const HUDDrawContext &ctx) const
{
  for (int i=0; i<Size(); i++)
    Get(i)->Draw(ctx);
}

void MFDDesc::Load(ParamEntryPar entry)
{
  _bones.Load(entry>>"Bones");
  _elements.Load(_bones,entry>>"Draw");
  GetValue(_color,entry>>"color");
  _posName = entry>>"topLeft";
  _rightName = entry>>"topRight";
  _downName = entry>>"bottomLeft";
  //RStringB fontName = entry>>"font";
  //_font = fontName.GetLength()>0 ? GEngine->LoadFont(GetFontID(fontName)) : NULL;

  // TODO: create font indirection
  _font = GEngine->LoadFont(GetFontID(Pars >> "fontHelicopterHUD"));

  _borderLeft=entry>>"borderLeft";
  _borderRight=entry>>"borderRight";
  _borderTop=entry>>"borderTop";
  _borderBottom=entry>>"borderBottom";

  if(entry.FindEntry("helmetMountedDisplay"))
  {
    _helmetMountedDisplay=entry>>"helmetMountedDisplay";
    GetValue(_helmetPosition,entry>>"helmetPosition");
    GetValue(_helmetRight,entry>>"helmetRight");
    GetValue(_helmetDown,entry>>"helmetDown");
  }
  else _helmetMountedDisplay = false;

  if(entry.FindEntry("enableParallax"))
  {
    _enableParallax=entry>>"enableParallax"; 
  }
  else _enableParallax = false; 
}

void MFDDesc::InitShape(LODShape *shape)
{
  
  _position = _right = _down = -1;
  const Shape *mem = shape->MemoryLevel();
  if (mem)
  {
    _position = shape->PointIndex(mem,_posName);
    _right = shape->PointIndex(mem,_rightName);
    _down = shape->PointIndex(mem,_downName);
  }
  
}

void MFDDesc::Draw(int cb, Transport *container, const PositionRender &pos) const
{
  if (!_helmetMountedDisplay && (_right<0 || _down<0 || _position<0)) return;

  //draw helmet Mounted Display only for (pilot && player)
  if (_helmetMountedDisplay)
  {
    if(!GWorld->PlayerOn() || (container->Driver() != GWorld->PlayerOn() && container->Copilot() != GWorld->PlayerOn())) return;
    if(GWorld->UI()->IGUI_HMD()) return;
  }

  HUDDrawContext ctx;

  if(!_helmetMountedDisplay)
  {
    int memLevel = container->GetShape()->FindMemoryLevel();

    Vector3 hudPosition = pos.position.FastTransform(container->AnimatePoint(container->RenderVisualState(),memLevel,_position));
    Vector3 hudRight = pos.position.FastTransform(container->AnimatePoint(container->RenderVisualState(),memLevel,_right));
    Vector3 hudDown = pos.position.FastTransform(container->AnimatePoint(container->RenderVisualState(),memLevel,_down));

    Vector3 position = hudPosition + (hudRight-hudPosition)*_borderLeft + (hudDown-hudPosition)*_borderTop;
    Vector3 right = (hudRight-hudPosition)*(1-_borderLeft-_borderRight);
    Vector3 down = (hudDown-hudPosition)*(1-_borderTop-_borderBottom);

    ctx._viewport = Create3DViewport(cb,position, right, down, NULL, true, pos.camSpace);

    if (_enableParallax)
    {
      Person *driver= GWorld->PlayerOn();
      if (driver)
      {
        float sizeX = right.Size();
        float sizeY = down.Size();
        if (sizeX>0 && sizeY >0)
        {
          Vector3 offset = driver->GetHUDOffset();
          ctx._offsetX = offset.X() / sizeX;
          ctx._offsetY = -offset.Y() / sizeY;
        }
      }
    }
  }
  else
  {
    ctx._viewport = Create3DViewport(cb,_helmetPosition, _helmetRight, _helmetDown , NULL, true, pos.camSpace);
  }
  ctx._viewportClipped = ctx._viewport;
  
  Rect2DFloat area;
  ctx._viewport->GetDrawArea(area);
  ctx._yCoef = area.h;

  ctx.color = _color;
  ctx.font = _font;
  ctx.vehicle = container;

  ctx.conditions = container->HUDMode(ctx.conditionMask);

  _elements.Draw(ctx);

  if (ctx._viewportClipped!=ctx._viewport)
  {
    ctx._viewport->DestroyChild(ctx._viewportClipped);
    ctx._viewportClipped = ctx._viewport;
  }
  Destroy3DViewport(ctx._viewport);
}
