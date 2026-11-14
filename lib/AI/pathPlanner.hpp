// pathPlanner.hpp: interface for the pathPlanner class.
//
//////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PATHPLANNER_HPP
#define _PATHPLANNER_HPP

#include "aiTypes.hpp"
#include "../vehicleAI.hpp"

#define GET_UNACCESSIBLE				1e20F
#define SET_UNACCESSIBLE				1e30F

typedef int AITime;

//enum Rank;

class AI : public SerializedObject
{
protected:
	NetworkId _networkId;
	bool _local;

public:
	enum Semaphore
	{
		SemaphoreBlue,
		SemaphoreGreen,
		SemaphoreWhite,
		SemaphoreYellow,
		SemaphoreRed,
		NSemaphores
	};
	enum Answer
	{
		NoAnswer					= -1,
// Unit => Subgroup (answer to GoTo)
		UnitDestroyed = 0,
		HealthCritical,
		DamageCritical,
		FuelCritical,
		ReportPosition,
		ReportSemaphore,
		AmmoCritical,
		FuelLow,
		AmmoLow,
		IsLeader,
// Subgroup => Group (answer to Command)
		CommandCompleted	= 0x100,
		CommandFailed,
		SubgroupDestinationUnreacheable,
// Group => Center (answer to Mission)
		MissionCompleted	= 0x200,
		MissionFailed,
		WorkCompleted,
		WorkFailed,
		DestinationUnreacheable,
		GroupDestroyed
	};
	enum ThinkImportance
	{
		LevelOperative,
		LevelFastOperative,
		LevelStrategic,
		LevelCommands
	};

public:
	AI() {_local = true;}
	virtual ~AI(){}
	static AITime GetActualTime();
	static int CalcDirection(Vector3Par direction);
  static int DirectionAngleDifference(int dirD);
	static float ExpForRank(Rank rank, bool ingame = false);
	static Rank RankFromExp(float exp, bool ingame = false);
	static void InitTables();

	NetworkId GetNetworkId() const {return _networkId;}
	void SetNetworkId(NetworkId &id) {_networkId = id;}
	bool IsLocal() const {return _local;}
	void SetLocal(bool local = true) {_local = local;}
};

DEFINE_ENUM_BEG(UnitPosition)
	UPUp,UPMiddle,UPDown,UPAuto,NUnitPositions
DEFINE_ENUM_END(UnitPosition)

#if _ENABLE_AI

struct FieldPassing
{
	enum Mode
	{
		Move,
		MoveOnRoad,
	};

	Mode _mode;
  Vector3 _pos;
	float _cost;

	LSError Serialize(ParamArchive &ar);
};

TypeIsSimple(FieldPassing);

#include <Es/Memory/normalNew.hpp>
class PathTreeNode : public RefCount 
{
public:
	float _cost;
	float _heur;
	PathTreeNode* _left;
	PathTreeNode* _right;
	PathTreeNode* _parent;
	Ref<PathTreeNode> _next;
	int _depth;
	WORD _x;
	WORD _z;
	BYTE _mode;
	BYTE _direction;
	bool _open;

	PathTreeNode(WORD x, WORD z, BYTE mode, BYTE direction,
		PathTreeNode* left, PathTreeNode* right, PathTreeNode* parent,
		float cost, float heur, int depth)
	{
		_x = x;
		_z = z;
		_mode = mode;
		_direction = direction;
		_cost = cost;
		_left = left;
		_right = right;
		_parent = parent;
		_heur = heur;
		_depth = depth;
		_open = true;
		_next = NULL;
	}
	~PathTreeNode()
	{
		if (_next) 
			_next = NULL;
	}

	USE_FAST_ALLOCATOR;
};
#include <Es/Memory/debugNew.hpp>

typedef float (*CostFunction)(int x, int z, bool road, bool includeGradient, float gradient, void *param);


/// strategic path finding interface
class IAIPathPlanner : public SerializeClass
{
public:
	virtual ~IAIPathPlanner(){}

	virtual void Init() = 0 ;

	virtual bool IsSearching() const = 0;

	virtual int GetPlanSize() const = 0;
	virtual float GetTotalCost() const =0;

	virtual int FindBestIndex(Vector3Par pos) const = 0;
	virtual bool GetPlanPosition(int index, PlanPosition &pos) const = 0;
	virtual FieldPassing::Mode GetPlanMode(int index) const = 0;
	virtual bool IsOnPath(int x, int z, int from, int to) const = 0;

	virtual bool StartSearching(AI::ThinkImportance prec, EntityAI *veh, Vector3Par ptStart, const PlanPosition &ptEnd) = 0;
	virtual void StopSearching() = 0;
	virtual bool ProcessSearching() = 0;

  // functions for diagnostics
  virtual Vector3Val GetDestination() const = 0;
  virtual Vector3 GetBest() const = 0;
  virtual void GetOpenList(AutoArray<Vector3> &list) const = 0;
  virtual void GetClosedList(AutoArray<Vector3> &list) const = 0;
};

IAIPathPlanner *CreateAIPathPlanner(CostFunction func, void *param);


#endif // _ENABLE_AI

#endif
