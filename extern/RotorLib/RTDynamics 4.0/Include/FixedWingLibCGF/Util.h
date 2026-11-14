#pragma once
#include "RTD/BotBase.h"



namespace FixedWingLibCGF
{
	class AirplaneBot;

	class SpeedController 
	{
	public:
		virtual void computeThrustAlongV( const AirplaneBot* controlledBot , smReal Sd, smReal deltaTime , smReal& Talongv );
		virtual void computeThrust( const AirplaneBot* bot , smReal Sd , smReal deltaTime , smReal& thrust , smReal& brakePercentage);
	};


}