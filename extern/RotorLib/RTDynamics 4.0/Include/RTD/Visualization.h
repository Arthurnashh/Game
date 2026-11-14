/*
(c) Copyright 2008, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "RTD/BotBase.h"
#include "RTD/Common.h"

namespace RTD
{
    class PIDController;
    class PIDControllerD;

    /** Visualization class contains data which can be used to visualize the
        physical state of a vehicle or internal working of a control system.
     */
    class RTDYNAMICS_EXPORT Visualization
    {
    public:
		/**
            This struct defines a single vector to be rendered.

            \see addVectorToQueue
            \see getVectorQueue

        */
		struct RTDYNAMICS_EXPORT VisualizationVector
		{
			VisualizationVector();

            /** The time of creation of this vector. */
			smDouble m_simulationTime;

            /** Position of the vector in world coordinates. This vector is in meters. */
			smVec3r	m_position;

            /** Vector to be visualized.  */
			smVec3r m_vector;

            /** Color of the vector at its root. Vector color will be interpolated between m_startColor and m_endColor.  */
			smVec4r m_startColor;

            /** Color of the vector at its tip. Vector color will be interpolated between m_startColor and m_endColor.  */
			smVec4r m_endColor;
		};

		/** Constructor of the class. \e vehicle is the vehicle data of which 
			will be visualized. */
        Visualization(VehicleBase* vehicle);

        /** The update method is regularly called by the VehicleBase::update
         *  automatically. Calling this method directly is not necessary. */
        virtual void        update();

        /** The reset method is called by the VehicleBase::reset
         *  automatically. Calling this method directly will clear the stored
         *  data in this class, most importantly the deques.
         */
        virtual void        reset();

        /** Visualization can be disabled with setEnabled method to stop
         *  Visualization class recording data. This would reduce the memory
         *  usage.
         */
        virtual void        setEnabled( bool status ) { m_enabled = status ; }

		/**
			Returns true if visualization is enabled.
		*/
		bool				isEnabled() const	{ return m_enabled;	}

        /** Coloring adds transparent fade-in and fade-out coloring. Depending
         *  on the ribbon length, the coloring has an additional CPU overhead
         *  which is negligable in optimized builds but can be observed in debug
         *  builds.
         *  
         *  setColoring can be used to turn off coloring, which is enabled by
         *  default.
         */
        virtual void        setColoring( bool status ) { m_coloring = status ; }

        /** Use this method to specify the distance between the left and the
         *  right wing tip. */
        virtual void        setWingSpan( float span ) { m_wingSpan = span ; }

        /** Use this method to specify the total length of the ribbon paths */
        virtual void        setRibbonLength( float length ) { m_ribbonLength = length ; }

        /** Use this method to specify the distance between the center of
         *  gravity (CoG) of the vehicle and the first point of the ribbon. This
         *  distance is used to prevent the 3d ribbon collide with the vehicle
         *  3d model.
         *  
         *  Current implementation of the getColors method will return
         *  transparent colors for the region between CoG and first ribbon.
         */
        virtual void        setOffsetToVehicle( float offset ) { m_offsetToVehicle = offset ; }

        /** getLeftWingTipPath returns an std::deque which contains the
         *  positions of the left wing tip over the time in world coordinate
         *  system.
         *  
         *  The first entry in the deque is the newest.
         */
        std::deque<smVec3r>&   getLeftWingTipPath()    { return m_leftWingTipPath; }

        /** getRightWingTipPath returns an std::deque which contains the
         *  positions of the right wing tip over the time in world coordinate
         *  system.
         *  
         *  The first entry in the deque is the newest.
         */
        std::deque<smVec3r>&   getRightWingTipPath()   { return m_rightWingTipPath; }

        /** The Visualization maintains a color list which can be used to
         *  visualize a flight path (ribbon). The deque returned by the
         *  getColors method can be used as a hint.
         *  
         *  The first entry in the deque is the newest.
         */
        std::deque<smVec4r>&   getColors()             { return m_colors; }

		/**
            Adds a new vector to the vector queue. 
            
            The vectors with a time stamp older than the current simulation time will be deleted automatically during update().
        */
		void addVectorToQueue( const smVec3r& positionWCS, const smVec3r& vectorWCS , const smVec4r& startColor,const smVec4r& endColor );

		/**
            Returns the current vector queue.
        */
		const std::deque<VisualizationVector>& getVectorQueue() const;


    protected:


        VehicleBase*                m_vehicle ;
		

		std::deque<VisualizationVector>	m_vectors;

        bool                        m_enabled;
        smReal                      m_curLength;
        std::deque<smVec3r>         m_leftWingTipPath;
        std::deque<smVec3r>         m_rightWingTipPath;
        std::deque<smVec4r>         m_colors;

        bool                        m_coloring;
        smReal                      m_wingSpan;
        smReal                      m_ribbonLength;
        smReal                      m_offsetToVehicle;
    };
}
