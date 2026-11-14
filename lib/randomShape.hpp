#ifdef _MSC_VER
#pragma once
#endif

#ifndef _RANDOM_SHAPE_HPP
#define _RANDOM_SHAPE_HPP

#include "vehicle.hpp"

struct RandomShapeInfo
{
	Ref<LODShape> _shape;
	float _probab;
};

#if 0//SUPPORT_RANDOM_SHAPES

TypeIsMovableZeroed(RandomShapeInfo)

class RandomShapeType: public EntityType
{
	typedef EntityType base;

	friend class RandomShape;

	AutoArray<RandomShapeInfo> _shapes;

	public:
	RandomShapeType(ParamEntryPar param);
	~RandomShapeType();

	void Load(ParamEntryPar cfg);

	void InitShape();
	void DeinitShape();

	LODShape *SelectShape(float x) const;
};

class RandomShape: public ObjectTyped
{
	typedef ObjectTyped base;

	public:
	RandomShape(const RandomShapeType *type, int id);

	const RandomShapeType *Type() const
	{
		return static_cast<const RandomShapeType *>(GetEntityType());
	}
  void Draw(
    int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
    const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
  );

	LODShape *SelectShape(Vector3Val pos) const;
	LODShape *GetShapeOnPos(Vector3Val pos) const;
};

#endif

#endif
