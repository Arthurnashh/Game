// define conventions

#define GEOMETRY_SPEC ( 1e13 )

#define SPEC_LOD 1e15
#define MEMORY_SPEC ( SPEC_LOD*1 )
#define LANDCONTACT_SPEC ( SPEC_LOD*2 )
#define ROADWAY_SPEC ( SPEC_LOD*3 )
#define PATHS_SPEC ( SPEC_LOD*4 )
#define HITPOINTS_SPEC ( SPEC_LOD*5 )
#define SHADOWVOLUME_SPEC ( 10000.0f )
#define SHADOWVOLUME_MAX ( 10999.0f )
#define SHADOWBUFFER_SPEC ( 11000.0f )
#define SHADOWBUFFER_MAX  ( 11999.0f )
#define VIEW_GEOM_SPEC ( SPEC_LOD*6 )
#define FIRE_GEOM_SPEC ( SPEC_LOD*7 )

#define SHADOW_VOLUME_CARGO (SPEC_LOD*18)
#define SHADOW_VOLUME_PILOT (SPEC_LOD*19)
#define SHADOW_VOLUME_GUNNER (SPEC_LOD*20)

#define VIEW_CARGO_GEOM_SPEC ( SPEC_LOD*8 )
#define VIEW_PILOT_GEOM_SPEC ( SPEC_LOD*13 )
#define VIEW_GUNNER_GEOM_SPEC ( SPEC_LOD*15 )
#define FIRE_GUNNER_GEOM_SPEC ( SPEC_LOD*16 )

#define SUB_PARTS_SPEC ( SPEC_LOD*17 )

#define WRECK_SPEC ( SPEC_LOD*21 )

#define VIEW_GUNNER 1000
#define VIEW_PILOT 1100
#define VIEW_CARGO 1200 // was VIEW_COMMANDER before

enum MaterialSection
{
	// predefined materials
  /// simple emissivity - be careful with HDR, can mostly be seen during night time only
	MSShining=200,
  /// no diffuse
	MSInShadow,
  /// half diffuse, normal ignored
	MSHalfLighted,
  /// full diffuse, normal ignored
	MSFullLighted,
  /// little diffuse, normal used
	MSInside,
  /// color * 0.75, no diffuse
	MSInShadow75,
  /// color * 0.75, little diffuse
	MSInside75,
  /// color * 0.50, no diffuse
	MSInShadow50,
  /// color * 0.50, little diffuse
	MSInside50,
  /// auto-adjusting emissivity - provides reasonable luminance both day and night
	MSShiningAdjustable,
};

inline bool IsSpec( float resolution, float spec )
{
	if( fabs(resolution-spec)<spec*1e-3f ) return true;
	return false;
}

inline bool IsSpecInterval(float resolution, float spec, float maxSpec)
{
  if ((resolution > (spec - 1e-3f)) && (resolution < (maxSpec + 1e-3f))) return true;
  return false;
}

inline bool IsSpecShadowVolume(float resolution)
{
  return IsSpecInterval(resolution, SHADOWVOLUME_SPEC, SHADOWVOLUME_MAX) ||
         IsSpec(resolution, SHADOW_VOLUME_CARGO) ||
         IsSpec(resolution, SHADOW_VOLUME_PILOT) ||
         IsSpec(resolution, SHADOW_VOLUME_GUNNER);
}
