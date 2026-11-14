#pragma once

namespace ShapeFiles
{

	class ShapeList
	{
		AutoArray<PShape> _shapeList;
		unsigned int _nextFree;
	public:

		ShapeList(void) : _nextFree(-1)
		{
		}

		unsigned int AddShape(const PShape& shape)
		{
			if (_nextFree!=-1)
			{
				unsigned int ret = _nextFree;
				_shapeList[_nextFree] = shape;
				for (unsigned int i = _nextFree + 1, cnt = _shapeList.Size(); i < cnt; ++i)
				{
					if (_shapeList[i].IsNull()) 
					{
						_nextFree = i;
						return ret;
					}
				}
				_nextFree = -1;
				return ret;
			}
			else
			{
				unsigned int ret = _shapeList.Size();
				_shapeList.Add(shape);
				return ret;
			}
		}

		void DeleteShape(unsigned int id)
		{
			_shapeList[id] = 0;
			_nextFree = __min(_nextFree, id);
			if (id == _shapeList.Size() - 1)
			{
				_shapeList.Resize(id - 1);
				if (_nextFree == id) _nextFree = -1;
			}
		}

		void SetShape(unsigned int id, const PShape& shape)
		{
			if (id > (unsigned)_shapeList.Size())
			{
				_nextFree = __min(_nextFree, (unsigned)_shapeList.Size());
			}
			_shapeList.Access(id);
			_shapeList[id]=shape;
		}

		const IShape* GetShape(unsigned int id) const
		{
			return _shapeList[id];
		}

		IShape* GetShape(unsigned int id)
		{
			return _shapeList[id];
		}

		template<class Functor>
		int EnumShapes(const Functor& funct) 
		{
			for (int i = 0; i < _shapeList.Size(); ++i)
			{
				if (_shapeList[i].NotNull())
				{
					int ret = funct(i, _shapeList[i]);
					if (ret) return ret;
				}
			}
			return 0;
		}

		template<class ShapeListType>
		class IterT
		{
			friend class ShapeList;
			unsigned int pos;
			ShapeListType* outer;

			IterT(ShapeListType* arr, unsigned int pos) : pos(pos), outer(arr) {}

		public:
			operator bool() 
			{
				return pos != -1;
			}
			IterT& operator ++ () 			
			{
				outer->Next(pos);
				return *this;
			}
			IterT& operator -- () 
			{
				outer->Prev(pos);
				return *this;
			}
			IterT operator ++ (int) 
			{
				int saveVx = pos;
				outer->Next(pos);
				return IterT<ShapeListType>(outer, saveVx);
			}
			IterT operator -- (int) 
			{
				int saveVx = pos;
				outer->Prev(pos);
				return IterT<ShapeListType>(outer, saveVx);
			}

			const IShape* GetShape() const 
			{
				return pos != -1 ? outer->GetShape(pos) : 0;
			}
			IShape* GetShape() 
			{
				return pos != -1 ? const_cast<IShape*>(outer->GetShape(pos)) : 0;
			}
			void SetShape(const SRef<PShape>& shape) 
			{ 
				if (pos != -1) outer->SetShape(pos, shape);
			}
			void DeleteShape() 
			{
				outer->DeleteShape(pos);
			}      
			unsigned int GetShapeId() const 
			{
				return pos;
			}
		};

		typedef IterT<ShapeList> Iter;
		typedef IterT<const ShapeList> IterC;

		friend class IterT<ShapeList>;
		friend class IterT<const ShapeList>;

		Iter First()
		{
			for (int i = 0; i < _shapeList.Size(); i++) 
			{
				if (_shapeList[i].NotNull()) return Iter(this, i);
			}
			return Iter(this, -1);
		}

		IterC First() const
		{
			for (int i = 0; i <_shapeList.Size(); ++i) 
			{
				if (_shapeList[i].NotNull()) return IterC(this, i);
			}
			return IterC(this, -1);
		}

		Iter Last()
		{
			for (int i = _shapeList.Size() - 1; i >= 0; i--) 
			{
				if (_shapeList[i].NotNull()) return Iter(this, i);
			}
			return Iter(this, -1);
		}

		IterC Last() const
		{
			for (int i = _shapeList.Size() - 1; i >= 0; i--) 
			{
				if (_shapeList[i].NotNull()) return IterC(this, i);
			}
			return IterC(this, -1);
		}

	protected:
		void Next(unsigned int& pos) const
		{
			if (pos == -1) return;
			while (++pos < (unsigned)_shapeList.Size())
			{
				if (_shapeList[pos].NotNull()) return;
			}
			pos = -1;
		}

		void Prev(unsigned int& pos) const
		{
			if (pos == -1) return;
			while (--pos != -1)
			{
				if (_shapeList[pos].NotNull()) return;        
			}
		}

    public:
		unsigned int Size() const
		{
			return _shapeList.Size();
		}
	};
}