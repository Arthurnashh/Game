#include <Es/essencepch.hpp>
#include "StdAfx.h"
#include ".\ShapeHelpers.h"

RString GetShapeType(const ShapeFiles::IShape* shape)
{
	const ShapeFiles::ShapePolygon* shapePolygon = dynamic_cast<const ShapeFiles::ShapePolygon*>(shape);
	if (shapePolygon)
	{
		return SHAPE_NAME_POLYGON;
	}

	const ShapeFiles::ShapePolyLine* shapePolyline = dynamic_cast<const ShapeFiles::ShapePolyLine*>(shape);
	if (shapePolyline)
	{
		return SHAPE_NAME_POLYLINE;
	}

	const ShapeFiles::ShapePoint* shapePoint = dynamic_cast<const ShapeFiles::ShapePoint*>(shape);
	if (shapePoint)
	{
		return SHAPE_NAME_POINT;
	}

	const ShapeFiles::ShapeMultiPoint* shapeMultiPoint = dynamic_cast<const ShapeFiles::ShapeMultiPoint*>(shape);
	if (shapeMultiPoint)
	{
		return SHAPE_NAME_MULTIPOINT;
	}

	return "";
}

// --------------------------------------------------------------------//

double GetShapePerimeter(const ShapeFiles::IShape* shape)
{
	RString type = GetShapeType(shape);
	if (type == SHAPE_NAME_POINT) return 0.0;
	if (type == SHAPE_NAME_MULTIPOINT) return 0.0;

	int vCount = shape->GetVertexCount();

	double perimeter = 0.0f;

	for (int i = 0; i < (vCount - 1); ++i)
	{
		ShapeFiles::DVertex v1 = shape->GetVertex(i);
		ShapeFiles::DVertex v2 = shape->GetVertex(i + 1);
		perimeter += sqrt(v1.DistanceXY2(v2));
	}

	if (type)
	{
		if (type == SHAPE_NAME_POLYGON)
		{
			// closes the polygon
			ShapeFiles::DVertex v1 = shape->GetVertex(0);
			ShapeFiles::DVertex v2 = shape->GetVertex(vCount - 1);
			perimeter += sqrt(v1.DistanceXY2(v2));
		}
	}
	return perimeter;
}

// --------------------------------------------------------------------//

double GetShapeArea(const ShapeFiles::IShape* shape)
{
	RString type = GetShapeType(shape);
	if (type == SHAPE_NAME_POLYGON)
	{
		// gets the number of vertices
		int numVertices = shape->GetVertexCount();
		
		double area = 0.0;

		// calculates area (gauss algorithm)
		for (int i = 0, j = (numVertices - 1); i < numVertices; j = i++)
		{
			ShapeFiles::DVertex v1 = shape->GetVertex(j);
			ShapeFiles::DVertex v2 = shape->GetVertex(i);

			area += v1.x * v2.y - v1.y * v2.x;
		}
		area /= 2.0;

		area = (area > 0.0) ? area : -area;

		return area;
	}
	else
	{
		return 0.0;
	}
}

// --------------------------------------------------------------------//

ShapeFiles::DVector NearestToEdge(const ShapeFiles::IShape* shape, const ShapeFiles::DVector& vx)
{
	RString type = GetShapeType(shape);
	if(type == SHAPE_NAME_POLYGON)
	{
		const ShapeFiles::ShapePolygon* shapePolygon = dynamic_cast<const ShapeFiles::ShapePolygon*>(shape);		
		return shapePolygon->NearestToEdge(vx, true);
	}
	else if(type == SHAPE_NAME_POLYLINE)
	{
		const ShapeFiles::ShapePolyLine* shapePolyline = dynamic_cast<const ShapeFiles::ShapePolyLine*>(shape);		
		return shapePolyline->NearestToEdge(vx, false);
	}
	return ShapeFiles::DVector(0.0, 0.0, 0.0);
}

// --------------------------------------------------------------------//

double RandomInRangeF(double min, double max)
{
	return min + (static_cast<double>(rand()) / static_cast<double>(RAND_MAX)) * (max - min);
}

// --------------------------------------------------------------------//

ShapeFiles::DVertex GetShapeBarycenter(const ShapeFiles::IShape* shape)
{
	ShapeFiles::DVertex barycenter(0.0, 0.0, 0.0);

	int numVertices = shape->GetVertexCount();

	// if no vertices returns (0, 0, 0)
	if (numVertices == 0) return barycenter;

	for (int i = 0; i < numVertices; ++i)
	{
		ShapeFiles::DVertex v = shape->GetVertex(i);
		barycenter.x += v.x;
		barycenter.y += v.y;
		barycenter.z += v.z;
	}

	barycenter.x /= numVertices;
	barycenter.y /= numVertices;
	barycenter.z /= numVertices;

	return barycenter;
}

// --------------------------------------------------------------------//

ShapeFiles::DVector GetPointOnShapeAt(const ShapeFiles::IShape* shape, double longCoordinate)
{
	ShapeFiles::DVector vNull(0.0, 0.0);

	RString type = GetShapeType(shape);
	if (type == SHAPE_NAME_POINT) return vNull;
	if (type == SHAPE_NAME_MULTIPOINT) return vNull;

	unsigned int numVertices = shape->GetVertexCount();
	if (numVertices == 0) return vNull;

  //double shapeLength = GetShapePerimeter(shape);
  double shapeLength = shape->GetPerimeter();

	if (longCoordinate < 0.0 || shapeLength < longCoordinate) return vNull;

  if (longCoordinate == 0.0)
	{
		ShapeFiles::DVertex v = shape->GetVertex(0);
		return ShapeFiles::DVector(v.x, v.y);
	}

	if (longCoordinate == shapeLength)
	{
		if (type == SHAPE_NAME_POLYLINE)
		{
			ShapeFiles::DVertex v = shape->GetVertex(numVertices - 1);
			return ShapeFiles::DVector(v.x, v.y);
		}
		else if (type == SHAPE_NAME_POLYGON)
		{
			ShapeFiles::DVertex v = shape->GetVertex(0);
			return ShapeFiles::DVector(v.x, v.y);
		}
	}

	double tempLength = 0.0;
	for (unsigned int i = 1; i < numVertices; ++i)
	{
		ShapeFiles::DVertex v0 = shape->GetVertex(i - 1);
		ShapeFiles::DVertex v1 = shape->GetVertex(i);
		ShapeFiles::DVector v01(v1, v0);

		double d = v01.SizeXY();
		if (tempLength + d > longCoordinate)
		{
			double pos = longCoordinate - tempLength;
			double angle = v01.DirectionXY();
			double x = v0.x + pos * cos(angle);
			double y = v0.y + pos * sin(angle);
			return ShapeFiles::DVector(x, y);
		}
		tempLength += d;
	}

	// we arrive here only if the shape is a polygon
	ShapeFiles::DVertex v0 = shape->GetVertex(numVertices - 1);
	ShapeFiles::DVertex v1 = shape->GetVertex(0);
	ShapeFiles::DVector v01(v1, v0);
	double pos = longCoordinate - tempLength;
	double angle = v01.DirectionXY();
	double x = v0.x + pos * cos(angle);
	double y = v0.y + pos * sin(angle);
	return ShapeFiles::DVector(x, y);
}