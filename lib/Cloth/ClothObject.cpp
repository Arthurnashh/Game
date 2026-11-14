// ClothObject.cpp: implementation of the CClothObject class.
//
//////////////////////////////////////////////////////////////////////
#include "../wpch.hpp"
#include <El/ParamFile/paramFile.hpp>
#include "../vehicle.hpp"
#include "ClothObject.h"
#include <El/Common/perfProf.hpp>

//#include <stdio.h>
#include <Es/Containers/staticArray.hpp>
//#include <windows.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ClothObject::ClothObject()
{
}

ClothObject::~ClothObject()
{
	_knots.Clear();
}

void ClothObjectType::Init(ParamEntryPar config)
{
	_maxStep = config>>"stepSize";
  
	_stretchCoef = config>>"stretchCoef";
	_fricCoef = config>>"fricCoef";
	_windCoef = config>>"windCoef";
	_gravCoef = config>>"gravCoef";

	_rowPoints = config>>"rowPoints";
	_colPoints = config>>"colPoints";
	
  _xMin = 0;
  _yMin = 0;
  _xSize = 0.5;
  _ySize = 0.5;
}

void ClothObjectType::InitShape(float xMin, float yMin, float sizeX, float sizeY)
{
	_xMin = xMin, _yMin = yMin;
	_xSize = sizeX;
	_ySize = sizeY;
}

void ClothObject::Init(const ClothObjectType &type,	Matrix4Val pos)
{
	_knots.Dim(type._colPoints,type._rowPoints);

	InitPos(type,pos,VZero);

}

void ClothObject::InitPos( const ClothObjectType &type, Matrix4Val pos, Vector3Val vel )
{
	// set initial positions
	float xGrid = type._xSize/(_knots.W()-1);
	float yGrid = type._ySize/(_knots.H()-1);
	Vector3 norm = pos.Rotate(VAside);
	for (int y=0; y<_knots.H(); y++) for (int x=0; x<_knots.W(); x++)
	{
		ClothKnot &knot = _knots(x,y);
		Vector3 relPos(x*xGrid+type._xMin,y*yGrid+type._yMin,0);
		knot._pos = pos.FastTransform(relPos);
		knot._vel = vel;
		knot._norm = norm;
	}

  _lastPos = pos;
  _lastVelocity = vel;
}

#define G_CONST 9.8066f

//#include "../global.hpp"


void ClothObject::Simulate
(
  const ClothObjectType &type,
	Matrix4Val pos, // world space position of fixed points
	Vector3Val velocity, // world space velocity of fixed points
	float time, Vector3Par wind, Vector3Par inertia,
	SimulationImportance importance
)
{
  PROFILE_SCOPE(clothsim);
	if( !EnableVisualEffects(pos.Position(),importance))
	{
		InitPos(type,pos,velocity);
		return;
	}

	// create acceleration array
	int nKnots = _knots.W()*_knots.H();
	AUTO_STATIC_ARRAY(Vector3,accel,64*64);
	accel.Reserve(nKnots);
  accel.Resize(nKnots);
	// init acceleration of all knots with gravity
	Vector3 grav(0,-G_CONST*type._gravCoef,0);

	struct Neighbourh
	{
		int dx,dy;
		float factor;
		int align;
	};
	static const Neighbourh neighbourhs[]=
	{
	  {+1, 0,1.0f},
		{ 0,+1,1.0f},
		{+1,+1,0.5f},
		{+1,-1,0.5f}
	};
	// calculate stretch forces
	const float xGrid = type._xSize/(_knots.W()-1);
	const float yGrid = type._ySize/(_knots.H()-1);
	const int nNeighbourhs = sizeof(neighbourhs)/sizeof(*neighbourhs);


  if (importance >= SimulateVisibleFar)
  {
    if (importance >= SimulateInvisibleNear)
      time = type._maxStep;
    else
      time *= 0.5f;
  }

  float wholeTime = time;
	while (time>0)
	{
    //PROFILE_SCOPE(clothsimmain);		
		float step = floatMin(time,type._maxStep);
		time -= step;

		for (int i=0; i<nKnots; i++)
		{
			accel[i] = grav;
		}

    //START_PROFILE(clothcon);    

    // interpolate constrained values
    Vector3 actVel = _lastVelocity * time + velocity * (wholeTime - time);
    actVel /= wholeTime;
    Matrix4 actPos = (_lastPos * time +  pos * (wholeTime - time)) * (1/wholeTime);
    
    // fixup contrained knots
    // note: constrained are all points with x==0

    for (int y=0; y<_knots.H(); y++)
    {
      int offset = _knots.CoordOffset(0,y);
      Vector3 relPos(0*xGrid+type._xMin,y*yGrid+type._yMin,0);
      ClothKnot &knot = _knots[offset];
      knot._pos = actPos.FastTransform(relPos);
      knot._vel = actVel;
      accel[offset] = VZero;
    }

    // keep all knots in certain limits
    // spread limits from constraints
    // now we assume all constraints are x==0 
    // main limiting distance is in x-direction

#if 1
    for (int y=0; y<_knots.H(); y++)
    {
      for (int x=1; x<_knots.W(); x++)
      {
        ClothKnot &knot = _knots(x,y);
        ClothKnot &pKnot = _knots(x-1,y);
        // fixup distance to previous knot
        Vector3 dir = knot._pos-pKnot._pos;
        float dist2 = dir.SquareSize();
        const float minDistFactor = 0.9; //  max. distance allowed
        const float maxDistFactor = 1.1; //  min. distance allowed

        if (dist2>Square(xGrid*maxDistFactor))
        {
          knot._pos = pKnot._pos + dir*(xGrid*maxDistFactor*InvSqrt(dist2));
        }
        else if (dist2<Square(xGrid*minDistFactor))
        {
          knot._pos = pKnot._pos + dir*(xGrid*minDistFactor*InvSqrt(dist2));
        }

        // fixup speed - may not be too different from speed of rod
        Vector3 dirVel = knot._vel-velocity;
        float distVel2 = dirVel.SquareSize();
        const float maxVelocity = 10; // max vel. distance allowed

        //float factor = xGrid/dist;
        if (distVel2>Square(maxVelocity))
        {
          knot._vel = velocity + dirVel*(InvSqrt(distVel2)*maxVelocity);
        }				
      }
    } // fixup/limit velocity loop
#endif 
    //END_PROFILE(clothcon);

    //START_PROFILE(clothaccel);
		for (int y=0; y<_knots.H(); y++) for (int x=0; x<_knots.W(); x++)
		{
			int offset = _knots.CoordOffset(x,y);
			ClothKnot &knot = _knots(x,y);
			// consider each of neighbourgs 
			for (int i=0; i<nNeighbourhs; i++)
			{
				int xd = neighbourhs[i].dx;
				int yd = neighbourhs[i].dy;
				int xn = x+xd;
				int yn = y+yd;
				if (xn<0 || xn>=_knots.W()) continue;
				if (yn<0 || yn>=_knots.H()) continue;
				// calculate regular distance
				float regDist2 = Square(xd*xGrid)+Square(yd*yGrid);
				const ClothKnot &nKnot = _knots(xn,yn);

				// calculate distance from knot
				float nFactor = neighbourhs[i].factor;
				Vector3 dir = nKnot._pos-knot._pos;

				float dist2 = dir.SquareSize();

				float regDist = regDist2*InvSqrt(regDist2);
				const float minDist = 1e-3;
				float invDist = dist2>Square(minDist) ? InvSqrt(dist2) : (1/minDist);
				float dist = dist2*invDist;

				// calculate difference between distance and regular distance
				float diff = dist-regDist;
				saturate(diff,-regDist*20,+regDist*20);
				// if dist is bigger than regDist, force is applied along dir
				// towards the nKnot knot
				// add stretch
        Vector3 force = dir*(invDist*diff*type._stretchCoef*nFactor);
				//accel[offset] += dir*(invDist*diff*_stretchCoef*nFactor);

				// calculate friction based on difference of velocity
				Vector3 velDiff = nKnot._vel-knot._vel;
        force += velDiff*(type._fricCoef*nFactor);

				accel[offset] += force;
        // action - reactin
        accel[_knots.CoordOffset(xn,yn)] -= force;        
			}

			// calculate airfriction based on velocity relative to wind speed
			// TODO: consider also surface normal here
			Vector3 airDiff = wind-knot._vel;
			if (airDiff.SquareSize()>Square(1e-3))
			{
				Vector3 airDiffDir = airDiff.Normalized();

				float windCosAngle = airDiffDir.DotProduct(knot._norm);
				accel[offset] += airDiff*(type._windCoef*fabs(windCosAngle));
			}

		}
    //END_PROFILE(clothaccel);


		// integrate forces
    for (int y=0; y<_knots.H(); y++)
    {
      for (int x=1; x<_knots.W(); x++)
      {
			  ClothKnot &knot = _knots(x,y);
			  Vector3 velDelta = step*accel[_knots.CoordOffset(x,y)];
			  Vector3 midVel = knot._vel+velDelta*0.5;
			  knot._vel += velDelta;
			  knot._pos += midVel*step;
		  }
    }

#if 1
    if (time <= 0)
    {
      // last iteration check if knots are correct
      // keep all knots in certain limits
      // spread limits from constraints
      // now we assume all constraints are x==0 
      // main limiting distance is in x-direction

      for (int y=0; y<_knots.H(); y++)
      {
        for (int x=1; x<_knots.W(); x++)
        {
          ClothKnot &knot = _knots(x,y);
          ClothKnot &pKnot = _knots(x-1,y);
          // fixup distance to previous knot
          Vector3 dir = knot._pos-pKnot._pos;
          float dist2 = dir.SquareSize();
          const float minDistFactor = 0.9; //  max. distance allowed
          const float maxDistFactor = 1.1; //  min. distance allowed

          if (dist2>Square(xGrid*maxDistFactor))
          {
            knot._pos = pKnot._pos + dir*(xGrid*maxDistFactor*InvSqrt(dist2));
          }
          else if (dist2<Square(xGrid*minDistFactor))
          {
            knot._pos = pKnot._pos + dir*(xGrid*minDistFactor*InvSqrt(dist2));
          }

          // fixup speed - may not be too different from speed of rod
          Vector3 dirVel = knot._vel-velocity;
          float distVel2 = dirVel.SquareSize();
          const float maxVelocity = 10; // max vel. distance allowed

          //float factor = xGrid/dist;
          if (distVel2>Square(maxVelocity))
          {
            knot._vel = velocity + dirVel*(InvSqrt(distVel2)*maxVelocity);
          }				
        }
      } // fixup/limit velocity loop
    }
#endif 
    //END_PROFILE(clothcon);

    //START_PROFILE(clothnorm);
		// recalcuculate normals
		for (int y=0; y<_knots.H(); y++) for (int x=0; x<_knots.W(); x++)
		{
			ClothKnot &knot = _knots(x,y);
			
      int nx1 = x - 1; 
      int nx2 = x + 1;      
      int ny1 = y + 1;
      int ny2 = y - 1;

      if (nx1<0) nx1 = 0;
      if (nx2 >= _knots.W())  nx2 = _knots.W() - 1;
      
      if (ny1>=_knots.H()) ny1 = _knots.H() - 1;
      if (ny2<0) ny2 = 0;

      // calculate normal of this particular neighbourgh
      Vector3 nxDif = _knots(nx1,y)._pos - _knots(nx2,y)._pos;
      Vector3 nyDif = _knots(x,ny1)._pos - _knots(x,ny2)._pos;

      knot._norm = nxDif.CrossProduct(nyDif);      

			// normal me be impossible to normalize
			if (knot._norm.SquareSize()>Square(0.001))
			{
				knot._norm.Normalize();
			}
			else
			{
				knot._norm = VUp;
			}
		}
    //END_PROFILE(clothnorm);
	}

  _lastPos = pos;
  _lastVelocity = velocity;
}

void ClothObject::SetKnot
(
	float x, float y,
	Vector3Par pos, Vector3Par vel, Vector3Par norm
)
{
	ClothKnot &knot = _knots.Set((int)x,(int)y);
	knot._pos = pos;
	knot._vel = vel;
}

bool ClothObject::IsConstraint(int x, int y)
{
	return x==0;
}

Vector3Val ClothObject::GetPosition( float x, float y ) const
{
	// all simulation is done in world space
	// convert float to int coordinates
	int xInt = toInt(x*(_knots.W()-1));
	int yInt = toInt(y*(_knots.H()-1));
	saturate(xInt,0,_knots.W()-1);
	saturate(yInt,0,_knots.H()-1);
	//LogF("x,y: %.3f,%.3f %d,%d",x,y,xInt,yInt);
	// we get coorid
	const ClothKnot &knot = _knots.Get(xInt,yInt);
	return knot._pos;
}

Vector3Val ClothObject::GetNormal( float x, float y ) const
{
	// all simulation is done in world space
	int xInt = toInt(x*(_knots.W()-1));
	int yInt = toInt(y*(_knots.H()-1));
	saturate(xInt,0,_knots.W()-1);
	saturate(yInt,0,_knots.H());
	// we get coorid
	const ClothKnot &knot = _knots.Get(xInt,yInt);
	return knot._norm;
}

