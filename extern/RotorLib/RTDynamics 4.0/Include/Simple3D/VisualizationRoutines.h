#pragma once


#include "FixedWingLibFDM/GenAirplane100.h"
#include "FixedWingLibCGF/SimplifiedAirplane.h"
#include "RotorLibFDM/GenHeli500.h"
#include "RotorLibFDM/GenHeli600.h"
#include "RotorLibCGF/SimplifiedRotorcraft.h"
#include "FixedWingLibCGF/AirplaneBot.h"

#include "RTD/FGGroundContact200.h"
#include "RTD/FGRotorBladeElementTheory.h"


    

#define  S3DVIS_WHITE		smVec4r(1,1,1,1)
#define  S3DVIS_GRAY		smVec4r(0.5,0.5,0.5,1)
#define  S3DVIS_RED			smVec4r(1,0,0,1)
#define  S3DVIS_DARKRED     smVec4r(0.5f,0,0,1)
#define  S3DVIS_GREEN       smVec4r(0,1,0,1)
#define  S3DVIS_DARKGREEN   smVec4r(0,0.5f,0,1)
#define  S3DVIS_BLUE        smVec4r(0.25f,0.25f,1,1)
#define  S3DVIS_DARKBLUE    smVec4r(0,0,1,1)
#define  S3DVIS_YELLOW      smVec4r(1,1,0,1)
#define  S3DVIS_DARKYELLOW  smVec4r(0.25f,0.25f,0,1)
#define  S3DVIS_PURPLE      smVec4r(1,0,1,1)
#define  S3DVIS_DARKPURPLE  smVec4r(0.25f,0,0.25f,1)





static void visualizeVectors_RotorLibFDM( RotorLibFDM::GenHeli500* heli , Simple3D* s3d )
{	
   // Visualize the forces on helicopter
    RTD::FGGroundContact200* gc0 =  dynamic_cast<RTD::FGGroundContact200*>(heli->getGroundContact(0));
    RTD::FGGroundContact200* gc1 =  dynamic_cast<RTD::FGGroundContact200*>(heli->getGroundContact(1));
    RTD::FGGroundContact200* gc2 =  dynamic_cast<RTD::FGGroundContact200*>(heli->getGroundContact(2));

    

    s3d->drawVectorBCS(  heli ,  heli->getMainRotor(), 0.0005f, S3DVIS_GREEN, S3DVIS_GREEN ) ;
    s3d->drawVectorBCS(  heli ,  heli->getTailRotor(), 0.0005f, S3DVIS_GREEN, S3DVIS_GREEN  ) ;
	
    if ( gc0 && gc0->isTouchingGround() )
    {
        s3d->drawVectorWCS(  gc0->getContactPosition()+ smVec3r(0.0f,0.0f,0.01f) ,  gc0->getInplaneForceAtContact()+ smVec3r(0.0f,0.0f,0.01f)  , 0.0005f , S3DVIS_RED, S3DVIS_RED ) ;
        s3d->drawVectorWCS(  gc0->getContactPosition(),  gc0->getNormalForceAtContact()  , 0.0005f , S3DVIS_BLUE, S3DVIS_BLUE ) ;
    }
    else
    {
        s3d->drawVectorWCS(  smVec3r(0.0f,0.0f,0.0f), smVec3r(0.0f,0.0f,0.0f)  , 0.0005f , S3DVIS_RED, S3DVIS_RED ) ;
        s3d->drawVectorWCS(  smVec3r(0.0f,0.0f,0.0f), smVec3r(0.0f,0.0f,0.0f)  , 0.0005f , S3DVIS_RED, S3DVIS_RED ) ;
    }


    if ( gc1 && gc1->isTouchingGround() )
    {
        s3d->drawVectorWCS(  gc1->getContactPosition()+ smVec3r(0.0f,0.0f,0.01f) ,  gc1->getInplaneForceAtContact()+ smVec3r(0.0f,0.0f,0.01f)  , 0.0005f , S3DVIS_RED, S3DVIS_RED ) ;
        s3d->drawVectorWCS(  gc1->getContactPosition(),  gc1->getNormalForceAtContact()  , 0.0005f , S3DVIS_BLUE, S3DVIS_BLUE ) ;
    }
    else
    {
        s3d->drawVectorWCS(  smVec3r(0.0f,0.0f,0.0f), smVec3r(0.0f,0.0f,0.0f)  , 0.0005f , S3DVIS_RED, S3DVIS_RED ) ;
        s3d->drawVectorWCS(  smVec3r(0.0f,0.0f,0.0f), smVec3r(0.0f,0.0f,0.0f)  , 0.0005f , S3DVIS_RED, S3DVIS_RED ) ;
    }

    if ( gc2 && gc2->isTouchingGround() )
    {
        s3d->drawVectorWCS(  gc2->getContactPosition()+ smVec3r(0.0f,0.0f,0.01f) ,  gc2->getInplaneForceAtContact()+ smVec3r(0.0f,0.0f,0.01f)  , 0.0005f  , S3DVIS_RED, S3DVIS_RED ) ;
        s3d->drawVectorWCS(  gc2->getContactPosition(),  gc2->getNormalForceAtContact()  , 0.0005f  , S3DVIS_BLUE, S3DVIS_BLUE ) ;
    }
    else
    {
        s3d->drawVectorWCS(  smVec3r(0.0f,0.0f,0.0f), smVec3r(0.0f,0.0f,0.0f)  , 0.0005f , S3DVIS_RED, S3DVIS_RED ) ;
        s3d->drawVectorWCS(  smVec3r(0.0f,0.0f,0.0f), smVec3r(0.0f,0.0f,0.0f)  , 0.0005f , S3DVIS_RED, S3DVIS_RED ) ;
    }

    if ( heli->getFuselage() )
    {
        s3d->drawVectorBCS( heli , heli->getFuselage()->getConfigParams().m_centerOfPressure, heli->getFuselage()->getLift(),0.0005f, S3DVIS_GREEN, S3DVIS_GREEN ) ;
        s3d->drawVectorBCS( heli, heli->getFuselage()->getConfigParams().m_centerOfPressure,heli->getFuselage()->getDrag(),0.0005f, S3DVIS_RED, S3DVIS_RED ) ;
        s3d->drawVectorBCS( heli, heli->getFuselage()->getConfigParams().m_centerOfPressure,heli->getFuselage()->getSideforce(),0.0005f, S3DVIS_BLUE, S3DVIS_BLUE ) ;
    }

    if ( heli->getHorizontalStabilizerRight() )
    {
        RTD::FGWing* stabilizer = heli->getHorizontalStabilizerRight();

        s3d->drawVectorBCS( heli,  stabilizer->getConfigParams()->position, stabilizer->getCurrentLift(),0.0005f, S3DVIS_GREEN, S3DVIS_GREEN ) ;
        s3d->drawVectorBCS( heli,  stabilizer->getConfigParams()->position, stabilizer->getCurrentDrag(), 0.0005f, S3DVIS_RED, S3DVIS_RED ) ;
    }

    if ( heli->getHorizontalStabilizerLeft() )
    {
        RTD::FGWing* stabilizer = heli->getHorizontalStabilizerLeft();

        s3d->drawVectorBCS( heli,  stabilizer->getConfigParams()->position,stabilizer->getCurrentLift(),0.0005f, S3DVIS_GREEN, S3DVIS_GREEN ) ;
        s3d->drawVectorBCS( heli,  stabilizer->getConfigParams()->position,stabilizer->getCurrentDrag(),0.0005f, S3DVIS_RED, S3DVIS_RED ) ;
    }

    if ( heli->getVerticalStabilizer() )
    {
        RTD::FGWing* stabilizer = heli->getVerticalStabilizer();

        s3d->drawVectorBCS( heli,  stabilizer->getConfigParams()->position,stabilizer->getCurrentLift(),0.0005f, S3DVIS_GREEN, S3DVIS_GREEN ) ;
        s3d->drawVectorBCS( heli,  stabilizer->getConfigParams()->position,stabilizer->getCurrentDrag(),0.0005f, S3DVIS_RED, S3DVIS_RED ) ;
    }

}

static void visualizeVectors_RotorLibFDM( RotorLibFDM::GenHeli600* heli , Simple3D* s3d )
{	

	// Render the actual debugging primitives of the vehicle
	for ( unsigned int vecNo = 0 ; vecNo < heli->getVisualization()->getVectorQueue().size() ; vecNo ++ )
	{
		RTD::Visualization::VisualizationVector visvec = heli->getVisualization()->getVectorQueue()[vecNo];
		s3d->drawVectorWCS( visvec.m_position , visvec.m_vector  , 1.0f , visvec.m_startColor , visvec.m_endColor );
	}


	for ( unsigned int i = 0 ; i < heli->getNumGroundContacts() ; i++ )
	{
		RTD::FGGroundContact200* gc =  dynamic_cast<RTD::FGGroundContact200*>(heli->getGroundContact(i));	

		
		
		if ( gc )
		{
			s3d->drawVectorWCS(  gc->getWheelPositionWCS() ,  gc->getForce()  , 0.0005f, S3DVIS_RED, S3DVIS_RED ) ;
		}

	    if ( gc && gc->isTouchingGround() )
		{
			s3d->drawVectorWCS(  gc->getContactPosition()+ smVec3r(0.0f,0.0f,0.01f) ,  gc->getInplaneForceAtContact()+ smVec3r(0.0f,0.0f,0.01f)  , 0.0005f , S3DVIS_RED, S3DVIS_RED ) ;
			s3d->drawVectorWCS(  gc->getContactPosition(),  gc->getNormalForceAtContact()  , 0.0005f , S3DVIS_BLUE, S3DVIS_BLUE ) ;
		}
		else
		{
			s3d->drawVectorWCS(  smVec3r(0.0f,0.0f,0.0f), smVec3r(0.0f,0.0f,0.0f)  , 0.0005f , S3DVIS_RED, S3DVIS_RED ) ;
			s3d->drawVectorWCS(  smVec3r(0.0f,0.0f,0.0f), smVec3r(0.0f,0.0f,0.0f)  , 0.0005f , S3DVIS_RED, S3DVIS_RED ) ;
		}
	}
       
    s3d->drawVectorBCS(  heli ,  heli->getMainRotor(), 0.0005f, S3DVIS_GREEN, S3DVIS_GREEN ) ;
    s3d->drawVectorBCS(  heli ,  heli->getTailRotor(), 0.0005f, S3DVIS_GREEN, S3DVIS_GREEN  ) ;

    if ( heli->getFuselage() )
    {
        s3d->drawVectorBCS( heli , heli->getFuselage()->getConfigParams().m_centerOfPressure, heli->getFuselage()->getLift(),0.0005f, S3DVIS_GREEN, S3DVIS_GREEN ) ;
        s3d->drawVectorBCS( heli, heli->getFuselage()->getConfigParams().m_centerOfPressure,heli->getFuselage()->getDrag(),0.0005f, S3DVIS_RED, S3DVIS_RED ) ;
        s3d->drawVectorBCS( heli, heli->getFuselage()->getConfigParams().m_centerOfPressure,heli->getFuselage()->getSideforce(),0.0005f, S3DVIS_BLUE, S3DVIS_BLUE ) ;
    }

	for ( unsigned int i = 0 ; i < heli->getNumStabilizers() ; i++ )
	{
        RTD::FGWing* stabilizer = heli->getStabilizer(i);

        s3d->drawVectorBCS( heli,  stabilizer->getConfigParams()->position, stabilizer->getCurrentLift(),0.0005f, S3DVIS_GREEN, S3DVIS_GREEN ) ;
        s3d->drawVectorBCS( heli,  stabilizer->getConfigParams()->position, stabilizer->getCurrentDrag(), 0.0005f, S3DVIS_RED, S3DVIS_RED ) ;
	}

    std::vector<RTD::FGRotorBladeElementTheory*> betRotors;

    RTD::FGRotorBladeElementTheory* betRotor = NULL;

    betRotor = dynamic_cast<RTD::FGRotorBladeElementTheory*>(heli->getMainRotor());
	if (  betRotor )
    {
        betRotors.push_back( betRotor );
    }

    betRotor = dynamic_cast<RTD::FGRotorBladeElementTheory*>(heli->getTailRotor());
	if (  betRotor )
    {
        betRotors.push_back( betRotor );
    }

	for ( size_t i = 0 ; i < betRotors.size() ; i++ )
    {
		std::vector<RTD::FGRotorBladeElementTheory::VisualizationData> vizData;

		betRotors[i]->generateVisualizationData( vizData );

		for ( unsigned int vectorIndex = 0; vectorIndex <  vizData.size() ; vectorIndex++ )
		{
			s3d->drawVectorBCS( heli,  vizData[vectorIndex].m_pos , vizData[vectorIndex].m_vec ,1.0f, vizData[vectorIndex].m_color, vizData[vectorIndex].m_color ) ;
		}
	}

}



static void visualizeVectors_FixedWingLibCGF( FixedWingLibCGF::SimplifiedAirplane* airplane ,FixedWingLibCGF::SimplifiedAirplane* secondAirplane , FixedWingLibCGF::AirplaneBot* firstBot , Simple3D* s3d )
{
	// Visualize the forces on the airplane
    RTD::FGGroundContact200* gc0 =  dynamic_cast<RTD::FGGroundContact200*>(airplane->getGroundContact(0));
    RTD::FGGroundContact200* gc1 =  dynamic_cast<RTD::FGGroundContact200*>(airplane->getGroundContact(1));
    RTD::FGGroundContact200* gc2 =  dynamic_cast<RTD::FGGroundContact200*>(airplane->getGroundContact(2));





    if ( gc0 && gc0->isTouchingGround() )
    {
        s3d->drawVectorWCS( gc0->getContactPosition()+ smVec3r(0.0f,0.0f,0.01f) ,  gc0->getInplaneForceAtContact()+ smVec3r(0.0f,0.0f,0.01f)  , 0.00005f , S3DVIS_RED, S3DVIS_RED ) ;
        s3d->drawVectorWCS( gc0->getContactPosition(),  gc0->getNormalForceAtContact()  , 0.00005f , S3DVIS_BLUE, S3DVIS_BLUE ) ;
    }
    else
    {
        s3d->drawVectorWCS( smVec3r(0.0f,0.0f,0.0f), smVec3r(0.0f,0.0f,0.0f)  , 0.00005f , S3DVIS_RED, S3DVIS_RED ) ;
        s3d->drawVectorWCS( smVec3r(0.0f,0.0f,0.0f), smVec3r(0.0f,0.0f,0.0f)  , 0.00005f , S3DVIS_RED, S3DVIS_RED ) ;
    }


    if ( gc1 && gc1->isTouchingGround() )
    {
        s3d->drawVectorWCS(  gc1->getContactPosition()+ smVec3r(0.0f,0.0f,0.01f) ,  gc1->getInplaneForceAtContact()+ smVec3r(0.0f,0.0f,0.01f)  , 0.00005f , S3DVIS_RED, S3DVIS_RED ) ;
        s3d->drawVectorWCS(  gc1->getContactPosition(),  gc1->getNormalForceAtContact()  , 0.00005f , S3DVIS_BLUE, S3DVIS_BLUE ) ;
    }
    else
    {
        s3d->drawVectorWCS(  smVec3r(0.0f,0.0f,0.0f), smVec3r(0.0f,0.0f,0.0f)  , 0.00005f , S3DVIS_RED, S3DVIS_RED ) ;
        s3d->drawVectorWCS(  smVec3r(0.0f,0.0f,0.0f), smVec3r(0.0f,0.0f,0.0f)  , 0.00005f , S3DVIS_RED, S3DVIS_RED ) ;
    }

    if ( gc2 && gc2->isTouchingGround() )
    {
        s3d->drawVectorWCS(  gc2->getContactPosition()+ smVec3r(0.0f,0.0f,0.01f) ,  gc2->getInplaneForceAtContact()+ smVec3r(0.0f,0.0f,0.01f)  , 0.00005f  , S3DVIS_RED, S3DVIS_RED ) ;
        s3d->drawVectorWCS(  gc2->getContactPosition(),  gc2->getNormalForceAtContact()  , 0.00005f  , S3DVIS_BLUE, S3DVIS_BLUE ) ;
    }
    else
    {
        s3d->drawVectorWCS(  smVec3r(0.0f,0.0f,0.0f), smVec3r(0.0f,0.0f,0.0f)  , 0.00005f , S3DVIS_RED, S3DVIS_RED ) ;
        s3d->drawVectorWCS(  smVec3r(0.0f,0.0f,0.0f), smVec3r(0.0f,0.0f,0.0f)  , 0.00005f , S3DVIS_RED, S3DVIS_RED ) ;
    }


    s3d->drawVectorBCS( airplane, smVec3r(0.0f,0.0f,0.0f) , airplane->getLift(),0.00005f, S3DVIS_GREEN, S3DVIS_GREEN ) ;
    s3d->drawVectorBCS( airplane, smVec3r(0.0f,0.0f,0.0f) , airplane->getDrag(),0.00005f, S3DVIS_RED, S3DVIS_RED ) ;
    s3d->drawVectorBCS( airplane, smVec3r(0.0f,0.0f,0.0f) , airplane->getThrust(),0.00005f, S3DVIS_DARKBLUE, S3DVIS_DARKBLUE ) ;
	s3d->drawVectorBCS( airplane, smVec3r(0.0f,0.0f,0.0f) , airplane->getSideforce(),0.00005f, S3DVIS_BLUE, S3DVIS_BLUE ) ;
    s3d->drawVectorWCS( airplane->getPosition() , airplane->getWorld()->getGravity() * (smReal) airplane->getTotalMass(),0.00005f, S3DVIS_DARKRED, S3DVIS_DARKRED ) ;
    s3d->drawVectorWCS( airplane->getPosition() , airplane->getVelocity() ,0.1f, S3DVIS_WHITE, S3DVIS_WHITE ) ;


    FixedWingLibCGF::VelocityController* avc = firstBot->getVelocityController();
	if (avc->wasUpdatedPreviousFrame())
	{
		s3d->drawVectorWCS( airplane->getPosition() + smVec3r(0.0f ,0.0f , 0.1f), firstBot->getVelocityController()->m_dynParams.m_desiredVelocity ,0.1f, S3DVIS_GRAY, S3DVIS_GRAY ) ;
		s3d->drawVectorWCS( airplane->getPosition() + smVec3r(0.05f ,0.05f , 0.0f) , avc->getDesiredLift() ,0.00005f, S3DVIS_YELLOW, S3DVIS_YELLOW ) ;
	}
	else
	{
		s3d->drawVectorWCS( airplane->getPosition() , smVec3r(0.0f,0.0f,0.0f)  ,0.1f, S3DVIS_GRAY, S3DVIS_GRAY ) ;
		s3d->drawVectorWCS( airplane->getPosition()  , smVec3r(0.0f,0.0f,0.0f) ,0.00005f, S3DVIS_YELLOW, S3DVIS_YELLOW ) ;

	}



    // A vector which points to the second airplane. To avoid visual clutter it is
    // drawn with an offset from the A/C center.
    if ( secondAirplane )
    {
        smVec3r Pac =  airplane->getPosition();
        smVec3r Ptarget =  secondAirplane->getPosition();

        smVec3r Rac_to_target = (Ptarget-Pac) ;
        smVec3r Rac_to_targetNorm = Rac_to_target ;
        Rac_to_targetNorm.normalize();

        smReal trim = 15.0f ;
        smVec3r offset = Rac_to_targetNorm * trim;

        if ( Rac_to_target.length() < (trim*2.0f) )
        {
            offset = smVec3r(0,0,0);
        }

        smReal scale = (1.0f - 2.0f * trim / Rac_to_target.length() ) ;
#ifndef SCREENSHOT_MODE
        s3d->drawVectorWCS(  Pac + offset, Rac_to_target, scale, S3DVIS_RED, S3DVIS_BLUE ) ;
#endif
    }
	else
	{
		s3d->drawVectorWCS(  smVec3r(0.0f,0.0f,0.0f), smVec3r(0.0f,0.0f,0.0f), 1, S3DVIS_RED, S3DVIS_BLUE ) ;
	}
}