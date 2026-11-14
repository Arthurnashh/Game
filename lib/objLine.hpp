#ifdef _MSC_VER
#pragma once
#endif

#ifndef _OBJ_LINE_HPP
#define _OBJ_LINE_HPP

#include "object.hpp"

class ObjectLine
{
	public:
	static LODShape *CreateShape();
	static void SetPos( LODShape *lShape, Vector3Par beg, Vector3Par end );
};

#include <Es/Memory/normalNew.hpp>

// TODO: move ObjectLineDiag from vehicleAI.cpp
class ObjectLineDiag: public ObjectColored
{
	typedef ObjectColored base;
  //! Width of the line
  float _lineWidth;
public:
  //! Constructor
	ObjectLineDiag( LODShape *shape );
  //! Virtual method
	virtual void Draw(
	  int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
    const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
	) override; // virtual

  virtual bool CanObjDrawAsTask(int level) const {return false;}
  //! Virtual method
  virtual void SetConstantWidth(float width) {_lineWidth = width;}
  //! Virtual method
  virtual float GetConstantWidth() const {return _lineWidth;}

	USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

#endif
