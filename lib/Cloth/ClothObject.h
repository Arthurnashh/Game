#ifdef _MSC_VER
#pragma once
#endif

// ClothObject.h: interface for the ClothObject class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _CLOTH_HPP
#define _CLOTH_HPP

#include "../head.hpp"

class ClothKnot
{
	public:
	Vector3 _pos,_vel,_norm; // position, velocity, normal
};

TypeIsSimpleZeroed(ClothKnot)

template <class Type>
class Array2DCloth
{
	Temp<Type> _data;
	int _w,_h;

	public:
	void Dim( int w, int h )
	{
		_w=w, _h=h;
		_data.Realloc(w*h);
	}
	void Clear() {_w=0, _h=0, _data.Free();}
	int W() const {return _w;}
	int H() const {return _h;}
	const Type &Get( int x, int y ) const {return _data[y*_w+x];}
	Type &Set( int x, int y ) {return _data[y*_w+x];}

	const Type &operator ()( int x, int y ) const {return _data[y*_w+x];}
	Type &operator ()( int x, int y ) {return _data[y*_w+x];}

	const Type &operator []( int offset ) const {return _data[offset];}
	Type &operator []( int offset ) {return _data[offset];}

	int CoordOffset( int x, int y ) const {return x+y*_w;}
	int XFromOffset( int id ) const {return id%_w;}
	int YFromOffset( int id ) const {return id/_w;}
};

class ClothObjectType
{
  friend class ClothObject;

	int _colPoints,_rowPoints;
	
	float _maxStep;

	float _stretchCoef;
	float _fricCoef;
	float _windCoef;
	float _gravCoef;
	
	float _xMin,_yMin;
	float _xSize, _ySize;
	
	public:
  void Init(ParamEntryPar config);
  void InitShape(float xMin, float yMin, float sizeX, float sizeY);

	float GetSizeX() const {return _xSize;}
	float GetSizeY() const {return _ySize;}
	float GetMinX() const {return _xMin;}
	float GetMinY() const {return _yMin;}
};

class ClothObject
{
private:

	Array2DCloth<ClothKnot> _knots;

  Vector3 _lastVelocity;
  Matrix4 _lastPos;

public:
	ClothObject();
	~ClothObject();

  void Init(const ClothObjectType &type, Matrix4Val pos);
	void InitPos(const ClothObjectType &type, Matrix4Val pos, Vector3Val vel );

	// get results
	Vector3Val GetPosition( float x, float y ) const;
	Vector3Val GetNormal( float x, float y ) const;


	void Simulate
	(
	  const ClothObjectType &type,
		Matrix4Val pos, // world space position of fixed points
		Vector3Val velocity, // world space velocity of fixed points
		float time, Vector3Par wind, Vector3Par inertia,
		SimulationImportance importance
	);
	// some knot may be fixed someplace
	void SetKnot
	(
		float x, float y,
		Vector3Par pos, Vector3Par vel, Vector3Par norm
	);

	bool IsConstraint(int x, int y);

  Matrix4 WorldPosUsed() const {return _lastPos;}
};

#endif
