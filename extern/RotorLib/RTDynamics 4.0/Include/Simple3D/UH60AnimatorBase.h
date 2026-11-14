#pragma once
#include "Simple3D/Common.h"
#include "Physics/smVecTypes.h"

#include <string>
            
class SIMPLE3D_EXPORT UH60AnimatorBase
{
public:
	UH60AnimatorBase(){}
	virtual ~UH60AnimatorBase(){}
	
	virtual void	setTailGear( const smVec3r& XYZ , const smVec3r& HPR ){}
	virtual void	setLeftMainGear( const smVec3r& XYZ , const smVec3r& HPR ){}
	virtual void	setRightMainGear( const smVec3r& XYZ , const smVec3r& HPR ){}
};

