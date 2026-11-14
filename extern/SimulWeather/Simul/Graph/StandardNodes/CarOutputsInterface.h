#ifndef SIMUL_META_CAROUTPUTSINTERFACE_H
#define SIMUL_META_CAROUTPUTSINTERFACE_H
namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			class CarOutputsInterface
			{
			public:
				virtual float GetSpeedInMPH() const=0;
				virtual float GetWheelbase() const =0;
			/*	virtual int GetGear() const=0;
				virtual float GetGearRatio(int) const=0;
				virtual float GetRPM() const =0;
				virtual float GetEngineOn() const =0;
				virtual float GetEngineIgnition() const =0;
				virtual float GetSteeringWheelAngle() const =0;
				virtual float GetTime() const =0;
				virtual float GetLastTime() const =0;
				virtual float GetBestTime() const =0;

				virtual float GetAligningTorque() const =0;
				virtual float GetAligningTorqueSlope() const =0;
				virtual float GetFeedbackDamping() const =0;
				virtual float GetRoadRumble() const =0;

				
				virtual float GetSteerAssistFactor() const =0;
				virtual float GetTractionControlFactor() const =0;
				virtual float GetBrakeAssistFactor() const =0;*/
				virtual float GetSlipRatio(int) const =0;
				virtual float GetPeakSlipRatio(int) const =0;

			};
		}
	}
}
#endif