#include "../wpch.hpp"
#include "faceArray.hpp"
#include <Es/Algorithms/qsort.hpp>

void FaceArray::SetSections( const ShapeSection *sec, int nSec)
{
  _sections.Realloc(nSec);
  _sections.Resize(nSec);
  for (int i=0; i<nSec; i++)
  {
    _sections[i] = sec[i];
  }
}

static StaticStorage<char> CharStorageF; // storage for faces
static StaticStorage<char> CharStorageS; // storage for sections

#define VERIFY_TXT_PTR 0

#if VERIFY_TXT_PTR
#include "win.h"
#endif

double FaceArray::GetMemoryUsed() const
{
  double sum = 0;
  if (_data) sum += _data->_faces.RawSize();
  sum += _sections.GetMemoryUsed();
  return sum;
}

bool FaceArray::VerifyStructure() const
{
	#if VERIFY_TXT_PTR
	// verify all face textures are valid
	for (Offset f=Begin(); f<End(); Next(f))
	{
		const Poly &face = (*this)[f];
		// force access to texture
		// check if pointer is valid
		Texture *texture = face.GetTexture();
		if (::IsBadReadPtr(texture,sizeof(Texture)))
		{
			LogF("FaceArray::VerifyStructure: Bad read pointer");
			return false;
		}
		if (::IsBadWritePtr(texture,sizeof(Texture)))
		{
			LogF("FaceArray::VerifyStructure: Bad write pointer");
			return false;
		}
	}
	#endif

	// verify section structure
	Offset lastOffset = Offset(0);

	for (int s=0; s<_sections.Size(); s++)
	{
		const ShapeSection &sec = _sections[s];
		if (sec.beg!=lastOffset)
		{
			ErrF("sec.beg!=lastOffset : %d!=%d",sec.beg,lastOffset);
			return false;
		}
		lastOffset = sec.end;
	}

	return true;
}

void FaceArray::ReserveFaces( int size, bool dynamic )
{
  if (size>0)
    _data = new FaceData;
	if( !dynamic )
	{
		if (_data) _data->_faces.GetData().SetStorage(CharStorageF.Init(64*1024));
		_sections.SetStorage(CharStorageS.Init(1024));
	}
	if (_data) _data->_faces.Reserve(size);
}

FaceArray::FaceArray( int size, bool dynamic )
{
  if (size>0)
    _data = new FaceData;
	if( !dynamic )
	{
		if (_data) _data->_faces.GetData().SetStorage(CharStorageF.Init(64*1024));
		_sections.SetStorage(CharStorageS.Init(1024));
	}
	if (size>0)
	{
		if (_data) _data->_faces.Realloc(size);
	}
}

FaceArray::FaceArray(const FaceArray &src) :_data(src._data ? new FaceData(*src._data) : NULL),_sections(src._sections)
{
}


void FaceArray::operator=(const FaceArray &src)
{
  _data = src._data ? new FaceData(*src._data) : NULL;
  _sections = src._sections;
}

void FaceArray::ExtendSection(const ShapeSectionInfo &prop, bool forceNewSection)
{
	// check end offset of last section
	Offset end=Offset(0);
	if (_sections.Size()>0)
	{
		end = _sections[_sections.Size()-1].end;
	}
	// check if there is something to add
	if (End()==end) return;

  // If the current section can be used, check if it is the same
  if (!forceNewSection)
  {
    // close current section
    // check if we can add into the last section
    if (_sections.Size()>0)
    {
      ShapeSection &ss = _sections[_sections.Size()-1];
      if (ss==prop)
      {
        ss.end = End();
        return;
      }
    }
  }

  // caution: any reference into _sections may become invalid here
  // make sure prop is pointing someplace else
  // TODO: add Assert here
	ShapeSection &sec = _sections.Append();
	sec.beg = end;
	sec.end = End();
	sec.SetInfo(prop);
}

TypeIsSimple(const ShapeSection *)

void FaceArray::SortSections(int (*Compare)( const ShapeSection **p0, const ShapeSection **p1 ))
{
  if (!_data)
  {
    DoAssert(_sections.Size() == 0);
    return;
  }

	AUTO_STATIC_ARRAY(const ShapeSection *,index,256);
	for (int i=0; i<_sections.Size(); i++)
	{
	  index.Add(&_sections[i]);
	}
	QSort(index.Data(),index.Size(),Compare);
	// stream faces from source sections depending on order specified by the index

	// copy source data
	StreamArray<Poly,StaticArrayAuto<char> > copy;
	const int copyStaticSize = 32*1024;
	AUTO_STORAGE(char,copyStorage,copyStaticSize);

	copy.SetStorage(MemAllocSA(copyStorage,sizeof(copyStorage)));
	copy.RawRealloc(copyStaticSize);
	copy.Merge(_data->_faces.RawData(),_data->_faces.RawSize(),_data->_faces.Size());
	// clear destination, avoid reallocation - size should not change
	_data->_faces.Resize0();
	// clearing _sections will invalidate index
	AUTO_STATIC_ARRAY(ShapeSection,sortedSections,256);
	for (int i=0; i<index.Size(); i++)
	{
	  sortedSections.Add(*index[i]);
	}
	_sections.Resize(0);
	RawRealloc(copy.RawSize());

	for (int s=0; s<sortedSections.Size(); s++)
	{
		const ShapeSection &srcSec = sortedSections[s];
		//TODO: optimize: whole segment may be copied at once
		for (Offset o=srcSec.beg; o<srcSec.end; copy.Next(o))
		{
			Add(copy[o]);
		}
		ExtendSection(srcSec);
	}

	_sections.Compact();

}

void FaceArray::ConcatSections(int start)
{
	AutoArray<ShapeSection, MemAllocLocal<ShapeSection,256> > sections;
	
	for (int i=start; i<NSections(); i++)
	{
	  sections.Add(GetSection(i));
	}
	_sections.Resize(start);

	for (int s=0; s<sections.Size(); s++)
	{
		const ShapeSection &srcSec = sections[s];

	  // check if we can add into the last section
	  if (_sections.Size()>start)
	  {
		  ShapeSection &ss = _sections[_sections.Size()-1];
		  if (ss.ShapeSectionInfo::operator ==(srcSec))
		  {
		    ss.end = srcSec.end;
			  continue;
		  }

	  }

    // caution: any reference into _sections may become invalid here
    // make sure prop is pointing someplace else
    // TODO: add Assert here
	  ShapeSection &sec = _sections.Append();
	  sec = srcSec;
	}

	_sections.Compact();
}

void FaceArray::CloseSection(const ShapeSectionInfo &prop)
{
	// check end offset of last section
	Offset end=Offset(0);
	if (_sections.Size()>0)
	{
		end = _sections[_sections.Size()-1].end;
	}
	// check if there is something to add
	if (End()==end) return;
	// close current section
	ShapeSection &sec = _sections.Append();
	sec.beg = end;
	sec.end = End();
	sec.SetInfo(prop);
}

void FaceArray::SetCanonical(bool val)
{
  // verify the caller is not lying
  #if _DEBUG
    for (Offset o=Begin(); o<End(); Next(o))
    {
      const Poly &poly = (*this)[o];
      DoAssert(poly.N()==3);
      DoAssert(poly.GetVertex(0) != poly.GetVertex(1));
      DoAssert(poly.GetVertex(1) != poly.GetVertex(2));
      DoAssert(poly.GetVertex(2) != poly.GetVertex(0));
    }
  #endif
  _data->_canonical = val;
}
