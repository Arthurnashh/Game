#include <stdlib.h>
#include "pointList.h"
#include <map>

typedef Vector3 VECT2;


namespace TreeEngine
{

    //!{ Quick hull algorithm
    static const VECT2 *Sort_Pts;
    static float Angle(const VECT2 P0, const VECT2 P1, const VECT2 P2)
    {
        // "Angle" between P0->P1 and P0->P2.
        // actually returns: ||(P1-P0) ^ (P2-P0)||
        float dx21 = P2[0] - P1[0];
        float dy21 = P2[1] - P1[1];
        float dx10 = P1[0] - P0[0];
        float dy10 = P1[1] - P0[1];
        return (dy21*dx10 - dx21*dy10);
    }
    static int Compare(const VECT2 A, const VECT2 B) {
        if (A[0]<B[0]) return  1;
        else return -1;  // thanks to Sjoerd Velthuijsen for pointing me to the previously incorrect comparison
    }
    static int Cmp_Lo(const void *a, const void *b) {
        return Compare( Sort_Pts[*(int*)a], Sort_Pts[*(int*)b] );
    }
    static int Cmp_Hi(const void *a, const void *b) { 
        return Compare( Sort_Pts[*(int*)b], Sort_Pts[*(int*)a] );
    }
#define SWAP(i,j) { int Tmp = Ind[i]; Ind[i] = Ind[j]; Ind[j] = Tmp; }
    static int QHull_Internal(int N, const VECT2 *Pts, int *Ind,
        int iA, int iB)
    {
        int n, nn, n2, m, Split1, Split2, iMid;
        int Mid_Pt;
        double dMax;

        if (N<=2) return N;

        // As middle point, search the farthest away from line [A-->B]

        Mid_Pt = -1;
        dMax = 0.0;
        for(n=1; n<N; ++n) {
            double d = fabs( Angle( Pts[Ind[n]], Pts[iA], Pts[iB] ) );
            if (d>=dMax) { dMax = d; Mid_Pt = n; }
        }

        // Partition against midpoint

        iMid = Ind[Mid_Pt];
        Ind[Mid_Pt] = Ind[N-1];

        //Ind = [A|...........|xxx] (B)
        //  n = [0|1.......N-2|N-1|

        nn = N-2;
        n  = 1;
        while(n<=nn) {
            float d = Angle( Pts[Ind[n]], Pts[iA], Pts[iMid] );
            if (d>=0.0) { SWAP(n,nn); nn--; }
            else n++;
        }
        Ind[N-1] = Ind[n];
        Ind[n] = iMid;
        Split1 = n++;

        //Ind = (A) [..(+)..| M |...........] (B)
        //  n =     [1......|Sp1|........N-1]

        nn = N-1;
        while(n<=nn) {
            float d = Angle( Pts[Ind[n]], Pts[iMid], Pts[iB] );
            if (d>=0.0) { SWAP(n,nn); nn--; }
            else n++;
        }
        Split2 = n;

        // Status:
        //Ind = (A) [....(+)...| M |....(-)....|(trash)......] (B)
        //  N =     [1.........|Sp1|...........|Sp2.......N-1]

        // Recurse each sub-partition

        n  = QHull_Internal(Split1,        Pts, Ind       , iA, iMid);
        n2 = QHull_Internal(Split2-Split1, Pts, Ind+Split1, iMid, iB);
        m = Split1;
        while(n2-->0) {
            SWAP(n,m);
            m++; n++;
        }

        return n;
    }
    int Quick_Hull_2D(int Nb, const VECT2 *Pts, int *Ind)
    {
        int n, nn, m, iA, iB;

        if (Nb<=2) return Nb;

        Sort_Pts = Pts; // nasty. Only to overcome qsort()'s API
        qsort(Ind, Nb, sizeof(int), Cmp_Lo);

        // first partitioning: classify points with respect to
        // the line joining the extreme points #0 and #Nb-1

        iA = Ind[0];
        iB = Ind[Nb-1];

        m = Nb-2;
        n = 1;
        while(n<=m) {
            float d = Angle( Pts[Ind[n]], Pts[iA], Pts[iB] );
            if (d>=0.0) { SWAP(n,m); m--; }
            else n++;
        }
        Ind[Nb-1] = Ind[n];
        Ind[n] = iB;

        // Partition is now:
        //  Ind = [ A|...(+)....[B]...(-)...|A ]
        //   n  = [0 |1.........[n].........|Nb]
        // We now process the two halves [A..(+)..B] and [B..(-)..A]

        m  = QHull_Internal(   n, Pts, Ind  , iA, iB); // 1st half [A..(+)..B]
        nn = QHull_Internal(Nb-n, Pts, Ind+n, iB, iA); // 2nd half [B..(-)..A]

        while(nn-->0) {
            SWAP(m,n);
            m++; n++;
        }
        return m;
    }
    //!}

    void CPointList::InitWithConvexHull2D(const CPointList &pointList, int maxHullVertices)
    {
        // 4 vertices is the minimum of the hull (as there is a rectangle at the beginning)
        DoAssert(maxHullVertices >= 4);
        saturateMax(maxHullVertices, 4);

        // We know the final number of hull vertices and we want the array to be cleared
        _points.Realloc(maxHullVertices);
        _points.Resize(0);

        // If no points are specified, then don't create any hull
        if (pointList.NPoints() <= 0) return;

        // Reduce the point list 
        CPointList reducedPointList;
        {
            // Prepare the source array
            AutoArray<VECT2> src;
            src.Realloc(pointList.NPoints());
            src.Resize(pointList.NPoints());
            for (int i = 0; i < src.Size(); i++)
            {
                const Vector3 &p = pointList.GetPoint(i);
                src[i][0] = p.X();
                src[i][1] = p.Y();
            }

            // Prepare the destination array
            AutoArray<int> dst;
            dst.Realloc(pointList.NPoints() + 1);
            dst.Resize(pointList.NPoints() + 1);
            for (int i = 0; i < dst.Size(); i++)
            {
                dst[i] = i;
            }

            // Create the convex hull
            int n = Quick_Hull_2D(src.Size(), src.Data(), dst.Data());

            // Copy the result to reduced list
            reducedPointList._points.Realloc(n);
            reducedPointList._points.Resize(0);
            for (int i = 0; i < n; i++)
            {
                reducedPointList.AddPoint(Vector3(src[dst[i]][0], src[dst[i]][1], 0));
            }
        }
        //reducedPointList.InitWithReduction(pointList);
        LogF("Info: Point list reduced from %d to %d", pointList.NPoints(), reducedPointList.NPoints());

        // Fill out first 4 points from the bounding square. The points will be in a clockwise order
        {
            float minX, maxX, minY, maxY;
            reducedPointList.GetMinMax2D(minX, maxX, minY, maxY);
            AddPoint(Vector3(minX, minY, 0));
            AddPoint(Vector3(minX, maxY, 0));
            AddPoint(Vector3(maxX, maxY, 0));
            AddPoint(Vector3(maxX, minY, 0));
        }

        // Cut the largest edge in cycle till we reach the maxHullVertices
        while (_points.Size() < maxHullVertices)
        {
            // Find the largest edge
            float maxArea2 = -1.0f;
            Vector3 maxNewB[2];
            int maxNewBCount = -1;
            int maxB = -1;
            for (int i = 0; i < _points.Size(); i++)
            {
                Vector3 newB[2];
                int newBCount;
                int a = i;
                int b = (i+1)%_points.Size();
                int c = (i+2)%_points.Size();
                float area2 = reducedPointList.CutEdge2D(_points[a], _points[b], _points[c], newB, newBCount);
                if (area2 > maxArea2)
                {
                    maxArea2 = area2;
                    for (int j = 0; j < newBCount; j++) maxNewB[j] = newB[j];
                    maxNewBCount = newBCount;
                    maxB = b;
                }
            }

            // If no edge to be cut found, finish
            if (maxArea2 <= 0.0f) break;

            // Some suitable edge was found (resp. if cut area is greater than zero), then cut it
            _points.Delete(maxB);
            for (int i = maxNewBCount - 1; i >= 0; i--)
            {
                _points.Insert(maxB, maxNewB[i]);
            }
        }
        //       Vector3 newB[2];
        //       int newBCount;
        //       reducedPointList.CutEdge2D(_points[0], _points[1], _points[2], newB, newBCount);
        //       _points.Delete(1);
        //       for (int i = newBCount - 1; i >= 0; i--)
        //       {
        //         _points.Insert(1, newB[i]);
        //       }
    }

    void CPointList::FindRepresentatives(float distance, 
        RepresentativeArray &representatives) const {


            struct SIntCoord {
                int x;
                int y;
                int z;
                SIntCoord (int x, int y, int z):x(x),y(y),z(z) {}
                SIntCoord (const Vector3 &vec, float distance):
                x(toLargeInt(vec[0]/distance)),
                    y(toLargeInt(vec[1]/distance)),
                    z(toLargeInt(vec[2]/distance)) {}

                int cmp(const SIntCoord &other) const {
                    if (x>other.x) return 1;
                    if (x<other.x) return -1;
                    if (y>other.y) return 1;
                    if (y<other.y) return -1;
                    if (z>other.z) return 1;
                    if (z<other.z) return -1;
                    return 0;
                }

                bool operator<(const SIntCoord &other) const {return cmp(other)<0;}
                bool operator>(const SIntCoord &other) const {return cmp(other)>0;}
                bool operator==(const SIntCoord &other) const {return cmp(other)==0;}
                bool operator!=(const SIntCoord &other) const {return cmp(other)!=0;}
                bool operator>=(const SIntCoord &other) const {return cmp(other)>=0;}
                bool operator<=(const SIntCoord &other) const {return cmp(other)<=0;}

            };

            typedef std::map<SIntCoord, Representative> PointMap;

            PointMap pmap;


            for (int i = 0; i<_points.Size(); i++) {

                SIntCoord key(_points[i], distance);
                float nearest = distance;
                Representative *best = 0;
                PointMap::iterator iter = pmap.find(key);
                if (iter != pmap.end()) {
                    float distance = iter->second.GetAvgPosition().Distance(_points[i]);

                    iter->second._sumPosition += _points[i];
                    iter->second._sumCenterDistance += distance;
                    iter->second._n ++;
                }
                else 
                {
                    Representative x;
                    x._sumCenterDistance = distance/2;
                    x._sumPosition = _points[i];
                    x._n = 1;
                    pmap.insert(std::make_pair(key,x));
                }
            }

            representatives.Clear();
            for (PointMap::const_iterator iter = pmap.begin(); iter != pmap.end(); ++iter) {
                representatives.Add(iter->second);
            }


    }

}
