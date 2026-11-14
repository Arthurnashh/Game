//# pragma optimize( "", off ) 
// Poseidon - lights
// (C) 1997, Suma
#include "wpch.hpp"

#include "lights.hpp"
#include "tlVertex.hpp"
#include "vehicle.hpp"
#include "scene.hpp"
#include "camera.hpp"
#include "global.hpp"
#include "landscape.hpp"
#include "world.hpp"
#include "fileLocator.hpp"
#include "paramFileExt.hpp"
#include "paramArchiveExt.hpp"

LightSun::LightSun()
{
  LoadDefault();
	_moonPhase = 0;
	_diffuse=HWhite; // default - full sun
  _ambient=HWhite;
  _sunOrMoon = 1;
}

Color LightSun::AmbientResult() const
{
	return _ambientPrecalc;
}

Color LightSun::ApplyDiffuse(Vector3Par normal, const TLMaterial &mat) const
{
  Color res = mat.forcedDiffuse*_diffuse;
  float cosAlpha = normal*_lightDirection;
  if (cosAlpha<0)
  {
    res = res - mat.diffuse*_diffuse*cosAlpha;
  }
  return res;
}
Color LightSun::ApplyAmbient(Vector3Par normal, const TLMaterial &mat) const
{
  return mat.ambient*_ambient;
}

Color LightSun::Apply(Vector3Par normal, const TLMaterial &mat) const
{
  Color res = mat.ambient*_ambient+mat.forcedDiffuse*_diffuse;
  float cosAlpha = normal*_lightDirection;
  if (cosAlpha<0)
  {
    res = res - mat.diffuse*_diffuse*cosAlpha;
  }
  return res;
}

Color LightSun::SimulateDiffuse( float diffuse) const
{
	return _diffuse*diffuse+_ambient;
}

Color LightSun::FullResult( float diffuse ) const
{
	return _diffusePrecalc*diffuse+_ambientPrecalc;
}

float LightSun::MoonIntensity() const
{
  float cosMoonPhase=-(_direction*_moonDirection);
  // if the moon is below horizon (light direction Y positive), it cannot cast any light
  float moonAboveHorizon = InterpolativC(_moonDirection.Y(),-0.07f,+0.006f,1,0);
  // intensity needs to interpolate between full moon and no moon as well
  // starlight is ambient only, and it is always there
  return (cosMoonPhase+1)*0.5f*moonAboveHorizon;
}

float LightSun::GetEyeAdaptMin(bool r2t) const
{
  // intense light eye adaptation limit
  static float EyeAdaptMin = 0.00020f;//0.0003f;
  static float EyeAdaptMinR2T = 0.000350f;
  return r2t ? EyeAdaptMinR2T : EyeAdaptMin;
}

float LightSun::GetEyeAdaptMax() const
{
  // darkness adaptation limit
  static float EyeAdaptMax = 4.000f;
  return EyeAdaptMax;
}


/**
@param fast avoid any collision detection - function needs to be extemely fast
  this is used for eye pupil animation
*/

float LightSun::EyeAdaptation(VisualStateAge age, Vector3Par observerDir, float fovLeft, float fovTop, bool fast) const
{
  // first implementation - ignore sun direction
  const float avgMaterial = 0.5f;
  float diffuseBrightness = _diffuse.Brightness()*avgMaterial;
  float ambientBrightness = _ambient.Brightness()*avgMaterial;

  // the more we are looking the same direction as the sun,
  // the more it is likely we see sun lighted surfaces
  float cosAngle = observerDir*LightDirection();

  // how big part of the screen is used for sky
  // should be dependent on camera FOV
  float fovFactor = 0.6f/fovTop;
  float skyFactor = floatMax(observerDir.Y()*fovFactor+0.5f,0);
  Color sky,aroundSun;

#if _ARMA3_NEW_LIGHTING_CFG
  if (GLandscape && GLandscape->UseNewLandLighting() && GScene)
  {
    sky = GScene->GetCurrentLighting()._sky;
    aroundSun = GScene->GetCurrentLighting()._skyAroundSun;
  }
  else
#endif
  {
  float cosSun = -SunDirection().Y();
  GLandscape->GetSkyColors(sky,aroundSun,cosSun);
  }

  // TODO: we can consider sky / sky around sun depending on viewing direction
  // TODO: check average texture colors
  // assume average sky texture brightness is around 0.65f
  float skyBrightness = sky.Brightness()*0.65f;
  // how much are we blinded by the sun, 1 means not blinded, lower value is blinded
  float sunBlinding = 0;
  float onTheSun = 1; // when player is in the shadow, accommodate a little bit more
  Vector3 lightDir = LightDirection();
  if (lightDir.Y()<+0.02f)
  {
    if (fast)
    {
      sunBlinding = 1;
    }
    else
    {
      // fictive sun position
      Vector3 cPos = GScene->GetCamera()->Position();
      Vector3 sunPos = cPos - LightDirection()*Glob.config.horizontZ;

      // check intersection with ground and objects
      float t = GLandscape->IntersectWithGroundOrSea(
        NULL,cPos,-lightDir,0,Glob.config.horizontZ*1.1
      );
      float tCockpit = GWorld->IntersectWithCockpit(
        cPos,-lightDir,0,Glob.config.horizontZ*1.1
      );
      float visLand = t>=Glob.config.horizontZ && tCockpit>=Glob.config.horizontZ;
      if (visLand>0)
      {
        // check line against view geometries
        CollisionBuffer col;
        Object *camOn = GWorld->CameraOn();
        if (camOn && !GWorld->GetCameraEffect())
        {
          GLandscape->ObjectCollisionLine(age,col,Landscape::FilterIgnoreOne(camOn),cPos,sunPos,0,ObjIntersectView, ObjIntersectNone, CF_FIRST_CONTACT);
          // check if any of the objects is not considered
          if(col.Size())
              visLand = 0;
        }
        sunBlinding = 1-visLand*0.9f;
        // how much of diffuse sunlight do we consider when in the shadow
        const float sunInShadow = 0.3;
        onTheSun = visLand*(1-sunInShadow)+sunInShadow;
      }
    }
  }
  // check sun intensity - if the sun is quite low, it is soft and will not blind us much
  float sunBright = -lightDir.Y()*1.6;
  saturate(sunBright,0,1);
  float sunHard = sunBright*floatMax(-cosAngle,0);
  
  sunBlinding = sqrt((1-sunHard) + sunHard*sunBlinding);


  float diffuseEstimate;
  const float estForCos0 = 0.06f;
  const float estForCosP1 = 0.4f;
  const float estForCosM1 = 0.02f;
  if (cosAngle>0)
  {
    diffuseEstimate = cosAngle*(estForCosP1-estForCos0)+estForCos0;
  }
  else
  {
    diffuseEstimate = (cosAngle+1)*(estForCos0-estForCosM1)+estForCosM1;
  }
  
  float objectsBrightness = diffuseBrightness*diffuseEstimate*onTheSun+ambientBrightness;

  float typicalBrightness = skyFactor*skyBrightness + (1-skyFactor)*objectsBrightness;
  
  const float invFactorSqrt = 1;
  #if 0 //_PROFILE
    DIAG_MESSAGE(
      100,Format(
        "Acc %.2f, diff est %.3f, sun %.3f, hard %.3f",
        invFactorSqrt*InvSqrt(typicalBrightness),diffuseEstimate,
        sunBlinding,sunHard
      )
    );
  #endif
  // TODO: consolidate, currently exists in World.Simulate as well
  // search for _scene.MainLight()->EyeAccommodation
  //float eyeAccom = invFactorSqrt*typicalBrightness*sunBlinding;
  float minEyeAccom = GetEyeAdaptMin();
  float maxEyeAccom = GetEyeAdaptMax();

  if (sunBlinding*invFactorSqrt < minEyeAccom*typicalBrightness)
  {
    return minEyeAccom;
  }
  if (sunBlinding*invFactorSqrt > maxEyeAccom*typicalBrightness)
  {
    return maxEyeAccom;
  }
        
  return invFactorSqrt*sunBlinding/typicalBrightness;
}


void LightSun::LoadDefault()
{
  _groundReflection = Color(0.085,0.068,0.034); 

  _moonObjectColorFull = Color(0.9,0.9,1.0,0.7); 
  _moonHaloObjectColorFull = Color(0.9,0.9,1.0,0.05); 
  _moonsetObjectColor = Color(0.9,0.75,0.4); 
  _moonsetHaloObjectColor = Color(0.9,0.5,0.2); 

  _sunSunset=20*(H_PI/180);
  _endSunset=10*(H_PI/180);
  _nightAngle=5*(H_PI/180);

  _sinSunSunset=sin(_sunSunset),_invSinSunSunset=1/_sinSunSunset;
  _sinEndSunset=sin(_endSunset),_invSinEndSunset=1/_sinEndSunset;
  _sinNightAngle=sin(_nightAngle),_invSinNightAngle=1/_sinNightAngle;
}

void LightSun::Load(ParamEntryPar cfg)
{
  GetValue(_groundReflection, cfg>>"groundReflection");

  GetValue(_moonObjectColorFull, cfg>>"moonObjectColorFull"); 
  GetValue(_moonHaloObjectColorFull, cfg>>"moonHaloObjectColorFull"); 
  GetValue(_moonsetObjectColor, cfg>>"moonsetObjectColor"); 
  GetValue(_moonsetHaloObjectColor, cfg>>"moonsetHaloObjectColor"); 

  _sunSunset = float(cfg>>"sunSunset")*(H_PI/180);
  _endSunset = float(cfg>>"endSunset")*(H_PI/180);
  _nightAngle = float(cfg>>"nightAngle")*(H_PI/180);

  _sinSunSunset=sin(_sunSunset),_invSinSunSunset=1/_sinSunSunset;
  _sinEndSunset=sin(_endSunset),_invSinEndSunset=1/_sinEndSunset;
  _sinNightAngle=sin(_nightAngle),_invSinNightAngle=1/_sinNightAngle;
}

inline float ConvertSunAngle( float sunAngle )
{
	return AngleDifference(H_PI,sunAngle);
}

// create a coordinate system simulating Earth and Moon movement relative to sun

/*!
\patch_internal 1.50 Date 4/15/2002 by Ondra
- New: Variable longitude/latitude support.
*/

extern bool autoDir;


void LightSun::Recalculate(World *world)
{
	if (!world) world = GWorld;
	float latitudeCoord = world ? world->GetLatitude() : -40*H_PI/180;

	static const Matrix3 moonOrbitAngle(MRotationZ,5*H_PI/180);
	static const Matrix3 earthAxis=Matrix3(MRotationX,23*H_PI/180);
	Matrix3 latitude(MRotationX,latitudeCoord); // -40 - Croatia, -90 - north pole

	const float initMoonOnOrbitPos=0.5f;
	const float day=1.0f/365;
	const float lunarMonth=28*day;

  // shortest day (earth pos. 0) is Dec 21 - 11 days before the beginning of the year
	float timeInYear = Glob.clockWeather.GetTimeInYear()-11*day;
	float timeOfDay = Glob.clockWeather.GetTimeOfDay();
	//float dayOfYear=date*day; // 0 - the first day of winter
	float moonOrbitPhase = Glob.clockWeather.GetTimeInYear()+Glob.clockWeather.GetYear()-1985-3*day;
	float moonOnOrbitPos=initMoonOnOrbitPos+moonOrbitPhase*(1.0f/lunarMonth);
	Matrix3Val moonOnOrbit=moonOrbitAngle*Matrix3(MRotationY,moonOnOrbitPos*(H_PI*2));
	Matrix3Val earthOnOrbit=Matrix3(MRotationY,timeInYear*(H_PI*2));
	// note - midnight is on the point furthest from the sun
	Matrix3Val midnightToCurrent=Matrix3(MRotationY,timeOfDay*(H_PI*2));
	// calculate sun and moon position relative to current position
	Matrix3Val cameraToCosmos=earthAxis*midnightToCurrent*earthOnOrbit*latitude;
	Matrix3Val cameraToStars=midnightToCurrent*earthOnOrbit*latitude;
	Matrix3Val cosmosToCamera=cameraToCosmos.InverseRotation();
	Matrix3Val starsToCamera=cameraToStars.InverseRotation();
	// use rotation of PI/2 to achieve this
	static const Matrix3 normalDirection(MRotationX,-H_PI/2);
	Matrix3Val convert=normalDirection*cosmosToCamera;
	_direction=convert*earthOnOrbit.Direction();
	_moonDirection=convert*moonOnOrbit.Direction();

	_starsOrientation=normalDirection*starsToCamera;

	// reverse N-S, W-W
	_direction[0]=-_direction[0];
	_direction[2]=-_direction[2];
	
	_moonDirection[0]=-_moonDirection[0];
	_moonDirection[2]=-_moonDirection[2];

#if _ARMA3_SIMUL_WEATHER
//
	/*
  if (!autoDir && GLandscape && GLandscape->GetSimulWeather() && GLandscape->IsSimulWeatherEnabled())
  {
    // override sun and moon direction with SW values
    if (GLandscape->GetSimulWeather()->GetCloudRenderer() && GLandscape->GetSimulWeather()->GetCloudRenderer()->GetSkyInterface())
    {
      simul::sky::float4 dirSun = GLandscape->GetSimulWeather()->GetCloudRenderer()->GetSkyInterface()->GetDirectionToSun();
      _direction = Vector3(-dirSun.x, -dirSun.z, -dirSun.y);

      simul::sky::float4 dirMoon = GLandscape->GetSimulWeather()->GetCloudRenderer()->GetSkyInterface()->GetDirectionToMoon();
      _moonDirection= Vector3(-dirMoon.x, -dirMoon.z, -dirMoon.y);
      _moonDirection.Normalize();
    }
  }
	*/
#endif

	_direction.Normalize();
	
	_starsOrientation(0,0)=-_starsOrientation(0,0);
	_starsOrientation(2,0)=-_starsOrientation(2,0);
	_starsOrientation(0,1)=-_starsOrientation(0,1);
	_starsOrientation(2,1)=-_starsOrientation(2,1);
	_starsOrientation(0,2)=-_starsOrientation(0,2);
	_starsOrientation(2,2)=-_starsOrientation(2,2);

	// calculate _moonDirectionUp so that moon is always facing the sun 
	// note: this is inaccurate, but as both sun
	Vector3 moonDirectionUp = _moonDirection-_direction;
  // make sun->MoonDirectionUp() perpendicular to MoonDirection() to prevent gimbal lock
	// TODO: it should be possible to somehow fix the moon "up" vector, but it needs thinking
  Matrix3 moonDir(MDirection,_moonDirection,_moonDirection-_direction);
  _moonDirectionUp = moonDir.DirectionUp();
	// both _direction and _moonDirection are normalized
	// moon phase is determined by position of moon relative to sun
	float cosMoonPhase=-(_direction*_moonDirection);
	float moonHaloIntensity=(cosMoonPhase+1)*0.5;
	float moonIntensity=floatMin(cosMoonPhase+1,1);
	//float moonHaloIntensity=1.0;
	//float moonIntensity=1.0;
	// cos==0 -> full moon, cos == -1 -> dark moon
	//Assert( cosMoonPhase>=0 );
	// determine if we should rather use C or D moon
	// use D moon if moon follows sun
	//float moonPolar=atan2(_moonDirection.X(),_moonDirection.Z());
	//float sunPolar=atan2(_direction.X(),_direction.Z());

	_moonPhase=0.5+acos(cosMoonPhase)/(2*H_PI); // D moon
	//LogF("_moonPhase %g, %g",_moonPhase,cosMoonPhase);


	float sinSun=-_direction.Y();
	float absSinSun=fabs(sinSun);
	//float whiteCoef=0;

	if( sinSun<0 )
	{
		// night, early morning or late evening
		if( absSinSun>_sinEndSunset )
		{
			_starsVisible=1;
		}
		else
		{
			_starsVisible=absSinSun*_invSinEndSunset;
		}
		_nightEffect=1;
	}
	else
	{
		// day
		if( absSinSun>_sinNightAngle ) _nightEffect=0;
		else _nightEffect=1-absSinSun*_invSinNightAngle;
		_starsVisible=0;
	}

  // Get the sun vs moon control
  //float diffuseCrossingFactor; // 1 - full diffuse, 0 - all diffuse moved to ambient
  if (_sunOrMoon>=0.5f)
  {
    _lightDirection = _direction;
  }
  else
  {
    _lightDirection = _moonDirection;
  }
  
	float sinMoon=-_moonDirection.Y();
	if( sinMoon<0 ) // moon below horizont
	{
		_moonObjectColor=_moonsetObjectColor;
		_moonHaloObjectColor=_moonsetHaloObjectColor;
	}
	else
	{
		if( sinMoon>_sinSunSunset )
		{
			_moonObjectColor=_moonObjectColorFull;
			_moonHaloObjectColor=_moonHaloObjectColorFull;
		}
		else
		{
			float sunset=1-sinMoon*_invSinSunSunset;
			_moonObjectColor=_moonHaloObjectColorFull+(_moonsetHaloObjectColor-_moonHaloObjectColorFull)*sunset;
			_moonHaloObjectColor=_moonObjectColorFull+(_moonsetObjectColor-_moonObjectColorFull)*sunset;
		}
	}
	_moonObjectColor.SetA(_moonObjectColorFull.A()*moonIntensity);
	_moonHaloObjectColor.SetA(_moonHaloObjectColorFull.A()*moonHaloIntensity);

	// enable lights when night effects are on

  // Remember direction of the sun
  _sunDirection=_direction;
//LogF("%f", _moonDirection.Y());
  // Get the shadow direction (note that it contains moon direction in the night)
  {
    _shadowDirection=_lightDirection;
#if _ENABLE_REPORT
    static float maxShadowDer = -0.2f;
#else
    const float maxShadowDer = -0.2f;
#endif
    if (_shadowDirection[1] > maxShadowDer)
    {
      _shadowDirection[1] = maxShadowDer;
      _shadowDirection.Normalize();
    }
    //_shadowDirection = Vector3(0, -1, 0);
  }
}

/**
@return ambient intensity modulation factor - during night time this depends on the moon phase.

\patch 5128 Date 2/14/2007 by Ondra
- Fixed: Moon could light the scene even when below horizon, resulting in strangely lit objects.

\patch 5128 Date 2/14/2007 by Ondra
- Fixed: Change of the light level between day and night was too abrupt.
*/
void LightSun::SetLighting(ColorVal amb, ColorVal dif, ColorVal sun, float sunOrMoon)
{
  _sunOrMoon = sunOrMoon;
  float cosMoonPhase=-(_direction*_moonDirection);

  // if the moon is below horizon (light direction Y positive), it cannot cast any light

  float moonAboveHorizon = InterpolativC(_moonDirection.Y(),-0.07f,+0.006f,1,0);

  // always guarantee some minimum amount of both diffuse and ambient
  const float starLight = 0.10f;
  // intensity needs to interpolate between full moon and no moon as well
  // starlight is ambient only, and it is always there
  float moonIntensity = (cosMoonPhase+1)*0.5f*moonAboveHorizon;
  float ambIntensity = _sunOrMoon + (1-_sunOrMoon)*(moonIntensity+starLight);
  _ambient = amb * ambIntensity;
  // when cross is near 0.5, diff should be near 0
  Assert(fabs(_sunOrMoon-0.5f)>0.1f || dif.Brightness()<fabs(_sunOrMoon*2-1)*50.0f);
  if (_sunOrMoon<0.5f)
  {
    //float cross = 1-_sunOrMoon*2;
    // light is Moon - modulate by Moon phase
	  _diffuse = dif*moonIntensity;
    _lightDirection = _moonDirection;
  }
  else
  {
    // _sunOrMoon 0.5 .. 1.0
    //float cross = _sunOrMoon*2-1;
    _diffuse = dif;
    _lightDirection = _direction;
  }
  float a = sun.A();
  static float objRelColor = 1.2f;
  static float haloRelColor = 0.7f;
  _sunObjectColor = sun*objRelColor;
  _sunHaloObjectColor = sun*haloRelColor;
  _sunObjectColor.SetA(a);
  _sunHaloObjectColor.SetA(a);
}


//-----------------------------------------------------------------------------
bool Light::IsShining() const
{
	if (!(_iFlags & LF_ON))
		return false;

	//day light shines anytime
	if (_iFlags & LF_DAY_LIGHT)
		return true;

	//otherwise just in night
	return GScene->MainLight()->NightEffect() >= 0.01f;
}


//-----------------------------------------------------------------------------
int Light::CompareInternal(Vector3Par impPos, const Light &with) const
{
	// Compare distances to the important position
	Vector3 relThisPos=impPos-Position();
	Vector3 relWithPos=impPos-with.Position();

	float distThis2=relThisPos.SquareSize();
	float distWith2=relWithPos.SquareSize();

	float diff=distWith2*SortBrightness()-distThis2*with.SortBrightness();
	if( diff>0 ) return +1;
	if( diff<0 ) return -1;

	return 0;
}

//-----------------------------------------------------------------------------
int Light::Compare( const Light &with, const LightContext &context ) const
{
  // Get the important position - it can be either position of the camera if camera lies within
  // the radius, or position of the point from the radius closest to the camera
  Vector3 impPos;
 
	const Vector3 &cameraPosition = GLOB_SCENE->GetCamera()->Position();

	// Get the object2camera vector and object to camera distance
  Vector3 object2camera = cameraPosition - context.position;
  float squareDistance = object2camera.SquareSize();

	// Get the position within the radius, closest to the camera
	if (squareDistance < Square(context.radius))
  {
    impPos = cameraPosition;
  }
  else
  {
		object2camera.Normalize();
	  impPos = context.position + object2camera * context.radius;
  }

	return CompareInternal(impPos, with);
}

//-----------------------------------------------------------------------------
int Light::Compare( const Light &with ) const
{
	return CompareInternal(GLOB_SCENE->GetCamera()->Position(), with);
}

//-----------------------------------------------------------------------------
float Light::SortBrightness() const
{
	return Brightness();
}

//-----------------------------------------------------------------------------
bool Light::Visible( const Object *obj ) const
{
	return false;
}

//-----------------------------------------------------------------------------
void PointLight::Draw(int cb, int level, ClipFlags clipFlags,
  const DrawParameters &dp, const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
  , int drawListId
#endif
  )
{
  if (!GEngine->GetThermalVision())
    GLandscape->DrawPoint(cb,pos.PositionWorld(*GScene->GetCamera()), _intensity, _color
#if _HELISIM_RENDER_TO_TEXTURE
    , drawListId
#endif    
    );
}

//-----------------------------------------------------------------------------
void LightPositioned::Prepare( const Matrix4 &worldToModel )
{
	_modelPos=worldToModel.FastTransform(Position());
	_modelDir=worldToModel.Rotate(Direction());
	_modelDir.Normalize();
}

//-----------------------------------------------------------------------------
void LightSun::SetMaterial(const TLMaterial &mat, ColorVal accom)
{
	_ambientPrecalc = (_ambient*mat.ambient+_diffuse*mat.forcedDiffuse) * accom;
	_diffusePrecalc = (_diffuse*mat.diffuse) * accom;
}

//-----------------------------------------------------------------------------
void LightPositionedColored::SetMaterial(const TLMaterial &mat, ColorVal accom)
{
	_ambientPrecalc = (_ambient*mat.ambient+_diffuse*mat.forcedDiffuse) * accom;
	_diffusePrecalc = (_diffuse*mat.diffuse) * accom;
}

//-----------------------------------------------------------------------------
bool LightPoint::Visible( const Object *obj ) const
{
	return SquareDistance(obj->FutureVisualState().Position()) < Square(_radius);
}

//-----------------------------------------------------------------------------
__forceinline float GetDistanceAttenuation(Vector3Par relPos, float radius, float invRadius, float& invDist)
{
	float dist2 = relPos.SquareSize();

	if (dist2 >= Square(radius))
		return 0.0f;

	invDist = InvSqrt(dist2);
	float distance = dist2*invDist;

	return Square(Square(1 - distance*invRadius));
}

//-----------------------------------------------------------------------------
float LightPoint::FlareIntensity( Vector3Par camPos, Vector3Par camDir ) const
{
	Vector3Val relPos = camPos - Position();
	float invDist, distAtt = GetDistanceAttenuation(relPos, _radius, _invRadius, invDist);

	if (distAtt > 0.0)
		return (-camDir.DotProduct(relPos*invDist))*distAtt;

	return 0.0f;
}

//-----------------------------------------------------------------------------
__forceinline Color LightPoint::Apply( Vector3Par point, Vector3Par normal, const Color& diffuse, const Color& ambient) const
{
	// normal and point is given in model space, calculate distance from pointlight
	Vector3Val relPos = point - _modelPos;
	float invDist, distAtt = GetDistanceAttenuation(relPos, _radius, _invRadius, invDist);

	if (distAtt <= 0.0)
		return Color(HBlack);

	//note cosFi is actualy cosFi*size
	float cosFi = relPos*normal;
	if (cosFi > 0.0f)
	{
		cosFi *= invDist;
		return (diffuse*cosFi + ambient)*(distAtt*_brightnessCoef*_brightnessDayCoef);
	}

	return ambient*(distAtt*_brightnessCoef*_brightnessDayCoef);
}

//-----------------------------------------------------------------------------
Color LightPoint::Apply( Vector3Par pos, Vector3Par normal )
{
	if (!IsShining() || Position().Distance2(pos) == 0)
		return HBlack;

	return Apply(pos, normal, _diffuse, _ambient);
}

//-----------------------------------------------------------------------------
Color LightPoint::ApplyPrecalc( Vector3Par point, Vector3Par normal ) 
{ 
	return Apply(point, normal, _diffusePrecalc, _ambientPrecalc);
}

//-----------------------------------------------------------------------------
void LightPoint::GetDescription(LightDescription &desc, ColorVal accom) const
{
	desc.type = LTPoint;
	desc.dir = Direction();
	desc.pos = Position(); 
	desc.invRadius = _invRadius;
	
	float bright = _brightnessCoef*_brightnessDayCoef;
	desc.ambient = GetAmbient()*accom*bright;
	desc.diffuse = GetDiffuse()*accom*bright;
	
	desc.spotK = 0;
	desc.spotQ = 0;
}

//-----------------------------------------------------------------------------
bool LightPoint::Cull(Vector3Par pos, float radius) const
{
  return SquareDistance(pos) > Square(radius + _radius);
}

//-----------------------------------------------------------------------------
void LightReflector::AngleHasChanged()
{
	_angle = floatMinMax(_angle, 0.0f, 179.9f);

  float angleHalfRad = 0.5f*_angle*(H_PI/180.0f);
 	_invSinAngleHalf = 1.0f/sin(angleHalfRad);

	_cosPhi	= cos(angleHalfRad);
	float cosTheta = cos(angleHalfRad * _innerAngleRatio);

	enf::GetLineEq(cosTheta, _cosPhi, _spotK, _spotQ);
}

//-----------------------------------------------------------------------------
LightReflector::LightReflector(LODShape *shape, ColorVal color, ColorVal ambient, float angle, float angleInnerRatio, bool vlOrthogonalToPlayer, float size)
	:_shape(shape),base(color,ambient),_angle(angle), _innerAngleRatio(angleInnerRatio),_vlOrthogonalToPlayer(vlOrthogonalToPlayer),_size(size), _isFlashLight(false)
{
	SetRadius(50.0f);
  AngleHasChanged();
}

//----------------------------------------------------------------------------- 
LightReflector::LightReflector( LODShape *shape, ColorVal color, ColorVal ambient, float angle, float angleInnerRatio , Vector3Val scale, bool isFlashLight)
	:_shape(shape),base(color,ambient),_angle(angle), _innerAngleRatio(angleInnerRatio), _vlOrthogonalToPlayer(false),_scale(scale),_size(1),_isFlashLight(isFlashLight)
{
	SetRadius(50.0f);
  AngleHasChanged();
}

//-----------------------------------------------------------------------------
bool LightReflector::Visible( const Object *obj ) const
{
	//does it return if obj is lighted by light?
	return !Cull(obj->FutureVisualState().Position(), obj->GetRadius());

	// TODO: check object BSphere vs. light cone
	// default implementation: check only distance 
	//float dist2 = obj->FutureVisualState().Position().Distance2(Position());
	//return Brightness()>0.1*dist2;
}

//-----------------------------------------------------------------------------
__forceinline float GetSpotDistAttenuation(Vector3Par relPos, Vector3Par dir, float radius, float invRadius, float spotK, float spotQ, float& invDist)
{
	//far away
	float dist2 = relPos.SquareSize();
	if (dist2 >= Square(radius))
		return 0.0f;

	float cosAngle = dir.DotProduct(relPos);

	//opposite side of cone
	if (cosAngle <= 0.0f)
		return 0.0f;

	invDist = InvSqrt(dist2);

	//spot attenuation
	float cosPos = cosAngle*invDist;
	float spotAtt = floatMin(cosPos*spotK + spotQ, 1.0f);

	//not in spot
	if (spotAtt <= 0)
		return 0.0f;

	//distance attenuation simulation, power4
	float distAtt = Square(Square(1 - dist2*invDist*invRadius));

	return spotAtt*distAtt;
}

//-----------------------------------------------------------------------------
float LightReflector::FlareIntensity( Vector3Par camPos, Vector3Par camDir ) const
{
	Vector3Val relPos = camPos - Position();
	float invDist, att = GetSpotDistAttenuation(relPos, Direction(), _radius, _invRadius, _spotK, _spotQ, invDist);

	//todo: better camDir influence on position
	if (att > 0.0)
		return (-camDir.DotProduct(relPos*invDist))*att;

	return 0.0f;
}


//-----------------------------------------------------------------------------
Color LightReflector::Apply( Vector3Par pos, Vector3Par normal, const Color& diffuse, const Color& ambient)
{
	Vector3Val relPos = pos - Position();
	float invDist, att = GetSpotDistAttenuation(relPos, Direction(), _radius, _invRadius, _spotK, _spotQ, invDist);

	if (att <= 0.0)
		return Color(HBlack);

	//note cosFi is actualy cosFi*size
	float cosFi = relPos*normal;
	if (cosFi > 0.0f)
	{
		cosFi *= invDist;
		return (diffuse*cosFi + ambient)*(att*_brightnessCoef*_brightnessDayCoef);
	}

	return ambient*(att*_brightnessCoef*_brightnessDayCoef);
}

//-----------------------------------------------------------------------------
Color LightReflector::Apply( Vector3Par pos, Vector3Par normal )
{
  if (!IsShining() || Position().SquareSize() == 0) // hotfix: invalid light
    return HBlack;

	return Apply(pos, normal, _diffuse, _ambient);
}

//-----------------------------------------------------------------------------
Color LightReflector::ApplyPrecalc( Vector3Par point, Vector3Par normal )
{
	return Apply(point, normal, _diffusePrecalc, _ambientPrecalc);
}

//-----------------------------------------------------------------------------
void LightReflector::GetDescription(LightDescription &desc, ColorVal accom) const
{
	desc.type = LTSpotLight;
	desc.dir = Direction();
	desc.pos = Position(); // ignored for directional
	desc.invRadius = _invRadius;
	
	float bright = _brightnessCoef*_brightnessDayCoef;
	desc.ambient = GetAmbient() * accom * bright;
	desc.diffuse = GetDiffuse() * accom * bright;
	
	desc.spotK = _spotK;
  desc.spotQ = _spotQ;
}

//-----------------------------------------------------------------------------
//if result is true, pos is NOT in light cone and isn't affected by light
bool LightReflector::Cull(Vector3Par pos, float objRadius) const
{
	//test object distance to light maximum shine (radius)
	Vector3 relPos = pos - Position();
	float sqDist = relPos.SquareSize();

	if (sqDist > Square(_radius + objRadius))
		return true;

	//update light position by offseting back according the radius of object
	float offset = _invSinAngleHalf*objRadius;
	Vector3 updLightPos = Position() - Direction()*offset;

	//now we can test just the relation of pos and updated light cone
	relPos = pos - updLightPos;

	//not normalized ! thus cosAngle must be divided by relPos size to have exact cosAngle
	float cosAngle = Direction().DotProduct(relPos); 

	//opposite side of cone
	if (cosAngle <= 0.0f)
		return true;

	sqDist = relPos.SquareSize();
	float invDist = InvSqrt(sqDist);

	return cosAngle*invDist < _cosPhi;
}

//-----------------------------------------------------------------------------
void LightPoint::Load(ParamEntryPar cls)
{
	_diffuse = GetColor(cls >> "color");
	_ambient = GetColor(cls >> "ambient");

	float radius = cls >> "radius";

	SetRadius(radius);		
}

//-----------------------------------------------------------------------------
void LightPoint::Load(ParamEntryPar cls, EffectVariablesPar evars)
{
  evars.GetValue(_diffuse, cls >> "color");
  evars.GetValue(_ambient, cls >> "ambient");

	float radius = evars.GetValue(cls >> "radius");
  
	SetRadius(radius);
}

//-----------------------------------------------------------------------------
LightPoint *LightPoint::CreateObject(ParamArchive &ar)
{
  return new LightPoint();
}

//-----------------------------------------------------------------------------
LSError LightPoint::Serialize(ParamArchive &ar)
{
  CHECK(::Serialize(ar, "diffuse", _diffuse, 1))
  CHECK(::Serialize(ar, "ambient", _ambient, 1))
  CHECK(ar.Serialize("radius", _radius, 1))
	CHECK(ar.Serialize("brightness", _brightnessCoef, 1))
  return LSOK;
}

//-----------------------------------------------------------------------------
LightPointEntity::LightPointEntity(): Entity(NULL, VehicleTypes.New("#lightpoint"), CreateObjectId())
{
  _light = new LightPoint(HBlack, HBlack);
  GScene->AddLight(_light);
}

//-----------------------------------------------------------------------------
void LightPointEntity::Simulate(float deltaT, SimulationImportance prec)
{
  if (_light)
  {
    if (_object) SetPosition(_object->FutureVisualState().PositionModelToWorld(_modelPos));
    _light->SetTransform(FutureVisualState().Transform());
  }
}


DEFINE_CASTING(LightPointEntity)

//-----------------------------------------------------------------------------
void LightReflector::ToDraw( ClipFlags clipFlags, bool dimmed )
{
	// sphere light: draw volumetric light object
	if (!IsDayLight() && GScene->MainLight()->NightEffect() < 0.01) 
		return;

	if( dimmed ) return;
	Color c=GetObjectColor();
	//float cSize = c.R()*c.R()+c.G()*c.G()+c.B()*c.B();
	// if light is black, there is nothing to render
	//if (cSize<=1e-10) return;
	// TODO: precalculate in LightReflector SetColor
	//float invSize=InvSqrt(cSize);
	//c = c*invSize;
	if (_isFlashLight)
    // DrawVolumeLight - scaling not working properly when player rotate
    GScene->DrawFlashLight(_shape, Position(), Direction(), c, _scale);
#if 0 // news:hnvq2c$mst$1@new-server.localdomain
  //else
    //GScene->DrawVolumeLight(_shape, c, *this, _size, _vlOrthogonalToPlayer);
#endif
}

LightReflectorOnVehicle::LightReflectorOnVehicle (LODShape *shape, ColorVal diffuse, ColorVal ambient, Object *vehicle, Vector3Par position, Vector3Par direction, float angle, float angleInnerRatio, float size)
  :LightReflector(shape,diffuse,ambient,angle,angleInnerRatio,false,size),
  AttachedOnVehicle(vehicle,position,direction)
{
}

LightPointOnVehicle::LightPointOnVehicle (LODShape *shape, ColorVal diffuse, ColorVal ambient, Object *vehicle,  Vector3Par position, float size)
  : LightPointVisible(shape, diffuse, ambient, true, size),	AttachedOnVehicle(vehicle,position,Vector3(0,0,1))
{
}

LightPointOnVehicle::LightPointOnVehicle (Object *vehicle, Vector3Par position)
  : LightPointVisible(true), AttachedOnVehicle(vehicle,position,Vector3(0,0,1))
{
}
//-----------------------------------------------------------------------------
LightPointVisible::LightPointVisible(bool vlOrthogonalToPlayer)
{
  _size = 1.0f;
  _vlOrthogonalToPlayer = vlOrthogonalToPlayer;

  // Create the point light object, shade the shape with the VASI (it doesn't matter what shape will be there - just NULL can't be there)
  _pointLight = new PointLight(GLandscape->GetPointShape(), VISITOR_NO_ID);
}

//-----------------------------------------------------------------------------
LightPointVisible::LightPointVisible(LODShape *shape,	ColorVal color, ColorVal ambient, bool vlOrthogonalToPlayer, float size, bool visibleDuringDay)
	: LightPoint(color,ambient),_shape(shape),_size(size)
{
  _vlOrthogonalToPlayer = vlOrthogonalToPlayer;
	SwitchDayLight(visibleDuringDay);

  // Create the point light object, shade the shape with the VASI (it doesn't matter what shape will be there - just NULL can't be there)
  _pointLight = new PointLight(GLandscape->GetPointShape(), VISITOR_NO_ID);
}


//-----------------------------------------------------------------------------
void LightPositionedColored::PrepareHeatHaze(HeatHazeRenderArray& instances) const
{
	// apply perspective
	const Camera& camera = *GScene->GetCamera();
	Matrix4Val project = camera.Projection();

	//view position
	Vector3 viewPos = GScene->ScaledInvTransform()*Position();
	if (viewPos.z < camera.Near() || viewPos.z > Glob.config.GetShadowZ()) 
		return;

	float invZ = 1.0/viewPos[2];

	// perspective screen size
	static float sizeMul = 2.0f;

	float radius = sizeMul*invZ*GetHeatHazeRadius();
	float sizeX =  project(0,0)*camera.InvLeft()*radius;
	float sizeY = -project(1,1)*camera.InvTop()*radius;

	float minSize = floatMin(sizeX, sizeY);

	const float MinPixelSize		= 10.0f;
	const float NormalPixelSize = 20.0f;
	float sizeAtt = floatMin(1.0f, (minSize - MinPixelSize)/NormalPixelSize);

	if (sizeAtt < 0.0f)
		return;

	//last 25% of maximum possible distance slowly dissappear 
	float linearZ01 = viewPos.z*Glob.config.GetMaxInvLinearDepth();
	float distAtt = floatMinMax(4.0f*(1.0f - linearZ01) , 0.0f, 1.0f);

	static float overallPower = 1.0f;
	float power = distAtt*sizeAtt;
	if (power < 0.01f)
		return;

	power *= overallPower*GetHeatHazePower();

	Vector3 projPos;
	projPos[0] = project(0,2) + project(0,0)*viewPos[0]*invZ;
	projPos[1] = project(1,2) + project(1,1)*viewPos[1]*invZ;
	projPos[2] = project(2,2) + project.Position()[2]*invZ;

	HeatHazeRenderInstance& inst = instances.Append();
	inst.ProjPos	= projPos;
	inst.LinearZ	= linearZ01;
	inst.SizeX		= sizeX;
	inst.SizeY		= sizeY;
	inst.Power		= power;
}

//-----------------------------------------------------------------------------
void LightPointVisible::ToDraw( ClipFlags clipFlags, bool dimmed )
{
	// sphere light: draw volumetric light object
	if (_size <= 0.0f || (!IsDayLight() && GScene->MainLight()->NightEffect() < 0.01f))
    return;

	if( dimmed ) return;
	Color c=GetObjectColor();
	// TODO: precalculate in LightPointVisible SetColor
	//float invSize=InvSqrt(c.R()*c.R()+c.G()*c.G()+c.B()*c.B());
	//c=c*invSize;

  // If light is orthogonal to the player then draw the special point lights, else draw the old style volume lights
  if (_vlOrthogonalToPlayer)
  {
    // Draw
    _pointLight->SetTransform(Transform());
    _pointLight->SetPosition(Position());
    float dist2 = (GLOB_SCENE->GetCamera()->Position() - Position()).SquareSize();
    
 // Empirical constant tunned to match the previous solution size
    float intensityCoef = InterpolativC(GScene->MainLight()->NightEffect(), 0.0f, 1.0f, 4000.0f, 40.0f);

    if (GScene->MainLight()->NightEffect() < 0.01)
    {
			c.NormalizeColor();
			c.SetA(1.0f);
		}

    _pointLight->SetColorAndIntensity(c, intensityCoef * _size / dist2); 
    GScene->ObjectForDrawing(_pointLight, 0, ClipAll);
  }
  else
  {
    GScene->DrawVolumeLight(_shape, c, *this, _size, false);
  }
}

//-----------------------------------------------------------------------------
void LightPointVisible::Load(ParamEntryPar cls)
{
	LightPoint::Load(cls);
	_shape = Shapes.New(GetShapeName(cls >> "shape"), false, false);
	_size = cls >> "size";
}

//-----------------------------------------------------------------------------
void LightPointOnVehicle::Load(ParamEntryPar cls)
{
	LightPointVisible::Load(cls);
	Object *obj = AttachedOn();
	LODShape *shape = obj ? obj->GetShape() : NULL;
	if (shape)
	{
		RString pos = cls >> "position";
		Vector3Val position = shape->MemoryPoint(pos);
		SetAttachedPos(position, Vector3(0,0,1));
	}
}

