#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CAMERA_HPP
#define _CAMERA_HPP

#include "visual.hpp"
#include "Shape/plane.hpp"

class Camera: public FrameBase // was virtual public Frame
{
  // camera is frame with projection
  friend class Scene;

  private:
  //Matrix4 _perspective;
  Matrix4 _projection; // result in (0,0) .. (w,h) range
  Matrix4 _projectionNormal; // result in (-1,-1) .. (+1,+1) range
  Matrix4 _scale; // scale scene so that viewing frustum is +,-1
  Matrix4 _invScale; // rescale back again
  //@{ precalculated AOT factor for mipmap loading
  float _aotFactor;
  float _aotInvFactor;
  //@}
  
  // temporary storage for object clipping
  Matrix4 _scaledInvTransform;

  // temporary storage for surface drawing
  Matrix3 _camNormalTrans;
  Matrix4 _camInvTrans;


  Coord _cNear,_cFar,_cLeft,_cTop; // perspective parameters
  /// far clipping plane for shadow rendering
  Coord _cFarShadow;
  /// far clipping plane for secondary shadow rendering
  Coord _cFarSecShadow;
  //! Far fog clipping plane (especially for cloud it has a sense to have farFog smaller than far clipping plane)
  Coord _cFarFog;
  Coord _invCTop,_invCLeft;

  

  //@{ view frustum clipping halfspace - oriented outwards
  Plane _rClipPlane,_lClipPlane; // world space clipping planes
  Plane _tClipPlane,_bClipPlane;
  Plane _nClipPlane,_fClipPlane; // near and far
  Plane _fClipPlaneShadow; // far clipping plane for shadows
  Plane _fClipPlaneSecShadow; // far clipping plane for secondary shadows
  //@}
  
  Vector3 _speed; // used for sound calculations
  VisualStateAge _age;
  
  public:
  // properties
  Camera();

  const Matrix4 &Projection() const
  {
    // if projection matrix was set using no engine, it contains zeros
    // while such matrix is valid, it such should never be used
    Assert(_projection(0,0)!=0);
    return _projection;
  }
  const Matrix4 &ProjectionNormal() const {return _projectionNormal;}
  const Matrix4 &ScaleMatrix() const {return _scale;}
  const Matrix4 &InvScaleMatrix() const {return _invScale;}
  void SetPerspective(
    Coord cNear, Coord cFar, Coord cLeft, Coord cTop,
    Coord cFarShadow, Coord cFarSecShadow, Coord cFarFog
  );
  void Adjust( Engine *engine );
  void SetClipRange(
    Coord cNear, Coord cFar, Coord cFarShadow, Coord cFarSecShadow, Coord cFarFog
  );
  
  struct ClipRangeState
  {
    float _cNear;
    float _cFar;
    float _cFarShadow;
    float _cFarSecShadow;
    float _cFarFog;
  };

  void SaveClipRange(ClipRangeState &state) const
  {
    state._cNear = _cNear;
    state._cFar = _cFar;
    state._cFarShadow = _cFarShadow;
    state._cFarSecShadow = _cFarSecShadow;
    state._cFarFog = _cFarFog;
  }
  void RestoreClipRange(const ClipRangeState &state)
  {
    SetClipRange(
      state._cNear,state._cFar,
      state._cFarShadow,state._cFarSecShadow,state._cFarFog
    );
  }
  
  const Plane &GetNearClipPlane() const {return _nClipPlane;}
  const Plane &GetFarClipPlane() const {return _fClipPlane;}
  
  Coord Near() const {return _cNear;}
  Coord Far() const {return _cFar;}
  Coord Left() const {return _cLeft;}
  Coord Top() const {return _cTop;}
  Coord InvTop() const {return _invCTop;}
  Coord InvLeft() const {return _invCLeft;}
  Coord ClipNear() const {return _cNear;}
  Coord ClipFar() const {return _cFar;}
  Coord ClipFarShadow() const {return _cFarShadow;}
  Coord ClipFarSecShadow() const {return _cFarSecShadow;}
  Coord ClipFarFog() const {return _cFarFog;}

  Vector3Val Speed() const {return _speed;} // used for sound calculations
  void SetSpeed( Vector3Par speed ) {_speed=speed;} // used for sound calculations

  VisualStateAge GetAge() const { return _age; }
  void SetAge(VisualStateAge val) { _age = val; }
  

  /// Get camera dependent AOT factor
  float GetAOTFactor() const {return _aotFactor;}
  /// Get camera dependent AOT factor - inverse
  float GetInvAOTFactor() const {return _aotInvFactor;}

	/// world space clipping - bounding box
	ClipFlags IsClipped(Matrix4Par modelToWorld, const Vector3 *minmax, ClipFlags &mayBeClipped) const;
	/// world space clipping - bounding box
	ClipFlags IsClipped(const Vector3 *minmax, ClipFlags mayBeClipped) const;
	/*!
	world space clipping
	\param point sphere center (world space)
	\param radius sphere radius
	\param userPlane user plane index (not used)
	\param mayBeClipped [out] if provided, contains planes
	in which clipping may happen based on bsphere check
	*/
  inline ClipFlags IsClipped(
    Vector3Par point, float radius, ClipFlags *mayBeClipped=NULL
  ) const
	{
		ClipFlags clip=ClipNone;

		float nClipPlaneDistance = _nClipPlane.Distance(point);
		float fClipPlaneDistance = _fClipPlane.Distance(point);
		float rClipPlaneDistance = _rClipPlane.Distance(point);
		float lClipPlaneDistance = _lClipPlane.Distance(point);
		float tClipPlaneDistance = _tClipPlane.Distance(point);
		float bClipPlaneDistance = _bClipPlane.Distance(point);

		if( nClipPlaneDistance<=-radius ) clip|=ClipFront;
		if( fClipPlaneDistance<=-radius ) clip|=ClipBack;
		if( rClipPlaneDistance<=-radius ) clip|=ClipRight;
		if( lClipPlaneDistance<=-radius ) clip|=ClipLeft;
		if( tClipPlaneDistance<=-radius ) clip|=ClipTop;
		if( bClipPlaneDistance<=-radius ) clip|=ClipBottom;

		if (mayBeClipped)
		{
			// no clip guranteed in given axis
			ClipFlags noClip=ClipNone;
			if( nClipPlaneDistance>+radius ) noClip|=ClipFront;
			if( fClipPlaneDistance>+radius ) noClip|=ClipBack;
			if( rClipPlaneDistance>+radius ) noClip|=ClipRight;
			if( lClipPlaneDistance>+radius ) noClip|=ClipLeft;
			if( tClipPlaneDistance>+radius ) noClip|=ClipTop;
			if( bClipPlaneDistance>+radius ) noClip|=ClipBottom;
			*mayBeClipped = noClip^ClipAll;
		}

		return clip;
	}  
  /// world space clipping against custom back plane
  ClipFlags IsClippedCustomBackPlane(
    Vector3Par point, float radius, const Plane &backPlane, ClipFlags *mayBeClipped
  ) const;
  /// check in which planes the direction is inbound (going from outside)
  ClipFlags IsInboundCustomBackPlane(Vector3Par dir, const Plane &backPlane) const;
  /// world space clipping of a line against custom back plane
  bool IsClippedCustomBackPlane(Vector3Par p1, Vector3Par p2, float radius, const Plane &backPlane) const;
  
  //@{ clipping against shadow plane
  ClipFlags IsClippedShadow(Vector3Par point, float radius, ClipFlags *mayBeClipped=NULL) const;
  ClipFlags IsClippedShadowCustomBackPlane(Vector3Par point, float radius, ClipFlags *mayBeClipped, float shadowsZ) const;
  ClipFlags IsInboundShadow(Vector3Par dir) const;
  ClipFlags IsInboundShadowCustomBackPlane(Vector3Par dir, float shadowsZ) const;
  bool IsClippedShadow(Vector3Par p1, Vector3Par p2, float radius) const;
  bool IsClippedShadowCustomBackPlane(Vector3Par p1, Vector3Par p2, float radius, float shadowsZ) const;
  //@}

  //@{ clipping against secondary shadow plane
  ClipFlags IsClippedSecShadow(
    Vector3Par point, float radius, ClipFlags *mayBeClipped=NULL
  ) const;
  ClipFlags IsInboundSecShadow(Vector3Par dir) const;
  bool IsClippedSecShadow(Vector3Par p1, Vector3Par p2, float radius) const;
  //@}

  /// check if whole object is clipped - world space
  ClipFlags IsClipped(
    Vector3Par point, float radius,
    Matrix4Par modelToWorld, const Vector3 *minmax
  ) const;
	/// faster than IsClipped above, can be used when it does not matter which plane clips
	bool IsClippedByAnyPlane(
		Vector3Par point, float radius,
		Matrix4Par modelToWorld, const Vector3 *minmax, ClipFlags &mayBeClipped
		) const;
	/// specialized version for world-space boxes
	inline bool IsClippedByAnyPlane(
		Vector3Par point, float radius, const Vector3 *minmax, ClipFlags &mayBeClipped
		) const
	{
		ClipFlags isClipped;
		isClipped = IsClipped(point,radius,&mayBeClipped);
		return isClipped!=0 || mayBeClipped!=0 && (mayBeClipped & IsClipped(minmax,mayBeClipped))!=0;
	}
	/// specialized version for world-space boxes
	inline bool IsClippedByAnyPlane(
		Vector3Par point, float radius, const Vector3 *minmax
		) const
	{
		ClipFlags isClipped,mayBeClipped;
		isClipped = IsClipped(point,radius,&mayBeClipped);
		return isClipped!=0 || mayBeClipped!=0 && (mayBeClipped & IsClipped(minmax,mayBeClipped))!=0;
	}

  /// calculate how much of the object min-max box is on-screen
  float LeftByClipping(
    Vector3Par point, float radius,
    Matrix4Par modelToWorld, const Vector3 *minmax
  ) const;
  /// check if some part of the object may be clipped
  ClipFlags MayBeClipped(Vector3Par point, float radius) const;
};

#endif
