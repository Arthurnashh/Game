#include "wpch.hpp"
#include "randomShape.hpp"
#include "vehicle.hpp"
#include <El/ParamFile/paramFile.hpp>
#include <El/Common/randomGen.hpp>

#if SUPPORT_RANDOM_SHAPES

RandomShapeType::RandomShapeType(ParamEntryPar param)
:base(param)
{
}

RandomShapeType::~RandomShapeType()
{
}

void RandomShapeType::Load(ParamEntryPar cfg)
{
	base::Load(cfg);
}

void RandomShapeType::InitShape()
{
  base::InitShape();
	// load shapes
	ParamEntryVal shapes = (*_par)>>"models";
	for (int i=0; i<shapes.GetSize()-1; i+=2)
	{
		RString modelName = ::GetShapeName(shapes[i]);
		float modelProbab = shapes[i+1];

		RandomShapeInfo &info = _shapes.Append();
		info._shape = Shapes.New(modelName,false,true);
		info._probab = modelProbab;
	}
}

void RandomShapeType::DeinitShape()
{
	// release shapes
	_shapes.Clear();
}

LODShape *RandomShapeType::SelectShape(float x) const
{
	for (int i=0; i<_shapes.Size(); i++)
	{
		const RandomShapeInfo &info = _shapes[i];
		x -= info._probab;
		if (x>0) continue;
		return info._shape;
	}
	if (_shapes.Size()<=0) return 0;
	return _shapes[_shapes.Size()-1]._shape;
}


RandomShape::RandomShape(const RandomShapeType *type, int id)
:base(NULL,type,id) // select any shape
{
}

LODShape *RandomShape::SelectShape(Vector3Val pos) const
{
	// select random shape (based on position)
	float random = GRandGen.RandomValue
	(
		toIntFloor(pos.X()),
		toIntFloor(pos.Z()),
		toIntFloor(pos.Y())
	);
	return Type()->SelectShape(random);

}

LODShape *RandomShape::GetShapeOnPos(Vector3Val pos) const
{
	// virtual function of Object
	return SelectShape(pos);
}

void RandomShape::Draw(
  int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
  const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
)
{
	LODShape *shape = SelectShape(pos.Position());
	if (!shape) return;

	// temporarily override shape
	_shape = shape;
	base::Draw(cb,forceLOD, clipFlags, pos, parentObject);
	// reset shape to NULL
	_shape = NULL;
}

#endif

