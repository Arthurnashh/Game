#pragma once
#include <El/Math/math3d.hpp>
#include <El/Evaluator/expressImpl.hpp>
#include <Common/enf_mathlib.h>

/**@enum      GeneratorParamTypes
 * @brief     list of known spawn-related configuration parameters for generator of spawn points
 **/
enum GeneratorParamTypes
{
  GPT_gridDensity,
  GPT_gridWidth,
  GPT_gridHeight,
  GPT_minDist2Water,
  GPT_maxDist2Water,
  GPT_minDist2Static,
  GPT_maxDist2Static,
  GPT_minSteepness,
  GPT_maxSteepness,

  e_max_generatorparamstypes
};
/**@enum      GeneratorParams
 * @brief     spawn point generator configuration
 **/
struct GeneratorParams
{
  float _params[e_max_generatorparamstypes]; /// parameters indexed by GeneratorParamTypes

  GeneratorParams ()
  {
    float const PI = 3.1415926535897323f;
    _params[GPT_gridDensity]      = 1 / 5.0f;
    _params[GPT_gridWidth]        =  30.0f;
    _params[GPT_gridHeight]       =  30.0f;
    _params[GPT_minDist2Water]    =   1.0f;
    _params[GPT_maxDist2Water]    =  40.0f;
    _params[GPT_minDist2Static]   =   3.0f;
    _params[GPT_maxDist2Static]   =  10.0f;
    _params[GPT_minSteepness]     = -PI/4.0f;
    _params[GPT_maxSteepness]     = +PI/4.0f;
  }
};
/**@fn      GenerateSpawnPoints
 * @brief   scans area with sample points and scans these points if they are available for spawn
 *
 * In pass1 FindSpawnPoint will scan samples from the areas determined by oper[0] and exclude obvious bad
 * spawn points like 'too close to zombie' or 'inside structure'. Performed tests are in enum @SpawnPass1.
 *
 * In pass2 FindSpawnPoint attempts to rate the remaining points according to criteria in enum @SpawnPass2.
 *
 * Next it sorts the points from pass2 and picks one randomly.
 *
 * @param[in]   oper1   Array of two arrays, oper[0] contains area coordinates, oper[1] array
 *          contains GeneratorParams. List of the spawn parameters is determined by enum @GeneratorParamTypes.
 *
 * @return  nothing
 **/
GameValue GenerateSpawnPoints (const GameState * state, GameValuePar oper);


/**@enum      SpawnPass3
 * @brief     run time phase checks
 **/
enum SpawnPass3
{
  SP3_Dist2Zombie,        /// rate distance to closest zombie
  SP3_Dist2Player,        /// rate distance to closest player
  SP3_Dist2Static,        /// rate distance to dynamic geometry (lamps etc)

  e_max_spawnpass3
};
/**@enum      SpawnPass3ParamTypes
 * @brief     list of known spawn-related configuration parameters for pass3
 **/
enum SpawnPass3ParamTypes
{
  S3T_minDist2Zombie,
  S3T_maxDist2Zombie,
  S3T_minDist2Player,
  S3T_maxDist2Player,
  S3T_minDist2Static,
  S3T_maxDist2Static,

  e_max_spawnpass3paramstypes
};
struct SpawnPass3Params
{
  float _params[e_max_spawnpass3paramstypes]; /// parameters indexed by SpawnPass3ParamTypes
  float _pass3Weights[e_max_spawnpass3]; /// weights for pass3

  SpawnPass3Params ()
  {
    _params[S3T_minDist2Zombie]   =  30.0f;
    _params[S3T_maxDist2Zombie]   =  70.0f;
    _params[S3T_minDist2Player]   =  30.0f;
    _params[S3T_maxDist2Player]   = 100.0f;
    _params[S3T_minDist2Static]   =   1.0f;
    _params[S3T_maxDist2Static]   =  10.0f;

    for (size_t i = 0; i < e_max_spawnpass3; ++i)
      _pass3Weights[i] = 1.0f;

    _pass3Weights[SP3_Dist2Zombie] = 2.0f;
    _pass3Weights[SP3_Dist2Player] = 2.0f;
  }
};
/**@fn      FindCachedSpawnPoint
 * @brief   pick one spawn point from saved set
 *
 * In pass3 FindCachedSpawnPoint will scan points for dynamic obstacles like zombies or players
 * Next it sorts the points from pass3 and picks one randomly from the set of best samples.
 *
 * @param[in]   oper1   Array of two arrays, oper[0] contains filename, oper[1] array
 *          contains SpawnPass3Params. List of the spawn parameters is determined by enum @SpawnPass3ParamTypes.
 *
 * @return  array of coordinates for a chosen spawn point.
 **/
GameValue FindCachedSpawnPoint (const GameState * state, GameValuePar oper);



/**@enum      SpawnParamNPCTypes
 * @brief     list of known spawn-related configuration parameters for NPC spawn
 **/
enum SpawnParamNPCTypes
{
  SPN_gridDensity,
  SPN_gridWidth,
  SPN_gridHeight,
  SPN_minDist2Water,
  SPN_maxDist2Water,
  SPN_minDist2Static,
  SPN_maxDist2Static,
  SPN_minSteepness,
  SPN_maxSteepness,
  SPN_minDist2Zombie,
  SPN_maxDist2Zombie,
  SPN_minDist2Player,
  SPN_maxDist2Player,

  e_max_spawnparamsnpctypes
};
/**@enum      SpawnParamNPC
 * @brief     spawn configuration
 **/
struct SpawnParamsNPC
{
  float _params[e_max_spawnparamsnpctypes]; /// parameters indexed by SpawnParamTypesNPC
  //float _pass2Weights[e_max_spawnpass2]; /// weights for pass2
  //float _pass3Weights[e_max_spawnpass3]; /// weights for pass3

  SpawnParamsNPC ()
  {
    float const PI = 3.1415926535897323f;
    _params[SPN_gridDensity]      = 1 / 5.0f;
    _params[SPN_gridWidth]        =  30.0f;
    _params[SPN_gridHeight]       =  30.0f;
    _params[SPN_minDist2Water]    =  10.0f;
    _params[SPN_maxDist2Water]    =  40.0f;
    _params[SPN_minDist2Static]   =   3.0f;
    _params[SPN_maxDist2Static]   =  10.0f;
    _params[SPN_minSteepness]     = -PI/4.0f;
    _params[SPN_maxSteepness]     = +PI/4.0f;
    _params[SPN_minDist2Zombie]   = 100.0f;
    _params[SPN_maxDist2Zombie]   = 250.0f;
    _params[SPN_minDist2Player]   = 100.0f;
    _params[SPN_maxDist2Player]   = 250.0f;

    /*for (size_t i = 0; i < e_max_spawnpass2; ++i)
      _pass2Weights[i] = 1.0f;
    for (size_t i = 0; i < e_max_spawnpass3; ++i)
      _pass3Weights[i] = 1.0f;
    _pass3Weights[SP3_Dist2Zombie] = 2.0f;
    _pass3Weights[SP3_Dist2Player] = 2.0f;*/
  }
};
/**@fn      RespawnDeadNPC
 * @brief   pick one spawn point in the vicinity of death place
 *
 * @param[in]   oper1   Array of two arrays, oper[0] contains death position, oper[1] array
 *          contains SpawnParams. List of the spawn parameters is determined by enum @SpawnParamTypesNPC.
 *
 * @return  array of coordinates for a chosen spawn point.
 **/
GameValue RespawnDeadNPC (const GameState * state, GameValuePar oper);



/////// unused stuff (for now)
//GameValue FindSpawnPoint (const GameState * state, GameValuePar oper);

/**@fn      generateExcludedGeo
 * @brief   scans area for static geometry
 *
 * Next it sorts the points from pass2 and picks one randomly.
 *
 * @param[in]   oper1   Array of two arrays, oper[0] contains area coordinates, oper[1] array
 *          contains filename so far.
 *
 * @return  nothing
 **/
//
//GameValue GenerateExcludedGeo (const GameState * state, GameValuePar oper);

