#ifndef _FILE_MAP_HPP
#define _FILE_MAP_HPP

#include <limits.h>
#include <Es\Framework\debugLog.hpp>
#include <Es\Types\pointers.hpp>
#include <Es\Containers\array.hpp>
#include <Es\Containers\listBidir.hpp>
#include <xmmintrin.h>

/// wrap access into the memory mapped file
/**
This is used for abstraction - simulate continuous memory access using any suitable means
We simulate complete pointer arithmetics
*/

class FileMemMapped;

/// basic testing implementation - memory based
class FileMapAccessMemory
{
  friend class FileMapAccessView;
  
  /// base address
  char *_data;
  /// largest. allowed negative offset
  char *_begPtr; 
  /// largest. allowed positive offset
  char *_endPtr;

  FileMapAccessMemory(char *data, char *begPtr, char *endPtr)
  :_data(data),_begPtr(begPtr),_endPtr(endPtr)
  {}
public:
  FileMapAccessMemory(const FileMemMapped *file, char *data, int size)
  :_data(data),_begPtr(_data),_endPtr(_data+size)
  {
  }

  FileMapAccessMemory()
  :_data(0),_begPtr(0),_endPtr(0)
  {
  }

  const char *GetData() const {return _data;}
  const FileMapAccessMemory &operator += (int offset) {_data += offset;return *this;}
  const FileMapAccessMemory &operator -= (int offset) {_data -= offset;return *this;}

  FileMapAccessMemory operator + (int offset) const {return FileMapAccessMemory(_data+offset,_begPtr,_endPtr);}
  FileMapAccessMemory operator - (int offset) const {return FileMapAccessMemory(_data-offset,_begPtr,_endPtr);}
  int operator - (FileMapAccessMemory with) const {return _data-with._data;}

  const FileMapAccessMemory &operator ++ (){_data++;return *this;}

  const FileMapAccessMemory &operator -- (){_data--;return *this;}

  void operator ++ (int) {_data++;}

  void operator -- (int) {_data--;}

  char Get(int i) const
  {
    Assert(_data+i>=_begPtr && _data+i<_endPtr);
    return _data[i];
  }

  /// jump to the end of the access
  FileMapAccessMemory GetEnd() const {return FileMapAccessMemory(_endPtr,_begPtr,_endPtr);}
  /// jump to the beginning of the access
  FileMapAccessMemory GetBeg() const {return FileMapAccessMemory(_begPtr,_begPtr,_endPtr);}
  
  const char *GetRawData() const {return _data;}
  
  char operator [] (int i) const {return Get(i);}

  char operator * () const {return Get(0);}

  void Prefetch(int i) const {_mm_prefetch(_data+i,_MM_HINT_T0);}
#ifndef _M_X64
  unsigned char Compare8(__m64 i) const
  {
    Assert(_data>=_begPtr && _data+8<=_endPtr);
    return _mm_movemask_pi8(_mm_cmpeq_pi8(i,*(__m64 *)_data));
  }
#endif
  bool operator == (const FileMapAccessMemory &with )const {return _data==with._data;}
  bool operator > (const FileMapAccessMemory &with )const {return _data>with._data;}
  bool operator <= (const FileMapAccessMemory &with )const {return !operator >(with);}

  /// NULL test
  bool operator !() const {return _data==NULL;}
};

/// view encapsulation
class FileMemView: public RefCount, public TLinkBidirD
{
  friend class FileMemMapped;
  friend class FileMapAccessView;

  int _index;
  void *_view;
  int _size;

  FileMemView(int index){_index=index;}
  ~FileMemView()
  {
    Assert(!_view);
  }
};


/// basic testing implementation - short view windows based
class FileMapAccessView
{
  /// which file do we view into
  const FileMemMapped *_file;

  /// base address
  int _offset;
  /// largest. allowed negative offset
  int _beg; 
  /// largest. allowed positive offset
  int _end;

  FileMapAccessView(const FileMemMapped *file, int offset, int beg, int end)
  :_file(file),_offset(offset),_beg(beg),_end(end)
  {}
public:
  FileMapAccessView(const FileMemMapped *file, char *view, int size)
  :_file(file),_offset(0),_beg(0),_end(size)
  {}

  FileMapAccessView()
  :_file(NULL),_offset(0),_beg(0),_end(0)
  {
  }

  const FileMapAccessView &operator += (int offset)
  {
    _offset += offset;
    _beg -= offset;
    _end -= offset;
    return *this;
  }
  const FileMapAccessView &operator -= (int offset)
  {
    _offset -= offset;
    _beg += offset;
    _end += offset;
    return *this;
  }

  FileMapAccessView operator + (int offset) const
  {
    return FileMapAccessView(_file,_offset+offset,_beg-offset,_end-offset);
  }
  FileMapAccessView operator - (int offset) const
  {
    return FileMapAccessView(_file,_offset-offset,_beg+offset,_end+offset);
  }
  int operator - (FileMapAccessView with) const
  {
    return _offset-with._offset;
  }

  const FileMapAccessView &operator ++ (){_offset++;_beg--,_end--;return *this;}

  const FileMapAccessView &operator -- (){_offset--;_beg++,_end++;return *this;}

  void operator ++ (int) {_offset++;_beg--,_end--;}

  void operator -- (int) {_offset--;_beg++,_end++;}

  char Get(int i) const;

  char operator [] (int i) const {return Get(i);}

  char operator * () const {return Get(0);}

  bool operator == (const FileMapAccessView &with )const {return _offset==with._offset;}

  bool operator > (const FileMapAccessView &with )const {return _offset>with._offset;}

  bool operator <= (const FileMapAccessView &with )const {return !operator >(with);}
  
  /// get view starting with given point 
  FileMapAccessMemory GetViewFrom() const;

  /// get view end with given point (point not included)
  FileMapAccessMemory GetViewTo() const;

  /// jump to the end of the access // +_end-_offset
  
  FileMapAccessView GetEnd() const {return FileMapAccessView(_file,_end,_beg-_end+_offset,+_offset);}
  /// jump to the beginning of the access
  FileMapAccessView GetBeg() const {return FileMapAccessView(_file,_beg,+_offset,_end-_beg+_offset);}

  /// NULL test
  bool operator !() const {return _file==NULL;}
};

//typedef FileMapAccessMemory FileMapAccess;
typedef FileMapAccessView FileMapAccess;


// memory mapped file access
/**
This class was based on FileBufferMapped, but QStream implementation became too different */
class FileMemMapped
{
	HANDLE _fileHandle;
	HANDLE _mapHandle;
	void *_view;
  int _start;
	int _size;

  /// size of one view page
  int _viewPage;

  /// log 2 of _viewPage (used for shifting)
  int _viewPageLog2;

  /// slots for all views, NULL when not mapped
  /** caching implemented, once there are too many views, some are discarded */
  mutable RefArray<FileMemView> _views;

  mutable TListBidir<FileMemView, TLinkBidirD, SimpleCounter<FileMemView> > _lruViews;
  
  /// TODO: keep LRU of views, evict the one which was not used for a long time

	protected:
	void Open( HANDLE fileHandle, int start, int size );
  void MapWholeFile();

  Ref<FileMemView> NewView(int index, int start, int size) const;
  void CloseView(FileMemView *view) const;

	public:
	void Open( const char *name, int start=0, int size=INT_MAX );
	void Close();

	FileMemMapped();
	FileMemMapped( HANDLE file, int start=0, int size=INT_MAX );
	FileMemMapped( const char *name, int start=0, int size=INT_MAX );
	~FileMemMapped();

  /// some access types may need preparation
  template <class Type>
  void Activate() const {}
  /// memory access needs preparation
  template <>
  void Activate<FileMapAccessMemory>() const {unconst_cast(this)->MapWholeFile();}

	FileMapAccess GetData() const
	{
	  Activate<FileMapAccess>();
	  return FileMapAccess(this,(char *)_view,_size);
	}

  /// get one byte from the file
  char Get(int offset) const;

  /// get a view corresponding to a given offset
  FileMemView *GetView(int &offset) const;

  void LoadSlot( int index ) const;
	void *GetRawData();
  
	int GetSize() const {return _size;}

	bool GetError() const {return _mapHandle==NULL;}
};

/**
The only function referencing real data
*/
inline char FileMapAccessView::Get(int i) const
{
  // we need to make sure we have the data needed
  // we access the view and ask it
  // _data contains part of the offset, represented as base address + some offset
  Assert(i>=_beg && i<_end);
  // TODO: optimize (reuse last object)
  return _file->Get(i+_offset);
}

inline FileMapAccessMemory FileMapAccessView::GetViewFrom() const
{
  int newOffset = _offset;
  const FileMemView *view = _file->GetView(newOffset);
  int align = _offset-newOffset;
  char *base = (char *)view->_view+align; 
  return FileMapAccessMemory(base,base,(char *)view->_view+view->_size);
}

inline FileMapAccessMemory FileMapAccessView::GetViewTo() const
{
  if (_offset<=0)
  {
    // no data 
    return FileMapAccessMemory(NULL,NULL,NULL);
  }
  int newOffset = _offset-1;
  const FileMemView *view = _file->GetView(newOffset);
  int align = _offset-1-newOffset;
  char *base = (char *)view->_view; 
  return FileMapAccessMemory(base+align+1,base,base+align+1);
}

#endif
