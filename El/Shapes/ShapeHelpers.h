#pragma once

#include "stdafx.h"

// #include ".\Vertex.h"

#include ".\IShape.h"
#include ".\ShapePolygon.h"
#include ".\ShapePolyline.h"
#include ".\ShapePoint.h"

#define SHAPE_NAME_POINT      "point"
#define SHAPE_NAME_MULTIPOINT "multipoint"
#define SHAPE_NAME_POLYLINE   "polyline"
#define SHAPE_NAME_POLYGON    "polygon"

#define DEPRECATED_FN(text) __declspec(deprecated(text))

struct ModuleObjectOutput
{
	int             type;
	ShapeFiles::DVector position;
	double          rotation;
	double          scaleX;
	double          scaleY;
	double          scaleZ;

	// used only by VLB for preview
	double          length;
	double          width;
	double          height;
	ClassIsMovable(ModuleObjectOutput); 
};

typedef AutoArray<ModuleObjectOutput, MemAllocLocal<ModuleObjectOutput, 32> > ModuleObjectOutputArray;

// GetShapeType()
// returns the type of the given Shape
RString GetShapeType(const ShapeFiles::IShape* shape);

// GetShapePerimeter()
// returns the perimeter of the given shape
DEPRECATED_FN("Function is deprecated. Use IShape::GetPerimeter")
double GetShapePerimeter(const ShapeFiles::IShape* shape);

// GetShapeArea()
// returns the area of the given shape
DEPRECATED_FN("Function is deprecated. Use IShape::GetArea")
double GetShapeArea(const ShapeFiles::IShape* shape);

// NearestToEdge()
// returns the result from NearestToEdge() if ShapePolygon or ShapePolyline
DEPRECATED_FN("Function is deprecated. Use IShape::NearestToEdge - it is implemented on IShape base")
ShapeFiles::DVector NearestToEdge(const ShapeFiles::IShape* shape, const ShapeFiles::DVector& vx);

// RandomInRangeF()
// returns a random float in the given range
double RandomInRangeF(double min, double max);

// GetShapeBarycenter()
// returns the barycenter of the given shape
DEPRECATED_FN("Function is deprecated. Use IShape::GetBarycenter")
ShapeFiles::DVertex GetShapeBarycenter(const ShapeFiles::IShape* shape);

// GetPointOnShapeAt()
// returns the point along the shape at the given longitudinal coordinate
ShapeFiles::DVector GetPointOnShapeAt(const ShapeFiles::IShape* shape, double longCoordinate);