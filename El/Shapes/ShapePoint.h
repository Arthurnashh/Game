#pragma once
#include "ShapeBase.h"

namespace ShapeFiles
{
	///Shape that represents one point
	/**
	* This is the simplest shape. It represents one point. The shape has always one vertex 
	* position and always has one part.
	*/
	class ShapePoint : public ShapeBase
	{
		unsigned int _index;

	public:
		ShapePoint(unsigned int index = 0, VertexArray* vertices = 0)
		: _index(index)
		, ShapeBase(vertices) 
		{
		}
    
		virtual unsigned int GetVertexCount() const 
		{
			return 1;
		}

		virtual unsigned int GetPartCount() const 
		{
			return 1;
		}

		virtual unsigned int GetIndex(unsigned int pos) const 
		{
			return _index;
		}

		virtual unsigned int GetPartIndex(unsigned int partId) const 
		{
			return 0;
		}

		virtual unsigned int GetPartSize(unsigned int partId) const 
		{
			return 1;
		}

		///Function tests, whether the specified point is inside of shape
		/**
		* @copydoc IShape::PtInside
		* @note Function returns true, when vx is exactly the same as point returned by GetVertex(0) with epsilon difference.
		*/
		virtual bool PtInside(const DVector& vx) const
		{
			return GetVertex(0).EqualXY(vx);
		}

		virtual DVector NearestInside(const DVector& vx) const
		{
			return GetVertex(0);
		}

		virtual IShape* Clip(double xn, double yn, double pos, VertexArray* vxArr = 0) const
		{
			const DVertex& vx = GetVertex(0);      
			if (Side(xn, yn, pos, vx.x, vx.y) < 0) 
			{
				return 0;
			}
			else 
			{
				return Copy(vxArr);
			}
		}

		///Creates copy of the point
		/**
		* Derived class can reimplement this function to 
		* handle allocation by itself.
		* @return pointer to new instance. 
		*/
		virtual ShapePoint* NewInstance(const ShapePoint& orig) const
		{
			return new ShapePoint(orig);
		}

		virtual ShapePoint* Copy(VertexArray* vxArr = 0) const
		{
			if (vxArr == 0) 
			{
				return NewInstance(ShapePoint(_index, _vertices));
			}
			else 
			{
				return NewInstance(ShapePoint(vxArr->AddVertex(GetVertex(0)), vxArr));
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

		virtual ShapePoint* ExtractPart(unsigned int part) const 
		{
			if (part) return 0;
			return Copy();
		}
	};
}