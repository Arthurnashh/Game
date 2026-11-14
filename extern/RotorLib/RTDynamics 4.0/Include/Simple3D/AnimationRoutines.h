#pragma once


#include "FixedWingLibFDM/GenAirplane100.h"
#include "FixedWingLibCGF/SimplifiedAirplane.h"
#include "RotorLibFDM/GenHeli500.h"
#include "RotorLibCGF/SimplifiedRotorcraft.h"

// This function is used to animate a 3D A310 model using data produced by FixedWingLibFDM::GenAirplane100 class.
// AirplaneAnimator class is a wrapper around underlying graphics API. In your applications
// you will need to translate methods such as setNoseGear to corresponding graphics
// API calls ( e.g. for OpenSceneGraph you have to set the transformation matrix of the Transform
// Node )
static void updateA310Animation_FDM( FixedWingLibFDM::GenAirplane100* theAirplane , float dt , Simple3D* s3d )
{
	int vehIndex = s3d->getVehicleIndex( theAirplane );
	if ( vehIndex == -1 ) return;

	A310AnimatorBase* a310Animator = s3d->getA310Animator(vehIndex);

	// Get pointers to the landing gears.
	RTD::FGGroundContactBase* mainGear1 = dynamic_cast<RTD::FGGroundContactBase*>(theAirplane->getGroundContact(0));
	RTD::FGGroundContactBase* mainGear2 = dynamic_cast<RTD::FGGroundContactBase*>(theAirplane->getGroundContact(1));
	RTD::FGGroundContactBase* noseGear  = dynamic_cast<RTD::FGGroundContactBase*>(theAirplane->getGroundContact(2));


	// FixedWingLib computes all of the necessary information required to animate landing gears and presents the data with
	// getWheelTransOffsetToRestPosition() and getWheelRotOffsetToRestPosition() methods. These methods deliver the position
	// and orientation of the wheels relative to their default positions. Provided information is sufficient for suspension
	// compression/decompression, wheel rotation and direction change of the steering wheel.
	// In the following lines setNoseGear, setLeftMainGear and setRightMainGear get the animation information from FWL and
	// make required 3D engine calls to animate the 3D model. In your application you will need to do this model animation task
	// yourself.
	a310Animator->setNoseGear(		noseGear->getWheelTransOffsetToRestPosition()	,	noseGear->getWheelRotOffsetToRestPosition() );
	a310Animator->setLeftMainGear(	mainGear1->getWheelTransOffsetToRestPosition()	,	mainGear1->getWheelRotOffsetToRestPosition() );
	a310Animator->setRightMainGear(	mainGear2->getWheelTransOffsetToRestPosition()	,	mainGear2->getWheelRotOffsetToRestPosition() );
	

	// Here extending/retracting gears is animated. The landing gears and hatch doors have joints.
	// set...GearStatus() receives a value between 0 and 1 which determines how much extended/retracted
	// is the gear. This value is then used to set the joints in the underlying graphics engine.
	a310Animator->setSteeringGearStatus( theAirplane->getGearStatus() );
	a310Animator->setLeftMainGearStatus( theAirplane->getGearStatus() );
	a310Animator->setRightMainGearStatus( theAirplane->getGearStatus() );

	// Animation of the ailerons.
	// set...AileronDeflection makes necessary graphics engine calls to rotate the ailerons.
	{
		float leftAileronAngle = theAirplane->getLeftMainWing()->getControlSurfaceDeflection();
		a310Animator->setLeftAileronDeflection( leftAileronAngle   );

		float rightAileronAngle = theAirplane->getRightMainWing()->getControlSurfaceDeflection();
		a310Animator->setRightAileronDeflection( rightAileronAngle   );
	}

	// Animation of the elevators
	{
		float leftElevatorAngle = theAirplane->getLeftTailWing()->getControlSurfaceDeflection();
		a310Animator->setLeftElevatorDeflection(leftElevatorAngle);

		float rightElevatorAngle =theAirplane->getRightTailWing()->getControlSurfaceDeflection();
		a310Animator->setRightElevatorDeflection( rightElevatorAngle );
	}

	// Rudder animation 
	{
		float rudderAngle = theAirplane->getVerticalTailWing()->getControlSurfaceDeflection();
		
		a310Animator->setRudderDeflection( rudderAngle );
	}

	// Similar to landing gear animation, flaps also require movement of multiple parts
	// and a complex animation.
	// The Setting of the flaps is taken using get*FlapSetting() methods. They return a floating
	// point number which describes which flap configuration is active or if flaps are moving
	// the how much the transition is complete. For example A value of 1.2 means flaps are between 
	// configuration 1 and 2. The transition from 1 to 2 is 20% (or 2 to 1 is 80%). 
	// This value is fed to our animator class which sets joints of the 3D model accordingly.
	a310Animator->setRightFlapSetting( theAirplane->getRightFlapSetting() );
	a310Animator->setLeftFlapSetting( theAirplane->getLeftFlapSetting() );

	// Spoiler animation is similar to flaps. We use the getSpoilerSetting() method which
	// gives us a value between 0 and 1. 0 corresponds to completely retracted spoilers. 1 means
	// that the spoilers are fully deployed.
	a310Animator->setSpoilerStatus( theAirplane->getSpoilerSetting() );
	
	// In this block jet engine rotors are animated. The speed of rotation is found by multiplying
	// the thrust percentage with a user defined scale. Since even rotors of idle engines are rotating
	// we give a minimum rotational speed. This ensures that even if thrust is zero rotor rotates.
	// Multiplying these with delta time we find how much we have to rotate the rotors.
	{
		float rotationScale = 0.03f;
		float minSpeed = 30.0f;
		float angle;

		angle = a310Animator->getLeftEngineRotorAngle();
		angle += theAirplane->getEngine(0)->getForce() * rotationScale * dt;
		angle += minSpeed * dt;
		a310Animator->setLeftEngineRotorAngle( angle );

		angle = a310Animator->getRightEngineRotorAngle();
		angle += theAirplane->getEngine(1)->getForce() * rotationScale * dt;
		angle += minSpeed * dt;
		a310Animator->setRightEngineRotorAngle( angle );
	}
}


// This function is used to animate a 3D F16 model using data produced by FixedWingLibCGF::SimplifiedAirplane class.
// FighterJetAnimator class is a wrapper around underlying graphics API. In your applications
// you will need to translate methods such as setNoseGear to corresponding graphics
// API calls ( e.g. for OpenSceneGraph you have to set the transformation matrix of the Transform
// Node )
static void updateF16Animation_CGF( FixedWingLibCGF::SimplifiedAirplane* airplane , Simple3D* s3d , float dt )
{
	int vehIndex = s3d->getVehicleIndex( airplane );
	if ( vehIndex == -1 ) return;

	FighterJetAnimatorBase* fighterAnimator = s3d->getFighterJetAnimator(vehIndex);

    // Animation of the ailerons.
    {
        float leftAileronAngle = smMath::degToRad(airplane->getAileronDeflection());
        fighterAnimator->setLeftAileronDeflection( leftAileronAngle  );

        float rightAileronAngle = smMath::degToRad(-airplane->getAileronDeflection());
        fighterAnimator->setRightAileronDeflection(rightAileronAngle  );
    }


    // Animation of the elevators
    // This is almost same as aileron animation.
    {
        float leftElevatorAngle = -smMath::degToRad(airplane->getElevatorDeflection());
        fighterAnimator->setLeftElevatorDeflection( leftElevatorAngle );

        float rightElevatorAngle = -smMath::degToRad(airplane->getElevatorDeflection());
        fighterAnimator->setRightElevatorDeflection( rightElevatorAngle  );
    }

    // Rudder animation is also pretty straightforward...
    {
        fighterAnimator->setRudderDeflection( smMath::degToRad( airplane->getRudderDeflection() ) );
    }

	fighterAnimator->setAirbrakeDeflection( smMath::degToRad( airplane->getAirbrakeDeflection() ) );

}

// This function is used to animate a 3D UH-60 model using data produced by RotorLibFDM::GenHeli500 class.
template<typename HeliType>
static void updateUH60Animation_FDM( HeliType* theHeli , Simple3D* s3d )
{
	int vehIndex = s3d->getVehicleIndex( theHeli );
	if ( vehIndex == -1 ) return;

	UH60AnimatorBase* uh60Animator = s3d->getUH60Animator(vehIndex);

    RTD::FGGroundContactBase* mainGearRight = dynamic_cast<RTD::FGGroundContactBase*>(theHeli->getGroundContact(0));
    RTD::FGGroundContactBase* mainGearLeft = dynamic_cast<RTD::FGGroundContactBase*>(theHeli->getGroundContact(1));
    RTD::FGGroundContactBase* tailGear  = dynamic_cast<RTD::FGGroundContactBase*>(theHeli->getGroundContact(2));

    // Each ground contact can give translational and rotational offsets of the wheels relative to rest (default) positions.
    // Using this information, we can simply move the wheels (and if necessary other parts of the gears).
    // In this example applying the offsets to the 3D model is undertaken by UH60 animator class' setLeftMainGear, setRightMainGear
    // and setTailGear methods.
    if (mainGearLeft)
       uh60Animator->setLeftMainGear( mainGearLeft->getWheelTransOffsetToRestPosition() , mainGearLeft->getWheelRotOffsetToRestPosition() );

    if (mainGearRight)
       uh60Animator->setRightMainGear( mainGearRight->getWheelTransOffsetToRestPosition() , mainGearRight->getWheelRotOffsetToRestPosition() );

    if (tailGear)
       uh60Animator->setTailGear( tailGear->getWheelTransOffsetToRestPosition() , tailGear->getWheelRotOffsetToRestPosition() );
}

// This function is used to animate a 3D UH-60 model using data produced by RotorLibCGF::SimplifiedRotorcraft class.
static void updateUH60Animation_CGF( RotorLibCGF::SimplifiedRotorcraft* theHeli , Simple3D* s3d )
{
	int vehIndex = s3d->getVehicleIndex( theHeli );
	if ( vehIndex == -1 ) return;

	UH60AnimatorBase* uh60Animator = s3d->getUH60Animator(vehIndex);

    RTD::FGGroundContactBase* mainGearRight = dynamic_cast<RTD::FGGroundContactBase*>(theHeli->getGroundContact(0));
    RTD::FGGroundContactBase* mainGearLeft = dynamic_cast<RTD::FGGroundContactBase*>(theHeli->getGroundContact(1));
    RTD::FGGroundContactBase* tailGear  = dynamic_cast<RTD::FGGroundContactBase*>(theHeli->getGroundContact(2));

    // Each ground contact can give translational and rotational offsets of the wheels relative to rest (default) positions.
    // Using this information, we can simply move the wheels (and if necessary other parts of the gears).
    // In this example applying the offsets to the 3D model is undertaken by UH60 animator class' setLeftMainGear, setRightMainGear
    // and setTailGear methods.
    if (mainGearLeft)
       uh60Animator->setLeftMainGear( mainGearLeft->getWheelTransOffsetToRestPosition() , mainGearLeft->getWheelRotOffsetToRestPosition() );

    if (mainGearRight)
       uh60Animator->setRightMainGear( mainGearRight->getWheelTransOffsetToRestPosition() , mainGearRight->getWheelRotOffsetToRestPosition() );

    if (tailGear)
       uh60Animator->setTailGear( tailGear->getWheelTransOffsetToRestPosition() , tailGear->getWheelRotOffsetToRestPosition() );
}
