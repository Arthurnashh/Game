// Poseidon - shape management
// (C) 1998, SUMA

#include "El/elementpch.hpp"
#include "cutShape.hpp"
//#include "engine.hpp"
//#include "global.hpp"
//#include "object.hpp"
//#include "scene.hpp"
#include <El/QStream/QBStream.hpp>
#include "cutParamFileExt.hpp"
#include "textbank.hpp"
#include "keyInput.hpp"
#include "dikCodes.h"
#include "animation.hpp"
#include "tlVertex.hpp"
#include "fileServer.hpp"
#include "strFormat.hpp"

#include <El/Common/perfLog.hpp>
#include <El/QStream/serializeBin.hpp>

#include "edges.hpp"
#include "SpecLods.hpp"

#include <Es/Algorithms/qsort.hpp>
#include <Es/Common/filenames.hpp>

//#include <malloc.h>

#include "data3d.h"

#include "mapTypes.hpp"

#ifdef _MSC_VER
	#pragma warning( disable: 4355 )
#endif

inline bool IsSpec( float resolution, float spec )
{
	if( fabs(resolution-spec)<spec*1e-3 ) return true;
	return false;
}

extern bool EnableHWTL;
extern bool EnablePIII;


DEFINE_FAST_ALLOCATOR(Shape)

Shape::Shape()
:_special(0),_faceNormalsValid(false)
{
	_color=_colorTop=PackedBlack;
	_loadWarning = false;
	_level = -1;

}

// ShapeSection is a group of faces that can be drawn with one call
// this means they share at least texture and flags
// in case of T&L engine they also need to share animation properties



Shape::~Shape(){Clear();}

Shape::Shape( const Shape &src, bool copyAnimations )
:VertexTable(src),
_face(src._face),
_plane(src._plane),
_color(src._color),
_special(src._special),
_prop(src._prop), // copy named properties
_sel(src._sel), // copy named selections
_level(src._level),
_textures(src._textures), // copy texture list
_pointToVertex(src._pointToVertex),
_vertexToPoint(src._vertexToPoint),
//_sections(src._sections),
_faceNormalsValid(src._faceNormalsValid)
{
	if( copyAnimations ) _phase=src._phase;
	_minMax[0]=src._minMax[0];
	_minMax[1]=src._minMax[1];
	//_boundingSphere=src._boundingSphere;
	for (int i=0; i<_proxy.Size(); i++)
	{
		_proxy[i] = new ProxyObject(*src._proxy[i]);
	}
}
#ifdef _MSC_VER
	#pragma warning( default: 4355 )
#endif

struct MTextureItem
{
	RStringB name; // path name (data\sample.pac)
	int x,y; // position
	int w,h; // orig. dimensions
	int scale; // scale (can be negative - zoom out)
	int angle;

	void Load( const ParamEntry &entry );
};

TypeIsMovableZeroed(MTextureItem)

struct MTextureSet: public AutoArray<MTextureItem>
{
	RString filename; // set texture name
	mutable LLink<Texture> tex;
	int w,h;

	void Load( const ParamEntry &entry );
	int Contains( RStringB name ) const;
	Ref<Texture> GetTex() const;
};

TypeIsMovable(MTextureSet)

class MTextureBank: public AutoArray<MTextureSet>
{
	bool _loaded;

	public:
	MTextureBank();
	void Load( QIStream &f );
	int FindBest( const RefArray<Texture> &find ) const;
};

struct MTextureMap
{
	Ref<Texture> texture; // texture id
	int set,item; // coordinades in merged bank
};

TypeIsMovableZeroed(MTextureMap)

const static RStringB DataFolder="data\\";

void MTextureItem::Load( const ParamEntry &entry )
{
	RString lowName = DataFolder+(entry>>"name").GetValue();
	lowName.Lower();
	name = lowName;
	x=entry>>"x";
	y=entry>>"y";
	angle=entry>>"angle";
	scale=entry>>"scale";
	w=entry>>"w";
	h=entry>>"h";
}

void MTextureSet::Load( const ParamEntry &entry )
{
	filename=entry>>"file";
	filename.Lower();
	w=entry>>"w";
	h=entry>>"h";
	const ParamEntry &a=entry>>"Items";
	for( int i=0; i<a.GetEntryCount(); i++ )
	{
		const ParamEntry &e=a.GetEntry(i);
		MTextureItem &item=Set(Add());
		item.Load(e);
	}
}

int MTextureSet::Contains( RStringB name ) const
{
	for( int i=0; i<Size(); i++ )
	{
		const MTextureItem &item=Get(i);
		if( item.name==name ) return i;
	}
	return -1;
}

Ref<Texture> MTextureSet::GetTex() const
{
	if( tex ) return tex.GetLink();
	// compose name from partial name
	Ref<Texture> texture=GlobLoadTexture(filename);
	Texture *castTemp=texture;
	tex=castTemp;
	return texture;
}


MTextureBank::MTextureBank()
{
	_loaded=false;
}

void MTextureBank::Load( QIStream &f )
{
	if( _loaded ) return;
	_loaded=true;

	ParamFile file;
	file.Parse(f);
	if( file.GetEntryCount()<=0 ) return;
	const ParamEntry &a=file>>"InfTexMerge";
	for( int i=0; i<a.GetEntryCount(); i++ )
	{
		const ParamEntry &e=a.GetEntry(i);
		if (!e.IsClass()) continue;
		MTextureSet &set=Set(Add());
		set.Load(e);
	}
}

int MTextureBank::FindBest( const RefArray<Texture> &find ) const
{
	// check all textures
	// consider only sets that contain at least two textures from texture array
	// prefer merged - may be shared between lods
	int maxCount=0;
	int bestI=-1;
	for( int i=0; i<Size(); i++ )
	{
		const MTextureSet &set=Get(i);
		int count=0;
		// check by name
		for( int t=0; t<find.Size(); t++ )
		{
			if (!find[t]) continue; // TODO: do not include NULL texture in the list
			if( set.Contains(find[t]->GetName())>=0 ) count++;
		}
		if( count>maxCount )
		{
			maxCount=count;
			bestI=i;
		}
	}
	return bestI;
}

static MTextureBank MergedTextures;

#if _DEBUG
#define VERBOSE 1
#else
#define VERBOSE 0
#endif

struct SortVertex
{
	int vertex,point,prior;
};
TypeIsSimple(SortVertex)

static int CmpSortVertex( const SortVertex *v0, const SortVertex *v1 )
{
	int d;
	d= v0->prior-v1->prior;
	if (d) return d;
	d = v0->point-v1->point;
	return d;
}

static bool LoadTag( QIStream &f, char *name, int &size )
{
	f.read(name,64);
	f.read((char *)&size,sizeof(size));
	if( f.fail() || f.eof() ) return false;
	return true;
}

bool GMergeTextures = false;
// convert old point attributes
// to new material indices (introduced on 12 Feb 2001)

static bool ResolGeometryOnly(float resolution)
{
	return
	(
		IsSpec(resolution,GEOMETRY_SPEC) ||
		IsSpec(resolution,VIEW_GEOM_SPEC) ||
		IsSpec(resolution,FIRE_GEOM_SPEC) ||
		IsSpec(resolution,VIEW_PILOT_GEOM_SPEC) ||
		IsSpec(resolution,VIEW_CARGO_GEOM_SPEC) ||
		IsSpec(resolution,VIEW_COMMANDER_GEOM_SPEC) ||
		IsSpec(resolution,VIEW_GUNNER_GEOM_SPEC) ||
		IsSpec(resolution,MEMORY_SPEC) ||
		IsSpec(resolution,LANDCONTACT_SPEC) ||
		IsSpec(resolution,ROADWAY_SPEC) ||
		IsSpec(resolution,PATHS_SPEC) ||
		IsSpec(resolution,HITPOINTS_SPEC)
	);
}

const int fullFlags = (MSFullLighted*ClipUserStep);
const int halfFlags = (MSHalfLighted*ClipUserStep);
const int ambFlags = (MSInShadow*ClipUserStep);
const int shineFlags = (MSShining*ClipUserStep);

struct OxygenLEOriginTag
{
	int prot_componumber;
	int prot_magicnumber;
	int prot_unlockcode;
	int prot_requestcode;
};



float Shape::LoadTagged
(
	QIStream &f, bool reversed, int ver, bool geometryOnly,
	AutoArray<float> &massArray, bool tagged
)
{

	_loadWarning = false;
	_face.Clear();

	bool extended=false;
	DataHeaderEx head;
	f.read((char *)&head,sizeof(head));
	if (f.fail() || strncmp(head.magic,"SP3X",4))
	{
		// is not extended format, try old
		f.seekg(-(int)sizeof(head),QIOS::cur);
		DataHeader oHead;
		f.read((char *)&oHead,sizeof(oHead));
		if( f.fail() || strncmp(oHead.magic,"SP3D",4) )
		{
			char magicName[5];
			strncpy(magicName,head.magic,4);
			magicName[4]=0;
			WarningMessage("Bad file format (%s).",magicName);
			return 0; // file input error
		}
		head.version=0;
		head.nPos=oHead.nPos;
		head.nNorm=oHead.nNorm;
		head.nFace=oHead.nFace;
		head.flags=0;
		head.headSize=sizeof(oHead);
	}
	else
	{
		extended=true;
		// skip rest of header
		f.seekg(head.headSize-sizeof(head),QIOS::cur);
	}
	
	_face.Realloc(head.nFace);

	//bool specialLod=false;
		
	_pos.Realloc(head.nPos);
	_clip.Realloc(head.nPos);
	_norm.Realloc(head.nNorm);

	#define maxPoints 2048
	AUTO_STATIC_ARRAY(Vector3,pos,maxPoints);
	AUTO_STATIC_ARRAY(Vector3,norm,maxPoints);
	AUTO_STATIC_ARRAY(ClipFlags,clip,maxPoints);
	AUTO_STATIC_ARRAY(bool,hidden,maxPoints);
	pos.Resize(head.nPos);
	clip.Resize(head.nPos);
	hidden.Resize(head.nPos);
	norm.Resize(head.nNorm);
	_pointToVertex.Resize(head.nPos);
	_vertexToPoint.Realloc(head.nPos);
	for( int i=0; i<_pointToVertex.Size(); i++ )
	{
		_pointToVertex[i]=-1;
	}

	// load points, normals and mapping pairs
	//bool hiddenFace=false;
	bool reportInvalid=true;
	for( int i=0; i<head.nPos; i++ )
	{
		if( !extended )
		{
			DataPoint src;
			f.read((char *)&src,sizeof(src));
			if( f.fail() ) break;
			//SetPos(i)=Vector3(src.X(),src.Y(),src.Z());
			//SetClip(i,ClipAll);
			pos[i]=Vector3(src.X(),src.Y(),src.Z());
			clip[i]=ClipAll;
			hidden[i]=false;
		}
		else
		{
			DataPointEx src;
			f.read((char *)&src,sizeof(src));
			if( f.fail() ) break;
			
			ClipFlags hints=ClipAll;
			const int allFlags=
			(
				POINT_ONLAND|POINT_UNDERLAND|POINT_ABOVELAND|POINT_KEEPLAND
				|POINT_DECAL|POINT_VDECAL
				|POINT_NOLIGHT|POINT_FULLLIGHT|POINT_HALFLIGHT|POINT_AMBIENT
				|POINT_NOFOG|POINT_SKYFOG|POINT_USER_MASK
				|POINT_SPECIAL_MASK
			);
			hidden[i]=false;
			// check for valid flags - detect objektiv bug
			if( src.flags&~allFlags )
			{
				src.flags=0;
				if( reportInvalid )
				{
					reportInvalid=false;
					LogF("Invalid point flags.");
					_loadWarning = true;
				}
			}
			if( src.flags&allFlags )
			{
				if( src.flags&POINT_ONLAND ) hints|=ClipLandOn;
				//if( src.flags&POINT_ONLAND ) hints|=ClipLandKeep;
				else if( src.flags&POINT_UNDERLAND ) hints|=ClipLandUnder;
				else if( src.flags&POINT_ABOVELAND ) hints|=ClipLandAbove;
				else if( src.flags&POINT_KEEPLAND ) hints|=ClipLandKeep;
				
				if( src.flags&POINT_DECAL ) hints|=ClipDecalNormal;
				else if( src.flags&POINT_VDECAL ) hints|=ClipDecalVertical;

				// convert lighting flags to materials
				if( src.flags&POINT_NOLIGHT ) hints|=shineFlags;
				else if( src.flags&POINT_FULLLIGHT ) hints|=fullFlags;
				else if( src.flags&POINT_HALFLIGHT ) hints|=halfFlags;
				else if( src.flags&POINT_AMBIENT ) hints|=ambFlags;


				if( src.flags&POINT_NOFOG ) hints|=ClipFogDisable;
				else if( src.flags&POINT_SKYFOG ) hints|=ClipFogSky;

				if( src.flags&POINT_USER_MASK )
				{
					int user=(src.flags&POINT_USER_MASK)/POINT_USER_STEP;
					hints|=user*ClipUserStep;
				}
				if( src.flags&POINT_SPECIAL_HIDDEN )
				{
					hidden[i]=true;
				}

				/*
				if (hints&ClipUserMask)
				{
					LogF("point %d",(hints&ClipUserMask)/ClipUserStep);
				}
				*/
			}
			if( reversed ) src[0]=-src[0],src[2]=-src[2];	
			pos[i]=Vector3(src.X(),src.Y(),src.Z());
			clip[i]=hints;
		}
	}
	
	for( int i=0; i<head.nNorm; i++ )
	{
		DataNormal src;
		f.read((char *)&src,sizeof(src));
		if( f.fail() ) break;
		if( reversed ) src[0]=-src[0],src[2]=-src[2];
		if
		(
			!_finite(src[0]) || _isnan(src[0]) ||
			!_finite(src[1]) || _isnan(src[1]) ||
			!_finite(src[2]) || _isnan(src[2])
		)
		{
			if (reportInvalid)
			{
				LogF("Invalid normal");
				reportInvalid = false;
			}
			src[0]=0, src[1]=1, src[2]=0;
			_loadWarning = true;
		}
		// normals in geometry ignored to avoid vertex multiplication
		if (geometryOnly) src[0]=0, src[1]=1, src[2]=0;
		//SetNorm(i)=Vector3(src.X(),src.Y(),src.Z());
		norm[i]=Vector3(src.X(),src.Y(),src.Z());
	}
	
	int andSpecial=~0;
	// load faces
	for( int i=0; i<head.nFace; i++ )
	{
		DataFaceEx srcFace;
		if( !extended )
		{
			DataFace oFace;
			f.read((char *)&oFace,sizeof(oFace));
			if( f.fail() ) break;
			srcFace.DataFace::operator = (oFace);
			srcFace.flags=0;
		}
		else
		{
			f.read((char *)&srcFace,sizeof(srcFace));
			if( f.fail() ) break;
		}
		
		int spec=0;
		Poly poly;
		Ref<Texture> texture;
		if( !*srcFace.texture )
		{
			texture=DefaultTexture;
			_textures.AddUnique(texture);
		}
		else
		{
			strlwr(srcFace.texture);
			// search object textures first
			for( int t=0; t<_textures.Size(); t++ )
			{
				Texture *txt=_textures[t];
				if (!txt) continue;
				if( !strcmp(txt->Name(),srcFace.texture) ) {texture=txt;break;}
			}
			if( !texture )
			{
				// texture not present in this object - load new
				// detect animated texture
				//AbstractTextBank *bank=GEngine->TextBank();
				texture=GlobLoadTexture(srcFace.texture);
				if (texture)
				{
					_textures.Add(texture);
				}
			}
		}
		Assert( !texture || _textures.Find(texture)>=0 );
		poly.SetTexture(texture);
		poly.SetN(srcFace.n);
		bool skipPoly=true;

		const int allFlags=
		(
			FACE_NOLIGHT|FACE_AMBIENT|FACE_FULLLIGHT|FACE_BOTHSIDESLIGHT|FACE_SKYLIGHT
			|FACE_REVERSELIGHT|FACE_FLATLIGHT
			|FACE_ISSHADOW|FACE_NOSHADOW
			|FACE_DISABLE_TEXMERGE|FACE_USER_MASK|FACE_Z_BIAS_MASK
		);
		// check for valid flags - detect objektiv bug
		if( srcFace.flags&~allFlags )
		{
			// some invalid flags
			if (reportInvalid)
			{
				LogF("Invalid face flags");
				reportInvalid = false;
				_loadWarning = true;
			}

			srcFace.flags=0; // reset - assume it is result of Objektiv bug
		}
		ClipFlags clipLightAttr=0;
		if( srcFace.flags&allFlags )
		{
			if( srcFace.flags&FACE_NOLIGHT ) clipLightAttr|=shineFlags;
			else if( srcFace.flags&FACE_AMBIENT ) clipLightAttr|=ambFlags;
			else if( srcFace.flags&FACE_FULLLIGHT ) clipLightAttr|=fullFlags;
			if( srcFace.flags&FACE_ISSHADOW ) spec|=IsShadow;
			if( srcFace.flags&FACE_NOSHADOW ) spec|=NoShadow;
			if( srcFace.flags&FACE_DISABLE_TEXMERGE ) spec|=NoTexMerger;
			if (srcFace.flags&FACE_USER_MASK)
			{
				int material = ((srcFace.flags&FACE_USER_MASK)>>FACE_USER_SHIFT)&0xff;
				clipLightAttr = material*ClipUserStep;
			}
			if (srcFace.flags&FACE_Z_BIAS_MASK)
			{
				int bias = (srcFace.flags&FACE_Z_BIAS_MASK)/FACE_Z_BIAS_STEP;
				spec |= ZBiasStep*bias;
			}
		}

		for( int j=0; j<srcFace.n; j++ )
		{
			int srcPoint=srcFace.vs[j].point;
			int srcNormal=srcFace.vs[j].normal;
			float srcU=srcFace.vs[j].mapU;
			float srcV=srcFace.vs[j].mapV;
			#if 1 //!_RELEASE
				// TODO: move this to Objektiv structure check
				if (fabs(srcU)>1e5 || fabs(srcV)>1e5)
				{
					srcU=srcV=0;
					if (reportInvalid)
					{
						LogF("Invalid uv coordinates");
						reportInvalid = false;
						_loadWarning = true;
					}
				}

			#endif
			// uv in geometry ignored to avoid vertex multiplication
			if (geometryOnly) srcU = srcV = 0;

			if (texture)
			{
				srcU=texture->UToPhysical(srcU);
				srcV=texture->VToPhysical(srcV);
			}

			// TODO: remove hidden points?
			if( !hidden[srcPoint] )
			{
				skipPoly=false;
			}


			ClipFlags c = clip[srcPoint];
			if (clipLightAttr)
			{
				// combine material with old flags
				// shining is offset +20
				// inShadow is offset +40
				ClipFlags user = c&ClipUserMask;
				if (user)
				{
					// note: offsetting is quite a hack
					// we should rather reserve some bits for special lighting
					// and some leave to user material definition
					int offset = 0;
					if (user==shineFlags) offset = 20;
					else if (user==ambFlags) offset = 40;
					c &= ~ClipUserMask;
					c |= clipLightAttr + offset * ClipUserStep;
				}
				else
				{
					c |= clipLightAttr;
				}
			}
			ClipFlags fog=c&ClipFogMask;
			if( fog==ClipFogSky || fog==ClipFogDisable ) c&=~ClipBack;

			//
			int vertex=AddVertex
			(
				pos[srcPoint],norm[srcNormal],c,
				srcU,srcV,
				&_vertexToPoint,srcPoint
			);

			#if VERBOSE>2
				LogF
				(
					"    vertex %d from %d,%d, clip %x",
					vertex,srcPoint,srcNormal,clip[srcPoint]
				);
			#endif

			poly.Set(j,vertex);

			_vertexToPoint.Access(vertex);
			_vertexToPoint[vertex]=srcPoint;
			_pointToVertex[srcPoint]=vertex; // objektiv point index to vertex index conversion
		}
		poly.Reverse(); // models have reversed geometry

		// lighting attributes must be applied to all vertices as well
		for( int j=0; j<srcFace.n; j++ )
		{
			int srcPoint=srcFace.vs[j].point;
			ClipFlags c=clip[srcPoint];
			//ClipFlags c=Clip(srcPoint);
			ClipFlags land=c&ClipLandMask;
			if( land==ClipLandOn ) spec|=OnSurface;
		}

		if (texture)
		{
			texture->LoadHeaders();
			if( texture->IsAlpha() ) spec|=IsAlpha|IsAlphaOrdered;
			if( texture->IsTransparent() ) spec|=IsTransparent;
			if (texture->IsAnimated()) spec|=::IsAnimated;
			// TODO: replace ".paa" check with alpha order Objektive flags
			// trick - disable alpha ordering by extension
			if (!strcmpi(GetFileExt(texture->Name()),".paa"))
			{
				// another trick - merged textures should not be alpha ordered
				if (!strstr(texture->Name(),"\\000"))
				{
					spec |= IsAlphaOrdered;
				}
			}
			// check special case ".paa"
		}
		if( skipPoly )
		{
			// this is currently used only for proxy objects
			// it is safe to disable texture merging on them
			spec|=IsHidden|NoTexMerger;
		}

		poly.SetSpecial(spec);
		_special|=spec;
		andSpecial&=spec;
		_face.Add(poly);
	}

	// make sure all points are represented with vertices

	for( int i=0; i<_pointToVertex.Size(); i++ )
	{
		//if( _pointToVertex[i]<0 && !hidden[i] )
		if (_pointToVertex[i]<0)
		{
			int vertex=AddVertex
			(
				pos[i],VZero,clip[i], // normal set to zero - should not be used
				0,0, // no u-v mapping
				&_vertexToPoint,i
			);
			_pointToVertex[i]=vertex;
			_vertexToPoint.Access(vertex);
			_vertexToPoint[vertex]=i;
		}
	}

	#if 1 // enable/disable material optimization
	
	SortVertices();

	#endif
	
	_special&=IsAlpha|IsTransparent|::IsAnimated|OnSurface;
	_special|=andSpecial&(NoShadow|ZBiasMask);
	CalculateHints();	// VertexTable optimization
	//if( GetOrHints()&ClipLandOn ) _special|=OnSurface;
	#if !_RELEASE
		if( GetOrHints()&ClipLandOn )
		{
			Assert( _special&OnSurface );
		}
	#endif
	if( f.fail() )
	{
		WarningMessage("Bad object.");
		Clear();
		return 0; // file input error
	}
	_pos.Compact();
	_norm.Compact();
	_clip.Compact();
	_vertexToPoint.Compact();
	_pointToVertex.Compact();
	AutoClamp();
	RecalculateAreas();
	RecalculateNormals(true);
	CalculateMinMax();
	StoreOriginalMinMax();
	CalculateColor();

	if (!geometryOnly && GMergeTextures)
	{
		MergeTextures(false);
	}

	// skip all tags (if any), terminate at #EndOfFile#
	// check for TAGG magic
	char magic[4];
	f.read(magic,sizeof(magic));
	if( f.fail() || f.eof() )
	{
		WarningMessage("Error loading tag");
		_loadWarning = true;
		return 0;
	}
	if( strncmp("TAGG",magic,sizeof(magic)) )
	{
		WarningMessage("Bad format");
		_loadWarning = true;
		return 0;
	}

	_sel.Clear(); // clear selections
	for(;;)
	{
		int tagSize;
		char tagName[64];
		if( !LoadTag(f,tagName,tagSize) ) break;
		else if( !strcmpi(tagName,"#EndOfFile#") ) break;
		else if( !strcmpi(tagName,"#Mass#") )
		{
			if( ver==0 )
			{
				LogF("%s: Old mass no longer supported");
				f.seekg(tagSize,QIOS::cur); // skip tag
			}
			else
			{
				// only geometry level stores mass information in 1.xx
				int nMass=_pointToVertex.Size();
				Assert( tagSize==(int)sizeof(float)*nMass);
				Assert( massArray.Size()==0 );
				if( nMass>0 )
				{
					massArray.Realloc(nMass);
					massArray.Resize(nMass);
					f.read(massArray.Data(),sizeof(float)*nMass);
				}
			}
		}
		else if( !strcmpi(tagName,"#Animation#") )
		{
			int pSize=_pointToVertex.Size();
			Temp<DataVec> animTemp(pSize);
			AnimationPhase anim;
			float time=0;
			Assert( (size_t)tagSize==sizeof(animTemp[0])*pSize+sizeof(time) );
			f.read((char *)&time,sizeof(time));
			f.read((char *)&animTemp[0],sizeof(animTemp[0])*pSize);
			anim.Resize(NPos());
			for( int a=0; a<NPos(); a++ )
			{
				int vp = _vertexToPoint[a];
				Assert (vp>=0);
				const DataVec &at=animTemp[vp];
				anim[a]=Vector3(at.X(),at.Y(),at.Z());
				if( reversed ) anim[a][0]=-anim[a][0],anim[a][2]=-anim[a][2];	
			}
			anim.SetTime(time);
			AddPhase(anim);
		}
		else if( !strcmpi(tagName,"#Property#") )
		{
			struct {char name[64],value[64];} prop;
			Assert( (size_t)tagSize==sizeof(prop) );
			f.read((char *)&prop,sizeof(prop));
			strlwr(prop.name);
			strlwr(prop.value);
			_prop.Add(NamedProperty(prop.name,prop.value));
		}
		else if( !strcmpi(tagName,"#MaterialIndex#") )
		{
			OxygenLEOriginTag originTag;
			Assert( (size_t)tagSize==sizeof(originTag) );
			f.read(&originTag,sizeof(originTag));
			// convert to named properties

			_prop.Add(NamedProperty("__ambient",Format("08x",originTag.prot_componumber)));
			_prop.Add(NamedProperty("__diffuse",Format("08x",originTag.prot_magicnumber)));
			_prop.Add(NamedProperty("__specular",Format("08x",originTag.prot_unlockcode)));
			_prop.Add(NamedProperty("__emissive",Format("08x",originTag.prot_requestcode)));
		}
		else if( *tagName!='#' ) // named selection
		{
			int pSize=(int)sizeof(bool)*_pointToVertex.Size();
			int fSize=(int)sizeof(bool)*NFaces();
			if( tagSize!=pSize+fSize )
			{
				_loadWarning = true;
				LogF("Invalid named selection %s",tagName);
				f.seekg(tagSize,QIOS::cur); // skip tag
			}
			else if( *tagName=='-' || *tagName=='.' )
			{
				// ignore selection
				f.seekg(tagSize,QIOS::cur); // skip tag
			}
			else
			{
				Temp<byte> tempPoints(pSize);
				Temp<bool> tempFaces(NFaces());
				f.read(tempPoints.Data(),pSize);
				f.read(tempFaces.Data(),fSize);
				//f.seekg(fSize,QIOS::cur);
				AUTO_STATIC_ARRAY(SelInfo,selPoints,2048);
				AUTO_STATIC_ARRAY(VertexIndex,selFaces,2048);
				int i;
				for( i=0; i<NPos(); i++ )
				{
					int vp = _vertexToPoint[i];
					Assert (vp>=0);
					byte val=tempPoints[vp];
					if( val ) selPoints.Add(SelInfo(i,-val));
				}
				#if _DEBUG
				if (!strcmpi(tagName,"zasleh"))
				{
					__asm nop;
				}
				#endif
				for( i=0; i<NFaces(); i++ ) if( tempFaces[i] ) selFaces.Add(i);
				NamedSelection sel
				(
					tagName,
					selPoints.Data(),selPoints.Size(),
					selFaces.Data(),selFaces.Size()
				);
				//Log("Create intervals in %s:%s",Name(),tagName);
				//sel.CreateIntervals();
				AddNamedSel(sel);
			}
		}
		else f.seekg(tagSize,QIOS::cur); // skip tag
	}

	/*
	if (shape->_sel.Size()<=0)
	{
		// no selections - no point indices may be referred
		shape->_vertexToPoint.Clear();
		shape->_pointToVertex.Clear();
	}
	*/


	Compact();

	float resolution=0;
	f.read((char *)&resolution,sizeof(resolution));
	if( f.fail() || f.eof() ) resolution=0.0f;

	/*
	if (!geometryOnly && GMergeTextures && !ResolGeometryOnly(resolution))
	{
		// untile only very bad LOD levels
		MergeTextures(resolution>10);
	}
	*/

	return resolution;
}

bool LODShape::CheckLegalCreator() const
{
	int componumber = strtoul(PropertyValue("__ambient"),NULL,16);
	int magicnumber = strtoul(PropertyValue("__diffuse"),NULL,16);
	int unlockcode = strtoul(PropertyValue("__specular"),NULL,16);
	int requestcode = strtoul(PropertyValue("__emissive"),NULL,16);

	return
	(
		componumber!=~0 || magicnumber!=~0 || unlockcode!=~0 || requestcode!=~0
	);
}

void FaceArray::SetSections( const ShapeSection *sec, int nSec)
{
	_sections.Realloc(nSec);
	_sections.Resize(nSec);
	for (int i=0; i<nSec; i++)
	{
		_sections[i] = sec[i];
	}
}


void Shape::DefineSections(const ParamEntry &cfg)
{
	const ParamEntry &notes = Pars>>"CfgModels";


	_face._sections.Resize(0);

	// find sections - take care of animated textures

	// scan faces, group them to sections
	// some named selections are marked as section-based
	// no section may split across any such selection

	const ParamEntry *cfgI = &cfg;
	bool split = false;
	for(;;)
	{
		const ParamEntry &entry = (*cfgI)>>"sections";
		// check if sections need to be split
		// mark named selections as section aware
		for (int i=0; i<entry.GetSize(); i++)
		{
			RStringB selName = entry[i];
			int si = FindNamedSel(selName);
			if (si<0) continue;
			NamedSelection &sel = NamedSel(si);
			sel.SetNeedsSections(true);
			split = true;
		}
		const RStringB &inherit = (*cfgI)>>"sectionsInherit";
		if (inherit.GetLength()<=0) break;
		cfgI = &(notes>>inherit);
	}

	Offset beg = BeginFaces();
	Offset end = EndFaces();
	// when detected change, save old section
	ShapeSection sec; // actual section
	bool secOpen = false;
	//LogF("FindSection - split %d",split);

	for (Offset o = beg;o<end; NextFace(o))
	{
		const Poly &face = Face(o);
		if (!secOpen)
		{
			sec.beg = o;
			sec.end = o;
			sec.properties = face; // get properties from the face
			secOpen = true;
		}
		else
		{
			// check if we match section
			if
			(
				sec.properties.GetTexture()==face.GetTexture() &&
				sec.properties.Special()==face.Special()
			)
			{
				//Assert(sec.beg<=sec.end);
				// extend section
				sec.end = o; 
			}
			else
			{
				// we need to close section
				NextFace(sec.end);
				if (split) AddSection(sec);
				else _face._sections.Add(sec);
				// and immediately open another one

				sec.beg = o;
				sec.end = o;
				sec.properties = face; // get properties from the face
			}
		}
	}
	if (secOpen)
	{
		NextFace(sec.end);
		if (split) AddSection(sec);
		else _face._sections.Add(sec);
		secOpen = false;
	}
	_face._sections.Compact();
}

void Shape::UntileTextures(const MTextureMap *mapping, int nMapping)
{
	#if 1
	// prepare textures for merging by untiling them

	FaceArray clipped;

	for( Offset f=_face.Begin(); f<_face.End(); )
	{
		Poly &face=_face[f];
		if (face.Special()&NoTexMerger)
		{
			_face.Next(f);
			continue;
		}

		Texture *texture=face.GetTexture();

		// check it there is some mapping ready
		
		// try to untile texture (if possible)
		float uMin=+1e10,vMin=+1e10;
		float uMax=-1e10,vMax=-1e10;
		for( int ii=0; ii<face.N(); ii++ )
		{
			int vi=face.GetVertex(ii);
			float u=0, v=0;
			if (texture)
			{
				u=texture->UToLogical(U(vi));
				v=texture->VToLogical(V(vi));
			}
			saturateMin(uMin,u);
			saturateMin(vMin,v);
			saturateMax(uMax,u);
			saturateMax(vMax,v);
		}
		float uBase=toIntFloor(uMin)-1;
		float vBase=toIntFloor(vMin)-1;

		// note: smaller epsilon is more accurate, but requires more vertices
		const float eps = texture->IsTransparent() ? 0.10 : 0.35;

		while (uMax>uBase+1) uBase += 1;
		while (uMin<uBase-eps && uMax<uBase+1-eps) uBase -= 1;
		uMax -= uBase, uMin -= uBase;

		while (vMax>vBase+1) vBase += 1;
		while (vMin<vBase-eps && vMax<vBase+1-eps) vBase -= 1;
		vMax -= vBase, vMin -= vBase;

		if (uMin>=-eps && vMin>=-eps && uMax<=1+eps && vMax<=1+eps)
		{
			// not tiled - no processing required
			_face.Next(f);
			continue;
		}

		int mapIndex=-1;
		for( int i=0; i<nMapping; i++ )
		{
			if( mapping[i].texture==texture )
			{
				mapIndex=i;
				break;
			}
		}
		if (mapIndex<0)
		{
			// no mapping - no need to untile
			_face.Next(f);
			continue;
		}

		// we need to perform some splitting on the face

		// texture is tiled
		LogF
		(
			"%s removing tiling (%.3f): u=%.3f..%.3f, v=%.3f..%.3f",
			texture->Name(),eps,uMin+uBase,uMax+uBase,vMin+vBase,vMax+vBase
		);

		const float epsIgnore = 0.05;
		int ubmin = toIntCeil(uMin+uBase+epsIgnore);
		int ubmax = toIntFloor(uMax+uBase-epsIgnore);

		int vbmin = toIntCeil(vMin+vBase+epsIgnore);
		int vbmax = toIntFloor(vMax+vBase-epsIgnore);

		LogF
		(
			"                 clip:  u=%d..%d, v=%d..%d",
			ubmin,ubmax,vbmin,vbmax
		);
		if (ubmax<ubmin && vbmax<vbmin)
		{
			// no clipping boundary present
			Log("Late uv-clipping skip");
			_face.Next(f);
			continue;
		}

		// split by all required u boundaries
		Poly sourceU = face;
		// remove original face from the shape
		_face.Delete(f);
		// note: variable face is now invalid - it is only reference
		// TODO: check which selection belongs this face to
		// include splitted faces in the same selections

		Poly lessU = sourceU, greaterU = sourceU;
		lessU.SetN(0);
		greaterU.SetN(0);
		for (int ub=ubmin; ub<=ubmax; ub++)
		{
			//Log("Clipping u %d",ub);
			// a*u + bv + c <0 means vertex is out
			// we want out (greaterU) to contain ub<u
			// u>ub  <=> u-ub>0 <=> -u + ub <0 => a = -1, b = 0, c = ub
			sourceU.SplitUV(*this,lessU,greaterU,-1,0,ub);
			sourceU = greaterU;
			// lessU needs to be clipped against all v-boundaries
			Poly sourceV = lessU;
			Poly lessV = sourceV, greaterV = sourceV;
			lessV.SetN(0);
			greaterV.SetN(0);
			for (int vb=vbmin; vb<=vbmax; vb++)
			{
				//Log("  Clipping v %d",ub);
				sourceV.SplitUV(*this,lessV,greaterV,0,-1,vb);
				sourceV = greaterV;
				// lessV may be safely added to the shape
				if (lessV.N()>=3)
				{
					//Log("    add");
					clipped.Add(lessV);
				}
			}
			if (sourceV.N()>=3)
			{
				//Log("    add rest");
				clipped.Add(sourceV);
			}

		}
		if (sourceU.N()>=3)
		{
			// if anything is left in sourceU, clip it agains v-boundaries

			// lessU needs to be clipped against all v-boundaries
			//Log("  u rest");
			Poly sourceV = sourceU;
			Poly lessV = sourceV, greaterV = sourceV;
			lessV.SetN(0);
			greaterV.SetN(0);
			for (int vb=vbmin; vb<=vbmax; vb++)
			{
				//Log("  Clipping v %d",vb);
				sourceV.SplitUV(*this,lessV,greaterV,0,-1,vb);
				sourceV = greaterV;
				// lessV may be safely added to the shape
				if (lessV.N()>=3)
				{
					//Log("    add");
					clipped.Add(lessV);
				}
			}
			if (sourceV.N()>=3)
			{
				//Log("    add rest");
				clipped.Add(sourceV);
			}

		}


	}
	if (clipped.Size()>0)
	{
		Log("Merge %d to %d",clipped.Size(),_face.Size());
		_face.Merge(clipped);
	}
	// new vertices might be introduced,
	// which do not correspond to any objektiv point
	// we should mark them
	if (NVertex()>_vertexToPoint.Size())
	{
		int oldSize = _vertexToPoint.Size();
		_vertexToPoint.Resize(NVertex());
		for (int i=oldSize; i<NVertex(); i++)
		{
			_vertexToPoint[i] = -1; // mark as non-corresponding to anything
		}
	}

	#endif


}

void Shape::MergeTextures(bool untile)
{
	#if 1
	if( MergedTextures.Size()<=0 )
	{
		static bool once = true;
		if (once)
		{
			// open from merged file
			QIFStream file;
			file.open("merged\\merged.ptm");
			MergedTextures.Load(file);
			once = false;			
		}
		if( MergedTextures.Size()<=0 ) return;
	}

	RefArray<Texture> needTextures;
	needTextures=_textures;

	AUTO_STATIC_ARRAY(MTextureMap,mapping,256);

	// check if there are some mergers that could be used
	// 
	while( needTextures.Size()>0 )
	{
		int setIndex=MergedTextures.FindBest(needTextures);
		if( setIndex<0 ) break; // no match - terminate
		const MTextureSet &set=MergedTextures[setIndex];
		for( int i=0; i<needTextures.Size(); )
		{
			Texture *texture=needTextures[i];
			if (!texture)
			{
				i++;
				continue;
			}
			int item=set.Contains(texture->GetName());
			if( item<0 )
			{
				i++; // advance to the next texture
				continue;
			}
			// create mapping
			// replace texture with corresponding part of texture set
			MTextureMap map;
			map.texture=texture;
			map.set=setIndex;
			map.item=item;
			mapping.Add(map);
			needTextures.Delete(i);
		}
	}

	#if 0
		// debug untiling only
		UntileTextures(mapping.Data(),mapping.Size());

		return;
	
	#else
	// it we are not able to map any texture, do not attempt untiling
		if( mapping.Size()<=0 ) return;

		// if we have more than one and are able to map only one, it is not worth doing
		if (mapping.Size()<=1 && needTextures.Size()>0) return;

		//UntileTextures(mapping.Data(),mapping.Size());

	#endif


	VertexTable old=*this;

	// TODO: use AutoStaticArray
	AutoArray<VertexIndex> oldVertexToPoint=_vertexToPoint;
	AutoArray<VertexIndex> oldPointToVertex=_pointToVertex;

	_vertexToPoint.Resize(0);
	for( int i=0; i<_pointToVertex.Size(); i++ ) _pointToVertex[i]=-1;

	// all vertices must be generated again
	Resize(0);

	// recreate list of textures
	RefArray<Texture> newTextures;
	// scan all faces and try to merge textures
	Texture *lastTexture = (Texture *)-1;
	// when we use non-merged texture
	// we should continue using it as long as possible
	for( Offset f=_face.Begin(); f<_face.End(); _face.Next(f) )
	{
		Poly &face=_face[f];
		Texture *texture=face.GetTexture();
		// check it there is some mapping ready
		int mapIndex=-1;
		if( texture!=lastTexture && (face.Special()&NoTexMerger)==0 )
		{
			for( int i=0; i<mapping.Size(); i++ )
			{
				if( mapping[i].texture==texture )
				{
					mapIndex=i;
					break;
				}
			}
		}
		if( mapIndex<0 )
		{
			if( texture ) newTextures.AddUnique(texture);
			// copy all required vertices
			for( int ii=0; ii<face.N(); ii++ )
			{
				int vi = face.GetVertex(ii);
				Vector3Val pos = old.Pos(vi);
				Vector3Val norm = old.Norm(vi);
				ClipFlags clip = old.Clip(vi);
				float u = old.U(vi);
				float v = old.V(vi);
				int pindex = oldVertexToPoint[vi];
				int vindex = AddVertex(pos,norm,clip,u,v);
				face.Set(ii,vindex);
				_vertexToPoint.Access(vindex);
				_vertexToPoint[vindex] = pindex;
				if (pindex>=0) _pointToVertex[pindex] = vindex;
			}
			lastTexture = texture;
			continue;
		}
		lastTexture = (Texture *)-1; // no last texture

		// remap all required vertices
		// try to untile texture (if possible)
		float uMin=+1e10,vMin=+1e10;
		float uMax=-1e10,vMax=-1e10;
		for( int ii=0; ii<face.N(); ii++ )
		{
			int vi=face.GetVertex(ii);
			float u=0, v=0;
			if (texture)
			{
				u=texture->UToLogical(old.U(vi));
				v=texture->VToLogical(old.V(vi));
			}
			saturateMin(uMin,u);
			saturateMin(vMin,v);
			saturateMax(uMax,u);
			saturateMax(vMax,v);
		}
		float uBase=toIntFloor(uMin)-1;
		float vBase=toIntFloor(vMin)-1;
		//const float eps=6e-3;
		//const float eps = 0.05;
		// try best fit in both directions

		const float eps = texture->IsTransparent() ? 0.10 : 0.35;

		while (uMax>uBase+1) uBase += 1;
		while (uMin<uBase-eps && uMax<uBase+1-eps) uBase -= 1;
		uMax -= uBase, uMin -= uBase;

		while (vMax>vBase+1) vBase += 1;
		while (vMin<vBase-eps && vMax<vBase+1-eps) vBase -= 1;
		vMax -= vBase, vMin -= vBase;

		if (uMin<-eps || vMin<-eps || uMax>1+eps || vMax>1+eps)
		{
			// we need to perform some splitting on the faces

			// texture is tiled
			LogF
			(
				"%s tiling prevents merging (%.3f): u=%.3f..%.3f, v=%.3f..%.3f",
				texture->Name(),eps,uMin,uMax,vMin,vMax
			);
			if( texture ) newTextures.AddUnique(texture);
			// copy all required vertices
			for( int ii=0; ii<face.N(); ii++ )
			{
				int vi=face.GetVertex(ii);
				Vector3Val pos=old.Pos(vi);
				Vector3Val norm=old.Norm(vi);
				ClipFlags clip=old.Clip(vi);
				float u=old.U(vi);
				float v=old.V(vi);
				int pindex=oldVertexToPoint[vi];
				int vindex=AddVertex(pos,norm,clip,u,v);
				face.Set(ii,vindex);
				_vertexToPoint.Access(vindex);
				_vertexToPoint[vindex]=pindex;
				if (pindex>=0) _pointToVertex[pindex]=vindex;
			}
			lastTexture = texture;
			continue;
		}

		// there is some mapping - replace texture
		const MTextureMap &map=mapping[mapIndex];
		const MTextureSet &set=MergedTextures[map.set];
		const MTextureItem &item=set[map.item];

		// note: we must update mapping
		Ref<Texture> mergedTexture=set.GetTex();
		Assert( mergedTexture );
		newTextures.AddUnique(mergedTexture);
		face.SetTexture(mergedTexture);
		//face.SetTexture(NULL);
		face.AndSpecial(~NoClamp);
		face.OrSpecial(ClampU|ClampV);
		int at = 0;

		if (mergedTexture->IsTransparent()) at |= IsTransparent;
		if (mergedTexture->IsAlpha()) at |= IsAlpha;
		face.AndSpecial(~(IsTransparent|IsAlpha));
		face.OrSpecial(at);
		for( int ii=0; ii<face.N(); ii++ )
		{
			int vi=face.GetVertex(ii);
			Vector3Val pos=old.Pos(vi);
			Vector3Val norm=old.Norm(vi);
			ClipFlags clip=old.Clip(vi);
			float u=old.U(vi);
			float v=old.V(vi);
			if (texture)
			{
				u=texture->UToLogical(u)-uBase;
				v=texture->VToLogical(v)-vBase;
			}

			// u,v are texture coordinates
			// convert them to source area coordinates
			int angle=item.angle&3;
			// consider angle
			float wr = item.w;
			float hr = item.h;
			while( --angle>=0 )
			{
				// rotate matrix
				// multiply matrix with ( 0,-1)
				// multiply matrix with (+1, 0)
				//  			    0    +1
				// 	  		   -1     0
				// uu uv    uv    -uu
				// vu vv    vv    -vu
				float un=+v;
				float vn=1-u;
				u=un;
				v=vn;
				float wrn = hr;
				float hrn = wr;
				wr = wrn;
				hr = hrn;
			}

			int scale1024=item.scale>=0 ? 1024<<item.scale : 1024>>-item.scale;
			float scale=scale1024*(1.0/1024);

			float uit = (u*wr*scale+item.x)/set.w;
			float vit = (v*hr*scale+item.y)/set.h;
			
			// check if result is in supposed range
			// note: it may be necessary to split some faces
			// in order to avoid tiling

			uit = mergedTexture->UToPhysical(uit);
			vit = mergedTexture->VToPhysical(vit);

			int pindex=oldVertexToPoint[vi];
			int vindex=AddVertex(pos,norm,clip,uit,vit);
			face.Set(ii,vindex);
			_vertexToPoint.Access(vindex);
			_vertexToPoint[vindex]=pindex;
			if (pindex>=0) _pointToVertex[pindex]=vindex;
		}
		// note: only used vertices are inserted to VertexTable
		// so there are no unused vertices now
	}
	_vertexToPoint.Compact();
	_textures=newTextures;
	// note: when using late texture merging, we need to redefine selections now
	RecalculateAreas();
	Compact();
	#endif
}

void Shape::Clear()
{
	_plane.Clear();
	_textures.Clear();

	_sel.Clear();
	_prop.Clear();
	_phase.Clear();
	
	_face.Clear();
	VertexTable::ReleaseTables();
}

void Shape::CalculateColor()
{
	// calculate average of all texture colors
	// scan all faces and weight by area
	Color sum(HBlack),sumTop(HBlack);
	float sumA=0,sumATop=0;
	float sumArea=0,sumAreaTop=0;
	for( Offset i=BeginFaces(),e=EndFaces(); i<e; NextFace(i) )
	{
		Poly &face=_face[i];
		Texture *texture=face.GetTexture();
		//float area=1;
		// calculate area
		float area=face.GetArea(*this);
		sumArea+=area;
		Color color=texture ? texture->GetColor() : Color(1, 1, 1, 1);
		sumA+=color.A()*area;
		sum+=color*(color.A()*area);
		// calculate area from top
		float areaTop=face.GetAreaTop(*this);
		sumAreaTop+=areaTop;
		sumATop+=color.A()*areaTop;
		sumTop+=color*(color.A()*areaTop);
	}
	if( sumA>0 ) sum=sum*(1/sumA);
	if( sumArea>0 ) sum.SetA(sumA/sumArea);
	//if( sum.A()<0.01 ) Fail("Alpha 0");
	_color=PackedColor(sum);
	if( sumATop>0 ) sumTop=sumTop*(1/sumATop);
	if( sumAreaTop>0 ) sumTop.SetA(sumATop/sumAreaTop);
	_colorTop=PackedColor(sumTop);
}


void Shape::AutoClamp()
{
	//const VertexMesh &mesh=_mesh;
	//if( (_special&(NoClamp|ClampU|ClampV))!=0 ) return;
	// make clamping flags per texture
	AUTO_STATIC_ARRAY(bool,tileU,256);
	AUTO_STATIC_ARRAY(bool,tileV,256);
	tileU.Resize(_textures.Size());
	tileV.Resize(_textures.Size());
	for( int i=0; i<_textures.Size(); i++ )
	{
		tileU[i]=tileV[i]=false;
	}
	for( Offset f=BeginFaces(); f<EndFaces(); NextFace(f) )
	{
		const Poly &face=Face(f);
		// ignore explicit faces
		if( face.Special()&(NoClamp|ClampU|ClampV) ) continue;
		#define CLAMP_EPS ( 1.0/128 ) // when should we clamp
		//#define CLAMP_EPS ( 0.1 ) // when should we clamp
		Texture *texture=face.GetTexture();
		if( !texture ) continue;
		int textureIndex=_textures.Find(texture);
		if( textureIndex<0 )
		{
			Fail("Texture?");
			continue;
		}
		float minU=1e10,maxU=-1e10;
		float minV=1e10,maxV=-1e10;
		for( int vi=0; vi<face.N(); vi++ )
		{
			int vv=face.GetVertex(vi);
			float u=0, v=0;
			if (texture)
			{
				u=texture->UToLogical(U(vv));
				v=texture->VToLogical(V(vv));
			}
			saturateMin(minU,u);
			saturateMax(maxU,u);
			saturateMin(minV,v);
			saturateMax(maxV,v);
		}
		// force clamping if necessary		
		if( minU<-CLAMP_EPS || maxU>1+CLAMP_EPS ) tileU[textureIndex]=true;
		if( minV<-CLAMP_EPS || maxV>1+CLAMP_EPS ) tileV[textureIndex]=true;
	}

	for( Offset f=BeginFaces(); f<EndFaces(); NextFace(f) )
	{
		Poly &face=Face(f);
		if( face.Special()&(NoClamp|ClampU|ClampV) ) continue;
		if( !face.GetTexture() )
		{
			face.OrSpecial(ClampU|ClampV);
			continue;
		}
		int textureIndex=_textures.Find(face.GetTexture());
		if( textureIndex<0 )
		{
			Fail("Texture?");
			continue;
		}
		int spec=0;
		if( !tileU[textureIndex] ) spec|=ClampU;
		if( !tileV[textureIndex] ) spec|=ClampV;
		if( !spec ) spec|=NoClamp;
		face.OrSpecial(spec);
	}
}

void Shape::BuildFaceIndexToOffset() const
{
	// already buildded
	if( NFaces()==_faceIndexToOffset.Size() ) return;
	// building neccessary
	_faceIndexToOffset.Realloc(NFaces());
	_faceIndexToOffset.Resize(NFaces());
	int i=0;
	for( Offset o=BeginFaces(); o<EndFaces(); NextFace(o),i++ )
	{
		_faceIndexToOffset[i]=o;
	}
}

void Shape::SetPoints
(
	const Vector3 *point, const ClipFlags *clip, int nPoints,
	const Vector3 *normal, int nNormals
	//const VertexMesh &mesh
)
{
	Assert( nPoints==nNormals );
	ReallocTable(nPoints);
	int i;
	for( i=0; i<nPoints; i++ )
	{
		SetPos(i)=point[i];
		SetClip(i,clip[i]);
	}
	for( i=0; i<nNormals; i++ )
	{
		SetNorm(i)=normal[i];
	}
	CalculateHints(); // precalculate hints for possible optimizations
}

#ifdef _DEBUG
	Offset OffsetChangeBase(Offset a, int b)
	{
		a.Advance(b);
		return a;
	}
#else
	Offset OffsetChangeBase(Offset a, int b)
	{
		return a+b;
	}
#endif

void Shape::Merge( const Shape *with, const Matrix4 &transform )
{
	// assume this and with have the same coordinate space
	// merge points and normals
	// create a permutation

	if( with->NPos()==with->NNorm() && NPos()==NNorm() )
	{
		enum {maxTable=256};
		AUTO_STATIC_ARRAY(int,pointWithToThis,maxTable);
		pointWithToThis.Resize(with->NPos());

		//LogF("Merge from %d",NPos());
		Reserve(with->NPos()+NPos());
		Vector3 pos,norm;
		for( int i=0; i<with->NPos(); i++ )
		{
			pos.SetFastTransform(transform,with->Pos(i));
			norm.SetRotate(transform,with->Norm(i));
			ClipFlags clip=with->Clip(i);
			int vertex=AddVertex
			(
				pos,norm,clip,with->U(i),with->V(i)
			);
			//LogF("  uv %.2f,%.2f",with->U(i),with->V(i));
			// no vertex information possible on merged shapes
			_vertexToPoint.Access(vertex);
			_vertexToPoint[vertex]=-1;
			pointWithToThis[i]=vertex;
		}
		/*
		for( int i=0; i<withMesh.NVertex(); i++ )
		{
			meshWithToThis[i]=Mesh().AddVertex
			(
				pointWithToThis[withMesh.IPos(i)],
				normalWithToThis[withMesh.INormal(i)],
				withMesh.U(i),withMesh.V(i)
			);
		}
		*/
		Assert(_face.VerifyStructure());
		Assert(with->_face.VerifyStructure());
		int offsetDiff = OffsetDiff(EndFaces(), BeginFaces());
		_face.ReserveRaw(_face.RawSize()+with->_face.RawSize());
		// merge sections
		//Offset oldOffset = with->EndFaces();
		for (int s=0; s<with->NSections(); s++)
		{
			const ShapeSection &sec = with->_face._sections[s];
			for( Offset f=sec.beg,e=sec.end; f<e; with->NextFace(f) )
			{
				Offset index=_face.Add(with->Face(f));
				Poly &face=_face[index];
				for( int v=0; v<face.N(); v++ )
				{
					face.Set(v,pointWithToThis[face.GetVertex(v)]);
				}
			}
			// merge faces
			ShapeSection dstSec = sec;
			dstSec.beg = OffsetChangeBase(sec.beg, offsetDiff);
			dstSec.end = OffsetChangeBase(sec.end, offsetDiff);
			AddSection(dstSec);
		}
		Assert(_face.VerifyStructure());
	}
	else
	{
		Fail("Merge with old points/normals");
	}
	// merge textures
	for( int i=0; i<with->_textures.Size(); i++ )
	{
		int index = _textures.AddUnique(with->_textures[i]);
		if (index<0)
		{
			index = _textures.Find(with->_textures[i]);
		}
		if (index<_areaOTex.Size())
		{
			saturateMax(_areaOTex[index],with->_areaOTex[i]);
		}
		else
		{
			_areaOTex.Access(index);
			_areaOTex[index] = with->_areaOTex[i];
		}
	}
	_special|=with->_special;
}


void Shape::MergeFast( const Shape *with, const Matrix4 &transform )
{
	// assume this and with have the same coordinate space
	// no duplicate points/normals assumed
	int i;
	// create a permutation
	//const VertexMesh &withMesh=with->Mesh();
	//VertexMesh &mesh=Mesh();
	int oldNPos=NPos();
	//int oldNNorm=NNorm();
	//int oldNVertex=mesh.NVertex();
	// reserve space for with->NPos()...

	if( with->NPos()==with->NNorm() && NPos()==NNorm() )
	{

		Reserve(with->NPos()+NPos());
		Vector3 pos,norm;
		for( int i=0; i<with->NPos(); i++ )
		{
			pos.SetFastTransform(transform,with->Pos(i));
			norm.SetRotate(transform,with->Norm(i));
			ClipFlags clip=with->Clip(i);
			int vertex=AddVertexFast
			(
				pos,norm,clip,with->U(i),with->V(i)
			);
			// no vertex information possible on merged shapes
			_vertexToPoint.Access(vertex);
			_vertexToPoint[vertex]=-1;
		}

		// merge meshes
		/*
		Assert( with->NPos()==withMesh.NVertex() );
		for( i=0; i<withMesh.NVertex(); i++ )
		{
			//int index=mesh.AddVertexUnknown();
			//Assert( index==oldNVertex+i );
			//Assert( withMesh.IPos(i)+oldNPos==index );
			//Assert( withMesh.INormal(i)+oldNNorm==index );
			mesh.Set
			(
				index,index,index,
				withMesh.U(i),withMesh.V(i)
			);
		}
		*/
		// merge faces
		_face.ReserveRaw(_face.RawSize()+with->_face.RawSize());
		for( Offset f=with->BeginFaces(),e=with->EndFaces(); f<e; with->NextFace(f) )
		{
			Offset index=_face.Add(with->Face(f));
			Poly &face=_face[index];
			for( int v=0; v<face.N(); v++ )
			{
				face.Set(v,face.GetVertex(v)+oldNPos);
			}
		}
	}
	else
	{
		Fail("Merge with old points/normals");

	}
	// merge textures
	for( i=0; i<with->_textures.Size(); i++ )
	{
		int index = _textures.AddUnique(with->_textures[i]);
		if (index<_areaOTex.Size())
		{
			saturateMax(_areaOTex[index],with->_areaOTex[i]);
		}
		else
		{
			_areaOTex.Access(index);
			_areaOTex[index] = with->_areaOTex[i];
		}
	}
	_special|=with->_special;
}

struct MergedVertex
{
	Vector3 pos;
	int n;
	int sel;
	int pointIndex;
};

TypeIsSimple(MergedVertex)

Shape *Shape::ExtractPath() // optimize this shape as path lod
{
	// scan all faces
	// replace vertices with degenerate
	// create a new shape with 2-vertex (degenerate) faces
	Shape *res = new Shape;

	res->_pointToVertex.Realloc(_pointToVertex.Size());
	res->_pointToVertex.Resize(_pointToVertex.Size());
	for( int i=0; i<res->_pointToVertex.Size(); i++ )
	{
		res->_pointToVertex[i]=-1;
	}

	// first of all convert all points to their respective "merged" variants
	// TODO: use some heuristics to estimate critical distance
	// find longest of the shortest edges

	float maxMerge2  = 0;
	for ( Offset o=BeginFaces(); o<EndFaces(); NextFace(o))
	{
		float shortestEdge2 = 1e10;
		const Poly &face = Face(o);
		for (int v=0,p=face.N()-1; v<face.N(); p=v++)
		{
			Vector3Val vpos = Pos(face.GetVertex(v));
			Vector3Val ppos = Pos(face.GetVertex(p));
			float dist2 = vpos.Distance2(ppos);
			saturateMin(shortestEdge2,dist2);
		}
		if (shortestEdge2<Square(1.5))
		{
			saturateMax(maxMerge2,shortestEdge2);
		}
	}
	maxMerge2 *= 1.01;
	LogF("  longest shortest edge %.1f",sqrt(maxMerge2));
	saturate(maxMerge2,Square(0.5),Square(1.5));
	// or use property from model

	AutoArray<MergedVertex> vertex;
	Temp<int> vertexReplacedBy(NPos());

	res->_pointToVertex.Realloc(_pointToVertex.Size());
	res->_pointToVertex.Resize(_pointToVertex.Size());
	for (int i=0; i<res->_pointToVertex.Size(); i++)
	{
		res->_pointToVertex[i] = -1;
	}
	for (int i=0; i<NPos(); i++)
	{
		Vector3Val pos = Pos(i);
		// search if the point is already present
		// check i point selection
		// check i 
		int pointI = VertexToPoint(i);
		int selI = -1;
		const char *name = "";
		for (int s=0; s<NNamedSel(); s++)
		{
			if (NamedSel(s).IsSelected(i))
			{
				Assert( selI<0 );
				selI = s;
				name = NamedSel(s).Name();
			}
		}
		float minDist2 = 1e10;
		int minJ = -1;
		for (int j=0; j<vertex.Size(); j++)
		{
			float dist2 = pos.Distance2(vertex[j].pos);
			if (dist2<minDist2)
			{
				minJ = j;
				minDist2 = dist2;
			}
		}
		if (minJ>=0 && minDist2<=maxMerge2)
		{
			MergedVertex &vtx = vertex[minJ];
			vtx.pos = (vtx.pos*vtx.n + pos) * (1/ (vtx.n+1));
			vtx.n++;

			if (selI>=0 )
			{
				if( vtx.sel>=0 && vtx.sel!=selI )
				{
					LogF
					(
						"Double selection %s, %s",
						NamedSel(selI).Name(),NamedSel(vtx.sel).Name()
					);
				}
				vtx.sel = selI;
				vtx.pointIndex = pointI;
			}
			vertexReplacedBy[i] = minJ;
		}
		else
		{
			int j = vertex.Add();
			vertex[j].pos = pos;
			vertex[j].n = 1;
			vertex[j].pointIndex = pointI;
			vertex[j].sel = selI;

			vertexReplacedBy[i] = j;
		}
	}
	// 

	for (int j=0; j<vertex.Size(); j++)
	{
		const MergedVertex &vtx = vertex[j];
		int vIndex = res->AddVertexFast(vtx.pos,VUp,ClipAll,0,0);
		res->_vertexToPoint.Access(vIndex);
		res->_vertexToPoint[vIndex] = vtx.pointIndex;
		res->_pointToVertex[vtx.pointIndex] = vIndex;
		if (vtx.sel>=0)
		{
			NamedSelection oSel=NamedSel(vtx.sel);
			SelInfo info(vIndex,255);
			NamedSelection sel(oSel.GetName(),&info,1,NULL,0);
			res->AddNamedSel(sel);
		}
	}

	for ( Offset o=BeginFaces(); o<EndFaces(); NextFace(o))
	{
		const Poly &face = Face(o);
		int n = face.N();
		//
		Poly resFace;
		resFace.Init();
		int resN = 0;
		// replace vertices 
		for (int v=0; v<n; v++)
		{
			// check edge pv,v
			int vIndex = face.GetVertex(v);
			int replaceBy = vertexReplacedBy[vIndex];
			// check if vertex is already present
			bool present = false;
			for (int p=0; p<resN; p++)
			{
				if (resFace.GetVertex(p)==replaceBy) {present = true;break;}
			}
			if (!present)
			{
				resFace.Set(resN,replaceBy);
				++resN;
			}
		}
		resFace.SetSpecial(NoClamp);
		resFace.SetN(resN);
		// longest edge is from p to v
		// vertices to merge are: p-1 with p and v with v+1

		if( resN>1)
		{
			// note: resN should be 2
			res->AddFace(resFace);
		}
	}
	res->CalculateMinMax();
	res->CalculateHints();
	res->StoreOriginalMinMax();
	res->Compact();
	return res;
}

void Shape::SortVertices()
{
	// make permutation of vertices to that _vertexToPoint is sorted
	AUTO_STATIC_ARRAY(SortVertex,sort,2048);
	sort.Resize(_vertexToPoint.Size());
	for (int i=0; i<_vertexToPoint.Size(); i++)
	{
		sort[i].vertex = i;
		sort[i].point = _vertexToPoint[i];
		//sort[i].prior = _clip[i]&ClipLightMask;
		// sort first by UserMask (Material index), then by LightMask
		sort[i].prior = _clip[i]&(ClipUserMask|ClipLightMask);
	}

	QSort(sort.Data(),sort.Size(),CmpSortVertex);
	// save current state
	AutoArray<ClipFlags> oClip = _clip;
	AutoArray<Vector3> oPos = _pos;
	AutoArray<Vector3> oNorm = _norm;
	AutoArray<VertexIndex> oVertexToPoint = _vertexToPoint;
	AutoArray<UVPair> oTex = _tex;

	// change vertices to respect a new permutation
	for (int i=0; i<_vertexToPoint.Size(); i++)
	{
		int newIndex = i;
		int oldIndex = sort[i].vertex;
		_clip[newIndex] = oClip[oldIndex];
		_pos[newIndex] = oPos[oldIndex];
		_norm[newIndex] = oNorm[oldIndex];
		_tex[newIndex] = oTex[oldIndex];
		_vertexToPoint[newIndex] = oVertexToPoint[oldIndex];
	}
	
	AUTO_STATIC_ARRAY(int,invSort,2048);
	invSort.Resize(sort.Size());
	for (int i=0; i<invSort.Size(); i++)
	{
		invSort[sort[i].vertex] = i;
	}
	// change all existing references to vertices
	for (int i=0; i<_pointToVertex.Size(); i++)
	{
		int oldI = _pointToVertex[i];
		_pointToVertex[i] = invSort[oldI];
	}
	for( Offset o=BeginFaces(),e=EndFaces(); o<e; NextFace(o) )
	{
		Poly &face = Face(o);
		for (int v=0; v<face.N(); v++)
		{
			face.Set(v,invSort[face.GetVertex(v)]);
		}
	}
	// note: some references may be in selections
	DoAssert (_sel.Size()==0);
}

class CompareTextureR
{
	public:
	int operator () ( const Poly *p0, const Poly *p1 );
};

int CompareTextureR::operator ()( const Poly *p0, const Poly *p1 )
{
	// check if some texture is alpha
	//const Texture *t0=p0->GetTexture();
	//const Texture *t1=p1->GetTexture();
	bool t0Alpha = (p0->Special()&IsAlphaOrdered)!=0;
	bool t1Alpha = (p0->Special()&IsAlphaOrdered)!=0;
	//bool t0Alpha=t0 && t0->IsAlpha();
	//bool t1Alpha=t1 && t1->IsAlpha();
	if( t0Alpha || t1Alpha )
	{
		if( t0Alpha )
		{
			if( t1Alpha ) return 0; // both alpha - keep order
			return +1;
		}
		return -1;
	}
	const Texture *t0=p0->GetTexture();
	const Texture *t1=p1->GetTexture();
	return (char *)t0-(char *)t1;
}

static int CompareTextureROffset
(
	const StreamSortSegment *s1, const StreamSortSegment *s2, FaceArray *face
)
{
	CompareTextureR functor;
	return functor(&face->Get(s1->beg),&face->Get(s2->beg));
}

void Shape::Optimize()
{
	Assert( _sel.Size()==0 );
	// note: face indices will become invalid
	CompareTextureR functor;
	_face.QSortSeq(functor);
}

void Shape::AddSection(const ShapeSection &sec)
{
	// add section, respect selection boundaries
	// scan if we match any selection
	// mark faces that need to be added
	// check if section may be split

	// faces: all face offsets from the shape
	AUTO_STATIC_ARRAY(Offset,faces,1024);
	int i=0;
	faces.Resize(NFaces());
	for( Offset o=BeginFaces(); o<EndFaces(); NextFace(o),i++ )
	{
		faces[i]=o;
	}

	// start with having one section
	AUTO_STATIC_ARRAY(ShapeSection,sections,256);
	sections.Add(sec);

	AUTO_STATIC_ARRAY(ShapeSection,selections,256);
	// selections are "sections" representing section-aware selections

	// convert all section-aware selections to section representation
	for (int i=0; i<NNamedSel(); i++)
	{
		const NamedSelection &sel = NamedSel(i);
		if (!sel.GetNeedsSections()) continue;
		// convert selection to section list
		Offset beg = Offset(0),end = Offset(0);
		bool open = false;
		for (int ff=0; ff<sel.Faces().Size(); ff++)
		{
			int fi = sel.Faces()[ff];
			Offset fo = faces[fi];
			Offset fn = fo;
			NextFace(fn);
			if (!open)
			{
				beg = fo;
				end = fn;
				open = true;
			}
			else if (fo==end)
			{
				// extend section
				end = fn;
			}
			else
			{
				// flush section
				ShapeSection &selSec = selections.Append();
				selSec.beg = beg;
				selSec.end = end;
				// open a new section
				beg = fo;
				end = fn;
			}
		}
		if (open)
		{
			ShapeSection &selSec = selections.Append();
			selSec.beg = beg;
			selSec.end = end;
		}

	}

	// we must split the section by all selections that are section aware
	// note: in most cases this will mean no splitting, but generally
	// when splitting by n sections, the result might be x^n parts


	for (int i=0; i<selections.Size(); i++)
	{
		const ShapeSection &sel = selections[i];
		// use sel to split all existing sections
		// each section
		int n = sections.Size(); // do not split sections added during splitting
		for (int s=0; s<n; s++)
		{
			ShapeSection &sec = sections[s];
			// nothing to do if sec and sel do not overlap
			if (sec.beg>=sel.end || sec.end<=sel.beg) continue;

			// split sec.beg...sec.end by two boundaries: sel.beg and sel.end
			if (sel.beg>sec.beg && sel.beg<sec.end)
			{
				// sel.beg inside sec - split neccessary
				ShapeSection &secNew = sections.Append();
				secNew.properties = sec.properties;
				secNew.beg = sec.beg;
				secNew.end = sel.beg;
				sec.beg = sel.beg;
				// 
			}
			if (sel.end>sec.beg && sel.end<sec.end)
			{
				// sel.end inside sec - split neccessary
				ShapeSection &secNew = sections.Append();
				secNew.properties = sec.properties;
				secNew.beg = sel.end;
				secNew.end = sec.end;
				sec.end = sel.end;
			}
			// splitting is necessary

		}

	}
	// add all constructed sections
	for (int i=0; i<sections.Size(); i++)
	{
		_face._sections.Add(sections[i]);
	}


}

static int CmpSection(const ShapeSection *s1, const ShapeSection *s2)
{
	if (s1->beg>s2->beg) return +1;
	if (s1->beg<s2->beg) return -1;
	return 0;
}

//! integer range - interval
/*!
	used to represent set of integers: i>=beg && i<end
*/

struct Interval
{
	int beg,end;

	bool IsInside(int i) const {return i>=beg && i<end;}
	bool IsOverlapped(const Interval &j) const
	{
		return j.end>beg && j.beg<end;
		//if (j.end>beg) return false;
		//if (j.beg<end) return false;
		//return true;
	}
};

TypeIsSimple(Interval)

//! memory effective implementation of
//! integer set containing many continuos regions

class IntervalArray
{
	AutoArray<Interval> _data;

	public:
	IntervalArray();
	~IntervalArray();

	void Delete(int i);

	void AddInterval(int beg, int end);
	bool IsPresent(int beg) const;
	bool IsEmpty() const {return _data.Size()==0;}

	int NIntervals() const {return _data.Size();}

	const Interval &GetInterval(int i) const {return _data.Get(i);}
	void Add(int i){AddInterval(i,i+1);}

	bool CheckIntegrity() const;
};

IntervalArray::IntervalArray()
{
}
IntervalArray::~IntervalArray()
{
}

#define LOG_INTERVAL_OPS 0

void IntervalArray::Delete(int index)
{
	Assert(CheckIntegrity());
	#if LOG_INTERVAL_OPS
		Log("%08x: delete %d",this,index);
	#endif
	#if _DEBUG
	bool present = false;
	for (int i=0; i<_data.Size(); i++)
	{
		Interval &ii = _data[i];
		if (!ii.IsInside(index)) continue;
		present = true;
	}
	Assert (present);
	#endif

	for (int i=0; i<_data.Size();)
	{
		//Interval &ii = _data[i];
		if (!_data[i].IsInside(index))
		{
			i++;
			continue;
		}
	#if LOG_INTERVAL_OPS
		int obeg = ii.beg;
		int oend = ii.end;
	#endif
		// we found given interval
		// if one of end-points, we may simply remove it
		if (_data[i].beg<index && _data[i].end>index+1)
		{
			// we need to split given interval
			Interval &ni = _data.Append();
			ni.beg = index+1; // new interval (index,end)
			ni.end = _data[i].end;
			_data[i].end = index; // old interval <beg,index)
			Assert(ni.end>ni.beg);
			Assert(_data[i].end>_data[i].beg);
			Assert(CheckIntegrity());
	#if LOG_INTERVAL_OPS
				Log("  %d..%d split to %d..%d, %d..%d",obeg,oend,ii.beg,ii.end,ni.beg,ni.end);
	#endif
		}
		else
		{
			Assert(_data[i].end>_data[i].beg);
			if (_data[i].beg==index) _data[i].beg++;
			else if (_data[i].end==index+1) _data[i].end--;
			if (_data[i].end==_data[i].beg)
			{
	#if LOG_INTERVAL_OPS
				Log("  %d..%d deleted",obeg,oend);
	#endif
				_data.Delete(i);
				Assert(CheckIntegrity());
				break;
			}
			else
			{
				#if LOG_INTERVAL_OPS
				Log("  %d..%d changed to %d..%d",obeg,oend,_data[i].beg,_data[i].end);
				#endif
				Assert(CheckIntegrity());
				Assert(_data[i].end>_data[i].beg);
			}
		}
		// assume any number is contained in max. one interval
		break;
	}
	// verify point is not present in any other selection
	#if _DEBUG
	for (int i=0; i<_data.Size(); i++)
	{
		Interval &ii = _data[i];
		if (!ii.IsInside(index)) continue;
		Fail("Index present twice");
	}
	#endif
	Assert(CheckIntegrity());
}

void IntervalArray::AddInterval(int beg, int end)
{
	#if LOG_INTERVAL_OPS
	Log("%08x: add %d..%d",this,beg,end);
	#endif
	Assert(end>beg);
	// if interval is overlapping or touching any other, merge it
	for (int i=0; i<_data.Size(); i++)
	{
		Interval &ii = _data[i];
		if (ii.end>beg) continue;
		if (ii.beg<end) continue;
		#if LOG_INTERVAL_OPS
		int obeg = ii.beg;
		int oend = ii.end;
		#endif
		saturateMin(ii.beg,beg);
		saturateMax(ii.end,end);
		Assert(ii.end>ii.beg);
		#if LOG_INTERVAL_OPS
			Log("  %d..%d changed to %d..%d",obeg,oend,ii.beg,ii.end);
		#endif
		Assert(CheckIntegrity());
		return;
	}

	Assert(end>beg);
	Interval &ni = _data.Append();
	ni.beg = beg;
	ni.end = end;
	Assert(CheckIntegrity());
}

bool IntervalArray::IsPresent(int index) const
{
	for (int i=0; i<_data.Size(); i++)
	{
		const Interval &ii = _data[i];
		if (ii.IsInside(index)) return true;
	}
	return false;
}

bool IntervalArray::CheckIntegrity() const
{
	for (int i=0; i<_data.Size(); i++)
	{
		const Interval &ii = _data[i];
		if (ii.beg>=ii.end)
		{
			ErrF("ii.beg>=ii.end: %d,%d",ii.beg,ii.end);
			return false;
		}
	}
	for (int i=0; i<_data.Size(); i++) for (int j=0; j<i; j++)
	{
		const Interval &ii = _data[i];
		const Interval &jj = _data[j];
		if (ii.IsOverlapped(jj))
		{
			ErrF
			(
				"Overlapping %d and %d (%d..%d, %d..%d)",
				i,j,ii.beg,ii.end,jj.beg,jj.end
			);
			return false;
		}
	}
	return true;
}

struct ShapeSectionWork: public ShapeSectionInfo
{
	// used during section creation
	//FindArray<int> _faces; // set of face indices
	IntervalArray _faces;
};

TypeIsMovable(ShapeSectionWork)

struct FindShapeSections
{
	Shape *_shape;

	AutoArray<ShapeSectionWork> _sections;
	AutoArray<Offset> _offsets; // face index->offset helper

	FindShapeSections(Shape *shape);

	void CreateOffsets();
	int FindSection(const PolyProperties &prop, int material) const;
	int AssignSection(const PolyProperties &prop, int material);

	void SplitByNamedSelection
	(
		ShapeSectionWork &src,
		ShapeSectionWork &split, // what is split-out
		const NamedSelection &sel
	);

	void SplitByNamedSelection(const NamedSelection &sel);

	void Perform(bool forceMaterial0);
};

void FindShapeSections::CreateOffsets()
{
	_offsets.Resize(_shape->NFaces()+1);
	int i=0;
	for( Offset o=_shape->BeginFaces(); o<_shape->EndFaces(); _shape->NextFace(o),i++ )
	{
		_offsets[i]=o;
	}
	_offsets[i] = _shape->EndFaces();
}

int FindShapeSections::FindSection(const PolyProperties &prop, int material) const
{
	for (int i=0; i<_sections.Size(); i++)
	{
		const ShapeSectionWork &sw = _sections[i];
		if (sw.material!=material) continue;
		if (sw.properties.GetTexture()!=prop.GetTexture()) continue;
		if (sw.properties.Special()!=prop.Special()) continue;
		return i;
	}
	return -1;
}

int FindShapeSections::AssignSection(const PolyProperties &prop, int material)
{
	int index = FindSection(prop,material);
	if (index>=0) return index;
	index = _sections.Add();
	ShapeSectionWork &sw = _sections[index];
	sw.material = material;
	sw.properties = prop;
	return index;
}

FindShapeSections::FindShapeSections(Shape *shape)
:_shape(shape)
{
}

void FindShapeSections::SplitByNamedSelection
(
	ShapeSectionWork &src,
	ShapeSectionWork &split, // what is split-out
	const NamedSelection &sel
)
{
	split.material = src.material;
	split.properties = src.properties;
	const Selection &faces = sel.Faces();
	for (int i=0; i<faces.Size(); i++)
	{
		int fi = faces[i];
		// check if fo is in src
		// if it is in both section and selection, move it to split
		/*
		int index = src._faces.Find(fi);
		if (index>=0)
		{
			split._faces.Add(fi);
			src._faces.DeleteAt(index);
		}
		*/
		if (src._faces.IsPresent(fi))
		{
			split._faces.Add(fi);
			src._faces.Delete(fi);
		}
	}
	// src and split are now both correct in relation to sel
	// one (both?) of them may be empty now
}

void FindShapeSections::SplitByNamedSelection(const NamedSelection &sel)
{
	int ns = _sections.Size();
	// if _sections will grow, there is no need to test new sections
	for (int i=0; i<ns; i++)
	{
		ShapeSectionWork &sw = _sections[i];
		ShapeSectionWork split;
		SplitByNamedSelection(sw,split,sel);
		if (split._faces.IsEmpty()) continue;
		if (sw._faces.IsEmpty())
		{
			// nothing left in sw, use split instead of sw
			sw = split;
		}
		else
		{
			// two part - insert a new one
			_sections.Add(split);
		}
		
	}
}

struct SortSecPoly
{
	Offset polyBeg, polyEnd;
	const ShapeSectionWork *sec;
	int polyIndexBeg,polyIndexEnd;
};

TypeIsSimple(SortSecPoly)

static int CompareSecPoly(const SortSecPoly *s1, const SortSecPoly *s2)
{
	// first sort by section
	const ShapeSectionWork *sec1 = s1->sec;
	const ShapeSectionWork *sec2 = s2->sec;
	// sections: first sort by texture
	const Texture *t1 = sec1->properties.GetTexture();
	const Texture *t2 = sec2->properties.GetTexture();
	// and must retain given order
	// note: due to nasty trick used in tank cockpits
	// all paa textures should retain in given order
	bool t1Alpha = (sec1->properties.Special()&IsAlphaOrdered)!=0;
	bool t2Alpha = (sec2->properties.Special()&IsAlphaOrdered)!=0;
	//bool t1Alpha = t1 && t1->IsAlpha();
	//bool t2Alpha = t2 && t2->IsAlpha();
	if (t1Alpha!=t2Alpha)
	{
		// alpha transparent goes last
		return t1Alpha-t2Alpha;
	}
	if (t1Alpha)
	{
		// both alpha - retain order
		if (s1->polyBeg>s2->polyBeg) return +1;
		if (s1->polyBeg<s2->polyBeg) return -1;
		return 0;
	}
	// non-alpha: may sort by section
	// TODO: alpha order vs. sections
	// none alpha: sort by texture
	int d = (const char *)t1-(const char *)t2;
	if (d) return d;
	// next by special
	d = sec1->properties.Special()-sec2->properties.Special();
	if (d) return d;
	// next by material
	d = sec1->material-sec2->material;
	if (d) return d;
	// sections may have equal properties, but need to be
	// distiguished anyway
	d = (const char *)sec1-(const char *)sec2;
	if (d) return d;
	// any order will do
	return 0;
}


void FindShapeSections::Perform(bool forceMaterial0)
{
	bool neededSplit = false;
	for (int i=0; i<_shape->NNamedSel(); i++)
	{
		const NamedSelection &sel = _shape->NamedSel(i);
		if (!sel.GetNeedsSections()) continue;
		neededSplit = true;
	}
	if (!neededSplit)
	{
		// detect singular case: sections already contiguos
		// each section may start only once
		AUTO_FIND_ARRAY(ShapeSection,sections,256);
		ShapeSection *openSec = NULL;
		bool singular = true;
		for (Offset o = _shape->BeginFaces(); o<_shape->EndFaces(); _shape->NextFace(o))
		{
			const Poly &poly = _shape->Face(o);
			int mat = forceMaterial0 ? 0 : poly.GetMaterial(*_shape);
			// if aplha sorting is required, we cannot use singular case
			if (poly.Special()&IsAlphaOrdered)
			{
				singular = false;
				break;
			}

			if (!openSec)
			{
				// no section started yet
				openSec = &sections.Append();
				openSec->material = mat;
				openSec->properties = poly;
				openSec->beg = o;
				openSec->end = o;
			}
			else if
			(
				openSec->material!=mat ||
				openSec->properties.GetTexture()!=poly.GetTexture() ||
				openSec->properties.Special()!=poly.Special()
			)
			{
				// check if section was already opened
				ShapeSection info;
				info.properties = poly;
				info.material = mat;
				if (sections.Find(info))
				{
					singular = false;
					break;
				}
				// close section
				openSec->end = o;
				_shape->NextFace(openSec->end);
				// section closed  - we need to open next section
				openSec = &sections.Append();
				openSec->material = mat;
				openSec->properties = poly;
				openSec->beg = o;
				openSec->end = o;
			}
		}
		if (singular)
		{
			// perform simplified conversion to sections
			// close last open sections if necessary
			if (openSec)
			{
				openSec->end = _shape->EndFaces();
			}
			// convert information from sections to _sections
			_shape->Faces().SetSections(sections.Data(),sections.Size());

			return;
		}
	}

	CreateOffsets();
	// define sections by properties
	int lastSection = -1;
	int lastSectionStart = -1;
	for (int i=0; i<_shape->NFaces(); i++)
	{
		Offset fo = _offsets[i];
		const Poly &poly = _shape->Face(fo);
		int mat = forceMaterial0 ? 0 : poly.GetMaterial(*_shape);
		// note: polygons are often almost ordered
		// best candidate to search for section
		// is therefore section of last polygon
		if (lastSection>=0)
		{
			ShapeSectionWork &ls = _sections[lastSection];
			if
			(
				ls.properties.GetTexture()==poly.GetTexture() &&
				ls.properties.Special()==poly.Special() &&
				ls.material==mat
			)
			{
				continue;
			}
			Assert(i>lastSectionStart);
			ls._faces.AddInterval(lastSectionStart,i);
			lastSectionStart = i;
		}
		int sec = AssignSection(poly,mat);
		//ShapeSectionWork &sw = _sections[sec];
		//sw._faces.Add(i);
		lastSection = sec;
		lastSectionStart = i;
	}
	if (lastSection>=0)
	{
		ShapeSectionWork &ls = _sections[lastSection];
		Assert(_shape->NFaces()>lastSectionStart);
		ls._faces.AddInterval(lastSectionStart,_shape->NFaces());
	}
	// check all sections against all section-aware selections
	for (int i=0; i<_shape->NNamedSel(); i++)
	{
		const NamedSelection &sel = _shape->NamedSel(i);
		if (!sel.GetNeedsSections()) continue;
		if (sel.Faces().Size()<=0) continue;
		// we must check all sections against this one
		SplitByNamedSelection(sel);
	}

	// sort faces to that sections are contiguos
	AUTO_STATIC_ARRAY(SortSecPoly,sortFaces,256);
	sortFaces.Realloc(_shape->NFaces());
	// insert polygons from all sections
	for (int i=0; i<_sections.Size(); i++)
	{
		const ShapeSectionWork &sw = _sections[i];
		for (int f=0; f<sw._faces.NIntervals(); f++)
		{
			const Interval &i = sw._faces.GetInterval(f);
			Assert (i.end>i.beg);
			Offset fBeg = _offsets[i.beg];
			Offset fEnd = _offsets[i.end];
			SortSecPoly &sp = sortFaces.Append();
			sp.polyBeg = fBeg;
			sp.polyEnd = fEnd;
			sp.polyIndexBeg = i.beg;
			sp.polyIndexEnd = i.end;
			Assert (fEnd>fBeg);
			sp.sec = &sw;
		}
	}
	Assert(sortFaces.Size()<=_shape->NFaces());
	// sort by alpha, texture, material ... as neccessary
	QSort(sortFaces.Data(),sortFaces.Size(),CompareSecPoly);
	// sortFaces is sorted
	// we have to reorder faces in shape now
	#if 0
	// dump info about textures
	for (int i=0; i<sortFaces.Size(); i++)
	{
		SortSecPoly &sp = sortFaces[i];
		LogF
		(
			"  %d:sp %s,%x",
			i,(const char *)sp.poly->GetDebugText(),sp.sec
		);
	}
	#endif

	AutoArray<int> reorder;
	reorder.Resize(_shape->NFaces());
	int order = 0;
	for (int i=0; i<sortFaces.Size(); i++)
	{
		int siBeg = sortFaces[i].polyIndexBeg;
		int siEnd = sortFaces[i].polyIndexEnd;
		for (int si=siBeg; si<siEnd; si++)
		{
			reorder[si] = order++;
		}
	}

	// this means reordering all named selections
	for (int s=0; s<_shape->NNamedSel(); s++)
	{
		NamedSelection &sel = _shape->NamedSel(s);
		if (sel.Faces().Size()<=0) continue;
		Assert (!sel.FaceOffsetsReady());
		Selection &faceSel = sel.Faces();

		AutoArray<VertexIndex> newSel;
		newSel.Resize(faceSel.Size());
		for (int f=0; f<faceSel.Size(); f++)
		{
			int of = faceSel[f];
			int nf = reorder[of];
			newSel[f] = nf;
		}
		faceSel = Selection(newSel.Data(),newSel.Size());
	}
	// now we can reorder all faces 
	// we may directly create sections during final sort
	FaceArray sorted;
	sorted.RawRealloc(_shape->FacesRawSize());
	ShapeSection sec;
	const ShapeSectionWork *lastSec = NULL;
	AutoArray<ShapeSection> sections;
	for (int i=0; i<sortFaces.Size(); i++)
	{
		const SortSecPoly &sp	= sortFaces[i];
		Offset oBeg = sorted.End();
		sorted.Merge
		(
			(const char *)&_shape->Face(sp.polyBeg),
			OffsetDiff(sp.polyEnd,sp.polyBeg),
			sp.polyIndexEnd-sp.polyIndexBeg
		);
		Offset oEnd = sorted.End();

		if (sp.sec!=lastSec)
		{
			if (lastSec)
			{
				sections.Add(sec);
			}
			sec.properties = sp.sec->properties;
			sec.material = sp.sec->material;
			sec.beg = oBeg;
			sec.end = oEnd;
			lastSec = sp.sec;
		}
		else
		{
			Assert(sec.end==oBeg);
			sec.end = oEnd;
		}
	}
	if (lastSec)
	{
		sections.Add(sec);
	}
	sorted.SetSections(sections.Data(),sections.Size());
	_shape->SetFaces(sorted);
	// now we have to check which selections contain which sections
	// check on by-selection basis
	// it is guranteed now section is either contained whole
	// or not at all in any section-aware selection

	#if 0
	// dump info about all sections
	for (int i=0; i<_shape->NSections(); i++)
	{
		const ShapeSection &sec = _shape->GetSection(i);
		LogF
		(
			"Section %d -  %s (%d..%d)",
			i,(const char *)sec.properties.GetDebugText(),
			sec.beg,sec.end
		);
	}
	#endif
	// check all sections against all section-aware selections
	for (int i=0; i<_shape->NNamedSel(); i++)
	{
		NamedSelection &sel = _shape->NamedSel(i);
		if (!sel.GetNeedsSections()) continue;
		if (sel.Faces().Size()<=0) continue;
		sel.FaceOffsets(_shape); // get ready face offsets
		sel.RescanSections(_shape);
	}
}

void ShapeSection::SerializeBin(SerializeBinStream &f, Shape *shape)
{
	f.TransferBinary(&beg,sizeof(beg));
	f.TransferBinary(&end,sizeof(end));
	f.Transfer(material);
	// transfer poly properties
	// use Shape to get texture index
	if (f.IsSaving())
	{
		Texture *tex = properties.GetTexture();
		int index = tex ? shape->GetTextureIndex(tex) : -1;
		f.SaveShort(index);
		f.SaveInt(properties.Special());
	}
	else
	{
		int index = f.LoadShort();
		Texture *texture = index>=0 ? shape->GetTexture(index) : NULL;
		properties.SetTexture(texture);
		properties.SetSpecial(f.LoadInt());
		surfMat = GTexMaterialBank.TextureToMaterial(texture);
		/*
		int flags = 0;
		if (texture)
		{
			if (texture->IsAnimated()) flags |= ::IsAnimated;
			if (texture->IsAlpha()) flags |= ::IsAlpha;
			if (texture->IsTransparent()) flags |= ::IsTransparent;
		}
		properties.OrSpecial(flags);
		*/
	}
}

void ShapeSection::PrepareTL
(
	const TLMaterial &mat, const LightList &lights, int spec
) const
{
	if (surfMat)
	{
		// modulate material by material given in section
		TLMaterial matmod;
		surfMat->Combine(matmod,mat);
		GEngine->SetMaterial(matmod,lights,spec);
	}
	else
	{
		// set material given by model material supplier
		GEngine->SetMaterial(mat,lights,spec);
	}
	properties.PrepareTL();
}


void Shape::FindSections(bool forceMaterial0)
{
	FindShapeSections find(this);
	find.Perform(forceMaterial0);
}

const char *LevelName(float resolution)
{
	static char buf[256];
	if( IsSpec(resolution,GEOMETRY_SPEC) ) return "geometry";
	else if( IsSpec(resolution,MEMORY_SPEC) ) return "memory";
	else if( IsSpec(resolution,LANDCONTACT_SPEC) ) return "landContact";
	else if( IsSpec(resolution,ROADWAY_SPEC) ) return "roadway";
	else if( IsSpec(resolution,PATHS_SPEC) ) return "paths";
	else if( IsSpec(resolution,HITPOINTS_SPEC) ) return "hitpoints";
	else if( IsSpec(resolution,VIEW_GEOM_SPEC) ) return "geometryView";
	else if( IsSpec(resolution,FIRE_GEOM_SPEC) ) return "geometryFire";
	else if( IsSpec(resolution,VIEW_PILOT_GEOM_SPEC) ) return "geometryViewPilot";
	else if( IsSpec(resolution,VIEW_GUNNER_GEOM_SPEC) ) return "geometryViewGunner";
	else if( IsSpec(resolution,VIEW_COMMANDER_GEOM_SPEC) ) return "geometryViewCommander";
	else if( IsSpec(resolution,VIEW_CARGO_GEOM_SPEC) ) return "geometryViewCargo";
	sprintf(buf,"%g",resolution);
	return buf;
}

static const char *LevelName(LODShape *shape, Shape *level)
{
	for (int i=0; i<shape->NLevels(); i++)
	{
		if (shape->Level(i)==level) return LevelName(shape->Resolution(i));
	}
	return "Error";
}


void LODShape::Optimize()
{
	for_each_alpha
	for( int level=0; level<NLevels(); level++ )
	{
		Shape *shape=Level(level);
		if( !shape ) continue;
		shape->Optimize();
	}
}

void LODShape::Translate( Vector3Par offset )
{
	for_each_alpha
	for( int level=0; level<NLevels(); level++ )
	{
		Shape *shape=Level(level);
		if( !shape ) continue;
		for( int i=0; i<shape->_pos.Size(); i++ )
		{
			V3 &pos=shape->_pos[i];
			pos+=offset;
		}
		shape->RecalculateNormals(false);
	}
	_minMax[0]+=offset;
	_minMax[1]+=offset;
	_boundingCenter-=offset;
	if( _massArray.Size()>0 ) CalculateMass();
	ScanProperties();
}

void LODShape::InternalTransform( const Matrix4 &transform )
{
	for_each_alpha
	for( int level=0; level<NLevels(); level++ )
	{
		Shape *shape=Level(level);
		if( !shape ) continue;
		for( int i=0; i<shape->_pos.Size(); i++ )
		{
			V3 &pos=shape->_pos[i];
			pos+=_boundingCenter; // restore original position
			pos.SetFastTransform(transform,pos);
		}
		for( int p=0; p<shape->_phase.Size(); p++ )
		{
			for( int i=0; i<shape->_phase[p].Size(); i++ )
			{
				Vector3 &pos=shape->_phase[p][i];
				pos+=_boundingCenter;
				pos.SetFastTransform(transform,pos);
			}
		}
		_boundingCenter=VZero;
		shape->RecalculateNormals(true);
		shape->RecalculateAreas();
	}
	CalculateMinMax(true);
	if( _massArray.Size()>0 ) CalculateMass();
	ScanProperties();
}

bool Shape::VerifyStructure() const
{
	return true;
	//return _face.VerifyStructure();
}

void Shape::SetFaces(const FaceArray &src)
{
	_face = src;
	// all named selections must be reordered before calling this function
	//_sel.Clear();
	// any other face offset should be invalidated
	_faceIndexToOffset.Clear();
}
void Shape::SetSpecial( int special )
{
	_special=special;
	for( Offset i=BeginFaces(),e=EndFaces(); i<e; NextFace(i) )
	{
		Face(i).SetSpecial(special);
	}
	for (int s=0; s<_face._sections.Size(); s++)
	{
		_face._sections[s].properties.SetSpecial(special);
	}
}

/*!
\patch_internal 1.21 Date 8/16/2001 by Ondra.
- Fixed: Or/AndSpecial functions did not modify T&L sections.
- This made grass effects experiments impossible.
- This change should not affect any older code.
*/

void Shape::MakeCockpit()
{
	const int flags=NoDropdown;
	_special|=flags;
	for( Offset i=BeginFaces(),e=EndFaces(); i<e; NextFace(i) )
	{
		Face(i).OrSpecial(flags);
		// some faces can be with no texture at all
		Texture *tex = Face(i).GetTexture();
		if (tex)
		{
			if (tex->AMaxSize()<Glob.config.maxCockText)
			{
				tex->SetMaxSize(Glob.config.maxCockText);
			}
		}
	}
}

void Shape::OrSpecial( int special )
{
	_special|=special;
	for( Offset i=BeginFaces(),e=EndFaces(); i<e; NextFace(i) )
	{
		//_face[i].SetSpecial(_face[i].Special()|special);
		_face[i].OrSpecial(special);
	}
	for (int i=0; i<_face._sections.Size(); i++)
	{
		_face._sections[i].properties.OrSpecial(special);
	}
}
void Shape::AndSpecial( int special )
{
	_special&=special;
	for( Offset i=BeginFaces(),e=EndFaces(); i<e; NextFace(i) )
	{
		//_face[i].SetSpecial(_face[i].Special()&special);
		_face[i].AndSpecial(special);
	}
	for (int i=0; i<_face._sections.Size(); i++)
	{
		_face._sections[i].properties.AndSpecial(special);
	}
}

void Shape::Triangulate()
{
	const FaceArray source = _face;
	_face.Clear(); 
	for( Offset i=source.Begin(),e=source.End(); i<e; source.Next(i) )
	{
		Poly src=source[i];
		int nv=src.N();
		if( nv>3 )
		{
			#ifdef __ICL
			#pragma novector
			#endif
			// this one needs to be trianglulated
			for( int v=2; v<nv-1; v++ )
			{
				// use triangle 0,v,v+1
				Poly dst;
				dst.Set(0,src.GetVertex(0));
				dst.Set(1,src.GetVertex(v));
				dst.Set(2,src.GetVertex(v+1));
				dst.CopyProperties(src);
				dst.SetN(3);
			}
			src.SetN(3);
		}
		_face.Add(src);
	}
}

Texture *Shape::FindTexture( const char *name ) const
{
	for (int t=0; t<_textures.Size(); t++)
	{
		Texture *tex = _textures[t];
		if (tex && !strcmp(tex->Name(),name)) return tex;
	}
	return NULL;
}

Texture *LODShape::FindTexture( const char *name ) const
{
	for (int level=0; level<NLevels(); level++)
	{
		Texture *tex = Level(level)->FindTexture(name);
		if (tex) return tex;
	}
	return NULL;
}


void Shape::RegisterTexture( Texture *texture, float areaOTex )
{
	if (!texture) return;
	// check if texture is present
	for (int t=0; t<_textures.Size(); t++)
	{
		if (_textures[t]==texture)
		{
			saturateMax(_areaOTex[t],areaOTex);
			return;
		}
	}
	int index = _textures.Add(texture);
	_areaOTex.Access(index);
	_areaOTex[index] = areaOTex;
}

void Shape::RegisterTexture( Texture *texture, Texture *oldTexture )
{
	if (!texture) return;
	int index = -1;
	float areaOTex = 0;
	// check if texture is present
	for (int t=0; t<_textures.Size(); t++)
	{
		if (_textures[t]==texture)
		{
			index = t;
			saturateMax(areaOTex,_areaOTex[t]);
			break;
		}
	}

	for (int t=0; t<_textures.Size(); t++)
	{
		if (_textures[t]==oldTexture)
		{
			saturateMax(areaOTex,_areaOTex[t]);
			break;
		}
	}
	if (index<0)
	{
		Assert(_textures.Size()==_areaOTex.Size() );
		index = _textures.Add(texture);
		_areaOTex.Add(areaOTex);
	}
	else
	{
		_areaOTex[index] = areaOTex;
	}
}

void Shape::RegisterTexture( Texture *texture, int selection )
{
	if (selection<0) return;
	if (!texture) return;
	// get normal texture from selection
	const NamedSelection &sel = NamedSel(selection);
	const Selection &faceSel = sel.Faces();
	if (faceSel.Size()<1 ) return; // empty selection
	int faceIndex = faceSel[0];
	const Poly &face = FaceIndexed(faceIndex);
	Texture *oldTexture = face.GetTexture();
	RegisterTexture(texture,oldTexture);
}

void LODShape::RegisterTexture( Texture *texture, const Animation &anim )
{
	// scan all LOD-levels
	if (!texture) return;
	for (int i=0; i<NLevels(); i++)
	{
		Shape *shape = Level(i);
		shape->RegisterTexture(texture,anim.GetSelection(i));
	}
}


void Shape::RecalculateAreas()
{
	// scan all faces with same texture
	AUTO_STATIC_ARRAY(float,areas,256);
	AUTO_STATIC_ARRAY(float,texts,256);
	_areaOTex.Realloc(_textures.Size());
	_areaOTex.Resize(_textures.Size());
	areas.Resize(_textures.Size());
	texts.Resize(_textures.Size());
	for (int i=0; i<_textures.Size(); i++)
	{
		_areaOTex[i] = 0;
		areas[i] = 0;
		texts[i] = 0;
	}
	const float maxAOT = 1e14;
	for( Offset i=BeginFaces(),e=EndFaces(); i<e; NextFace(i) )
	{
		Poly &face=Face(i);
		if (face.N()<3)
		{
			ErrF("Degenerate face - %d vertices",face.N());
			continue;
		}
		int index=_textures.Find(face.GetTexture());
		if( index>=0 )
		{
			/**/
			float area = face.CalculateArea(*this);
			float text = face.CalculateUVArea(*this);

			float aot = maxAOT;
			if (area<maxAOT*text) aot = area/text;
			saturateMax(_areaOTex[index],aot);
		}
	}
	#if 0
	LogF("shape %x",this);
	for( int i=0; i<_textures.Size(); i++ )
	{
		if (_textures[i])
		{
			LogF("  Texture %s area %.2f",_textures[i]->Name(),_areaOTex[i]);
		}
	}
	#endif
}

void Shape::InitPlanes()
{
	if( _plane.Size()>0 ) return;
	_plane.Realloc(_face.Size());
	_plane.Resize(_face.Size());
	RecalculateNormals(true);
}

void Shape::InvalidateNormals()
{
	_faceNormalsValid=false;
	InvalidateMinMax();
}

void Shape::RecalculateNormals( bool full )
{
	if( !full && !_faceNormalsValid ) return;
	if( _plane.Size()>0 )
	{
		int p=0;
		for( Offset i=BeginFaces(),e=EndFaces(); i<e; NextFace(i),p++ )
		{
			Poly &face=Face(i);
			Plane &plane=GetPlane(p);
			if( full )
			{
				face.CalculateNormal(plane,*this);
				//face.CalculateArea(_mesh);
			}
			else face.CalculateD(plane,*this);
		}
		Assert( p==_plane.Size() );
	}
	_faceNormalsValid=true;

	// if necessary, recalculate bsphere and minmax box
	if (_minMaxDirty)
	{
		CalculateMinMax();
	}
	//CalculateMinMax();
}

void Shape::Compact()
{
	VertexTable::Compact();
	_face.Compact();
	_sel.Compact();
	_prop.Compact();
	_phase.Compact();
	_textures.Compact();
}

void Shape::AddPhase( const AnimationPhase &phase )
{
	int i=0;
	for( ; i<_phase.Size(); i++ )
	{
		if( _phase[i].Time()>phase.Time() ) break;
	}
	_phase.Insert(i,phase);
}

int Shape::PrevAnimationPhase( float time ) const
{
	int i,n=_phase.Size();
	int prev=n-1,next=0;
	float prevTime=-1e10,nextTime=+1e10;
	for( i=0; i<n; i++ )
	{
		float pTime=_phase[i].Time();
		if( pTime<=time && pTime>prevTime ) prevTime=pTime,prev=i;
		if( pTime>time && pTime<nextTime ) nextTime=pTime,next=i;
	}
	return prev;
}

int Shape::NextAnimationPhase( float time ) const
{
	int i,n=_phase.Size();
	int prev=n-1,next=0;
	float prevTime=-1e10,nextTime=+1e10;
	for( i=0; i<n; i++ )
	{
		float pTime=_phase[i].Time();
		if( pTime<=time && pTime>prevTime ) prevTime=pTime,prev=i;
		if( pTime>time && pTime<nextTime ) nextTime=pTime,next=i;
	}
	return next;
}

void Shape::SetPhaseIndex( int index )
{
	const AnimationPhase &pos=_phase[index];
	for( int i=0; i<NPos(); i++ )
	{
		SetPos(i)=pos[i];
	}
}

void Shape::PreparePhase
(
	const AnimationPhase *&prevPos, const AnimationPhase *&nextPos, float &interpol,
	float time, float baseTime
) const
{
	bool looped = baseTime>=0;
	// interpolate between two nearest phases
	prevPos=nextPos=&_phase[0]; // default to first phasis
	int n=_phase.Size();
	if( n<2 ) return; // no animation
	int i = 0;
	int prev = 0, next = n-1;
	//float prevTime=_phase[0].Time(),nextTime=_phase[next].Time();
	float prevTime = -1e10, nextTime = +1e10;
	if (looped)
	{
		// ignore out of range negative phases
		for( i=0; i<n; i++ )
		{
			float pTime=_phase[i].Time();
			if( pTime>baseTime-0.1 ) break;
		}
		if( i>=n ) return; // no positive phase
		prev = n-1,next = i;
		prevTime = -1e10, nextTime = +1e10;
	}
	for( ; i<n; i++ )
	{
		float pTime=_phase[i].Time();
		if( looped && pTime>=baseTime+1 ) break;
		if( pTime<=time && pTime>prevTime ) prevTime=pTime,prev=i;
		if( pTime>time && pTime<nextTime ) nextTime=pTime,next=i;
	}
	prevPos=&_phase[prev];
	nextPos=&_phase[next];
	nextTime=nextPos->Time();
	prevTime=prevPos->Time();
	// cycled through 0 - we use mod 1 arithmetics
	if (prev==next )
	{
		interpol = 1;
	}
	else
	{
		float nextDelta=nextTime-prevTime;if( looped && nextDelta<=0 ) nextDelta+=1;
		float timeDelta=time-prevTime;if( looped && timeDelta<0 ) timeDelta+=1;
		interpol=timeDelta/nextDelta;
	}
}


void Shape::SetPhase( float time, float baseTime )
{
	if( IsAnimated() )
	{
		const AnimationPhase *prevPos,*nextPos;
		float interpol;
		PreparePhase(prevPos,nextPos,interpol,time,baseTime);
		for( int i=0; i<NPos(); i++ )
		{
			SetPos(i)=( (*prevPos)[i]+((*nextPos)[i]-(*prevPos)[i])*interpol );
		}
		_faceNormalsValid = false;
	}
}

void Shape::SetPhase( const Selection &anim, float time, float baseTime )
{
	if( IsAnimated() )
	{
		const AnimationPhase *prevPos,*nextPos;
		float interpol;
		PreparePhase(prevPos,nextPos,interpol,time,baseTime);
		for( int ai=0; ai<anim.Size(); ai++ )
		{
			int i=anim[ai];
			SetPos(i)=( (*prevPos)[i]+((*nextPos)[i]-(*prevPos)[i])*interpol );
		}
		_faceNormalsValid = false;
	}
}

Vector3 Shape::PointPhase( int i, float time, float baseTime ) const
{
	if( IsAnimated() )
	{
		const AnimationPhase *prevPos,*nextPos;
		float interpol;
		PreparePhase(prevPos,nextPos,interpol,time,baseTime);
		return (*prevPos)[i]+((*nextPos)[i]-(*prevPos)[i])*interpol;
	}
	else
	{
		return Pos(i);
	}
}

void LODShape::CalculateHints()
{
	_andHints=~0;
	_orHints=0;
	for_each_alpha
	for( int level=0; level<NLevels(); level++ )
	{
		Shape *shape=Level(level);
		if( !shape ) continue;
		_andHints&=shape->GetAndHints();
		_orHints|=shape->GetOrHints();
	}
	#if ALPHA_SPLIT
	Shape *oShape=LevelOpaque(0);
	Shape *aShape=LevelAlpha(0);
	if( !aShape || oShape->NFaces()>=aShape->NFaces() ) aShape=oShape;
	#else
	Shape *aShape=LevelOpaque(0);
	#endif
	_color=aShape->_color,_colorTop=aShape->_colorTop;
	float alpha = _color.A8()*(1.0/255);
	//float transparency = 1-alpha*2;
	float transparency = 1-alpha*1.5;
	
	if (transparency>=0.99) _viewDensity = 0;
	if (transparency>0.01) _viewDensity = log(transparency)*6;
	else _viewDensity = -10;

	/*
	// empirical adjustement:
	// forest density is significantly lesser than other objects
	float vis10m = exp(_viewDensity*10);
	LogF
	(
		"Shape %s, transp %.3f, density %.5f, vis10m %.5f",
		(const char *)Name(),transparency,_viewDensity,vis10m
	);
	*/


}


void LODShape::DefineMinMax( int level )
{
	Shape *oShape=LevelOpaque(level);
	Shape *aShape=LevelOpaque(level);
	_minMax[0]=oShape->Min();
	_minMax[1]=oShape->Max();
	if( aShape )
	{
		SaturateMin(_minMax[0],aShape->Min());
		SaturateMin(_minMax[1],aShape->Max());
	}
	CalculateBoundingSphere();
}

/*!
\patch_internal 1.30 Date 11/2/2001 by Ondra.
- Fixed: When shape was enlarged and bounding center was not change,
bounding radius was not updated. This caused blue fog in distance.
\patch 1.30 Date 11/2/2001 by Ondra.
- Fixed: Blue/dark grey horizont with large viewing distance.
*/

void LODShape::CalculateBoundingSphereRadius()
{
	float maxSphere2=0;
	for( int level=0; level<_nLods; level++ )
	{
		Shape *shape=Level(level);
		if( !shape ) continue;
		for( int i=0; i<shape->_pos.Size(); i++ )
		{
			const V3 &pos=shape->_pos[i];
			float sphere2=pos.SquareSize();
			saturateMax(maxSphere2,sphere2);
		}
		for( int p=0; p<shape->_phase.Size(); p++ )
		{
			for( int i=0; i<shape->_phase[p].Size(); i++ )
			{
				const Vector3 &pos=shape->_phase[p][i];
				float sphere2=pos.SquareSize();
				saturateMax(maxSphere2,sphere2);
			}
		}
	}
	_boundingSphere=sqrt(maxSphere2);
}


void LODShape::CalculateBoundingSphere()
{
	// -_boundingCenter center is original zero positioned in new coordinate system
	// when you add _boundingCenter to object coordinates, you will get original position

	// calculate bounding sphere from min-max information
	Vector3 oldBoundingCenter=_boundingCenter;
	Vector3 changeBoundingCenter=VZero;
	if (!_lockAutoCenter)
	{
		if (_autoCenter && NLevels()>0)
		{
			// consider only clipflags of 
			changeBoundingCenter=(_minMax[0]+_minMax[1])*0.5;
			// for OnSurface shapes do not change y component
			Shape *level0 = Level(0);
			if ((level0->GetAndHints()&ClipLandOn) && (level0->Special()&OnSurface))
			{
				changeBoundingCenter[1] = 0;
			}
		}
		else
		{
			// we might need to reset bounding center to zero
			changeBoundingCenter = -_boundingCenter;
		}

		if (changeBoundingCenter.SquareSize()<1e-10 && _boundingSphere>0 )
		{
			// note: bounding sphere may be changed even when center is not
			CalculateBoundingSphereRadius();
			return; // no change
		}
	}
	_boundingCenter += changeBoundingCenter;
	_minMax[0] -= changeBoundingCenter;
	_minMax[1] -= changeBoundingCenter;
	_aimingCenter -= changeBoundingCenter;
	
	float maxSphere2=0;
	for_each_alpha
	for( int level=0; level<_nLods; level++ )
	{
		Shape *shape=Level(level);
		if( !shape ) continue;
		for( int i=0; i<shape->_pos.Size(); i++ )
		{
			V3 &pos=shape->_pos[i];
			pos-=changeBoundingCenter;
			float sphere2=pos.SquareSize();
			saturateMax(maxSphere2,sphere2);
		}
		for( int p=0; p<shape->_phase.Size(); p++ )
		{
			for( int i=0; i<shape->_phase[p].Size(); i++ )
			{
				Vector3 &pos=shape->_phase[p][i];
				pos-=changeBoundingCenter;
				float sphere2=pos.SquareSize();
				saturateMax(maxSphere2,sphere2);
			}
		}
		// change min-max offsets of all lods
		shape->_minMax[0] -= changeBoundingCenter;
		shape->_minMax[1] -= changeBoundingCenter;
		shape->_bCenter -= changeBoundingCenter;
		shape->_minMaxOrig[0] -= changeBoundingCenter;
		shape->_minMaxOrig[1] -= changeBoundingCenter;
		shape->_bCenterOrig -= changeBoundingCenter;
		// change proxy positions
		for( int i=0; i<shape->_proxy.Size(); i++ )
		{
			ProxyObject *proxy=shape->_proxy[i];
			Object *pobj = proxy->obj;
			pobj->SetPosition(pobj->Position()-changeBoundingCenter);
			// recalc. inverse transform
			proxy->invTransform=pobj->InverseScaled();
		}
	}
	_boundingSphere=sqrt(maxSphere2);
	// d coefs of planes are changed
	if( changeBoundingCenter.SquareSize()>0.01 )
	{
		for_each_alpha
		for( int level=0; level<_nLods; level++ )
		{
			Shape *shape=Level(level);
			if( shape ) shape->RecalculateNormals(false);
		}
	}
}

void LODShape::CalculateMinMax( bool recalcLevels )
{
	//Assert( _boundingCenter.SquareSize()<=0.0001 );
	// calculate only for first LOD-level
	// we assume this will be valid for the other too
		// scan through all vertices of all levels
	if( recalcLevels )
	for_each_alpha
	for( int level=0; level<NLevels(); level++ )
	{
		Shape *shape=Level(level);
		if( !shape ) continue;
		shape->CalculateMinMax();
		shape->StoreOriginalMinMax();
	}

	// calculate min-max of all levels
	_minMax[0]=Vector3(1e10,1e10,1e10); // min
	_minMax[1]=Vector3(-1e10,-1e10,-1e10); // max
	//_minMax[0]=Vector3(COORD_MAX,COORD_MAX,COORD_MAX); // min
	//_minMax[1]=Vector3(COORD_MIN,COORD_MIN,COORD_MIN); // max
	//if( recalcLevels )
	for_each_alpha
	for( int level=0; level<NLevels(); level++ )
	{
		Shape *shape=Level(level);
		if( !shape ) continue;
		SaturateMin(_minMax[0],shape->Min());
		SaturateMax(_minMax[1],shape->Max());
	}

	CalculateBoundingSphere();
}

void LODShape::CheckForcedProperties()
{
	const ParamEntry &notes = Pars>>"CfgModels";
	char shortName[256];
	GetFilename(shortName,GetName());
	while (strpbrk(shortName," -/()"))
	{
		*strpbrk(shortName," -/()")='_';
	}
	const ParamEntry *modelNotes = notes.FindEntry(shortName);
	if (modelNotes)
	{
		// check if some properties are defined
		const ParamEntry *props = modelNotes->FindEntry("properties");
		if (props && NLevels()>0)
		{
			// scan properties and add them into geometry or topmost level
			Shape *geom = GeometryLevel();
			if (!geom) geom = Level(0);
			for (int i=0; i<props->GetSize()-1; i+=2)
			{
				RStringB propName = (*props)[i];
				RStringB propVal = (*props)[i+1];
				geom->SetProperty(propName,propVal);
			}
			// some force properties may change meaning of some shapes
			ScanShapes();

		}
	}
}

void LODShape::ScanProperties()
{
	const char *armor=PropertyValue("armor");
	if( armor && *armor ) _armor=atof(armor);
	else _armor=200;
	if (_armor>1e-10)
	{
		_invArmor=1/_armor;
		_logArmor = log(_armor);
	}
	else
	{
		_invArmor = 1e10;
		_logArmor = 25;
	}
}

/*!
\patch 1.28 Date 10/22/2001 by Ondra.
- Fixed: Bug in angular inertia tensor calculation.
  This bug caused fuel truck turning very slow.
*/

void LODShape::CalculateMass()
{
	// mass should always be stored in geometry
	//Shape *shape=Level(0);
	Shape *shape=GeometryLevel();
	if( !shape ) return;
	double totalMass=0;
	int i;
	// calculate position of center of mass
	Vector3 sum(VZero);
	for( i=0; i<shape->_pointToVertex.Size(); i++ )
	{
		Vector3Val r=shape->Pos(shape->_pointToVertex[i]);
		float m=_massArray[i];
		totalMass+=m;
		sum+=r*m;
	}
	if( totalMass>0.0 ) _centerOfMass=sum*(1/totalMass);
	else _centerOfMass=VZero;

	Matrix3 totalInertia(M3Zero);
	for( i=0; i<shape->_pointToVertex.Size(); i++ )
	{
		//{{FIX inertia
		Vector3Val r=shape->Pos(shape->_pointToVertex[i])-_centerOfMass;
		//}}FIX inertia
		float m=_massArray[i];
		Matrix3Val rTilda=r.Tilda();
		totalInertia-=rTilda*rTilda*m;
	}
	_mass=totalMass;
	if (totalMass>0)
	{
		_invInertia=totalInertia.InverseGeneral();
		_invMass=1/totalMass;
	}
	else
	{
		_invInertia=M3Identity;
		_invMass=1;
	}
}

// LOD implementation

void LODShape::DoClear()
{ // forget everything but name
	_canOcclude = false; // do not use for occlusions unless told otherwise
	_canBeOccluded = false;
	_autoCenter = true;
	_allowAnimation = false;
	_lockAutoCenter = false;
	_special=0;
	_andHints=_orHints=0; // default: no hints
	//strcpy(_name,":temp:");
	_nLods=0;
	_remarks=0;
	_boundingCenter=VZero;
	_geometryCenter=VZero;
	_boundingSphere = 0;
	_minMax[0]=VZero;
	_minMax[1]=VZero;
	_geometry=_memory=_landContact=_roadway=_hitpoints=_paths=-1;
	_geometryFire=_geometryView=-1;

	_geometryViewPilot=-1;
	_geometryViewGunner=-1;
	_geometryViewCommander=-1;
	_geometryViewCargo=-1;

	_invInertia.SetIdentity();
	_centerOfMass=VZero;
	_mass=0;
	_invMass=1e10;
	_aimingCenter=VZero;
	_geomComponents=new ConvexComponents();
	_viewComponents=new ConvexComponents();
	_fireComponents=new ConvexComponents();
	_massArray.Clear();
	_viewDensity=-100;
	_color=PackedBlack;
	_colorTop=PackedBlack;
}

void LODShape::DoConstruct()
{
	_name="";
	DoClear();
}

void LODShape::DoConstruct( const LODShape &src, bool copyAnimations )
{
	// copy shapes, not only references
	for_each_alpha
	for( int i=0; i<src._nLods; i++ )
	{
		if( src._lods[i] )
		{
			_lods[i]=new Shape(*src._lods[i],copyAnimations);
		}
		else
		{
			_lods[i]=NULL;
		}
		_resolutions[i]=src._resolutions[i];
	}
	_nLods=src._nLods;

	_autoCenter = src._autoCenter;
	_lockAutoCenter = src._lockAutoCenter;
	_allowAnimation = src._allowAnimation;
	_minMax[0] = src._minMax[0],_minMax[1]=src._minMax[1];
	_boundingCenter = src._boundingCenter;
	_boundingSphere = src._boundingSphere;
	_special=src._special;
	_remarks=src._remarks;
	_andHints=src._andHints;
	_orHints=src._orHints;
	_geometry=src._geometry;
	_geometryFire=src._geometryFire;
	_geometryView=src._geometryView;

	_geometryViewPilot = src._geometryViewPilot;
	_geometryViewCommander = src._geometryViewCommander;
	_geometryViewGunner = src._geometryViewGunner;
	_geometryViewCargo = src._geometryViewCargo;

	_memory=src._memory;
	_landContact=src._landContact;
	_roadway=src._roadway;
	_paths=src._paths;
	_hitpoints=src._hitpoints;
	//strcpy(_name,src._name);
	_name=""; // copy name is empty
	_invInertia = src._invInertia;
	_mass=src._mass;
	_invMass=src._invMass;
	_centerOfMass=src._centerOfMass;
	_aimingCenter=src._aimingCenter;
	if( copyAnimations )
	{
		_geomComponents=new ConvexComponents(*src._geomComponents);
		_viewComponents=new ConvexComponents(*src._viewComponents);
		_fireComponents=new ConvexComponents(*src._fireComponents);
		if( src._massArray.Size()>0 && GeometryLevel() )
		{
			_massArray=src._massArray;
		}
		else _massArray.Clear();
	}
	else
	{
		_geomComponents=new ConvexComponents();
		_viewComponents=new ConvexComponents();
		_fireComponents=new ConvexComponents();
		_massArray.Clear();
	}
}

void LODShape::DoDestruct()
{
	#if VERBOSE
		if( _name[0] )
		{
			LogF("Destruct shape %s",(const char *)_name);
		}
	#endif
	int i;
	for( i=0; i<_nLods; i++ )
	{
		_lods[i].Free();
	}
	_nLods=0;
}



void LODShape::OptimizeShapes()
{
	// delete LODs that are too complex
	// this reduces memory usage
	int i=0;
	// scan normal LODs 
	#if !_RELEASE
	if (!strcmpi(Name(),"data3d\\les ctverec pruchozi_T1.p3d"))
	{
		__asm nop;
	}
	#endif
	//LogF("%s:",(const char *)_name);
	for( ; i<_nLods; i++ )
	{
		if( _resolutions[i]>=900 ) break;
		float relRes=_resolutions[i]/_boundingSphere;
		//LogF("  %d: Relative resolution: %.3f",i,relRes);
		if( relRes>Glob.config.objectLODLimit ) break; // this LOD is neccessary
		int needed = atoi(_lods[i]->PropertyValue("lodneeded"));
		if (needed>=2 ) break;
	}
	int n=i-1; // last LOD is always neccesary

	for( i=0; i<n; i++ )
	{
		// delete too complex LODs
		_lods[i]=NULL;
		LogF("Dropped %s: %d (%f)",(const char *)_name,i,_resolutions[i]);
	}

	// remove any NULL LODs
	int d=0;
	for( int s=0; s<_nLods; s++ )
	{
		if( _lods[s] )
		{

			// delete any vdecal lods (used for some trees)
			if
			(
				s!=0 && (_lods[s]->GetAndHints()&ClipDecalMask)!=ClipDecalNone
				&& _lods[s]->NPos()>0
			)
			{
				LogF("VDecal  %s: %d (%f)",(const char *)_name,s,_resolutions[s]);
			}
			else if (EnableHWTL && atoi(_lods[s]->PropertyValue("notl"))>0)
			{
				LogF("TL dropped %s: %d (%f)",(const char *)_name,s,_resolutions[s]);
			}
			else
			{
				_resolutions[d]=_resolutions[s];
				_lods[d]=_lods[s];
				_lods[d]->SetLevel(d);
				d++;
			}
		}
	}
	for (int s=d; s<_nLods; s++)
	{
		_lods[s] = NULL;
	}
	_nLods=d;

	// scan shadow LODs 
	for( i=0; i<_nLods; i++ )
	{
		if( _resolutions[i]>=900 ) break;
		float relRes=_resolutions[i]/_boundingSphere;
		if( relRes>Glob.config.shadowLODLimit ) break; // this LOD is neccessary


	}
	n=i-1; // last LOD is always neccesary

	// optimize shadow LODs
	// if some lod is disabled for shadowing
	// all lods before it should be disabled to
	for( i=0; i<n; i++ )
	{
		// disable shadows of too complex LODs
		if( _lods[i]->FindProperty("lodnoshadow")<0 )
		{
			_lods[i]->_prop.Add(NamedProperty("lodnoshadow","1"));
		}
	}
	ScanShapes();
}


// TODO: move shapebank out of landscape
#include "landscape.hpp"

DEFINE_FAST_ALLOCATOR(ProxyObject)

static RString ReplacesChars(RString shapeName, char c, char w)
{
	if (!strchr(shapeName,c)) return shapeName;
	shapeName.MakeMutable();
	char *str = shapeName.MutableData();
	while (*str)
	{
		if (*str==c) *str = w;
		str++;
	}
	return shapeName;
}

bool GReplaceProxies = true;

void LODShape::Load( QIStream &f, bool reversed )
{
	DoClear();

	#if VERBOSE
	LogF("Load %s, %s",(const char *)_name,reversed ? "Reversed" : "");
	#endif

	// check optimized load
	int seekStart = f.tellg();
	if (LoadOptimized(f))
	{
		// reverse all vector data if necessary
		if (reversed)
		{
			Reverse();
			_remarks|=REM_REVERSED;
		}
		if (!CheckLegalCreator())
		{
			#if _SUPER_RELEASE
			Clear();
			#else
			RptF("Bad file format (%s).",Name());
			#endif
		}
		return;
	}
	f.seekg(seekStart,QIOS::beg);
	// check magic

	_special=0;
	_remarks=0;
	

	if( reversed ) _remarks|=REM_REVERSED;

	// load all LODs and their setups respectivelly
	// check for LOD header 
	char magic[4];
	int nLods=1;
	bool tagged=false;
	int ver=0;

	_mass=0;
	_invMass=1e10;
	_invInertia.SetZero();
	_centerOfMass=Vector3(VZero);
	
	f.read(magic,sizeof(magic));
	if( f.fail() || f.eof() )
	{
		WarningMessage("Error loading %s (Magic)",(const char *)_name);
		goto Error;
	}
	if( !strncmp(magic,"NLOD",sizeof(magic)) )
	{
		// LOD header - we will load multiple LODs
		Log("Warning: NLOD format in object %s",(const char *)_name);
		tagged=true;
		f.read((char *)&nLods,sizeof(nLods));
		if( f.fail() || f.eof() ) {WarningMessage("Error loading %s (nLods)",(const char *)_name);goto Error;}
	}
	else if( !strncmp(magic,"MLOD",sizeof(magic)) )
	{
		// LOD header - we will load multiple LODs
		tagged=true;
		f.read((char *)&ver,sizeof(ver));
		f.read((char *)&nLods,sizeof(nLods));
		if( f.fail() || f.eof() ) {WarningMessage("Error loading %s (nLods)",(const char *)_name);goto Error;}
	}
	else
	{
		ErrorMessage("Warning: preNLOD format in object %s",(const char *)_name);
		f.seekg(-(int)sizeof(magic),QIOS::cur);
		Fail("Very old object loaded.");
	}
	int major,minor;
	major=ver>>8;
	minor=ver&0xff;
	#if 0 //_DEBUG
		if (strstr(_name,"znacka") && strstr(_name,"end"))
		{
			__asm nop;
		}
	#endif
	//if( major>1 ) // only format versions 0.xx and 1.xx supported
	if( major>1 ) // only format versions 1.xx supported
	{
		WarningMessage("%s: Unsupported version %d.%02d",(const char *)_name,major,minor);
		goto Error;
	}

	for( int i=0; i<nLods; i++ )
	{
		#if VERBOSE>1
		LogF("  Load level %d",i);
		#endif
		Ref<Shape> shape=new Shape();
		float resolution = 0;

		int startShapeInStream = f.tellg();

		bool wasMassArray = _massArray.Size()>0;
		resolution = shape->LoadTagged
		(
			f,reversed,ver,false,_massArray,tagged
		);

		bool geometryOnly = ResolGeometryOnly(resolution);
		if (geometryOnly && shape->NFaces()>0)
		{
			// to avoid unnecessary vertex sharing,
			// reload geometry with no normals
			int endShapeInStream = f.tellg();
			f.seekg(startShapeInStream,QIOS::beg);

			// revert state - massArray might be set during LoadTagged
			if (!wasMassArray) _massArray.Clear();

			shape = new Shape();
			shape->LoadTagged(f,reversed,ver,false,_massArray,tagged);
			// revert to new position
			f.seekg(endShapeInStream,QIOS::beg);
		}

		if (shape->_loadWarning)
		{
			LogF("Warnings in %s:%f",(const char *)_name,resolution);
		}
		AddShape(shape,resolution);

		// scan selections for proxy objects

		// scan for proxies
		for( int i=0; i<shape->_sel.Size(); i++ )
		{
			static const char proxyName[]="proxy:";
			static int proxyNameLen=strlen(proxyName);

			const NamedSelection &sel=shape->_sel[i];
			const char *selName=sel.Name();

			if( strncmp(selName,proxyName,proxyNameLen) ) continue;
			selName+=proxyNameLen;

			if( sel.Size()!=3 )
			{
				RptF
				(
					"%s:%s: Bad proxy object definition %s",
					(const char *)_name,LevelName(this,shape),sel.Name()
				);
				continue;
			}

			// check if proxy selection is hidden
			if (sel.Faces().Size()!=1)
			{
				RptF("%s: Proxy object should be single face %s",(const char *)_name,sel.Name());
				if (sel.Faces().Size()<1) continue;
			}

			Poly &face = shape->FaceIndexed(sel.Faces()[0]);
			face.OrSpecial(IsHiddenProxy|NoTexMerger);
			face.SetTexture(NULL);
			if ((face.Special()&(IsHidden|IsHiddenProxy))==0)
			{
				LogF
				(
					"%s:%s: Proxy face should be hidden %s",
					(const char *)_name,LevelName(this,shape),sel.Name()
				);
			}

			char shapeName[256];
			strcpy(shapeName,selName);
			char *ext=strchr(shapeName,'.');
			int id = -1;
			if (ext)
			{
				*(ext++)=0;
				id = atoi(ext);
			}
			// it is proxy: define it


			Ref<ProxyObject> obj = new ProxyObject;
			// create object from shape name
			// shape name may be name of some vehicle class
			//  replace any spaces with '_';
			if (GReplaceProxies)
			{
				extern Object *NewObject( RString typeName, RString shapeName );
				RString proxyType = RString("Proxy")+ReplacesChars(shapeName,' ','_');
				RString proxyShape = GetShapeName(shapeName);
				Ref<Object> pobj = NewObject(proxyType,proxyShape);
				if (!pobj)
				{
					RptF("Cannot create proxy object %s",shapeName);
					continue;
				}
				obj->obj = pobj;
			}
			else
			{
				extern Object *NewProxyObject(RString shapeName);
				obj->obj = NewProxyObject(shapeName);
			}
			obj->name = shapeName;
			obj->id = id;
			// get proxy object coordinates
			// scan selection
			int pi0=sel[0],pi1=sel[1],pi2=sel[2];

			const V3 *p0=&shape->Pos(pi0);
			const V3 *p1=&shape->Pos(pi1);
			const V3 *p2=&shape->Pos(pi2);

			float dist01=p0->Distance2(*p1);
			float dist02=p0->Distance2(*p2);
			float dist12=p1->Distance2(*p2);

			// p0,p1 should be the shortest distance
			if( dist01>dist02 ) swap(p1,p2),swap(dist01,dist02); // swap points 1,2
			if( dist01>dist12 ) swap(p0,p2),swap(dist01,dist12); // swap points 0,2
			// p0,p2 should be the second shortest distance
			if( dist02>dist12 ) swap(p0,p1),swap(dist02,dist12);
			//  verify results

			Matrix4 trans;
			trans.SetPosition(*p0);
			trans.SetDirectionAndUp((*p1-*p0),(*p2-*p0));
			LODShapeWithShadow *pshape = obj->obj->GetShape();
			if (pshape)
			{
				trans.SetPosition(trans.FastTransform(pshape->BoundingCenter()));
			}
			obj->obj->SetTransform(trans);
			//obj->transform=trans;
			obj->invTransform=trans.InverseScaled();
			obj->selection = i;
			obj->obj->SetDestructType(DestructNo);
			shape->_proxy.Add(obj);
		}

		if (shape->_sel.Size()<=0)
		{
			// no selections: we may safely optimize
			shape->Optimize();
		}


		// scan for sections
		if (Pars.FindEntry("CfgModels"))
		{
			const ParamEntry &notes = Pars>>"CfgModels";
			// note: shape name may contain spaces
			char shortName[256];
			GetFilename(shortName,GetName());
			while (strpbrk(shortName," -/()"))
			{
				*strpbrk(shortName," -/()")='_';
			}
			if (notes.FindEntry(shortName))
			{
				shape->DefineSections(notes>>shortName);
			}
			else
			{
				shape->DefineSections(notes>>"default");
			}
			if (!geometryOnly) shape->FindSections();
		}

	}
	Assert( _nLods>=1 );

	// map type
	{
		const char *mapType = PropertyValue("map");
		if (!*mapType)
		{
			// TODO: Fail
			//LogF("%s: No map type %s",(const char *)(const char *)_name,mapType);
			_mapType = MapHide;
		}
		else if (stricmp(mapType, "TREE") == 0)
		{
			_mapType = MapTree;
		}
		else if (stricmp(mapType, "SMALL TREE") == 0)
		{
			_mapType = MapSmallTree;
		}
		else if (stricmp(mapType, "BUSH") == 0)
		{
			_mapType = MapBush;
		}
		else if (stricmp(mapType, "BUILDING") == 0)
		{
			_mapType = MapBuilding;
		}
		else if (stricmp(mapType, "HOUSE") == 0)
		{
			_mapType = MapHouse;
		}
		else if (stricmp(mapType, "FOREST BORDER") == 0)
		{
			_mapType = MapForestBorder;
		}
		else if (stricmp(mapType, "FOREST TRIANGLE") == 0)
		{
			_mapType = MapForestTriangle;
		}
		else if (stricmp(mapType, "FOREST SQUARE") == 0)
		{
			_mapType = MapForestSquare;
		}
		else if (stricmp(mapType, "CHURCH") == 0)
		{
			_mapType = MapChurch;
		}
		else if (stricmp(mapType, "CHAPEL") == 0)
		{
			_mapType = MapChapel;
		}
		else if (stricmp(mapType, "CROSS") == 0)
		{
			_mapType = MapCross;
		}
		else if (stricmp(mapType, "ROCK") == 0)
		{
			_mapType = MapRock;
		}
		else if (stricmp(mapType, "BUNKER") == 0)
		{
			_mapType = MapBunker;
		}
		else if (stricmp(mapType, "FORTRESS") == 0)
		{
			_mapType = MapFortress;
		}
		else if (stricmp(mapType, "FOUNTAIN") == 0)
		{
			_mapType = MapFountain;
		}
		else if (stricmp(mapType, "VIEW-TOWER") == 0)
		{
			_mapType = MapViewTower;
		}
		else if (stricmp(mapType, "LIGHTHOUSE") == 0)
		{
			_mapType = MapLighthouse;
		}
		else if (stricmp(mapType, "QUAY") == 0)
		{
			_mapType = MapQuay;
		}
		else if (stricmp(mapType, "FUELSTATION") == 0)
		{
			_mapType = MapFuelstation;
		}
		else if (stricmp(mapType, "HOSPITAL") == 0)
		{
			_mapType = MapHospital;
		}
		else if (stricmp(mapType, "FENCE") == 0)
		{
			_mapType = MapFence;
		}
		else if (stricmp(mapType, "WALL") == 0)
		{
			_mapType = MapWall;
		}
		else if (stricmp(mapType, "HIDE") == 0)
		{
			_mapType = MapHide;
		}
		else if (stricmp(mapType, "BUSSTOP") == 0)
		{
			_mapType = MapBusStop;
		}
		else
		{
			RptF("%s: Unknown map type %s",(const char *)_name,mapType);
			//Fail("Unknown map type");
			_mapType = MapHide;
		}
	}
	// move (0,0,0) to the geometry center
	{
		const char *autoCenter=PropertyValue("autocenter");
		if( *autoCenter && atoi(autoCenter)==0 ) _autoCenter=false;
		CalculateMinMax();
		CalculateHints();
		// calculate mass, center of mass and inertia
		if( _massArray.Size()>0 ) CalculateMass();
	}
	// autodetect if animation should be allowed
	// TODO: may be moved to Shape
	if (_orHints&(ClipLandMask|ClipDecalMask))
	{
		_allowAnimation = true;
	}
	if ((_orHints&ClipLightMask)==(_andHints&ClipLightMask))
	{
		ClipFlags light = _orHints&ClipLightMask;
		switch (light)
		{
			case ClipLightSky:
			case ClipLightCloud:
			case ClipLightStars:
			case ClipLightLine:
				_allowAnimation = true;
			break;
				
		}		
	}

	{
		const char *animated=PropertyValue("animated");
		if( *animated ) _allowAnimation=atoi(animated)!=0;
	}

	for( int i=0; i<_nLods; i++ )
	{
		if( _resolutions[i]<900 ) saturateMin(_resolutions[i],BoundingSphere()*2);
	}
	// remove too complex LODs
	OptimizeShapes();
	// scan only normal LODs
	//bool someShadow=false;

	for( int i=0; i<_nLods; i++ ) if( _resolutions[i]<900 && _lods[i] )
	{
		Shape *oShape=_lods[i];
		if( oShape ) _special|=oShape->Special();
	}
	CalculateMinMax();
	if( _massArray.Size()>0 ) CalculateMass();

	CheckForcedProperties();
	InitConvexComponents();
	ScanProperties();
	CalculateHints();

	#if 0 //_DEBUG
	if (_paths>=0)
	{
		// optimize paths lod
		LogF("ExtractPath from %s",Name());
		_lods[_paths] = _lods[_paths]->ExtractPath();
		_lods[_paths]->SetLevel(_paths);
	}
	#endif
	_propertyClass = PropertyValue("class");
	_propertyDammage = PropertyValue("dammage");

	{
		int complexity = LevelOpaque(0)->NFaces();
		float size = BoundingSphere();
		Shape *view = ViewGeometryLevel();
		int viewComplexity = view ? view->NFaces() : 0;
		// check size and complexity
		// large or simple objects may occlude
		if( viewComplexity<=0 ) _canOcclude=false; // view geometry empty
		else if( size>5 ) _canOcclude = true;
		else if( size>2 && viewComplexity<=6 ) _canOcclude = true;
		// large or complex objects may be occluded
		if( complexity>=6 ) _canBeOccluded = true;
		if( size>5 ) _canBeOccluded = true;
		// allow override with property
		const char *expCanOcclude=PropertyValue("canocclude");
		const char *expCanBeOccluded=PropertyValue("canbeoccluded");
		if( *expCanOcclude ) _canOcclude = atoi(expCanOcclude)!=0;
		if( *expCanBeOccluded ) _canBeOccluded = atoi(expCanOcclude)!=0;
	}

	if (GUseFileBanks && !CheckLegalCreator())
	{
		#if _SUPER_RELEASE
			goto Error;
		#else
			RptF("Bad file format (%s).",Name());
		#endif
	}

	return;
	Error:
	// create empty shape
	_lods[0]=new Shape();
	_lods[0]->SetLevel(0);
	_resolutions[0]=0.0f;
	_nLods=1;
	return;
}

void LODShape::PrepareProperties(const ParamEntry &cfg)
{
	// read names of selections that must not split across sections
}

void LODShape::Reload( QIStream &f, bool reversed )
{
	Log("Reload shape %s",(const char *)_name);

	Load(f,reversed);
}

void LODShape::Load( const char *name, bool reversed )
{
	// convert Data.. structures into Poly
	// load external file, convert ...
	// if applied outside constructor, Unload should be performed first
	// on error return empty object
	char nameTemp[1024];
	if( strlen(name)>sizeof(nameTemp) )
	{
		WarningMessage("Name '%s' too long.");
	}
	strncpy(nameTemp,name,sizeof(nameTemp)); // save name
	nameTemp[sizeof(nameTemp)-1]=0;
	strlwr(nameTemp);
	_name = nameTemp;
	//Log("Load shape %s",_name);

  QIFStream f;
	if (GFileServer) GFileServer->Open(f,_name);
	else f.open(_name);
	if( f.fail() )
	{
		WarningMessage("Cannot open object %s",(const char *)_name);
		return;
	}
	Load(f,reversed);
}

ConvexComponents *LODShape::GetConvexComponents(int level) const
{
	if (level==FindGeometryLevel())
	{
		return _geomComponents;
	}
	else if (level==FindFireGeometryLevel())
	{
		return _fireComponents;
	}
	else if (level==FindViewGeometryLevel())
	{
		return _viewComponents;
	}
	else if (level==FindViewPilotGeometryLevel())
	{
		return _viewPilotComponents;
	}
	else if (level==FindViewGunnerGeometryLevel())
	{
		return _viewGunnerComponents;
	}
	else if (level==FindViewCommanderGeometryLevel())
	{
		return _viewCommanderComponents;
	}
	else if (level==FindViewCargoGeometryLevel())
	{
		return _viewCargoComponents;
	}
	else
	{
		return NULL;
	}
}

void LODShape::FindHitComponents(FindArray<int> &hits, const char *name) const
{
	// scan firegeometry components
	hits.Resize(0);
	Shape *geom = FireGeometryLevel();
	if (!geom) return;
	int sel = geom->FindNamedSel(name);
	if (sel<0) return;
	// check if it is in some Component%02d selection
	const NamedSelection &namedSel = geom->NamedSel(sel);

	for( int i=1; i<1000; i++ )
	{
		char name[64];
		sprintf(name,"Component%02d",i);
		int selIndex=geom->FindNamedSel(name);
		if( selIndex<0 ) break;
		const NamedSelection &sel=geom->NamedSel(selIndex);
		if (sel.IsSubset(namedSel) || namedSel.IsSubset(sel))
		{
			// selection in component or component in selection
			hits.AddUnique(i-1);
		}
	}
}

void LODShape::InvalidateConvexComponents(int level)
{
	ConvexComponents *cc = GetConvexComponents(level);
	if (cc)
	{
		cc->Invalidate();
	}
}

void LODShape::RecalculateConvexComponentsAsNeeded(int level)
{
	ConvexComponents *cc = GetConvexComponents(level);
	if (cc)
	{
		cc->RecalculateAsNeeded(Level(level));
	}
}

void LODShape::InitConvexComponents( ConvexComponents &cc, Shape *geom )
{
	geom->RecalculateNormalsAsNeeded();
	for( int i=1; i<1000; i++ )
	{
		char name[64];
		sprintf(name,"Component%02d",i);
		int selIndex=geom->FindNamedSel(name);
		if( selIndex<0 ) break;
		const NamedSelection &sel=geom->NamedSel(selIndex);
		if( sel.Faces().Size()<4 )
		{
			if( sel.Faces().Size()>0 )
			{
				RptF
				(
					"Strange convex component %s in %s:%s",
					(const char *)_name,(const char *)name,LevelName(this,geom)
				);
			}
			continue;
		}
		ConvexComponent *component=new ConvexComponent;
		cc.Add(component);
		component->Init(geom,name);
	}
	cc.Validate();
}


void LODShape::InitCC( Ref<ConvexComponents> &cc, Shape *shape )
{
	if (shape)
	{
		cc = new ConvexComponents();
		InitConvexComponents(*cc,shape);
		if( cc->RecalculateEdges(shape) )
		{
			RptF
			(
				"Shape %s:%s - bad components",
				(const char *)Name(),LevelName(this,shape)
			);
		}
	}
}

void LODShape::InitConvexComponents()
{
	Shape *geom=GeometryLevel();
	Shape *fire=FireGeometryLevel();
	Shape *view=ViewGeometryLevel();
	if( geom )
	{
		InitConvexComponents(*_geomComponents,geom);
	}
	if( fire )
	{
		if( fire==geom ) _fireComponents=_geomComponents;
		else InitConvexComponents(*_fireComponents,fire);
	}
	if( view )
	{
		if( view==geom ) _viewComponents=_geomComponents;
		else if( view==fire ) _viewComponents=_fireComponents;
		else InitConvexComponents(*_viewComponents,view);

		const char *expCanOcclude=PropertyValue("canocclude");
		if( *expCanOcclude && atoi(expCanOcclude)==0 )
		{
			// occlusion disabled - no edges 
		}
		else
		{
			if( _viewComponents->RecalculateEdges(view) )
			{
				RptF
				(
					"Shape %s:%s - bad components",
					(const char *)Name(),LevelName(this,view)
				);
			}
		}
	}

	if( geom )
	{
		_geometryCenter=(geom->Max()+geom->Min())*0.5;
		_geometrySphere=geom->Max().Distance(geom->Min())*0.5;
		_aimingCenter=_geometryCenter;
	}
	else
	{
		_geometrySphere=_boundingSphere;
		_geometryCenter=_boundingCenter;
		_aimingCenter=_boundingCenter;
	}
	// override aiming point if necessary
	const char *aimName="zamerny";
	if( MemoryPointExists(aimName) )
	{
		_aimingCenter=MemoryPoint(aimName);
	}

	InitCC(_viewPilotComponents,ViewPilotGeometryLevel());
	InitCC(_viewGunnerComponents,ViewGunnerGeometryLevel());
	InitCC(_viewCommanderComponents,ViewCommanderGeometryLevel());
	InitCC(_viewCargoComponents,ViewCargoGeometryLevel());
}

const RStringB &LODShape::PropertyValue( const char *name ) const
{
	Shape *level=GeometryLevel();
	if( level )
	{
		const RStringB &value=level->PropertyValue(name);
		if (value.GetLength()>0) return value;
	}
	level=_lods[0];
	if( !level )
	{
		Fail("No shape");
		return RStringBEmpty;
	}
	return level->PropertyValue(name);
}

int LODShape::FindLevel( float resolution, bool noDecal ) const
{
	// find first suitable LOD level
	Assert( _nLods>=1 );
	if( resolution<900 )
	{
		// find normal LOD
		int i=1;
		for( ; i<_nLods; i++ )
		{
			float aRes=_resolutions[i];
			if( aRes>resolution ) break; // this one is too rough
			if( aRes>=900 ) break; // memory or special LOD
		}
		i--;
		if( noDecal )
		{
			while
			(
				i>0 && _lods[i-1] && (_lods[i]->GetAndHints()&ClipDecalMask)!=ClipDecalNone
			)
			{
				i--;
			}
		}
		return i;
	}
	else
	{
		// find special LOD
		float minDiff=1e20;
		int minI=0;
		for( int i=0; i<_nLods; i++ )
		{
			float diff=fabs(_resolutions[i]-resolution);
			if( minDiff>diff ) minDiff=diff,minI=i;
		}
		return minI;
	}
}

bool LODShape::IsSpecLevel( int level, float spec ) const
{
	return
	(
		level>=0 &&
		_resolutions[level]>spec*0.99 && _resolutions[level]<spec*1.01
	);
}

int LODShape::FindSpecLevel( float spec ) const
{
	int level=FindLevel(spec);
	if( level>=0 && _resolutions[level]>spec*0.99 && _resolutions[level]<spec*1.01 )
	{
		return level;
	}
	return -1;
}

int LODShape::FindSqrtLevel( float resolution2, bool noDecal ) const
{
	// find first suitable LOD level
	Assert( _nLods>=1 );
	// only normal LOD searched
	int i=1;
	for( ; i<_nLods; i++ )
	{
		float aRes=_resolutions[i];
		if( aRes*aRes>resolution2 ) break; // this one is too rough
		if( aRes>=900 ) break; // memory LOD
	}
	i--;
	if( noDecal )
	{
		while
		(
			i>0 && _lods[i-1] && (_lods[i]->GetAndHints()&ClipDecalMask)!=ClipDecalNone
		) i--;
	}
	return i;
}

int LODShape::FindNearestWithoutProperty( int i, const char *property ) const
{
	if( i==LOD_INVISIBLE ) return i;
	Assert( i<_nLods );
	Assert( i>=0 );
	while( i>0 && _lods[i] && _lods[i]->FindProperty(property)>=0 ) i--;
	while( i<_nLods && _lods[i] && _resolutions[i]<900 && _lods[i]->FindProperty(property)>=0 ) i++;
	if( i>=_nLods )
	{
		return -1;
	}
	return i;
}

int Shape::PointIndex( const char *name ) const
{
	int index=FindNamedSel(name);
	if( index<0 ) return -1;
	const Selection &sel=NamedSel(index);
	// selection should be one point only
	if( sel.Size()<1 )
	{
		ErrF("No point in selection %s.",name);
		return -1;
	}
	return sel[0];
}

const V3 &Shape::NamedPosition( const char *name, const char *altName ) const
{
	int pIndex=PointIndex(name);
	if( pIndex>=0 ) return Pos(pIndex);
	if( altName )
	{
		int pIndex=PointIndex(altName);
		if( pIndex>=0 ) return Pos(pIndex);
	}
	return V3Zero;
}

DEFINE_FAST_ALLOCATOR(LODShapeWithShadow)
LODShapeWithShadow::LODShapeWithShadow()
//:_isVehicle(false)
{
}

LODShapeWithShadow::LODShapeWithShadow( const char *name, bool reversed )
:LODShape(name,reversed)
{
}

LODShapeWithShadow::LODShapeWithShadow( QIStream &f, bool reversed )
:LODShape(f,reversed)
{
}


LODShape::LODShape(){DoConstruct();}
LODShape::LODShape( const LODShape &src, bool copyAnimations )
{
	DoConstruct(src,copyAnimations);
	ScanProperties();
}
void LODShape::operator = ( const LODShape &src )
{
	DoDestruct();
	DoConstruct(src,true);
}
LODShape::~LODShape()
{
	DoDestruct();
}

LODShape::LODShape( const char *name, bool reversed )
{
	DoConstruct();
	Load(name,reversed);
}
LODShape::LODShape( QIStream &f, bool reversed )
{
	DoConstruct();Load(f,reversed);
}

const V3 &LODShape::NamedPoint( int level, const char *name, const char *altName ) const
{
	Shape *shape=Level(level);
	int pIndex=shape->PointIndex(name);
	if( pIndex>=0 ) return shape->Pos(pIndex);
	if( altName )
	{
		int pIndex=shape->PointIndex(altName);
		if( pIndex>=0 ) return shape->Pos(pIndex);
	}
	return V3Zero;
}

const V3 &LODShape::MemoryPoint( const char *name, const char *altName ) const
{
	Shape *memory=MemoryLevel();
	if( !memory ) return V3Zero;
	return memory->NamedPosition(name,altName);
}

bool LODShape::MemoryPointExists( const char *name ) const
{
	Shape *memory=MemoryLevel();
	if( !memory ) return false;
	int pIndex=memory->PointIndex(name);
	if( pIndex<0 ) return false;
	return true;
}

bool LODShape::IsInside( Vector3Par pos ) const
{
	// make sure there is well defined geometry LOD
	if (_geomComponents->Size()<=0) return false;

	GeometryLevel()->RecalculateNormalsAsNeeded();
	RecalculateGeomComponentsAsNeeded();
	// all calculation will be performed in model space
	for( int iThis=0; iThis<_geomComponents->Size(); iThis++ )
	{
		const ConvexComponent &cThis=*(*_geomComponents)[iThis];
		// check intersection will all convex components
		if( cThis.IsInside(pos) )
		{
			return true;
		}
	}
	return false;
}

void LODShape::OrSpecial( int special )
{
	_special|=special;
	for_each_alpha
	for( int i=0; i<_nLods; i++ ) if( _lods[i] ) _lods[i]->OrSpecial(special);
}
void LODShape::AndSpecial( int special )
{
	_special&=special;
	for_each_alpha
	for( int i=0; i<_nLods; i++ ) if( _lods[i] ) _lods[i]->AndSpecial(special);
}
void LODShape::SetSpecial( int special )
{
	_special=special;
	for_each_alpha
	for( int i=0; i<_nLods; i++ ) if( _lods[i] ) _lods[i]->SetSpecial(special);
}
void LODShape::RescanSpecial()
{
	_special=0;
	for_each_alpha
	for( int i=0; i<_nLods; i++ )
	{
		if( _resolutions[i]>900 ) continue;
		if( _lods[i] ) _special|=_lods[i]->Special();
	}
}

void LODShape::ScanShapes()
{
	_geometry=-1;
	_geometryFire=-1;
	_geometryView=-1;

	_geometryViewPilot = -1;
	_geometryViewGunner = -1;
	_geometryViewCommander = -1;
	_geometryViewCargo = -1;

	_memory=-1;
	_landContact=-1;
	_roadway=-1;
	_hitpoints=-1;
	_paths=-1;
	for( int i=0; i<_nLods; i++ )
	{
		float resolution=_resolutions[i];
		if( resolution>900 )
		{
			if( IsSpec(resolution,GEOMETRY_SPEC) ) _geometry=i;
			else if( IsSpec(resolution,MEMORY_SPEC) ) _memory=i;
			else if( IsSpec(resolution,LANDCONTACT_SPEC) ) _landContact=i;
			else if( IsSpec(resolution,ROADWAY_SPEC) ) _roadway=i;
			else if( IsSpec(resolution,PATHS_SPEC) ) _paths=i;
			else if( IsSpec(resolution,HITPOINTS_SPEC) ) _hitpoints=i;
			else if( IsSpec(resolution,VIEW_GEOM_SPEC) ) _geometryView=i;
			else if( IsSpec(resolution,FIRE_GEOM_SPEC) ) _geometryFire=i;
			else if( IsSpec(resolution,VIEW_PILOT_GEOM_SPEC) ) _geometryViewPilot=i;
			else if( IsSpec(resolution,VIEW_GUNNER_GEOM_SPEC) ) _geometryViewGunner=i;
			else if( IsSpec(resolution,VIEW_COMMANDER_GEOM_SPEC) ) _geometryViewCommander=i;
			else if( IsSpec(resolution,VIEW_CARGO_GEOM_SPEC) ) _geometryViewCargo=i;
			else if( resolution<2000 ) // cockpit LOD
			{
			}
			else if( resolution<10000 ) // spec cockpit LOD
			{
			}
			else
			{
				LogF("%s: Uknown spec lod (%g)",(const char *)Name(),resolution);
			}
		}
	}
	if (_geometry>=0)
	{
		// geometry is made alpha transparent
		_lods[_geometry]->OrSpecial(IsAlpha|IsAlphaFog|IsColored);
	}
	if (_geometryView>=0)
	{
		// geometry is made alpha transparent
		_lods[_geometryView]->OrSpecial(IsAlpha|IsAlphaFog|IsColored);
	}
	if (_geometryFire>=0)
	{
		// geometry is made alpha transparent
		_lods[_geometryFire]->OrSpecial(IsAlpha|IsAlphaFog|IsColored);
	}
	if( _geometryView<0 ) _geometryView=_geometry;
	if( _geometryFire<0 ) _geometryFire=_geometryView;
	if (_geometry>=0)
	{
		const char *fireGeom = _lods[_geometry]->PropertyValue("firegeometry");
		if (atoi(fireGeom)>0)
		{
			_geometryFire = _geometry;
		}
		const char *viewGeom = _lods[_geometry]->PropertyValue("viewgeometry");
		if (atoi(viewGeom)>0)
		{
			_geometryView = _geometry;
		}
	}
}

void LODShape::AddShape( Shape *shape, float resolution )
{
	// added to the end of the LOD list
	Assert( _nLods<MAX_LOD_LEVELS );
	_lods[_nLods]=shape;
	_resolutions[_nLods]=resolution;
	shape->SetLevel(_nLods);
	_nLods++;
	ScanShapes();
}

void LODShape::ChangeShape( int level, Shape *shape )
{
	Assert( level<_nLods );
	_lods[level]=shape;
	shape->SetLevel(_nLods);
	//ScanShapes();
}

