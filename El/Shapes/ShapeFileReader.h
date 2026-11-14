#pragma once
#include "EL\qstream\qStream.hpp"

namespace ShapeFiles
{
	class VertexArray;
	class ShapeList;
	class IShape;
	class ShapePoint;
	class ShapeMultiPoint;
	class ShapePoly;

	class ShapeFileReader
	{
	public:
		struct ShapeHeader
		{
			unsigned long fileCode;
			unsigned long fileLength;
			unsigned long version;
			unsigned long shapeType;
			double Xmin,Ymin,Xmax,Ymax,Zmin,Zmax,Mmin,Mmax;
		};
	protected:
		ShapeHeader _header;
    
	public:
		enum ShapeReaderError
		{
			sheOk=0,
			sheReadError=1,
			sheUnexceptedEndOfFile=2,
			sheOpenError=3,
			sheFileTagMismach=4,
			sheVersionNotSupported=5
		};

//		ShapeReaderError ReadShapeFile(std::istream& input, VertexArray &vxArray, ShapeList &shapeList, AutoArray<unsigned int>* indexList = 0);
    ShapeReaderError ReadShapeFile(QIStream& input, VertexArray &vxArray, ShapeList &shapeList, AutoArray<unsigned int>* indexList = 0);
		const ShapeHeader& GetShapeFileHeader() const 
		{
			return _header;
		}
		ShapeReaderError ReadShapeFile(const _TCHAR* filename, VertexArray &vxArray, ShapeList &shapeList, AutoArray<unsigned int>* indexList = 0);

	public:
		virtual IShape* CreatePoint(const ShapePoint& origShape);
		virtual IShape* CreateMultipoint(const ShapeMultiPoint& origShape);
		virtual IShape* CreatePolyLine(const ShapePoly& origShape);
		virtual IShape* CreatePolygon(const ShapePoly& origShape);
	};
}
