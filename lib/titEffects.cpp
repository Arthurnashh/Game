// implementation of title effects

#include "wpch.hpp"
#include "world.hpp"
#include "titEffects.hpp"
#include <El/ParamFile/paramFile.hpp>
#include "engine.hpp"
#include "camera.hpp"
#include "landscape.hpp"
#include "UI/uiControls.hpp"
#include "txtPreload.hpp"
#include "fileLocator.hpp"
#include "drawText.hpp"
#include "stringtableExt.hpp"

//#include "interpol.hpp"

#include <El/Enum/enumNames.hpp>

#include "mbcs.hpp"

#include "UI/uiViewport.hpp"

static const EnumName TitEffectNameNames[]=
{
	EnumName(TitPlain, "PLAIN", &IDS_TITEFFECT_PLAIN),
	EnumName(TitPlainDown, "PLAIN DOWN", &IDS_TITEFFECT_PLAIN_DOWN),
	EnumName(TitBlack, "BLACK", &IDS_TITEFFECT_BLACK),
	EnumName(TitBlackFaded, "BLACK FADED", &IDS_TITEFFECT_BLACKFADED),
	EnumName(TitBlackOut, "BLACK OUT", &IDS_TITEFFECT_BLACKOUT),
	EnumName(TitBlackIn, "BLACK IN", &IDS_TITEFFECT_BLACKIN),
	EnumName(TitWhiteOut, "WHITE OUT", &IDS_TITEFFECT_WHITEOUT),
	EnumName(TitWhiteIn, "WHITE IN", &IDS_TITEFFECT_WHITEIN),
  EnumName(TitPlainNoFade, "PLAIN NOFADE", &IDS_TITEFFECT_PLAIN_NOFADE),
	EnumName()
};
template<>
const EnumName *GetEnumNames(TitEffectName dummy)
{
	return TitEffectNameNames;
}

class TitleEffectTimed: public TitleEffect
{
	typedef TitleEffect base;

	protected:
	float _speed; // all times are scaled relative to this speed

	float _timeToLive;

	float _fadeInTime; // before time to live
	float _fadeInTotal;
	float _fadeOutTime; // after time to live
	float _fadeOutTotal;

	float _alpha;

	public:
	TitleEffectTimed();

	void SetTimes( float fadeIn, float live, float fadeOut );
	void Simulate( float deltaT );
	bool IsTerminated() const;
	void SetSpeed( float speed ) {_speed=speed;}

	void Terminate(float fadeOutTime = -1.0f);
	void Prolong( float time );
};

void TitleEffectTimed::Terminate(float fadeOutTime)
{
	_timeToLive=0;
  if (fadeOutTime >= 0) _fadeOutTime = _fadeOutTotal = fadeOutTime;
}

void TitleEffectTimed::Prolong( float time )
{
	if( _timeToLive>time ) return; // no need to prolong
	if( _fadeOutTime>=_fadeOutTotal )
	{ // fade out not started yet
		_timeToLive=time;
	}
	else if( _fadeOutTime<time )
	{
		// keep alpha unchanged
		_fadeOutTime=time;
		saturateMax(_alpha,0.5f);
		_fadeOutTotal=_fadeOutTime/_alpha;
	}
}

TitleEffectTimed::TitleEffectTimed()
{
	_speed = 1;

	_timeToLive=0;
	_fadeInTime=0;
	_fadeOutTime=0;
	_alpha=0;
}

void TitleEffectTimed::SetTimes( float fadeIn, float live, float fadeOut )
{
	_fadeInTime=_fadeInTotal=fadeIn;
	_fadeOutTime=_fadeOutTotal=fadeOut;
	_timeToLive=live;
	_alpha=0;
}

bool TitleEffectTimed::IsTerminated() const
{
	if( _fadeInTime>0 ) return false;
	if( _timeToLive>0 ) return false;
	if( _fadeOutTime>0 ) return false;
	return true;
}

void TitleEffectTimed::Simulate( float deltaT )
{
	Assert( deltaT>=0 );

	if( _fadeInTime>0 )
	{
		_fadeInTime-=deltaT / _speed;
		_alpha=1-_fadeInTime/_fadeInTotal;
	}
	else if( _timeToLive>0 )
	{
    if (_speed > 0)
      _timeToLive -= deltaT / _speed;
    else
      _timeToLive = 0;
		_alpha = 1;
	}
	else
	{
		_fadeOutTime-=deltaT / _speed;
		_alpha=_fadeOutTime/_fadeOutTotal;
	}
	saturate(_alpha,0,1);
}

class TitleEffectBasic: public TitleEffectTimed
{
	typedef TitleEffectTimed base;

	Ref<INode> _text;
	float _textSize;
	float _textYOffset;

	Ref<Object> _object;
	Vector3 _objectCamera;

	Ref<Display> _rsc;
	float _yOffset;
  bool _drawInMap;


	void DrawText();
	void DrawObject();
	void DrawRsc();

  /// preload object data (lod 0)
  bool PreloadObject();
  /// preload resource data (textures and objects)
  bool PreloadRsc();
  /// preload text data (fonts and pictures)
  bool PreloadText();
  
public:
  TitleEffectBasic() : base(), _drawInMap(true) {};

  // type specific init
	virtual void Init( RString text, bool structured );
	virtual void Init( ParamEntryVal rsc );
	virtual void Init( LODShape *shape, Vector3Val camera );

	// common init
	virtual void Init( float time );

	void SetTextPos( float yOffset );
	void SetTextFont(RString font, float size);
	void SetFade( float in, float out );
	void Draw();
	bool Preload();

  void SetDrawInMap( bool drawInMap);
  virtual bool DrawInMap() {return _drawInMap;}

};

//! Title effect resource (display)
class DisplayTitle: public Display
{
	typedef Display base;

	public:
	//! constructor
	DisplayTitle();
};

DisplayTitle::DisplayTitle()
: Display(NULL)
{
	SetCursor(NULL);
}


// TitleEffectBasic handles basic title drawing

void TitleEffectBasic::SetTextPos(float yOffset )
{
	_textYOffset=yOffset;
}
void TitleEffectBasic::SetTextFont(RString font, float size)
{
	if (_text) 	SetTextAttribute(_text, "font", font);
	_textSize = size;
}
void TitleEffectBasic::SetFade( float in, float out )
{
	_fadeInTime=_fadeInTotal=in;
	_fadeOutTime=_fadeOutTotal=out;
}

void TitleEffectBasic::SetDrawInMap( bool drawInMap)
{
  _drawInMap = drawInMap;
}


void TitleEffectBasic::Init( float time )
{
	float defaultIn = Pars >> "CfgTitles" >> "defaultIn";
	float defaultOut = Pars >> "CfgTitles" >> "defaultOut";
	SetTimes(defaultIn,time,defaultOut);
}

void TitleEffectBasic::Init( RString text, bool structured )
{
	ParamEntryVal cls = Pars >> "RscTitlesText";

	if (structured)
		_text = CreateTextStructured(NULL, text);
	else
		_text = CreateTextPlain(NULL, text);
	SetTextAttribute(_text, "align", "center");
	SetTextAttribute(_text, "font", cls >> "fontBasic");

  ConstParamEntryPtr entry = cls.FindEntry("Attributes");
  if (entry)
  {
    for (int i=0; i<entry->GetEntryCount(); i++)
    {
      ParamEntryVal attr = entry->GetEntry(i);
      RString name = attr.GetName();
      RString value = attr;
      SetTextAttribute(_text, name, value);
    }
  }

	_textSize = cls >> "sizeExBasic";
	_textYOffset = 0;

  if(cls.FindEntry("drawInMap")) _drawInMap = cls >> "drawInMap";
  else _drawInMap = true;
}

void TitleEffectBasic::Init(ParamEntryVal rsc)
{
	_rsc=new DisplayTitle;
	_rsc->LoadFromEntry(rsc);
}

void TitleEffectBasic::Init(LODShape *shape, Vector3Val camera)
{
	_object=new ObjectColored(shape,VISITOR_NO_ID);
	_objectCamera=camera;
}

void TitleEffectBasic::DrawText()
{
  UIViewport *vp = Create2DViewport();
	float top = 0.5 + _textYOffset;
	::DrawText(vp, _text, Rect2DFloat(0, top, 1, 1 /*- top*/), _textSize, _alpha); //top removed - text was clipped at the bottom
	Destroy2DViewport(vp);
}

void TitleEffectBasic::DrawObject()
{
	if( !GScene->GetCamera() ) return;
	// temporary override camera
	if (!_object->GetShape() || _object->GetShape()->NLevels()<=0) return;
	Camera oldCam=*GScene->GetCamera();
	Camera cam;
	// set object drawing parameters
	cam.SetPosition(_objectCamera);

	float fov=0.7;
	//cam.SetPerspective(0.1f,GScene->GetFogMaxRange(),fov,0.75f*fov);
	float cNear=-_objectCamera.Z()*0.5;
	saturate(cNear,0.01,100);
	AspectSettings as;
	GEngine->GetAspectSettings(as);
	cam.SetPerspective(
	  cNear,GScene->GetFogMaxRange(),fov*as.leftFOV,fov*as.topFOV,
	  Glob.config.GetShadowZ(),Glob.config.GetProjShadowsZ(),GScene->GetFogMaxRange()
	);
	cam.Adjust(GEngine);
	GScene->SetCamera(cam);
	PackedColor color(Color(1,1,1,_alpha));
	_object->SetConstantColor(color);
#if _HELISIM_RENDER_TO_TEXTURE
  DoAssert(CheckMainThread());
  _object->Draw(-1, 0, ClipAll, DrawParameters::_default, InstanceParameters::_default, 0, _object->GetFrameBase(), NULL
    , GScene->GetDrawObjectIndex());
#else
  _object->Draw(-1, 0, ClipAll, DrawParameters::_default, InstanceParameters::_default, 0, _object->GetFrameBase(), NULL);
#endif
	// restore camera
	GScene->SetCamera(oldCam);
}

void TitleEffectBasic::DrawRsc()
{
	_rsc->DrawHUD(NULL,_alpha);	
}

bool TitleEffectBasic::PreloadRsc()
{
  return _rsc->IsReady();
}

bool TitleEffectBasic::PreloadObject()
{
  LODShape *lShape = _object->GetShape();
  if (!lShape || lShape->NLevels()<=0) return true;
  if (!lShape->CheckLevelLoaded(0,true))
  {
    return false;
  }
  ShapeUsed shape = lShape->Level(0);
  return shape->PreloadTextures(0, NULL, true);
}

bool TitleEffectBasic::PreloadText()
{
  return ::IsTextReadyToDraw(_text, _textSize);
}

bool TitleEffectBasic::Preload()
{
  bool ret = true;
	if( _object )
	{
	  if (!PreloadObject())
      ret = false;
	}
	if( _rsc )
	{
	  if (!PreloadRsc())
      ret = false;
	}
  if( _text )
  {
    if (!PreloadText()) ret = false;
  }
	return ret;
}

void TitleEffectBasic::Draw()
{
	if( _object ) DrawObject();
	if( _rsc ) DrawRsc();
	if (_text) DrawText();
}

// TitleEffectPlain is plain title (with game in background)

class TitleEffectPlain: public TitleEffectBasic
{
	typedef TitleEffectBasic base;
};

// TitleEffectPlainNoFade is plain title (with game in background) without fade in and fade out

class TitleEffectPlainNoFade: public TitleEffectBasic
{
  typedef TitleEffectBasic base;

public:
  virtual void Init(float time);
};

void TitleEffectPlainNoFade::Init(float time)
{
  SetTimes(0, time, 0);
}

// TitleEffectPlainDown is bottom title (with game in background)

class TitleEffectPlainDown: public TitleEffectBasic
{
	typedef TitleEffectBasic base;

public:
	void Init(RString text, bool structured);
};


void TitleEffectPlainDown::Init(RString text, bool structured)
{
	base::Init(text, structured);
	
	ParamEntryVal cls = Pars >> "RscTitlesText";
	SetTextFont(cls >> "fontDown", cls >> "sizeExDown");
	SetTextPos(cls >> "offsetDown");
}

class TitleEffectBlackFaded: public TitleEffectBasic
{
	typedef TitleEffectBasic base;
	PackedColor _color;

public:
	TitleEffectBlackFaded( PackedColor color=PackedBlack );
	void Draw();
	bool IsTransparent() const {return _alpha>=0.99;}
};

TitleEffectBlackFaded::TitleEffectBlackFaded( PackedColor color )
:_color(color)
{}

void TitleEffectBlackFaded::Draw()
{
	Texture *texture=GScene->Preloaded(TextureWhite);
	Draw2DPars pars;
	pars.mip= GLOB_ENGINE->TextBank()->UseMipmap(texture,0,0);
	int alphaI = toInt(_alpha*255);
	saturate(alphaI,0,255);
	PackedColor color=PackedColorRGB(_color,alphaI);
	pars.SetColor(color);
	pars.spec = NoZBuf|IsAlpha|NoClamp|IsAlphaFog;
	pars.SetU(0,1);
	pars.SetV(0,1);
	Rect2DAbs rect;
	rect.x=0,rect.y=0;
	rect.w=GEngine->WidthBB(),rect.h=GEngine->HeightBB();
	GEngine->Draw2D(pars,rect);
	base::Draw();
}

class TitleEffectColorOut: public TitleEffectBlackFaded
{
	typedef TitleEffectBlackFaded base;
public:
	TitleEffectColorOut(PackedColor color);
	void Init(float time) {SetTimes(1,1e20,1);}
};

TitleEffectColorOut::TitleEffectColorOut( PackedColor color )
:base(color)
{
	SetTimes(1,1e10,1);
}

class TitleEffectColorIn: public TitleEffectBlackFaded
{
	typedef TitleEffectBlackFaded base;
	public:
	TitleEffectColorIn( PackedColor color );
	void Init( float time ) {SetTimes(0,0,1);}
};

TitleEffectColorIn::TitleEffectColorIn( PackedColor color )
:base(color)
{
	SetTimes(0,0,1);
}

class TitleEffectBlackIn: public TitleEffectColorIn
{
public:
	TitleEffectBlackIn():TitleEffectColorIn(PackedBlack){}
};

class TitleEffectBlackOut: public TitleEffectColorOut
{
public:
	TitleEffectBlackOut():TitleEffectColorOut(PackedBlack){}
};

class TitleEffectWhiteIn: public TitleEffectColorIn
{
public:
	TitleEffectWhiteIn():TitleEffectColorIn(PackedWhite){}
};

class TitleEffectWhiteOut: public TitleEffectColorOut
{
public:
	TitleEffectWhiteOut():TitleEffectColorOut(PackedWhite){}
};

class TitleEffectBlack: public TitleEffectBlackFaded
{
	typedef TitleEffectBlackFaded base;
public:
	void Init(float time);
};

void TitleEffectBlack::Init(float time)
{
	//base::Init();
  SetTimes(0, time, 0);
}

static TitleEffectBasic *CreateEffect(TitEffectName name)
{
	switch( name )
	{
		case TitBlack:
		case TitBlackOut:return new TitleEffectBlackOut; 
		case TitBlackIn:return new TitleEffectBlackIn; 
		case TitWhiteOut:return new TitleEffectWhiteOut; 
		case TitWhiteIn:return new TitleEffectWhiteIn; 
		case TitBlackFaded:return new TitleEffectBlack; 
		case TitPlain: return new TitleEffectPlain;
		case TitPlainDown: return new TitleEffectPlainDown;
    case TitPlainNoFade: return new TitleEffectPlainNoFade;
	}
	return NULL;
}

static void SetupEffect(TitEffectName name, TitleEffectBasic *effect)
{
	if (effect)
	  effect->Simulate(0);
}

static float CheckSpeed(float speed)
{
	if (speed > 0)
    return speed;
	return Pars >> "CfgTitles" >> "defaultSpeed";
}

TitleEffect *CreateTitleEffectObj( TitEffectName name, ParamEntryVal entry, float speed, bool showInMap)
{
	RString shapeName=GetShapeName(entry>>"model");
	Ref<LODShape> shape=Shapes.New(shapeName,false,false);
	//Ref<LODShape> shape=new LODShape(shapeName);
	shape->OrSpecial(IsColored|IsAlphaFog|IsAlpha);
	TitleEffectBasic *result=CreateEffect(name);
	if (result)
	{
	  Vector3 camera;
	  ParamEntryVal camEntry=entry>>"camera";
	  camera.Init();
	  camera[0]=camEntry[0];
	  camera[1]=camEntry[1];
	  camera[2]=camEntry[2];
	  float duration=entry>>"duration";
	  result->Init(duration);
	  result->Init(shape,camera);
	  result->SetSpeed(CheckSpeed(speed));
	  SetupEffect(name,result);
    result->SetDrawInMap(showInMap);
	}
	return result;
}

TitleEffect *CreateTitleEffect( TitEffectName name, RString text, float speed, RString font, float size, bool structured, bool showInMap )
{
  if (text.GetLength()==0 && (name==TitPlain || name==TitPlainDown || name==TitPlainNoFade))
    return NULL;
	TitleEffectBasic *result=CreateEffect(name);
	if (result)
	{
	  float defaultTime = Pars >> "CfgTitles" >> "defaultTime";
	  result->Init(defaultTime);
	  result->Init(text, structured);
	  if (font.GetLength() > 0) result->SetTextFont(font, size);
	  result->SetSpeed(CheckSpeed(speed));
	  SetupEffect(name,result);
    result->SetDrawInMap(showInMap);
	}
	return result;
}

TitleEffect *CreateTitleEffectRsc( TitEffectName name, ParamEntryVal entry, float speed, bool showInMap)
{
	TitleEffectBasic *result=CreateEffect(name);
	if (result)
	{
	  float duration=entry>>"duration";
	  float fadeIn;
	  if (entry.FindEntry("fadeIn")) fadeIn = entry >> "fadeIn";
	  else fadeIn = Pars >> "CfgTitles" >> "defaultIn";
	  float fadeOut;
	  if (entry.FindEntry("fadeOut")) fadeOut = entry >> "fadeOut";
	  else fadeOut = Pars >> "CfgTitles" >> "defaultOut";

	  result->Init(duration);
	  result->Init(entry);
	  result->SetSpeed(CheckSpeed(speed));
	  result->SetFade(fadeIn, fadeOut);
	  SetupEffect(name,result);
    result->SetDrawInMap(showInMap);
	}
	return result;
}
