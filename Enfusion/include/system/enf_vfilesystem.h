//-----------------------------------------------------------------------------
// File: enf_vfilesystem.h
//
// Desc: Virtual filesystem
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_VFILESYSTEM_H
#define ENF_VFILESYSTEM_H

#include "enf_config.h"
#include "engine/enf_module.h"
#include "common/enf_types.h"
#include "enf_time.h"

namespace enf
{
/** @defgroup Filesystem File system
@{*/
class FileSystem;
class IFileSystem;
class TiXmlElement;
class MemoryBuffer;

extern FileSystem* g_pFileSystem;

//-----------------------------------------------------------------------------
//!File attributes. See FileDescription
enum FileAttribute
{
	FILEATTR_DIRECTORY	= BITF(0),	///<File is directory
	FILEATTR_HIDDEN		= BITF(1),	///<File is hidden
	FILEATTR_READONLY		= BITF(2),	///<File is read-only
	FILEATTR_COMPRESSED	= BITF(3),	///<File is from compressed archive
	FILEATTR_ARCHIVE		= BITF(4),	///<File is archived
	FILEATTR_SYSTEM		= BITF(5),
	FILEATTR_NOINDEXED	= BITF(6),
	FILEATTR_ENCRYPTED	= BITF(7),
	FILEATTR_ISCREATOR	= BITF(8),	///<For consoles, it indicates, that the file is created by current user
	FILEATTR_INVALID		= 0xffffffff
};

//!Mode for opening file. See FileSystem::Open
enum FileMode
{
	FILEMODE_READ	= 1,		///<File is readonly
	FILEMODE_OPENW	= 2,	///<File is just opened for write
	FILEMODE_WRITE	= 3,	///<File is read-write. When do not exists, it's created
	FILEMODE_MASK	= 0xff,
	FILEFLAGS_NO_PREREAD = BITF(8),
	FILEMODE_FORCE_DWORD = 0x7fffffff
};

//!Mode of seeking. See VFile::Seek
enum SeekMode
{
	SEEKMODE_BEGIN,	///<Offset is from the begin of file
	SEEKMODE_CURRENT,	///<Offset is from current position in file
   SEEKMODE_FORCE_DWORD = 0x7fffffff
};

enum FileError
{
	FILEERR_OK,
	FILEERR_NOT_FOUND,
	FILEERR_FAILED,
	FILEERR_NOT_ENOUGH_SPACE,
	FILEERR_DEVICE_REMOVED,
	FILEERR_CORRUPT,
	FILEERR_BAD_OWNER,
	FILEERR_NOT_SIGNED,
};

//-----------------------------------------------------------------------
/*!
Description of file. See VFile::GetDescription(),
FileSystem::GetFileDescription() and
FileSystem::FindFile()
*/
struct SFileDescription
{
	//!Name of file
	char			FileName[MAX_FILENAME_LENGTH];
#ifdef ENF_PS3
	//!PS3 specific
	char			Title[128];
	//!PS3 specific
	char			Detail[128];
	//!PS3 specific
	char			UserData[8];
#endif
	//!Attributes. See enf::FileAttribute
	uint			Attributes;
	//!Creation date&time
	SDateTime	CreationTime;
	//!Last write date&time
	SDateTime	LastWriteTime;
	//!Size of file
	size_t		FileSize;
	//!Size of compressed file
	size_t		CompressedFileSize;

	SFileDescription(): Attributes(0), FileSize(0), CompressedFileSize(0)
	{
#ifdef _DEBUG
		ZeroMem(FileName, sizeof(FileName));
#else
		FileName[0] = 0;
#endif
#ifdef ENF_PS3
		Title[0] = 0;
		ZeroMem(UserData, 8);
#endif
	}
};

//-----------------------------------------------------------------------
/*!
Class for iterating files.
See FileSystem::FindFile()
*/
class PUBLIC_API VFindFile: private CDisableCopy
{
public:
	ENF_DECLARE_MMOBJECT(VFindFile);

	/*!
	Iterates next file
	\param desc
	Description of next found file, if any
	\return
	True when there is next file
	*/
	virtual bool GetNext(SFileDescription& desc)=0;
	
	/*!
	Must be called at the end of iteration.
	It cleans up and deletes the object
	*/
	virtual void Close()=0;
	
	void SetFSIndex(uint index)
	{
		FileSystemIndex = index;
	}

	ENF_INLINE uint GetFileSystemIndex() const {return FileSystemIndex;}

protected:
friend class IFileSystem;
	IFileSystem*	Owner;
	DString			Name;
	uint				FileSystemIndex;
	uint				Flags;

	VFindFile(IFileSystem* fs, uint flags, CStr name);
	virtual ~VFindFile() {};
};

//-----------------------------------------------------------------------
/*!
Class representing opened file
*/
class PUBLIC_API VFile: private CDisableCopy
{
public:
   virtual void Close()=0;
	virtual FileOffset Read(void *buffer, FileOffset offset, FileOffset length)=0;
	virtual void Preread(FileOffset offset, FileOffset length) {}
	virtual FileOffset Read(void *buffer, FileOffset len)=0;
	virtual FileOffset Write(const void *buffer,FileOffset len)=0;
	virtual FileOffset GetPos() const=0;
	virtual FileOffset GetLength() const=0;
	virtual void Seek(FileOffset offset, SeekMode mode = SEEKMODE_BEGIN)=0;
   virtual char* Gets(char *buffer, FileOffset len);
	virtual bool GetDescription(SFileDescription& desc) const=0;
	virtual bool SetDescription(const SFileDescription& desc) { return true; }
	virtual bool IsEOF() const=0;
	CStr GetName() const;
	//!Gets handle to file. Must be released by FileSystem::FreeHandle then!
	FileHandle GetHandle() const;

protected:
	friend class IFileSystem;
	
	FileSystem*	Owner;
	FileHandle		m_FileHandle;

	VFile(FileSystem* fs): Owner(fs) {}
	virtual ~VFile();
};

//-----------------------------------------------------------------------
class PUBLIC_API VFileInMemory: public VFile
{
public:
	virtual void Close() { delete this; }
	virtual FileOffset Read(void *buffer, FileOffset offset, FileOffset length);
	virtual FileOffset Read(void *buffer, FileOffset len);
	virtual FileOffset Write(const void *buffer,FileOffset len) { return 0; }
	virtual FileOffset GetPos() const { return m_iPos; }
	virtual FileOffset GetLength() const { return m_iDataSize; }
	virtual void Seek(FileOffset offset, SeekMode mode = SEEKMODE_BEGIN);
	virtual bool GetDescription(SFileDescription& desc) const;
	virtual bool SetDescription(const SFileDescription& desc) { return true; }
	virtual bool IsEOF() const {return m_iPos == m_iDataSize; }
	VFileInMemory(FileSystem* fs, void* pData, size_t dataSize): VFile(fs), m_pData(pData), m_iDataSize(dataSize), m_iPos(0) {}

protected:
	friend class IFileSystem;
	void*		m_pData;
	size_t	m_iDataSize;
	FileOffset	m_iPos;

	virtual ~VFileInMemory() {}
};

//-----------------------------------------------------------------------
enum FindFileFlags
{
	FFF_DIRECTORIES = BITF(0)
};

//-----------------------------------------------------------------------
enum FileSystemFlags
{
	FSF_FORCE	= BITF(0), ///<When true, path is mandatory and must exist. If not, it's reported to log
	FSF_CONFIG	= BITF(1)
};

enum ContentType
{
	CONTENT_ANY,
	CONTENT_SAVEGAME,
	CONTENT_CONFIG,
	CONTENT_ACHIEVEMENTS,
};

class PUBLIC_API FileSystem: public Module
{
protected:
	/*!
	Destroys whole filesystem.
	*/
	virtual ~FileSystem() {}
	static uint m_iProductID;

public:
	ENF_DECLARE_ITEM(FileSystem, Module);

	void SetCurrent()
	{
		g_pFileSystem = this;
	}
	FileSystem(CStr profileName, bool logging) {}

	virtual void OnInsert(TiXmlElement* xmlnode)=0;
	virtual bool OnChangeSettings(TiXmlElement* xmlnode)=0;

	/*!
	return true if path is absolute. ie. it begins with C:\\
	\param name
	string containing path
	\return
	true if it is absolute, false otherwise.
	*/
	bool IsAbsoluteFileName(CStr name) const;
	
	/*!
	Add filesystem to collection. The earlier added has highest priority
	\param path
	string containing root path to filesystem, or name of zip file
	\param flags
	See FileSystemFlags
	\param name
	Optional name of filesystem
	\param order Ordinal number
	\return
	true if filesystem has been added, false otherwise
	*/
	virtual bool Add(CStr path, uint flags, CStr name, int order)=0;

	/*!
	Add filesystem to collection. The earlier added has highest priority
	\param fin
	VFile representation of data, or NULL when directory
	\param path
	string containing root path to filesystem, or name of zip file
	\param flags
	See FileSystemFlags
	\param name
	Optional name of filesystem
	\param order Ordinal number
	\return
	true if filesystem has been added, false otherwise
	*/
	virtual bool AddFileSystem(VFile* fin, CStr path, uint flags, CStr name, int order)=0;

	//!Remove filesystem by its name
	virtual bool Remove(CStr name)=0;

	/*! Removes all added filesystems*/
	virtual void RemoveAll()=0;
	
	/*!
	Returns number of filesystems
	*/
	virtual uint GetNumFileSystems() const=0;

	/*!
	Returns name of the filesystem
	\param index
	Index of filesystem 0...GetNumFileSystems()
	\return
	The name, might be null
	*/
	virtual CStr GetFileSystemName(uint index) const=0;

	/*!
	Returns path of the filesystem
	\param index
	Index of filesystem 0...GetNumFileSystems()
	\return
	The path
	*/
	virtual CStr GetFileSystemPath(uint index) const=0;

	/*!
	Finds first file that matches pattern (accepts wildcards) and returns
	object VFindFile, when it's found. By this object you can
	iterate all files that match pattern.
	\param pattern
	Name of file, or wildcards (* and ? characters)
	\param firstfile
	Description of the first found file
	\param flags
	Optional flags
	\return
	Object for iterating all files;
	*/
	virtual VFindFile* FindFile(CStr pattern, SFileDescription& firstfile, uint flags = 0, uint filesystem = INDEX_NOT_FOUND)=0;

	/*!
	Loads file if exists. Memory is allocated and it's guaranteed that
	allocated buffer is one byte bigger and zero terminated.
	\param name
	File name
	\param ptr
	pointer to pointer on allocated memory
	\param len
	pointer to length of loaded data
	\return
	true if file is loaded, false otherwise
	*/
	virtual int Load(CStr name, void** ptr, size_t* len, bool nullTerminate = true)=0;

	/*!
	Frees allocated memory by Load() method.
	\param ptr
	Pointer to loaded data
	*/
	virtual void UnloadFile(void* ptr)=0;

	/*!
	Opens file by absolute name.
	\param name
	file name
	\param mode
	read/write mode
	\return
	pointer to VFile instance
	*/
	virtual VFile* GlobalOpen(CStr name, uint mode)=0;

	/*!
	Opens file by relative name.
	\param name	file name
	\param mode	read/write mode
	\return pointer to VFile instance
	*/
	virtual VFile* Open(CStr name, uint mode)=0;

	/*!
	Creates file, specialized version for creating save content
	\param name	file name
	\param titleName Name of save-file, visible in console tools
	\param reserveSize Reserved size for save. When not available, returns NULL
	\return pointer to VFile instance, or NULL when not enough space
	*/
	virtual VFile* CreateContent(CStr name, const wchar_t* titleName, size_t reserveSize)=0;

	virtual bool SaveContent(ContentType ct, CStr name, const wchar_t* title, const wchar_t* detail, const void* buff, size_t size, const char* userData = NULL);
	virtual bool LoadContent(ContentType ct, CStr name, MemoryBuffer& mbuff);
	virtual bool DeleteContent(ContentType ct, CStr root, const char** names, uint count);
	//by default, we have plenty of space
	virtual FileOffset GetContentFreeSpace() const { return 64 * 1024 * 1024; }

	/*!
	Opens file by file handle.
	\param name	file name
	\param mode	read/write mode
	\return pointer to VFile instance
	*/
	virtual VFile* Open(FileHandle handle, uint mode)=0;

	/*!
	Seek for file in all filesystems and returns absolute path to it.
	\param name
	file name
	\param path
	pointer to resulting string
	\param mustExist
	Must the file exist?
	\return
	true if file exists, false otherwise
	*/
	virtual bool GetAbsoluteName(CStr name, char* path, bool mustExist = true) const=0;

	//!Check whether file exists
	virtual bool FileExists(CStr name, uint filesystem = INDEX_NOT_FOUND)=0;

	/*!
	Gets description of file without opening it
	\param name
	Name of file. Relative, or absolute one
	\param desc
	Structure to return the description
	\return
	True when file exists
	*/
	virtual bool GetFileDescription(CStr name, SFileDescription& desc)=0;
	virtual bool SetFileDescription(CStr name, const SFileDescription& desc)=0;

	/*!
	Seek for file in all filesystems and returns relative path to it.
	\param name
	file name
	\param cdir
	pointer to resulting string
	\return
	true if file exists, false otherwise
	*/
	virtual bool GetDirectoryOfFile(CStr name, char* cdir) const=0;

	/*!
	Seek for file in all filesystems and returns relative path to it.
	Filesystem root path is stripped.
	\param name
	file name
	\param rname
	pointer to resulting string
	\return
	true if file exists, false otherwise
	*/
	virtual bool GetRelativeName(CStr name, char* rname) const=0;

	virtual bool Delete(CStr name)=0;

	virtual bool CopyFile(VFile* target, VFile* source)=0;

	/*!
	Makes directory. When it's absolute name, it's just created there.
	When relative one, it's created in the first path, which is directory
	(not compressed archive)
	\param name
	Name of directory
	\return
	True when directory was created
	*/
	virtual bool MakeDirectory(CStr name)=0;

	/*!
	Splits filename from path
	\param dest
	pointer to resulting file name
	\param source
	pointer to source file name. As result there will be only path to file.
	\return
	the same pointer as source parameter.
	*/
	static char* GetPathPart(char* dest, char* source);

	/*!
	Generates not existing file name from template name and
	incremental number. Should be at least MAX_FILENAME_LENGTH big.
	Template name MUST contain %d!!!
	\param temp
	Template of name
	\param name
	Pointer to buffer for resulting name
	\param maxnumber
	Maximum number to be inserted into template name
	\return
	the same pointer as name, or NULL, when name cannot be generated
	*/
	virtual char* GetFreeFileName(const char* temp, char* name, uint maxnumber)=0;

	/*!
	Internal static method for cleaning file names.
	Characters are lowered and slashes are converted to backslashes
	\param out
	resulting string
	\param in
	string to be converted
	\return
	length of resulting string
	*/
	static uint ConvertToSystemName(char *out, CStr in);

	/*!
	Internal static method for cleaning file names.
	Characters are lowered and backslashes are converted to slashes
	\param out
	resulting string
	\param in
	string to be converted
	\return
	length of resulting string
	*/
	static uint ConvertToInternalName(char *out, CStr in);

	/*!
	Tells whether file name matches with pattern
	\param name
	Name of file to be tested
	\param pattern
	Pattern. Standard wildcards are supported (?,*)
	\return
	True when matches
	*/
	static bool MatchPattern(const char* name, const char* pattern);
	static void ConcatenatePath(char* targetPath, CStr pathPart1, CStr pathPart2);
	static DString ConcatenatePath(CStr pathPart1, CStr pathPart2);
	static DString FileSystemNameFromFileName(CStr fileName);

	//!Frees handle. Returns number of references
	virtual uint FreeHandle(FileHandle handle)=0;
	//!Returns file name from handle
	virtual CStr GetNameFromHandle(FileHandle handle) const=0;
	//!Returns ideal sorting number for sequential file access
	virtual uint GetOrdinalNumberFromHandle(FileHandle handle) const=0;

	virtual FileError GetLastError() { return FILEERR_OK; }
	virtual void SetLastError(FileError err) {}
	static void SetProductID(uint productID)
	{
		m_iProductID = productID;
	}
	virtual void SetFileSigns(const char* signs)=0;
};

//-----------------------------------------------------------------------
#define SAFE_DELETE_LOADFILE(p) {GetFileSystem()->UnloadFile(p); p = NULL;}

PUBLIC_API FileSystem* GetFileSystem();
PUBLIC_API bool IsFileSystem();
/*!
Creates new instance of FileSystem.
\param profileName Either name of $profile directory in user profile, or NULL
\param logging FileSystem logger
*/
PUBLIC_API FileSystem* CreateFileSystem(CStr profileName, bool logging = false);
//@}
}
#endif //ENF_VFILESYSTEM_H