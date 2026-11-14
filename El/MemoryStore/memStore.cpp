#include "memStore.hpp"
#include <El/Common/perfProf.hpp>

#if USE_MEM_STORE

// with Win32 we can use the filemapping
# define NO_MAP 0
  /* note: most operations with BigPagePages=1 are optimized out */
  const int BigPagePages = 1;

MemoryStore::MemoryStore()
{
  _mapping = NULL;
  _pageSize = 0;
  _allocSize = 0;
  _committed = 0;
  _mapped=AtomicInt(0);
}
void MemoryStore::Init(size_t size)
{
  DoAssert(!_mapping);
  #if NO_MAP
  int flags = MEM_RESERVE;
  // on PC we do only Big page emulation for debugging - not using real big pages
  _mapping = VirtualAlloc(NULL,size,flags,PAGE_NOACCESS);
  _pageSize = 4*1024;
  _allocSize = 64*1024;
  #else
  _mapping = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE|SEC_COMMIT,0,DWORD(size),NULL);
  if (!_mapping)
  {
    ErrorMessage("Not enough physical memory / swap file space for %d KB",DWORD(size)/1024);
  }
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  _pageSize = si.dwPageSize;
  _allocSize = si.dwAllocationGranularity;
  #endif
  _pageCount = size/_pageSize;
  #if !NO_MAP
    int regionCount = size/_allocSize;
    _mappingCache.Realloc(regionCount);
    _mappingCache.Resize(regionCount);
  #endif
  // make sure we have enough space to contain list of all pages without any more allocations
  // with 300 MB cache this should take ~75 KB
  _free.Realloc(_pageCount);
  _allocCount.Realloc(_pageCount);
  _allocCount.Resize(_pageCount);
  for (int i=0; i<_allocCount.Size(); i++) _allocCount[i] = 0;
  // all pages are free
  _freeFrom = 0;
  // no pages are committed, no are mapped
  _committed=0;
  _mapped=AtomicInt(0);
}
MemoryStore::~MemoryStore()
{
  #if NO_MAP
  VirtualFree(_mapping,0,MEM_RELEASE);
  #else
  // if there are any views still open, they should be closed
  CloseHandle(_mapping);
  #endif
}

int MemoryStore::Alloc()
{
  #if MEM_STORE_THREADSAFE
  ScopeLockSection lock(_lock);
  #endif
  //AssertSameThread(_threadId);
  // find any unused page
  int index = -1;
  // first try to reuse holes
  if (_free.Size()>0)
  {
    // use the last entry, so that extraction is fast: O(1)
    index = _free.Last();
    _free.Delete(_free.Size()-1);
  }
  else if (_freeFrom<_pageCount)
  { // if no holes, take a new page from the free space
    index = _freeFrom++;
  }


  // if there is none, we are in trouble - this means the store is not sufficient
  // what can we do on a failure? Do we free the page again, and report an error?
  if (index>=0)
  {
    #if NO_MAP
      // consider storing hierarchical commit count - memory requirements should be modest
      // check if any other page in the same large page region is already committed
      // if it is, no need to commit
      int pageStart = index&~(BigPagePages-1);
      int pageSize = BigPagePages*_pageSize;
      
      bool committed = false;
      for (int i=0; i<BigPagePages; i++)
      {
        if (_allocCount[pageStart+i]>0) {committed=true;break;}
      }
      if (!committed)
      {
        PROFILE_SCOPE_EX(msVAC,file)
        void *mem = (char *)_mapping+pageStart*_pageSize;
        void *ok = VirtualAlloc(mem,pageSize,MEM_COMMIT,PAGE_READWRITE);
        if (!ok)
        {
          // VirtualAlloc MEM_COMMIT might fail - what can we do?
          // 
          MarkFree(index);
          return -1;
        }
        Assert(ok==mem);
      }
    #endif
    _committed++;
    // fist allocation always must have a count of zero
    Assert(_allocCount[index]==0)
    _allocCount[index]++;
  }
  // check invariant
  Assert(_committed==_freeFrom-_free.Size());
  return index;
}

void MemoryStore::ShareAlloc(int index)
{
  #if MEM_STORE_THREADSAFE
  ScopeLockSection lock(_lock);
  #endif
  //AssertSameThread(_threadId);
  Assert(_allocCount[index]<UCHAR_MAX)
  _allocCount[index]++;
}

void MemoryStore::Free(int index)
{
  #if MEM_STORE_THREADSAFE
  ScopeLockSection lock(_lock);
  #endif
  //AssertSameThread(_threadId);
  if (--_allocCount[index]>0) return;
  Assert(_allocCount[index]==0);
  #if NO_MAP
    // check if other pages in the same big page are already free
    // if they are, decommit
    // consider storing hierarchical commit count - memory requirements should be modest
    // check if any other page in the same large page region is already committed
    // if it is, no need to commit
    int pageStart = index&~(BigPagePages-1);
    int pageSize = BigPagePages*_pageSize;
    // bool would be enough, but we prefer int, as this can be done without branching
    bool committed = false;
    for (int i=0; i<BigPagePages; i++)
    {
      if (_allocCount[pageStart+i]>0) {committed=true;break;}
    }
    if (!committed)
    {
      PROFILE_SCOPE_EX(msVFC,file)
      void *mem = (char *)_mapping+pageStart*_pageSize;
      VirtualFree(mem,pageSize,MEM_DECOMMIT);
    }
  #endif
  MarkFree(index);
  _committed--;
  // check invariant
  Assert(_committed==_freeFrom-_free.Size());
}


void *MemoryStore::MapView(int index, bool write)
{
  int offset = index*_pageSize;
  #if NO_MAP  
  void *addr = (char *)_mapping+offset;
  return addr;
  #else

  int posAligned = offset&~(_allocSize-1);
  int sizeAligned = (offset-posAligned+_pageSize+_allocSize-1)&~(_allocSize-1);
  
  // TODO: calculate only once
  //int pageToRegion = _allocSize/_pageSize;
  //int regionIndex = index/pageToRegion;
  int regionIndex = offset/_allocSize;
  
  #if MEM_STORE_THREADSAFE
  ScopeLockSection lock(_lockMappingCache);
  #endif
  if (_mappingCache[regionIndex].mapping)
  {
    DoAssert(_mappingCache[regionIndex].refCount>0);
    _mappingCache[regionIndex].refCount++;
    return (char *)_mappingCache[regionIndex].mapping + offset-posAligned;
  }
  
  Assert(_mapped>=0);
  _mapped++;
  // create a view

  PROFILE_SCOPE_EX(msMVF,file)
  //DWORD access = write ? FILE_MAP_WRITE : FILE_MAP_READ;
  // because of caching, we cannot check access, or we need to cache one access only, or to cache both separately
  DWORD access = FILE_MAP_WRITE;
  void *view = MapViewOfFile(_mapping,access,0,posAligned,sizeAligned);
  if (view==NULL)
  {
    MEMORYSTATUS memStat;
    memStat.dwLength = sizeof(memStat);
    GlobalMemoryStatus(&memStat);
    RptF(
      "Memory store: Failed mapping, already mapped %d KB, error %x",
      _mapped*(_pageSize/1024),GetLastError()
    );
    RptF(
      "Virtual free %u B, page free %u B, physical free %u B",
      memStat.dwAvailVirtual,memStat.dwAvailPageFile,memStat.dwAvailPhys
    );
    #if _ENABLE_REPORT
    void PrintVMMap(int extended, const char *title=NULL);
    PrintVMMap(0);
    #endif  
    _mapped--;  
    return NULL;
  }
  
  _mappingCache[regionIndex].mapping = view;
  _mappingCache[regionIndex].refCount = 1;
  
  // we want to return the address of the requested page, not of the whole view
  return (char *)view + (offset-posAligned);
  #endif
}
void MemoryStore::UnmapView(int index, void *mem)
{
  #if !NO_MAP
  PROFILE_SCOPE_EX(msUVF,file)

  #if MEM_STORE_THREADSAFE
  ScopeLockSection lock(_lockMappingCache);
  #endif
  
  int regionIndex = index*_pageSize/_allocSize;
  DoAssert(_mappingCache[regionIndex].refCount>0);
  if (--_mappingCache[regionIndex].refCount>0)
    return;
  
  intptr_t memAddrAligned = (intptr_t)mem&~(_allocSize-1);
  DoAssert(_mappingCache[regionIndex].mapping==(void *)memAddrAligned);
  UnmapViewOfFile((void *)memAddrAligned);
  {
  _mapped--;
  Assert(_mapped>=0);
  }
  _mappingCache[regionIndex].mapping=NULL;
  #endif
}

#endif
