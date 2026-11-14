/*!
\file
Mission editor cursor
*/

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MISSION_EDITOR_CURSOR_HPP
#define _MISSION_EDITOR_CURSOR_HPP

#if _ENABLE_EDITOR2 && !_VBS2

#include "../vehicle.hpp"

struct EditorProxy
{
  LLink<EditorObject> edObj;
  Ref<Object> object;
  RString id;
};
TypeIsMovableZeroed(EditorProxy)

class MissionEditCursorContainer
{
protected:
  AutoArray<EditorProxy> _proxies;

public:
  const EditorProxy *FindProxy(Vector3Par pos, GameState *gstate) const;
  const EditorProxy *FindProxy(Object *obj) const;
  void AddProxy(Object *object,EditorObject *obj);
  void UpdateProxy(Object *object,EditorObject *edObj);
  void DeleteProxy(EditorObject *edObj);

  // events
  virtual void OnObjectMoved(EditorObject *obj, Vector3Par offset) = NULL;
  virtual void RotateObject(EditorObject *obj, Vector3Par center, float angle) = NULL;
  virtual bool IsDragging() = NULL;
};

class MissionEditorCursor: public Entity
{
  typedef Entity base;

protected:
  // describe orientation (Euler parameters)
  float _heading, _dive;

  float _camDistance;
  float _camHeight;

  // contained by _parent, simple pointer is OK
  MissionEditCursorContainer *_parent;

public:
  MissionEditorCursor(MissionEditCursorContainer *parent);
  ~MissionEditorCursor();

  // Entity implementation
  void Simulate(float deltaT, SimulationImportance prec);

  Matrix4 InsideCamera(CameraType camType) const {return Matrix4(MTranslation, Vector3(0, 0, -_camDistance));}
  float OutsideCameraDistance(CameraType camType) const {return _camDistance;}

  int InsideLOD(CameraType camType) const {return 0;}
  SimulationImportance WorstImportance() const {return SimulateVisibleNear;}

  void LimitVirtual(CameraType camType, float &heading, float &dive, float &fov) const;
  Vector3 ExternalCameraPosition( CameraType camType ) const;
};

// mission editor camera
#include "../cameraHold.hpp"

class MissionEditorCamera : public CameraVehicle
{
  typedef CameraVehicle base;

protected:
  // currently selected proxy object
  //const EditorProxy *_selected;

  // records the last mouse position when a proxy object is selected by left-click
    // used to smooth out dragging so the selected proxy object doesn't jump to the mouse position
  Vector3 _lastPos;

  // this is the marker used to highlight proxy objects when the mouse is over them
  Ref<Texture> _selectedCursor;
  float _selectedCursorWidth, _selectedCursorHeight;

  // set to true by camera movement code, allows camera to freelook again
  bool _resetTargets;

  // used to prevent mouse triggering on-edge camera movement if a unit was recently selected
  bool _dragging;

  // lock onto target?
  bool _lockToTarget;

  // allow move on edge
  bool _allowMoveOnEdge;

  // moving camera quickly
  bool _isMoving;

  // camera should rotate?
  bool _isRotating;

  // contained by _parent, simple pointer is OK
  MissionEditCursorContainer *_parent;

  PackedColor _color;

  // settings from config
  float _speedFwdBack;
  float _speedLeftRight;
  float _speedUpDown;
  float _speedRotate;
  float _speedElevation;
  float _speedTurboMultiplier;

public:
  MissionEditorCamera(MissionEditCursorContainer *parent);
  ~MissionEditorCamera();

  bool IsDragging() {return _dragging;}
  bool IsRotating() {return _isRotating;}
  void SetRotating(bool rotating) {_isRotating = rotating;}

  void Simulate(float deltaT, SimulationImportance prec, GameState *gstate, bool cursorOverCamera);
  void ResetTargets();

  void LookAt(Object *target, bool lockCamera = false);
  void LookAt(Vector3Par target);

  bool IsLocked() {return _lockToTarget;}
  void UnLock();
  void LockTo(Object *lockTo);

  //const EditorProxy *SelectedProxy() const {return _selected;}
  //void ClearSelected() {_selected = NULL;}

  bool IsMoving() {return _isMoving;}

  PackedColor SelectedColor() const {return _color;}
  void DrawSelectedIcon(Vector3Val pos, PackedColor color);

  // get ground intercept of current mouse x,y
  Vector3 GetGroundIntercept();

  // get ground intercept of specified x,y
  Vector3 GetGroundIntercept(float x, float y);

  // return direction of cursor
  Vector3 GetCursorDir();

  bool DrawIcon
  (
    Vector3Val dir, Texture *texture, float width, float height, PackedColor color, bool shadow
  );

  void DrawLine
  (
    Vector3Val from, Vector3Val to, PackedColor color, bool shadow
  );
};

#endif // _ENABLE_EDITOR2

#endif
