#include <Es/essencepch.hpp>
#include "StdAfx.h"
#include "ShapeFileReader.h"
#include "DBFReader.h"
#include ".\shapeorganizer.h"

namespace ShapeFiles
{
	using namespace ExceptReg;

	// -------------------------------------------------------------------------- //

	ShapeOrganizer::ShapeOrganizer(IShapeFactory* factory) 
	: _factory(factory)
	, _nextcpcountervalue(1)
	{
	}

	// -------------------------------------------------------------------------- //

	ShapeOrganizer::~ShapeOrganizer(void)
	{
	}

	// -------------------------------------------------------------------------- //

	const char* ShapeOrganizer::algorithmFieldName = "~Algorithm";
	const char* ShapeOrganizer::cpFieldName = "~ConstParams";

	// -------------------------------------------------------------------------- //

	unsigned int ShapeOrganizer::AllocateNewConstantParamsID(const char* algorithm)
	{
		_cp.SetField(_nextcpcountervalue, algorithmFieldName, algorithm);
		return _nextcpcountervalue++;
	}

	// -------------------------------------------------------------------------- //

	void ShapeOrganizer::DeleteConstantParams(unsigned int ID)
	{
		_cp.DeleteShape(ID);
	}

	// -------------------------------------------------------------------------- //

	void ShapeOrganizer::SetConstantParam(unsigned int cpID, const char* name, const char* value)
	{
		_cp.SetField(cpID, name, value);
	}

	// -------------------------------------------------------------------------- //

	const char* ShapeOrganizer::GetConstantParam(unsigned int cpID, const char* name) const
	{
		return _cp.GetField(cpID, name);
	}

	// -------------------------------------------------------------------------- //

	bool ShapeOrganizer::SetShapeCP(unsigned int ShapeID, unsigned int constantParamsID)
	{
		char buff[50];
		sprintf(buff, "%X", constantParamsID);
		return _shapeData.SetField(ShapeID, cpFieldName, buff);
	}

	// -------------------------------------------------------------------------- //

	const char* ShapeOrganizer::GetShapeParameter(unsigned int ShapeID, const char* fieldname) const
	{
		return _shapeData.GetField(ShapeID, fieldname);
	}

	// -------------------------------------------------------------------------- //

	unsigned int ShapeOrganizer::GetShapeCP(unsigned int ShapeID) const
	{
		const char* c = _shapeData.GetField(ShapeID, cpFieldName);
		if (c == 0) return -1;
		unsigned int cpindex;
		if (sscanf(c, "%X", &cpindex) != 1) return -1;
		return cpindex;
	}

	// -------------------------------------------------------------------------- //

	const char* ShapeOrganizer::GetShapeConstParameter(unsigned int ShapeID, const char* fieldname) const
	{
		return GetConstantParam(GetShapeCP(ShapeID), fieldname);
	}

	// -------------------------------------------------------------------------- //

	unsigned int ShapeOrganizer::AddShape(IShape* shape, unsigned int constantParamsID)
	{
		unsigned int id = _listOfShapes.AddShape(shape);
		if (id != -1) SetShapeCP(id, constantParamsID);
		return id;
	}

	// -------------------------------------------------------------------------- //

	bool ShapeOrganizer::DeleteShape(unsigned int ID)
	{
		_listOfShapes.DeleteShape(ID);
		return _shapeData.DeleteShape(ID);
	}

	// -------------------------------------------------------------------------- //

	void ShapeOrganizer::DeleteUnusedCP(bool report)
	{
		BTree<unsigned int> usedCP;
		for (unsigned int curCP = -1; (curCP = _shapeData.GetNextUsedShapeID(curCP)) != -1;)
		{
			unsigned int cpid = GetShapeCP(curCP);
			usedCP.Add(cpid);
		}
		for (unsigned int curCP = -1; (curCP = _cp.GetNextUsedShapeID(curCP)) != -1;)
		{
			if (usedCP.Find(curCP) == 0)
			{
        //TODO: implement exceptions?
//        if (!report || RegExcpt(UnusedCP(curCP, this)) != ExceptionRegister::fbIgnore)
        if (!report)
				{
					_cp.DeleteShape(curCP);
				}
			}
		}
	}

	// -------------------------------------------------------------------------- //

	bool ShapeOrganizer::ReadShapeFile(const Pathname& name, 
                                       unsigned int constantParamsID, 
                                       IDBColumnNameConvert* dbconvert, 
                                       const Pathname& forceDBChange,
                                       AutoArray<unsigned int> *shapeIndexes)
	{
		class MyShapeFileReader : public ShapeFileReader
		{
			IShapeFactory* _factory;
		public:
			MyShapeFileReader(IShapeFactory* factory) 
			: _factory(factory) 
			{
			}

			// -------------------------------------------------------------------------- //

			virtual IShape* CreatePoint(const ShapePoint& origShape)
			{
				return _factory ? _factory->CreatePoint(origShape) : ShapeFileReader::CreatePoint(origShape);
			}

			// -------------------------------------------------------------------------- //

			virtual IShape* CreateMultipoint(const ShapeMultiPoint& origShape)
			{
				return _factory ? _factory->CreateMultipoint(origShape) : ShapeFileReader::CreateMultipoint(origShape);
			}

			// -------------------------------------------------------------------------- //

			virtual IShape* CreatePolyLine(const ShapePoly& origShape)
			{
				return _factory ? _factory->CreatePolyLine(origShape) : ShapeFileReader::CreatePolyLine(origShape);
			}

			// -------------------------------------------------------------------------- //

			virtual IShape* CreatePolygon(const ShapePoly& origShape)
			{
				return _factory ? _factory->CreatePolygon(origShape) : ShapeFileReader::CreatePolygon(origShape);
			}
		};

		// -------------------------------------------------------------------------- //

		MyShapeFileReader rdr(_factory);
		AutoArray<unsigned int> shpidx;
        if (shapeIndexes==0) shapeIndexes = &shpidx;
		ShapeFileReader::ShapeReaderError err = rdr.ReadShapeFile(name, _vxList, _listOfShapes, shapeIndexes);
		if (err) 
		{
//			RegExcpt(ShapeLoaderError(err, name));
		}
		else
		{
			Pathname dbfname = forceDBChange;
			if (dbfname.IsNull())
			{
				dbfname = name;
				dbfname.SetExtension(".dbf");
			}
			DBFReader dbfreader;
			DBFReader::ReaderError derr = dbfreader.ReadDBF(dbfname, _shapeData, *shapeIndexes, dbconvert);
			if (derr) 
			{
//				RegExcpt(DatabaseLoaderError(err,name));
			}
			else
			{
				for(int i = 0; i < shapeIndexes->Size(); i++)
				{
					SetShapeCP((*shapeIndexes)[i], constantParamsID);
				}
				return true;
			}
		}
		for(int i = 0; i < shapeIndexes->Size(); i++)
		{
			DeleteShape((*shapeIndexes)[i]);
		}
		return false;
	}
  
	// -------------------------------------------------------------------------- //
	
	DBox ShapeOrganizer::CalcBoundingBox() const
	{  
		DBox res(DVertex(0, 0), DVertex(0, 0));
		VertexArray::IterC iter = _vxList.First();
		if (iter)
		{
			res.lo = res.hi = iter.GetVertex();
			++iter;
			while (iter)
			{
				const DVertex& x = iter.GetVertex();
				res.lo.SetMinimum(x);
				res.hi.SetMaximum(x);
				++iter;
			}
		}
		return res;
	}

	// -------------------------------------------------------------------------- //

	void ShapeOrganizer::TransformVertices(const Matrix4& a)
	{
		for (VertexArray::Iter iter = _vxList.First(); iter; ++iter)
		{
			const DVertex& vx = iter.GetVertex();
			DVertex nx;
			nx.x = a(0, 0) * vx.x + a(0, 1) * vx.y + a(0, 2) * vx.z + a(0, 3);
			nx.y = a(1, 0) * vx.x + a(1, 1) * vx.y + a(1, 2) * vx.z + a(1, 3);
			nx.z = a(2, 0) * vx.x + a(2, 1) * vx.y + a(2, 2) * vx.z + a(2, 3);
			iter.SetVertex(nx);
		}
	}
}
