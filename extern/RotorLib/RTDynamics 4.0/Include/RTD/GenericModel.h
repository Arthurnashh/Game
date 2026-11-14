/*
(c) Copyright 2011, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "Common.h"
#include "ParamDb.h"
#include "Physics/smGraph2D.h"
#include "LookUpTable.h"
#include "GenericModelFunction.h"

namespace RTD
{   

    /*  GenericModel class and associated generic model functions (e.g. GenericModelCurveLookUp) provide a tool set
        for mathematical model development without C++ programming. It is used by model components (for example by 
        GenericControlSystem) to enable the user extend certain parts of the dynamics model using XML. 

        \see Generic Mathematical Model section in the Programmer's Guide for more information


    */
    class GenericModel : public RTD::SimulationVariableUser
    {
    public:
        struct VariableDecleration
        {
            VariableDecleration();
            virtual ~VariableDecleration();
            void    setSimVar();
            void    setInterchangeMemory();

            bool                    isCreatedVariable;
            SimulationVariableBase* simVar;
            int                     member;
            std::string             alias;
            double                  interchangeMemory;
        };


        GenericModel (const std::string& name , SimulationVariableUser* parent , SimulationVariableUser* simVarTreeRoot );
        virtual ~GenericModel();

        SimulationVariableUser* getSimVarTreeRoot();
        const SimulationVariableUser* getSimVarTreeRoot() const;

        bool    readParamStr( const std::string& parameterString , std::string& error);
        virtual void update( smDouble timestep );
        virtual void reset();

        void    updateInterchangeMemoriesFromSimVars();
        void    udateSimVarsFromInterchangeMemories();
        
        unsigned int    getNumVariableDeclerations() const;
        const VariableDecleration*    getVariableDecleration(unsigned int) const;
        VariableDecleration*          getVariableDecleration(unsigned int);
        
        SimulationVariableBase*             getSimVarByAlias( const std::string& alias );
        const SimulationVariableBase*       getSimVarByAlias( const std::string& alias )  const;
        bool                                getSimVarAndMemberIndexByAlias( const std::string& alias , SimulationVariableBase*& var , int& memberIndex  );

        smGraph2D*      getCurveByName( const std::string& curveName );
        const smGraph2D*      getCurveByName( const std::string& curveName ) const;

        LookUpTable*    getTableByName( const std::string& tableName );
        const LookUpTable*    getTableByName( const std::string& tableName ) const;

    protected:
        void processDataSection( const std::string& parameterString);
        void processVariablesSection( const std::string& parameterString);
        SimulationVariableBase* createStateVariable( const std::string& dataTypeName , RTD::SimulationVariableUser* owner , const std::string& name , std::string& error );

        std::vector<VariableDecleration*>    m_variableDeclerations;
        std::vector<GenericModelFunction*>  m_functions;
        SimulationVariableUser* m_simVarTreeRoot;

        std::map<std::string, smGraph2D>  m_curves;
        std::map<std::string, LookUpTable>  m_tables;
    };
}