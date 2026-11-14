#include "scenery.hpp"

#include "lib/scene.hpp"
#include "lib/object.hpp"

#include "Common\enf_mathlib.h"
#include "Common\enf_mathlib2d.h"

class TempTerrainVehicle: public Entity
{
  public:
  TempTerrainVehicle(LODShape *shape)
  :Entity(shape,VehicleTypes.New("temp"),CreateObjectId())
  {}

  void DrawSimpleInstanced(int,int,unsigned int,struct DrawParameters const &,struct ObjectInstanceInfo *,int,float,class LightList const &,class SortObject *,float){}
  void DrawShadowVolume(int,class LODShape *,int,struct PositionRender const &,class SortObject *){}
  void Draw(int,int,unsigned int,struct DrawParameters const &,struct InstanceParameters const &,float,struct PositionRender const &,class SortObject *){}
};

Scenery::Scenery(LODShape *shape, const CreateObjectId &id):
Object(shape, id),
_geography(32, 32, 0),
_lodBias(0),
_elevationOffset(0),
_tex(32, 32, 0)
{
  RawType raw = HeightToRaw(-5);
  for (int z=0; z<_data.GetYRange(); z++) for (int x=0; x<_data.GetXRange(); x++)
    _data(x,z) = raw;
}

//-----------------------------------------------------------------------
/*!
test one terrain triangle, start is moved relatively to startx and starty coordinates

     V11 (1,1,h11)
     /|
 e3 / | e2
   /  |
  /   |
 /____|
V00   V10
    e1
*/
//-----------------------------------------------------------------------
__forceinline bool TestTerrainTriangleRB (const Vector3 &start, const Vector3 &dir, const float h00, const float h10, const float h11, Vector3 &inters, Vector3 &normal, float &d, float epsilon)
{
	//edges e1 = (1, 0, h10 - h00), e2 = (0, 1, h11 - h10), e3 (-1, -1, h00 - h11)
	//get normal of plane
	normal = Vector3((h00 - h10), 1.0, (h10 - h11));
  
	//with V0 = ( 0, 0, h00 ) = h00*n.z
	d = normal.Y() * (h00);

	//have now plane equation ro(X) = n*X + d = 0, compute the intersection with the line p(t) = start +dir*t
	float denominator = normal.DotProduct(dir);

	//line and plane are parallel
	if (fabs(denominator) < (float) 0.000001)
	{
		return false;
	}

	float t = (d - normal.DotProduct(start))/denominator;
	inters = start + dir*t;

	//now test if the intersection is inside the triangle, for terrain the test can be simplified
	//into 2D space because the triangles from heightmap cannot overlap and ortho projection always exists
	//three distance from edge and intersection point, all must have the same sign, this can be simplified
	//into following conditions
	//if (inters.x < inters.y || inters.y < 0.0 || inters.x > 1.0)

	//use version with epsilon
	if (inters.X() + epsilon < inters.Z() || inters.Z() < -epsilon || inters.X() > 1.0 + epsilon)
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------
/*!
test one terrain triangle, start is moved relatively to startx and starty coordinates

V01  e2   V11 (1,1,h11)
   ______
   |    /
   |   /
e1 |  /  e3
   | /
	|/
V00   
    
*/
//-----------------------------------------------------------------------
__forceinline bool TestTerrainTriangleLU (const Vector3 &start, const Vector3 &dir, float h00, float h11, float h01, Vector3 &inters, Vector3 &normal, float &d, float epsilon)
{
	//edges e1 = (0, h01 - h00, 1), e2 = (1, h11 - h01, 0), e3 (-1, h00 - h11, -1)
	//get normal of plane
	normal = Vector3((h01 - h11), 1.0, (h00 - h01));
  
	//with V0 = ( 0, 0, h00 ) = h00*n.z
	d = normal.Y() * (h00);

	//have now plane equation ro(X) = n*X + d = 0, compute the intersection with the line p(t) = start +dir*t
  float denominator = normal.DotProduct(dir);

	//line and plane are paralel
	if (abs(denominator) < (float) 0.000001f)
	{
		return false;
	}

	float t = (d - normal.DotProduct(start))/denominator;
	inters = start + dir*t;

	//now test if the intersection is inside the triangle, for terrain the test can be simplified
	//into 2D space because the triangles from heightmap cannot overlap and ortho projection always exists
	//three distance from edge and intersection point, all must have the same sign, this can be simplified
	//into following conditions
	//if (inters.x > inters.y || inters.y > 1.0 || inters.x < 0.0)

	//but due to numeric errors, use version with epsilon
	if (inters.X() > inters.Z() + epsilon || inters.Z() > 1.0 + epsilon || inters.X() < - epsilon)
  {
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
/*!
return ratio of intersection of a plane defined with normal and distance and the 
points of line start and end. If the result is <0..1> then the line intersects
the triangle and the intersection is start + (end - start)*ratio
*/
//-----------------------------------------------------------------------------
__forceinline bool GetPlaneLineRatio(const Vector3& normal, const float minus_d, const Vector3 &start, const Vector3& end, float& ratio)
{
	float tstart	= normal.DotProduct(start) - minus_d;
	float tend		= normal.DotProduct(end) - minus_d;

	float diff		=	tstart - tend;

	if (fabs(diff) < 0.00001f)
	{
		ratio = 0.0f;
		return true;
	}

	ratio = tstart/diff;
	//return ratio > -0.00001 && ratio < 1.00001 ? true : false;
  return ratio > -0.00001 && ratio < 1.00001 ? true : false;
}

//-----------------------------------------------------------------------
/*!
test one terrain quad for intersection
*/
//-----------------------------------------------------------------------
TerrainIntersection Scenery::TestTerrainQuadSimple(const Vector3 &start, const Vector3 &end, IntersectedTriangle &inters, unsigned int startx, unsigned int starty, float epsilon) const
{
	//Assert(startx < (unsigned int) m_iWidth && starty < (unsigned int) m_iHeight);

	//for test which triangle to use
	float sx = (float) startx;// * _terrainGrid;
	float sz = (float) starty;// * _terrainGrid;;
	
	enf::Vector2 offsstart ((start.X() - sx), (start.Z() - sz));
	enf::Vector2 offsend ((end.X() - sx), (end.Z() - sz));

	float dstart	= offsstart.x - offsstart.y;
	float dend		= offsend.x - offsend.y; 

	//heights in the diagonal
	//const TerrainHeight *src = m_pHeightMap + startx + starty*m_iWidth;
  float h00 = GetData(startx, starty);
	float h11 = GetData(startx + 1, starty + 1);

	//relative start and end of line
  Vector3 relstart (offsstart.x, (start.Y()), offsstart.y);  
	Vector3 relend (end.X() - sx, (end.Y()), end.Z() - sz);

	//direction vector
  Vector3 dir(end.X() - start.X(), (end.Y() - start.Y()), end.Z() - start.Z());

	//if intersection is in both parts
	if (dstart*dend < 0.0f)
	{
		//for output
		float d1 = 0.0f, d2 = 0.0f, ratio1 = 0.0f, ratio2 = 0.0f;
		Vector3 I1, I2, normal1, normal2;

		float h10 = GetData(startx + 1, starty);
		float h01 = GetData(startx, starty + 1);
		
		bool res1 = TestTerrainTriangleRB(relstart, dir, h00, h10, h11, I1, normal1, d1, epsilon);
		bool res2 = TestTerrainTriangleLU(relstart, dir, h00, h11, h01, I2, normal2, d2, epsilon);

		res1 = (res1 && GetPlaneLineRatio(normal1, d1, relstart, relend, ratio1)) ? true : false;
		res2 = (res2 && GetPlaneLineRatio(normal2, d2, relstart, relend, ratio2)) ? true : false;

		if (res1)
		{
			//if both are intersected, choose the closest one
			if (res2 && ratio2 < ratio1)
			{
        inters.Save(startx, starty, Vector3 (I2.X() + sx, (I2.Y()), I2.Z() + sz), Vector3(startx, h00, starty),	Vector3(startx, h01, starty + 1), Vector3(startx + 1, h11, starty + 1), Vector3(normal2.X(), normal2.Y(), normal2.Z()), ratio2);
				return INTERSECTION_QUAD;//IsHoleTC(start.x, start.y) ? INTERSECTION_HOLE : INTERSECTION_QUAD;
			}

			inters.Save(startx, starty, Vector3 (I1.X() + sx, (I1.Y()), I1.Z() + sz), Vector3(startx, h00, starty), Vector3(startx + 1, h11, starty + 1), Vector3(startx + 1, h10, starty), Vector3(normal1.X(), normal1.Y(), normal1.Z()), ratio1);
			return INTERSECTION_QUAD;//IsHoleTC(start.x, start.y) ? INTERSECTION_HOLE : INTERSECTION_QUAD;
		}

		if (res2)
		{
			inters.Save(startx, starty, Vector3 (I1.X() + sx, (I1.Y()), I1.Z() + sz), Vector3(startx, h00, starty), Vector3(startx + 1, h11, starty + 1), Vector3(startx + 1, h10, starty), Vector3(normal1.X(), normal1.Y(), normal1.Z()), ratio1);
			return INTERSECTION_QUAD;//IsHoleTC(start.x, start.y) ? INTERSECTION_HOLE : INTERSECTION_QUAD;
		}
	} else
	{
		Vector3 I, normal;
		float d, ratio;

		if (dstart > 0.0f && dend > 0.0f)
		{
			//test right bottom triangle
			float h10 = GetData(startx + 1, starty);

			if (TestTerrainTriangleRB(relstart, dir, h00, h10, h11, I, normal, d, epsilon))
			{
				if (GetPlaneLineRatio(normal, d, relstart, relend, ratio))
				{
					inters.Save(startx, starty, Vector3(I.X() + sx, (I.Y()), I.Z() + sz), Vector3(startx, h00, starty), Vector3(startx + 1, h11, starty + 1), Vector3(startx + 1, h10, starty), Vector3(normal.X(), normal.Y(), normal.Z()), ratio);
					return INTERSECTION_QUAD;//IsHoleTC(start.x, start.y) ? INTERSECTION_HOLE : INTERSECTION_QUAD;
				}
			}

			return INTERSECTION_NONE;
		} else
		{
			//test left upper triangle
			float h01 = GetData(startx, starty + 1);

			if (TestTerrainTriangleLU(relstart, dir, h00, h11, h01, I, normal, d, epsilon))
			{
				if (GetPlaneLineRatio(normal, d, relstart, relend, ratio))
				{
					inters.Save(startx, starty, Vector3(I.X() + sx, (I.Y()), I.Z() + sz), Vector3(startx, h00, starty),	Vector3(startx, h01, starty + 1), Vector3(startx + 1, h11, starty + 1), Vector3(normal.X(), normal.Y(), normal.Z()), ratio);
					return INTERSECTION_QUAD;//IsHoleTC(start.x, start.y) ? INTERSECTION_HOLE : INTERSECTION_QUAD;
				}
			}

			return INTERSECTION_NONE;
		}
	}

	return INTERSECTION_NONE;
}


//-----------------------------------------------------------------------
/*!
test one terrain quad for intersection
*/
//-----------------------------------------------------------------------
TerrainIntersection Scenery::TestTerrainQuad(const Vector3 &start, const Vector3 &end, const Vector3 &dir, IntersectedTriangle &inters, unsigned int startx, unsigned int starty, float epsilon) const
{
	//create source quad
	float sx = (float) startx;
	float sz = (float) starty;
	Vector3 relstart (start.X() - sx, (start.Y()), start.Z() - sz);
	Vector3 relend (end.X() - sx, (end.Y()), end.Z() - sz);
	Vector3 reldir (dir.X(), (dir.Y()), dir.Z());

	//const TerrainHeight *src = m_pHeightMap + startx + starty*m_iWidth;
  float h00 = GetData(startx, starty);
	float h10 = GetData(startx + 1, starty);

	//src += m_iWidth;
	float h01 = GetData(startx, starty + 1);
	float h11 = GetData(startx + 1, starty + 1);


	//test two triangles of quad
	Vector3 I1, I2, normal1, normal2;
	float d1 = 0.0f, d2 = 0.0f, ratio1 = 0.0f, ratio2 = 0.0f;

	bool res1 = TestTerrainTriangleRB(relstart, reldir, h00, h10, h11, I1, normal1, d1, epsilon);
	bool res2 = TestTerrainTriangleLU(relstart, reldir, h00, h11, h01, I2, normal2, d2, epsilon);

	res1 = res1 && GetPlaneLineRatio(normal1, d1, relstart, relend, ratio1); 
	res2 = res2 && GetPlaneLineRatio(normal2, d2, relstart, relend, ratio2); 

	//if both are intersected, choose the closest one
	if (res1)
	{
		if (res2 && ratio2 < ratio1)
		{
			inters.Save(startx, starty, Vector3(I2.X() + sx, I2.Y(), I2.Z() + sz), 	Vector3(startx, h00, starty),	Vector3(startx, h01, starty + 1), Vector3(startx + 1, h11, starty + 1), Vector3(normal2.X(), normal2.Y(), normal2.Z()), ratio2);
			return INTERSECTION_QUAD;//IsHoleTC(start.X(), start.Z()) ? INTERSECTION_HOLE : INTERSECTION_QUAD;
		}

		inters.Save(startx, starty, Vector3(I1.X() + sx, I1.Y(), I1.Z() + sz), 	Vector3(startx, h00, starty),	Vector3(startx + 1, h11, starty + 1), Vector3(startx + 1, h10, starty), Vector3(normal2.X(), normal2.Y(), normal2.Z()), ratio1);
		return INTERSECTION_QUAD;//IsHoleTC(start.X(), start.Z()) ? INTERSECTION_HOLE : INTERSECTION_QUAD;
	}

	if (res2)
	{
    inters.Save(startx, starty, Vector3(I2.X() + sx, I2.Y(), I2.Z() + sz), 	Vector3(startx, h00, starty),	Vector3(startx, h01, starty + 1), Vector3(startx + 1, h11, starty + 1), Vector3(normal2.X(), normal2.Y(), normal2.Z()), ratio2);
		return INTERSECTION_QUAD;//IsHoleTC(start.X(), start.Z()) ? INTERSECTION_HOLE : INTERSECTION_QUAD;
	}

	return INTERSECTION_NONE;
}

const Scenery::TextureInfo &Scenery::ClippedTextureInfo( int z, int x ) const
{
  if( this_InRange(z,x) )
  {
    int index = _tex(x,z);//GetTex(x,z);
    return _texture[index];
  }
  return _outsideTextureInfo;
}

//-----------------------------------------------------------------------
/*!
compute terrain intersection with ray, return false if no exists,
exact DDA
*/
//-----------------------------------------------------------------------
bool Scenery::TestIntersectionTTT(const Vector3 &start, const Vector3 &end, const Vector3 &dir, IntersectedTriangle &inters, bool holes) const
{
  Vector3 dirrrrr = end - start;

	int sx = enf::enf_truncint(start.X());
  int sz = enf::enf_truncint(start.Z());
  int ex = enf::enf_truncint(end.X());
  int ez = enf::enf_truncint(end.Z());
	
	int deltaX = ex - sx;
  int deltaZ = ez - sz;
	
  int absDeltaX = abs(deltaX);
  int absDeltaZ = abs(deltaZ);

	float dx, dz;
	unsigned int steps;
	
	//deltas and steps
	if (absDeltaX > absDeltaZ)
	{
		steps = absDeltaX + 1;
		dx		= dir.X() < 0.0f ? -1.0f : 1.0;
		dz		= (end.Z() - start.Z())/steps;
	} else
	{
		steps = absDeltaZ + 1;
		dx		= (end.X() - start.X())/steps;
		dz		= dir.Z() < 0.0f ? -1.0f : 1.0f;
	}


	enf::Vector2 actpos, oldpos(start.X(), start.Z());
	int iposx, iposz, ioldposx(sx - 1), ioldposz(sz);		//must be s.x - 1, to have loop correct start
	TerrainIntersection res;

	//epsilon computation, changes with distance, so on 100m the delta is 0.1
	float epsilon = 0.0f, epsilondelta = 0.1f/(32.0f*100.0f);

	//compute direction of line for y = kx + q
	float k = 0.0f;
	if (abs(dir.X()) > 0.000001f)
	{
		k = dir.Z()/dir.X();
	} 


	//for each cell in a line
	for (unsigned int i = 0; i < steps; i++)
	{
		actpos.x = start.X() + dx*i;
		actpos.y = start.Z() + dz*i;

		epsilon = 0.001f + epsilondelta*i;

		iposx = enf::enf_truncint(actpos.x);
		iposz = enf::enf_truncint(actpos.y);

		//if new cell is in +- 1,1 direction, we must test other cells too
		int idiffx = iposx - ioldposx;
    int idiffz = iposz - ioldposz; 
		Assert (enf::enf_in(abs(idiffx) + abs(idiffz), 1, 2));

		//test cells
		if (abs(idiffx) + abs(idiffz) == 2)
		{
			//currx, curry is in interval <0..1)
			float currx = oldpos.x - ioldposx;
			float currz = oldpos.y - ioldposz;

			//q = Sy - k*Sx
			float q = currz - k*currx;

			//compute curent y intersection with the line x = 1 when going right or x = 0 when qoing left
			float y = idiffx > 0 ? k + q : q;
				
			if (y < 0.0f || y > 1.0f)
			{
				res = TestTerrainQuad(start, end, dirrrrr, inters, ioldposx, iposz, epsilon);
				if (res == INTERSECTION_QUAD)// || (holes && res == INTERSECTION_HOLE))
				{
					return true;
				}
			} else 
			{
				res = TestTerrainQuad(start, end, dirrrrr, inters, iposx, ioldposz, epsilon);
				if (res == INTERSECTION_QUAD)// || (holes && res == INTERSECTION_HOLE))
				{
					return true;
				}
			}			
		}


		//test intersection with current cell
		res = TestTerrainQuad(start, end, dirrrrr, inters, iposx, iposz, epsilon);
		if (res == INTERSECTION_QUAD)// || (holes && res == INTERSECTION_HOLE))
		{
			return true;
		}

		ioldposx	= iposx;
    ioldposz	= iposz;
		oldpos	= actpos;
	}

	//last one
	//Assert(enf::enf_in(abs(e.x - ioldpos.x) + abs(e.y - ioldpos.y), 0, 1));

	if (ioldposx != ex || ioldposz != ez)
	{
		res = TestTerrainQuad(start, end, dirrrrr, inters, ex, ez, epsilon);
		if (res == INTERSECTION_QUAD)// || (holes && res == INTERSECTION_HOLE))
		{
			return true;
		}
	}

	return false;
}

const enf::RectangleFloat Scenery::GetWorldClipRect() const
{
  return enf::RectangleFloat(_terrainGrid * _data.GetXRange(), _terrainGrid * _data.GetYRange());
}

void Scenery::SetLandGrid(float grid)
{
  #if _DEBUG1
    // no object may be present
    for (int z=0; z<_terrainRange; z++)
    for (int x=0; x<_terrainRange; x++)
    {
      const ObjectList &ol = _objects(x,z);
      Assert(ol.Size()==0);
      const MapObjectList &mol = _mapObjects(x,z);
      Assert(mol.Size()==0);
    }
  #endif
  _landGrid = grid;
  _invLandGrid = 1/grid;
  int terrainLog = _terrainRangeLog-_landRangeLog;
  float invTerrainCoef = 1.0/(1<<terrainLog);
  _terrainGrid = grid*invTerrainCoef;
  _invTerrainGrid = 1/_terrainGrid;
}

GeographyInfo Scenery::GetGeography( int x, int z ) const
{
  if( !this_InRange(z,x) )
  {
    GeographyInfo geogr(0);
    // check if the "island" is in the water or in the desert
    // TODO: cooperate with TerSynt when it is used?
    if (x<0) x=0;if (x>_landRangeMask) x=_landRangeMask;
    if (z<0) z=0;if (z>_landRangeMask) z=_landRangeMask;
    geogr.u.minWaterDepth = GeographyInfo(_geography(x,z)).u.minWaterDepth;
    geogr.u.maxWaterDepth = GeographyInfo(_geography(x,z)).u.maxWaterDepth;
    return geogr;
  }
  return GeographyInfo(_geography(x,z));
}

float Scenery::GetOutsideData(int x, int z, TerrainCache *cache) const
{
  if (_outsideData)
  {
    static bool doCache = true;
    if (doCache)
    {
      // check if we have any data cached for this part
      if (cache && cache->Contains(x,z))
      {
        float ret = cache->GetValue(x,z);
        #if _DEBUG1
          float pixel = _outsideData->GetPixel(x,z);
          Assert(fabs(pixel-ret)<0.1f);
        #endif
        return ret;
      }
    }
    return _outsideData->GetPixel(x,z);
  }
  saturate(x, 0, _terrainRangeMask);
  saturate(z, 0, _terrainRangeMask);
  return GetData(x, z);
}

float Scenery::CheckSeparationGuaranteed(int x, int z, float minLineY) const
{
  if (_outsideData && !this_InRange(x,z))
  {
    // sometimes we can know for sure there is no collision without testing it
    // this is significant optimization in case of TerSynth, which is quite expensive
    //float minY = _outsideData->GetMinHeight(x,z,x+1,z+1);
    float maxY = _outsideData->GetMaxHeight(x,z,x+1,z+1);
    return minLineY-maxY;
  }
  // no guarantees at all
  return -FLT_MAX;
}

void Scenery::InitOutsideTerrain()
{
  _outsideData.Free();
  _outsideCache.Free();
  if (_terSynth)
  {
    // TODO: implement on the fly conversion of data for Xbox as well
    _outsideData = new Terrain;
    _outsideData->SetTerrain(_data.Data1D(),_data.GetXRange(),_data.GetYRange(), _terrainGrid);
  }
}

float Scenery::GetHeight( int z, int x ) const
{
  return ClippedData(z,x);
}

float Scenery::GetLandElevationOffset() const
{
  return _elevationOffset;
}

enf::AAB Scenery::GetBlockAAB(int x, int z) const 
{
  Assert((x + 1) < _data.GetXRange() && (z + 1) < _data.GetYRange());
  float y00 = GetData(x, z);
  float y10 = GetData(x + 1, z);
  float y01 = GetData(x, z + 1);
  float y11 = GetData(x + 1, z + 1);

  float yMin = enf::enf_min(enf::enf_min(y00, y01), enf::enf_min(y10, y11));
  float yMax = enf::enf_max(enf::enf_max(y00, y01), enf::enf_max(y10, y11));

  float xMin = _terrainGrid * x;
  float zMin = _terrainGrid * z;
  return enf::AAB(Vector3(xMin, yMin, zMin), Vector3(xMin + _terrainGrid, yMax, zMin + _terrainGrid));
}

void Scenery::Dim(int x, int z, int rx, int rz, int xlog, int rxlog)
{
  _landRange = x;
  _invLandRange = 1.0f/x;
  _landRangeMask = x-1;
  _landRangeLog = xlog;

  _terrainRange = rx;
  _terrainRangeMask = rx-1;
  _terrainRangeLog = rxlog;
  _terrainRangeLogSource = _terrainRangeLog;

  _geography.Dim(x,z);

  _data.Dim(rx,rz);

  _maxHeight = 0;
  _minHeight = 0;
}

