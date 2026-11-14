/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/VehicleBase.h"
#include "RTD/MBDBody.h"

namespace RTD
{
    class World ;

    /**
        This callback is called by the MBDVehicle when an object that is attached to it is detached.
    */
	class RTDYNAMICS_EXPORT ObjectDetachedCallback 
	{
		public:
            /** \param object This is the object from which another object is detached.  
                \param detachedObj This is the object that is detached.  
            */
			virtual void objectDetached(MBDVehicle* object, MBDVehicle* detachedObj) = 0 ;

		protected:
			/** Destructor of the class. */
			virtual ~ObjectDetachedCallback(){}
	};

	/** MBDVehicle class serves as a base class for all vehicle models
	*	which use a multi-body-dynamics model for simulation of mechanical
	*	components on the vehicle such as the landing gear.
	*
	*	This class is not supposed to be directly used, it solely serves
	*	as a base class.
	*/
	class RTDYNAMICS_EXPORT MBDVehicle : public RTD::VehicleBase
	{
	public:
        /** Constructor of the class. */
		MBDVehicle(World* world, const std::string& name) ;

        /** Destructor of the class. */
		virtual ~MBDVehicle() ;


		/** Enables/disables this vehicle. When disabled the vehicle remains
		 *  unchanged during simulation.
		 */
		virtual void setEnabled( bool enabled );

		/** Returns true if this vehicle is enabled.   */
		virtual bool isEnabled( );


		

		/**	preUpdate is called by RTD::World::update() at every step of the simulation
		*	before the multi body dynamics simulation step.
		*/
		virtual void preUpdate( smTime timestep ) ;

		/**	postUpdate is called by RTD::World::update() at every step of the simulation
		*	after the multi body dynamics simulation step.
		*/
		virtual void postUpdate( smTime timestep );

		/** This method updates all the force generators and computes the
		*	total sum of forces and torques on this vehicle.
		*
		*	FGUpdate is called by RTD::World::update() at every step of the simulation.
		*/
		virtual void FGUpdate( smTime timestep ) ;
		


		/** Forces and torques acting on the vehicle can be scaled easily
		*	with this method for flight modeling debugging purposes.
		*
		*	Under normal circumstances all force and torques are set to 1.0 which
		*	means it is disabled and it is not necessary to call this method.
		*/
		virtual void		setForceTorqueScale(smReal forceScale, smReal torqueScale ) ;

		/** Returns the multi body dynamics body object. */
		MBDBody&	getMBDBody() { return m_rbBody; }

		/** Returns the multi body dynamics body object. */
		const MBDBody&	getMBDBody() const { return m_rbBody; }


		/**
		*   Attaches given object to this vehicle at given position and orientation. If the object that is
		*   to be attached is already attached to another object, it is detached first from it.
		*
		*   \param obj 				object to be attached. This object should be in the same simulation World
		*							as the parent object.
		*  
		*   \param xyz 				position of the attached object relative to
		*                           this object. In meters.
		*  
		*   \param hpr 				orientation of the attached object relative
		*                           to this object. In degrees.
		*  
		*   \param deleteWithParent If this argument is true, when this object is deleted, attached object will also be deleted. Otherwise
		*   						attached object will be detached but will remain in the simulation.
		*/
		virtual bool	attachObject( RTD::MBDVehicle* obj , const smVec3r& xyz , const smVec3r& hpr , bool deleteWithParent );

		/** Detaches given object from the parent object.  */
		virtual void	detachObject( RTD::MBDVehicle* obj );

		/** Returns the true if this object is attached to specified vehicle
		 *  ,\e obj
		 */
		virtual bool	isObjectAttached( const RTD::MBDVehicle* obj ) const;

		/** Returns attached object at given index. If index is out of bounds NULL is returned. */
		const RTD::MBDVehicle*	getAttachedObject( unsigned int index ) const;

		/** Returns attached object at given index. If index is out of bounds NULL is returned. */
		RTD::MBDVehicle*	getAttachedObject( unsigned int index );

		/** Returns attached object at given index. If index is out of bounds NULL is returned. 
			In \e xyz and \e hpr local offset (i.e. attachment point on the parent object) 
			of the attached object is returned. \e deleteWithParent is returned true of current setting is that the 
			attached object will be deleted	when parent is deleted.*/
		RTD::MBDVehicle*	getAttachedObject( unsigned int index , smVec3r& xyz , smVec3r& hpr , bool& deleteWithParent );
		
		/** Returns attached object at given index. If index is out of bounds NULL is returned. 
			In \e xyz and \e hpr local offset (i.e. attachment point on the parent object) 
			of the attached object is returned. \e deleteWithParent is returned true of current setting is that the 
			attached object will be deleted	when parent is deleted.*/
		const RTD::MBDVehicle*	getAttachedObject( unsigned int index , smVec3r& xyz , smVec3r& hpr , bool& deleteWithParent ) const;
		

		/** Returns number of objects which are attached to this object. */
		unsigned int		getNumAttachedObjects() const	{	return (unsigned int)m_attachedObjects.getNum();	}

		/** Returns the object to which this object is attached. If this object is not attached
			to any other objects, NULL is returned.*/
		RTD::MBDVehicle*	getParentObject();

		/** Returns the object to which this object is attached. If this object is not attached
			to any other objects, NULL is returned.*/
		const RTD::MBDVehicle*	getParentObject() const;

        /** */
		virtual void					setMainBodyMass( smDouble mass );

		/** */
		virtual void					setMainBodyMomentsOfInertia( const smVec4d& moi );

		/** */
		virtual void					setMainBodyCenterOfGravityBCS( const smVec3d& cogBCS );

        /** */
		virtual void					serialize( std::vector<unsigned char>& result );
		
        /** */
        virtual bool					deserialize( const std::vector<unsigned char>& serializedArray, std::vector<unsigned char>& remainder );

        /** */
		virtual void					reset();


        /**
            This method equalizes the values of all simulation variables to the values of corresponding
            simulation variables of \e rhs. The simulation variable names and hierarchies must be the same.

            After the simulation variables are set applySimVarsToMBD() is called to update the multi-body engine
            state.

            acquireState() can be used to copy the state of a vehicle to another.
        */
		virtual void					acquireState( const SimulationVariableUser& rhs );

        /**
            This method reads the values of the simulation variables and applies them to the underlying
            multi-body dynamics engine.
        */
		virtual void					applySimVarsToMBD( bool stateConsistent ) ;

        /**
            This method retrieves the state of the multi-body dynamics state and update the simulation variables
            with this information.
        */
		virtual void					updateSimVarsFromMBD() ;

        /**
            Sets an object detached callback.
        */
		virtual void					setObjectDetachedCallback( ObjectDetachedCallback* cb );
		
        /**
            Returns the object detached callback if any available. NULL is returned if a callback is not set.
        */
        virtual ObjectDetachedCallback*	getObjectDetachedCallback(  );

	protected:
		World*				m_rbWorld ;
		mutable MBDBody		m_rbBody ;
		smVec3d				m_prevSpd;
		smVec3d				m_prevRotSpeed ;

		smReal				m_forceScale ;
		smReal				m_torqueScale ;
		ObjectDetachedCallback*	m_objDetachCB ;

		struct AttachmentInfo
		{
			AttachmentInfo()
			{
				m_joint		= NULL;
				m_object	= NULL;
				m_relXYZ.set(0.0f,0.0f,0.0f);
				m_relHPR.set(0.0f,0.0f,0.0f);
			}

			bool	m_deleteWithParent;
			void*	m_joint;
			RTD::MBDVehicle* m_object;
			smVec3r	m_relXYZ;
			smVec3r	m_relHPR;

		};
		
		RTD::MBDVehicle* m_parent;
		smManager<AttachmentInfo>	m_attachedObjects;

		virtual void computeMassProperties() const;

		virtual void onExternalPhysicalStateChange();

		virtual void create();

	};



}
