#include "spawn.hpp"
#include "zombie.hpp" // for isInWater etc
#include "gameStateExt.hpp" // for isInWater etc
#include <Common/enf_mathlib2d.h>
#include <Common/enf_mathlib.h>
#include <lib/thing.hpp>
#include <lib/seagull.hpp>
#include <lib/world.hpp>
#include <vector>

//#define SPAWN_DEBUG 1

using enf::Vector2;
using enf::Vector4;

struct Quad {
  Vector2 _x0;
  Vector2 _x1;
  Vector2 _x2;
  Vector2 _x3;
};

enum GOT
{
  GOT_unknown,
  GOT_zombie,
  GOT_player,
  GOT_static
};

/**@class     InGameObject
 * @brief     information about objects in the area
 **/
struct InGameObject
{
  GOT       _type; /// enum GOT
  Vector3   _x;    /// position vector, as in physics
  enf::AAB  _aabb; /// bounding box of the object
  Object *  _obj;

  InGameObject () { }
};
TypeIsSimpleZeroed(InGameObject);

/**@enum      SpawnPass1
 * @brief     list of 1st pass tests for the spawn point candidate (static phase)
 **/
enum SpawnPass1
{
  SP1_InWaterTest = 0,    /// point is in water
  SP1_TooClose2Static,    /// distance from static geometry is less than SPT_minDist2Static
  SP1_TooSteep,           /// terrain is outside interval [SPT_minSteepness, SPT_maxSteepness]
  SP1_InsideTest,         /// point is inside building

  e_max_spawnpass1
};

/**@enum      SpawnPass2
 * @brief     list of 2nd pass tests for the spawn point candidate (static phase)
 **/
enum SpawnPass2
{
  SP2_Dist2Water,         /// rate distance to water
  SP2_Dist2Static,        /// rate distance to static geometry
  SP2_Steepness,          /// rate max slope of terrain

  e_max_spawnpass2
};


/**@class     SpawnPoint
 * @brief     information about potential spawn point
 **/
struct SpawnPoint
{
  Vector3 _x;
  bool  _pass1Values[e_max_spawnpass1];
  float _pass2Values[e_max_spawnpass2];
  float _pass2Scores[e_max_spawnpass2];
  float _score;

  SpawnPoint () { }
};
TypeIsSimpleZeroed(SpawnPoint);

struct Pass3 {
  Vector3 _x;
  float _score;
  float _staticScore;
  float _pass3Values[e_max_spawnpass3];           /// runtime rating values for pass3
  float _pass3Scores[e_max_spawnpass3];           /// runtime scores of pass3

  Pass3 () { memset(this, 0, sizeof(*this)); }
};
TypeIsSimpleZeroed(Pass3);

typedef AutoArray<Pass3>         pass3points_t;   /// score information for pass3 (run time)

struct SpawnPoints
{
  typedef AutoArray<Vector3>     spawnpoints_t;   /// raw grid spawn points
  typedef AutoArray<float>       spawnscores_t;   /// scores from static scan
  spawnpoints_t _spawnpoints;
  spawnscores_t _spawnscores;
  float _rms;
  unsigned _rmsindex;

  bool Load (SerializeBinStream & s)
  {
    _rms = s.LoadFloat();
    _rmsindex = s.LoadInt();
    const int sz = s.LoadInt();
    _spawnpoints.Realloc(sz);
    for (int i = 0; i < sz; ++i)
    {
      float x = 0.0f, y = 0.0f, z = 0.0f;
      s.LoadVector3(&x, &y, &z);
      _spawnpoints.Add(Vector3(x,y,z));
    }
    _spawnscores.Realloc(sz);
    for (int i = 0; i < sz; ++i)
    {
      const float x = s.LoadFloat();
      _spawnscores.Add(x);
    }
    return true;
  }

  bool Save (SerializeBinStream & str) const
  {
    str.SaveFloat(_rms);
    str.SaveInt(_rmsindex);
    const int sz = _spawnpoints.Size();
    str.SaveInt(sz);
    for (int i = 0; i < sz; ++i)
    {
      const Vector3 & v = _spawnpoints[i];
      str.SaveVector3(v[0], v[1], v[2]);
    }
    for (int i = 0; i < sz; ++i)
      str.SaveFloat(_spawnscores[i]);
    return true;
  }

};
TypeIsSimpleZeroed(SpawnPoints);

struct Areas
{
  typedef AutoArray<SpawnPoints> areaspoints_t;
  areaspoints_t _areaspoints;
  static const int s_iCONTENT_FILE_VERSION = 0x00000001;
  static const char * s_sBINPROXY_FILEHEADER;
  RString _filename;

  void Clear () { _areaspoints.Clear(); }
  size_t Size () const { return _areaspoints.Size(); }
  void AddArea () { _areaspoints.Add(SpawnPoints()); }
  SpawnPoints & LastArea () { return _areaspoints.Last(); }

  bool LoadFromFile (const char * name)
  {
    _filename = name;
    QIFStream in;
    in.open(name);
    if (in.fail())
    {
      RptF("!!! Failed to load spawn data from file \"%s\" - failed to open the file.", cc_cast(name));
      return false;
    }
    SerializeBinStream bs(&in);
    Load(bs);
    in.close();
    return true;
  }

  bool SaveToFile (const char * name) const
  {
    QOFStream out;
    out.open(name);
    if (out.fail())
    {
      RptF("!!! Failed to save spawn data to file \"%s\" - failed to open the file.", cc_cast(name));
      return false;
    }
    SerializeBinStream bs(&out);
    Save(bs);
    out.close();
    return true;
  }

  bool Load (SerializeBinStream & s)
  {
    const int ver = s.LoadInt();
    if (ver != s_iCONTENT_FILE_VERSION )
    {
      RptF("!!! Failed to load Spawn data file - version %d, expected %d... Please regenerate!",ver,s_iCONTENT_FILE_VERSION);
      return false;
    }
    const RString header = s.LoadString();
    if( !(header == s_sBINPROXY_FILEHEADER) )
    {
      RptF("!!! Failed to load Spawn data file - Invalid format!");
      return false;
    };

    const int sz = s.LoadInt();
    if (sz > 0)
    {
      _areaspoints.Realloc(sz);
      for (int i = 0; i < sz; ++i)
      {
        _areaspoints.Add();
        if (!_areaspoints.Last().Load(s))
          return false;
      };
    };
    return true;
  }

  void Save (SerializeBinStream & s) const
  {
    s.SaveInt(s_iCONTENT_FILE_VERSION);
    s.SaveString(s_sBINPROXY_FILEHEADER);
    const int sz = _areaspoints.Size();
    s.SaveInt(sz);
    for (int i = 0; i < sz; ++i)
      _areaspoints[i].Save(s);
  }
};
const char * Areas::s_sBINPROXY_FILEHEADER = "BINARIZED_SPAWNPOINT_EXPORT";
TypeIsSimpleZeroed(Areas);

AutoArray<Areas> g_CachedAreaFiles;

typedef AutoArray<SpawnPoint>     gridpoints_t;   /// raw grid spawn points
typedef AutoArray<SpawnPoint *>   gridptptrs_t;   /// pointers to candidate spawn points
typedef AutoArray<SpawnPoint *>   waterpoints_t;  /// pointers to points in water
namespace enf { TypeIsSimple(AAB); }
typedef AutoArray<enf::AAB>       aabbs_t; /// bounding boxes of the objects

// excluded geometry
struct ExcludedGeo
{
  aabbs_t _aabbs;
  static const int s_iCONTENT_FILE_VERSION = 0x00000001;
  static const char * s_sBINPROXY_FILEHEADER;
  RString _filename;

  void Clear () { _aabbs.Clear(); }
  size_t Size () const { return _aabbs.Size(); }

  bool LoadFromFile (const char * name)
  {
    _filename = name;
    QIFStream in;
    in.open(name);
    if (in.fail())
    {
      RptF("!!! Failed to load spawn data from file \"%s\" - failed to open the file.", cc_cast(name));
      return false;
    }
    SerializeBinStream bs(&in);
    Load(bs);
    in.close();
    return true;
  }

  bool SaveToFile (const char * name) const
  {
    QOFStream out;
    out.open(name);
    if (out.fail())
    {
      RptF("!!! Failed to save spawn data to file \"%s\" - failed to open the file.", cc_cast(name));
      return false;
    }
    SerializeBinStream bs(&out);
    Save(bs);
    out.close();
    return true;
  }

  bool Load (SerializeBinStream & s)
  {
    const int ver = s.LoadInt();
    if (ver != s_iCONTENT_FILE_VERSION )
    {
      RptF("!!! Failed to load Spawn data file - version %d, expected %d... Please regenerate!",ver,s_iCONTENT_FILE_VERSION);
      return false;
    }
    const RString header = s.LoadString();
    if( !(header == s_sBINPROXY_FILEHEADER) )
    {
      RptF("!!! Failed to load Spawn data file - Invalid format!");
      return false;
    };

    const int sz = s.LoadInt();
    if (sz > 0)
    {
      _aabbs.Realloc(sz);
      for (int i = 0; i < sz; ++i)
      {
        _aabbs.Add();
        enf::AAB & aabb = _aabbs.Last();
        s.LoadVector3(aabb.Mins);
        s.LoadVector3(aabb.Maxs);
        //if (!_aabbs.Last().Load(s))
          //return false;
      };
    };
    return true;
  }

  void Save (SerializeBinStream & s) const
  {
    s.SaveInt(s_iCONTENT_FILE_VERSION);
    s.SaveString(s_sBINPROXY_FILEHEADER);
    const int sz = _aabbs.Size();
    s.SaveInt(sz);
    for (int i = 0; i < sz; ++i)
    {
      s.SaveVector3(_aabbs[i].Mins);
      s.SaveVector3(_aabbs[i].Maxs);
    }
  }
};
const char * ExcludedGeo::s_sBINPROXY_FILEHEADER = "BINARIZED_SPAWNPOINT_EXPORT";
TypeIsSimpleZeroed(ExcludedGeo);


/**@class     InGameObjects
 * @brief     collection of in game objects like zombies, trees, buildings etc
 **/
struct InGameObjects
{
  AutoArray<InGameObject>   _objects;             /// game objects returned by QueryObjects
  AutoArray<InGameObject *> _zombies;
  AutoArray<InGameObject *> _players;
  AutoArray<InGameObject *> _statics;
  AutoArray<InGameObject *> _unknowns;
};

typedef AutoArray<Vector2>        hullpoints_t;

#if defined SPAWN_DEBUG
# include "spawn_debug.hpp"
  debugshapes_t g_dbg;
  std::vector<Object *> g_unknown_gots;
#endif

const float far_enough = 4096.0f;

namespace {
  // @FIXME: sorry i did not found matrix 4x4 anywhere
  struct ProxyMatrix44
  {
    const float * _coeff;

    ProxyMatrix44 (const float (&coeff)[16]) :_coeff(coeff) { }

    void Mul (const Vector4 & v, Vector4 & result) const
    {
      result = Vector4(0,0,0,0);
      for (size_t i = 0; i < 4; ++i)
        for (size_t j = 0; j < 4; ++j)
          result.v[i] += _coeff[i * 4 + j] * v[j];
    }
  };
}

size_t makeGrid (const Quad & quad, float density, Vector3 * buffer, size_t maxsz)
{
  Vector4 x(quad._x0[0], quad._x1[0], quad._x2[0], quad._x3[0]);
  Vector4 y(quad._x0[1], quad._x1[1], quad._x2[1], quad._x3[1]);

  const float coeff[] =
    { 1, 0, 0, 0,
     -1, 1, 0, 0,
     -1, 0, 0, 1,
      1,-1, 1,-1
    };
  const ProxyMatrix44 m44inv(coeff);

  Vector4 a, b;
  m44inv.Mul(x, a); // a = m44 * x;
  m44inv.Mul(y, b); // b = m44 * y;

  const float ii = density;
  const float jj = density;

  size_t n = 0;
  for (float i = 0.0f; i <= 1.0f; i += ii)
    for (float j = 0.0f; j <= 1.0f; j += jj)
    {
      const float gx = a[0] + a[1] * i + a[2] * j + a[3] * i * j;
      const float gz = b[0] + b[1] * i + b[2] * j + b[3] * i * j;
      if (n < maxsz)
      {
        const float h = GLandscape->SurfaceY(gx, gz);
        *(buffer + n++) = Vector3(gx, h, gz);
      }
      else
        return n;
    }
  return n;
}

namespace {
  float distPoint2Line (const Vector3 & pt, const Vector3 & l0, const Vector3 & l1)
  {
    const Vector3 v = l1 - l0;
    const Vector3 w = pt - l0;
    const float c1 = w.DotProduct(v);
    const float c2 = v.DotProduct(v);
    const float  b = c1 / c2;
    const Vector3 p1 = l0 + b * v;
    return (p1 - pt).Size();
  }
}

struct WaterInfo
{
  waterpoints_t _pts;
  hullpoints_t _hull;

  WaterInfo (size_t n)
  {
    _pts.Reserve(n);
    _hull.Reserve(8);
  }

  bool isThereAnyWaterOutThere()
  {
    return _hull.Size() > 0;
  }

  void guessInterface ()
  {
    if (const size_t n = _pts.Size())
    {
      if (n == 1)
      {
        _hull.Add(Vector2(_pts[0]->_x[0], _pts[0]->_x[2]));
        return;
      }
      else if (n == 2)
      {
        _hull.Add(Vector2(_pts[0]->_x[0], _pts[0]->_x[2]));
        _hull.Add(Vector2(_pts[1]->_x[0], _pts[1]->_x[2]));
        return;
      }

      VECT2 * pts = static_cast<VECT2 *>(alloca(n * sizeof(VECT2)));
      int * ind = static_cast<int *>(alloca(n * sizeof(int)));
      for (size_t i = 0, ie = _pts.Size(); i < ie; ++i)
      {
        pts[i][0] = _pts[i]->_x[0];
        pts[i][1] = _pts[i]->_x[2];
        ind[i] = i;
      }

      if (const size_t hull_size = Quick_Hull_2D(n, pts, ind))
      {
        _hull.Clear();
        _hull.Resize(hull_size);
        for (size_t i = 0; i < hull_size; ++i)
        {
          const VECT2 & h = pts[ind[i]];
          _hull[i] = Vector2(h[0], h[1]);
        }
      }
    }
#if defined SPAWN_DEBUG
    debugWater(g_dbg, _hull);
#endif
  }

  float distPt2HullEdge (const Vector3 & v, size_t i, size_t j) const
  {
    const Vector3 v0(_hull[i].x, GLandscape->SurfaceY(_hull[i].x, _hull[i].y), _hull[i].y);
    const Vector3 v1(_hull[j].x, GLandscape->SurfaceY(_hull[j].x, _hull[j].y), _hull[j].y);
    const float d = distPoint2Line(v, v0, v1);
    return d;
  }

  float distToInterface (const Vector3 & v) const
  {
    const size_t n = _hull.Size();
    if (n == 0)
      return far_enough;
    else if (n == 1)
    {
      const Vector3 h(_hull[0].x, GLandscape->SurfaceY(_hull[0].x, _hull[0].y), _hull[0].y);
      return (v - h).Size();
    }
    else if (n == 2)
    {
      return distPt2HullEdge(v, 0, 1);
    }
    else
    {
      float min_d = far_enough;
      for (size_t i = 0; i < n; ++i)
      {
        const size_t j = (i + 1) % n;
        const float d = distPt2HullEdge(v, i, j);
        if (d < min_d)
          min_d = d;
      }
      return min_d;
    }
  }
};

bool isZombie (const Object * obj)
{
  Ref<EntityType> type = VehicleTypes.New("zombieBase"); //@TODO: to member
  return obj->GetEntityType()->IsKindOf(type);
}

bool isPlayer (const Object * obj)
{
  if (const EntityAI * veh = dyn_cast<const EntityAI>(obj))
  {
    if (const AIBrain * commander = veh->CommanderUnit())
      return commander->IsAnyPlayer();
  }
  return false;
}

bool isStatic (const Object * obj)
{
  //@TODO: ptr form
  return obj->GetEntityType()->IsKindOf("Building") || obj->GetEntityType()->IsKindOf("StreetLamp");
}

bool isInside (const Vector3 & v, const enf::AAB & aabb)
{
  return aabb.Intersects(v);
}

float dist2Water (const Vector3 & v, const WaterInfo & wi)
{
  return wi.distToInterface(v);
}

template <class ContainerT>
float distToClosestObject (const Vector3 & v, const ContainerT & container)
{
  float min_d2 = far_enough * far_enough;
  for (size_t i = 0, ie = container.Size(); i < ie; ++i)
  {
    typename ContainerT::DataType item = container[i];
    if (item)
    {
      const Vector3 nearpt = item->_aabb.GetNearestPosition(v);
      const float d2 = (nearpt - v).SquareSize();
      if (d2 < min_d2)
        min_d2 = d2;
    }
  }
  return sqrtf(min_d2); //@FIXME: get rid of sqrt
}

float dist2Zombie (const Vector3 & v, const InGameObjects & go)
{
  return distToClosestObject(v, go._zombies);
}

float dist2Player (const Vector3 & v, const InGameObjects & go)
{
  return distToClosestObject(v, go._players);
}

float dist2Static (const Vector3 & v, const InGameObjects & go)
{
  return distToClosestObject(v, go._statics);
}

template <class ContainerT>
bool isInsideObject (const Vector3 & v, const ContainerT & container)
{
  for (size_t i = 0, ie = container.Size(); i < ie; ++i)
  {
    typename ContainerT::DataType item = container[i];
    if (item && isInside(v, item->_aabb))
        return true;
  }
  return false;
}

namespace {
  struct NearObj : public World::ObjectQuery
  {
    InGameObjects & _gameobjects;
    NearObj (InGameObjects & go) : _gameobjects(go) { }

    bool Check (Object * obj) const
    {
      const LODShape * shape = obj->GetShape();
      if (!shape)
        return false;

      const EntityType * type = obj->GetEntityType();
      if (!type)
      {
        const ObjectType t = obj->GetType();
        if (t == Network || t == TypeTempVehicle)
          return false;
#if _ARMA3_TERRAIN_DECALS
        if (t == LandDecal) return false;
#endif
        if (shape->FindGeometryLevel() < 0 || obj->GetRadius() < 0.5f)
          return false;

        if (const ObjectWindAnim * o = dyn_cast<const ObjectWindAnim>(obj))
          return false;
      }
      else
      {
        if (const ThingEffectLight * o = dyn_cast<const ThingEffectLight>(obj))
          return false;
        if (const SeaGull * o = dyn_cast<const SeaGull>(obj))
          return false;
      }

      _gameobjects._objects.Add(InGameObject());
      InGameObject & go = _gameobjects._objects.Last();
      const Matrix4 & m = obj->FutureVisualState();
      go._x = m.Position();

      enf::AAB aabb(true);
      aabb.Add(shape->Min());
      aabb.Add(shape->Max());

      const Vector3 c[] = {
          aabb.GetCornerBySigns(-1, -1, -1),
          aabb.GetCornerBySigns(+1, -1, -1),
          aabb.GetCornerBySigns(-1, +1, -1),
          aabb.GetCornerBySigns(+1, +1, -1),
          aabb.GetCornerBySigns(-1, -1, +1),
          aabb.GetCornerBySigns(+1, -1, +1),
          aabb.GetCornerBySigns(-1, +1, +1),
          aabb.GetCornerBySigns(+1, +1, +1)
      };

      enf::AAB aabb_w(true);
      for (size_t i = 0; i < 8; ++i)
      {
        const Vector3 & v = m * c[i];
        aabb_w.Add(v);
      }
      go._obj = obj;
      go._aabb = aabb_w;
     
#if defined SPAWN_DEBUG
      if (obj->GetEntityType() && obj->GetEntityType()->GetName())
        LogF("SPAWN: found object, name=%s", obj->GetEntityType()->GetName().Data());
      else
        LogF("SPAWN: some weird entity");
      g_dbg.Add(enf::Shape::CreateBox(aabb_w.GetMins(), aabb_w.GetMaxs(), 0xffff0000, enf::SS_WIREFRAME));
#endif

      if (isZombie(obj))
      {
        _gameobjects._zombies.Add(&_gameobjects._objects.Last());
        _gameobjects._objects.Last()._type = GOT_zombie;
      }
      else if (isPlayer(obj))
      {
        _gameobjects._players.Add(&_gameobjects._objects.Last());
        _gameobjects._objects.Last()._type = GOT_player;
      }
      else if (isStatic(obj))
      {
        _gameobjects._statics.Add(&_gameobjects._objects.Last());
        _gameobjects._objects.Last()._type = GOT_static;
      }
      else
      {
        _gameobjects._unknowns.Add(&_gameobjects._objects.Last());
        _gameobjects._objects.Last()._type = GOT_unknown;
#if defined SPAWN_DEBUG
        g_unknown_gots.push_back(obj);
#endif
      }
      return true;
    }

    bool operator() (Object * obj) const { return true; }
  };
}

bool scanEnv (const Quad & quad, InGameObjects & gameobjects, bool include_static)
{
  enf::AAB aabb(true);
  aabb.Add(Vector3(quad._x0.x, -FLT_MAX, quad._x0.y));
  aabb.Add(Vector3(quad._x1.x, -FLT_MAX, quad._x1.y));
  aabb.Add(Vector3(quad._x2.x, +FLT_MAX, quad._x2.y));
  aabb.Add(Vector3(quad._x3.x, +FLT_MAX, quad._x3.y));

  NearObj query(gameobjects);

#if defined SPAWN_DEBUG
  g_unknown_gots.clear();
  //g_dbg.Add(enf::Shape::CreateBox(aabb.GetMins(), aabb.GetMaxs(), enf::SS_WIREFRAME, 0));
#endif
  const unsigned flags = World::QF_DYNAMIC | (include_static ? World::QF_STATIC : 0);
  GWorld->QueryObjects(query, aabb.GetMins(), aabb.GetMaxs(), flags);
  return gameobjects._objects.Size() > 0;
}

bool pass3ScanEnv (const Vector3 & x, float d, InGameObjects & gameobjects)
{
  NearObj query(gameobjects);
  GWorld->QueryObjects(query, x, d, World::QF_DYNAMIC);
  return gameobjects._objects.Size() > 0;
}

bool scanPreloadedEnv (const Quad & quad, InGameObjects & gameobjects)
{
  enf::AAB aabb(true);
  aabb.Add(Vector3(quad._x0.x, -FLT_MAX, quad._x0.y));
  aabb.Add(Vector3(quad._x1.x, -FLT_MAX, quad._x1.y));
  aabb.Add(Vector3(quad._x2.x, +FLT_MAX, quad._x2.y));
  aabb.Add(Vector3(quad._x3.x, +FLT_MAX, quad._x3.y));

  NearObj query(gameobjects);

#if defined SPAWN_DEBUG
  g_unknown_gots.clear();
#endif
  GWorld->UseObjects(query, aabb.GetMins(), aabb.GetMaxs(), World::QF_DYNAMIC | World::QF_STATIC);
  return gameobjects._objects.Size() > 0;
}

inline bool isInRange (float value, float min, float max)
{
  return value <= max && value >= min;
}

void fillEnvInfo (gridpoints_t & gridpoints, float d2maxwater, const InGameObjects & gameobjects)
{
  size_t waterpts = 0;
  for (size_t i = 0, ie = gridpoints.Size(); i < ie; ++i)
  {
    SpawnPoint & pt = gridpoints[i];
    const Vector3 & x = pt._x;

    bool const in_water = IsInWater(x);
    if (in_water)
      waterpts++;
    pt._pass1Values[SP1_InWaterTest]      = in_water;
    const bool inside =  isInsideObject(x, gameobjects._statics) || isInsideObject(x, gameobjects._unknowns);
    pt._pass1Values[SP1_InsideTest]       = inside;
    pt._pass2Values[SP2_Dist2Static]      = dist2Static(x, gameobjects);
    pt._pass2Values[SP2_Steepness]        = MaxTerrainGradient(x, 1.0f);
  }

  WaterInfo wi(waterpts);
  if (waterpts)
  {
    for (size_t i = 0, ie = gridpoints.Size(); i < ie; ++i)
    {
      SpawnPoint & pt = gridpoints[i];
      if (pt._pass1Values[SP1_InWaterTest])
        wi._pts.Add(&gridpoints[i]);
    }
    wi.guessInterface();
  }

  for (size_t i = 0, ie = gridpoints.Size(); i < ie; ++i)
  {
    SpawnPoint & pt = gridpoints[i];
    const Vector3 & x = pt._x;

    if (pt._pass1Values[SP1_InWaterTest])
      pt._pass2Values[SP2_Dist2Water]     = -1.0f;
    else if (wi.isThereAnyWaterOutThere())
    {
      pt._pass2Values[SP2_Dist2Water]     = d2maxwater;
    }
    else
    {
      pt._pass2Values[SP2_Dist2Water]     = dist2Water(x, wi);
    }
    //LogF("SPAWN: dist from water interface[%i] = %3.2f", i, pt._pass2Values[SP2_Dist2Water]);
  }
}

void pass3FillEnvInfo (pass3points_t & pass3points, const InGameObjects & gameobjects)
{
  for (size_t i = 0, ie = pass3points.Size(); i < ie; ++i)
  {
    const Vector3 & x = pass3points[i]._x;
    pass3points[i]._pass3Values[SP3_Dist2Zombie]      = dist2Zombie(x, gameobjects);
    pass3points[i]._pass3Values[SP3_Dist2Player]      = dist2Player(x, gameobjects);
    pass3points[i]._pass3Values[SP3_Dist2Static]      = dist2Static(x, gameobjects);
  }
}

size_t pass1Evaluate (gridpoints_t & src, float min_dist2static, float min_steep, float max_steep, gridptptrs_t & dst)
{
  for (size_t i = 0, ie = src.Size(); i < ie; ++i)
  {
    SpawnPoint & pt = src[i];

    const bool too_close2static = pt._pass2Values[SP2_Dist2Static] <= min_dist2static;
    const bool too_steep        = !isInRange(pt._pass2Values[SP2_Steepness], min_steep, max_steep);

    pt._pass1Values[SP1_TooClose2Static]  = too_close2static;
    pt._pass1Values[SP1_TooSteep]         = too_steep;

    bool accept = true;
    for (size_t c = 0; c < e_max_spawnpass1; ++c)
      accept &= !pt._pass1Values[c];

    if (accept)
      dst.Add(&pt);
  }
  return dst.Size();
}

float rateDist2Water (const SpawnPoint & pt, float mindist, float maxdist)
{
  const float d = pt._pass2Values[SP2_Dist2Water];
  if (d <= 0.0f)
    return 0.0f;
  else if (d < mindist)
  {
    const float v = d / mindist;
    return v;
  }
  else if (d >= mindist && d <= maxdist)
  {
    return 1.0f;
  }
  else
  {
    const float v = 1.0f / exp(d - maxdist);
    return v;
  }
}

float rateDist2Anything (float d, float mindist, float maxdist)
{
  if (d < mindist)
    return 0.0f;
  else if (d >= mindist && d <= maxdist)
  {
    const float v = d / (maxdist - mindist);
    return v;
  }
  else
    return 1.0f;
}

float rateDist2Zombie (const Pass3 & pt, float mindist, float maxdist)
{
  const float d = pt._pass3Values[SP3_Dist2Zombie];
  return rateDist2Anything(d, mindist, maxdist);
}

float rateDist2Player (const Pass3 & pt, float mindist, float maxdist)
{
  const float d = pt._pass3Values[SP3_Dist2Player];
  return rateDist2Anything(d, mindist, maxdist);
}

float rateDist2Static (const SpawnPoint & pt, float mindist, float maxdist)
{
  const float d = pt._pass2Values[SP2_Dist2Static];
  return rateDist2Anything(d, mindist, maxdist);
}
float rateDist2Static (const Pass3 & pt, float mindist, float maxdist)
{
  const float d = pt._pass3Values[SP3_Dist2Static];
  return rateDist2Anything(d, mindist, maxdist);
}

float rateSteepness (const SpawnPoint & pt, float mindist, float maxdist)
{
  const float d = pt._pass2Values[SP2_Steepness];
  const float PI = 3.1415926535897323f; // @TODO: use arma internal
  return sin(2.0f * (d + (PI/4.0f)));
}

void pass2Evaluate (gridptptrs_t & src
    , float min_d2w, float max_d2w, float min_d2s, float max_d2s, float min_steep, float max_steep
    , gridptptrs_t & dst)
{
  float weight_sum = 0.0f;
  for (size_t s = 0; s < e_max_spawnpass2; ++s)
    weight_sum += 1.0f; // @TODO: spawnparams._pass2Weights[s];

  for (size_t i = 0, ie = src.Size(); i < ie; ++i)
  {
    SpawnPoint & pt = *src[i];
    pt._pass2Scores[SP2_Dist2Water]  = rateDist2Water(pt, min_d2w, max_d2w);
    pt._pass2Scores[SP2_Dist2Static] = rateDist2Static(pt, min_d2s, max_d2s);
    pt._pass2Scores[SP2_Steepness]   = rateSteepness(pt, min_steep, max_steep);

    float sum = 0.0f;
    for (size_t s = 0; s < e_max_spawnpass2; ++s)
      sum += pt._pass2Scores[s];

    const float score = sum / weight_sum;
    src[i]->_score = score;

    if (score > 0.0f)
      dst.Add(src[i]);
  }
}

void pass3Evaluate (pass3points_t & src, const SpawnPass3Params & spawnparams)
{
  float weight_sum = 0.0f;
  for (size_t s = 0; s < e_max_spawnpass3; ++s)
    weight_sum += spawnparams._pass3Weights[s];

  for (size_t i = 0, ie = src.Size(); i < ie; ++i)
  {
    Pass3 & pass3 = src[i];
    pass3._pass3Scores[SP3_Dist2Zombie] = rateDist2Zombie(pass3, spawnparams._params[S3T_minDist2Zombie], spawnparams._params[S3T_maxDist2Zombie]);
    pass3._pass3Scores[SP3_Dist2Player] = rateDist2Player(pass3, spawnparams._params[S3T_minDist2Player], spawnparams._params[S3T_maxDist2Player]);
    pass3._pass3Scores[SP3_Dist2Static] = rateDist2Static(pass3, spawnparams._params[S3T_minDist2Static], spawnparams._params[S3T_maxDist2Static]);

    float sum = 0.0f;
    for (size_t s = 0; s < e_max_spawnpass3; ++s)
      sum += pass3._pass3Scores[s];

    sum += src[i]._staticScore;

    const float score = sum / (weight_sum + 1.0f);
    src[i]._score = score;
  }
}

namespace {
  struct ScoreComparator
  {
    int operator() (Pass3 * lhs, Pass3 * rhs) const
    {
      const bool less = lhs->_score < rhs->_score;
      const bool more = lhs->_score > rhs->_score;
      return less ? -1 : (more ? +1 : 0);
    }
    int operator() (SpawnPoint * * lhs, SpawnPoint * * rhs) const
    {
      const bool less = (*lhs)->_score < (*rhs)->_score;
      const bool more = (*lhs)->_score > (*rhs)->_score;
      return less ? -1 : (more ? +1 : 0);
    }
  } g_Comparator;
}

bool generateSpawnPoints (const GeneratorParams & spawnparams, const Quad & quad, SpawnPoints & pts)
{
  enum { max_buff_sz = 4096 };
  Vector3 samples[max_buff_sz]; // @TODO: alloca to gridX*gridY
  const size_t npts = makeGrid(quad, spawnparams._params[GPT_gridDensity], samples, max_buff_sz);

  gridpoints_t gridpoints;
  gridpoints.Resize(npts);

  for (size_t i = 0; i < npts; ++i)
  {
    SpawnPoint & pt = gridpoints[i];
    pt._x = samples[i];
#if defined SPAWN_DEBUG
    g_dbg.Add(enf::Shape::CreateSphere(pt._x, 0.15f, 0xff333333, enf::SS_NOOUTLINE));
#endif
  }

#if defined SPAWN_DEBUG
  debugDraw(g_dbg, quad);
  LogF("SPAWN: quad = (%4.1f %4.1f) (%4.1f %4.1f) (%4.1f %4.1f) (%4.1f %4.1f)", quad._x0.x, quad._x0.y, quad._x1.x, quad._x1.y, quad._x2.x, quad._x2.y, quad._x3.x, quad._x3.y);
  LogF("SPAWN: sizeof(SpawnPoint)=%d", sizeof(SpawnPoint));
  LogF("SPAWN: sizeof(InGameObjects)=%d", sizeof(SpawnPoint));
  LogF("SPAWN: sizeof sample points: n=%d, mem=%d", gridpoints.Size(), gridpoints.Size() * sizeof(SpawnPoint));
#endif

  InGameObjects gameobjects;
  scanPreloadedEnv(quad, gameobjects);

  fillEnvInfo(gridpoints, spawnparams._params[GPT_maxDist2Water], gameobjects);

#if defined SPAWN_DEBUG
  debugDrawPass1(g_dbg, gridpoints);
  LogF("SPAWN: objects from query: n=%d, mem=%d", gameobjects._objects.Size(), gameobjects._objects.Size() * sizeof(InGameObject));
#endif

  gridptptrs_t gridpoints_passed1;
  gridpoints_passed1.Reserve(npts);
  const float min2s = spawnparams._params[GPT_minDist2Static];
  const float minS = spawnparams._params[GPT_minSteepness];
  const float maxS = spawnparams._params[GPT_maxSteepness];
  pass1Evaluate(gridpoints, min2s, minS, maxS, gridpoints_passed1);

#if defined SPAWN_DEBUG
  debugDrawPass1Result(g_dbg, gridpoints_passed1);
  LogF("SPAWN: points after pass1: n=%d, mem=%d", gridpoints_passed1.Size(), gridpoints_passed1.Size() * sizeof(SpawnPoint *));
#endif

  gridptptrs_t gridpoints_passed2;
  gridpoints_passed2.Reserve(gridpoints_passed1.Size());
  const float min2w = spawnparams._params[GPT_minDist2Water];
  const float max2w = spawnparams._params[GPT_maxDist2Water];
  const float max2s = spawnparams._params[GPT_maxDist2Static];
  pass2Evaluate(gridpoints_passed1, min2w, max2w, min2s, max2s, minS, maxS, gridpoints_passed2);

#if defined SPAWN_DEBUG
  LogF("SPAWN: points after pass2: n=%d, mem=%d", gridpoints_passed2.Size(), gridpoints_passed2.Size() * sizeof(SpawnPoint *));
  debugDrawPass2Result(g_dbg, gridpoints_passed2);
#endif

  if (const size_t max_n = gridpoints_passed2.Size())
  {
    QSort(gridpoints_passed2.Data(), gridpoints_passed2.Size(), g_Comparator);

    float sum2 = 0.0f;
    for (size_t i = 0; i < max_n; ++i)
      sum2 += gridpoints_passed2[i]->_score * gridpoints_passed2[i]->_score;
    const float rms = sqrt(sum2 / max_n);

    size_t rms_index = max_n - 1;
    for (size_t i = 0; i < max_n; ++i)
      if (gridpoints_passed2[i]->_score < rms)
        rms_index = i;

    pts._rms = rms;
    pts._rmsindex = rms_index;
    pts._spawnpoints.Reserve(max_n);
    pts._spawnscores.Reserve(max_n);
    for (size_t i = 0; i < max_n; ++i)
    {
      pts._spawnpoints.Add(gridpoints_passed2[i]->_x);
      pts._spawnscores.Add(gridpoints_passed2[i]->_score);
    }
    return true;
  }
  else
  {
    // error
    return false;
  }
}

/*bool generateExcludedGeo (const char * filename, const Quad & quad, aabbs_t & aabbs_out)
{
  InGameObjects gameobjects;
  scanPreloadedEnv(quad, gameobjects);

  aabbs_t aabbs;
  aabbs_t aabbs2;

  for (size_t i = 0, ie = gameobjects._statics.Size(); i < ie; ++i)
  {
    enf::AAB const & aabb = gameobjects._statics[i]->_aabb;
    aabbs2.Add(aabb);
  }

  aabbs_t aabbs3;
  for (size_t i = 0, ie = aabbs2.Size(); i < ie; ++i)
  {
    enf::AAB const & aabb = aabbs2[i];

    bool inside = false;
    for (size_t j = 0, je = aabbs2.Size(); j < je; ++j)
    {
      if (aabb.Inside(aabbs2[j]))
      {
        inside = true;
      }
    }

    if (!inside)
      aabbs3.Add(aabb);
  }

  std::vector<int> flags(aabbs3.Size());
  aabbs_t aabbs4;
  for (size_t i = 0, ie = aabbs3.Size(); i < ie; ++i)
  {
    enf::AAB aabb = aabbs3[i];

    if (flags[i] == 1)
      continue;

    bool merged = false;
    for (size_t j = 0, je = aabbs3.Size(); j < je; ++j)
    {
      if (flags[j] == 0 && (aabb.Intersects(aabbs3[j]) || aabb.Contacts(aabbs3[j])))
      {
        flags[j] = 1;
        aabb.Add(aabbs3[j]);
        merged = true;
      }
    }

    aabbs4.Add(aabb);
  }
  aabbs_out = aabbs4;

  return true;
}

///////////////////////////////////////////////////////////////////////////////
GameValue GenerateExcludedGeo (const GameState * state, GameValuePar op)
{
#if defined SPAWN_DEBUG
  g_dbg.Clear();
#endif
  const GameArrayType & ops = op;
  if (ops.Size() < 2)
  {
    state->SetError(EvalDim, ops.Size(), 2);
    return NOTHING;
  }

  if (!CheckType(state, ops[0], GameArray)) return NOTHING;
  if (!CheckType(state, ops[1], GameArray)) return NOTHING;

  // parse spawnparams first
  const GameArrayType & param = ops[1];
  RString filename = "excludegeo.table";
  if (!CheckSize(state, param, 1)) // + 1 for filename at the beginning
  {
    // will use defaults
  }
  else
  {
    if (!CheckType(state, param[0], GameString))
      return NOTHING;
    filename = param[0];
  }

  // parse area(s) coordinates
  const GameArrayType & rects = ops[0];
  const int n = rects.Size();
  if (n < 1)
  {
    state->SetError(EvalDim, n, 1);
    return NOTHING;
  }

  ExcludedGeo egs;
  for (size_t i = 0; i < n; ++i)
  {
    const int j = i;
    const GameArrayType & rect_j = rects[j];

    Quad q;
    if (rect_j.Size() == 4)   // argument is probably quad
    {
      const GameArrayType & q0 = rect_j[0];
      const GameArrayType & q1 = rect_j[1];
      const GameArrayType & q2 = rect_j[2];
      const GameArrayType & q3 = rect_j[3];

      if (q0.Size() != 3 || q1.Size() != 3 || q2.Size() != 3 || q3.Size() != 3)
      {
        state->SetError(EvalDim, 0, 3);
        return NOTHING;
      }

      // argument is quadrilateral
      q._x0 = Vector2(q0[0], q0[1]);
      q._x1 = Vector2(q1[0], q1[1]);
      q._x2 = Vector2(q2[0], q2[1]);
      q._x3 = Vector2(q3[0], q3[1]);
    }
    else if (rect_j.Size() > 1 && rect_j.Size() <= 3)   // argument is 2d or 3d point
    {
      if (rect_j.Size() != 3)
      {
        state->SetError(EvalDim, rect_j.Size(), 3);
        return NOTHING;
      }

      const float x = rect_j[0], y = rect_j[1];
      const float w = 256;
      const float h = 246;
      q._x0 = Vector2(x    , y    );
      q._x1 = Vector2(x    , y + h);
      q._x2 = Vector2(x + w, y + h);
      q._x3 = Vector2(x + w, y    );
    }
    else
    {
      state->SetError(EvalDim, rect_j.Size(), 3);
      return NOTHING;
    }

    generateExcludedGeo(filename, q, egs._aabbs);
  }

  egs.SaveToFile(filename);

  return NOTHING;
}*/

bool averageAndPickOne (pass3points_t & pass3, Vector3 & point)
{
  const size_t max_n = pass3.Size();
  QSort(pass3.Data(), max_n, g_Comparator);

  if (max_n <= 3)
  {
    point = pass3[max_n - 1]._x;
  }
  else
  {
    float sum2 = 0.0f;
    for (size_t i = 0; i < max_n; ++i)
      sum2 += pass3[i]._score * pass3[i]._score;
    const float rms = sqrt(sum2 / max_n);

    size_t rms_index = max_n - 1;
    for (size_t i = 0; i < max_n; ++i)
      if (pass3[i]._score < rms)
        rms_index = i;

    const size_t rnd_n = GRandGen.RandomInt(rms_index, max_n);
    point = pass3[rnd_n]._x;

#if defined SPAWN_DEBUG
    debugDrawPass3Result(g_dbg, pass3, rms_index);
    g_dbg.Add(enf::Shape::CreateSphere(point + Vector3(0,1,0), 0.25f, 0xff00ff00, 0));
    //LogF("SPAWN: points after pass3: n=%d", pass3.Size());
#endif
  }
  return true;
}

bool findCachedSpawnPoint (const SpawnPoints & pts, const SpawnPass3Params & spawnparams, Vector3 & point)
{
  const size_t n = pts._spawnpoints.Size();
  pass3points_t pass3;
  pass3.Realloc(n);

  Vector3 center(0.0f, 0.0f, 0.0f);
  for (size_t i = 0; i < n; ++i)
  {
    pass3.Add(Pass3());
    center += pass3[i]._x = pts._spawnpoints[i];
    pass3[i]._staticScore = pts._spawnscores[i];
  }
  center /= n;

  InGameObjects gameobjects;
  const float d = spawnparams._params[S3T_maxDist2Zombie];
  pass3ScanEnv(center, d, gameobjects);
  pass3FillEnvInfo(pass3, gameobjects);
  pass3Evaluate(pass3, spawnparams);

#if defined SPAWN_DEBUG
  debugDrawCachedResult(g_dbg, pts);
  LogF("SPAWN: points after pass3: n=%d", pass3.Size());
#endif

  if (const size_t max_n = pass3.Size())
  {
    averageAndPickOne(pass3, point);
    return true;
  }
  else
  {
    // error
    return false;
  }
}

bool findSpawnPointForNPC (const SpawnParamsNPC & spawnparams, const Quad & quad, Vector3 & point)
{
  enum { max_buff_sz = 1024 };
  Vector3 samples[max_buff_sz]; // @TODO: alloca to gridX*gridY
  const size_t npts = makeGrid(quad, spawnparams._params[SPN_gridDensity], samples, max_buff_sz);

  gridpoints_t gridpoints;
  gridpoints.Resize(npts);

  for (size_t i = 0; i < npts; ++i)
  {
    SpawnPoint & pt = gridpoints[i];
    pt._x = samples[i];
#if defined SPAWN_DEBUG
    g_dbg.Add(enf::Shape::CreateSphere(pt._x, 0.15f, 0xff333333, enf::SS_NOOUTLINE));
#endif
  }

#if defined SPAWN_DEBUG
  debugDraw(g_dbg, quad);
  LogF("SPAWN: quad = (%4.1f %4.1f) (%4.1f %4.1f) (%4.1f %4.1f) (%4.1f %4.1f)", quad._x0.x, quad._x0.y, quad._x1.x, quad._x1.y, quad._x2.x, quad._x2.y, quad._x3.x, quad._x3.y);
  LogF("SPAWN: sizeof(SpawnPoint)=%d", sizeof(SpawnPoint));
  LogF("SPAWN: sizeof(InGameObjects)=%d", sizeof(SpawnPoint));
  LogF("SPAWN: sizeof sample points: n=%d, mem=%d", gridpoints.Size(), gridpoints.Size() * sizeof(SpawnPoint));
#endif

  InGameObjects gameobjects;
  scanEnv(quad, gameobjects, true);

  fillEnvInfo(gridpoints, spawnparams._params[SPN_maxDist2Water], gameobjects);

#if defined SPAWN_DEBUG
  debugDrawPass1(g_dbg, gridpoints);
  LogF("SPAWN: objects from query: n=%d, mem=%d", gameobjects._objects.Size(), gameobjects._objects.Size() * sizeof(InGameObject));
#endif

  gridptptrs_t gridpoints_passed1;
  gridpoints_passed1.Reserve(npts);
  const float min2s = spawnparams._params[SPN_minDist2Static];
  const float minS = spawnparams._params[SPN_minSteepness];
  const float maxS = spawnparams._params[SPN_maxSteepness];
  pass1Evaluate(gridpoints, min2s, minS, maxS, gridpoints_passed1);

#if defined SPAWN_DEBUG
  debugDrawPass1Result(g_dbg, gridpoints_passed1);
  LogF("SPAWN: points after pass1: n=%d, mem=%d", gridpoints_passed1.Size(), gridpoints_passed1.Size() * sizeof(SpawnPoint *));
#endif

  gridptptrs_t gridpoints_passed2;
  gridpoints_passed2.Reserve(gridpoints_passed1.Size());
  const float min2w = spawnparams._params[SPN_minDist2Water];
  const float max2w = spawnparams._params[SPN_maxDist2Water];
  const float max2s = spawnparams._params[SPN_maxDist2Static];
  pass2Evaluate(gridpoints_passed1, min2w, max2w, min2s, max2s, minS, maxS, gridpoints_passed2);

#if defined SPAWN_DEBUG
  LogF("SPAWN: points after pass2: n=%d, mem=%d", gridpoints_passed2.Size(), gridpoints_passed2.Size() * sizeof(SpawnPoint *));
  debugDrawPass2Result(g_dbg, gridpoints_passed2);
#endif

  if (const size_t max_n = gridpoints_passed2.Size())
  {
    const size_t n = gridpoints_passed2.Size();
    pass3points_t pass3;
    pass3.Realloc(n);

    for (size_t i = 0; i < n; ++i)
    {
      pass3.Add(Pass3());
      pass3[i]._x = gridpoints_passed2[i]->_x;
      pass3[i]._staticScore = gridpoints_passed2[i]->_score;
    }

    pass3FillEnvInfo(pass3, gameobjects);
    SpawnPass3Params p3params;
    p3params._params[S3T_minDist2Zombie] = spawnparams._params[SPN_minDist2Zombie];
    p3params._params[S3T_maxDist2Zombie] = spawnparams._params[SPN_maxDist2Zombie];
    p3params._params[S3T_minDist2Player] = spawnparams._params[SPN_minDist2Player];
    p3params._params[S3T_maxDist2Player] = spawnparams._params[SPN_maxDist2Player];
    p3params._params[S3T_minDist2Static] = spawnparams._params[SPN_minDist2Static];
    p3params._params[S3T_maxDist2Static] = spawnparams._params[SPN_maxDist2Static];
    pass3Evaluate(pass3, p3params);

#if defined SPAWN_DEBUG
    //debugDrawCachedResult(g_dbg, pts);
    LogF("SPAWN: points after pass3: n=%d", pass3.Size());
#endif

    if (const size_t max_n = pass3.Size())
    {
      averageAndPickOne(pass3, point);
#if defined SPAWN_DEBUG
      g_dbg.Add(enf::Shape::CreateSphere(point + Vector3(0,1,0), 0.25f, 0xff00ff00, 0));
#endif
      return true;
    }
  }
  return false; // something poo-pooed
}

///////////////////////////////////////////////////////////////////////////////
GameValue RespawnDeadNPC (const GameState * state, GameValuePar op)
{
#if defined SPAWN_DEBUG
  g_dbg.Clear();
#endif
  const GameArrayType & ops = op;

  if (ops.Size() < 2)
  {
    state->SetError(EvalDim, ops.Size(), 2);
    return NOTHING;
  }

  if (!CheckType(state, ops[0], GameArray)) return NOTHING;
  if (!CheckType(state, ops[1], GameArray)) return NOTHING;

  const GameArrayType & dpos = ops[0];
  const int n = dpos.Size();
  if (dpos.Size() != 3)
  {
    state->SetError(EvalDim, n, 3);
    return NOTHING;
  }

  SpawnParamsNPC spawnparams;
  const GameArrayType & param = ops[1];
  if (!CheckSize(state, param, e_max_spawnparamsnpctypes))
  {
    // will use defaults
  }
  else
  {
    for (size_t i = 0; i < e_max_spawnparamsnpctypes; ++i)
    {
      if (!CheckType(state, param[i], GameScalar))
        return NOTHING;
      spawnparams._params[i] = param[i];
    }
  }

  const Vector3 dpt(dpos[0], dpos[1], dpos[2]);
  const Vector3 backup_spawnpoint(dpos[0], dpos[1], dpos[2]);

  enum { max_buff_sz = 1024 };
  Vector3 samples[max_buff_sz]; // @TODO: alloca to gridX*gridY

  Quad quad;
  const float x = dpos[0], y = dpos[1];
  const float w = spawnparams._params[SPN_gridWidth];
  const float h = spawnparams._params[SPN_gridHeight];
  quad._x0 = Vector2(x - w/2.0f , y - h/2.0f);
  quad._x1 = Vector2(x - w/2.0f , y + h/2.0f);
  quad._x2 = Vector2(x + w/2.0f , y + h/2.0f);
  quad._x3 = Vector2(x + w/2.0f , y - h/2.0f);

  Vector3 pos(dpt);
  const bool pos_found = findSpawnPointForNPC(spawnparams, quad, pos);

  GameValue result = state->CreateGameValue(GameArray);
  GameArrayType & arrayOut = result;
  arrayOut.Resize(3);

  if (pos_found)
  {
    arrayOut[0] = pos[0];
    arrayOut[1] = pos[2];
    arrayOut[2] = pos[1];
  }
  else // epic fail
  {
    arrayOut[0] = backup_spawnpoint[0];
    arrayOut[1] = backup_spawnpoint[1];
    arrayOut[2] = backup_spawnpoint[2];
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
/*GameValue FindSpawnPoint (const GameState * state, GameValuePar op)
{
#if defined SPAWN_DEBUG
  g_dbg.Clear();
#endif
  const GameArrayType & ops = op;
  if (ops.Size() < 2)
  {
    state->SetError(EvalDim, ops.Size(), 2);
    return NOTHING;
  }

  if (!CheckType(state, ops[0], GameArray)) return NOTHING;
  if (!CheckType(state, ops[1], GameArray)) return NOTHING;

  // parse spawnparams first
  SpawnParams spawnparams;
  const GameArrayType & param = ops[1];
  if (!CheckSize(state, param, e_max_spawnparamstypes))
  {
    // will use defaults
  }
  else
  {
    for (size_t i = 0; i < e_max_spawnparamstypes; ++i)
    {
      if (!CheckType(state, param[i], GameScalar))
        return NOTHING;
      spawnparams._params[i] = param[i];
    }
  }

  // parse area(s) coordinates
  const GameArrayType & rects = ops[0];
  const int n = rects.Size();
  if (n < 1)
  {
    state->SetError(EvalDim, n, 1);
    return NOTHING;
  }

  unsigned * lottery = static_cast<unsigned *>(alloca(n * sizeof(unsigned)));
  memset(lottery, 0, n * sizeof(unsigned));
  for (size_t i = 0; i < n; ++i)
    lottery[i] = i;
  std::random_shuffle(lottery, lottery + n);
  
  Vector3 backup_spawnpoint(0,0,0);
  // try to spawn in first random area
  Vector3 pos(0, 0, 0);
  bool pos_found = false;
  for (size_t i = 0; i < n; ++i)
  {
#if defined SPAWN_DEBUG
    const int j = i;
#else
    const int j = lottery[i];
#endif

    LogF("SPAWN: lottery: %i/%i", j / n);
    const GameArrayType & rect_j = rects[j];

    Quad q;
    if (rect_j.Size() == 4)   // argument is probably quad
    {
      const GameArrayType & q0 = rect_j[0];
      const GameArrayType & q1 = rect_j[1];
      const GameArrayType & q2 = rect_j[2];
      const GameArrayType & q3 = rect_j[3];

      if (q0.Size() != 3 || q1.Size() != 3 || q2.Size() != 3 || q3.Size() != 3)
      {
        state->SetError(EvalDim, 0, 3);
        return NOTHING;
      }

      // argument is quadrilateral
      q._x0 = Vector2(q0[0], q0[1]);
      q._x1 = Vector2(q1[0], q1[1]);
      q._x2 = Vector2(q2[0], q2[1]);
      q._x3 = Vector2(q3[0], q3[1]);
      backup_spawnpoint = Vector3(q0[0], q0[1], q0[2]);
    }
    else if (rect_j.Size() > 1 && rect_j.Size() <= 3)   // argument is 2d or 3d point
    {
      if (rect_j.Size() != 3)
      {
        state->SetError(EvalDim, rect_j.Size(), 3);
        return NOTHING;
      }

      const float x = rect_j[0], y = rect_j[1];
      const float w = spawnparams._params[SPT_gridWidth];
      const float h = spawnparams._params[SPT_gridHeight];
      q._x0 = Vector2(x    , y    );
      q._x1 = Vector2(x    , y + h);
      q._x2 = Vector2(x + w, y + h);
      q._x3 = Vector2(x + w, y    );
      backup_spawnpoint = Vector3(x, y, rect_j[2]);
    }
    else
    {
      state->SetError(EvalDim, rect_j.Size(), 3);
      return NOTHING;
    }

    if (pos_found |= findSpawnPoint(spawnparams, q, pos))
      break;
  }

  GameValue result = state->CreateGameValue(GameArray);
  GameArrayType & arrayOut = result;
  arrayOut.Resize(3);

  if (pos_found)
  {
    arrayOut[0] = pos[0];
    arrayOut[1] = pos[2];
    arrayOut[2] = pos[1];
  }
  else // epic fail
  {
    arrayOut[0] = backup_spawnpoint[0];
    arrayOut[1] = backup_spawnpoint[1];
    arrayOut[2] = backup_spawnpoint[2];
  }
  return result;
}*/

///////////////////////////////////////////////////////////////////////////////
GameValue FindCachedSpawnPoint (const GameState * state, GameValuePar op)
{
#if defined SPAWN_DEBUG
  g_dbg.Clear();
#endif
  const GameArrayType & ops = op;

  if (ops.Size() < 2)
  {
    state->SetError(EvalDim, ops.Size(), 2);
    return NOTHING;
  }

  if (!CheckType(state, ops[0], GameString)) return NOTHING;
  if (!CheckType(state, ops[1], GameArray)) return NOTHING;

  const GameStringType & str = ops[0];
  if (!CheckType(state, str, GameString))
    return NOTHING;
  RString filename(str);

  // parse spawnparams first
  SpawnPass3Params spawnparams3;
  const GameArrayType & param = ops[1];
  if (!CheckSize(state, param, e_max_spawnpass3paramstypes))
  {
    // will use defaults
  }
  else
  {
    for (size_t i = 0; i < e_max_spawnpass3paramstypes; ++i)
    {
      if (!CheckType(state, param[i], GameScalar))
        return NOTHING;
      spawnparams3._params[i] = param[i];
    }
  }

  Areas * areas = 0;
  for (size_t i = 0, ie = g_CachedAreaFiles.Size(); i < ie; ++i)
    if (g_CachedAreaFiles[i]._filename == filename)
    {
      areas = &g_CachedAreaFiles[i];
      break;
    }

  if (!areas)
  {
    g_CachedAreaFiles.Add();
    areas = &g_CachedAreaFiles.Last();
    areas->LoadFromFile(filename);
  }

  const int n = areas->Size();
  if (n < 1)
  {
    state->SetError(EvalDim, n, 1);
    return NOTHING;
  }

  unsigned * lottery = static_cast<unsigned *>(alloca(n * sizeof(unsigned)));
  memset(lottery, 0, n * sizeof(unsigned));
  for (size_t i = 0; i < n; ++i)
    lottery[i] = i;
  std::random_shuffle(lottery, lottery + n, [](int n) { return GRandGen.RandomInt(0, n); });

  Vector3 backup_spawnpoint(0,0,0);
  // try to spawn in first random area
  Vector3 pos(0, 0, 0);
  bool pos_found = false;
  for (size_t i = 0; i < n; ++i)
  {
#if defined SPAWN_DEBUG
    //const int j = i;
    static int k = 0;
    static int l = 0;
    //if (++l % 2)
    if (l)
      k++;
    const int j = k % n;
    //const int j = 0;
    //LogF("SPAWN: lottery[%d of %d]= %d", i, n, j);
#else
    const int j = lottery[i];
    LogF("SPAWN: lottery: %i/%i -> %i", i, n, j);
#endif
    if (pos_found |= findCachedSpawnPoint(areas->_areaspoints[j], spawnparams3, pos))
      break;
  }

  GameValue result = state->CreateGameValue(GameArray);
  GameArrayType & arrayOut = result;
  arrayOut.Resize(3);

  if (pos_found)
  {
    arrayOut[0] = pos[0];
    arrayOut[1] = pos[2];
    arrayOut[2] = pos[1];
  }
  else // epic fail
  {
    arrayOut[0] = backup_spawnpoint[0];
    arrayOut[1] = backup_spawnpoint[1];
    arrayOut[2] = backup_spawnpoint[2];
  }
  return result;
}


///////////////////////////////////////////////////////////////////////////////
GameValue GenerateSpawnPoints (const GameState * state, GameValuePar op)
{
#if defined SPAWN_DEBUG
  g_dbg.Clear();
#endif
  const GameArrayType & ops = op;
  if (ops.Size() < 2)
  {
    state->SetError(EvalDim, ops.Size(), 2);
    return NOTHING;
  }

  if (!CheckType(state, ops[0], GameArray)) return NOTHING;
  if (!CheckType(state, ops[1], GameArray)) return NOTHING;

  // parse spawnparams first
  GeneratorParams spawnparams;
  const GameArrayType & param = ops[1];
  RString filename = "spawnpoints.table";
  if (!CheckSize(state, param, e_max_generatorparamstypes + 1)) // + 1 for filename at the beginning
  {
    // will use defaults
  }
  else
  {
    if (!CheckType(state, param[0], GameString))
      return NOTHING;
    filename = param[0];
    for (size_t i = 0; i < e_max_generatorparamstypes; ++i)
    {
      if (!CheckType(state, param[i + 1], GameScalar))
        return NOTHING;
      spawnparams._params[i] = param[i + 1];
    }
  }

  // parse area(s) coordinates
  const GameArrayType & rects = ops[0];
  const int n = rects.Size();
  if (n < 1)
  {
    state->SetError(EvalDim, n, 1);
    return NOTHING;
  }

  Areas ars;
  for (size_t i = 0; i < n; ++i)
  {
    const int j = i;
    const GameArrayType & rect_j = rects[j];

    Quad q;
    if (rect_j.Size() == 4)   // argument is probably quad
    {
      const GameArrayType & q0 = rect_j[0];
      const GameArrayType & q1 = rect_j[1];
      const GameArrayType & q2 = rect_j[2];
      const GameArrayType & q3 = rect_j[3];

      if (q0.Size() != 3 || q1.Size() != 3 || q2.Size() != 3 || q3.Size() != 3)
      {
        state->SetError(EvalDim, 0, 3);
        return NOTHING;
      }

      // argument is quadrilateral
      q._x0 = Vector2(q0[0], q0[1]);
      q._x1 = Vector2(q1[0], q1[1]);
      q._x2 = Vector2(q2[0], q2[1]);
      q._x3 = Vector2(q3[0], q3[1]);
      //backup_spawnpoint = Vector3(q0[0], q0[1], q0[2]);
    }
    else if (rect_j.Size() > 1 && rect_j.Size() <= 3)   // argument is 2d or 3d point
    {
      if (rect_j.Size() != 3)
      {
        state->SetError(EvalDim, rect_j.Size(), 3);
        return NOTHING;
      }

      const float x = rect_j[0], y = rect_j[1];
      const float w = spawnparams._params[GPT_gridWidth];
      const float h = spawnparams._params[GPT_gridHeight];
      q._x0 = Vector2(x    , y    );
      q._x1 = Vector2(x    , y + h);
      q._x2 = Vector2(x + w, y + h);
      q._x3 = Vector2(x + w, y    );
      //backup_spawnpoint = Vector3(x, y, rect_j[2]);
    }
    else
    {
      state->SetError(EvalDim, rect_j.Size(), 3);
      return NOTHING;
    }

    ars.AddArea();
    generateSpawnPoints(spawnparams, q, ars.LastArea());
  }

  ars.SaveToFile(filename);

  return NOTHING;
}

#if defined SPAWN_DEBUG
# undef SPAWN_DEBUG
#endif

