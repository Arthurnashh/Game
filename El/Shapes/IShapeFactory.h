#pragma once

namespace ShapeFiles
{
	class ShapePoint;
	class ShapeMultiPoint;
	class ShapePoly;

	class IShapeFactory
	{
	public:
		virtual IShape* CreatePoint(const ShapePoint& origShape) = 0;
		virtual IShape* CreateMultipoint(const ShapeMultiPoint& origShape) = 0;
		virtual IShape* CreatePolyLine(const ShapePoly& origShape) = 0;
		virtual IShape* CreatePolygon(const ShapePoly& origShape) = 0;
	};
}