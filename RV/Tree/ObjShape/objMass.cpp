// 3D physics - mass, inercia, momentum
// (C) Ondrej Spanel, Suma

#include "El/elementpch.hpp"
//FLY#include "global.hpp"

#if _MSC_VER < 1300
#include <fstream.h>
#include <strstrea.h>
#else
#include <fstream>
#include <strstream>
#endif
//#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <Es/Types/pointers.hpp>
//#include <macros.h>
//#include <stdc\fileutil.h>

#include "objobje.hpp"

void PointAttrib<float>::Clear( int i )
{
    _attrib[i]=0;
}


void ObjectData::SetSelectionMass( double mass, bool constTotal )
{
    int i;
    double total=0;
    double rest=0;
    double change=0;
    int nSel=0,nRest=0;
    _mass.Validate(NPoints());
    for( i=0; i<NPoints(); i++ )
    {
        if( PointSelected(i) ) total+=_mass[i],nSel++;
        else rest+=_mass[i],nRest++;
    }
    if( nSel<=0 ) return;
    if( total<1e-3 )
    {
        double vertexMass=mass/nSel;
        for( i=0; i<NPoints(); i++ ) if( PointSelected(i) ) _mass[i]=vertexMass;
    }
    else
    {
        float massRatio=mass/total;
        for( i=0; i<NPoints(); i++ ) if( PointSelected(i) )
        {
            double newMass=massRatio*_mass[i];
            change+=newMass-_mass[i];
            _mass[i]=newMass;
        }
    }
    if( constTotal && nRest>0 )
    { // leave total mass unchanged
        float restRatio=(rest-change)/rest;
        if( restRatio<=0 ) return;
        for( i=0; i<NPoints(); i++ ) if( !PointSelected(i) )
        {
            double newMass=restRatio*_mass[i];
            //change+=newMass-_mass[i];
            _mass[i]=newMass;
        }
    }
    SetDirty();
}

double ObjectData::GetSelectionMass() const
{
    double total=0;
    for( int i=0; i<_mass.N(); i++ ) if( PointSelected(i) ) total+=_mass[i];
    return total;
}

double ObjectData::GetTotalMass() const
{
    double total=0;
    for( int i=0; i<_mass.N(); i++ ) total+=_mass[i];
    return total;
}

const char *ObjectData::GetNamedProp( const char *name ) const
{
    for( int i=0; i<MAX_NAMED_PROP; i++ )
    {
        if( _namedProp[i] )
        {
            if( !strcmpi(_namedProp[i]->Name(),name) ) return _namedProp[i]->Value();
        }
    }
    return NULL;
}

bool ObjectData::SetNamedProp( const char *name, const char *value )
{
    int i;
    for( i=0; i<MAX_NAMED_PROP; i++ )
    {
        if( _namedProp[i] )
        {
            if( !strcmpi(_namedProp[i]->Name(),name) )
            {
                _namedProp[i]->SetValue(value);
                 SetDirty();
                 return true;
           }
        }
    }
    for( i=0; i<MAX_NAMED_PROP; i++ )
    {
        if( !_namedProp[i] )
        {
            _namedProp[i]=new NamedProperty(name,value);
             SetDirty();
            return true;
        }
    }
    return false;
}

const char *ObjectData::GetNamedProp( int i ) const
{
    if( !_namedProp[i] ) return NULL;
    return _namedProp[i]->Name();
}

bool ObjectData::DeleteNamedProp( const char *name )
{
    int i;
    for( i=0; i<MAX_NAMED_PROP; i++ )
    {
        if( _namedProp[i] )
        {
            if( !strcmpi(_namedProp[i]->Name(),name) )
            {
                 delete _namedProp[i];
                 _namedProp[i]=NULL;
                 SetDirty();
                 return true;
           }
        }
    }
     return false;
}

bool ObjectData::RenameNamedProp( const char *name, const char *newname )
{
    if( GetNamedProp(newname) ) return false;
    int i;
    for( i=0; i<MAX_NAMED_PROP; i++ )
    {
        if( _namedProp[i] )
        {
            if( !strcmpi(_namedProp[i]->Name(),name) )
            {
                 _namedProp[i]->SetName(newname);
                 SetDirty();
                 return true;
           }
        }
    }
    return false;
}


