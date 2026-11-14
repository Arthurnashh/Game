#pragma once


#include "FixedWingLibFDM/GenAirplane100.h"
#include "FixedWingLibFDM/FCSAirplaneAutoTrim100.h"
#include "FixedWingLibCGF/SimplifiedAirplane.h"
#include "RotorLibFDM/GenHeli500.h"
#include "RotorLibCGF/RotorCraftBot.h"
#include "FixedWingLibCGF/AirplaneBot.h"

#include "RTD/FGGroundContact200.h"
#include "RotorLibFDM/FCSHelicopterAutoTrim100.h"
#include "RTD/GenericTurbineEngine.h"
#include "RTD/ConstantRPMEngine.h"
#include "RTD/WindsSystem.h"

 


template<typename HeliType>
static std::string createInfoString_RotorLibFDM( HeliType* heli )
{		
    std::string autoTrimStr;
    {
        RotorLibFDM::FCSHelicopterAutoTrim100* autoTrim = dynamic_cast<RotorLibFDM::FCSHelicopterAutoTrim100*>(heli->getCSComponent(0));
        if ( autoTrim )
		{
			if ( autoTrim->isEnabled() )
			{
				autoTrimStr = "Autotrim";
			}
			else
			{
				autoTrimStr = "";
			}
		}
		else
		{
			autoTrimStr = "";
		}
    }

    std::string wheelBrakeStr;
    {
        if ( heli->getPilotInputData()->m_wheelBrakesRight > 0.0 )
        {
            wheelBrakeStr = "Wheel brakes active.";
        }
        else
        {
            wheelBrakeStr = "Wheel brakes inactive.";
        }
    }

	char gearsStr[128];
	if ( heli->getPilotInputData()->m_extendGears  )
	{
		sprintf( gearsStr, "Extend gears. (%%%i extended)" , (int)(heli->getGearStatus()*100.0f)  );
	}
	else
	{
		sprintf( gearsStr, "Retract gears.(%%%i retracted)" , (int)(100.0f - heli->getGearStatus()*100.0f)  );
	}

	std::string autoPilotStr;
	if ( heli->getPilotInputData()->m_autoPilotMasterSwitch )
	{
		autoPilotStr = "AP: ";
		autoPilotStr += autoTrimStr + " ";

		if ( heli->getPilotInputData()->m_sasPitchDamper )
			autoPilotStr += "Pdamper ";
		
		if ( heli->getPilotInputData()->m_sasRollDamper )
			autoPilotStr += "Rdamper ";

		if ( heli->getPilotInputData()->m_sasYawDamper )
			autoPilotStr += "Ydamper ";

		if ( heli->getPilotInputData()->m_sasPitchHold )
			autoPilotStr += "Phold ";

		if ( heli->getPilotInputData()->m_sasRollHold )
			autoPilotStr += "Rhold ";

		if ( heli->getPilotInputData()->m_sasHeadingHold )
			autoPilotStr += "Hhold ";

		if ( heli->getPilotInputData()->m_sasTurbulenceMode )
			autoPilotStr += "Turb ";

		if ( heli->getPilotInputData()->m_autoPilotALT )
			autoPilotStr += "ALT ";

		if ( heli->getPilotInputData()->m_autoPilotALTA )
		{
			autoPilotStr += "ALTA(" + smToString( heli->getPilotInputData()->m_autoPilotALTAAltitudeFeet.getValue() ) + "ft) ";
		}

		if ( heli->getPilotInputData()->m_autoPilotSPD )
		{
			autoPilotStr += "SPD(" + smToString( heli->getPilotInputData()->m_autoPilotSPDSpeedKts.getValue() , 3 ) + "kts) ";			
		}

		if ( heli->getPilotInputData()->m_autoPilotVS )
		{
			autoPilotStr += "VS(" + smToString( heli->getPilotInputData()->m_autoPilotVSVerticalSpeedFtMin.getValue() ) + "ft/min) ";
		}

		if ( heli->getPilotInputData()->m_autoPilotHDG )
		{
			autoPilotStr += "HDG(" + smToString( heli->getPilotInputData()->m_autoPilotHDGHeading.getValue() ) + "deg) ";			
		}

		if ( heli->getPilotInputData()->m_autoPilotHover )
		{
			autoPilotStr += "HVR ";			
		}

		if ( heli->getPilotInputData()->m_autoPilotHHT )
		{
			autoPilotStr += "H.HT(" + smToString( heli->getPilotInputData()->m_autoPilotHoverHeightFeet.getValue() ) + "ft) ";
		}

		if ( heli->getPilotInputData()->m_autoPilotTDown )
		{
			autoPilotStr += "T.DWN ";			
		}

		if ( heli->getPilotInputData()->m_autoPilotTUp )
		{
			autoPilotStr += "T.UP("+ smToString( heli->getPilotInputData()->m_autoPilotCruiseHeightFeet.getValue() ) + "ft) ";
		}

		if ( heli->getPilotInputData()->m_autoPilotNAV )
		{
			autoPilotStr += "NAV(" + smToString( heli->getPilotInputData()->m_autoPilotNAVWPIndex.getValue() ) + ") ";	
		}

	}
	else
	{
		if ( autoTrimStr == "" )
			autoPilotStr = "AP off";
		else
			autoPilotStr = "AP: " + autoTrimStr;
			
	}


	std::string engineStr;	
	for ( size_t i = 0; i < heli->getNumEngines() ; i++ )
	{
		char buf[512];
		buf[0] = 0;

		if ( dynamic_cast<RTD::ConstantRPMEngine*>( heli->getEngine(i) ) )
		{
			RTD::ConstantRPMEngine* engine = dynamic_cast<RTD::ConstantRPMEngine*>( heli->getEngine(i) );
			sprintf( buf , "Engine %i: Nf %5.0f Power(HP):%5.0f\n" , i , engine->getRPM() , engine->getPowerHP() );
		}
		else if ( dynamic_cast<RTD::GenericTurbineEngine*>( heli->getEngine(i) ) )
		{
			RTD::GenericTurbineEngine* engine = dynamic_cast<RTD::GenericTurbineEngine*>( heli->getEngine(i) );
			sprintf( buf , "Engine %i: Thr:%3.2f Ng%%:%3.0f Nf%%:%3.0f Torque%:%3.0f Power(HP):%4.0f EGT(C):%4.0f\n" , i , engine->getThrottle(), engine->getNg(true)  , engine->getNp(true) , engine->getTorque(true), engine->getPowerHP() , engine->getEGTCelsius() );
		}

		engineStr = engineStr + buf;		
	}
	
	std::string rotorStr;		
	{	
		char buf[512];
		sprintf( buf , "Main Rotor Power(HP):%6.0f RPM:%4.0f\n" ,heli->getMainRotor()->getPowerHP(),heli->getMainRotor()->getRPM());

		rotorStr = rotorStr + buf;	
	}

	std::string collectiveDetentStr;
	{
		if ( heli->getPilotInputData()->m_collectiveOutOfDetent.getValue() )
		{
			collectiveDetentStr = "out-of-detent";
		}
		else
		{
			collectiveDetentStr = "detent";
		}

	}

	smReal aoa,sideslip;
	heli->getAoASideslip(aoa, sideslip);
	char  msg[1024];
	sprintf(msg,
				"Wind:%s Turbulance strength:%s \n%s\n%s\nClimb[1000ft/min]=%4.2f\nSpeed[KIAS]=%4.2f [GndS]:%4.2f kts\n"
				"Altitude[ft]=%4.2f Radar Alt[ft]:%4.2f\n"
				"Heading[deg]=%4.2f\nPitch[deg]=%4.2f\nRoll[deg]=%4.2f\n"
				"AoA[deg]=%4.2f\nSideslip[deg]=%4.2f\n"
				"Collective=%4.2f(%s)\nCyclic Roll=%4.2f\nCyclic Pitch=%4.2f\nPedals=%4.2f\n"
				"%s"
				"%s"
				"%s\n"
				"Geo. pos.=%s\n",
				heli->getWindsSystem()->getMeanWind()->getLinearWindVelocityWCS().toString().c_str(),
				smToString( heli->getWindsSystem()->getTurbulence()->getStrength() ).c_str(),
				autoPilotStr.c_str(),
				wheelBrakeStr.c_str(),
				(heli->getState().getSpeed()[2] * 0.001f * 60.0f) / 0.305f, 
				heli->getIndicatedAirSpeedKnots(),
				std::sqrt( heli->getVelocity()[0]* heli->getVelocity()[0] + heli->getVelocity()[1]* heli->getVelocity()[1]) * RTD::Units::mps_to_knots,				
				heli->getAltitudeInFeet(), heli->getAltitudeInFeet()- heli->getTerrainHeight()* RTD::Units::meters_to_feet ,
				heli->getState().getRotAsHPR()[0],
				heli->getState().getRotAsHPR()[1],
				heli->getState().getRotAsHPR()[2],
				aoa, sideslip,
				heli->getPilotInputData()->m_collective.getValue(), collectiveDetentStr.c_str() , 
				heli->getPilotInputData()->m_rollPitchControl.getValue()[0],
				heli->getPilotInputData()->m_rollPitchControl.getValue()[1],
				heli->getPilotInputData()->m_pedals.getValue(),
				engineStr.c_str(),
				rotorStr.c_str(),
				gearsStr,
				RTD::EarthModelWGS84::llaToString(heli->getPositionAsLLA()).c_str()
			);

	return std::string( msg );
}

static std::string createInfoString_RotorLibCGF( RotorLibCGF::SimplifiedRotorcraft* firstAirplane,RotorLibCGF::RotorcraftBot* firstBot )
{

    smReal AoA, sideslip;
	firstAirplane->getState().getAoASideslip( AoA, sideslip , firstAirplane->getLinearWindVelocity());

	char  msg[1024];
    sprintf(msg,
        "Controller:%s\n"
		"State:%s\n"
		"Climb[1000ft/min]=%4.2f\nSpeed[KTAS]=%4.2f [KIAS]=%4.2f [M]=%4.3f\n"
        "Altitude[ft]=%4.1f\nHeading[deg]=%8.4f\n"
        "Pitch[deg]=%5.2f\nRoll[deg]=%5.2f\n"
        "Throttle=%4.2f\nYoke/Stick Pitch=%5.2f\n"
        "Yoke/Stick Roll=%4.2f \nPedals=%4.2f\n"
        "AoA:%4.2f\nsideslip:%4.2f\n"
        "Load Factor:%.2f\n",
        firstBot->getMasterController() ? firstBot->getMasterController()->getName().c_str() : "none",
		firstBot->getStatusString().c_str(),
        firstAirplane->getState().getSpeed()[2] * RTD::Units::mps_to_fpmin * 0.001f,
        firstAirplane->getState().getSpeed().length() * RTD::Units::mps_to_knots , 
		firstAirplane->getState().getSpeed().length() * RTD::Units::mps_to_knots * std::sqrt(firstAirplane->getWorld()->getAirDensity( firstAirplane->getPosition()[2])/ 1.225f), 
		RTD::computeMachNumber( firstAirplane->getWorld() , firstAirplane->getPosition()[2] ,  firstAirplane->getVelocity().length()  ),						
		firstAirplane->getState().getPosition()[2] * RTD::Units::meters_to_feet,
        firstAirplane->getState().getRotAsHPR()[0],
        firstAirplane->getState().getRotAsHPR()[1],
        firstAirplane->getState().getRotAsHPR()[2],
        firstAirplane->getPilotInputData()->m_throttle[0].getValue(),
        firstAirplane->getPilotInputData()->m_rollPitchControl.getValue()[1],
        firstAirplane->getPilotInputData()->m_rollPitchControl.getValue()[0],
        firstAirplane->getPilotInputData()->m_pedals.getValue(),
        AoA, sideslip,
        firstAirplane->getState().getLoadFactor( firstAirplane->getWorld()->getGravity())   );

		return std::string( msg );    
}

static std::string createInfoString_FixedWingLibFDM( FixedWingLibFDM::GenAirplane100* theAirplane )
{
	char spoilerStr[128];
	if ( theAirplane->getPilotInputData()->m_spoilers  )
	{
		sprintf( spoilerStr, "Extend spoilers. (%%%i extended)" , (int)(theAirplane->getSpoilerSetting()*100.0f)  );
	}
	else
	{
		sprintf( spoilerStr, "Retract spoilers.(%%%i retracted)" , (int)(100.0f - theAirplane->getSpoilerSetting()*100.0f)  );
	}


	char wheelBrakeStr[128];
	if (( theAirplane->getPilotInputData()->m_wheelBrakesLeft > 0.0f )||( theAirplane->getPilotInputData()->m_wheelBrakesRight > 0.0f ) )
	{
		sprintf( wheelBrakeStr, "Wheel brakes active." );
	}
	else
	{
		sprintf( wheelBrakeStr, "Wheel brakes inactive." );
	}

	char gearsStr[128];
	if ( theAirplane->getPilotInputData()->m_extendGears  )
	{
		sprintf( gearsStr, "Extend gears. (%%%i extended)" , (int)(theAirplane->getGearStatus()*100.0f)  );
	}
	else
	{
		sprintf( gearsStr, "Retract gears.(%%%i retracted)" , (int)(100.0f - theAirplane->getGearStatus()*100.0f)  );
	}

	char autoTrimStr[128];
	FixedWingLibFDM::FCSAirplaneAutoTrim100* autoTrim = (FixedWingLibFDM::FCSAirplaneAutoTrim100*)theAirplane->getCSComponent(0);				
	if ( autoTrim->isEnabled()  )
	{
		sprintf( autoTrimStr, "Autotrim active" );
	}
	else
	{
		sprintf( autoTrimStr, "Autotrim disabled" );
	}

	smReal AoA, sideslip;
	theAirplane->getAoASideslip( AoA, sideslip );

	char  msg[1024];
	sprintf(msg, 
		"\n%s\n\n"
		"Climb[1000ft/min]=%4.2f\n"
		"Speed[knots]=%4.2f\n"
		"Altitude[ft]=%4.1f\n"
		"Heading[deg]=%8.4f\n"
		"Pitch[deg]=%4.2f\n"
		"Roll[deg]=%4.2f\n"
		"Throttle=%4.2f\n"
		"Yoke/Stick Pitch=%4.2f Pitch Trim:%4.2f\n"
		"Yoke/Stick Roll=%4.2f Roll Trim:%4.2f\n"
		"Pedals=%4.2f Yaw Trim:%4.2f\n"
		"Flap config:%3.2f\n"
		"%s\n"
		"%s\n"
		"%s\n"
		"AoA:%4.2f\nsideslip:%4.2f\n"
		"Load Factor:%.2f\n"
		"Accel=%s\n"
		"Centri. Accel=%s\n"
		"Insta. Turnrate=%s", 
		autoTrimStr,
		theAirplane->getState().getSpeed()[2] * RTD::Units::mps_to_fpmin * 0.001f,
		theAirplane->getState().getSpeed().length() * RTD::Units::mps_to_knots ,
		theAirplane->getState().getPosition()[2] * 3.2786f,
		theAirplane->getState().getRotAsHPR()[0],
		theAirplane->getState().getRotAsHPR()[1],
		theAirplane->getState().getRotAsHPR()[2],
		theAirplane->getPilotInputData()->m_throttle[0].getValue(),
		theAirplane->getPilotInputData()->m_rollPitchControl.getValue()[1] , theAirplane->getPilotInputData()->m_pitchTrim.getValue(),
		theAirplane->getPilotInputData()->m_rollPitchControl.getValue()[0], theAirplane->getPilotInputData()->m_rollTrim.getValue(),
		theAirplane->getPilotInputData()->m_pedals.getValue(), theAirplane->getPilotInputData()->m_yawTrim.getValue(),
		theAirplane->getLeftFlapSetting(),
		spoilerStr,
		wheelBrakeStr,
		gearsStr,				
		AoA, sideslip,
		theAirplane->getState().getLoadFactor( theAirplane->getWorld()->getGravity() ),
		theAirplane->getState().getAccel().toString().c_str(),
		theAirplane->getState().getCentrifugalAccel( ).toString().c_str(),
		theAirplane->getState().getInstantaneousAngularVel( ).toString().c_str()
		);

	return std::string( msg );
}

static std::string createInfoString_FixedWingLibCGF( FixedWingLibCGF::SimplifiedAirplane* firstAirplane,FixedWingLibCGF::AirplaneBot* firstBot )
{

    smReal AoA, sideslip;
	firstAirplane->getState().getAoASideslip( AoA, sideslip , firstAirplane->getLinearWindVelocity() );

	char  msg[1024];
    sprintf(msg,
        "Controller:%s\n"
		"State:%s\n"
		"Climb[1000ft/min]=%4.2f\nSpeed[KTAS]=%4.2f [KIAS]=%4.2f [M]=%4.3f\n"
        "Altitude[ft]=%4.1f"
        " [m]=%4.1f\n"
		"Heading[deg]=%8.4f\n"
        "Pitch[deg]=%5.2f\nRoll[deg]=%5.2f\n"
        "Throttle=%4.2f\nYoke/Stick Pitch=%5.2f\n"
        "Yoke/Stick Roll=%4.2f \nPedals=%4.2f\n"
        "AoA:%4.2f\nsideslip:%4.2f\n"
        "Load Factor:%.2f\n",
        firstBot->getMasterController() ? firstBot->getMasterController()->getName().c_str() : "none",
		firstBot->getStatusString().c_str(),
        firstAirplane->getState().getSpeed()[2] * RTD::Units::mps_to_fpmin * 0.001f,
        firstAirplane->getState().getSpeed().length() * RTD::Units::mps_to_knots , 
		firstAirplane->getState().getSpeed().length() * RTD::Units::mps_to_knots * std::sqrt(firstAirplane->getWorld()->getAirDensity( firstAirplane->getPosition()[2])/ 1.225f), 
		RTD::computeMachNumber( firstAirplane->getWorld() , firstAirplane->getPosition()[2] ,  firstAirplane->getVelocity().length()  ),						
		firstAirplane->getState().getPosition()[2] * RTD::Units::meters_to_feet,
		firstAirplane->getState().getPosition()[2],
        firstAirplane->getState().getRotAsHPR()[0],
        firstAirplane->getState().getRotAsHPR()[1],
        firstAirplane->getState().getRotAsHPR()[2],
        firstAirplane->getPilotInputData()->m_throttle[0].getValue(),
        firstAirplane->getPilotInputData()->m_rollPitchControl.getValue()[1],
        firstAirplane->getPilotInputData()->m_rollPitchControl.getValue()[0],
        firstAirplane->getPilotInputData()->m_pedals.getValue(),
        AoA, sideslip,
        firstAirplane->getState().getLoadFactor( firstAirplane->getWorld()->getGravity())   );

		return std::string( msg );    
}