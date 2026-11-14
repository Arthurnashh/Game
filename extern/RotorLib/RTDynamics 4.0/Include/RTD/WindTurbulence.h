/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/WindBase.h"
#include "RTD/LookupTable.h"
#include "RTD/GaussianRandomNumber.h"

namespace RTD
{
	/** WindTurbulence implements the MIL-F-8785C and Tustin Turbulence models.
	*
	*	MIL-F-8785C is only available if this build is compiled with military features.
	*
	*	TODO: how to use WindTurbulence class
	*	TODO: what are most commonly changed parameters and how they shall be set.
	*	
	*/
	class RTDYNAMICS_EXPORT WindTurbulence : public WindBase
    {
	public:
		enum Strength
		{
			Strength_Disabled = 0,
			Strength_1,
			Strength_2,
			Strength_Light,
			Strength_Moderate,
			Strength_5,
			Strength_Severe,
			Strength_7,

			No_Severities
		};

		enum ModelType
		{
			ModelType_MIL_F_8785C = 0,
			ModelType_Tustin,
		};

		class RTDYNAMICS_EXPORT ConfigParams
		{
		public:
			virtual bool readParamStr( const std::string& parameterString, std::string& error );
		};

	public:
        /** Constructor of the class. 
        */
        explicit WindTurbulence( const std::string& name , SimulationVariableUser* parent );
        explicit WindTurbulence( const WindTurbulence& right ) ;
        virtual ~WindTurbulence( ) ;

		WindTurbulence & operator=(const WindTurbulence &right ) ;

		virtual void	update( float deltaTime );
		virtual void	finalize();
		virtual void	reset();

		std::string			getStatusString() const;

		virtual void		setModelType( ModelType m ) { i_modelType = m; }
		virtual ModelType	getModelType( ) const { return static_cast<ModelType>(i_modelType.getValue()) ; }

		virtual void		setStrength( Strength s ) { i_POEIndex = s; }
		virtual Strength	getStrength( ) const { return static_cast<Strength>(i_POEIndex.getValue()) ; }

		virtual void		setWindSpeedAt20ftFps( smReal v ) { i_windSpeedAt20ftFps = v; }
		virtual smReal		getWindSpeedAt20ftFps( ) const { return i_windSpeedAt20ftFps ; }

		virtual LookUpTable	getPOELookupTable( ) const { return m_POELookupTable ; }

		virtual void		setAltitudeAboveTerrain( const smReal& h ) { i_h = h; }
		virtual smReal		getAltitudeAboveTerrain() const	{ return i_h.getValue();	}

		virtual void		setVelocity( const smReal& v ) { i_v = v; }
		virtual smReal		getVelocity() const { return i_v.getValue();	}

		/** setWingSpan can be used to specify the wing span required in the calculation of the 
		*	turbulence on the angular rates. */
		virtual void		setWingSpan( const smReal& b ) { i_wingSpan = b; }
		virtual smReal		getWingSpan() const { return i_wingSpan.getValue();	}

		virtual void		setWindDirection( const smReal& v ) { i_psiw = v; }
		virtual smReal		getWindDirection() const { return i_psiw.getValue(); }

		static bool			runUnitTests( const std::string& outputDir );

	protected:
		LookUpTable	m_POELookupTable;
		double getPOE(int POEIndex, double h);

		// Note: only the Tustin model requires *_km2 variables.
		StateVariableDouble	xi_u_km1, nu_u_km1,
							xi_v_km1, xi_v_km2, nu_v_km1, nu_v_km2,
							xi_w_km1, xi_w_km2, nu_w_km1, nu_w_km2,
							xi_p_km1, nu_p_km1,
							xi_q_km1, xi_r_km1;

		GaussianRandomNumber*	m_grn;


		InputVariableReal		i_h;
		InputVariableReal		i_v;
		InputVariableReal		i_wingSpan;
		InputVariableReal		i_psiw;

		InputVariableReal		i_windSpeedAt20ftFps;
		InputVariableUInt		i_POEIndex; 
		InputVariableUInt		i_modelType; 

		InputVariableVec3d		o_scaleLengths;

	};
}
