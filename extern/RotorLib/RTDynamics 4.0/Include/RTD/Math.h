/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/Common.h"


namespace RTD
{
	class VehicleBase;
	class ReferenceEntityBase;
	class World;

	/** Gets an angle in degrees and returns result in degrees in -180,180 range */
	smReal	RTDYNAMICS_EXPORT standardizeAngle( smReal angle );

	/** Gets an angle in degrees and returns result in degrees in -180,180 range */
	smDouble	RTDYNAMICS_EXPORT standardizeAngle( smDouble angle );

	/**
	 Returns angle1 - angle2. This difference is such that it is in the
	 shortest turn direction. E.g. if angle1 = 170 , angle2=-170 then angle1 - angle2 is
	 340. This is a CW turn. smallestAngleDifference would return -20 (CCW) for this difference.
	 If returned result is positive a CCW rotation is meant. Negative results are for CW rotations.
	 This is shortest distance between these angles.
	 Input arguments do not need to be standardized.
	*/
	smReal	RTDYNAMICS_EXPORT smallestAngleDifference( smReal angle1 , smReal angle2  );

	/**
	 Returns angle1 - angle2. This difference is such that it is in the
	 shortest turn direction. E.g. if angle1 = 170 , angle2=-170 then angle1 - angle2 is
	 340. This is a CW turn. smallestAngleDifference would return -20 (CCW) for this difference.
	 If returned result is positive a CCW rotation is meant. Negative results are for CW rotations.
	 This is shortest distance between these angles.
	 Input arguments do not need to be standardized.		
	*/
	smDouble	RTDYNAMICS_EXPORT smallestAngleDifference( smDouble angle1 , smDouble angle2  );

	/**
	 Returns the difference of angle1-angle2 in clockwise direction. E.g. if 
	 angle1=-10 , angle2=10 then angle1 - angle2 is -20. This is a CCW turn.
	 For this case clockwiseAngleDifference would return 340 which is CW.
	 Return value of this method is always negative, which signifies a CW rotation.	
	 Input arguments do not need to be standardized.
	 Note that if returned value is standardized it might not be CW anymore!
	*/
	smReal	RTDYNAMICS_EXPORT clockwiseAngleDifference( smReal angle1 , smReal angle2  );

	/**
	 Returns the difference of angle1-angle2 in clockwise direction. E.g. if 
	 angle1=10 , angle2=-10 then angle1 - angle2 is 20. This is a CW turn.
	 For this case counterclockwiseAngleDifference would return -340 which is CCW.
	 Return value of this method is always positive, which signifies a CW rotation.
	 Input arguments do not need to be standardized.
	 Note that if returned value is standardized it might not be CCW anymore!
	*/
	smReal	RTDYNAMICS_EXPORT counterclockwiseAngleDifference( smReal angle1 , smReal angle2  );


	/**
		Computes position and orientation in a given reference frame. 
		
		This function can be used in attaching objects to other objetcs. For this purpose position and
		orientation of the main object has to be provided in refXYZ and refHPR. relativeXYZ and relativeHPR
		are position and orientation of the second object (the object that is going to be attached to the 
		main object) relative to the main object. computeRelativePosition, then, returns the world coordinate
		position and orientation of the attached object in resultXYZ and resultHPR.		
	*/
	void RTDYNAMICS_EXPORT computeRelativePosition(	  const smVec3r& refXYZ , const smVec3r& refHPR , 
													  const smVec3r& relativeXYZ , const smVec3r& relativeHPR , 
													  smVec3r& resultXYZ , smVec3r& resultHPR );

	
	
	smReal	RTDYNAMICS_EXPORT smoothstep( smReal edge0 , smReal edge1 , smReal x );

	/**
		Computes the projection of a vector onto horizontal plane. Horizontal plane is determined
		using the gravity vector, a plane that is perpendicular to this vector is a horizontal plane.
	*/
	smVec3r	RTDYNAMICS_EXPORT computeVectorHorizontalProjection( const smVec3r& input , const smVec3r& gravity );


	/**
	*   This function computes and returns the velocity vector (in world
	*   coordinate* system) of a point of an object (e.g. a ship) whose
	*   translational and rotational velocities are given.
	* 
	*	\param objectXYZ 		Position of the object.
	*   \param objectHPR 		Orientation of the object as heading,pitch and
	*          					roll (in degrees)
	*	\param objVelocity		Velocity of the object
	*	\param objRotVelocity	Rotational velocity of the object in body coordinates. \e objRotVelocity[0] is rotational
	*							speed around x axis, \e objRotVelocity[1] around y axis and \e objRotVelocity[2] is around
	*							z axis.	Rotational speeds are in rad/sec.
	*	\param pointInWCS		point position in world coordinate system.
	*/
	smVec3d RTDYNAMICS_EXPORT computeVelocityAtAPointOnObject(	const smVec3d& objectXYZ , const smVec3d& objectHPR ,
																const smVec3d& objVelocity ,const smVec3d& objRotVelocity ,
																const smVec3d& pointInWCS );



	/**
		This function is used to compute angle of attack and side slip angles of a body. The computations
		are made in a coordinate system provided by the user. RTD coordinate system convention (Y forward,X
		right, Z up) is used.

		In this method \e side, \e front and \e up vectors are used to define a second coordinate
		system \b within the body coordinate system. \e relativeWindVelocity is defined relative to
		\b body coordinate system whereas the angle of attack and side slip angles will be computed
		in user provided coordinate system.

		The function is useful to model wings and stabilizers which are not aligned with the aircraft body.

		\param relativeWindVelocity  Velocity of the free stream relative to the body. In meters.
		\param aoa Resulting angle of attack in degrees.
		\param sideslip Resulting side slip in degrees.
		\param side 	vector pointing sideward to the right
		\param front	vector pointing forward
		\param up		vector pointing upward
	*/
	void	RTDYNAMICS_EXPORT computeAoAAndSideslip( const smVec3r& side,
												     const smVec3r& front,
													 const smVec3r& up,
													 const smVec3r& relativeWindVelocity,
													 smReal& aoa , smReal& sideslip );

	/**
		This function is used to compute angle of attack and side slip angles of a body. The computations
		are made in body coordinate system with RTD coordinate system convention (Y forward,X
		right, Z up).

		\param relativeWindVelocity  Velocity of the free stream relative to the body. In meters.
		\param aoa Resulting angle of attack in degrees.
		\param sideslip Resulting side slip in degrees.
	*/
	void	RTDYNAMICS_EXPORT computeAoAAndSideslip( const smVec3r& relativeWindVelocity,
													 smReal& aoa , smReal& sideslip );


	/**
		This function computes and returns the mach number for a given speed at the given altitude. The 
		function looks up the atmospheric conditions from the \e world object.

		\param world  World object which contains the atmospheric conditions.
		\param altitude Altitude in meters.
		\param speed speed in m/s.

	*/
	smReal RTDYNAMICS_EXPORT computeMachNumber( const RTD::World* world, smReal altitude , smReal speed );


	/**
		Computes the angle between the two vectors. This method ensures that no INF or NaN is produced.
		All boundary cases are handled safely.

		Within the method |A||B|sin( x ) = |A x B| equation is used instead of more common
		|A||B|cos(x) = A . B equation. Even though the latter has better performance it has
		problems around x = 0. When A and B is very close to each other A.B/(|A||B|) > 1.0 case
		is observed. When this happens acos is undefined. Cross product based method provides
		much better precision around zero.
	*/
	smDouble	RTDYNAMICS_EXPORT computeAngleBetweenVectors( const smVec3d& vec1, const smVec3d& vec2 );


	/**
		See computeAngleBetweenVectors for double precision.
	*/
	smReal	RTDYNAMICS_EXPORT computeAngleBetweenVectors( const smVec3r& vec1, const smVec3r& vec2 );


	/**
		Returns solutions to quadratic equation. The equation has following form

		a*x^2 + b*x + c = 0

		Solutions are returned in s1 and s2. 

		Function returns true if at least one solution is found. 
	*/
	bool RTDYNAMICS_EXPORT solveQuadraticEquation( const smReal& a , const smReal&  b , const smReal& c , smReal& s1 , smReal& s2  );
	
	/**
		Returns solutions to quadratic equation. The equation has following form

		a*x^2 + b*x + c = 0

		Solutions are returned in s1 and s2. 

		Function returns true if at least one solution is found. 
	*/
	bool RTDYNAMICS_EXPORT solveQuadraticEquation( const smDouble& a , const smDouble&  b , const smDouble& c , smDouble& s1 , smDouble& s2  );

	/**
		Solves for x,y in following equation system:
		a1 x + b1 y = c1
		a2 x + b2 y = c2

		When a solution is found true is returned. In case the equation system does not have a solution (i.e. parallel lines) false is returned. 
	*/
	bool RTDYNAMICS_EXPORT solveLinearEquationSystemOfTwoUnknowns( const smDouble& a1 , const smDouble&  b1 , const smDouble& c1 , 
																   const smDouble& a2 , const smDouble&  b2 , const smDouble& c2, 																	
																   smDouble& x , smDouble& y  );


	/**
        Returns the ground distance (distance on XY plane) between two positions. 
    */
	smReal RTDYNAMICS_EXPORT computeGroundDistance( const smVec3r& pos1 , const smVec3r& pos2 );

	/**
        Returns the ground distance (distance on XY plane) between two positions. 
    */
	smDouble RTDYNAMICS_EXPORT computeGroundDistance( const smVec3d& pos1 , const smVec3d& pos2 );

    /** 
	 *  \e referencePos and \e referenceHPR define the position and orientation of a reference object in 3D space. 
	 *	This function returns true if the position given by \e targetXYZ is in front hemisphere of the reference object in the.
	 *  plan view (2D, top down view).
     */ 
	bool RTDYNAMICS_EXPORT isInFrontHemisphereInPlanView( const smVec3r& referencePos, const smVec3r& referenceHPR , const smVec3r& targetXYZ );

    /** 
	 *	This function returns true if the \e target is in front hemisphere of the \e reference in the.
	 *  plan view (2D, top down view).
     */ 
	bool RTDYNAMICS_EXPORT isInFrontHemisphereInPlanView( const RTD::VehicleBase* reference ,  const RTD::VehicleBase* target );

   /** 
	 *	This function returns true if the \e target is in front hemisphere of the \e reference in the.
	 *  plan view (2D, top down view).
     */ 

	bool RTDYNAMICS_EXPORT isInFrontHemisphereInPlanView( const RTD::VehicleBase* reference ,  const RTD::ReferenceEntityBase* target );
   /** 
	 *	This function returns true if the \e target is in front hemisphere of the \e reference in the.
	 *  plan view (2D, top down view).
     */ 
	bool RTDYNAMICS_EXPORT isInFrontHemisphereInPlanView( const RTD::ReferenceEntityBase* reference ,  const RTD::VehicleBase* target );

   /** 
	 *	This function returns true if the \e target is in front hemisphere of the \e reference in the.
	 *  plan view (2D, top down view).
     */ 
	bool isInFrontHemisphereInPlanView( const RTD::ReferenceEntityBase* reference ,  const RTD::ReferenceEntityBase* target );


    /** 
	 *  \e referencePos and \e referenceHPR define the position and orientation of a reference object in 3D space. 
	 *	This function returns true if the position given by \e targetXYZ is in right hemisphere of the reference object in the.
	 *  plan view (2D, top down view).
     */ 
	bool RTDYNAMICS_EXPORT isInRightHemisphereInPlanView( const smVec3r& referencePos, const smVec3r& referenceHPR , const smVec3r& targetXYZ );

    /** 
	 *	This function returns true if the \e target is in right hemisphere of the \e reference in the.
	 *  plan view (2D, top down view).
     */ 
	bool RTDYNAMICS_EXPORT isInRightHemisphereInPlanView( const RTD::VehicleBase* reference ,  const RTD::VehicleBase* target );

   /** 
	 *	This function returns true if the \e target is in right hemisphere of the \e reference in the.
	 *  plan view (2D, top down view).
     */ 

	bool RTDYNAMICS_EXPORT isInRightHemisphereInPlanView( const RTD::VehicleBase* reference ,  const RTD::ReferenceEntityBase* target );
   /** 
	 *	This function returns true if the \e target is in right hemisphere of the \e reference in the.
	 *  plan view (2D, top down view).
     */ 
	bool RTDYNAMICS_EXPORT isInRightHemisphereInPlanView( const RTD::ReferenceEntityBase* reference ,  const RTD::VehicleBase* target );

   /** 
	 *	This function returns true if the \e target is in right hemisphere of the \e reference in the.
	 *  plan view (2D, top down view).
     */ 
	bool RTDYNAMICS_EXPORT isInRightHemisphereInPlanView( const RTD::ReferenceEntityBase* reference ,  const RTD::ReferenceEntityBase* target );


	/**
		Solves for x,y in following equation system:
		a1 x + b1 y = c1
		a2 x + b2 y = c2

		When a solution is found true is returned. In case the equation system does not have a solution (i.e. parallel lines) false is returned. 
	*/
	bool RTDYNAMICS_EXPORT solveLinearEquationSystemOfTwoUnknowns( const smReal& a1 , const smReal&  b1 , const smReal& c1 , 
																   const smReal& a2 , const smReal&  b2 , const smReal& c2, 																	
																   smReal& x , smReal& y  );

	/**
        Returns the maximum random number that can be generated.
    */
	int RTDYNAMICS_EXPORT getMaxRandomNumber();
	
	/**
        Returns a random integer depending on the previosly generated random number.
    */
	int RTDYNAMICS_EXPORT getRandomInt( int lastRandomNumber );

	/**
        Creates an integer random seed using the input string.
    */
	int RTDYNAMICS_EXPORT makeRandomSeedFromName( const std::string& name );

	/**
        Returns the square of input.
    */
	template<typename T>
	T sqr( const T& val ) { return val*val; }


#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES
	/** This function is used internally. */
	smVec3r	RTDYNAMICS_EXPORT computeVelocityForWeave( const smVec3r& Vwithoutweave, smReal weaveTimer , smReal amplitude , smReal frequencyWeavePerMin );
#endif

	template<typename T>
	void appendDataToByteArray( const T& data , std::vector<unsigned char>& result )
	{
		size_t dataSize = sizeof( T );
		unsigned char byteArray[sizeof( T )];
		memcpy( byteArray ,&data , dataSize );
		for ( size_t i = 0 ; i < dataSize ; i++ )
		{
			result.push_back( byteArray[i] );
		}		
	}

	void RTDYNAMICS_EXPORT appendDataToByteArray( const std::string& data , std::vector<unsigned char>& result );


	template<typename T>
	unsigned int getDataFromByteArray( const std::vector<unsigned char>& input , unsigned int startFrom, T& data )
	{
		size_t dataSize = sizeof( T );
		unsigned char byteArray[sizeof( T )];		
		for ( size_t i = 0 ; i < dataSize ; i++ )
		{
			byteArray[i] = input[i+startFrom];
		}		

		memcpy( &data ,byteArray , dataSize );
		return (unsigned int) dataSize;
	}

	unsigned int RTDYNAMICS_EXPORT getStringFromByteArray( const std::vector<unsigned char>& input  , unsigned int startFrom , char limiter , std::string& str );

	template<typename T , typename TimeT>
	T moveToTargetValue( T inputValue , T targetValue , TimeT deltaTime , T speed )
	{
		if ( targetValue > inputValue )
		{
			inputValue += speed*deltaTime;
			if ( inputValue > targetValue ) 
				inputValue = targetValue;
		}
		else if ( targetValue < inputValue )
		{
			inputValue -= speed*deltaTime;
			if ( inputValue < targetValue ) 
				inputValue = targetValue;			
		}

		return inputValue;
	}


	template<typename T , unsigned int QueueSize=3>
	class	DataHistory
	{
	public:
		typedef T  data_type;

		DataHistory( const T& zeroValue):
		  m_queueSize(QueueSize),
			m_zeroValue( zeroValue )
		{
			if ( m_queueSize < 3 ) 
				m_queueSize = 3;
		}

		virtual void setQueueSize( unsigned int qs ) 
		{
			m_queueSize = qs;
			if ( m_queueSize < 3 ) 
				m_queueSize = 3;
		}

		virtual unsigned int getMaxQueueSize() const
		{
			return m_queueSize;
		}

		enum QueueSizePolicy
		{
			FixedSizeQueue,
			GrowingSizeQueue,

			No_QueueSizePolicy
		};

		virtual void	addData( const T& data , double timeStep, QueueSizePolicy sizePolicy = FixedSizeQueue )
		{
			// We do not accept two data points at the same time. DataHistory does not
			// support this and it can result in division by zero in some methods.
			if ( smMath::almostEqual( timeStep , 0.0 , 0.00000001 )) return;

			m_dataQueue.push_front( data );
			m_timeStepQueue.push_front( timeStep );

			if ( sizePolicy == FixedSizeQueue )
			{
			while (m_dataQueue.size() > m_queueSize )
			{
				m_dataQueue.pop_back();
				m_timeStepQueue.pop_back();
			}
		}
		}

		virtual void	replaceLastData( const T& data , double timeStep )
		{
			// We do not accept two data points at the same time. DataHistory does not
			// support this and it can result in division by zero in some methods.
			if ( smMath::almostEqual( timeStep , 0.0 , 0.00000001 )) return;

			if (m_dataQueue.size() > 0 )
			{
				m_dataQueue[0] = data;
				m_timeStepQueue[0] = timeStep ;
			}
			else
				addData( data , timeStep );
		}

		T getData( unsigned int index ) const
		{
			if ( index < m_dataQueue.size() )
			{
				return m_dataQueue[index];
			}

			return m_zeroValue;
		}

		double getTimeStep( unsigned int index ) const
		{
			if ( index < m_timeStepQueue.size() )
			{
				return m_timeStepQueue[index];
			}

			return 0.0;
		}

		T getCurrentValue() const
		{
			return getData(0);
		}

		T getDerivative() const
		{
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4244 )
#endif
			if ( m_dataQueue.size() < 2 ) return m_zeroValue;
			return static_cast<T>((getData(0)-getData(1)) / m_timeStepQueue[0]);
#ifdef _MSC_VER
#pragma warning( pop )
#endif
		}

		T getSecondDerivative() const
		{
			if ( m_dataQueue.size() < 3 ) return m_zeroValue;
			T firstDerivative1 = static_cast<T>((getData(0)-getData(1)) / m_timeStepQueue[0]);
			T firstDerivative2 = static_cast<T>((getData(1)-getData(2)) / m_timeStepQueue[1]);
			return static_cast<T>((firstDerivative1 - firstDerivative2) / m_timeStepQueue[0]);
		}

		// Returns the average value of the data in the data queue. Note that 
		// this is \b not a time average! Time steps are completely ignored in this method.
		T getSimpleAverage() const
		{
			// Calculate average
            T sum = m_zeroValue;
            typename std::deque<T>::const_iterator i = m_dataQueue.begin();
            for ( ; i != m_dataQueue.end(); i++ )
            {
                sum = sum + *i;
            }
            return sum / (smReal) m_dataQueue.size();
		}

		T getStandardDeviation() const
		{
			T mean = getSimpleAverage();
			T sum = m_zeroValue;

            typename std::deque<T>::const_iterator i = m_dataQueue.begin();
            for ( ; i != m_dataQueue.end(); i++ )
            {
                sum = sum + (*i - mean)*(*i - mean);
            }

			return smMath::sqrt( sum );
		}

		T getMax() const
		{
			if ( m_dataQueue.size() == 0 ) return m_zeroValue;
            T max = m_dataQueue[0];
            typename std::deque<T>::const_iterator i = m_dataQueue.begin();
            for ( ; i != m_dataQueue.end(); i++ )
            {
                if ( *i > max )
					max = *i;
            }

			return max;
		}

		T getMin() const
		{
			if ( m_dataQueue.size() == 0 ) return m_zeroValue;
            T min = m_dataQueue[0];
            typename std::deque<T>::const_iterator i = m_dataQueue.begin();
            for ( ; i != m_dataQueue.end(); i++ )
            {
                if ( *i < min )
					min = *i;
            }

			return min;
		}

		void	clear()
		{
			m_dataQueue.clear();
			m_timeStepQueue.clear();
		}

		unsigned int	getActualQueueLength() const
		{
			return (unsigned int)m_dataQueue.size();
		}

	protected:
		unsigned int		m_queueSize;
		std::deque<T>		m_dataQueue;
		T					m_zeroValue;
		std::deque<double>	m_timeStepQueue;
	};

	template<typename T , unsigned int QueueSize>
	std::ostream& operator<< (std::ostream & out, const DataHistory<T,QueueSize> & dh)
	{
		// Note that we stream the data starting from last element in the queue. We want that
		// the oldest data is at the leftmost position on the queue.
		for ( int i = (int) dh.getActualQueueLength()-1 ; i >= 0  ; i-- )
		{
			out << "(" << dh.getTimeStep(i) << ","  << dh.getData(i) << ")";
		}
		return out;
	}

	template<typename T , unsigned int QueueSize>
	std::istream& operator>> (std::istream & in, DataHistory<T,QueueSize> & dh)
	{
		dh.clear();

		std::string tmp;
		
		while ( !in.eof() )
		{
			char c =0;
			while ( c != '(' )
			{
				c = in.get();
				if ( in.eof() ) return in;
				if ( c != '(' && !isspace( c ) ) return in;
			}

			double time;
			
			in >> time;

			while ( c != ',' )
			{
				c = in.get();
				if ( in.eof() ) return in;
				if ( c != ',' && !isspace( c ) ) return in;
			}

			T data;
			in >> data ;

			while ( c != ')' )
			{
				c = in.get();
				if ( in.eof() ) return in;
				if ( c != ')' && !isspace( c ) ) return in;
			}

			if ( dh.getMaxQueueSize() == dh.getActualQueueLength() )
			{
				dh.setQueueSize( dh.getMaxQueueSize()+1);
			}

			dh.addData( data , time );
		}

		return in;
	}

	class RealDataHistory	: public DataHistory<smReal , 3>
	{
	public:
		RealDataHistory(): DataHistory<smReal , 3>( 0.0f ) {}

		virtual void	addDataAsAngle( const smReal& data , double timeStep )
		{
			if ( m_dataQueue.size() > 0 )
			{
				addData( getCurrentValue() - RTD::smallestAngleDifference(getCurrentValue() , data ) , timeStep );
			}
			else 
			{
				addData(  data , timeStep );
			}	
		}

	};

	
	class DoubleDataHistory	: public DataHistory<smDouble , 3>
	{
	public:
		DoubleDataHistory(): DataHistory<smDouble , 3>( 0.0 ) {}

		virtual void	addDataAsAngle( const smDouble& data , double timeStep )
		{
			if ( m_dataQueue.size() > 0 )
			{
				addData( getCurrentValue() + RTD::smallestAngleDifference(getCurrentValue() , data ) , timeStep );
			}
			else 
			{
				addData(  data , timeStep );
			}	
		}

	};

	
	class Vec2rDataHistory	: public DataHistory<smVec2r , 3>
	{
	public:
		Vec2rDataHistory(): DataHistory<smVec2r , 3>( smVec2r(0.0f,0.0f) ) {}
	};

	
	class Vec3rDataHistory	: public DataHistory<smVec3r , 3>
	{
	public:
		Vec3rDataHistory(): DataHistory<smVec3r , 3>( smVec3r(0.0f,0.0f,0.0f) ) {}
	};

	
	class Vec4rDataHistory	: public DataHistory<smVec4r , 3>
	{
	public:
		Vec4rDataHistory(): DataHistory<smVec4r , 3>( smVec4r(0.0f,0.0f,0.0f,0.0f) ) {}
	};

	class Vec2dDataHistory	: public DataHistory<smVec2d , 3>
	{
	public:
		Vec2dDataHistory(): DataHistory<smVec2d , 3>( smVec2d(0.0f,0.0f) ) {}
	};

	
	class Vec3dDataHistory	: public DataHistory<smVec3d , 3>
	{
	public:
		Vec3dDataHistory(): DataHistory<smVec3d , 3>( smVec3d(0.0f,0.0f,0.0f) ) {}
	};

	
	class Vec4dDataHistory	: public DataHistory<smVec4d , 3>
	{
	public:
		Vec4dDataHistory(): DataHistory<smVec4d , 3>( smVec4d(0.0f,0.0f,0.0f,0.0f) ) {}
	};
}
