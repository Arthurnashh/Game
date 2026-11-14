#pragma once
#include "shapepoly.h"

namespace ShapeFiles
{
	static const double ShapePolyLine_thinLineWidth = 0.0000000001;

	class ShapePolyLine : public ShapePoly
	{
	public:

		ShapePolyLine(const Array<unsigned int>& points = Array<unsigned int>(0, 0), 
					  const Array<unsigned int>& parts = Array<unsigned int>(0), VertexArray* vx = 0) 
		: ShapePoly(points, parts, vx) 
		{
		}

		ShapePolyLine(const ShapePoly& other) 
		: ShapePoly(other)
		{
		}

		virtual DVector NearestInside(const DVector& vx) const
		{
			return NearestToEdge(vx, false);
		}

		virtual bool PtInside(const DVector& vx) const
		{
			DVector res = NearestInside(vx);
			return (vx.x - res.x) * (vx.x - res.x) + (vx.x - res.y) * (vx.x - res.y) < (ShapePolyLine_thinLineWidth * ShapePolyLine_thinLineWidth);
		}

		virtual IShape* Clip(double xn, double yn, double pos, VertexArray* vxArr = 0) const
		{
			if (vxArr == 0) vxArr = _vertices;
			AutoArray<unsigned int, MemAllocLocal<unsigned int, 64> > newParts;
			AutoArray<unsigned int, MemAllocLocal<unsigned int, 64> > newPoints;
			ClipHelp(xn, yn, pos, vxArr, newParts, newPoints);
			if (newPoints.Size() == 0) 
			{
				return 0;
			}
			else 
			{
				return NewInstance(ShapePolyLine(newPoints, newParts, vxArr));
			}
		}

		virtual ShapePolyLine* NewInstance(const ShapePoly& other) const
		{
			return NewInstance(ShapePolyLine(other));
		}

		///Creates copy of the shape
		/**
		* @copydoc ShapeMultiPoint::NewInstance 
		*/
		virtual ShapePolyLine* NewInstance(const ShapePolyLine& other) const
		{
			return new ShapePolyLine(other);
		}

		///retrieves shape perimeter
		/**
		@return shape perimeter
		*/
		virtual double GetPerimeter() const 
		{
			return ShapePoly::GetPerimeter(false);
		}
	};
}
