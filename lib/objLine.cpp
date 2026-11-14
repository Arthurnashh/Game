#include "wpch.hpp"

#include <Es/ReportStack/reportStack.hpp>
#include "objLine.hpp"
#include "txtPreload.hpp"
#include "scene.hpp"

#if _VBS3
  #include "Shape\speclods.hpp"
#endif
LODShape *ObjectLine::CreateShape()
{
  REPORTSTACKITEM(RString("ObjectLine::CreateShape"));

	LODShape *lShape=new LODShape;

	lShape->SetAutoCenter(false);
	// create all required LOD levels
	Shape *shape=new Shape;
	lShape->AddShape(shape,0);
	// set flags
	const int special=IsAlpha|NoShadow|IsAlphaFog|IsColored;
	lShape->SetSpecial(special);

	// this is the first face
	// initialize lod level
	shape->ReallocTable(2);

#if _VBS3 //make sure lines are visible at night time as well

  const int clip=ClipAll | (MSShiningAdjustable * ClipUserStep);
#else
	const int clip=ClipAll|ClipLightLine;
#endif
	shape->SetPos(0) = VZero;
	shape->SetPos(1) = VForward;
	shape->SetClipAll(clip);
	shape->SetNorm(0)=Vector3Compressed::Up;
	shape->SetNorm(1)=Vector3Compressed::Up;

  UVPair invUV;
  shape->InitUV(invUV, UVPair(0, 0), UVPair(1, 1));
	shape->SetUV(0, invUV, 0, 0);
	shape->SetUV(1, invUV, 1, 1);

	// precalculate hints for possible optimizations
	// change face parameters
	Poly face;
	face.Init();
	face.SetN(4); // set degenerate square
	face.Set(0,1);
	face.Set(1,0);
	face.Set(2,0);
	face.Set(3,1);
	ShapeSectionInfo prop;
	prop.Init();
	prop.SetTexture(GScene->Preloaded(TextureWhite));
	prop.SetSpecial(special|lShape->Special());
	shape->ReserveFaces(1);
	shape->AddFace(face);
	shape->SetSpecial(special);
	shape->SetAsOneSection(prop);
	shape->RecalculateAreas();
	//shape->FindSections();
	shape->CalculateHints();
	lShape->CalculateMinMax(true);
	return lShape;
}

void ObjectLine::SetPos( LODShape *lShape, Vector3Par beg, Vector3Par end )
{
  // TODO: check if modification of shape is legal
  ShapeUsed lock = lShape->Level(0);
  Assert(lock.NotNull());
  Shape *shape = lock.InitShape();

	Assert( shape->NPos()==2 );
	Assert( shape->NFaces()==1 );
	shape->SetPos(0)=beg;
	shape->SetPos(1)=end;
	lShape->CalculateMinMax(true);
}

DEFINE_FAST_ALLOCATOR(ObjectLineDiag)

ObjectLineDiag::ObjectLineDiag( LODShape *shape )
:base(shape,VISITOR_NO_ID)
{
  _lineWidth = 3.0f;
}
void ObjectLineDiag::Draw(
  int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
  const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
)
{
	DrawLines(cb,level,clipFlags,pos);
}


