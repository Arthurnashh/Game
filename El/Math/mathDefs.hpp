#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MATHDEFS_HPP
#define _MATHDEFS_HPP

// common KNI/P declarations

// different enum placeholders for different constructors

/// no init - initializes to NaN in debug, uninitialized otherwise
enum _noInit {NoInit};
/// guard agains uninitialized values by initalizing to NaN
/** this should typically not be used in Retail builds */
enum _guardInit {GuardInit};

enum _vRotate {VRotate};
enum _vFastTransform {VFastTransform};
enum _vFastTransformA {VFastTransformA};
enum _vPerspective {VPerspective};
enum _vMultiply {VMultiply};
enum _vMultiplyLeft {VMultiplyLeft};

//enum _mZero {MZero};
//enum _mIdentity {MIdentity};
enum _mTranslation {MTranslation};
enum _mTilda {MTilda};
enum _mRotationX {MRotationX};
enum _mRotationY {MRotationY};
enum _mRotationZ {MRotationZ};
enum _mRotationAxis {MRotationAxis};
enum _mScale {MScale};
enum _mPerspective {MPerspective};
enum _mDirection {MDirection};
enum _mUpAndDirection {MUpAndDirection};
enum _mView {MView};
enum _mMultiply {MMultiply};
enum _mInverseRotation {MInverseRotation};
enum _mInverseScaled {MInverseScaled};
enum _mInverseGeneral {MInverseGeneral};
enum _mNormalTransform {MNormalTransform};

typedef float Coord;
#define COORD_MIN ( -FLT_MAX )
#define COORD_MAX ( +FLT_MAX )
#define coord(x) (float)(x)
	
#define H_2PI ( 6.28318530717958647692f )
#define H_PI  ( 3.14159265358979323846f )
#define H_PI2 ( 1.57079632679489661923f ) 
#define H_PI4 ( 0.78539816339744830961f ) 
#define H_SQRT2 ( 1.4142135624f )
#define H_INVSQRT2 ( 0.70710678119f )

#define METER_TO_FEET(x) (x * 3.28083f)
#define METER_TO_KNOT(x) (x * 1.94384f)
#define METER_TO_KM_PER_HOUR(x) (x * 3.6f)
#define METER_TO_NAUTICAL_MILE(x) (x * 0.000539956803f)
#define METER_TO_KILOMETER(x) (x * 0.001f)
#define METER_TO_MILE(x) (x * 0.000621371192f)
#define METER_TO_MILE_PER_HOUR(x) (x * 2.23693629f)
#define METER_TO_FEET_PER_MINIT(x) (x * 1.9685f)
//


#endif
