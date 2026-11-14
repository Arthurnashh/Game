//-----------------------------------------------------------------------------
// File: enf_vfilesystem.h
//
// Desc: Virtual filesystem, internal
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_IFILESYSTEM_H
#define ENF_IFILESYSTEM_H

#include "enf_config.h"
#include "enf_vfilesystem.h"
#include "common/enf_hashmap.h"
#include "enf_main.h"

namespace enf
{
/** @addtogroup Filesystem File system
@{*/
struct CachePage;
struct OverlappedRequest;
struct OverlappedRequestWin32;
class PhysFile;
class PackFile;

//-----------------------------------------------------------------------
struct CachePage
{
	CachePage*	ENF_LIST_NEXT;
	CachePage*	ENF_LIST_PREV;
	void*			m_pOriginalPtr;
	void*			m_pBuffer;
	PhysFile*	m_pFile;
	FileOffset	m_Offset;
	FileOffset	m_Length;
	OverlappedRequest* m_pRequest;
	IL_INT32		m_Lock;

	ENF_INLINE bool IsReady() const
	{
		return m_pRequest == NULL;
	}
};

#ifdef ENF_CONSOLE
	const uint	CACHE_PAGE_SIZE = 32768;
	const uint	CACHE_SIZE_MEGS = 8;
	const uint	CACHE_PREREAD_PAGES = 4;
#else
	const uint	CACHE_PAGE_SIZE = 32768;
	const uint	CACHE_SIZE_MEGS = 64;
	const uint	CACHE_PREREAD_PAGES = 4;
#endif

//-----------------------------------------------------------------------
class PUBLIC_API IFileSystem: public FileSystem
{
public:
	//-----------------------------------------------------------------------
	enum FSType
	{
		FSTYPE_DIR,
		FSTYPE_PAK,
		FSTYPE_CONTENT //for content save
	};

	//-----------------------------------------------------------------------
	class FileSystemDef: public RefCountObject
	{
	friend class IFileSystem;
	friend class VFindFile;

		PackFile*	m_pPack;
		DString		m_strPath;
		DString		m_strName;
		FSType		Type;
		uint			m_iFlags;
	public:
		ENF_DECLARE_SMALLOBJECT(FileSystemDef);
		FileSystemDef(const char* name, const char* path, FSType type, uint flags);
		~FileSystemDef();
		FSType GetType() const
		{
			return Type;
		}
		CStr GetPath() const
		{
			return m_strPath.cstr();
		}
		void SetPath(CStr path)
		{
			m_strPath = path.cstr();
		}
		CStr GetName() const
		{
			return m_strName.cstr();
		}
		void SetPack(PackFile* p)
		{
			m_pPack = p;
		}
		PackFile* GetPack() const
		{
			return m_pPack;
		}
	};
	typedef Pair<int, FileSystemDef*> TFSRef;

	IFileSystem(CStr profileName, bool logging);

	FileHandle AllocHandle(FileSystemDef* fs, CStr name, bool managedName, uint userIndex);
	void AddRefHandle(FileHandle handle);
	uint FreeHandle(FileHandle handle);
	CStr GetNameFromHandle(FileHandle handle) const;
	uint GetOrdinalNumberFromHandle(FileHandle handle) const;

	virtual void OnInsert(TiXmlElement* xmlnode);
	virtual bool OnChangeSettings(TiXmlElement* xmlnode);

	void RemoveAll();
	bool Remove(CStr name);
	uint GetNumFileSystems() const
	{
		return m_Filesystems.GetCardinality();
	}
	bool ContainsNamedFileSystem(CStr name);
	CStr GetNamedFileSystemPath(CStr name);
	CStr GetFileSystemName(uint index) const;
	CStr GetFileSystemPath(uint index) const;
	int Load(CStr name, void** ptr, size_t* len, bool nullTerminate);
	void UnloadFile(void* ptr);
	VFile* GlobalOpen(CStr name, uint mode);
	VFile* Open(CStr name, uint mode);
	VFile* CreateContent(CStr name, const wchar_t* titleName, size_t reserveSize);
	bool AddFileSystem(VFile* fin, CStr path, uint flags, CStr name, int order);

	VFile* Open(FileHandle handle, uint mode);
	bool GetAbsoluteName(CStr name, char* path, bool mustExist = true) const;
	bool FileExists(CStr name, uint filesystem = INDEX_NOT_FOUND);
	bool CopyFile(VFile* target, VFile* source);

	bool GetFileDescription(CStr name, SFileDescription& desc);
	bool SetFileDescription(CStr name, const SFileDescription& desc);

	bool GetDirectoryOfFile(CStr name, char* cdir) const;
	bool GetRelativeName(CStr name, char* rname) const;
	char* GetFreeFileName(const char* temp, char* name, uint maxnumber);
	void FreeCaches(PhysFile* file);
	void FreeRequest(OverlappedRequest* request);
	CMutex		m_CacheMutex;

/*	const Array<FileSystemDef*>& GetFilesystems() const
	{
		return m_Filesystems;
	}*/
	const Set<TFSRef>& GetFilesystems() const
	{
		return m_Filesystems;
	}

	virtual VFile* InternalSysOpenFile(IFileSystem::FileSystemDef* fsdef, const char* origname, const char* name, uint mode, bool explicitFS, const wchar_t* titleName, size_t reserveSize)=0;
	VFile* SysOpenFile(const char* origname, FileSystemDef* fs, const char* name, uint mode, bool explicitFS, const wchar_t* titleName, size_t reserveSize);
	const char* SysGetFilePath(const FileSystemDef* fs, const char* name, char* buff) const;

	void InitCache();
	void FreeCache();
	CachePage* AllocCachePage(PhysFile* vf, FileOffset offset, uint* pageIndex);
	void FreeCachePage(CachePage* page);
	virtual CachePage* RequestPage(PhysFile* file, FileOffset toRead, FileOffset alignedOffset, uint* pageIndex)=0;
	virtual FileError WaitForData(PhysFile* owner, CachePage* page)=0;

	virtual uint SysFileAttributes(CStr fileName) const=0;
	void SetFileSigns(const char* signs);

protected:
	friend class PhysFile;

	CMutex		m_Mutex;


	Set<TFSRef>	m_Filesystems;
	Set<DString> m_FileSigns;

//	Array<FileSystemDef*>	m_Filesystems;
	HashMap<CStr, FileSystemDef*, CStrHashFunc> m_NamedFileSystems;
	bool	m_bLogging;
	TLog m_Log;
	struct FileHandleRef
	{
		uint	m_iRef;
		CStr	m_strName;
		uint	m_iSpinCount;
		uint	m_iUserIndex;
		bool	m_bManagedName;

		union
		{
			uint	m_iNextFree;
			FileSystemDef*	m_pFS;
		};
		FileHandleRef(): m_iRef(0), m_strName(NULL), m_iSpinCount(0), m_iUserIndex(0), m_bManagedName(false), m_iNextFree(0)
		{
		}
	};
	
	Array<FileHandleRef>	m_FileHandles;
	uint						m_iFirstFreeHandle;

	~IFileSystem();

	SString<MAX_FILENAME_LENGTH>	m_strAppDir;

	static const uint	CACHE_SIZE = CACHE_SIZE_MEGS * 1024 * 1024;
	static const uint	NUM_CACHE_PAGES = CACHE_SIZE / CACHE_PAGE_SIZE;

	CachePage			m_Pages[NUM_CACHE_PAGES];
	List<CachePage>	m_FreePages;
	List<CachePage>	m_UsedPages;
	Array<OverlappedRequest*>	m_FreeRequests;

	enum FilePrepare
	{
		FN_BAD,
		FN_ABSOLUTE,
		FN_LOCAL
	};

	FilePrepare PrepareName(CStr& name, FileSystemDef*& fs) const;
	FileHandleRef* GetHandleRef(FileHandle handle) const;
	uint GrowFileHandles(uint count);
	virtual void InitProfile(CStr profileName) {}
};

struct OverlappedRequest
{
	ENF_DECLARE_SMALLOBJECT(OverlappedRequest);
	CachePage*	m_pPage;

	OverlappedRequest(CachePage* page): m_pPage(page)
	{
	}

	virtual ~OverlappedRequest()
	{
	}
};

struct CachedPageRefByOffset
{
	FileOffset	offset;
	CachePage*	page;

	CachedPageRefByOffset(FileOffset off, CachePage* cp): offset(off), page(cp) {}
	CachedPageRefByOffset(): offset(0), page(NULL) {}


	bool operator==(const CachedPageRefByOffset& other) const
	{
		return offset == other.offset;
	}
	bool operator>(const CachedPageRefByOffset& other) const
	{
		return offset > other.offset;
	}
};

class PUBLIC_API PhysFile: public VFile
{
public:
	ENF_DECLARE_SMALLOBJECT(PhysFile);

	FileOffset Read(void *buffer, FileOffset offset, FileOffset len);
	FileOffset Read(void *buffer,FileOffset len)
	{
		FileOffset totalRead = Read(buffer, m_Pos, len);
		m_Pos += totalRead;
		return totalRead;
	}

	virtual bool GetDescription(SFileDescription& desc) const=0;

	ENF_INLINE void PrereadFromPage(FileOffset nextPage, uint pageIndex);

	virtual void Preread(FileOffset offset, FileOffset length);

	FileOffset GetPos() const
	{
		return m_Pos;
	}

	FileOffset GetLength() const
	{
		return m_Length;
	}

	void Seek(FileOffset offset, SeekMode mode = SEEKMODE_BEGIN);
	bool IsEOF() const
	{
		return m_Pos == m_Length;
	}

	void RemoveCache(CachePage* cp)
	{
		enf_assert(cp->m_pFile == this);
		int index = m_CachedPages.GetIndexOf(CachedPageRefByOffset(cp->m_Offset, cp));
		enf_assert(index >= 0);
		m_CachedPages.RemoveElement(index);
	}

	void AddCache(FileOffset offset, CachePage* cp, uint* pageIndex)
	{
		m_CachedPages.Insert(CachedPageRefByOffset(offset, cp), pageIndex);
	}

	void AddRequest(OverlappedRequest* request)
	{
		bool insert = m_PendingRequests.Insert(request);
		enf_assert(insert);
	}

	void RemoveRequest(OverlappedRequest* request)
	{
		bool removed = m_PendingRequests.Remove(request);
		enf_assert(removed);
	}

protected:
	friend class IFileSystem;
	FileMode		m_Mode;
	FileOffset	m_Length;
	FileOffset	m_Pos;
	Set<CachedPageRefByOffset>	m_CachedPages;
	Set<OverlappedRequest*>	m_PendingRequests;
	IFileSystem::FileSystemDef* m_pFSDef;

	PhysFile(IFileSystem* fs, IFileSystem::FileSystemDef* fsdef, CStr name, FileMode mode);
	~PhysFile();
};

typedef char TPackKey[16];

class PackFile
{
protected:
	virtual ~PackFile() {}
public:
	/*!
	Close a ZipFile opened with Open.
	If there is files inside the .Zip opened with OpenCurrentFile (see later),
	these files MUST be closed with CloseCurrentFile before call Close.
	return UNZ_OK if there is no problem. */
	virtual void Close()=0;

	/*!
	Try locate the file szFileName in the zipfile.
	\returns Index to cache, or INDEX_NOT_FOUND
	*/
	virtual uint LocateFile(const char *szFileName, bool checkDirs) const=0;
	virtual void* FindFirst(const char* fileMask, SFileDescription& desc) const=0;
	virtual void* FindNext(void* handle, const char* fileMask, SFileDescription& desc) const=0;
	virtual bool FileExists (const char *file) const=0;
	virtual uint NumEntries () const=0;
	virtual void GetDescription(uint index, SFileDescription& desc) const=0;

	/* for reading the content of the current zipfile, you can open it, read data
	from it, and close it (you can close it before reading all the file)
	*/

	virtual uint GetProductID() const=0;
	virtual const TPackKey& GetKey() const=0;
	virtual enf::VFile* OpenFile (enf::IFileSystem* fs, enf::IFileSystem::FileSystemDef* fsdef, enf::uint elemIndex, enf::uint mode)=0;
};


//-----------------------------------------------------------------------
//@}
}
#endif //ENF_IFILESYSTEM_H