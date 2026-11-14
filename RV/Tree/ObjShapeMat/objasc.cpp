#include "objshape.h"
// 3D model import
// (C) Ondrej Spanel, Suma, 6/1996

// import formats: 3d studio ASCII

#include "wpch.hpp"
#include "global.hpp"

#include <fstream>
#include <strstream>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <Es/Types/pointers.hpp>
//#include <macros.h>
//#include <stdc\fileutil.h>

#include "objobje.hpp"

//    example: cube
/*
Named Object: "Box01"
Tri-mesh, Vertices: 8     Faces: 12
Vertex list:
Vertex 0: X: -17,710842     Y: -51,927708     Z: 0,000000
Vertex 1: X: 82,289154     Y: -51,927708     Z: 0,000000
Vertex 2: X: -17,710842     Y: 48,072292     Z: 0,000000
Vertex 3: X: 82,289154     Y: 48,072292     Z: 0,000000
Vertex 4: X: -17,710842     Y: -51,927708     Z: 50,000000
Vertex 5: X: 82,289154     Y: -51,927708     Z: 50,000000
Vertex 6: X: -17,710842     Y: 48,072292     Z: 50,000000
Vertex 7: X: 82,289154     Y: 48,072292     Z: 50,000000
Face list:
Face 0:    A:0 B:2 C:3 AB:1 BC:1 CA:0
Smoothing: 1 
Face 1:    A:3 B:1 C:0 AB:1 BC:1 CA:0
Smoothing: 2 
Face 2:    A:4 B:5 C:7 AB:1 BC:1 CA:0
Smoothing: 1 
Face 3:    A:7 B:6 C:4 AB:1 BC:1 CA:0
Smoothing: 2 
Face 4:    A:0 B:1 C:5 AB:1 BC:1 CA:0
Smoothing: 3 
Face 5:    A:5 B:4 C:0 AB:1 BC:1 CA:0
Smoothing: 4 
Face 6:    A:1 B:3 C:7 AB:1 BC:1 CA:0
Smoothing: 4 
Face 7:    A:7 B:5 C:1 AB:1 BC:1 CA:0
Smoothing: 5 
Face 8:    A:3 B:2 C:6 AB:1 BC:1 CA:0
Smoothing: 3 
Face 9:    A:6 B:7 C:3 AB:1 BC:1 CA:0
Smoothing: 6 
Face 10:    A:2 B:0 C:4 AB:1 BC:1 CA:0
Smoothing: 5 
Face 11:    A:4 B:6 C:2 AB:1 BC:1 CA:0
Smoothing: 7 
*/

const char *GetLine( istream &f )
  {
  static char buf[512];
  f.getline(buf,sizeof(buf)-1);
  if( f.fail() ) return NULL;
  if( f.eof() ) return NULL;
  // remove eol
  char *eol=strchr(buf,'\n');
  if( eol ) *eol=0;
  return buf;
  }

//--------------------------------------------------

inline int StrCmpBegin( const char *line, const char *beg )
  {
  int lenBeg=strlen(beg);
  return strncmp(line,beg,lenBeg);
  }

//--------------------------------------------------

static bool Skip( const char *&line, const char *beg )
  {
  if( StrCmpBegin(line,beg) ) return false;
  line+=strlen(beg);
  return true;
  }

//--------------------------------------------------

static bool SkipSpaces( const char *&line )
  {
  while( *line && isspace(*line) ) line++;
  return true;
  }

//--------------------------------------------------

static int GetInt(  const char *&line )
  {
  if( !SkipSpaces(line) ) return 0;
  return strtol(line,(char **)&line,10);
  }

//--------------------------------------------------

static double GetFloat(  const char *&line )
  {
  // caution: numbers may contain ',' instead of '.'
  if( !SkipSpaces(line) ) return 0;
  // get integer part of number
  bool negative=false;
  if( *line=='-' ) negative=true,line++;
  float result=strtol(line,(char **)&line,10);
  if( *line=='.' || *line==',' )
    {
    line++;
    double decPos=1;
    while( isdigit(*line) )
      {
      decPos*=10;
      result+=(*line-'0')/decPos;
      line++;
      }
    }
  if( negative ) result=-result;
  return result;
  }

//--------------------------------------------------

const float AscScale=0.01;
const float InvAscScale=1.0/AscScale;

int ObjectData::LoadGenAsc( istream &f )
  {
  DoDestruct();
  DoConstruct();
  
  // scan until you reach line starting with "Tri-mesh"
  
  const char *line;
  for(;;)
    {
    line=GetLine(f);
    if( !line ) return -1;
    if( Skip(line,"Tri-mesh,") ) break;    
    }
  
  _faces.Clear();
  _points.Clear();
  _normals.Clear();
  
  // first significant line  has format:
  //Tri-mesh, Vertices: 8     Faces: 12
  if( !SkipSpaces(line) ) return -1;
  if( !Skip(line,"Vertices:") ) return -1;
  int nVertices=GetInt(line);
  if( !SkipSpaces(line) ) return -1;
  if( !Skip(line,"Faces:") ) return -1;
  int nFaces=GetInt(line);
  line=GetLine(f);
  if( !line ) return -1;
  //Vertex list:
  if( !Skip(line,"Vertex list:") ) return -1;
  int i;
  _points.Resize(nVertices);
  _faces.Resize(nFaces);
  _mass.Validate(nVertices);
  for( i=0; i<nVertices; i++ )
    {
    //Vertex 0: X: -17,710842     Y: -51,927708     Z: 0,000000
    PosT pos;
    line=GetLine(f);
    if( !line ) return -1;
    if( !Skip(line,"Vertex") ) return -1;
    int chkI=GetInt(line);
    if( chkI!=i ) return -1;
    if( !Skip(line,":") ) return -1;
    if( !SkipSpaces(line) ) return -1;
    if( !Skip(line,"X:") ) return -1;
    pos[0]=GetFloat(line)*AscScale;
    if( !SkipSpaces(line) ) return -1;
    if( !Skip(line,"Y:") ) return -1;
    pos[2]=GetFloat(line)*AscScale;
    if( !SkipSpaces(line) ) return -1;
    if( !Skip(line,"Z:") ) return -1;
    pos[1]=GetFloat(line)*AscScale;
    pos.flags=0;
    // save this point
    Point(i)=pos;
    //_mass[i]=0;
    }
  
  //Face list:
  line=GetLine(f);
  if( !line ) return -1;
  if( !Skip(line,"Face list:") ) return -1;
  line=GetLine(f);
  for( i=0; i<nFaces; i++ )
    {
    if( !line ) return -1;
    //Face 0:    A:0 B:2 C:3 AB:1 BC:1 CA:0
    FaceT face(this);
    if( !Skip(line,"Face") ) return -1;
    int chkI=GetInt(line);
    if( chkI!=i ) return -1;
    if( !Skip(line,":") ) return -1;
    face.SetN(3);
    if( !SkipSpaces(line) ) return -1;
    if( !Skip(line,"A:") ) return -1;
    face.SetPoint(0,GetInt(line));
    if( !SkipSpaces(line) ) return -1;
    if( !Skip(line,"B:") ) return -1;
    face.SetPoint(1,GetInt(line));
    if( !SkipSpaces(line) ) return -1;
    if( !Skip(line,"C:") ) return -1;
    face.SetPoint(2,GetInt(line));
    // use pairs to mark smooth/sharp edges
    bool sharp;
    if( !SkipSpaces(line) ) return -1;
    if( !Skip(line,"AB:") ) return -1;
    sharp=GetInt(line)!=0;
    if( sharp ) AddSharpEdge(face.GetPoint(0),face.GetPoint(2));
    if( !SkipSpaces(line) ) return -1;
    if( !Skip(line,"BC:") ) return -1;
    sharp=GetInt(line)!=0;
    if( sharp ) AddSharpEdge(face.GetPoint(2),face.GetPoint(1));
    if( !SkipSpaces(line) ) return -1;
    if( !Skip(line,"CA:") ) return -1;
    sharp=GetInt(line)!=0;
    if( sharp ) AddSharpEdge(face.GetPoint(1),face.GetPoint(0));
    // use default texture
    for( int v=0; v<3; v++ )
      {
      face.SetNormal(v,0);
      face.SetUV(v,0,0);      
      }
    face.SetFlags(0);
//    face.vTexture="data\\default.pac";
    //Smoothing: 1
    line=GetLine(f);
    if( line && Skip(line,"Smoothing:") )
      {
      // skip smoothing line - start another face
      line=GetLine(f);
      }
    face.CopyFaceTo(i);    
    }
  Squarize(true);
  return 0;
  }

//--------------------------------------------------

int ObjectData::SaveGenAsc( ostream &f )
  {
  // caution: object must be composed only from triangles
  f << '\n';
  f << "Named Object: \"object\"\n";
  f << "Tri-mesh, Vertices: " << NPoints() << " Faces: " << NFaces() << '\n';
  f << "Vertex list:\n";
  int i;
  f.setf(ios::fixed,ios::floatfield);
  for( i=0; i<NPoints(); i++ )
    {
    const PosT  &pos=Point(i);
    f << "Vertex " << i << ": ";
    f << "X: " << pos.X()*InvAscScale  << " ";
    f << "Y: " << pos.Z()*InvAscScale << " ";
    f << "Z: " << pos.Y()*InvAscScale << '\n';
    }
  f << "Face list:\n";
  for( i=0; i<NFaces(); i++ )
    {
    const FaceT face(this,i);
    f << "Face " << i << ": ";
    f << "A:" << face.GetPoint(0) << " ";
    f << "B:" << face.GetPoint(1) << " ";
    f << "C:" << face.GetPoint(2) << " ";
    bool sharp=true;
    sharp=FindSharpEdge(face.GetPoint(0),face.GetPoint(2))>=0;
    f << "AB: " << (int)sharp << " ";
    sharp=FindSharpEdge(face.GetPoint(2),face.GetPoint(1))>=0;
    f << "BC: " << (int)sharp << " ";
    sharp=FindSharpEdge(face.GetPoint(1),face.GetPoint(0))>=0;
    f << "CA: " << (int)sharp << '\n';
    f << "Smoothing: " << 1 << '\n';
    }
  if( f.fail() ) return -1;
  return 0;
  }

//--------------------------------------------------

int ObjectData::LoadGenAsc( WFilePath name )
  {
  ifstream f;
  f.open(name,ios::in);
  if( f.fail() ) return -1;
  return LoadGenAsc(f);
  }

//--------------------------------------------------

int ObjectData::SaveGenAsc( WFilePath name )
  {
  SRef<ObjectData> temp=new ObjectData();
  temp[0]=*this;
  temp[0].Triangulate(true);
  ofstream f;
  f.open(name,ios::out);
  if( f.fail() ) return -1;
  return temp[0].SaveGenAsc(f);
  }

//--------------------------------------------------

