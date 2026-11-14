#ifndef _pointList_h_
#define _pointList_h_

#include <El/Math/math3d.hpp>
#include <Es/Containers/array.hpp>
#include <projects/ObjektivLib/ObjectData.h>
#include "IncidenceTable.h"

namespace TreeEngine
{

  //! Bounding box structure.
  struct SBoundingBox
  {
    //! Direction limits.
    float _MinDir, _MaxDir;
    //! Up limits.
    float _MinUp, _MaxUp;
    //! Aside limits.
    float _MinAside, _MaxAside;
    //! Retrieves focus of the bounding box.
    inline Vector3 GetFocus() {
      Vector3 Result;
      Result[0] = (_MaxAside + _MinAside) * 0.5f;
      Result[1] = (_MaxUp + _MinUp) * 0.5f;
      Result[2] = (_MaxDir + _MinDir) * 0.5f;
      return Result;
    }
    //! Retrieves dimensions of the bounding box.
    inline Vector3 GetDimension() {
      Vector3 Result;
      Result[0] = _MaxAside - _MinAside;
      Result[1] = _MaxUp - _MinUp;
      Result[2] = _MaxDir - _MinDir;
      return Result;
    }
    //! Retreive volume in m^3
    inline float GetVolume()
    {
      Vector3 dimension = GetDimension();
      return dimension.X() * dimension.Y() * dimension.Z();
    }
  };

  //! Structure to hold a single representative of cloud of points
  struct Representative
  {
    Vector3 _sumPosition;
    float _sumCenterDistance;
    int _n;
    Vector3 GetAvgPosition() const {return _sumPosition / (float)_n;}
    float GetAvgCenterDistance() const {return _sumCenterDistance / (float)_n;}
    ClassIsSimple(Representative)
  };
  typedef AutoArray<Representative> RepresentativeArray;

  class CPointList
  {
  private:
    AutoArray<Vector3> _points;
  public:
    //! Constructor
    CPointList(){}
    //! Constructor that transforms the source points according to given matrix
    CPointList(const CPointList &src, Matrix4Par transform)
    {
      _points.Realloc(src._points.Size());
      _points.Resize(src._points.Size());
      for (int i = 0; i < src._points.Size(); i++)
      {
        _points[i] = transform * src._points[i];
      }
    }

    int NPoints() const
    {
      return _points.Size();
    }
    bool IsEmpty() const
    {
      return _points.Size() == 0;
    }
    bool IsNull() const
    {
      if (IsEmpty()) return true;
      float x,y,z;
      BestOriginAndSize(x,y,z);
      if (x+y+z<0.001f) return true;
      return false;
    }
    void Clear()
    {
      _points.Resize(0);
    }
    void AddPoint(Vector3Par point)
    {
      _points.Add(point);
    }
    int ComputeFocus(Vector3 &focus) const
    {
      int pointsCount = _points.Size();
      if (pointsCount > 0) {
        Vector3 sum = Vector3(0, 0, 0);
        for (int i = 0; i < pointsCount; i++) {
          sum += _points[i];
        }
        focus = sum / ((float)pointsCount);

        return 1;
      }
      else {
        return 0;
      }
    }
    int ComputeFocus(Matrix4Par origin, Vector3 &focus) const
    {
      int pointsCount = _points.Size();
      if (pointsCount > 0) {
        Vector3 tfocus = Vector3(0, 0, 0);
        for (int i = 0; i < pointsCount; i++) {
          tfocus += _points[i] / ((float)pointsCount);
        }

  /*      
        Vector3 sum = Vector3(0, 0, 0);
        for (int i = 0; i < pointsCount; i++) {
          sum += _points[i];
        }
        Vector3 tfocus = sum / ((float)pointsCount);
  */
        Matrix4 invOrigin;
        invOrigin.SetInvertGeneral(origin);
        focus = invOrigin * tfocus;
        return 1;
      }
      else {
        return 0;
      }
    }
    int ComputeBoundingBox(Matrix4Par origin, SBoundingBox &bbox) const
    {
      int pointsCount = _points.Size();
      if (pointsCount > 0)
      {
        Matrix4 invOrigin;
        invOrigin.SetInvertGeneral(origin);
        bbox._MinDir = FLT_MAX;
        bbox._MaxDir = -FLT_MAX;
        bbox._MinUp = FLT_MAX;
        bbox._MaxUp = -FLT_MAX;
        bbox._MinAside = FLT_MAX;
        bbox._MaxAside = -FLT_MAX;
        for (int i = 0; i < pointsCount; i++)
        {
          Vector3 tPoint = invOrigin * _points[i];
          bbox._MinDir = __min(bbox._MinDir, tPoint.Z());
          bbox._MaxDir = __max(bbox._MaxDir, tPoint.Z());
          bbox._MinUp = __min(bbox._MinUp, tPoint.Y());
          bbox._MaxUp = __max(bbox._MaxUp, tPoint.Y());
          bbox._MinAside = __min(bbox._MinAside, tPoint.X());
          bbox._MaxAside = __max(bbox._MaxAside, tPoint.X());
        }
        return 1;
      }
      else
      {
        return 0;
      }
    }
    Matrix3 BestOrientation() const
    {
      Matrix3 cov;
      cov.SetCovarianceCenter(_points);
      cov *= 1.0f / cov.DirectionAside().Size(); // Make the matrix bigger (otherwise it may fall during rounding into a singular case)
      Matrix3 eigV;
      eigV.SetEigenStandard(cov);
      Matrix3 result;
      result.SetDirectionAndUp(eigV.DirectionAside(), eigV.DirectionUp());
      return result;
    }
    void BestSize(Vector3Par focus, Matrix4Par origin, float &sX, float &sY, float &sZ) const
    {
      Matrix4 invOrigin;
      invOrigin.SetInvertGeneral(origin);
      Vector3 sumDistance(0, 0, 0);
      for (int i = 0; i < _points.Size(); i++)
      {
        Vector3 tPoint = invOrigin * _points[i];
        sumDistance[0] += fabs(tPoint[0]);
        sumDistance[1] += fabs(tPoint[1]);
        sumDistance[2] += fabs(tPoint[2]);
      }
      float invSize = 1.0f / _points.Size();
      sX = sumDistance[0] * invSize;
      sY = sumDistance[1] * invSize;
      sZ = sumDistance[2] * invSize;
    }
    Matrix4 BestOriginAndSize(float &sX, float &sY, float &sZ) const
    {
      Vector3 focus;
      if (ComputeFocus(focus))
      {
        Matrix4 origin;
        origin.SetOrientation(BestOrientation());
        origin.SetPosition(focus);
        BestSize(focus, origin, sX, sY, sZ);
        return origin;
      }
      else
      {
        sX = sY = sZ = 1.0f;
        return MIdentity;
      }
    }

    const Vector3 &GetPoint(int index) const
    {
      return _points[index];
    }

    Vector3 RemovePoint(int index)
    {
      Vector3 p = _points[index];
      _points.Delete(index);
      return p;
    }

    //! Copy points from source
    void Copy(const CPointList &from)
    {
      _points = from._points;
    }

    class InciTableOnDemand: public  IInciTableOnDemandCalc {
        mutable IncidenceTableBestGroups &incTable;
        const AutoArray<Vector3> &_points;
    public:

        InciTableOnDemand(   IncidenceTableBestGroups &incTable,
                    const AutoArray<Vector3> &_points)
                    :incTable(incTable),_points(_points) {}

        virtual float Get(int a, int b) const {
            return  _points[a].Distance2(_points[b]);
        }
    };

     //! Find N representatives by method "global search through incidence table"
    void FindRepresentatives(float distance, RepresentativeArray &representatives) const;
/*
        //erase result array
        representatives.Clear();
        //create incidence table
        IncidenceTableBestGroups incTable(NPoints(), false);
        //object calculates distance between two distant points.
        //to emulate radius, we need double the distance
        distance *= 2;
        //calculate distance powered 2
        distance *= distance;
        
        //class will fill table by each to each distance on deamand

        InciTableOnDemand dmd(incTable,_points);
        incTable.SetOnDemand(&dmd);

   
        //create groups of points that lay up to distance
        int groupCount = incTable.CreateGroups(distance);
        //prepare space to result
        int *pointList = new int[NPoints()];
        SRef<int,SRefArrayTraits<int> > pointListAuto = pointList;

        //process all groups
        for (int i = 0; i<groupCount; i++) {
            int best;
            //retrieve group
            int ptcnt = incTable.GetGroup(i,best,pointList);
            //group must have atleast 1 point
            if (ptcnt != 0) {
                //prepare structure
                Representative ra;
                ra._sumPosition = Vector3(0,0,0);
                //find center
                for (int j = 0; j < ptcnt; j++) 
                    ra._sumPosition = ra._sumPosition  + _points[pointList[j]];
                ra._n = ptcnt;
                //center found
                Vector3 center = ra.GetAvgPosition();
                //find radius
                float maxdeviation = 0;
                //calculate max deviation
                for (int j = 0; j < ptcnt; j++)  {
                    float dev = _points[pointList[j]].Distance2(center);
                    if (dev > maxdeviation) maxdeviation = dev;
                }
                //calculate distance
                ra._sumCenterDistance = sqrt(maxdeviation) * ra._n;
                //add representative
                representatives.Add(ra);

            }
        }
    }


  */
      
    


    
/*    //! Find N representatives by method "link closest, expand bounding sphere"
      /** note: bullshit
    void FindNRepresentatives(int n, RepresentativeArray &representatives) const
    {
      // Make sure the output array is clear
      representatives.Clear();

      // If there are no source items, return
      if (_points.Size() <= 0) return;

      // Create point list that will contain remaining points to distribute
      CPointList p;
      p.Copy(*this);

      // Get the real number of representatives
      int nRepresentatives = min(max(1, n), p.NPoints());

      // Fill out n representatives randomly chosen from the list
      representatives.Realloc(nRepresentatives);
      representatives.Resize(nRepresentatives);
      for (int i = 0; i < nRepresentatives; i++)
      {
        representatives[i]._sumPosition = p.RemovePoint(rand() % p.NPoints());
        representatives[i]._sumCenterDistance = 0.0f;
        representatives[i]._n = 1;
      }

      // Distribute remaining points into the representatives and affect those representatives.
      // Note that resulting average distance is not mathematically correct, because center is
      // traveling during the computation
      while (p.NPoints() > 0)
      {
        // Pick random point from the source
        Vector3 point = p.RemovePoint(rand() % p.NPoints());

        // Find the closest representative
        int closestRepresentative = 0;
        float closestDistance2 = (point - representatives[0].GetAvgPosition()).SquareSize();
        for (int i = 1; i < nRepresentatives; i++)
        {
          float newDistance2 = (point - representatives[i].GetAvgPosition()).SquareSize();
          if (newDistance2 < closestDistance2)
          {
            closestDistance2 = newDistance2;
            closestRepresentative = i;
          }
        }

        // Update the closest representative
        representatives[closestRepresentative]._sumPosition += point;
        representatives[closestRepresentative]._sumCenterDistance += sqrt(closestDistance2);
        representatives[closestRepresentative]._n++;
       }
    }
    */
    
    //!{ 2D operations
    /*!
      Consider the space as space where positive directions are to right and up. Meaning of terms
      like "clockwise" is related to this presumption.
    */

  private:

    //! Function to retrieve coefficients of the specified line (normal [a, b] points right from line)
    /*!
      Consider line equation Ax + By + C = 0
    */
    static void GetLineCoef2D(Vector3Par pos, Vector3Par dir, float &a, float &b, float &c)
    {
      a = dir.Y(); // Normal [dir.Y, -dir.X] points right from the line
      b = -dir.X();
      c = -(a * pos.X() + b * pos.Y());
    }

    //! Function to determine the specified point is on right side of the line specified by [a, b, c]
    static bool IsPointOnRightSide2D(Vector3Par p, float a, float b, float c)
    {
      return (a * p.X() + b * p.Y() + c > -0.00001f);
    }

    //! Function to determine whether specified point is on right side of the line specified by pos and dir
    static bool IsPointOnRightSide2D(Vector3Par p, Vector3Par pos, Vector3Par dir)
    {
      float a, b, c;
      GetLineCoef2D(pos, dir, a, b, c);
      return IsPointOnRightSide2D(p, a, b, c);
    }

    //! Function to calculate intersection of 2 specified lines
    /*!
      Function returns intersection X as a float coefficient I, so that X = dirA*I + posA
    */
    static bool IntersectionACoef(Vector3Par posA, Vector3Par dirA, Vector3Par posB, Vector3Par dirB, float &outI)
    {
      // Get the divisor, if it's close to zero then report the lines are coplanar
      float d = dirB.Y() * dirA.X() - dirB.X() * dirA.Y();
      if (fabs(d) < 0.00001f) return false;

      // Calculate the intersection coefficient
      outI = (dirB.X() * (posA.Y() - posB.Y()) + dirB.Y() * (posB.X() - posA.X())) / d;

      // Return success
      return true;
    }

  public:

    //! Function to retrieve the bounding rectangle parameters
    void GetMinMax2D(float &minX, float &maxX, float &minY, float &maxY) const
    {
      minX = minY = FLT_MAX;
      maxX = maxY = -FLT_MAX;
      for (int i = 0; i < _points.Size(); i++)
      {
        const Vector3 &p = _points[i];
        if (p.X() < minX) minX = p.X();
        if (p.X() > maxX) maxX = p.X();
        if (p.Y() < minY) minY = p.Y();
        if (p.Y() > maxY) maxY = p.Y();
      }
    }

    //! Function to retrieve the bounding rectangle parameters - returns bounding points
    void GetMinMax2D(Vector3 &minX, Vector3 &maxX, Vector3 &minY, Vector3 &maxY) const
    {
      minX = Vector3(FLT_MAX, 0, 0);
      maxX = Vector3(-FLT_MAX, 0, 0);
      minY = Vector3(0, FLT_MAX, 0);
      maxY = Vector3(0, -FLT_MAX, 0);
      for (int i = 0; i < _points.Size(); i++)
      {
        const Vector3 &p = _points[i];
        if (p.X() < minX.X()) minX = p;
        if (p.X() > maxX.X()) maxX = p;
        if (p.Y() < minY.Y()) minY = p;
        if (p.Y() > maxY.Y()) maxY = p;
      }
    }

    //! Ask the point list whether all his points lie on right side of the specified line
    bool AreAllPointsOnRightSideOfLine2D(Vector3Par pos, Vector3Par dir) const
    {
      // Get the line coefficients
      float a, b, c;
      GetLineCoef2D(pos, dir, a, b, c);
      for (int i = 0; i < _points.Size(); i++)
      {
        if (!IsPointOnRightSide2D(_points[i], a, b, c)) return false;
      }
      return true;
    }

    //! Function will consider edge specified by [a, b, c] and creates zero or one or two points instead of b
    /*!
      Function returns square area that was saved with cutting the edge. Array newB must be prepared
      for 2 members.
    */
    float CutEdge2D(Vector3Par a, Vector3Par b, Vector3Par c, Vector3 *newB, int &newVertexCount) const
    {
      // Sanity check - c should lie right from a to b line
      if (!IsPointOnRightSide2D(c, a, b-a))
      {
        Fail("Error: Edge is not in a clockwise order");
        newB[0] = b;
        newVertexCount = 1;
      }

      // Go through all possible pairs of points, consider those that split the space
      // into B point and the rest points and remember the maximum area saved
      float maxArea2 = -1.0f;
      Vector3 maxVecBB1;
      Vector3 maxVecBB2;
      for (int y = 0; y < _points.Size(); y++)
      {
        for (int x = y + 1; x < _points.Size(); x++)
        {
          // Get the line
          Vector3 pos = _points[x];
          Vector3 dir = _points[y] - _points[x];

          // Skip cases where points are too close
          if (dir.SquareSize() < 0.00001f) continue;

          // Make sure the B point is on left side
          if (IsPointOnRightSide2D(b, pos, dir))
          {
            dir = -dir;
          }

          // If all the points are one right side of the line, then it can be considered
          // as possible cut - calculate intersections and area that was cut, remember the max
          if (AreAllPointsOnRightSideOfLine2D(pos, dir))
          {
            // Calculate intersections
            float coefBA;
            float coefBC;
            if (IntersectionACoef(b, a-b, pos, dir, coefBA) && IntersectionACoef(b, c-b, pos, dir, coefBC))
            {
              if ((coefBA > 0.00001f) && (coefBC > 0.00001f)) // Intersections must be positive to form a triangle
              {
                // Make sure the coefficients are in a proper interval
                saturateMin(coefBA, 1.0f);
                saturateMin(coefBC, 1.0f);

                // Get the vectors that determine triangle to be clipped
                Vector3 vecBB1 = (a-b) * coefBA;
                DoAssert(vecBB1.Z() == 0.0f);
                Vector3 vecBB2 = (c-b) * coefBC;
                DoAssert(vecBB2.Z() == 0.0f);

                // Determine triangle (resp. parallelogram) square size by cross product
                float area2 = vecBB1.CrossProduct(vecBB2).SquareSize();

                // If it exceeds the current maximum value then remember the new one with coefficients
                if (area2 > maxArea2)
                {
                  maxArea2 = area2;
                  maxVecBB1 = vecBB1;
                  maxVecBB2 = vecBB2;
                }
              }
            }
          }
        }
      }
      
      // Fill out the output values
      if (maxArea2 < 0.0f) // If the area is smaller than 0, that means the edge was not cut
      {
        newB[0] = b;
        newVertexCount = 1;
      }
      else
      {

        newVertexCount = 0;

        // If B1 is not equal to A, then add it as a new point
        if ((a-b).Distance2(maxVecBB1) > 0.00001f)
        {
          newB[newVertexCount++] = b + maxVecBB1;
        }

        // If B2 is not equal to B AND if it's not equal to B1, then add it as a new point
        if (((c-b).Distance2(maxVecBB2) > 0.00001f) && (maxVecBB1.Distance2(maxVecBB2) > 0.00001f))
        {
          newB[newVertexCount++] = b + maxVecBB2;
        }
      }

      return maxArea2;
    }

    //! Initialize point list with reduced source
    void InitWithReduction(const CPointList &pointList)
    {
      // Initialize the array
      _points.Realloc(pointList.NPoints()); // Number of source points is a good estimation
      _points.Clear();

      // Get leftmost, rightmost, upmost and downmost point
      Vector3 minX, maxX, minY, maxY;
      pointList.GetMinMax2D(minX, maxX, minY, maxY);

      for (int i = 0; i < pointList.NPoints(); i++)
      {
        const Vector3 &p = pointList.GetPoint(i);

        // If the point is close to some of the bounding points, then add it without further tests
        if ((p.Distance2(minX) > 0.00001f) &&
            (p.Distance2(maxX) > 0.00001f) &&
            (p.Distance2(minY) > 0.00001f) &&
            (p.Distance2(maxY) > 0.00001f))
        {
          // If point is inside the tetragon determined by the minmax values, don't add the point
          if (IsPointOnRightSide2D(p, minX, maxY-minX) &&
              IsPointOnRightSide2D(p, maxY, maxX-maxY) &&
              IsPointOnRightSide2D(p, maxX, minY-maxX) &&
              IsPointOnRightSide2D(p, minY, minX-minY))
          {
            continue;
          }
        }

        // Add the point
        _points.Add(p);
      }

      // Compacting possible, but not required by caller
      //_points.Compact();
    }

    //! Initialize point list with the convex hull of the points specified
    /*!
      Convex hull will be in clockwise order.
    */
    void InitWithConvexHull2D(const CPointList &pointList, int maxHullVertices);
    //!}
  };

}
#endif
