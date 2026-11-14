#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CAM_SHAKE_HPP
#define _CAM_SHAKE_HPP

#include "object.hpp"
#include "El/Math/mathStore.hpp"

/// source of camera shaking
class CameraShakeSource
{
public:
  typedef OLink(Object) TVehicleLink;

  /// type of the shake
  enum ShakeType
  {
    CSExplosion, ///< shake on explosion
    CSWeaponFire, ///< generic fire from weapon (like ear whistling -> weapon causes shake because fire is too loud)
    CSPlayerWeaponFire, ///< player fires from weapon  (shake caused by weapon recoil)
    CSPlayerHit, ///< shake on player hit
    CSVehicle, ///< shake caused by moving vehicle
    CSDamage, ///< shake caused by vehicle damage
    CSNone
  };

  CameraShakeSource();
  CameraShakeSource(CameraShakeSource::ShakeType type, float power, float duration, float frequency, TVehicleLink vehicle);

  bool IsExpired() const {return _curTime >= _duration;}
  float GetCurPower() const {return _curPower;}
  TVehicleLink GetVehicle() {return _vehicle;}
  ShakeType GetType() const {return _type;}
  float GetFrequency() const {return _frequency;}

  void Simulate(float deltaT);

  /// resets parameters for vehicle and restarts the shake
  void ResetVehicleParams(float power, float duration, float frequency);
private:
  /// computes current power
  float ComputeCurPower() const;

  /// type of the shake
  ShakeType _type;
  /// power of the source
  float _power;
  /// total duration of the shake (in s)
  float _duration;
  /// frequency of the camera changes (number of changes per sec)
  float _frequency;
  /// vehicle causing shake
  TVehicleLink _vehicle;

  /// current power 
  float _curPower;
  /// current time of the shake
  float _curTime;
};
TypeIsGeneric(CameraShakeSource);

//========================================================================================
/// params used for adding shake source
struct CameraShakeParams
{
  CameraShakeParams()
    :_shakeType(CameraShakeSource::CSNone), _pos(0,0,0), _ammoType(0), _forcePower(0), _forceDuration(0) {}
  CameraShakeParams(CameraShakeSource::ShakeType shakeType, const Vector3& pos, const AmmoType* const ammoType, 
    CameraShakeSource::TVehicleLink vehicle = CameraShakeSource::TVehicleLink())
    :_shakeType(shakeType), _pos(pos), _ammoType(ammoType), _forcePower(0),
    _forceDuration(0), _vehicle(vehicle) {}
  CameraShakeParams(CameraShakeSource::ShakeType shakeType, const Vector3& pos, RString ammoTypeName, 
    CameraShakeSource::TVehicleLink vehicle = CameraShakeSource::TVehicleLink())
    :_shakeType(shakeType), _pos(pos), _ammoTypeName(ammoTypeName), _ammoType(0), _forcePower(0), 
    _forceDuration(0), _vehicle(vehicle) {}

  /// type of the shake
  CameraShakeSource::ShakeType _shakeType;
  /// position of the shake source
  Vector3 _pos;
  /// if the source of shake is some vehicle (ie vehicle that produces loud noise)
  CameraShakeSource::TVehicleLink _vehicle;
  /// name of the ammo type (can be empty if ammoType is specified or no ammoType is used)
  RString _ammoTypeName;
  /// ammoType (if 0 and ammoTypeName is not empty, then ammoType is determined based on ammoTypeName)
  const AmmoType* _ammoType;
  /// if we want to force specific power of shake (if not 0, power will not be computed based on the params, but this value will be used)
  float _forcePower;
  /// if we want to force specific duration of shake (if not 0, duration will not be computed based on the params, but this value will be used)
  float _forceDuration;
};

//========================================================================================
/// camera shake manager
class CameraShakeManager
{
public: 
  /// coef used to multiply power to get positional change (in m)
  static float PosChangeCoef;
  /// coef used to multiply power to get rotation change in X (in degrees)
  static float RotXChangeCoef;
  /// coef used to multiply power to get rotation change in Y (in degrees)
  static float RotYChangeCoef;
  /// coef used to multiply power to get rotation change in Z (in degrees)
  static float RotZChangeCoef;
  /// if linear interpolation should be used between 2 camera positions
  static bool PerformLERP;

  /// if default values should be used when no values are present in config
  static bool UseDefaultValues;

  /// default power
  static float DefaultPower;
  /// default duration (in s)
  static float DefaultDuration;
  /// default max distance (in m)
  static float DefaultMaxDistance;
  /// default frequency of matrix update (number of updates per second)
  static float DefaultFrequency;
  /// default min speed of vehicle to cause shake
  static float DefaultMinSpeed;
  /// default min mass of vehicle to cause shake
  static float DefaultMinMass;
  /// default coef used to multiply caliber of ammo to get the coef for power and duration of shake used when the player is hit by ammo
  static float DefaultCaliberCoefPlayerHit;
  /// default coef used to multiply caliber of ammo to get the coef for power and duration of shake used when the weapon is fired
  static float DefaultCaliberCoefWeaponFire;
  /// default coef used to multiply default power of shake of vehicles passing near player
  static float DefaultPassingVehicleCoef;
  /// time between two updates of shake of passing vehicles
  static float DefaultPassingVehicleUpdateTime;
  /// default vehicle shake attenuation coef
  static float DefaultVehicleAttenuationCoef;

  /// global enable/disable of camera shakes
  static bool ShakeEnabled;

  CameraShakeManager();

  void Simulate(float deltaT);

  /// updates camera transformation based on the camera shake
  void UpdateCameraTransform(Matrix4& transform);
  /// gets matrix used to modify camera transformation
  const Matrix4& GetModMatrix() const {return _modMatrix;}

  /// adds shake source
  /// \param type type of the shake
  /// \param power power of the shake 
  /// \param duration duration of the shake (in s)
  /// \param frequency frequency of the camera changes (number of changes per sec)
  /// \param vehicle vehicle that is source of the shake (can be 0)
  void AddSource(CameraShakeSource::ShakeType type, float power, float duration, float frequency, CameraShakeSource::TVehicleLink vehicle);

  /// adds source based on given params
  void AddSource(const CameraShakeParams& params);

  /// resets state, deletes all sources
  void Clear();

  /// returns if shake is enabled
  static bool IsEnabled();

  /// testing for debug purposes
  void DebugTestVehicles();

  /// reads params from cfg
  static void ReadParams(ParamEntryPar cfg);

private:
  // if params were already read from cfg
  static bool ParamsRead;

  /// computes current mod matrix
  void ComputeModMatrix(float deltaT);
  /// resets mod matrix and all needed values for its computation
  void ResetMatrix();

  typedef AutoArray<CameraShakeSource> TSourceList;
  /// list of shake sources
  TSourceList _sources;
  /// index of best current source (-1 = no best source)
  int _bestSourceIdx;
  /// current mod matrix
  Matrix4 _modMatrix;

  /// start position transformation (used for lerp)
  Vector3 _startPos;
  /// start rotation (used for slerp)
  Quaternion<float> _startRot;
  /// end position transformation (used for lerp)
  Vector3 _endPos;
  /// end rotation (used for slerp)
  Quaternion<float> _endRot;


  /// remainng time to compute next mod matrix
  float _nextCompMatrixTime;

  static const int InvalidSourceIdx = -1;

};

#endif //_CAM_SHAKE_HPP
