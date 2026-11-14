#pragma once
#include "Simple3D/UH60AnimatorBase.h"

class UH60AnimatorData;

namespace Dina3D
{
    class DrawableObjectGeo;
}

namespace osg
{
    class Group;
}

namespace osgSim
{
    class DOFTransform;
}

class SIMPLE3D_EXPORT UH60AnimatorOSG : public UH60AnimatorBase
{
public:
    UH60AnimatorOSG(Dina3D::DrawableObjectGeo*);
    virtual ~UH60AnimatorOSG();

    void    setTailGear( const smVec3r& XYZ , const smVec3r& HPR );
    void    setLeftMainGear( const smVec3r& XYZ , const smVec3r& HPR );
    void    setRightMainGear( const smVec3r& XYZ , const smVec3r& HPR );

protected:  
    UH60AnimatorData*       m_data;

    bool getDOF( const std::string& nodeName ,  osg::Group* root , osgSim::DOFTransform*& nodePtr );
};

