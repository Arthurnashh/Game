//-----------------------------------------------------------------------------
// File: enf_unpack.cpp
//
// Desc: PAK1 implementation
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "enf_main.h"
#include "common/enf_streams.h"
#include "common/enf_iff.h"
#include "system/enf_packfile.h"
#include "system/enf_ifilesystem.h"
//#include "zlib/zlib.h"
//#include "zlib/zutil.h"

//#define _FIRST_PATCH_

using namespace enf;

namespace pak
{
class IPACKFile;
const uint BUFSIZE = 16384;

class PackItemDirectory;

//-----------------------------------------------------------------------
class PackItem
{
public:
	ENF_DECLARE_SMALLOBJECT(PackItem);
	DString	m_Name;
	PackItemDirectory* m_pParent;
	PackItem(): m_pParent(NULL) {}
	virtual ~PackItem() {}
	virtual bool IsFile() const { return false; }
	virtual bool IsDirectory() const { return false; }
};

//-----------------------------------------------------------------------
class PackItemDirectory: public PackItem
{
public:
	ENF_DECLARE_SMALLOBJECT(PackItemDirectory);
	CArray<Ptr<PackItem> >	m_Items;
	HashMap<CStr, PackItem*, CStrHashFunc> m_ItemsHash;
	virtual bool IsDirectory() const { return true; }
};

//-----------------------------------------------------------------------
class PackItemFile: public PackItem
{
public:
	ENF_DECLARE_SMALLOBJECT(PackItemFile);
	uint		m_Offset;
	uint		m_CompressedLength;
	uint		m_UncompressedLength;
	uint		m_CRC;
	ushort	m_Flags;
	ubyte		m_CompressionType;
	ubyte		m_CompressionLevel;
	uint		m_iOrdinal;
	virtual bool IsFile() const { return true; }
	
	void GetFilePath(char* buffer) const
	{
		//much faster than backward-wise assembling of path
		SArray<const PackItem*, 64> parts;

		const PackItem* item = this;
		while(true)
		{
			if(item->m_pParent == NULL)
				break;
			parts.Insert(item);
			item = item->m_pParent;
		}

		while(true)
		{
			const DString& name = parts.Pop()->m_Name;
			memcpy(buffer, name.cstr(), name.Length());
			buffer += name.Length();

			if(parts.IsEmpty())
				break;

			*buffer++ = '/';
		}
		*buffer = 0;
	}
};

//-----------------------------------------------------------------------
class File: public enf::VFile
{
protected:
	friend class IPACKFile;

	IPACKFile*			m_pPack;
	uint					m_iFileElem;
	FileOffset			m_iBeginOffset;
	FileOffset			m_iLength;

	virtual ~File() {}
public:
	File(enf::IFileSystem* fs, IPACKFile* pfile, PackItemFile* fileItem);

	virtual FileOffset GetLength() const
	{
		return m_iLength;
	}

	virtual FileOffset Write(const void *buffer, FileOffset len)
	{
		enf_assert(!"Unsupported");
		return 0;
	}

	virtual bool GetDescription(enf::SFileDescription& desc) const;
};

//-----------------------------------------------------------------------
class UncompressedFile: public File
{
protected:
	FileOffset m_Pos;

public:
	ENF_DECLARE_SMALLOBJECT(File);

	UncompressedFile(enf::IFileSystem* fs, IPACKFile* pfile, PackItemFile* fileItem): File(fs, pfile, fileItem), m_Pos(0)
	{
	}
	virtual void Close()
	{
		delete this;
	}

	virtual FileOffset Read(void* buf, FileOffset len);
	virtual FileOffset Read(void* buf, FileOffset offset, FileOffset length);
	virtual void Seek(FileOffset offset, enf::SeekMode mode)
	{
		if(mode == SEEKMODE_CURRENT)
			offset += m_Pos;

		offset = enf_min(offset, m_iLength);

		m_Pos = offset;
	}
	
	virtual FileOffset GetPos() const
	{
		return m_Pos;
	}
	virtual bool IsEOF() const
	{
		return (m_Pos == m_iLength);
	}
};

//-----------------------------------------------------------------------
#ifdef ENF_DONE
class CompressedFile: public File
{
protected:
	FileOffset m_iCompressedLength;

	ubyte			m_ReadBuffer[BUFSIZE];
	FileOffset	m_iCompressedPos;
	z_stream		m_Stream;

	ENF_INLINE FileOffset GetRestUncompressed() const
	{
		return m_iLength - m_Stream.total_out;
	}

public:
	ENF_DECLARE_MMOBJECT(File);
	CompressedFile(enf::IFileSystem* fs, IPACKFile* pfile, PackItemFile* fileItem);
	~CompressedFile();
	virtual void Close()
	{
		delete this;
	}
	virtual FileOffset Read(void* buf, FileOffset len);
	virtual FileOffset Read(void* buf, FileOffset offset, FileOffset length);
	virtual void Seek(FileOffset offset, enf::SeekMode mode);
	virtual FileOffset GetPos() const;
	virtual bool IsEOF() const;
};
#endif
//-----------------------------------------------------------------------
class IPACKFile: public enf::PackFile
{
protected:
	friend class File;

	enf::Array<PackItemFile*>	m_Files;

public:
	PackItemDirectory*	m_pRoot;
	enf::VFile*		m_pFile;
	SDateTime		m_DateTime;
	CMutex			m_Mutex;
	uint				m_ProductID;
	enf::TPackKey	m_Key;

	IPACKFile(VFile* file): m_pRoot(NULL), m_pFile(file) {}
	uint GetProductID() const
	{
		return m_ProductID;
	}

	const TPackKey& GetKey() const
	{
		return m_Key;
	}

	void r_Cleanup(PackItemDirectory* dir)
	{
		for(uint n = 0; n < dir->m_Items.GetCardinality(); n++)
		{
			PackItem* item = dir->m_Items[n];

			if(item->IsDirectory())
			{
				r_Cleanup(static_cast<PackItemDirectory*>(item));
			}
			else
			{
				delete item;
			}
		}
		delete dir;
	}

	virtual ~IPACKFile()
	{
		if(m_pRoot)
		{
			r_Cleanup(m_pRoot);
			m_pRoot = NULL;
		}
		m_Files.Clear();
	}

	ENF_INLINE enf::VFile* GetFile() const { return m_pFile; }
	void Close()
	{
		if(m_pFile)
		{
			m_pFile->Close();
			m_pFile = NULL;
		}
		delete this;
	}

	uint LocateFile(const char* szFileName, bool checkDirs) const
	{
		const char* str = szFileName;
		const PackItemDirectory* dir = m_pRoot;
		if(dir == NULL)
			return INDEX_NOT_FOUND;

		char temp[256];
		while(*str)
		{
			const char* slash = enf::strchr(str, '/');
			if(slash == NULL)
			{
				//file
				PackItem* file;
				if(!dir->m_ItemsHash.Find(CStr(str), file) )
					return INDEX_NOT_FOUND;

				if(file->IsDirectory() && !checkDirs)
					return INDEX_NOT_FOUND;

				return static_cast<PackItemFile*>(file)->m_iOrdinal;
			}
			else
			{
				memcpy(temp, str, slash - str);
				temp[slash - str] = 0;
				PackItem* newDir;
				if(!dir->m_ItemsHash.Find(CStr(temp), newDir) || !newDir->IsDirectory())
					return INDEX_NOT_FOUND;

				dir = static_cast<PackItemDirectory*>(newDir);
				str = slash + 1;
			}
		}
		return INDEX_NOT_FOUND;
	}
	void* FindNext(void* _current, const char* fileMask, SFileDescription& desc) const
	{
		const PackItem* curr = reinterpret_cast<const PackItem*>(_current);
		const PackItemDirectory* dir = curr->m_pParent;
		bool doFind = false;
		for(uint n = 0; n < dir->m_Items.GetCardinality(); n++)
		{
			PackItem* file = dir->m_Items[n];

			if(doFind)
			{
				if(FileSystem::MatchPattern(file->m_Name.cstr(), fileMask))
				{
					enf::strcpy(desc.FileName, file->m_Name.cstr());
					desc.CreationTime = m_DateTime;
					desc.LastWriteTime = m_DateTime;
					if(file->IsFile())
					{
						PackItemFile* ffile = static_cast<PackItemFile*>(file);
						desc.Attributes = (ffile->m_CompressionLevel ? FILEATTR_COMPRESSED : 0) | FILEATTR_READONLY;
						desc.CompressedFileSize = ffile->m_CompressedLength;
						desc.FileSize = ffile->m_UncompressedLength;
					}
					else
					{
						desc.Attributes = FILEATTR_DIRECTORY;
						desc.CompressedFileSize = 0;
						desc.FileSize = 0;
					}

					return file;
				}
			}
			else if(file == curr)
			{
				doFind = true;
			}
		}
		return NULL;
	}

	void* FindFirst(const char* fileMask, SFileDescription& desc) const
	{
		const char* str = fileMask;
		const PackItemDirectory* dir = m_pRoot;
		if(dir == NULL)
			return NULL;

		char temp[256];
		while(*str)
		{
			const char* slash = enf::strchr(str, '/');
			if(slash == NULL)
			{
				for(uint n = 0; n < dir->m_Items.GetCardinality(); n++)
				{
					PackItem* file = dir->m_Items[n];

					if(FileSystem::MatchPattern(file->m_Name.cstr(), str))
					{
						enf::strcpy(desc.FileName, file->m_Name.cstr());
						desc.CreationTime = m_DateTime;
						desc.LastWriteTime = m_DateTime;
						if(file->IsFile())
						{
							PackItemFile* ffile = static_cast<PackItemFile*>(file);
							desc.Attributes = (ffile->m_CompressionLevel ? FILEATTR_COMPRESSED : 0) | FILEATTR_READONLY;
							desc.CompressedFileSize = ffile->m_CompressedLength;
							desc.FileSize = ffile->m_UncompressedLength;
						}
						else
						{
							desc.Attributes = FILEATTR_DIRECTORY;
							desc.CompressedFileSize = 0;
							desc.FileSize = 0;
						}

						return file;
					}
				}
				return NULL;
			}
			else
			{
				memcpy(temp, str, slash - str);
				temp[slash - str] = 0;
				PackItem* newDir;
				if(!dir->m_ItemsHash.Find(CStr(temp), newDir) || !newDir->IsDirectory())
					return NULL;

				dir = static_cast<PackItemDirectory*>(newDir);
				str = slash + 1;
			}
		}
		return NULL;
	}

	bool FileExists (const char* file) const
	{
		return LocateFile(file, true) != INDEX_NOT_FOUND;
	}

	uint NumEntries () const
	{
		return m_Files.GetCardinality();
	}

	void GetDescription(uint index, SFileDescription& desc) const
	{
		PackItem* item = m_Files[index];
		PackItemFile* pfile = static_cast<PackItemFile*>(item);

		desc.Attributes = FILEATTR_ARCHIVE|FILEATTR_READONLY;
		desc.CompressedFileSize = pfile->m_CompressedLength;
		desc.FileSize = pfile->m_UncompressedLength;
		desc.CreationTime = m_DateTime;
		desc.LastWriteTime = m_DateTime;

		char* buffer = desc.FileName;
		enf::SArray<pak::PackItem*, 64> stack;

		while(item->m_pParent)
		{
			stack.Insert(item);
			item = item->m_pParent;
		}

		buffer[0] = 0;
		while(true)
		{
			PackItem* item = stack.Pop();
			memcpy(buffer, item->m_Name.cstr(), item->m_Name.Length());
			buffer += item->m_Name.Length();
			if(stack.IsEmpty())
			{
				*buffer = 0;
				break;
			}
			*buffer++ = '/';
		}
	}

	VFile* OpenFile (IFileSystem* fs, IFileSystem::FileSystemDef* fsdef, uint elemIndex, uint mode)
	{
		PackItemFile* fileItem = m_Files[elemIndex];

		File* file = NULL;

		if(fileItem->m_CompressionLevel == 0)
		{
			file = ENF_NEW UncompressedFile(fs, this, fileItem);
		}
		else
		{
			switch(fileItem->m_CompressionType)
			{
			case 1:
#ifdef ENF_DONE
        file = ENF_NEW CompressedFile(fs, this, fileItem);
#endif
				break;

			case 2:
				break;
			}
		}
		//copy file info
		file->m_iFileElem = elemIndex;
		char nameBuffer[MAX_FILENAME_LENGTH];
		fileItem->GetFilePath(nameBuffer);
		file->m_FileHandle = fs->AllocHandle(fsdef, nameBuffer, false, elemIndex);
		return file;
	}

	bool r_ParseDirectory(IFFInput& iff, uint num, PackItemDirectory* parent)
	{
		for(uint n = 0; n < num; n++)
		{
			DPackEntry entry;

			if(iff.Read(&entry, sizeof(entry)) != sizeof(entry))
				return false;

			uint nameLength = entry.NameLength;
			char* entryName = ALLOCA(char, nameLength + 1);
			iff.Read(entryName, nameLength);
			entryName[nameLength] = 0;

			if(entry.Type == PE_FILE)
			{
				DPackFile fileEntry;
				if(iff.Read(&fileEntry, sizeof(fileEntry)) != sizeof(fileEntry))
					return false;

				PackItemFile* pf = ENF_NEW PackItemFile;
				pf->m_Name.Assign(entryName, nameLength);
				pf->m_pParent = parent;
				pf->m_Offset = LE_INT(fileEntry.Offset);
				pf->m_CompressedLength = LE_INT(fileEntry.CompressedLength);
				pf->m_UncompressedLength = LE_INT(fileEntry.UncompressedLength);
				pf->m_CRC = LE_INT(fileEntry.CRC);
				pf->m_Flags = LE_SHORT(fileEntry.Flags);
				pf->m_CompressionType = fileEntry.CompressionType;
				pf->m_CompressionLevel = fileEntry.CompressionLevel;
				parent->m_Items[n] = pf;
				parent->m_ItemsHash.Insert(pf->m_Name.cstr(), pf);
				pf->m_iOrdinal = m_Files.GetCardinality();
				m_Files.Insert(pf);
			}
			else if(entry.Type == PE_DIR)
			{
				DPackDirectory dirEntry;
				if(iff.Read(&dirEntry, sizeof(dirEntry)) != sizeof(dirEntry))
					return false;

				PackItemDirectory* pd = ENF_NEW PackItemDirectory;
				if(m_pRoot == NULL)
					m_pRoot = pd;

				uint numChildren = LE_INT(dirEntry.NumFiles);
				pd->m_Name.Assign(entryName, nameLength);
				pd->m_Items.SetSize(numChildren);
				pd->m_ItemsHash.Reserve(numChildren);
				pd->m_pParent = parent;
				if(parent)
				{
					parent->m_Items[n] = pd;
					parent->m_ItemsHash.Insert(pd->m_Name.cstr(), pd);
				}
				if(!r_ParseDirectory(iff, numChildren, pd))
					return false;
			}
		}
		return true;
	}
};

//-----------------------------------------------------------------------
File::File(enf::IFileSystem* fs, IPACKFile* pfile, PackItemFile* fileItem) : VFile(fs), m_pPack(pfile)
{
	m_iBeginOffset	= fileItem->m_Offset;
	m_iLength = fileItem->m_UncompressedLength;
}

//-----------------------------------------------------------------------
bool File::GetDescription(enf::SFileDescription& desc) const
{
	desc.Attributes = 0;
	desc.LastWriteTime = m_pPack->m_DateTime;
	desc.CreationTime = m_pPack->m_DateTime;
	const PackItemFile* elem = m_pPack->m_Files[m_iFileElem];
	desc.FileSize = elem->m_UncompressedLength;
	desc.CompressedFileSize = elem->m_CompressedLength;
	elem->GetFilePath(desc.FileName);
	return true;
}

//-----------------------------------------------------------------------
FileOffset UncompressedFile::Read(void* buf, FileOffset len)
{
	len = enf_min(m_iLength - m_Pos, len);
	m_pPack->m_Mutex.Lock();
	FileOffset read = m_pPack->GetFile()->Read(buf, m_Pos + m_iBeginOffset, len);
	m_pPack->m_Mutex.Unlock();
	m_Pos += read;
	return read;
}

//-----------------------------------------------------------------------
FileOffset UncompressedFile::Read(void* buf, FileOffset offset, FileOffset length)
{
	length = enf_min(m_iLength - offset, length);
	m_pPack->m_Mutex.Lock();
	FileOffset read = m_pPack->GetFile()->Read(buf, offset + m_iBeginOffset, length);
	m_pPack->m_Mutex.Unlock();
	return read;
}

//-----------------------------------------------------------------------
#ifdef ENF_DONE
FileOffset CompressedFile::Read(void* buf, FileOffset len)
{
	if (len == 0)
		return 0;

	int err = 0;
	uInt iRead = 0;

	m_Stream.next_out = (Bytef *)buf;
	m_Stream.avail_out = enf::enf_min((uInt)len, (uInt)GetRestUncompressed());
	while (m_Stream.avail_out > 0)
	{
		int flush = Z_SYNC_FLUSH;

		if(m_Stream.avail_in == 0 && m_iCompressedPos < m_iCompressedLength)
		{
			uInt uReadThis = enf::enf_min(BUFSIZE, (uInt)(m_iCompressedLength - m_iCompressedPos));

			m_pPack->m_Mutex.Lock();
			uInt uRead = m_pPack->m_pFile->Read(m_ReadBuffer, m_iCompressedPos + m_iBeginOffset, uReadThis);
			m_pPack->m_Mutex.Unlock();

			//serious error
			if(uRead != uReadThis)
				return 0;

			m_iCompressedPos += uReadThis;
			m_Stream.next_in = (Bytef *)m_ReadBuffer;
			m_Stream.avail_in = (uInt)uReadThis;
		}

		uLong uTotalOutBefore = m_Stream.total_out;
		const Bytef* bufBefore = m_Stream.next_out;

		err = inflate(&m_Stream,flush);

		uLong uTotalOutAfter = m_Stream.total_out;
		uLong uOutThis = uTotalOutAfter-uTotalOutBefore;
		iRead += (uInt)(uTotalOutAfter - uTotalOutBefore);

		if (err == Z_STREAM_END)
			return iRead;

		enf_assert(err == Z_OK);

		if (err != Z_OK) 
			break;
	}

	if (err == Z_OK)
		return iRead;

	return 0;
}

//-----------------------------------------------------------------------
FileOffset CompressedFile::Read(void* buf, FileOffset offset, FileOffset length)
{
	enf_assert(!"Unsupported");
	return 0;
}

//-----------------------------------------------------------------------
void CompressedFile::Seek(FileOffset offset, enf::SeekMode mode)
{
	if((mode == SEEKMODE_BEGIN && offset == m_Stream.total_out))
		return; //no movement

	if((mode == SEEKMODE_BEGIN && offset == 0) || (m_Stream.total_out + offset) == 0)
	{
		//move at beginning
		inflateEnd(&m_Stream);
		m_Stream.total_out = 0;
		m_Stream.avail_in = (uInt)0;
		m_Stream.avail_out = (uInt)0;
		int err = inflateInit2(&m_Stream, DEF_WBITS);
		enf_assert(err == Z_OK);
		m_iCompressedPos = 0;
		return;
	}

	if(mode == SEEKMODE_BEGIN)
	{
		enf_assert2(offset >= m_Stream.total_out, "trying to seek backwards on packed file");
		offset -= m_Stream.total_out;
	}

	enf_assert2(offset >= 0, "Only positive seek is supported on packed file");

	while(offset > 0)
	{
		static const FileOffset BUFFER = 8192;

		ubyte buff[BUFFER];

		FileOffset pack = enf_min(offset, BUFFER);

		Read(buff, pack);

		offset -= pack;
	}
}

//-----------------------------------------------------------------------
FileOffset CompressedFile::GetPos() const
{
	return m_Stream.total_out;
}

//-----------------------------------------------------------------------
bool CompressedFile::IsEOF() const
{
	return m_Stream.total_out == m_iLength;
}

//-----------------------------------------------------------------------
CompressedFile::CompressedFile(enf::IFileSystem* fs, IPACKFile* pfile, PackItemFile* fileItem): File(fs, pfile, fileItem)
{
	m_iCompressedLength = fileItem->m_CompressedLength;

	m_Stream.zalloc = (alloc_func)0;
	m_Stream.zfree = (free_func)0;
	m_Stream.opaque = (voidpf)0; 
	m_Stream.avail_in = 0;
	m_Stream.avail_out = 0;
	m_Stream.total_out = 0;

	int err = inflateInit2(&m_Stream, DEF_WBITS);
	enf_assert(err == Z_OK);

	m_iCompressedPos = 0;
}

//-----------------------------------------------------------------------
CompressedFile::~CompressedFile()
{
	inflateEnd(&m_Stream);
}
#endif
//-----------------------------------------------------------------------
enf::PackFile* Open(enf::VFile* file)
{
	CFileInputStream inStream(file, true);
	IFFInput input;
	HunkID id;
	if(!input.Open(&inStream, id))
		return NULL;

	if(id != HunkID("PAC1"))
		return NULL;

	HunkID hunk;
	DPackFileHeader100 head;

	size_t hunkLen;
	while((hunkLen = input.BeginHunk(hunk)) != -1)
	{
		if(hunk == "HEAD")
		{
			if(hunkLen < sizeof(DPackFileHeader100))
				return false;

			input.Read(&head, sizeof(head));
			//allow extended versions
			input.EndHunk(true);
			continue;
		}
		else if(hunk == "DATA")
		{
			//skip data
			input.EndHunk(true);
			continue;
		}
		else if(hunk == "FILE")
		{
			uint version = LE_INT(head.Version);
			uint crc = LE_INT(head.CRC);
			uint productId = LE_INT(head.ProductID);

#ifdef _FIRST_PATCH_

			char productKey[] = "BIStudio19092012";
			uint length = enf::strlen(productKey);

			char key[32];
			sprintf(key, "%08x", (int)(productId ^ crc));
			sprintf(key + 8, "%08x", crc);
			for(uint n = 0; n < 16; n++)
			{
				key[n] = key[n] ^ productKey[n % length];
			}
			if(memcmp(key, head.Key, 16))
				return NULL;
#endif

			IPACKFile* pf = ENF_NEW IPACKFile(file);

			pf->m_ProductID = productId;
			pf->m_DateTime.Year = LE_SHORT(head.Year);
			pf->m_DateTime.Month = head.Month;
			pf->m_DateTime.Day = head.Day;
			pf->m_DateTime.Hour = head.Hour;
			pf->m_DateTime.Minute = head.Minute;
			pf->m_DateTime.Second = head.Second;

			if(!pf->r_ParseDirectory(input, 1, NULL))
			{
				delete pf;
				return NULL;
			}

			//everything must be read
			input.EndHunk();
			return pf;
			//continue;
		}
		//skip unknown hunk
		input.EndHunk(true);
	}
	return NULL;
}

}

