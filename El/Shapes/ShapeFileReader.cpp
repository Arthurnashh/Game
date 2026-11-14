#include <Es/essencepch.hpp>
#include "StdAfx.h"
#include "ShapePoint.h"
#include "ShapeMultiPoint.h"
#include "ShapePoly.h"
#include "ShapePolygon.h"
#include "ShapePolyLine.h"
#include "Vertex.h"
#include "ShapeList.h"
#include "shapefilereader.h"

//#include <fstream>

namespace ShapeFiles
{
	using namespace std;

	template<class T>
    bool ReadBig(QIStream& input, T& t)
	{
		char* c = reinterpret_cast<char*>(&t);
		for (int i = sizeof(t) - 1; i >= 0; i--)
		{
			input.read(c + i, 1);
			if (input.fail()) return false;
		}
		return true;
	}

	template<class T>
    bool ReadLittle(QIStream& input, T& t)
	{
		char* c = reinterpret_cast<char*>(&t);
		input.read(c, sizeof(t));
		if (input.fail()) return false;
		return true;
	}

  static ShapeFileReader::ShapeReaderError ReadShapeHeader(QIStream &input,ShapeFileReader::ShapeHeader &hdr)
  {
    char unused[5*4];
    if (!ReadBig(input,hdr.fileCode)) return ShapeFileReader::sheReadError;
    input.read(unused,sizeof(unused));
    if (!ReadBig(input,hdr.fileLength)) return ShapeFileReader::sheReadError;
    if (!ReadLittle(input,hdr.version)) return ShapeFileReader::sheReadError;
    if (!ReadLittle(input,hdr.shapeType)) return ShapeFileReader::sheReadError;
    if (!ReadLittle(input,hdr.Xmin)) return ShapeFileReader::sheReadError;
    if (!ReadLittle(input,hdr.Ymin)) return ShapeFileReader::sheReadError;
    if (!ReadLittle(input,hdr.Xmax)) return ShapeFileReader::sheReadError;
    if (!ReadLittle(input,hdr.Ymax)) return ShapeFileReader::sheReadError;
    if (!ReadLittle(input,hdr.Zmin)) return ShapeFileReader::sheReadError;
    if (!ReadLittle(input,hdr.Zmax)) return ShapeFileReader::sheReadError;
    if (!ReadLittle(input,hdr.Mmin)) return ShapeFileReader::sheReadError;
    if (!ReadLittle(input,hdr.Mmax)) return ShapeFileReader::sheReadError;    
    return ShapeFileReader::sheOk;  
  }

  struct RecordHeader
  {
    unsigned long recordNumber;
    unsigned long contentLength;
    unsigned long shapeType;
  };

  static bool ReadRecordHeader(QIStream &input, RecordHeader &hdr)
  {
    if (!ReadBig(input,hdr.recordNumber)) return false;
    if (!ReadBig(input,hdr.contentLength)) return false;
    if (!ReadLittle(input,hdr.shapeType)) return false;
    return true;
  }

  static unsigned int AddVertex(VertexArray &vxArray, double x, double y, AutoArray<unsigned int> *vxIndexes=0)
  {
    unsigned int res=vxArray.AddVertex(DVertex(x,y));
    if (vxIndexes) vxIndexes->Add(res);
    return res;
  }

  static ShapeFileReader::ShapeReaderError ReadPoint(QIStream &input, VertexArray &vxArray, ShapePoint &pt, AutoArray<unsigned int> *vxIndexes=0)
  {
    double x,y;
    if (!ReadLittle(input,x)) return ShapeFileReader::sheReadError;
    if (!ReadLittle(input,y)) return ShapeFileReader::sheReadError;
    unsigned int id=AddVertex(vxArray,x,y,vxIndexes);
    pt=ShapePoint(id,&vxArray);
    return ShapeFileReader::sheOk;
  }

  static ShapeFileReader::ShapeReaderError ReadMOrZ(QIStream &input, VertexArray &vxArray, const Array<unsigned int> &vxIndexes, bool readZ, bool readRange)
  {
    if (readRange)
    {
      double range[2];
      if (!ReadLittle(input,range)) return ShapeFileReader::sheReadError;
    }
    if (readZ) 
    {
      for (int i=0;i<vxIndexes.Size();i++) if (vxIndexes[i]!=-1)
      {
        DVertex vect=vxArray[vxIndexes[i]];
        if (!ReadLittle(input,vect.z)) return ShapeFileReader::sheReadError;
        vxArray.SetAt(vxIndexes[i],vect);
      }
      else
      {
        double unused;
        if (!ReadLittle(input,unused)) return ShapeFileReader::sheReadError;
      }
    }
    else
      for (int i=0;i<vxIndexes.Size();i++) if (vxIndexes[i]!=-1)
      {
        DVertex vect=vxArray[vxIndexes[i]];
        if (!ReadLittle(input,vect.m)) return ShapeFileReader::sheReadError;
        vxArray.SetAt(vxIndexes[i],vect);
      }
      else
      {
        double unused;
        if (!ReadLittle(input,unused)) return ShapeFileReader::sheReadError;
      }
      return ShapeFileReader::sheOk;
  }

static ShapeFileReader::ShapeReaderError ReadPointM(QIStream &input, VertexArray &vxArray, ShapePoint &pt)
{
  AutoArray<unsigned int> vxIndexes;
  ShapeFileReader::ShapeReaderError err=ReadPoint(input,vxArray,pt,&vxIndexes);
  if (err) return err;
  err=ReadMOrZ(input,vxArray,vxIndexes,false,false);
  return err;
}

static ShapeFileReader::ShapeReaderError ReadPointZ(QIStream &input, VertexArray &vxArray, ShapePoint &pt)
{
  AutoArray<unsigned int> vxIndexes;
  ShapeFileReader::ShapeReaderError err=ReadPoint(input,vxArray,pt,&vxIndexes);
  if (err) return err;
  err=ReadMOrZ(input,vxArray,vxIndexes,true,false);
  if (err) return err;
  err=ReadMOrZ(input,vxArray,vxIndexes,false,false);
  return err;
}


static ShapeFileReader::ShapeReaderError ReadMultipoint(QIStream &input, VertexArray &vxArray, ShapeMultiPoint &mp, AutoArray<unsigned int> *vxIndexes=0)
{
  double bb[4];
  if (!ReadLittle(input,bb)) return ShapeFileReader::sheReadError;
  unsigned long count;
  if (!ReadLittle(input,count)) return ShapeFileReader::sheReadError;
  AutoArray<unsigned int,MemAllocLocal<unsigned int,256> > vxlist;
  for (unsigned long i=0;i<count;i++)
  {
    double x,y;
    if (!ReadLittle(input,x)) return ShapeFileReader::sheReadError;
    if (!ReadLittle(input,y)) return ShapeFileReader::sheReadError;
    vxlist.Add(AddVertex(vxArray,x,y,vxIndexes));
  }
  mp=ShapeMultiPoint(vxlist,&vxArray);
  return ShapeFileReader::sheOk;
}

static ShapeFileReader::ShapeReaderError ReadMultipointM(QIStream &input, VertexArray &vxArray, ShapeMultiPoint &pt)
{
  AutoArray<unsigned int> vxIndexes;
  ShapeFileReader::ShapeReaderError err=ReadMultipoint(input,vxArray,pt,&vxIndexes);
  if (err) return err;
  err=ReadMOrZ(input,vxArray,vxIndexes,false,true);
  return err;
}

static ShapeFileReader::ShapeReaderError ReadMultipointZ(QIStream &input, VertexArray &vxArray, ShapeMultiPoint &pt)
{
  AutoArray<unsigned int> vxIndexes;
  ShapeFileReader::ShapeReaderError err=ReadMultipoint(input,vxArray,pt,&vxIndexes);
  if (err) return err;
  err=ReadMOrZ(input,vxArray,vxIndexes,true,true);
  if (err) return err;
  err=ReadMOrZ(input,vxArray,vxIndexes,false,true);
  return err;
}

static ShapeFileReader::ShapeReaderError ReadPolyLine(QIStream &input, VertexArray &vxArray, ShapePoly &pl, AutoArray<unsigned int> *vxIndexes=0)
{
  double box[4];
  if (!ReadLittle(input,box)) return ShapeFileReader::sheReadError;
  unsigned long numParts;
  unsigned long numPoints;
  AutoArray<unsigned int,MemAllocLocal<unsigned int,256> > vxlist;
  AutoArray<unsigned int,MemAllocLocal<unsigned int,256> > partlist;
  if (!ReadLittle(input,numParts)) return ShapeFileReader::sheReadError;
  if (!ReadLittle(input,numPoints)) return ShapeFileReader::sheReadError;
  partlist.Resize(numParts);
  vxlist.Resize(numPoints);
  for (unsigned int i=0;i<numParts;i++)
  {
    unsigned long v;
    if (!ReadLittle(input,v)) return ShapeFileReader::sheReadError;
    partlist[i]=v;
  }
  for (unsigned int i=0;i<numPoints;i++)
  {
    double x,y;
    if (!ReadLittle(input,x)) return ShapeFileReader::sheReadError;
    if (!ReadLittle(input,y)) return ShapeFileReader::sheReadError;
    vxlist[i]=AddVertex(vxArray,x,y,vxIndexes);
  }
  pl=ShapePoly(vxlist,partlist,&vxArray);
  return ShapeFileReader::sheOk;
}

static ShapeFileReader::ShapeReaderError ReadPolyLineM(QIStream &input, VertexArray &vxArray, ShapePoly &pl)
{
  AutoArray<unsigned int> vxIndexes;
  ShapeFileReader::ShapeReaderError err=ReadPolyLine(input,vxArray,pl,&vxIndexes);
  if (err) return err;
  err=ReadMOrZ(input,vxArray,vxIndexes,false,true);
  return err;
}

static ShapeFileReader::ShapeReaderError ReadPolyLineZ(QIStream &input, VertexArray &vxArray, ShapePoly &pl)
{
  AutoArray<unsigned int> vxIndexes;
  ShapeFileReader::ShapeReaderError err=ReadPolyLine(input,vxArray,pl,&vxIndexes);
  if (err) return err;
  err=ReadMOrZ(input,vxArray,vxIndexes,true,true);
  if (err) return err;
  err=ReadMOrZ(input,vxArray,vxIndexes,false,true);
  return err;
}

static ShapeFileReader::ShapeReaderError ReadPolygon(QIStream &input, VertexArray &vxArray, ShapePoly &pl, AutoArray<unsigned int> *vxIndexes=0)
{
  double box[4];
  if (!ReadLittle(input,box)) return ShapeFileReader::sheReadError;
  unsigned long numParts;
  unsigned long numPoints;
  AutoArray<unsigned int,MemAllocLocal<unsigned int,256> > vxlist;
  AutoArray<unsigned int,MemAllocLocal<unsigned int,256> > partlist;
  if (!ReadLittle(input,numParts)) return ShapeFileReader::sheReadError;
  if (!ReadLittle(input,numPoints)) return ShapeFileReader::sheReadError;
  partlist.Resize(numParts);
  vxlist.Resize(numPoints-numParts);
  for (unsigned int i=0;i<numParts;i++)
  {
    unsigned long v;
    if (!ReadLittle(input,v)) return ShapeFileReader::sheReadError;
    partlist[i]=v;
  }
  unsigned int nextPart=1;
  for (unsigned int i=0;i<numPoints;i++)
  {
    double x,y;
    if (!ReadLittle(input,x)) return ShapeFileReader::sheReadError;
    if (!ReadLittle(input,y)) return ShapeFileReader::sheReadError;
    //remove last point in the part, it must be the same as first.
    if ((nextPart>=numParts && i+1==numPoints) || (nextPart<numParts && i+1==partlist[nextPart]))     
    {
      nextPart++;    
      if (vxIndexes) vxIndexes->Add(-1);
    }
    else
      vxlist[i-nextPart+1]=AddVertex(vxArray,x,y,vxIndexes);
  }
  //fix part indexes
  for (unsigned int i=0;i<numParts;i++) partlist[i]=partlist[i]-i;
  pl=ShapePoly(vxlist,partlist,&vxArray);
  return ShapeFileReader::sheOk;
}

static ShapeFileReader::ShapeReaderError ReadPolygonM(QIStream &input, VertexArray &vxArray, ShapePoly &pl)
{
  AutoArray<unsigned int> vxIndexes;
  ShapeFileReader::ShapeReaderError err=ReadPolyLine(input,vxArray,pl,&vxIndexes);
  if (err) return err;
  err=ReadMOrZ(input,vxArray,vxIndexes,false,true);
  return err;
}

static ShapeFileReader::ShapeReaderError ReadPolygonZ(QIStream &input, VertexArray &vxArray, ShapePoly &pl)
{
  AutoArray<unsigned int> vxIndexes;
  ShapeFileReader::ShapeReaderError err=ReadPolyLine(input,vxArray,pl,&vxIndexes);
  if (err) return err;
  err=ReadMOrZ(input,vxArray,vxIndexes,true,true);
  if (err) return err;
  err=ReadMOrZ(input,vxArray,vxIndexes,false,true);
  return err;
}


ShapeFileReader::ShapeReaderError ShapeFileReader::ReadShapeFile(QIStream &input, VertexArray &vxArray, ShapeList &shapeList, AutoArray<unsigned int> *indexList)
{
  ShapeFileReader::ShapeReaderError res;

  res=ReadShapeHeader(input,_header);
  if (res) return res;
  if (_header.fileCode!=9994) return sheFileTagMismach;
  if (_header.version!=1000) return sheVersionNotSupported;

  unsigned long curSize=100;  
  while (curSize<_header.fileLength*2)
  {
    RecordHeader rhdr;
    if (!ReadRecordHeader(input, rhdr)) return sheUnexceptedEndOfFile;

    IShape *shape=0;

    switch (rhdr.shapeType)
    {
    case 0:break; //null shape
    case 1:
      {
        ShapePoint sh;
        res=ReadPoint(input,vxArray,sh);        
        if (res) return res;
        shape=CreatePoint(sh);
      }
      break;
    case 3:
      {
        ShapePoly sh;
        res=ReadPolyLine(input,vxArray,sh);        
        if (res) return res;
        shape=CreatePolyLine(sh);
      }
      break;
    case 5:
      {
        ShapePoly sh;
        res=ReadPolygon(input,vxArray,sh);
        if (res) return res;
        shape=CreatePolygon(sh);
      }
      break;
    case 8:
      {
        ShapeMultiPoint sh;
        res=ReadMultipoint(input,vxArray,sh);
        if (res) return res;
        shape=CreateMultipoint(sh);
      }
      break;
    case 11:
      {
        ShapePoint sh;
        res=ReadPointZ(input,vxArray,sh);        
        if (res) return res;
        shape=CreatePoint(sh);
      }
      break;
    case 13:
      {
        ShapePoly sh;
        res=ReadPolyLineZ(input,vxArray,sh);        
        if (res) return res;
        shape=CreatePolyLine(sh);
      }
      break;
    case 15:
      {
        ShapePoly sh;
        res=ReadPolygonZ(input,vxArray,sh);
        if (res) return res;
        shape=CreatePolygon(sh);
      }
      break;
    case 18:
      {
        ShapeMultiPoint sh;
        res=ReadMultipointZ(input,vxArray,sh);
        if (res) return res;
        shape=CreateMultipoint(sh);
      }
      break;
    case 21:
      {
        ShapePoint sh;
        res=ReadPointM(input,vxArray,sh);        
        if (res) return res;
        shape=CreatePoint(sh);
      }
      break;
    case 23:
      {
        ShapePoly sh;
        res=ReadPolyLineM(input,vxArray,sh);        
        if (res) return res;
        shape=CreatePolyLine(sh);
      }
      break;
    case 25:
      {
        ShapePoly sh;
        res=ReadPolygonM(input,vxArray,sh);
        if (res) return res;
        shape=CreatePolygon(sh);
      }
      break;
    case 28:
      {
        ShapeMultiPoint sh;
        res=ReadMultipointM(input,vxArray,sh);
        if (res) return res;
        shape=CreateMultipoint(sh);
      }
      break;
    }
    if (shape==0) ///shape has not been created. SkipIt
    {
      unsigned long skipOffset=rhdr.contentLength*2-4;
      if (skipOffset)
      {
        AutoArray<char, MemAllocLocal<char,256> >buff;
        buff.Resize(skipOffset);
        input.read(buff.Data(),skipOffset);
        if (input.fail()) return sheReadError;
//        if (input.GetDataSize()!=skipOffset) return sheUnexceptedEndOfFile;
        curSize+=skipOffset+12;
      }  
    }
    else
    {
      unsigned long skipOffset=rhdr.contentLength*2;
      unsigned long recordNumber=rhdr.recordNumber-1;
      unsigned long shapeId=shapeList.AddShape(PShape(shape));
      if (indexList)
      {
        while ((unsigned)indexList->Size()<recordNumber) indexList->Add(-1);
        indexList->Add(shapeId);
      }   
      curSize+=skipOffset+8;
    }    
  }
  return sheOk;

}

IShape *ShapeFileReader::CreatePoint(const ShapePoint &origShape)
{
  return new ShapePoint(origShape);
}
IShape *ShapeFileReader::CreateMultipoint(const ShapeMultiPoint &origShape)
{
  return new ShapeMultiPoint(origShape);
}
IShape *ShapeFileReader::CreatePolyLine(const ShapePoly &origShape)
{
  return new ShapePolyLine(origShape);
}
IShape *ShapeFileReader::CreatePolygon(const ShapePoly &origShape)
{
  return new ShapePolygon(origShape);
}

ShapeFileReader::ShapeReaderError ShapeFileReader::ReadShapeFile(const _TCHAR *filename, VertexArray &vxArray, ShapeList &shapeList, AutoArray<unsigned int> *indexList)
{
  QIFStream in;
  in.open(filename);
  if (in.fail()) return sheOpenError;
  return ReadShapeFile(in,vxArray,shapeList,indexList);
}

}