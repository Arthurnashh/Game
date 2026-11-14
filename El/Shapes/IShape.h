#pragma once

#include "Vertex.h"
#include "El\Interfaces\IMultiInterfaceBase.hpp"

namespace ShapeFiles
{
	///Interface that introduces services for shapes (from SHP shapefile)
	/**
	* This interface is general for various shapes in shape file. To 
	* use it, you must derive this interface, or ShapeBase class and 
	* implement all abstract functions.
	*
	* @see ShapePoint, ShapeMultiPoint, ShapePolygon, ShapePolyLine
	* 
	*/
	class IShape : public IMultiInterfaceBase
	{
	protected:
		VertexArray* _vertices;
    DBox _boundingBox;
	public:

		///Contructor
		/**
		 * @param vertices Pointer to instance of vertex array. Each shape must be connected to a vertex arrat
		 *  It is legal to construct shape not connected with vertex array, but you should call SetVertexArray 
		 *  brefore shape is used
		 */
		IShape(VertexArray* vertices = 0) 
		: _vertices(vertices) 
		{
		}

		virtual ~IShape(void) 
		{
		}

		///Returns count of vertices
		/**
		 * @return total count of vertices allocated by the shape 
		 */    
		virtual unsigned int GetVertexCount() const = 0;

		///Returns count of parts
		/**
		 * Some types of shapes can consist from parts. Part can be one point, one polyline, or 
		 * one polygon. At that, there can be more points, polylines, or polygons in one shape. 
		 * In example of polygon, one instance can contain two polygons, where the first is 
		 * solid polygon and second is hole in the first polygon.
		 *
		 * @return Function returns count of parts. Each shape has at least 1 part, only
		 * empty shape has 0 parts
		 */
		virtual unsigned int GetPartCount() const = 0;

		///Returns vertex index on specified position
		/**
		 * @param pos vertex position on the shape (for example vertex of polygon). This
		 * parameter must be greater or equal to zero and less than value returned by GetVertexCount()
		 * @return index to the vertex array.
		 */
		virtual unsigned int GetIndex(unsigned int pos) const = 0;

		///Returns vertex instance on given position
		/**
		  * @param pos vertex position on the shape (for example vertex of polygon). This
		  *  parameter must be greater or equal to zero and less them value returned by GetVertexCount()
		  * @return refrence to vertex. You should not remove const from that reference.
		  */
		const DVertex& GetVertex(unsigned int pos) const
		{
			return _vertices->GetVertex(GetIndex(pos));
		}

		bool SetVertex(unsigned int pos, const DVertex& vx)
		{
			return _vertices->SetVertex(GetIndex(pos), vx);
		}

		///Returns position offset of given part
		/**
		 * @param partId Identification of part in the shape. This parameter must be greater
		 * or equal to zero and less than value returned by GetPartCount() fuction
		 * @return Offset of first vertex position of specified part. Mostly first part
		 * starts at position 0, but seconds, third (and etc) may start on different offsets.
		 * First vertex of specified part is on result+0, second is on result+1, third=result+2, etc.
		 */
		virtual unsigned int GetPartIndex(unsigned int partId) const = 0;
    
		///Returns total vertices used byt specified part
		/**
		* @param partId Identification of part in the shape. This parameter must be greater
		* or equal to zero and less than value returned by GetPartCount() fuction
		* @return Count of vertices of given part. To process vertices of specified part,
		* get count of vertices by GetPartSize and get offset by GetPartIndex. The process
		* vertices from zero to count-1 and add the offset for each generated position
		*/
		virtual unsigned int GetPartSize(unsigned int partId) const = 0;
    
        ///Extracts part of shape as separate shape
        /**
        @param part index of part. Value must be less the GetPartCount.
        @return pointer to newly created shape. To destroy instance, call Release();
        */
        virtual IShape* ExtractPart(unsigned int part) const = 0;

		///Returns pointer to associated vertex array
		VertexArray* GetVertexArray() 
		{
			return _vertices;
		}

		///Returns pointer to associated vertex array
	    const VertexArray* GetVertexArray() const 
		{
			return _vertices;
		}

		///Function allowes to assign or change current vertex array
		/**
		 * @param vx pointer to new vertex array. New vertex array must have at least the
		 * same count of vertices as old one. This function is useful, when
		 * you assigned vertex array for first time, or for example when you created new array with
		 * transformed vertices and now you wish to apply these vertices on the shape.
		 */
		void SetVertexArray(VertexArray* vx) 
		{
			_vertices = vx;
		}

		///Function calculates the bounding box of the shape
		/**
		 * @return bounding box. There is no bounding box variable, so bounding box is calculated everytime when
		 *  function called
		 */
		virtual DBox CalcBoundingBox(bool forceRecalc = false) const
		{
      if(!forceRecalc && !_boundingBox.recalc)
        return _boundingBox;

			DVertex lo, hi;
			lo = hi = GetVertex(0);
			for (int i = 1, cnt = GetVertexCount(); i < cnt; i++)
			{
				const DVertex& cur = GetVertex(i);
				lo.SetMinimum(cur);
				hi.SetMaximum(cur);
			}
			return DBox(lo, hi);
		}
  
		///Function calculates the bounding box of the part in the shape
		/**
		* @return bounding box. There is no bounding box variable, so bounding box is calculated everytime when
		*  function called. It is recomended to store result, instead calling this function repeatly.
		*/
		virtual DBox CalcPartBoundingBox(unsigned int part) const
		{
			DVertex lo, hi;
			int partStart = GetPartIndex(part);
			lo = hi = GetVertex(partStart);
			for (int i = 1, cnt = GetPartSize(part); i < cnt; i++)
			{
				const DVertex& cur = GetVertex(partStart + i);
				lo.SetMinimum(cur);
				hi.SetMaximum(cur);
			}
			return DBox(lo, hi);
		}

		virtual void RotateAboutBarycenter(double angle)
		{
			DVertex barycenter = GetBarycenter();

			unsigned int numVertices = GetVertexCount();

			// if no vertices returns
			if(numVertices == 0) return;

			for (unsigned int i = 0; i < numVertices; ++i)
			{
				ShapeFiles::DVertex v = GetVertex(i);
				double dx = v.x - barycenter.x;
				double dy = v.y - barycenter.y;

				v.x = barycenter.x + dx * cos(angle) - dy * sin(angle);
				v.y = barycenter.y + dx * sin(angle) + dy * cos(angle);

				SetVertex(i, v);
			}
		}

		///Function tests, whether the specified point is inside of shape
		/**
		 * @param vx vertex position. Only X and Y is read. All shapes are 2D object, so 
		 * tests are provided on 2D space.
		 * @retval true point lies inside of shape
		 * @retval false point lies outside of shape     
		 */
		virtual bool PtInside(const DVector& vx) const = 0;
    
		///Function finds coordinates of nearest point that is inside of shape
		/**
		  * @param vx vertex position. Only X and Y is read.   All shapes are 2D object, so 
		  * tests are provided on 2D space.
		  * @return vertex position of nearest point that is inside or lies on edge of shape.
		  *  Only x and y members are valid. 
		  * @note Due float rounding error, returned point can fail PtInside test. That is
		  * not bug. In this case, you can assume, that returned vector is inside despite of 
		  * PtInside test.
		 */
		virtual DVector NearestInside(const DVector& vx) const = 0; 

		///Function finds coordinates of nearest point that is lying on edge
		/**
		  * @param vx vertex position. Only X and Y is read.   All shapes are 2D object, so 
		  * tests are provided on 2D space.
		  * @return vertex position of nearest point that lies on edge of shape.
		  *  Only x and y members are valid. 
          *
          * @note Only shapes with non-zero area can support this function. For other
          *   shape types, this function is same as NearestInside;
		 */
        virtual DVector NearestToEdge(const ShapeFiles::DVector& vx) const 
		{
			return NearestInside(vx);
		}

		enum IntersectionResult
		{
			irUnknown, ///<cannot handle. There is no relationship defined between objects
			irUkBounding, ///<cannot handle, but intersection is possible (bounding intersection)
			irNoIntersection, ///<no intersection
			irIntersection, ///<intersection found
			irIntersecInside, ///<intersection, this object is inside to other
			irIntersecOutside ///<intersection, this object is outside to other
		};

		///This function is currently disabled.
		virtual IntersectionResult TestIntersection(const IShape& other) const
		{
			if (other.CalcBoundingBox().IntersectionXY(CalcBoundingBox()))
			{
				return irUkBounding;
			}
			else
			{
				return irNoIntersection;
			}
		}

		///Clips shape
		/**
		 * Clips shape by bisector. All points or lines on other side than half-plane specified by
		 * normal vector are removed
		 * @param xn direction of the bisector normal, specifies side of half-plane
		 * @param yn direction of the bisector normal, specifies side of half-plane
		 * @param pos Position of the point that lies on the line and on the normal, 
		 *   divided by normal length (pos=-(x*xn+y*yn) where x and y is any point that
		 *   is lying on that bisector
		 * @param vxArr pointer to vertex array, that will be associated with newly shape. 
		 *    use 0 to associate current vertex array.
		 * @return pointer to new created shape. Retuns 0, if result shape is be empty.
		 */
		virtual IShape* Clip(double xn, double yn, double pos, VertexArray* vxArr = 0) const = 0;

		///Clones instance of the shape
		/**
		* @param vxArr pointer to vertex array, that will be associated with newly shape. 
		*    use 0 to associate current vertex array.
		* @return pointer to new created shape. 
		 */
		virtual IShape* Copy(VertexArray* vxArr = 0) const = 0;

		///Crops the shape by the rectangle
		/**
		 *  Crop uses Clip function to generate cropped shape. 
		 *  @param left x coordinate of left-top point
		 *  @param top y coordinate of left-top point
		 *  @param right x coordinate of right-bottom point
		 *  @param bottom y coordinate of right-bottom point
		 *  @param vxArr pointer to new vertex array that will receive new points. if vxArr is zero, current vertex array is used
		 *  @return pointer to new instance of shape. Returned pointer is allocated byt new operator by default. But derived class 
		 *   can override instance creation in NewInstance function. This gives you to implement own allocator, but 
		 *   detection which way has been used to allocate instance is only your job. Function returns 0, if 
		 *   shape is empty after cropping.
		 */
		IShape* Crop(double left, double top, double right, double bottom, VertexArray* vxArr = 0) const
		{
/*
      DBox& boundingBox = CalcBoundingBox();
      //map is not inside the bounding box, continue
      if(!boundingBox.PtInsideXY(DVector(left, top))
        && !boundingBox.PtInsideXY(DVector(left, bottom))
        && !boundingBox.PtInsideXY(DVector(right,  top))
        && !boundingBox.PtInsideXY(DVector(right, bottom))
        )
        return 0;
*/
			VertexArray tmp;
			IShape* stop = 0;
			IShape* topleft = 0;
			IShape* topright = 0;
			IShape* full = 0;
			stop = Clip(0, 1, -top, &tmp);
			if (stop == 0) return 0;
			topleft = stop->Clip(1, 0, -left, &tmp);
			if (topleft == 0) 
			{
				delete stop;
				return 0;
			}
			topright = topleft->Clip(-1, 0, right, &tmp);
			if (topright == 0) 
			{
				delete stop;
				delete topleft; 
				return 0;
			}      
			full = topright->Clip(0, -1, bottom, vxArr == 0 ? _vertices : vxArr);
			delete stop;
			delete topleft;
			delete topright;
			return full;
		}

        ///retrieves shape perimeter
        /**
        @return shape perimeter
        */
        virtual double GetPerimeter() const = 0;

        ///retrieves shape's area
        /**
        @return shape area
        */
        virtual double GetArea() const = 0;

        /// returns the barycenter of the given shape
        virtual DVertex GetBarycenter() const
        {
			ShapeFiles::DVertex barycenter(0.0, 0.0, 0.0);

			int numVertices = GetVertexCount();

			// if no vertices returns (0, 0, 0)
			if(numVertices == 0) return barycenter;

			for(int i = 0; i < numVertices; ++i)
			{
				ShapeFiles::DVertex v = GetVertex(i);
				barycenter.x += v.x;
				barycenter.y += v.y;
				barycenter.z += v.z;
			}

			barycenter.x /= numVertices;
			barycenter.y /= numVertices;
			barycenter.z /= numVertices;

			return barycenter;
        }

        virtual void Release() 
		{
			delete this;
		}
	};

    /// provide delete for an array pointer
    template <class Type>
    struct SRefReleaseTraits
    {
		static void Delete(Type* ptr) 
		{
			ptr->Release();
		}
    };

    typedef SRef<IShape, SRefReleaseTraits<IShape> > PShape;
}
