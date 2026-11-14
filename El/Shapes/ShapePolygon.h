#pragma once
#include "shapepoly.h"

namespace ShapeFiles
{
	class IShapePolygonTest
	{
	public:
		DECLARE_IFCUIDA('S', 'P', 'o', 'l', 'g', 'T');

		virtual bool IntersectionPoints(double yPos, Array<double>& sortedX) = 0;
	};

	class ShapePolygon : public ShapePoly, public IShapePolygonTest
	{    
	public:
		ShapePolygon(const Array<unsigned int>& points = Array<unsigned int>(0, 0),
					 const Array<unsigned int>& parts  = Array<unsigned int>(0),
					 VertexArray* vx = 0) 
		: ShapePoly(points, parts, vx) 
		{
		}

		ShapePolygon(const ShapePoly& other) 
		: ShapePoly(other)
		{
		}

	protected:
		static int AboveBelow(const DVector& b, double yline)
		{
			if (b.y > yline) return 1;
			if (b.y < yline) return -1;
			return 0;
		}

	public:
		virtual bool PtInside(const DVector& vx) const
		{
			bool inside = false;
			for (unsigned int i = 0, cnt = GetPartCount(); i < cnt; i++)
			{
				unsigned int pindex = GetPartIndex(i);
				unsigned int psize = GetPartSize(i);
				DVector a = GetVertex(pindex + psize - 1);
				int state = AboveBelow(a, vx.y);
				int counter = 0;
				for (unsigned int j = 0; j < psize; j++)
				{
					const DVector& b = GetVertex(j + pindex);
					int st1 = AboveBelow(b, vx.y);
					if (state == 0) 
					{
						st1 = state;
					}
					else if (st1 != 0)
					{
						if (state != st1 && fabs(b.y - a.y) > 0.00000000001)
						{
							double xp = (vx.y - a.y) / (b.y - a.y) * (b.x - a.x) + a.x;
							if (xp > vx.x) counter++;
							state = st1;
						}
					}
					a = b;
				}
				if (counter & 1) inside = !inside;
			}
			return inside;
		}

		virtual DVector NearestInside(const DVector& vx) const
		{
			if (PtInside(vx)) 
			{
				return vx;
			}
			else
			{
				return NearestToEdge(vx, true);
			}
		}

		virtual void* GetInterfacePtr(unsigned int ifcuid)
		{
			if (ifcuid == IShapePolygonTest::IFCUID) 
			{
				return static_cast<IShapePolygonTest*>(this);
			}
			else 
			{
				return ShapePoly::GetInterfacePtr(ifcuid);
			}
		}

		virtual bool IntersectionPoints(double yPos, Array<double>& sortedX)
		{
			double* lst = sortedX.Data();
			int maxlst = sortedX.Size();
			int curlst = 0;

			for (unsigned int i = 0, cnt = GetPartCount(); i < cnt; i++)
			{
				unsigned int pindex = GetPartIndex(i);
				unsigned int psize = GetPartSize(i);
				DVector a = GetVertex(pindex + psize - 1);
				int state = AboveBelow(a, yPos);
				for (unsigned int j = 0; j < psize; j++)
				{
					const DVector& b = GetVertex(j + pindex);
					int st1 = AboveBelow(b, yPos);
					if (state == 0) 
					{
						st1 = state;
					}
					else if (st1 != 0)
					{
						double xp = (yPos - a.y) / (b.y - a.y) * (b.x - a.x) + a.x;
						if (curlst == maxlst) return false;

						int i = curlst;
						while (i > 0 && lst[i - 1] > xp)
						{
							lst[i] = lst[i - 1];
							i--;
						}
						lst[i] = xp;
						curlst++;

						state = st1;
					}
					a = b;
				}
			}
			sortedX = Array<double>(lst, curlst);
			return true;
		}

		virtual ShapePolygon* NewInstance(const ShapePoly& other) const
		{
			return NewInstance(ShapePolygon(other));
		}

		///Creates copy of the shape
		/**
		* @copydoc ShapeMultiPoint::NewInstance 
		*/
		virtual ShapePolygon* NewInstance(const ShapePolygon& other) const
		{
			return new ShapePolygon(other);
		}

		///Function helps to calculating clipping of the shape
		/**
		* @copydoc ShapePoly::ClipHelp     
		*/
		void ClipHelp(double xn, double yn, double pos, VertexArray* vxArr, AutoArray<unsigned int, MemAllocLocal<unsigned int, 64> >& newParts, AutoArray<unsigned int, MemAllocLocal<unsigned int, 64> >& newPoints) const
		{
			ShapePoly::ClipHelp(xn, yn, pos, vxArr, newParts, newPoints);
			const DVertex& v1 = GetVertex(0);
			int sid1 = Side(xn, yn, pos, v1.x, v1.y);
			const DVertex& v2 = GetVertex(GetVertexCount() - 1);
			int sid2 = Side(xn, yn, pos, v2.x, v2.y);
			if (sid1 != 0 && sid2 != 0 && sid1 != sid2)
			{
				DVertex cp = this->ClipPoint(v2, v1, xn, yn, pos);
				if (sid2 < 0)
				{
					newParts.Add(newPoints.Size());
				}
				newPoints.Add(vxArr->AddVertex(cp));
				sid2 = 0;
			}
			if (sid1 >= 0 && sid2 >= 0 && newParts.Size() > 1)
			{
				for (unsigned int i = 0; i < newParts[1]; i++)
				{
					unsigned int p = newPoints[i];
					newPoints.Add(p);
				}
				newPoints.Delete(0, newParts[1]);
				newParts.Delete(0, 1);
				unsigned int offset = newParts[0];
				for (int i = 0; i < newParts.Size(); i++)
				{
					newParts[i] -= offset;
				}
			}
		}

		virtual IShape* Clip(double xn, double yn, double pos, VertexArray* vxArr = 0) const
		{
			AutoArray<unsigned int, MemAllocLocal<unsigned int, 64> > newParts;
			AutoArray<unsigned int, MemAllocLocal<unsigned int, 64> > newPoints;
			if (vxArr == 0) vxArr = _vertices;
			//if there is more then one part
			if (GetPartCount() > 1)
			{
				AutoArray<unsigned int, MemAllocLocal<unsigned int, 64> > curParts;
				AutoArray<unsigned int, MemAllocLocal<unsigned int, 64> > curPoints;
				for (int i = 0, cnt = GetPartCount(); i < cnt; i++)
				{
					curParts.Clear();
					curPoints.Clear();          
					unsigned int onepart = 0;
					ShapePolygon simplePartPoly(Array<unsigned int>(const_cast<unsigned int*>(_points.Data()) + GetPartIndex(i), GetPartSize(i)),
					Array<unsigned int>(&onepart, 1), _vertices);
					simplePartPoly.ClipHelp(xn, yn, pos, vxArr, curParts, curPoints);          
					unsigned int offset = newPoints.Size();
					newPoints.Append(curPoints);
					for (int i = 0; i < curParts.Size(); i++) 
					{
						newParts.Add(curParts[i] + offset);          
					}
				}
			}
			else
			{
				ClipHelp(xn, yn, pos, vxArr, newParts, newPoints);
			}
			if (newPoints.Size() == 0) return 0;
			return NewInstance(ShapePolygon(newPoints, newParts, vxArr));
		} 

		///retrieves shape perimeter
		/**
		@return shape perimeter
		*/
		virtual double GetPerimeter() const 
		{
			return ShapePoly::GetPerimeter(true);
		}

		///retrieves shape's area signed
    //useful to detect CW/ CCW
		/**
		@return shape area
		*/
		virtual double GetAreaSigned() const
		{
			double area = 0.0f;

			for (int i = 0, cnt = GetPartCount(); i<cnt; i++)
			{
				int start = GetPartIndex(i);
				int count = GetPartSize(i);
				for (int j = 0, cnt = count - 1; j<cnt; j++)
				{
					ShapeFiles::DVertex v1 = GetVertex(j + start);
					ShapeFiles::DVertex v2 = GetVertex(i + start + 1);          

					// calculates area (gauss algorithm)
					area += v1.x * v2.y - v1.y * v2.x;
				}
			}
			return area * 0.5f;
		}

    ///retrieves shape's area
    /**
    @return shape area
    */
    virtual double GetArea() const
    {
      return  fabs(GetAreaSigned());
    };

		virtual IShape* RemoveVertices(const Array<unsigned int>& pointsToRemove, VertexArray* vxArr) const
		{
			if (vxArr) vxArr = _vertices;

			AutoArray<unsigned int, MemAllocLocal<unsigned int, 64> > newParts;
			AutoArray<unsigned int, MemAllocLocal<unsigned int, 64> > newPoints;
			newParts  = _parts;

			for (unsigned int i = 0, srcCnt = _points.Size(); i < srcCnt; ++i)
			{
				bool found = false;
				for (unsigned int j = 0, remCnt = pointsToRemove.Size(); j < remCnt; ++j)
				{
					if (pointsToRemove[j] == i)
					{
						found = true;
						for (unsigned int k = 0, partCnt = newParts.Size(); k < partCnt; ++k)
						{
							if (newParts[k] >= i)
							{
								newParts[k]--;
							}
						}
						break;
					}
				}
				if (!found)
				{
					newPoints.Add(vxArr->AddVertex(GetVertex(i)));
				}
			}
			return NewInstance(ShapePolygon(newPoints, newParts, vxArr));
		}
	};
}
