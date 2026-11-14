#pragma once
#include "ShapeMultiPoint.h"

namespace ShapeFiles
{
	///Class is base class for ShapePolygon and ShapePolyLine
	/**
	* Shape is consists of groups list of vertices grouped into the parts. The shape doesn't
	* define usage for these vertices. To use vertices as polygons, create ShapePolygon, to use
	* vertices as polylines, create ShapePolyLine. Both class can be constructed from
	* this class.
	*/

	class ShapePoly : public ShapeMultiPoint
	{
	protected:
		AutoArray<unsigned int> _parts;

	public:

		ShapePoly(const Array<unsigned int>& points = Array<unsigned int>(0, 0),
				  const Array<unsigned int>& parts  = Array<unsigned int>(0),
				  VertexArray* vx = 0)
		: ShapeMultiPoint(points, vx) 
		{
			static unsigned int part0 = 0;
			if (parts.Size() == 0 && points.Size() != 0) 
			{
				_parts.Copy(Array<unsigned int>(&part0, 1).Data(), Array<unsigned int>(&part0, 1).Size());
			}
			else 
			{
				_parts.Copy(parts.Data(), parts.Size());
			}
		}

		virtual unsigned int GetPartCount() const 
		{
			return _parts.Size();
		}

		virtual unsigned int GetPartIndex(unsigned int partId) const 
		{
			return partId >= (unsigned)_parts.Size() ? GetVertexCount() : _parts[partId];
		}

		virtual unsigned int GetPartSize(unsigned int partId) const
		{
			return GetPartIndex(partId + 1) - GetPartIndex(partId);
		}

		///Function calculates the nearest point to specified point that lies on the stroke.
		/**
		*  Stroke is defined between points a and b. 
		* @param pt testpoint
		* @param a start of the stroke
		* @param b end of the stroke
		* @param res (out) found nearest point
		* @param d2 (out) distance powered by 2 from specified point and found point
		* @retval true point has been found
		* @retval false point cannot be mapped at stroke.
		*
		* @note function returns false, when vertical bisector at stroke that leads through point 'pt' has intersection with
		* that stroke outside of stroke (is not inside range between points 'a' and 'b'). To get distance in this case,
		* get nearest distance of 'pt' from 'a' and 'pt' from 'b'
		*/
		static bool PrumetNaUsecku(const DVector& pt, const DVector& a, const DVector& b, DVector& res, double& d2)
		{
			// ---------------------------------------
			// new implementation using vectors
			// ---------------------------------------

			// the vector on the segment
			DVector segment(b.x - a.x, b.y - a.y, 0.0);
			double segLength = segment.SizeXY();
			if (segLength > 0.0)
			// goes on only if the segment is not a null segment
			{
				// the unitary vector along the segment
				DVector unitSegment(segment);
				unitSegment.NormalizeXYInPlace();
				// the vector from the first end of the segment to the extern point 
				DVector v(pt.x - a.x, pt.y - a.y, 0.0);
				// the length of the component of the vector v along the segment
				double proj = v.DotXY(unitSegment);
				// if 0 <= proj <= segLength the projection of the node on
				// the segment is really ON the segment
				// we go on with calculation only in this case
				if(0.0 <= proj && proj <= segLength)
				{
					double angle = unitSegment.DirectionXY();
					// the projection on the segment
					res.x = a.x + proj * cos(angle);
					res.y = a.y + proj * sin(angle);
					// squared distance between extern point and its projection on the segment
					d2 = pt.DistanceXY2(res);
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}

/* old code
// this code is somehow buggy, doesn't work in any situation

		//rovnice primky
		double la = -(b.y - a.y);
		double lb = (b.x - a.x);
		double lc = -(la * a.x + lb * a.y);

		//kolmice vedena bode pt
		double pa = lb;
		double pb = -la;
		double pc = -(pa * pt.x + pb * pt.y);

		//bod v pruseciku

//		la*x+lb*y=-lc
//		pa*x+pb*y=-pc

//		la lb
//		pa pb

//		-lc lb
//		-pc pb

//		la -lc
//		pa -pc

		double dd = la * pb - lb * pa;
		double dx = -lc * pb + lb * pc;
		double dy = -la * pc + lc * pa;

		res.x = dx / dd;
		res.y = dy / dd;

		if (res.x >= a.x && res.x <= b.x || res.x <= a.x && res.x >= b.x ||
			res.y >= a.y && res.y <= b.y || res.y <= a.y && res.y >= b.y)
		{
			d2 = (res.x - pt.x) * (res.x - pt.x) + (res.y - pt.y) * (res.y - pt.y);
			return true;
		}
		return false;

*/
    }

		///Calculates clip point for given stroke
		/**
		* @note function excepted, that first point lies on other side then second point 
		*/
		static DVertex ClipPoint(const DVertex& from, const DVertex& to, double a, double b, double c)
		{
			//rovnice primky
			double la = -(from.y - to.y);
			double lb = (from.x - to.x);
			double lc = -(la * from.x + lb * from.y);

			double dd = la * b - lb * a;
			double dx = -lc * b + lb * c;
			double dy = -la * c + lc * a;

			double x = dx / dd;
			double y = dy / dd;
			double z, m;
			if (fabs(from.x - to.x) > fabs(from.y - to.y))
			{
				z = (x - from.x) / (to.x - from.x) * (to.z - from.z) + from.z;
				m = (x - from.x) / (to.x - from.x) * (to.m - from.m) + from.m;
			}
			else
			{
				z = (y - from.y) / (to.y - from.y) * (to.z - from.z) + from.z;
				m = (y - from.y) / (to.y - from.y) * (to.m - from.m) + from.m;
			}
			return DVertex(x, y, z, m);
		}

		///Calculates point that lies on shape edge and is nearest to given point
		/**
		* Function assumes, that shape is line or polygon. It finds nearest point to
		* given point.
		* @param vx given point. 'x' and 'y' members are used only.
		* @param closed when true, function assumes, that the shape is polygon, and there is
		*   an extra line between the last and the first point of the part. If false, the
		*   shape is polyline, so no extra line is necessery.
		*/
		DVector NearestToEdge(const DVector& vx, bool closed) const
		{
			double nearest = DBL_MAX;
			DVector nearestPt;

			for (unsigned int i = 0, cnt = GetPartCount(); i < cnt; ++i)
			{
				unsigned int part  = GetPartIndex(i);
				unsigned int psize = GetPartSize(i);

				DVector a = GetVertex(closed ? part + psize - 1 : part);

				double dx = vx.x - a.x;
				double dy = vx.y - a.y;
				double d = dx * dx + dy * dy;
				if (d < nearest)
				{
					nearest   = d;
					nearestPt = a;
				}

//				for (unsigned int j = closed ? 0 : 1; j < psize; ++j)
				for (unsigned int j = closed ? part : part + 1; j < part + psize; ++j)
				{
					DVector b = GetVertex(j);
					double dx, dy, d;
					DVector resV;

					if (PrumetNaUsecku(vx, a, b, resV, d))
					{
						if (d < nearest)
						{
							nearest = d;
							nearestPt = resV;
						}
					}
					else
					{
						dx = vx.x - b.x;
						dy = vx.y - b.y;
						d = dx * dx + dy * dy;
						if (d < nearest)
						{
							nearest   = d;
							nearestPt = b;
						}
					}
					a = b;
				}
			}
			return nearestPt;      
	    }

		///Creates copy of the shape
		/**
		* @copydoc ShapeMultiPoint::NewInstance 
		*/
		virtual ShapePoly* NewInstance(const ShapePoly& other) const
		{
			return new ShapePoly(other);
		}

		virtual IShape* Clone(VertexArray* vxArr = 0)
		{
			AutoArray<unsigned int, MemAllocLocal<unsigned int, 64> > newParts;
			AutoArray<unsigned int, MemAllocLocal<unsigned int, 64> > newPoints;
			newParts  = _parts;
			newPoints = _points;
			if (vxArr != 0)
			{
				for (int i = 0; i <= newPoints.Size(); i++)
				{
					newPoints[i] = vxArr->AddVertex(_vertices->operator [](newPoints[i]));
				}
			}
			return NewInstance(ShapePoly(newPoints, newParts, vxArr == 0 ? _vertices : vxArr));
		}

		///Function helps to calculating clipping of the shape
		/**
		* Function is called from derived classes to make clipping and generate
		* clip points.
		* @param xn x-coordinate of bisector normal
		* @param yn y-coordinate of bisector normal
		* @param pos position of bisector
		* @param vxArr vertex array associated with new created points
		* @param newParts (out) array that receives indexes of newly created parts
		* @param newPoints (out) array that receives indexes of newly created points
		*/
		void ClipHelp(double xn, double yn, double pos, VertexArray* vxArr,
					  AutoArray<unsigned int, MemAllocLocal<unsigned int, 64> > &newParts,
					  AutoArray<unsigned int, MemAllocLocal<unsigned int, 64> > &newPoints) const
		{
			if (vxArr == 0) vxArr = _vertices;
			for (int i = 0, cnt = GetPartCount(); i <cnt; i++)
			{
				int start = GetPartIndex(i);
				int sz = GetPartSize(i);
				int lastSide = 0;
				bool np = true;
				for (int j = 0; j < sz; j++)
				{
					const DVertex& vx = GetVertex(j + start);
					int side = Side(xn, yn, pos, vx.x, vx.y);
					if (lastSide >= 0 && side > 0 || side == 0)
					{
						if (np)
						{
							newParts.Add(newPoints.Size());
							np = false;
						}
						newPoints.Add(vxArr == _vertices ? GetIndex(j) : vxArr->AddVertex(vx));
					}
					if (lastSide >= 0 && side < 0)
					{
						if (lastSide > 0)
						{
							DVertex cp = ClipPoint(GetVertex(j + start - 1), vx, xn, yn, pos);              
							newPoints.Add(vxArr->AddVertex(cp));
						}
						np = true;
					}
					if (lastSide < 0 && side > 0)
					{            
						newParts.Add(newPoints.Size());
						np = false;
						DVertex cp = ClipPoint(GetVertex(j + start - 1), vx, xn, yn, pos);
						newPoints.Add(vxArr->AddVertex(cp));
						newPoints.Add(vxArr == _vertices ? GetIndex(j) : vxArr->AddVertex(vx));
					}
					lastSide = side;
				}
			}
		}

		///retrieves shape perimeter
		/**
		@return shape perimeter
		*/
		double GetPerimeter(bool closed) const 
		{
			double perimeter = 0.0f;

			for (int i = 0, cnt = GetPartCount(); i < cnt; i++)
			{
				int start = GetPartIndex(i);
				int count = GetPartSize(i);
				for (int j = 0, cnt = count - 1; j < cnt; j++)
				{
					ShapeFiles::DVertex v1 = GetVertex(j + start);
					ShapeFiles::DVertex v2 = GetVertex(i + start + 1);          
					perimeter += sqrt(v1.DistanceXY2(v2));
				}

				if (closed && count) 
				{
					ShapeFiles::DVertex v1 = GetVertex(start);
					ShapeFiles::DVertex v2 = GetVertex(start + count - 1);
					perimeter += sqrt(v1.DistanceXY2(v2));
				}
			}
			return perimeter;
		}

		virtual ShapePoly* ExtractPart(unsigned int part) const 
		{
			if (part >= GetPartCount()) return 0;
			unsigned int zero = 0;
			return NewInstance(ShapePoly(Array<unsigned int>(const_cast<unsigned int*>(_points.Data()) + GetPartIndex(part), GetPartSize(part)), Array<unsigned int>(&zero, 1), _vertices));    
		}
	};
}
