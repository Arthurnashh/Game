#ifndef _PATCH_HPP
#define _PATCH_HPP

#define USE_DELTA3_BINARY_DIFF 1

class IPatchAction
{
	public:
	enum Result
	{
    /// operation not done
	  Error,
    /// operation done
	  OK,
	  /// operation done in-place, no renaming necessary
	  OKInPlace,
    /// operation of removing was succesfull
    OKRemoved
	};
	virtual Result PerformByName(
		const char *tgtPath, const char *srcPath,const char *dataPath,
		bool removeOnError=true
	) = NULL;
	virtual Result Perform(
		const char *tgtPath, const char *srcPath,const char *dataPath,
		const char *name
	) = NULL;
	virtual void CleanUp() = NULL;
  virtual const char *GetErrorMessage() const {return "";}
	virtual ~IPatchAction(){}
};

extern char XDelta3TmpFile[]; //global
extern const char XDelta3FileName[];

IPatchAction *CreatePatchActionCreate(bool createNew);
IPatchAction *CreatePatchActionDelete();
IPatchAction *CreatePatchActionApply(AutoArray<RString> &uninstalLog, bool otherTargetDir);

#if _ENABLE_PBO_PROTECTION
/// decrypts fileName based on setup key and encrypts it based on serial number key
bool PBODecryptSetupEncryptSerial(const RString& fileName, RString *_errorMessage = NULL);
/// takes src PBO, makes copy of it to dst, decrypts dst based on serial number key and encrypts it based on setup key
bool PBODecryptSerialEncryptSetup(const RString& src, const RString &dst, RString *_errorMessage = NULL);
#endif //#if _ENABLE_PBO_PROTECTION

static void CCALL ToLogF(const char* format, ...);


#endif