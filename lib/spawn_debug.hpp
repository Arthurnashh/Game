#pragma once

#if defined SPAWN_DEBUG

typedef AutoArray<Ref<enf::Shape> > debugshapes_t;

inline void debugDraw (debugshapes_t & dbg, const Quad & q)
{
  Vector2 v[] = { q._x0, q._x1, q._x2, q._x3 };

  for (int i = 0; i < 4; ++i)
  {
    const int j = (i + 1) % 4;
    const Vector3 v0(v[i].x, GLandscape->SurfaceY(v[i].x, v[i].y), v[i].y);
    const Vector3 v1(v[j].x, GLandscape->SurfaceY(v[j].x, v[j].y), v[j].y);
    dbg.Add(enf::Shape::CreateLine(v0, v1, 0xffffffff, enf::SS_NOZBUFFER | enf::SS_NOCULL));
  }
  const Vector3 v0(v[0].x, GLandscape->SurfaceY(v[0].x, v[0].y), v[0].y);
  dbg.Add(enf::Shape::CreateSphere(v0, 0.5f, 0xff00ffff, enf::SS_NOZBUFFER | enf::SS_NOCULL | enf::SS_NOOUTLINE));
}

inline void debugDrawPass1 (debugshapes_t & dbg, const gridpoints_t & gridpoints)
{
  for (size_t i = 0, ie = gridpoints.Size(); i < ie; ++i)
  {
    const SpawnPoint & pt = gridpoints[i];

    unsigned color = 0xff000000;
    if (pt._pass1Values[SP1_InWaterTest] == 1)
      color = 0xff0000ff;
    if (pt._pass1Values[SP1_InsideTest] == 1)
      color = 0xffff0000;

    dbg.Add(enf::Shape::CreateSphere(pt._x + Vector3(0,0.2f,0), 0.15f, color, enf::SS_NOZBUFFER | enf::SS_NOCULL | enf::SS_NOOUTLINE));
  }
}

inline void debugDrawPass1Result (debugshapes_t & dbg, const gridptptrs_t & gridpoints)
{
  for (size_t i = 0, ie = gridpoints.Size(); i < ie; ++i)
  {
    const SpawnPoint * pt = gridpoints[i];

    const unsigned color = 0xff007700;
    dbg.Add(enf::Shape::CreateSphere(pt->_x + Vector3(0,0.4f,0), 0.15f, color, enf::SS_NOOUTLINE));
  }
}

inline void debugDrawPass2Result (debugshapes_t & dbg, const gridptptrs_t & gridpoints)
{
  for (size_t i = 0, ie = gridpoints.Size(); i < ie; ++i)
  {
    const SpawnPoint & pt = *gridpoints[i];

    unsigned color = 0xfff0f00f;
    if (pt._score < 0.5f)
      color = 0xff555500;
    else if (pt._score < 0.75)
      color = 0xffffff00;
    else if (pt._score < 1.0f)
    {
      const float dt = 1.0f - 0.75f;
      const float col = (pt._score - 0.75f) / dt * 255.0f;
      color = 0xff000000 + (static_cast<int>(col) << 8);
    }

    dbg.Add(enf::Shape::CreateSphere(pt._x + Vector3(0,0.65f,0), 0.25f, color, enf::SS_NOOUTLINE));
    //LogF("SPAWN: pt[%i] has score = %1.3f", i, pt._score);
  }
}

inline void debugDrawPass3Result (debugshapes_t & dbg, const pass3points_t & points, size_t rms_index)
{
  for (size_t i = 0, ie = points.Size(); i < ie; ++i)
  {
    const Pass3 & pt = points[i];

    unsigned color = 0xfff0000f;
    if (i < rms_index)
      color = 0xff000000;
    else
    {
      color = 0xffffffff;
      const int dt = ie - rms_index;
      if (dt != 0)
      {
        const float col = static_cast<float>(i - rms_index) / static_cast<float>(dt) * 255.0f;
        color = 0xff000000 + (static_cast<int>(col) << 8);
      }
    }
    dbg.Add(enf::Shape::CreateSphere(pt._x + Vector3(0,1.0f,0), 0.25f, color, enf::SS_NOOUTLINE));
    //LogF("SPAWN: pt[%i] has score = %1.3f", i, pt._score);
  }
}

inline void debugDrawCachedResult (debugshapes_t & dbg, const SpawnPoints & spawnpoints)
{
  for (size_t i = 0, ie = spawnpoints._spawnpoints.Size(); i < ie; ++i)
  {
    const Vector3 & pt = spawnpoints._spawnpoints[i];

    unsigned color = 0xfff0000f;
    if (i < spawnpoints._rmsindex)
      color = 0xff555500;
    else
    {
      color = 0xff00ff00;
      const int dt = ie - spawnpoints._rmsindex;
      if (dt != 0)
      {
        const float col = static_cast<float>(i - spawnpoints._rmsindex) / static_cast<float>(dt) * 255.0f;
        color = 0xff000000 + (static_cast<int>(col) << 8);
      }
    }
    dbg.Add(enf::Shape::CreateSphere(pt + Vector3(0,0.5f,0), 0.25f, color, enf::SS_NOOUTLINE));
  }
}

inline void debugWater (debugshapes_t & dbg, const hullpoints_t & pts)
{
  for (int i = 0, ie = pts.Size(); i < ie; ++i)
  {
    const int j = (i + 1) % ie;
    const Vector3 v0(pts[i].x, GLandscape->SurfaceY(pts[i].x, pts[i].y), pts[i].y);
    const Vector3 v1(pts[j].x, GLandscape->SurfaceY(pts[j].x, pts[j].y), pts[j].y);
    dbg.Add(enf::Shape::CreateSphere(v0 + Vector3(0,0.125f,0), 0.25f, 0xff0000ff, enf::SS_NOOUTLINE));
    dbg.Add(enf::Shape::CreateLine(v0, v1, 0xff0000ff, enf::SS_NOZBUFFER | enf::SS_NOCULL));
  }
}

#endif

