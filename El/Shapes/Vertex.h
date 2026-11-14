#pragma once

namespace ShapeFiles
{
	struct DVector
	{
		double x, y, z;
    
		// -------------------------------------------------------------------------- //

		DVector(double x = 0, double y = 0, double z = DBL_MIN) 
		: x(x)
		, y(y)
		, z(z) 
		{
		}    

		// -------------------------------------------------------------------------- //

		DVector(const DVector& v2, const DVector& v1)
		{
			x = v2.x - v1.x;
			y = v2.y - v1.y;
			z = v2.z - v1.z;
		}

		// -------------------------------------------------------------------------- //

		bool IsZUsed() 
		{
			return z != DBL_MIN;
		}
		
		// -------------------------------------------------------------------------- //

		ClassIsSimpleZeroed(DVector);

		// -------------------------------------------------------------------------- //

		void SetMaximum(const DVector& other)
		{
			if (other.x > x) x = other.x;
			if (other.y > y) y = other.y;
			if (other.z > z) z = other.z;
		}

		// -------------------------------------------------------------------------- //

		void SetMinimum(const DVector& other)
		{
			if (other.x < x) x = other.x;
			if (other.y < y) y = other.y;
			if (other.z < z) z = other.z;
		}

		// -------------------------------------------------------------------------- //

		bool EqualXY(const DVector& other) const
		{
			return other.x == x && other.y == y;
		}

		// -------------------------------------------------------------------------- //

		double DotXY(const DVector& other) const
		{
			return other.x * x + other.y * y;
		}

		// -------------------------------------------------------------------------- //

		// right hand rule
		DVector Cross(const DVector& other) const
		{
		    return DVector(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
		}

		// -------------------------------------------------------------------------- //

		double SizeXY2() const
		{
			return (x * x + y * y);
		}

		// -------------------------------------------------------------------------- //

		double SizeXY() const
		{
			return sqrt(x * x + y * y);
		}

		// -------------------------------------------------------------------------- //

		double SizeXYZ() const
		{
			return sqrt(x * x + y * y + z * z);
		}

		// -------------------------------------------------------------------------- //

		double DistanceXY2(const DVector& other) const
		{
			return ((*this) - (other)).SizeXY2();
		}

		// -------------------------------------------------------------------------- //

		double DistanceXY(const DVector& other) const
		{
			return ((*this) - (other)).SizeXY();
		}

		// -------------------------------------------------------------------------- //

		DVector operator - (const DVector& other) const
		{
			return DVector(x - other.x, y - other.y, z - other.z);
		}

		// -------------------------------------------------------------------------- //

		DVector operator + (const DVector& other) const
		{
			return DVector(x + other.x, y + other.y, z + other.z);
		}

		// -------------------------------------------------------------------------- //

		DVector operator * (double scalar) const
		{
			return DVector(x * scalar, y * scalar, z * scalar);
		}

		// -------------------------------------------------------------------------- //

		DVector operator / (double scalar) const
		{
			return operator * (1.0/scalar);
		}

		// -------------------------------------------------------------------------- //

		DVector& operator += (const DVector& other) 
		{
			x += other.x;
			y += other.y;
			z += other.z;
			return *this;
		}

		// -------------------------------------------------------------------------- //

		DVector& operator -= (const DVector& other) 
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			return *this;
		}

		// -------------------------------------------------------------------------- //

		DVector& operator *= (double scalar) 
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			return *this;
		}

		// -------------------------------------------------------------------------- //

		DVector& operator /= (double scalar) 
		{
			if(scalar != 0.0)
			{
				double invScalar = 1.0 / scalar;
				x *= invScalar;
				y *= invScalar;
				z *= invScalar;
			}
			else
			{
				x = DBL_MAX;
				y = DBL_MAX;
				z = DBL_MAX;
			}
			return *this;
		}

		// -------------------------------------------------------------------------- //

		DVector& NormalizeXYInPlace()
		{
			*this /= SizeXY();
			return *this;
		}

		// -------------------------------------------------------------------------- //

		DVector PerpXYCW() const
		{
			return DVector(y, -x);
		}

		// -------------------------------------------------------------------------- //

		DVector PerpXYCCW() const
		{
			return DVector(-y, x);
		}

		// -------------------------------------------------------------------------- //

		double DirectionXY() const
		{
			double pi = 4.0 * atan(1.0);

			if(x == 0.0)
			{
				if(y > 0.0) 
				{
					return pi * 0.5;
				}
				else if(y < 0.0)
				{
					return pi * 3 * 0.5;
				}
				else
				{
					return 0.0;
				}
			}
			else
			{
				if(y == 0.0)
				{
					if(x > 0.0)
					{
						return 0.0;
					}
					else
					{
						return pi;
					}
				}
				else
				{
					double angle = atan(fabs(y / x));
					if(x < 0 && y > 0)
					{
						return (pi - angle);
					}
					if(x < 0 && y < 0)
					{
						return (angle + pi);
					}
					if(x > 0 && y < 0)
					{
						return (2.0 * pi - angle);
					}
					return angle;
				}
			}
		}

        // -------------------------------------------------------------------------- //

		ShapeFiles::DVector GetSecondEndXY(const ShapeFiles::DVector& firstEnd)
		{
			double angle  = DirectionXY();
			double length = SizeXY();
			double x = firstEnd.x + length * cos(angle); 
			double y = firstEnd.y + length * sin(angle);
			return ShapeFiles::DVector(x, y);
		}
	};

	// -------------------------------------------------------------------------- //

	struct DVertex : public DVector
	{
		double m;

		DVertex(double x = 0, double y = 0, double z = DBL_MIN, double m = DBL_MIN) 
		: DVector(x, y, z)
		, m(m) 
		{
		}

        // -------------------------------------------------------------------------- //

		DVertex(const DVector& vx, double m = DBL_MIN) 
		: DVector(vx)
		, m(m) 
		{
		}

        // -------------------------------------------------------------------------- //

		bool IsMUsed() 
		{
			return m != DBL_MIN;
		}

		// -------------------------------------------------------------------------- //

		ClassIsSimpleZeroed(DVertex);

		// -------------------------------------------------------------------------- //

		bool IsInvalidVertex() const;
		static const DVertex& GetInvalidVertex();

		// -------------------------------------------------------------------------- //

		void SetMaximum(const DVertex& other)
		{
			if (other.m > m) m = other.m;
			DVector::SetMaximum(other);
		}

		// -------------------------------------------------------------------------- //

		void SetMinimum(const DVertex& other)
		{
			if (other.m < m) m = other.m;
			DVector::SetMinimum(other);
		}
	};

	// -------------------------------------------------------------------------- //

	class VertexArray
	{
	protected:
		AutoArray<DVertex> _v;
		AutoArray<unsigned char> _flags;
	public:

		// -------------------------------------------------------------------------- //

		enum Flags
		{
			fUsed = 1
		};

		// -------------------------------------------------------------------------- //

	protected:
		unsigned int _nextFree;
		unsigned int GetNextFreeAtPos(unsigned int pos) const;
		unsigned int GetPrevFreeAtPos(unsigned int pos) const;
		void SetNextFreeAtPos(unsigned int pos, unsigned int nextFree);
		unsigned int AllocVertex();
		void FreeVertex(unsigned int pos);

		// -------------------------------------------------------------------------- //

	public:
		unsigned int Size() const 
		{
			return this ? (unsigned)_v.Size() : 0;
		}    

		// -------------------------------------------------------------------------- //

		VertexArray() 
		: _nextFree(0) 
		{
		}

		// -------------------------------------------------------------------------- //

		unsigned int AddVertex(const DVertex& vx = DVertex());
		bool DeleteVertex(unsigned int id);
		const DVertex& GetVertex(unsigned int pos) const;
		bool SetVertex(unsigned int pos, const DVertex& vx);

		// -------------------------------------------------------------------------- //

		const DVertex& operator [](unsigned int id) const
		{
			return GetVertex(id);
		}

		// -------------------------------------------------------------------------- //

		bool SetAt(unsigned int id, const DVertex& vx) 
		{
			return SetVertex(id, vx);
		}
    
		// -------------------------------------------------------------------------- //

		template <class VertexArrayType>
		class IterT
		{
			friend class VertexArray;
			unsigned int vxPos;
			VertexArrayType* outer;

			IterT(VertexArrayType* arr, unsigned long vxPos) : vxPos(vxPos), outer(arr) {}
		public:
			operator bool () 
			{
				return vxPos != -1;
			}

			IterT& operator ++ () 
			{
				outer->NextVertex(vxPos);
				return *this;
			}

			IterT& operator -- () 
			{
				outer->PrevVertex(vxPos);
				return *this;
			}

			IterT operator ++ (int) 
			{
				int saveVx = vxPos;
				outer->NextVertex(vxPos);
				return IterT<VertexArrayType>(outer, saveVx);
			}

			IterT operator -- (int) 
			{
				int saveVx = vxPos;
				outer->PrevVertex(vxPos);
				return IterT<VertexArrayType>(outer, saveVx);
			}

			const DVertex& GetVertex() const 
			{
				return outer->GetVertex(vxPos);
			}

			bool SetVertex(const DVertex& vx)
			{
				return outer->SetVertex(vxPos, vx);
			}

			bool DeleteVertex()
			{
				return outer->DeleteVertex(vxPos);
			}

			unsigned int Index() const
			{
				return vxPos;
			}
		};

		// -------------------------------------------------------------------------- //

		typedef IterT<VertexArray> Iter;
		typedef IterT<const VertexArray> IterC;

		friend class IterT<VertexArray>;
		friend class IterT<const VertexArray>;

		Iter First();
		IterC First() const;
		Iter Last();
		IterC Last() const;

	protected:
		void NextVertex(unsigned int& vxPos) const;
		void PrevVertex(unsigned int& vxPos) const;
	};

	// -------------------------------------------------------------------------- //

	struct DBox
	{
		DVertex lo;
		DVertex hi;
    bool recalc;
	public:

		DBox(const DVertex lo, const DVertex hi) : lo(lo), hi(hi), recalc(false) 
		{
		}

		// -------------------------------------------------------------------------- //

    DBox(): recalc(true) 
		{
		}

		// -------------------------------------------------------------------------- //

		bool PtInsideXY(const DVector& pt)const
		{
			return lo.x <= pt.x && hi.x > pt.y && lo.y <= pt.y && hi.y > pt.y;
		}

		// -------------------------------------------------------------------------- //

		bool IntersectionXY(const DBox& box) const
		{
			if (box.lo.x < hi.x && box.hi.x > lo.x && box.lo.y < hi.y && box.hi.y > lo.y) return true;
			return false;
		}
	};

	// --------------------------------------------------------------------//
}
