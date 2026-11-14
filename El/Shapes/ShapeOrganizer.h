#pragma once

#include "IShape.h"
#include "IShapeFactory.h"
#include "ShapeDatabase.h"
#include "ShapeList.h"
#include "DBFReader.h"
#include "el\Pathname\Pathname.h"
#include "el\MultiThread\ExceptionRegister.h"
class Matrix4;

namespace ShapeFiles
{
	class ShapeOrganizer
	{
		ShapeList      _listOfShapes;
		ShapeDatabase  _shapeData;
		unsigned int   _nextcpcountervalue; //next ID of constant parameters
		ShapeDatabase  _cp; //constant parameters;
		IShapeFactory* _factory;
		VertexArray    _vxList;
	public:
		ShapeOrganizer(IShapeFactory* factory = 0);
		~ShapeOrganizer(void);

		static const char* algorithmFieldName;
		static const char* cpFieldName;

		unsigned int AllocateNewConstantParamsID(const char* algorithm);
		void DeleteConstantParams(unsigned int ID);

		void SetConstantParam(unsigned int cpID, const char* name, const char* value);
		const char* GetConstantParam(unsigned int cpID, const char* name) const;  

		// -------------------------------------------------------------------------- //

		const ShapeDatabase& GetConstantParamDB() const
		{
			return _cp;
		}

		// -------------------------------------------------------------------------- //

		ShapeDatabase& GetConstantParamDB()
		{
			return _cp;
		}

		// -------------------------------------------------------------------------- //

		///Reads shape file and database
		/**
		*  @param name name of shape file. Extension should be .shp. Different extension
		* is allowed, bud database name is always assumed with .dbf extension;
		*  @param constantParamsID ID of allocated constant params. Call AllocNewConstantParamsID 
		* to allocate new ID, or use an existing ID (depends on usage). For "nocp" use -1
		*  @param dbconvert pointer to class that provides column name conversion. Use NULL for no conversion
		*  @retval true, successfully loaded
		*  @retval false, error occurred. Error is reported through exception register.
		*/
		bool ReadShapeFile(const Pathname& name, 
                           unsigned int constantParamsID, 
                           IDBColumnNameConvert* dbconvert = 0, 
                           const Pathname& forceDBChange = Pathname(PathNull),
                           AutoArray<unsigned int>* shapeIndexes = 0);

		///Changes CP of specified shape.
		bool SetShapeCP(unsigned int ShapeID, unsigned int constantParamsID);

		// -------------------------------------------------------------------------- //
	
		ShapeDatabase& GetShapeDatabase()
		{
			return _shapeData;
		}

		// -------------------------------------------------------------------------- //
	
		const ShapeDatabase& GetShapeDatabase() const
		{
			return _shapeData;
		}

		// -------------------------------------------------------------------------- //

		const char* GetShapeParameter(unsigned int ShapeID, const char* fieldname) const;
		const char* GetShapeConstParameter(unsigned int ShapeID, const char* fieldname) const;

		// -------------------------------------------------------------------------- //

		const char* operator()(unsigned int ShapeID, const char* fieldname) const
		{
			const char* res = GetShapeParameter(ShapeID, fieldname);
			if (res == 0) res = GetShapeConstParameter(ShapeID, fieldname);
			return res;
		}

		// -------------------------------------------------------------------------- //

		unsigned int GetShapeCP(unsigned int ShapeID) const;

		// -------------------------------------------------------------------------- //

		template<class Functor>
		int ForEachShape(const Functor& funct)
		{
			_listOfShapes.EnumShapes(funct)
		}

		// -------------------------------------------------------------------------- //

		ShapeList::Iter FirstShape() 
		{
			return _listOfShapes.First();
		}

		// -------------------------------------------------------------------------- //

		ShapeList::IterC FirstShape() const 
		{
			return _listOfShapes.First();
		}

		// -------------------------------------------------------------------------- //

		ShapeList::Iter LastShape() 
		{
			return _listOfShapes.Last();
		}

		// -------------------------------------------------------------------------- //

		ShapeList::IterC LastShape() const 
		{
			return _listOfShapes.Last();
		}

		// -------------------------------------------------------------------------- //

		ShapeList& GetShapeList() 
		{
			return _listOfShapes;
		}

		// -------------------------------------------------------------------------- //

		const ShapeList& GetShapeList() const 
		{
			return _listOfShapes;
		}

		// -------------------------------------------------------------------------- //

		unsigned int AddShape(IShape* shape, unsigned int constantParamsID);
		bool DeleteShape(unsigned int ID);

		// -------------------------------------------------------------------------- //

		unsigned int Size() const 
		{
			return _listOfShapes.Size();
		}    

		// -------------------------------------------------------------------------- //

		///Function removes unused constant params
		/**
		* After some shapes deleted, there can be unused CP ID's in database
		* This CP records can be deleted
		* @param report - if true, deleted CP are reported through exception register. Registered
		*  catcher can selective choose, which CP are important. Use fbIgnore to mark CP important
		*  preventing deletion
		*
		*/
		void DeleteUnusedCP(bool report);

		DBox CalcBoundingBox() const;
		void TransformVertices(const Matrix4 &mx);

		// -------------------------------------------------------------------------- //

		void SetShapeFactory(IShapeFactory* f) 
		{
			_factory = f;
		}

		// -------------------------------------------------------------------------- //

	public: ///Exception register classes
		class IBase : public ExceptReg::IException
		{
		public:
			virtual const _TCHAR* GetModule() const 
			{
				return "ShapeOrganizer";
			}    
		};

		// -------------------------------------------------------------------------- //

		class TwoParamsSpecificBase : public IBase
		{
		protected:
			unsigned int _error;
			const char* _filename;

		public:
			TwoParamsSpecificBase(unsigned int error, const char* filename) 
			: _error(error)
			, _filename(filename) 
			{
			}

			// -------------------------------------------------------------------------- //

			virtual unsigned int GetParamCount() const 
			{
				return 2;
			}

			// -------------------------------------------------------------------------- //

			virtual bool GetParam(unsigned int pos, const char* &param) const
			{
				if (pos == 1) 
				{
					param = _filename;
					return true;
				}
				else
				{
					return IBase::GetParam(pos, param);
				}
			}

			// -------------------------------------------------------------------------- //
	
			virtual bool GetParam(unsigned int pos, unsigned int& param) const
			{
				if (pos == 0) 
				{
					param = _error;
					return true;
				}
				else
				{
					return IBase::GetParam(pos, param);
				}
			}
		};

		// -------------------------------------------------------------------------- //
	
		class ShapeLoaderError : public TwoParamsSpecificBase
		{
			unsigned int _error;
			const char* _filename;

		public:
			ShapeLoaderError(unsigned int error, const char* filename) 
			: TwoParamsSpecificBase(error, filename) 
			{
			}

			// -------------------------------------------------------------------------- //

			virtual unsigned int GetCode() const 
			{
				return 1;
			}

			// -------------------------------------------------------------------------- //

			virtual const _TCHAR* GetType() const 
			{
				return "ShapeLoaderError";
			}

			// -------------------------------------------------------------------------- //

			virtual const _TCHAR* GetDesc() const 
			{
				return "Shape loader error %d at file %s";
			}
		};

		// -------------------------------------------------------------------------- //

		class DatabaseLoaderError : public TwoParamsSpecificBase
		{
			unsigned int _error;
			const char* _filename;

		public:
			DatabaseLoaderError(unsigned int error, const char* filename) 
			: TwoParamsSpecificBase(error, filename) 
			{
			}

			// -------------------------------------------------------------------------- //

			virtual unsigned int GetCode() const 
			{
				return 2;
			}

			// -------------------------------------------------------------------------- //

			virtual const _TCHAR* GetType() const 
			{
				return "DatabaseLoaderError";
			}

			// -------------------------------------------------------------------------- //

			virtual const _TCHAR* GetDesc() const 
			{
				return "DBF loader error %d at file %s";
			}
		};

		// -------------------------------------------------------------------------- //

		class UnusedCP : public IBase
		{
			unsigned int _unusedCP;
			ShapeOrganizer* _owner;
		public:

			UnusedCP(unsigned int unusedCP, ShapeOrganizer* owner) 
			: _unusedCP(unusedCP)
			, _owner(owner) 
			{
			}

			// -------------------------------------------------------------------------- //

			virtual const _TCHAR* GetType() const 
			{
				return "DeleteUnusedCP";
			}

			// -------------------------------------------------------------------------- //

			virtual const _TCHAR* GetDesc() const 
			{
				return "Request for deletion of CP %d";
			}

			// -------------------------------------------------------------------------- //

			virtual unsigned int GetCode() const 
			{
				return 3;
			}

			// -------------------------------------------------------------------------- //

			virtual unsigned int GetParamCount() const 
			{
				return 2;
			}

			// -------------------------------------------------------------------------- //

			virtual bool GetParam(unsigned int pos, void* &param) const
			{
				if (pos == 1) 
				{
					param = _owner;
					return true;
				}
				else
				{
					return IBase::GetParam(pos, param);
				}
			}

			// -------------------------------------------------------------------------- //
		
			virtual bool GetParam(unsigned int pos, unsigned int& param) const
			{
				if (pos == 0) 
				{
					param = _unusedCP;
					return true;
				}
				else
				{
					return IBase::GetParam(pos, param);
				}
			}
		};
	
		// -------------------------------------------------------------------------- //

        VertexArray& GetVertexArray() 
		{
			return _vxList;
		}

		// -------------------------------------------------------------------------- //
    
		const VertexArray& GetVertexArray() const 
		{
			return _vxList;
		}
    };
}

