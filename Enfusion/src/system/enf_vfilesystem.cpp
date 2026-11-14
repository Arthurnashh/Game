//-----------------------------------------------------------------------------
// File: enf_vfilesystem.cpp
//
// Desc: Virtual filesystem
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "common/enf_serialize.h"
#include "common/enf_memorybuffer.h"
#include "system/enf_timer.h"
#include "system/enf_ifilesystem.h"
#include "system/enf_thread.h"
#include "system/enf_packfile.h"
#include "tinyxml/tinyxml.h"

//#define _FIRST_PATCH_

namespace enf
{
FileSystem* g_pFileSystem = NULL;

uint FileSystem::m_iProductID = 0xffffffff;

ENF_INLINEFUNC bool IsDelimiter(char c)
{
	return c == '/' || c == '\\';
}

#if defined(WIN32) || defined(ENF_XENON)
ENF_INLINEFUNC bool IsSystemDelimiter(char c)
{
	return c == '\\';
}

ENF_INLINEFUNC char ToSysDelimiter(char c)
{
	return IsDelimiter(c) ? '\\' : c;
}

#else
ENF_INLINEFUNC bool IsSystemDelimiter(char c)
{
	return c == '/';
}

ENF_INLINEFUNC char ToSysDelimiter(char c)
{
	return IsDelimiter(c) ? '/' : c;
}


#endif

//-----------------------------------------------------------------------
void IFileSystem::FreeRequest(OverlappedRequest* request)
{
	enf_assert(m_FreeRequests.GetIndexOf(request) == -1);
	m_FreeRequests.Insert(request);
}

//-----------------------------------------------------------------------
void IFileSystem::InitCache()
{
	for(uint n = 0; n < NUM_CACHE_PAGES; n++)
	{
		ubyte* origPtr = (ubyte*)MemoryManager::Alloc(CACHE_PAGE_SIZE + 4095, __FILE__, __LINE__);
		m_Pages[n].m_pOriginalPtr = origPtr;
		m_Pages[n].m_pBuffer = (void*)((intptr_t)(origPtr + 4095) & ~((intptr_t)4095));
		m_Pages[n].m_Lock = 0;
		m_FreePages.Insert(&m_Pages[n]);
	}
}

//-----------------------------------------------------------------------
void IFileSystem::FreeCache()
{
	for(uint n = 0; n < NUM_CACHE_PAGES; n++)
	{
		if(m_Pages[n].m_pOriginalPtr)
		{
			MemoryManager::Free(m_Pages[n].m_pOriginalPtr);
			m_Pages[n].m_pOriginalPtr = NULL;
		}
	}
	m_FreePages.Clear();
	m_UsedPages.Clear();
}

//-----------------------------------------------------------------------
CachePage* IFileSystem::AllocCachePage(PhysFile* vf, FileOffset offset, uint* pageIndex)
{
	CachePage* page;
	enf_assert((m_FreePages.GetCardinality() + m_UsedPages.GetCardinality()) == NUM_CACHE_PAGES);
	if(m_FreePages.IsEmpty())
	{
		while(true)
		{
			for(page = m_UsedPages.GetLastNode(); page; page = page->ENF_LIST_PREV)
			{
				if(IL_COMPARE_EXCHANGE(page->m_Lock, 1, 0) == 0)
				{
					m_UsedPages.Remove(page);
					page->m_pFile->RemoveCache(page);
					//TODO: what about to cancel IO request?
					WaitForData(page->m_pFile, page);
					break;
				}
			}
			if(page)
				break;
			CThread::Sleep(0);
		}
	}
	else
	{
		page = m_FreePages.GetFirstNode();
		m_FreePages.Remove(page);
	}
	IL_EXCHANGE(page->m_Lock, 1);
	m_UsedPages.InsertAsFirst(page);
	page->m_pFile = vf;
	page->m_Offset = offset;
	page->m_pRequest = NULL;
	page->m_Length = 0;
	vf->AddCache(offset, page, pageIndex);
	return page;
}

//-----------------------------------------------------------------------
void IFileSystem::FreeCachePage(CachePage* page)
{
	enf_assert(m_UsedPages.Contains(page));
	m_UsedPages.Remove(page);
	m_FreePages.Insert(page);
	page->m_pFile = NULL;
}

//-----------------------------------------------------------------------
struct FileDef
{
	const FileDef*	Parent;
	const char*		Name;
	uint				Attributes;
	uint				Dummy;
	ENF_DECLARE_SMALLOBJECT(FileDef);
	FileDef(): Parent(NULL), Name(NULL), Attributes(0) {}
	FileDef(const FileDef* parent, const char* name, uint attr): Parent(parent), Name(name), Attributes(attr) {}
};

struct FileDefDirectory: public FileDef
{
	Array<FileDef*>	Files;
	HashMap<const char*, FileDef*, StringHashFunc> FilesHash;
	ENF_DECLARE_SMALLOBJECT(FileDefDirectory);
};

class FileTree: public FileDefDirectory
{
protected:
	
public:
	const FileDef* Find(const char* name) const
	{
		const FileDefDirectory* directory = this;
		const FileDef* def = NULL;
		const char* start = name;
		while(*start && directory)
		{
			const char* end = enf::strchr(start, '/');

			if(end == NULL)
			{
				def = directory->FilesHash.Find(start);
				start += enf::strlen(start);
			}
			else
			{
				SString<MAX_FILENAME_LENGTH> str;

				str.Copy(start, end - start);
				def = directory->FilesHash.Find(str.cstr());
				start = end + 1;
			}
			if(def == NULL)
				return NULL;

			if(def->Attributes & FILEATTR_DIRECTORY)
				directory = (const FileDefDirectory*)def;
			else
				directory = NULL;
		}

		//not at the end
		if(*start != 0)
			return NULL;

		return def;
	}
};

//-----------------------------------------------------------------------
CStr IFileSystem::GetFileSystemName(uint index) const
{
	return m_Filesystems[index].Data->m_strName.cstr();
}

//-----------------------------------------------------------------------
CStr IFileSystem::GetFileSystemPath(uint index) const
{
	return m_Filesystems[index].Data->m_strPath.cstr();
}

//-----------------------------------------------------------------------
bool IFileSystem::ContainsNamedFileSystem(CStr name)
{
	return m_NamedFileSystems.Contains(name);
}

//-----------------------------------------------------------------------
CStr IFileSystem::GetNamedFileSystemPath(CStr name)
{
	FileSystemDef* fs = m_NamedFileSystems.Find(name);

	if(!fs)
		return NULL;

	return fs->GetPath();
}

//-----------------------------------------------------------------------
void SDateTime::Serialize(SerializeOutputAdapter* adapter)
{
	adapter->WriteUInt16(Year);
	adapter->WriteUInt16(Month);
	adapter->WriteUInt16(DayOfWeek);
	adapter->WriteUInt16(Day);
	adapter->WriteUInt16(Hour);
	adapter->WriteUInt16(Minute);
	adapter->WriteUInt16(Second);
	adapter->WriteUInt16(Milliseconds);
}

//-----------------------------------------------------------------------
void SDateTime::Serialize(SerializeInputAdapter* adapter)
{
	Year = adapter->ReadUInt16();
	Month = adapter->ReadUInt16();
	DayOfWeek = adapter->ReadUInt16();
	Day = adapter->ReadUInt16();
	Hour = adapter->ReadUInt16();
	Minute = adapter->ReadUInt16();
	Second = adapter->ReadUInt16();
	Milliseconds = adapter->ReadUInt16();
}

//-----------------------------------------------------------------------
SDateTime& SDateTime::operator=(const SDateTime& datetime)
{
	Year = datetime.Year;
	Month = datetime.Month;
	DayOfWeek = datetime.DayOfWeek;
	Day = datetime.Day;
	Hour = datetime.Hour;
	Minute = datetime.Minute;
	Second = datetime.Second;
	Milliseconds = datetime.Milliseconds;
	return *this;
}

//-----------------------------------------------------------------------
bool SDateTime::operator==(const SDateTime& datetime) const
{
	return (Year == datetime.Year &&
				Month == datetime.Month &&
				DayOfWeek == datetime.DayOfWeek &&
				Day == datetime.Day &&
				Hour == datetime.Hour &&
				Minute == datetime.Minute &&
				Second == datetime.Second &&
				Milliseconds == datetime.Milliseconds);
}

//-----------------------------------------------------------------------
bool SDateTime::operator<(const SDateTime& datetime) const
{
	if(Year > datetime.Year) return false;
	if(Year < datetime.Year) return true;
	if(Month > datetime.Month) return false;
	if(Month < datetime.Month) return true;
	if(Day > datetime.Day) return false;
	if(Day < datetime.Day) return true;
	if(Hour > datetime.Hour) return false;
	if(Hour < datetime.Hour) return true;
	if(Minute > datetime.Minute) return false;
	if(Minute < datetime.Minute) return true;
	if(Second > datetime.Second) return false;
	if(Second < datetime.Second) return true;
	if(Milliseconds > datetime.Milliseconds) return false;
	if(Milliseconds < datetime.Milliseconds) return true;
	return false;
}

//-----------------------------------------------------------------------
bool SDateTime::operator>(const SDateTime& datetime) const
{
	if(Year < datetime.Year) return false;
	if(Year > datetime.Year) return true;
	if(Month < datetime.Month) return false;
	if(Month > datetime.Month) return true;
	if(Day < datetime.Day) return false;
	if(Day > datetime.Day) return true;
	if(Hour < datetime.Hour) return false;
	if(Hour > datetime.Hour) return true;
	if(Minute < datetime.Minute) return false;
	if(Minute > datetime.Minute) return true;
	if(Second < datetime.Second) return false;
	if(Second > datetime.Second) return true;
	if(Milliseconds < datetime.Milliseconds) return false;
	if(Milliseconds > datetime.Milliseconds) return true;
	return false;
}

//-----------------------------------------------------------------------
FileSystem* GetFileSystem()
{
	enf_assert(g_pFileSystem);
	return g_pFileSystem;
}

bool IsFileSystem()
{
	return g_pFileSystem != NULL;
}

//-----------------------------------------------------------------------
IFileSystem::~IFileSystem()
{
	RemoveAll();
	{
		CMutexLock lock(m_CacheMutex);

		for(uint n = 0; n < m_FreeRequests.GetCardinality(); n++)
		{
			delete m_FreeRequests[n];
		}
		m_FreeRequests.Clear();
	}
	if(g_pFileSystem == this)
		g_pFileSystem = NULL;
	FreeCache();

}

//-----------------------------------------------------------------------
bool IFileSystem::OnChangeSettings(TiXmlElement* xmlnode)
{
	TiXmlElement* fselem = xmlnode->FindOrCreateChildElement("filesystem");

	for(uint n = 0; n < m_Filesystems.GetCardinality();)
	{
		if(!(m_Filesystems[n].Data->m_iFlags & FSF_CONFIG) || m_Filesystems[n].Data->RefCount() > 0)
		{
			n++;
			continue;
		}

		CStr name = m_Filesystems[n].Data->GetName();

		if(!name.IsEmpty())
		{
			bool ok = m_NamedFileSystems.Remove(name);
			enf_assert(ok);
		}

		SAFE_DELETE(m_Filesystems[n].Data);
		m_Filesystems.RemoveElement(n);
	}

	int order = 0;
	for(TiXmlElement* paths = fselem->FirstChildElement("path"); paths; paths = paths->NextSiblingElement("path"))
	{
		CStr directory = paths->Attribute("directory");
		if(directory.IsEmpty())
			continue;

		CStr name = paths->Attribute("name");
		CStr strOrder = paths->Attribute("order");
		int fsorder;
		if(!strOrder.IsEmpty())
			fsorder = strOrder.ToInt();
		else
			fsorder = order++;

		Add(directory, true, name, fsorder);
	}
	return true;
}

//-----------------------------------------------------------------------
void IFileSystem::OnInsert(TiXmlElement* xmlnode)
{
	OnChangeSettings(xmlnode);
}

//-----------------------------------------------------------------------
char* FileSystem::GetPathPart(char *destFile, char *path)
{
	int pos;

	pos = strlen(path) - 1;
	while (pos >= 0 && !IsDelimiter(path[pos]))
		pos--;

	strcpy(destFile, &path[pos + 1]);
	path[pos + 1] = 0;
	
	return path;
}

//-----------------------------------------------------------------------
VFile::~VFile()
{
	if(Owner)
		Owner->FreeHandle(m_FileHandle);
	m_FileHandle = BAD_FILE_HANDLE;
}

//-----------------------------------------------------------------------
FileHandle VFile::GetHandle() const
{
	static_cast<IFileSystem*>(Owner)->AddRefHandle(m_FileHandle);
	return m_FileHandle;
}

//-----------------------------------------------------------------------
CStr VFile::GetName() const
{
	return Owner->GetNameFromHandle(m_FileHandle);
}

//-----------------------------------------------------------------------
char* VFile::Gets(char *buffer, FileOffset len)
{
char *s = buffer;

	while(len--)
   {
		if(Read((void *)s, 1) != 1)
      	break;

      s++;

      if(s[-1] == '\n')
      {
			*s++ = 0;
         return buffer;
      }
   }

   return NULL;
}

ENF_INLINE FileOffset AlignOnCachePage(FileOffset off)
{
	return off & ~((FileOffset)(CACHE_PAGE_SIZE - 1));
}


TimeStamp totalTicks = 0;
int64 totalBytes = 0;
int megsPerSec = 0;

//-----------------------------------------------------------------------
void PhysFile::Preread(FileOffset offset, FileOffset length)
{
	//read some pages ahead
	FileOffset lastPage = AlignOnCachePage(offset + length);
	offset = AlignOnCachePage(offset);

	CMutexLock lockCache(static_cast<IFileSystem*>(Owner)->m_CacheMutex);

	for(; offset <= lastPage; offset += FileOffset(CACHE_PAGE_SIZE))
	{
		if(offset > m_Length)
			break;

		if(m_CachedPages.GetIndexOf(CachedPageRefByOffset(offset, NULL)) == INDEX_NOT_FOUND)
		{
			uint pageIndex;
			CachePage* page = static_cast<IFileSystem*>(Owner)->RequestPage(this, enf_min(m_Length - offset, FileOffset(CACHE_PAGE_SIZE)), offset, &pageIndex);
			//unlock
			IL_EXCHANGE(page->m_Lock, 0);
		}
	}
}

//-----------------------------------------------------------------------
void PhysFile::PrereadFromPage(FileOffset nextPage, uint index)
{
	enf_assert(index >= 0 && index < m_CachedPages.GetCardinality());

	//read some pages ahead
	for(uint page = 0; page < CACHE_PREREAD_PAGES; page++)
	{
		nextPage += FileOffset(CACHE_PAGE_SIZE);
		if(nextPage > m_Length)
			break;

		index++;
		if(m_CachedPages.GetCardinality() <= index || m_CachedPages[index].offset > nextPage)
		{
			enf_assert(m_CachedPages.GetIndexOf(CachedPageRefByOffset(nextPage, NULL)) == INDEX_NOT_FOUND);
			CachePage* page = static_cast<IFileSystem*>(Owner)->RequestPage(this, enf_min(m_Length - nextPage, FileOffset(CACHE_PAGE_SIZE)), nextPage, &index);
			//unlock
			IL_EXCHANGE(page->m_Lock, 0);
		}
	}
}
//-----------------------------------------------------------------------
FileOffset PhysFile::Read(void *buffer, FileOffset offset, FileOffset len)
{
	//crop the length
   len = enf_min(len, m_Length - offset);

	//nothing to read
	if(len <= 0)
		return 0;

	FileOffset totalRead = 0;
	//totalBytes += len;
	//Timer::PushTime();

	FileOffset endOffset = enf_min(m_Length, AlignOnCachePage(offset + len + (CACHE_PAGE_SIZE - 1)));
	FileOffset alignedOffset = AlignOnCachePage(offset);

	uint index = INDEX_NOT_FOUND;
	while(alignedOffset < endOffset)
	{
		//read whole page, or what rests
		size_t toRead = enf_min((FileOffset)(m_Length - alignedOffset), (FileOffset)CACHE_PAGE_SIZE);

		CachePage* ppage = NULL;
		size_t srcOffset = offset - alignedOffset;
		size_t toCopy = enf_min(toRead, (size_t)len);
		size_t toEndOfPage = CACHE_PAGE_SIZE - srcOffset;
		{
			CMutexLock lockCache(static_cast<IFileSystem*>(Owner)->m_CacheMutex);

			if(index == INDEX_NOT_FOUND || index >= m_CachedPages.GetCardinality() || m_CachedPages[index].offset != alignedOffset)
			{
				index = m_CachedPages.GetIndexOf(CachedPageRefByOffset(alignedOffset, NULL));
			}

			if(index == INDEX_NOT_FOUND)
			{
				ppage = static_cast<IFileSystem*>(Owner)->RequestPage(this, toRead, alignedOffset, &index);
			}
			else
			{
				ppage = m_CachedPages[index].page;
				IL_EXCHANGE(ppage->m_Lock, 1);
			}

			if(toCopy >= toEndOfPage)
			{
				toCopy = toEndOfPage;
				//when approaching end of page, try to preread something
				PrereadFromPage(alignedOffset, index);
			}
			totalRead += toCopy;
		}


		if(!ppage->IsReady())
		{
			FileError err = static_cast<IFileSystem*>(Owner)->WaitForData(this, ppage);
			if(err != FILEERR_OK)
			{
				static_cast<IFileSystem*>(Owner)->SetLastError(err);
				return totalRead;
			}
		}

#ifdef ENF_XENON
		XMemCpy(buffer, (ubyte*)ppage->m_pBuffer + srcOffset, toCopy);
#else
		memcpy(buffer, (ubyte*)ppage->m_pBuffer + srcOffset, toCopy);
#endif
		//unlock
		IL_EXCHANGE(ppage->m_Lock, 0);

		buffer = (void*)((ubyte*)buffer + toCopy);
		len -= toCopy;
		offset += toCopy;
		alignedOffset += CACHE_PAGE_SIZE;
	}
	
	//totalTicks += Timer::PopTick();
	//megsPerSec = (uint)((totalBytes / Timer::Tick2Time(totalTicks, 1)) / 1024.0);
	return totalRead;
}


//-----------------------------------------------------------------------
void PhysFile::Seek(FileOffset offset, SeekMode mode)
{
	if(mode == SEEKMODE_CURRENT)
     	offset += m_Pos;

//	offset = enf_min(offset, m_Length);

	m_Pos = offset;
}

//-----------------------------------------------------------------------
void IFileSystem::RemoveAll()
{
	for(int n = m_Filesystems.GetCardinality() - 1; n >= 0; n--)
		delete m_Filesystems[n].Data;
	m_Filesystems.Clear();
}

//-----------------------------------------------------------------------
bool IFileSystem::Remove(CStr name)
{
	for(uint n = 0; n < m_Filesystems.GetCardinality(); n++)
	{
		CStr fsName = m_Filesystems[n].Data->GetName();

		if(fsName.cmpi(name) == 0)
		{
			if(!name.IsEmpty())
			{
				bool ok = m_NamedFileSystems.Remove(name);
				enf_assert(ok);
			}

			m_Filesystems[n].Data->Release();
			m_Filesystems.RemoveElement(n);
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------
void IFileSystem::UnloadFile(void* ptr)
{
	if(ptr != NULL)
	{
		MemoryManager::Free(ptr);
	}
}

//-----------------------------------------------------------------------
int IFileSystem::Load(CStr name, void** ptr, size_t* len, bool nullTerminate)
{
	enf_assert(ptr != NULL && len != NULL);

	VFile* file = Open(name, FILEMODE_READ);

	if(file == false)
		return false;

	FileOffset length = file->GetLength();
   if(*ptr == NULL)
   {
		if(nullTerminate)
		{
			char *cptr = (char*)MemoryManager::Alloc(length + 1, __FILE__, __LINE__);
			if((*ptr = (void *)cptr)==NULL)
				return false;

			cptr[length] = 0;
		}
		else
		{
			*ptr = MemoryManager::Alloc(length, __FILE__, __LINE__);
		}
   }

   file->Read(*ptr, length);
	file->Close();

   *len = length;

   return true;
}

//-----------------------------------------------------------------------
VFile* IFileSystem::GlobalOpen(CStr name, uint mode)
{
	CMutexLock lock(m_Mutex);
	return InternalSysOpenFile(NULL, name.cstr(), name.cstr(), mode, false, NULL, 0);
}

//-----------------------------------------------------------------------
bool FileSystem::IsAbsoluteFileName(CStr name) const
{
	//hard to say for empty name ;)
	if(!name.IsEmpty())
	{
		//named filesystem is referenced
		if(name[0] == '$')
			return false;

		// ./ and .. case
		if(name[0] == '.')
			return true;

		// root case
		if(IsDelimiter(name[0]))
			return true;

		// C:\ or game:\ case
		if(name.Length() >= 3)
		{
			const char* del = name.strchr(':');
			if(del && IsDelimiter(del[1]))
				return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------
IFileSystem::FilePrepare IFileSystem::PrepareName(CStr& name, FileSystemDef*& fs) const
{
	fs = NULL;
	bool absFile = IsAbsoluteFileName(name);
	if(!absFile)
	{
		if(name[0] == '$')
		{
			const char* ddot = name.strchr(':');
			if(ddot)
			{
				SString<32> fsname;
				fsname.Copy(name.cstr() + 1, ddot - name.cstr() - 1);

				if(!m_NamedFileSystems.Find(fsname.cstr(), fs))
					return FN_BAD;
				name = ddot + 1;
			}
		}
		return FN_LOCAL;
	}
	else
	{
		return FN_ABSOLUTE;
	}
}

//-----------------------------------------------------------------------
VFile* IFileSystem::SysOpenFile(const char* origname, FileSystemDef* fs, const char* name, uint mode, bool explicitFS, const wchar_t* titleName, size_t reserveSize)
{
	char path[MAX_FILENAME_LENGTH];
	SysGetFilePath(fs, name, path);

	return InternalSysOpenFile(fs, name, path, mode, explicitFS, titleName, reserveSize);
}

//-----------------------------------------------------------------------
VFile* IFileSystem::CreateContent(CStr name, const wchar_t* titleName, size_t reserveSize)
{
	CMutexLock lock(m_Mutex);

	FileSystemDef* fs;
	FilePrepare fp = PrepareName(name, fs);
	if(fp == FN_BAD)
		return false;

	if(fs == NULL)
		return false;

	VFile* file = SysOpenFile(name.cstr(), fs, name.cstr(), FILEMODE_WRITE, fs != NULL, titleName, reserveSize);
	return file;
}

//-----------------------------------------------------------------------
char Mode2Type(uint mode)
{
	if((mode & FILEMODE_MASK) == FILEMODE_READ)
	{
		return (mode & FILEFLAGS_NO_PREREAD) ? 'O' : 'R';
	}
	else
	{
		return 'W';
	}
}

//-----------------------------------------------------------------------
VFile* IFileSystem::Open(CStr name, uint mode)
{
	CMutexLock lock(m_Mutex);
	
	FileSystemDef* fs;
	FilePrepare fp = PrepareName(name, fs);
	if(fp == FN_BAD)
		return false;

	char sname[MAX_FILENAME_LENGTH];
	if(fp == FN_ABSOLUTE)
		ConvertToSystemName(sname, name);

	if(mode == FILEMODE_WRITE)
   {
		if(fp == FN_ABSOLUTE)
		{
			return InternalSysOpenFile(NULL, sname, sname, mode, false, NULL, 0);
		}
		for(uint n = 0; n < m_Filesystems.GetCardinality(); n++)
		{
			FileSystemDef* cf = m_Filesystems[n].Data;

			if(fs)
				cf = fs;

			//cannot write to ZIP
			if(cf->Type == FSTYPE_DIR || cf->Type == FSTYPE_CONTENT)
			{
				VFile* file = SysOpenFile(name.cstr(), cf, name.cstr(), mode, fs != NULL, NULL, 0);

				if(file != NULL)
					return file;

				char path[MAX_FILENAME_LENGTH];
				SysGetFilePath(cf, name.cstr(), path);

				uint attr = SysFileAttributes(path);
				//if the file exists, but it's read-only, take it as error and don't
				//fuck around other filesystems
				if((attr != FILEATTR_INVALID) && (attr & FILEATTR_READONLY)) 
					return NULL; 
			}
			if(fs)
				break;
		}

		return NULL;
   }

	if(fp == FN_ABSOLUTE)
	{
		return InternalSysOpenFile(NULL, sname, sname, mode, false, NULL, 0);
	}

	VFile* file = NULL;
	for(uint n = 0; n < m_Filesystems.GetCardinality() && file == NULL; n++)
   {
		FileSystemDef* cf = m_Filesystems[n].Data;

		if(fs)
			cf = fs;

		if(cf->Type == FSTYPE_DIR || cf->Type == FSTYPE_CONTENT)
		{
			file = SysOpenFile(name.cstr(), cf, name.cstr(), mode, fs != NULL, NULL, 0);
		}
		else
		{
			char tmp[MAX_FILENAME_LENGTH];
			
			ConvertToInternalName(tmp, name);

			PackFile* zipfile = cf->m_pPack;

			uint elem = zipfile->LocateFile(tmp, false);
			if(elem != INDEX_NOT_FOUND)
			{
				file = zipfile->OpenFile(this, cf, elem, mode);
				if(file)
				{
					if(m_bLogging)
					{
						enf::FileHandle fh = file->GetHandle();
						uint order = this->GetOrdinalNumberFromHandle(fh);
						this->FreeHandle(fh);
						m_Log.TSPrintf(" %c %x %s:%s\n", Mode2Type(mode), order, cf->m_strName.cstr(), name.cstr());
					}
				}
			}
		}

		if(fs)
			break;
   }

   return file;
}

//-----------------------------------------------------------------------
VFile* IFileSystem::Open(FileHandle handle, uint mode)
{
	CMutexLock lock(m_Mutex);

	FileHandleRef* ref = GetHandleRef(handle);

	if(ref == NULL)
		return NULL;

	CStr name = ref->m_strName;
	FileSystemDef* fs = ref->m_pFS;
	FilePrepare fp = FN_LOCAL;

	if(fs == NULL)
	{
		fp = PrepareName(name, fs);
		if(fp == FN_BAD)
			return false;
	}

	char sname[MAX_FILENAME_LENGTH];
	if(fp == FN_ABSOLUTE)
		ConvertToSystemName(sname, name);

	if(mode == FILEMODE_WRITE)
   {
		if(fp == FN_ABSOLUTE)
		{
			return InternalSysOpenFile(NULL, sname, sname, mode, false, NULL, 0);
		}
		for(uint n = 0; n < m_Filesystems.GetCardinality(); n++)
		{
			FileSystemDef* cf = m_Filesystems[n].Data;

			if(fs)
				cf = fs;

			//cannot write to ZIP
			if(cf->Type == FSTYPE_DIR || cf->Type == FSTYPE_CONTENT)
			{
				VFile* file = SysOpenFile(name.cstr(), cf, name.cstr(), mode, fs != NULL, NULL, 0);

				if(file != NULL)
					return file;

				char path[MAX_FILENAME_LENGTH];
				SysGetFilePath(cf, name.cstr(), path);

				uint attr = SysFileAttributes(path); 
				//if the file exists, but it's read-only, take it as error and don't
				//fuck around other filesystems
				if((attr != FILEATTR_INVALID) && (attr & FILEATTR_READONLY)) 
					return NULL; 
			}
			if(fs)
				break;
		}

		return NULL;
   }

	if(fp == FN_ABSOLUTE)
	{
		return InternalSysOpenFile(NULL, sname, sname, mode, false, NULL, 0);
	}

	VFile* file = NULL;
	for(uint n = 0; n < m_Filesystems.GetCardinality() && file == NULL; n++)
   {
		FileSystemDef* cf = m_Filesystems[n].Data;

		if(fs)
			cf = fs;

		if(cf->Type == FSTYPE_DIR || cf->Type == FSTYPE_CONTENT)
		{
			file = SysOpenFile(name.cstr(), cf, name.cstr(), mode, fs != NULL, NULL, 0);
		}

		else if(cf->Type == FSTYPE_PAK)
		{
			PackFile* zipfile = (PackFile*)cf->m_pPack;

			file = zipfile->OpenFile(this, cf, ref->m_iUserIndex, mode);
			if(file)
			{
				if(m_bLogging)
				{
					uint order = GetOrdinalNumberFromHandle(handle);
					m_Log.TSPrintf(" %c %x %s:%s\n", Mode2Type(mode), order, cf->m_strName.cstr(), name.cstr());
				}
			}
		}

		if(fs)
			break;
   }

   return file;
}

//-----------------------------------------------------------------------
bool IFileSystem::FileExists(CStr name, uint filesystem)
{
	CMutexLock lock(m_Mutex);

	FileSystemDef* fs;
	FilePrepare fp = PrepareName(name, fs);
	if(fp == FN_BAD)
		return false;

	if(filesystem != INDEX_NOT_FOUND)
		fs = m_Filesystems[filesystem].Data;

	if(fp == FN_ABSOLUTE)
	{
		char sname[MAX_FILENAME_LENGTH];
		ConvertToSystemName(sname, name);
		return (SysFileAttributes(sname) != FILEATTR_INVALID);
	}

	for(uint n = 0; n < m_Filesystems.GetCardinality(); n++)
   {
		const FileSystemDef* cf = m_Filesystems[n].Data;

		if(fs)
			cf = fs;

		if(cf->Type == FSTYPE_DIR || cf->Type == FSTYPE_CONTENT)
		{
			char path[MAX_FILENAME_LENGTH];
			SysGetFilePath(cf, name.cstr(), path);

			if(SysFileAttributes(path) != FILEATTR_INVALID)
				return true;
		}
		else if(cf->Type == FSTYPE_PAK)
		{
			char tmp[MAX_FILENAME_LENGTH];

			ConvertToInternalName(tmp, name);

			if(cf->m_pPack->FileExists(tmp))
			{
				return true;
			}
		}

		if(fs)
			break;
   }

   return false;
}

//-----------------------------------------------------------------------
PhysFile::PhysFile(IFileSystem* fsys, IFileSystem::FileSystemDef* fsdef, CStr name, FileMode mode):
	VFile(fsys),
	m_Length(0),
	m_Pos(0),
	m_pFSDef(fsdef),
	m_Mode(mode)
{
	m_FileHandle = fsys->AllocHandle(fsdef, name, false, 0);
}

//-----------------------------------------------------------------------
PhysFile::~PhysFile()
{
}


//-----------------------------------------------------------------------
bool IFileSystem::SetFileDescription(CStr name, const SFileDescription& desc)
{
	VFile* vf = Open(name, FILEMODE_OPENW);
	bool res = false;

	if(vf != NULL)
	{
		res = vf->SetDescription(desc);
		vf->Close();
	}
	return res;
}

//-----------------------------------------------------------------------
bool IFileSystem::GetFileDescription(CStr name, SFileDescription& desc)
{
	//TODO: this method sucks!
	VFile* vf = Open(name, FILEMODE_READ|FILEFLAGS_NO_PREREAD);
	bool res = false;
	
	if(vf != NULL)
	{
		res = vf->GetDescription(desc);
		vf->Close();
	}

	uint attr = SysFileAttributes(name);
	if(attr != FILEATTR_INVALID)
	{
		if(attr & FILEATTR_DIRECTORY)
		{
			desc.Attributes = attr;
			enf::strcpy(desc.FileName, name.cstr());
			desc.FileSize = 0;
			return true;
		}
	}

	return res;
}

//-----------------------------------------------------------------------
uint FileSystem::ConvertToSystemName(char *out, CStr in)
{
	uint n;
	for(n = 0; in[n] != 0; n++)
	{
		out[n] = ToSysDelimiter(in[n]);
	}
	out[n] = 0;
	return n;
}

//-----------------------------------------------------------------------
uint FileSystem::ConvertToInternalName(char *out, CStr in)
{
	uint n;
	for(n = 0; in[n]; n++)
	{
		char chr = ::tolower(in[n]);
		out[n] = IsDelimiter(chr) ? '/' : chr;
	}
	out[n] = 0;
	return n;
}

//-----------------------------------------------------------------------
bool IFileSystem::GetRelativeName(CStr _name, char* rname) const
{
	CStr name = _name;
	FileSystemDef* fs;
	FilePrepare fp = PrepareName(name, fs);
	if(fp == FN_BAD)
		return false;

	if(fp != FN_ABSOLUTE)
	{
		char cname[MAX_FILENAME_LENGTH];
		uint len = ConvertToSystemName(cname, _name);
		ConvertToInternalName(rname, cname);
		return true;
	}

	char cname[256];
	uint len = ConvertToSystemName(cname, name);

   for(uint n = 0; n < m_Filesystems.GetCardinality(); n++)
   {
		const FileSystemDef* cf = m_Filesystems[n].Data;

		if(fs)
			cf = fs;

		if(cf->Type == FSTYPE_DIR || cf->Type == FSTYPE_CONTENT)
		{
			uint s = cf->m_strPath.Length();
			if(s <= len && enf_strncmpi(cf->m_strPath.cstr(), cname, s) == 0)
			{
				if(IsSystemDelimiter(cname[s]))
				{
					strcpy(rname, cname + s + 1);
					ConvertToInternalName(rname, rname);
					return true;
				}
			}
		}

		if(fs)
			break;
   }
	return false;
}

//-----------------------------------------------------------------------
const char* IFileSystem::SysGetFilePath(const FileSystemDef* fs, const char* name, char* buff) const
{
	uint off = fs->m_strPath.Length();
	if(off > 0)
	{
		memcpy(buff, fs->m_strPath.cstr(), off);
		buff[off++] = '/';
	}
	uint nameLen = enf::strlen(name);
	memcpy(buff + off, name, nameLen);
	off += nameLen;
	buff[off] = 0;

	ConvertToSystemName(buff, buff);
	return buff;
}

//-----------------------------------------------------------------------
bool IFileSystem::GetDirectoryOfFile(CStr name, char* cdir) const
{
	FileSystemDef* fs;
	FilePrepare fp = PrepareName(name, fs);
	if(fp == FN_BAD)
		return false;

	for(uint n = 0; n < m_Filesystems.GetCardinality(); n++)
   {
		const FileSystemDef* cf = m_Filesystems[n].Data;

		if(fs)
			cf = fs;

		if(cf->Type == FSTYPE_DIR)
		{
			char path[MAX_FILENAME_LENGTH];
			SysGetFilePath(cf, name.cstr(), path);

			if(SysFileAttributes(path) != FILEATTR_INVALID)
			{
				strcpy(cdir, name.cstr());
				char* sep = strrchr(cdir, '/');
				if(sep)
					*sep = 0;
				return true;
			}
		}
		else if(cf->Type == FSTYPE_PAK)
		{
			if(cf->m_pPack->FileExists (name.cstr()))
			{
				strcpy(cdir, name.cstr());
				char* sep = strrchr(cdir, '/');
				if(sep)
					*sep = 0;
				return true;
			}
		}

		if(fs)
			break;
   }
	return false;
}

//-----------------------------------------------------------------------
bool IFileSystem::GetAbsoluteName(CStr name, char* path, bool mustExist) const
{
	if(path)
	{
		//it's more convenient to initialize it nevertheless we'll find it
		path[0] = 0;
	}

	FileSystemDef* fs;
	FilePrepare fp = PrepareName(name, fs);
	if(fp == FN_BAD)
		return false;

	if(fp == FN_ABSOLUTE)
	{
		if(path)
			strcpy(path, name.cstr());
		return true;
	}

   for(uint n = 0; n < m_Filesystems.GetCardinality(); n++)
   {
		const FileSystemDef* cf = m_Filesystems[n].Data;

		if(fs)
			cf = fs;

		//there is no way to reconstruct absolute name for packed archives
		if(cf->Type == FSTYPE_DIR)
		{
			char temp[MAX_FILENAME_LENGTH];
			SysGetFilePath(cf, name.cstr(), temp);

			if(SysFileAttributes(temp) != FILEATTR_INVALID)
			{
				if(path)
					strcpy(path, temp);
				return true;
			}

			if(fs && !mustExist)
			{
				if(path)
					strcpy(path, temp);
				return true;
			}
		}
		if(fs)
			break;
   }
	return false;
}

//-----------------------------------------------------------------------
IFileSystem::IFileSystem(CStr profileName, bool logging): FileSystem(profileName, logging), m_bLogging(logging)
{
	g_pFileSystem = this;

	//Prepare some handles
	GrowFileHandles(256);

	if(m_bLogging)
	{
		m_Log.Open("filesystem.log", LOGF_CREATE);
	}

	InitCache();
}

//-----------------------------------------------------------------------
uint IFileSystem::GrowFileHandles(uint count)
{
	uint from = m_FileHandles.GetCardinality();
	m_FileHandles.SetSize(from + count);
	m_iFirstFreeHandle = from;
	for(uint n = 0; n < count; n++)
	{
		if(n == (count - 1))
			m_FileHandles[from + n].m_iNextFree = INDEX_NOT_FOUND;
		else
			m_FileHandles[from + n].m_iNextFree = from + n + 1;
	}

	return from;
}

//-----------------------------------------------------------------------
FileHandle IFileSystem::AllocHandle(IFileSystem::FileSystemDef* fs, CStr name, bool managedName, uint userIndex)
{
	//This is always called with locked mutex
	//CMutexLock lock(m_Mutex);

	uint index = m_iFirstFreeHandle;
	if(m_iFirstFreeHandle == INDEX_NOT_FOUND)
	{
		index = GrowFileHandles(256);
	}

	m_iFirstFreeHandle = m_FileHandles[index].m_iNextFree;

	FileHandleRef& ref = m_FileHandles[index];
	ref.m_iRef = 1;
	ref.m_iSpinCount = (ref.m_iSpinCount + 1) & 0xffff;
	ref.m_pFS = fs;
	ref.m_iUserIndex = userIndex;
	if(fs)
		fs->AddRef();

	ref.m_bManagedName = managedName;

	if(!managedName)
	{
		char* allocName = ENF_NEW char[name.Length() + 1];
		enf::strcpy(allocName, name.cstr());
		ref.m_strName = allocName;
	}
	else
		ref.m_strName = name;

	return (ref.m_iSpinCount << 16) | index;
}

//-----------------------------------------------------------------------
IFileSystem::FileHandleRef* IFileSystem::GetHandleRef(FileHandle handle) const
{
	uint spinCount = (handle >> 16);
	uint index = handle & 0xffff;
	enf_assert(m_FileHandles[index].m_iSpinCount == spinCount);
	enf_assert(m_FileHandles[index].m_iRef > 0);

	return const_cast<IFileSystem::FileHandleRef*>(&m_FileHandles[index]);
}

//-----------------------------------------------------------------------
void IFileSystem::AddRefHandle(FileHandle handle)
{
	CMutexLock lock(m_Mutex);
	FileHandleRef* ref = GetHandleRef(handle);
	ref->m_iRef++;
}

//-----------------------------------------------------------------------
uint IFileSystem::FreeHandle(FileHandle handle)
{
	CMutexLock lock(m_Mutex);
	FileHandleRef* ref = GetHandleRef(handle);
	uint index = handle & 0xffff;

	uint refs = --ref->m_iRef;
	if(refs == 0)
	{
		if(!ref->m_bManagedName)
		{
			char* name = (char*)ref->m_strName.cstr();
			delete[] name;

		}
		ref->m_strName = NULL;
		if(ref->m_pFS)
			ref->m_pFS->Release();
		ref->m_iNextFree = m_iFirstFreeHandle;
		m_iFirstFreeHandle = index;
	}
	return refs;
}

//-----------------------------------------------------------------------
CStr IFileSystem::GetNameFromHandle(FileHandle handle) const
{
	CMutexLock lock(m_Mutex);
	FileHandleRef* ref = GetHandleRef(handle);
	if(ref == NULL)
		return NULL;
	return ref->m_strName;
}

//-----------------------------------------------------------------------
uint IFileSystem::GetOrdinalNumberFromHandle(FileHandle handle) const
{
	if(handle != BAD_FILE_HANDLE)
	{
		CMutexLock lock(m_Mutex);
		FileHandleRef* ref = GetHandleRef(handle);
		if(ref == NULL)
			return 0xffffffff;
		for(uint n = 0; n < m_Filesystems.GetCardinality(); n++)
		{
			if(m_Filesystems[n].Data == ref->m_pFS)
			{
				return (((n & 0xff) << 24) | ref->m_iUserIndex);
			}
		}
	}
	return 0xffffffff;
}

//-----------------------------------------------------------------------
IFileSystem::FileSystemDef::FileSystemDef(const char* name, const char* path, FSType type, uint flags):
	Type(type),
	m_pPack(NULL),
	m_iFlags(flags)
{
	if(name)
		m_strName = name;

	m_strPath = path;

}

//-----------------------------------------------------------------------
IFileSystem::FileSystemDef::~FileSystemDef()
{
	if(m_pPack)
	{
		m_pPack->Close();
		m_pPack = NULL;
	}
}

//-----------------------------------------------------------------------
DString FileSystem::FileSystemNameFromFileName(CStr fileName)
{
	if(!fileName.IsEmpty() && fileName[0] == '$')
	{
		const char* delim = fileName.strchr(':');
		if(delim)
		{
			return DString(fileName.cstr(), delim - fileName.cstr() + 1);
		}
	}

	return "";
}

//-----------------------------------------------------------------------
void FileSystem::ConcatenatePath(char* targetPath, CStr pathPart1, CStr pathPart2)
{
	size_t len1 = pathPart1.Length();
	while(len1 > 0)
	{
		char chr = pathPart1[len1 - 1];
		if(!IsDelimiter(chr))
			break;

		len1--;
	}

	if(len1 > 0)
	{
		memcpy(targetPath, pathPart1.cstr(), len1);
		targetPath[len1++] = '/';
	}

	const char* part2 = pathPart2.cstr();
	while(true)
	{
		char chr = *part2;
		if(!IsDelimiter(chr))
			break;
		part2++;
	}

	strcpy(targetPath + len1, part2);
}

//-----------------------------------------------------------------------
DString FileSystem::ConcatenatePath(CStr pathPart1, CStr pathPart2)
{
	DString res;

	size_t len1 = pathPart1.Length();
	while(len1 > 0)
	{
		char chr = pathPart1[len1 - 1];
		if(!IsDelimiter(chr))
			break;

		len1--;
	}

	res.Assign(pathPart1.cstr(), len1);
	res += '/';

	const char* part2 = pathPart2.cstr();
	while(true)
	{
		char chr = *part2;
		if(!IsDelimiter(chr))
			break;
		part2++;
	}

	res += part2;
	return res;
}

//-----------------------------------------------------------------------
bool FileSystem::MatchPattern(const char* str, const char* pat)
{
	while(*str)
	{
		switch(*pat)
		{
		case '?':
			if (*str == '.') return false;
			break;
		case '*':
			do
			{
				++pat;
			} while (*pat == '*');
			if (!*pat)
				return true;
			while (*str)
			{
				if (MatchPattern(str++, pat))
					return true;
			}
			return false;
		default:
			if (::tolower(*str) != ::tolower(*pat))
				return false;
			break;
		}
		++pat;
		++str;
	}
	while (*pat == '*')
		++pat;
	return !*pat;
}

//-----------------------------------------------------------------------
char* IFileSystem::GetFreeFileName(const char* temp, char* name, uint maxnumber)
{
	for(uint ss = 0; ss < maxnumber; ss++)
	{
		sprintf(name, temp, ss);
		if(!FileExists(name))
			return name;
	}

	return NULL;
}

//-----------------------------------------------------------------------
bool IFileSystem::CopyFile(VFile* target, VFile* source)
{
	enf_assert(target);
	enf_assert(source);
	static const uint BLOCK_SIZE = CACHE_PAGE_SIZE;
	ubyte* buff = ALLOCA(ubyte, BLOCK_SIZE);

	while(true)
	{
		FileOffset read = source->Read(buff, BLOCK_SIZE);
		target->Write(buff, read);
		if(read < BLOCK_SIZE)
			break;
	}
	return true;
}

//-----------------------------------------------------------------------
bool IFileSystem::AddFileSystem(VFile* fin, CStr path, uint flags, CStr name, int order)
{
	FileSystemDef* fs = NULL;

	if(fin == NULL)
	{
		fs = ENF_NEW FileSystemDef(name.cstr(), path.cstr(), FSTYPE_DIR, flags);
	}
	else
	{
		PackFile* pfile = pak::Open(fin);

#ifdef _FIRST_PATCH_
		if(pfile && m_iProductID != 0xffffffff && m_iProductID != pfile->GetProductID())
		{
#ifdef ENF_BUILD
			LogF(LL_ERROR, "Cannot open resource file %s with different product ID", name.cstr());
#endif
			pfile->Close();
			pfile = NULL;
		}
		if(pfile && !m_FileSigns.IsEmpty() && m_FileSigns.Contains(pfile->GetKey()))
		{
#ifdef ENF_BUILD
			LogF(LL_ERROR, "Cannot open resource file %s with unknown key", name.cstr());
#endif
			pfile->Close();
			pfile = NULL;
		}
#endif
		if(pfile != NULL)
		{
			fs = ENF_NEW FileSystemDef(name.cstr(), path.cstr(), FSTYPE_PAK, flags);
			fs->SetPack(pfile);
		}
		else
		{
	#ifdef ENF_BUILD
			LogF(LL_ERROR, "Cannot open resource file %s", path.cstr());
	#endif
		}
	}

	if(fs)
	{
		if(!fs->GetName().IsEmpty())
		{
			bool ok = m_NamedFileSystems.Insert(fs->GetName(), fs);
			enf_assert(ok);
		}

		bool ok2 = m_Filesystems.Insert(TFSRef(order, fs));
		enf_assert(ok2);

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------
void IFileSystem::SetFileSigns(const char* signs)
{
	m_FileSigns.Clear();
	while(*signs)
	{
		const char* end = strstr(signs, ",");
		if(end)
			end++;
		else
			end = signs + strlen(signs);

		DString id;
		id.Assign(signs, end - signs);
		m_FileSigns.Insert(id);
		signs = end;
	}
}

//-----------------------------------------------------------------------
VFindFile::VFindFile(IFileSystem* fs, uint flags, CStr name):
	Owner(fs),
	Name(name),
	FileSystemIndex(INDEX_NOT_FOUND),
	Flags(flags)
{
}

//-----------------------------------------------------------------------
FileOffset VFileInMemory::Read(void* buffer, FileOffset offset, FileOffset length)
{
	offset = enf_limit(offset, 0U, (FileOffset)(m_iDataSize - 1));
	FileOffset toEnd = m_iDataSize - offset;
	FileOffset toRead = enf_min(length, toEnd);
	memcpy(buffer, (const ubyte*)m_pData + offset, toRead);
	return toRead;
}

//-----------------------------------------------------------------------
FileOffset VFileInMemory::Read(void* buffer, FileOffset length)
{
	FileOffset toEnd = m_iDataSize - m_iPos;
	FileOffset toRead = enf_min(length, toEnd);
	memcpy(buffer, (const ubyte*)m_pData + m_iPos, toRead);
	m_iPos += toRead;
	return toRead;
}

//-----------------------------------------------------------------------
void VFileInMemory::Seek(FileOffset offset, SeekMode mode)
{
	if(mode == SEEKMODE_CURRENT)
		offset += m_iPos;

	offset = enf_min(offset, (FileOffset)m_iDataSize);
	m_iPos = offset;
}

//-----------------------------------------------------------------------
bool VFileInMemory::GetDescription(SFileDescription& desc) const
{
	return false;
}


//-----------------------------------------------------------------------
bool FileSystem::DeleteContent(ContentType type, CStr root, const char** names, uint count)
{
	SString<MAX_FILENAME_LENGTH> name;

	for(uint n = 0; n < count; n++)
	{
		name = root;
		name += names[n];
		Delete(name.cstr());
	}
	return true;
}

//-----------------------------------------------------------------------
bool FileSystem::SaveContent(ContentType ct, CStr name, const wchar_t* title, const wchar_t* detail, const void* buff, size_t size, const char* userData)
{
	enf::VFile* file = CreateContent(name.cstr(), title, size);
	if(file)
	{
		file->Write(buff, size);
		file->Close();
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------
bool FileSystem::LoadContent(ContentType ct, CStr name, MemoryBuffer& mbuff)
{
	enf::VFile* file = Open(name.cstr(), FILEMODE_READ);
	if(file)
	{
		size_t size = file->GetLength();
		file->Read(mbuff.Append(file->GetLength()), size);
		file->Close();
		return true;
	}
	return false;
}

}