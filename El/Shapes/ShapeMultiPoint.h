#pragma once
#include "ShapeBase.h"

namespace ShapeFiles
{
	///Shape that represents one point
	/**
	* Shape consist on list of points. Each point is reported as part with one point. 
	* The shape has the same part count as vertex count and each part has one vertex.
	*/
	class ShapeMultiPoint : public ShapeBase
	{
	protected:
		AutoArray<unsigned int> _points;

	public:

		ShapeMultiPoint(const Array<unsigned int>& points = Array<unsigned int>(0, 0), VertexArray* vx = 0)
		: ShapeBase(vx) 
		{
			_points.Copy(points.Data(), points.Size());
		}

		virtual unsigned int GetVertexCount() const 
		{
			return _points.Size();
		}

		virtual unsigned int GetPartCount() const 
		{
			return _points.Size();
		}

		virtual unsigned int GetIndex(unsigned int pos) const 
		{
			return _points[pos];
		}

		virtual unsigned int GetPartIndex(unsigned int partId) const 
		{
			return partId;
		}

		virtual unsigned int GetPartSize(unsigned int partId) const 
		{
			return 1;
		}

    virtual bool ReverseVertexList()
    {
		  AutoArray<unsigned int> newPoints;
      newPoints.Resize(GetVertexCount());
      for (unsigned int cnt = GetVertexCount(), i = cnt-1; i >= 0; --i)
        newPoints.AddFast(GetIndex(i));
      _points = newPoints;
      return true;
    }
    ///Function tests, whether the specified point is inside of shape
		/**
		* @copydoc IShape::PtInside
		* @note Function returns true, when vx is exactly the same as one of the points in this shape(with zero epsilon difference).
		*/
		virtual bool PtInside(const DVector& vx) const
		{
			for (unsigned int i = 0, cnt = GetVertexCount(); i < cnt; i++)
			{
				if (GetVertex(i).EqualXY(vx)) return true;
			}
			return false;
		}

		virtual DVector NearestInside(const DVector& vx) const
		{
			double distance = DBL_MAX;
			unsigned int best = -1;
			for (unsigned int i = 0, cnt = GetVertexCount(); i < cnt; i++)
			{
				double d = GetVertex(i).DistanceXY2(vx);
				if (d<distance)
				{
					best = i;
					distance = d;
				}
			}
			return best != -1 ? GetVertex(best) : DVector();
		}

		virtual IShape* Clip(double xn, double yn, double pos, VertexArray* vxArr = 0) const
		{
			Array<unsigned int> accepted((unsigned int*)alloca(sizeof(unsigned int)*_points.Size()), _points.Size());
			int used = 0;

			for (int i = 0; i < _points.Size(); i++)
			{      
				const DVertex& vx = GetVertex(i);
				if (Side(xn, yn, pos, vx.x, vx.y) >= 0) accepted[used++] = _points[i];
			}
			if (used == 0) return 0;
			accepted = Array<unsigned int>(accepted.Data(), used);
			if (vxArr)
			{
				for (int i = 0; i < accepted.Size(); i++) 
				{
					accepted[i] = vxArr->AddVertex(GetVertex(accepted[i]));
				}
				return NewInstance(ShapeMultiPoint(accepted, vxArr));
			}
			else
			{
				return NewInstance(ShapeMultiPoint(accepted, _vertices));
			}
		}

		///Creates copy of the shape
		/**
		* Derived class can reimplement this function to 
		* handle allocation by itself.
		* @return pointer to new instance. 
		*/
		virtual ShapeMultiPoint* NewInstance(const ShapeMultiPoint& other) const
		{
			return new ShapeMultiPoint(other);
		}

		virtual IShape* Copy(VertexArray* vxArr = 0) const
		{
			if (vxArr)
			{      
				AutoArray<unsigned int> accepted = _points;
				for (int i = 0; i < accepted.Size(); i++) 
				{
					accepted[i] = vxArr->AddVertex(GetVertex(accepted[i]));
				}
				return NewInstance(ShapeMultiPoint(accepted, vxArr));
			}
			else
			{
				return NewInstance(ShapeMultiPoint(_points, _vertices));
			}
		} 

		///retrieves shape perimeter
		/**
		@return shape perimeter
		*/
		virtual double GetPerimeter() const 
		{
			return 0;
		}

		///retrieves shape's area
		/**
		@return shape area
		*/
		virtual double GetArea() const 
		{
			return 0;
		}

		virtual ShapeMultiPoint* ExtractPart(unsigned int part) const 
		{
			if (part >= (unsigned)_points.Size()) return 0;
			return NewInstance(ShapeMultiPoint(Array<unsigned int>(const_cast<unsigned int*>(_points.Data()) + part, 1), _vertices));    
		}
	};
}
  