#ifdef _MSC_VER
#pragma once
#endif

#ifndef __SWIZZLED_ARRAY_2D_HPP
#define __SWIZZLED_ARRAY_2D_HPP

#include <Es/Common/fltopts.hpp>

struct Plain2DTo1D
{
  __forceinline static int To1D(int x, int y, int xRange, int yRange)
  {
    // plain 2D layout - row major
    return x+y*xRange;
  }
  
  /// convert from linear to swizzled
  template <class Type>
  static void Swizzle(Type *data, int xRange, int yRange) {}
  /// convert from swizzled to linear
  template <class Type>
  static void Unswizzle(Type *data, int xRange, int yRange) {}
};

template <int logSize=2>
struct Swizzled2DTo1D
{
  static const int swizzledSize = 1<<logSize;
  static const int swizzledMask = swizzledSize-1;
  
  __forceinline static int To1D(int x, int y, int xRange, int yRange)
  {
    // index is constructed as follows:
    
    // y>>logSize x>>logSize y%swizzledSize x%swizzledSize
    
    #if _DEBUG
    // naive implementation
    int naive = ( 
      (y>>logSize)*(swizzledSize*xRange) +
      (x>>logSize)*(swizzledSize*swizzledSize) +
      (y&swizzledMask)*swizzledSize +
      (x&swizzledMask)
    );
    #endif
    int swizzled =
    (
      ((y&~swizzledMask)*xRange)|
      //((x&~swizzledMask)<<logSize)|
      //((y&swizzledMask)<<logSize)|
      (((x&~swizzledMask)|(y&swizzledMask))<<logSize)|
      (x&swizzledMask)
    );
    Assert(naive==swizzled);
    return swizzled;
  }


  template <class Type>
  static void Swizzle(Type *data, int xRange, int yRange)
  {
    // local space for a few rows is needed
    AutoArray<Type> local;
    local.Realloc(xRange*swizzledSize);
    local.Resize(xRange*swizzledSize);
    
    // process a bunch of lines
    for (int y=0; y<yRange; y+=swizzledSize)
    {
      // first create a local copy
      for (int yy=0; yy<swizzledSize; yy++) for (int x=0; x<xRange; x++)
      {
        local[x+yy*xRange] = data[x+(y+yy)*xRange];
      }
      // then swizzle back into the destination
      for (int yy=0; yy<swizzledSize; yy++) for (int x=0; x<xRange; x++)
      {
        data[To1D(x,y+yy,xRange,yRange)] = local[x+yy*xRange];
      }
    }
  
  }
  /// convert from swizzled to linear
  template <class Type>
  static void Unswizzle(Type *data, int xRange, int yRange)
  {
    Fail("Not implemented")
  }

};
//! two-dimensional dynamic array
/*!
AutoArray is used as underlying type
2D access is done on top of ir
*/

template
<
	class Type,class Allocator=MemAllocD,
	class Array1D=AutoArray<Type,Allocator>,
	class ConvertIndex=Plain2DTo1D
>
class Array2D
{
	//! actual data storage
	Array1D _data;
	//! array dimensions
	int _xRng;
	//! array dimensions
	int _yRng;

	public:
	Array2D();
	~Array2D();

	//! resize array
	void Dim(int x, int y);
	//! get array size
	int GetXRange() const {return _xRng;}
	//! get array size
	int GetYRange() const {return _yRng;}

	//! clear array
	void Clear()
	{
		_xRng=_yRng=0;
		_data.Clear();
	}
	//! read and write access
	__forceinline Type &Set(int x, int y)
	{
	  Assert(x>=0 && x<_xRng);
	  Assert(y>=0 && y<_yRng);
		return _data[ConvertIndex::To1D(x,y,_xRng,_yRng)];
	}
	//! read only access
	__forceinline const Type &Get(int x, int y) const
	{
	  Assert(x>=0 && x<_xRng);
	  Assert(y>=0 && y<_yRng);
		return _data[ConvertIndex::To1D(x,y,_xRng,_yRng)];
	}
  //! read only access - copy the value
  __forceinline Type GetValue(int x, int y) const
  {
	  Assert(x>=0 && x<_xRng);
	  Assert(y>=0 && y<_yRng);
    return _data[ConvertIndex::To1D(x,y,_xRng,_yRng)];
  }
	//! read only access
	const Type &GetClamped(int x, int y) const
	{
	  saturate(x,0,_xRng-1);
	  saturate(y,0,_yRng-1);
		return _data[ConvertIndex::To1D(x,y,_xRng,_yRng)];
	}
	//! read and write access
	__forceinline Type &operator ()(int x, int y)
	{
		return Set(x,y);
	}
	//! read only access
	__forceinline const Type &operator ()(int x, int y) const
	{
		return Get(x,y);
	}
	//@{ 1D access
	const Type *Data1D() const {return _data.Data();}
	Type *Data1D() {return _data.Data();}
	int Size1D() const {return _data.Size();}
	//@}
	
	//@{ raw (binary) access
	const void *RawData() const {return _data.Data();}
	void *RawData() {return _data.Data();}
	int RawSize() const {return _data.Size()*sizeof(Type);}
	//@}
	
	/// copy source to destination, clear source
	void Move(Array2D &src)
	{
	  Clear();
	  src.MoveTo(_data,_xRng,_yRng);
	}

	void MoveTo(Array1D &data, int &xRng, int &yRng)
	{
	  data.Move(_data);
	  xRng = _xRng;
	  yRng = _yRng;
	}
	
	template <class Source>
	void Move(Source &src)
	{
	  // in a typical scenario source data are not swizzled and Unswizzle is empty
	  src.Unswizzle();
	  // we assume underlying data can be moved
	  Clear();
	  src.MoveTo(_data,_xRng,_yRng);
	  // the data may need swizzling
	  Swizzle();
	}
	
	void Swizzle()
	{
	  ConvertIndex::Swizzle(_data.Data(),_xRng,_yRng);
	}

	void Unswizzle()
	{
	  ConvertIndex::Unswizzle(_data.Data(),_xRng,_yRng);
	}
	
	//// copy from other 2D array
	/**
	Note: copy from an identical 2D array is handled by a default (compiler generated) version (cf. 12.8/9 footnote 109):	
	*/
	template <class Source>
	void operator = (const Source &src)
	{
	  for (int y=0; y<src.GetXRange(); y++) for (int x=0; x<src.GetXRange(); x++)
	  {
	    Set(x,y) = src.Get(x,y);
	  }
	}
	
	
};

template <class Type,class Allocator, class Array1D, class LinearIndex>
void Array2D<Type,Allocator,Array1D,LinearIndex>::Dim(int x, int y)
{
  // if the data size has not changed, no need to realloc
	if (x*y!=_xRng*_yRng)
	{
	  // Realloc might cause fragmentation, as both old and new memory must exist at the same time
	  // release first
	  _data.Clear();
	  // allocate later
	  _data.Realloc(x*y);
	}
  _xRng = x;
  _yRng = y;
	// make sure all elements are fresh constructed
	_data.Resize(0);
	_data.Resize(x*y);
}

template <class Type,class Allocator, class Array1D, class LinearIndex>
Array2D<Type,Allocator,Array1D,LinearIndex>::Array2D()
{
	_xRng = _yRng = 0;
}

template <class Type,class Allocator, class Array1D, class LinearIndex>
Array2D<Type,Allocator,Array1D,LinearIndex>::~Array2D()
{
	Clear();
}

#if _DEBUG

// verify basic functionality
struct Array2DUnitTest
{
  Array2DUnitTest()
  {
    Array2D<int, MemAllocD, AutoArray<int,MemAllocD>, Swizzled2DTo1D<2> > bigArray;
    bigArray.Dim(16,16);
    for (int z=0; z<16; z++) for (int x=0; x<16; x++)
    {
      bigArray(x,z) = x+z*16;
    }
    for (int z=0; z<16; z++) for (int x=0; x<16; x++)
    {
      Assert( bigArray(x,z) == x+z*16);
    }

    Array2D<int, MemAllocD, AutoArray<int,MemAllocD>, Swizzled2DTo1D<2> > swizzledArray;
    
    swizzledArray.Dim(16,16);
    for (int z=0; z<16; z++) for (int x=0; x<16; x++)
    {
      swizzledArray.Data1D()[x+z*16] = x+z*16;
    }
    
    swizzledArray.Swizzle();
    for (int z=0; z<16; z++) for (int x=0; x<16; x++)
    {
      Assert( bigArray(x,z) == swizzledArray(x,z));
    }
  }
};

// to perform the test use: static Array2DUnitTest executeArray2DUnitTest;

#endif

#endif

