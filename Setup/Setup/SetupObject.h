/////////////////////////////////////////////////////////////////////////////
// CSetupObject - øídící objekt instalace

////////////////////////////////////
// Default data

#define SETUP_DAT_FILE_NAME			"setup.dat"
#define SETUP_CRC_FILE_NAME			_T("setup.crc")
//#define AVI_NAME_SETUP				"setup.avi"
//#define AVI_NAME_COPY				"copy.avi"
#define WAV_NAME_COPY					_T("data\\setup.wav")
#define DIRECTX_SUBDIRECTORY		_T("DirectX")

// aplikace realizuje jednotlivé dílcí kroky formou konecného automatu
// je možné vytváøet vlastní uživatelské stavy >= SETUP_STATE_USER0
// funkce DoSetupStep (resp. DoUserSetupStep) musi provést pøíslušný
// krok a vrací nový stav instalace (nový stav muze být jistou dobu
// shodný s aktuálním stavem).


#ifndef _SETUP_OBJECT_INCLUDE
#define _SETUP_OBJECT_INCLUDE

#pragma warning(disable : 4996)

#include "serial.hpp"
#include <Es/Strings/rString.hpp>
#include <El/QStream/qbStream.hpp>
#include <../Drobnosti/SetupConvert/setupConvertShare.hpp>
#if _ENABLE_PBO_PROTECTION
#include <El/Encryption/PBOHeaderProtection.h>
#endif //#if _ENABLE_PBO_PROTECTION

// stavy instalace
#define	SETUP_STATE_BEGIN		0		// zahájení instalace
#define	SETUP_STATE_STPWIZ		1		// setup - Wizard

#define	SETUP_STATE_COPY_BGN	10		// zahájeno kopírování
#define	SETUP_STATE_COPY_RUN	11		// probíhá kopírování
#define	SETUP_STATE_COPY_END	12		// ukonceno kopírování

#define	SETUP_STATE_DIRECTX		20		// instalace Direct X

#define SETUP_STATE_OPEN_AL   25    // instalace OpenAL runtime

#define SETUP_STATE_FIREWALL  28    // open the firewall

#define	SETUP_STATE_SHORTCUTS	30		// vytvoøení shortcuts

#define	SETUP_STATE_REGISTRY	40		// zápis do registry

#define	SETUP_STATE_GAME_EXPLORER 45  // registration in Game Explorer

#define SETUP_STATE_RUN_APPS 48    // running applications specified by commands Q:runApp:appName

#define	SETUP_STATE_SETUP_LOG	50		// vytvoøení setup.log

#define SETUP_STATE_REQUIRED_VERSION 75 // check instaled version

#define	SETUP_STATE_FINAL		100		// ukoncení instalace
#define	SETUP_STATE_ENDAPP		999		// konec aplikace

// uživatelské stavy >= 1000
#define	SETUP_STATE_USER0	1000


////////////////////////////////////
// CKeyOptions

class CKeyOptions
{
public:
	enum
	{
		skip,
		readFromDialog,
		readFromRegistry,
    readFromWeb,
    readFromOFPReg // CWA setup
	};

	int				action;
	unsigned char	m_publicKey[19];
	int				m_checkOffset;
	CString			m_checkWith;
	bool			m_cdKeyIsInRegistry;
  // new CD keys algorithm
  bool m_newKeys;
  int m_productID;
	unsigned char	m_cdKeyFromRegistry[KEY_BYTES];

public:
	CKeyOptions ()
		: action (skip), m_cdKeyIsInRegistry (false)
	{
	}

	bool initPublicKey (LPCTSTR begStr)
	{
		TCHAR *endPtr;
		for (int i = 0; i < 19; ++i)
		{
			m_publicKey[i] = (unsigned char) _tcstol(begStr, &endPtr, 0);
			if (endPtr == begStr)
			{
				action = skip;
				return true;
			}
			begStr = endPtr;
		}

		return false;
	}

	bool initCheckOffset (LPCTSTR begStr)
	{
		TCHAR *endPtr;
		m_checkOffset = _tcstol(begStr, &endPtr, 0);
		if (endPtr == begStr)
		{
			action = skip;
			return true;
		}

		return false;
	}

	void init (CString &options, bool newKeys)
	{
    m_newKeys = newKeys;
    m_productID = 0;

		switch (options[0])
		{
		case 'R':
		case 'r':
			{
				m_cdKeyIsInRegistry = false;
				action = readFromRegistry;

        // registry path
				int f00 = options.Find (':', 2);
				if (f00 == -1)
				{
					return;
				}
				CString regPath = options.Mid (2, f00 - 2);

        // registry name
				int f01 = options.Find (':', f00 + 1);
				if (f01 == -1)
				{
          if (newKeys) f01 = options.GetLength();
					else return;
				}
				CString regName = options.Mid (f00 + 1, f01 - (f00 + 1));

        // product id
        int id = options.Find (':', f01);
        if (id < 0) return;
#if UNICODE
        m_productID = _wtoi(options.Mid (id + 1));
#else
        m_productID = atoi(options.Mid (id + 1));
#endif

        // check if key is present in registry
				HKEY key;
				m_cdKeyIsInRegistry = RegOpenKey (HKEY_LOCAL_MACHINE, regPath, &key) == ERROR_SUCCESS;
				if (!m_cdKeyIsInRegistry)
				{
					return;
				}
				DWORD length = KEY_BYTES;
				m_cdKeyIsInRegistry = RegQueryValueEx (key, regName, 0, NULL, (BYTE*) (&m_cdKeyFromRegistry[0]), &length) == ERROR_SUCCESS;
				RegCloseKey (key);
				if (!m_cdKeyIsInRegistry)
				{
					return;
				}

        if (newKeys)
        {
          return; // enough parameters for new keys
        }

        // public key
				int f1 = options.Find (':', f01 + 1);
				if (f1 == -1)
				{
					m_cdKeyIsInRegistry = false;
					return;
				}
				if (initPublicKey (options.Mid (f01 + 1, f1 - (f01 + 1))))
				{
					m_cdKeyIsInRegistry = false;
					return;
				}

        // offset
				int f2 = options.Find (':', f1 + 1);
				if (f2 == -1)
				{
					m_cdKeyIsInRegistry = false;
					return;
				}
				if (initCheckOffset (options.Mid (f1 + 1, f2 - (f1 + 1))))
				{
					m_cdKeyIsInRegistry = false;
					return;
				}

        // fixed message
				m_checkWith = options.Mid (f2 + 1);
			}
			break;

    case 'C': case 'c':
    case 'W': case 'w':
    case 'D': case 'd':
			{
        if (options[0]=='C' || options[0]=='c')
        {
          action = readFromOFPReg;
          HKEY key;
          CString regPath = _T("SOFTWARE\\Codemasters\\Operation Flashpoint");
          m_cdKeyIsInRegistry = RegOpenKey (HKEY_LOCAL_MACHINE, regPath, &key) == ERROR_SUCCESS;
          if (m_cdKeyIsInRegistry)
          {
            DWORD length = KEY_BYTES;
            CString regName = _T("Key"); 
            m_cdKeyIsInRegistry = RegQueryValueEx (key, regName, 0, NULL, (BYTE*) (&m_cdKeyFromRegistry[0]), &length) == ERROR_SUCCESS;
            RegCloseKey (key);
          }
        }
        else if (options[0]=='W' || options[0]=='w')
          action = readFromWeb;
        else
          action = readFromDialog;
        if (newKeys)
        {
          // product id
          int id = options.Find (':', 1);
          if (id < 0) return;
          m_productID = _tstoi(options.Mid (id + 1));
          return; // enough parameters for new keys
        }

        int f1 = options.Find (':', 2);
				if (f1 == -1)
				{
					action = skip;
					return;
				}
				if (initPublicKey (options.Mid (2, f1 - 2)))
				{
					action = skip;
					return;
				}

				int f2 = options.Find (':', f1 + 1);
				if (f2 == -1)
				{
					action = skip;
					return;
				}
				if (initCheckOffset (options.Mid (f1 + 1, f2 - (f1 + 1))))
				{
					action = skip;
					return;
				}

				m_checkWith = options.Mid (f2 + 1);
			}
			break;

		default: // 'N', 'n'
			action = skip;
			break;
		}
	}
};

////////////////////////////////////
// CCdRom

class CCdRom
{
public:
	CString		_label;
	CString		_file;
	CString		_dir;

public:
	CCdRom (CString &label, CString &file, CString &dir)
		: _label (label), _file (file), _dir (dir)
	{
	}
};

////////////////////////////////////
// CCrcFile

class CCrcFile
{
public:
	CString _name;
	unsigned long _crc;

public:
	CCrcFile (CString &name, unsigned long crc)
		: _name (name), _crc (crc)
	{
	}
};

class SetupRegistryKey
{
public:
  HKEY root;
  CString path;
  CString valueName;
  DWORD valueType;
  CString valueData;

  SetupRegistryKey() {}
  bool Init(CString line);
  SetupRegistryKey(const SetupRegistryKey &key) 
  {
    root = key.root;
    path = key.path;
    valueName = key.valueName;
    valueType = key.valueType;
    valueData = key.valueData;
  }
  ClassIsMovable(SetupRegistryKey);
};

struct RequiredVersion
{
public:
  CString version;
  CString textMsg;
  CString textMsgBody;
  CString registryKey;
  CString registryVal;
  CString fileName;
  RequiredVersion() {}

  RequiredVersion(const RequiredVersion &ver)
  {
    version = ver.version;
    textMsg = ver.textMsg;
    textMsgBody = ver.textMsgBody;
    registryKey = ver.registryKey;
    registryVal = ver.registryVal;
    fileName = ver.fileName;
  }

  ClassIsMovable(RequiredVersion);
};

struct SaveContext
{
  QFBank *bank;
  RString folder;
  RString outfile;
};
//Save the specified file from bank
void SaveFile(const FileInfoO &fi, const FileBankType *files, void *context);
CString GetTempDirectory(CString appName);

// BankPosition is used when setupNN.bin files are appended to setup.exe
// It contains information about sizes of these setupNN.bin bank files.
struct BankFilePos {
  size_t start, size;
  BankFilePos() {};
  BankFilePos(int st, int sz) : start(st), size(sz) {}
  ClassIsMovable(BankFilePos);
};
struct BankPosition
{
  AutoArray<BankFilePos> filePos;
  bool initialized;
  size_t start, size;

  BankPosition() : initialized(false), start(0),size(0) {}
  void Init(RString fileName);
  void Get(int bankIx);
};

////////////////////////////////////
// CSetupFileItem

class CSetupFileItem : public CObject
{
public:
  CSetupFileItem(CStringArray *logFile)	
    : m_isDirectory (false), whereSaved(-1), savedFileSize(0), m_sIconPath(""), m_iIconIndex(-1)
  {
    if (logFile != NULL)
    {
      m_arLogFileName.Append (*logFile);
    }
  };

  // data
public:
  CString m_sSourceFileName;
  CString m_sDestinationFileName;
  CString m_sArguments; //for shortcuts there can be arguments of command line specified
  CString m_sIconPath; // for shortcuts if icon should be different than icon of the exe to which is linked to.
  int m_iIconIndex;

  CStringArray m_arLogFileName;
  CCdRom		*m_pCdRom;
  bool m_isDirectory;
  char whereSaved; // 0 == resource, 1 == setup01.bin atd. But it can be also in setup directory or temporary directory
  int savedFileSize;
  CString m_sCondition;
  CString m_sDifferentDest;
};

////////////////////////////////////
// CSetupObject

class CPosSetupApp;

class CStringWrp : public CString
{
public:
  CStringWrp() {}
  CStringWrp(LPCSTR str) : CString(str) {}
  CStringWrp(LPCTSTR str) : CString(str) {}
  CStringWrp(CString &str) : CString(str) {}
  ClassIsMovableZeroed(CStringWrp);
};

class ParsedLine : public AutoArray<CStringWrp>
{
public:
  CString ConcatenateRest(int from) const
  {
    CString retval;
    for (int i=from; i<Size(); i++)
    {
      if (i!=from) retval = retval + _T(":");
      retval = retval + Get(i);
    }
    return retval;
  }
  TCHAR GetChar(int ix, int offset=0, bool upper=true) const
  {
    if (ix<Size())
    {
      const CString &str = Get(ix);
      if (offset < str.GetLength())
        if (upper) return toupper(str[offset]);
        else return str[offset];
    }
    return 0;
  }
  CString GetString(int ix) const
  {
    if (ix<Size()) return Get(ix);
    return CString("");
  }
};

class CSetupObject : public CObject
{
public:
					CSetupObject();
					~CSetupObject();

      // inicializace dat
			BOOL	InitSetupParams();
			BOOL	LoadSetupDatFile();	
      /// LoadFile loads file from disk or resource or temporary directory storage
      RString LoadFile(CString fileName, BOOL reportErrors);

			BOOL	GetFileParamsSubstrings(LPCTSTR pLine,CString& sSource,CString& sDest) const;
			BOOL	BuildFileList();

					// modifikace hlavního okna
	static	void	DrawRainbow(HDC hDC,RECT* pRect,COLORREF cFrom,COLORREF cTo);
					// vykreslení pozadí okna (je možné mìnit dle stavu)
			void	DoDrawWindowBackground(HDC hDC,CRect rArea);

					// Setup Wizard
			BOOL	DoSetupWizard();

			BOOL	DoResultOK();

					// pomocná hlášení a operace
			BOOL	VerifyFileRead(LPCTSTR pFileName,BOOL bReportErrors);
			void	ReportFileNotFound(LPCTSTR pFileName,BOOL bSetInternalErr = TRUE);
			void	ReportFileErrorRead(LPCTSTR pFileName,BOOL bSetInternalErr = TRUE);

	static	LPCTSTR	MakeLongDirStr(CString& dest,LPCTSTR source);
	static	LPCTSTR	MakeShortDirStr(CString& dest,LPCTSTR source);
  static  CString ClearCompressionExt(CString filename);
	static	void	SetFilenameToControl(HWND hWnd,LPCTSTR pText);
			BOOL	MakePath(LPCTSTR logFile,LPCTSTR pPath,BOOL bForFile);

			void	AddInstallationSubdirectory(CString&);

      BOOL ReadRegistryKey(CString path, unsigned char (&cdkye)[KEY_BYTES]);

      BOOL  CheckCustomCDKey(int productID, unsigned char (&cdkey)[KEY_BYTES], unsigned char *publicKey, int checkOffset, const char *checkWith);
			BOOL	CheckCDKEY(int productID, CString &cdkey, unsigned char *publicKey, int checkOffset, const char *checkWith);
			BOOL	CheckCDKEYfromRegistry(int productID, unsigned char (&cdkey)[KEY_BYTES], unsigned char *publicKey, int checkOffset, const char *checkWith);

					// jednotlivé kroky instalace
			BOOL	DoSetupStep();				// TRUE = continue
			int		DoSetupStep(int nState);	
			int		DoUserSetupStep(int nState);	

					// proces kopírování a komunikace s procesem
			BOOL	IsRunningCopy();					// probíhá kopírování
			BOOL	IsLastCopyErr(CString&);			// chyba pøi kopírování ?
			void	CancelCopy();						// pøerušení kopírování
      void  CancelRunApp();		      // pøerušení runapp
      BOOL	DoRunCopy(LPCTSTR pDest,LPCTSTR pSrc, CSetupFileItem *pItem);// kopírování dvou souborù				
      void  DoRunCopyThread();

			DWORD CopyFilesFunc(); // background thread function

#if _ENABLE_PBO_PROTECTION
      /// protects PBO headers using encryption
      /// \param error contains error message if some error occured during protection
      bool ProtectPBOHeaders(RString fileName, CString& error);
      /// create serial numbers used for header encryption
      bool CreateHeaderProtectionSerials(PBOHeaderProtection::TByteBuffer& outSerial1, PBOHeaderProtection::TByteBuffer& outSerial2);
#endif //#if _ENABLE_PBO_PROTECTION
      // check if file exist
      bool TestCondition(CString m_sCondition,CString sDirectory = CString()); 

			void EnterLock();
			void LeaveLock();
			
					// Direct X
			void	DoSetupDirectX();

          // OpenAL
      void	DoSetupOpenAL();

          // Windows Firewall
      void	DoSetupFirewall();

          // shortcuts
			void  DoCreateShortcuts();

          // desktop shortcuts
      void  CreateDesktopShortcuts();
          // SendTo shortcuts

      void  CreateSendToShortcuts();
          // run applications specified by Q:runApp:appName commands
      int   DoRunAplications();
      unsigned int  DoRunAplicationsInThread();
      int   DoRunSingleApplication(CString directory, CString filename);
					
					//registry
			void	DoCreateRegistry();
  
          //prepare data from resources (if any)
      void PrepareResourceData(const char *resourceName);
      void PrepareResourceData();
      void PrepareResourceDirectX();

          //helper function to use FileList in registry
      void LoadSetupFileList();
          //geting number of setupNN.bin file where saved or 0 for resources or -1 for install directory
      int GetWhereSaved(LPCTSTR filename);
          //return whether setupNN.bin files are used in this setup
      bool IsSetupBinUsed();

      CString GetSystemPath(CString name, CString path);
          // Game Explorer registration
      void DoRegisterInGameExplorer();

					// vytvoøení setup.log
			void	DoCreateSetupLog();
      void  DoCreateCheckFileLog();
			void	AddSetupLogLine(LPCTSTR logFile, char nID,LPCTSTR pLine);
      void  AddCheckFileListItem(LPCTSTR file, __int64 fileSize);

					// Zmìna "%0" za jméno aplikace v daném CWnd a jeho potomcích
			void	ApplyApplicationNameAndStringTable (CWnd *item);
          // parsovani m_SetupDatLines[ix] na m_parsedSetupDatLines
      void  ParseSetupDatLine(int ix);
			int		SetupMessageBox (CString &text, UINT nType = MB_OK, UINT nIDHelp = 0, HWND hwnd = NULL);
			int		SetupMessageBox (LPCTSTR lpszText, UINT nType = MB_OK, UINT nIDHelp = 0, HWND hwnd = NULL);
			int		SetupMessageBox (UINT nIDPrompt, UINT nType = MB_OK, UINT nIDHelp = (UINT) -1, HWND hwnd = NULL);
			void	SetupFormatString1 (CString &rString, LPCTSTR fString, LPCTSTR lpsz1);
			void	SetupFormatString1 (CString &rString, CString &fString, LPCTSTR lpsz1);
			void	SetupFormatString2 (CString &rString, LPCTSTR fString, LPCTSTR lpsz1, LPCTSTR lpsz2);
			void	SetupFormatString2 (CString &rString, CString &fString, LPCTSTR lpsz1, LPCTSTR lpsz2);
	static	CString	SetupLocalizeString (LPCTSTR str);
	static	int		SetupFindOneOf (CString &where, LPCTSTR charSet, int start);
	static	bool	ApplyApplicationNameAndStringTable (CString &text, bool applyApplName);
	static	BOOL	IsCdRomPresent (CCdRom *cd, CString &drive);
  bool FindCRC(const LPCTSTR name, unsigned int &crc);

protected:
			void AddDirectory(CStringArray *logFile, CString srcPath, CString src, CString dst, CCdRom *curCdRom, int &position,CString sCondition=CString(),CString sDiffDest=CString());
			
			void UndoSetup(); // remove installed files if setup aborts

      BOOL IsSerialInBlackList(unsigned char sn[KEY_BYTES]) const;

// data
public:
	int			m_nCurrState;	// stav instalace = SETUP_STATE_???

	enum {
      stpDirectXError = -2,
      stpOKRestart = -1,
			stpOK = 0,
			stpInternalErr = 1,	// vnitøní chyba aplikace
			stpUserCancel  = 2,	// zrušeno uživatelem
			stpSetupError  = 3, // chyba pri instalaci
      stpSetupErrorIgnored  = 4, // chyba pri instalaci - setup ale pokraèuje
	} m_nSetupResult;	// výsledek instalace

	unsigned char m_cdkey[KEY_BYTES];

	// zdrojový adresáø (Setup.exe)
	CString			m_sSetupSourceDir;
	// cílový adresáø 
	CString			m_sSetupPoseidonDir;
	// parametrický soubor pro Setup (Setup.dat)
	CString			m_sSetupDatFile;
	CStringArray	m_SetupDatLines;
  AutoArray<ParsedLine> m_parsedSetupDatLines;
  
  // array of informations for uninstall LocalSettings and MyDocuments, need to be stored and apply in SETUP_STATE_SETUP_LOG
  CStringArray m_updateUnistallAfterInstallEnd;
	// seznam souborù pro kopírování
	CObArray		m_arFileList;
  // array of shortcuts in Start menu
	CObArray		m_arShortcuts;
  // array of shortcuts on desktop
  CObArray		m_arShortcutsDesktop;
  // array of shortcuts on Send To
  CObArray		m_arShortcutsSendTo;
	// list of pictures
	CObArray		m_arPictures;
	// délka souborù ke kopírování
	__int64			m_TotalCopySize;	
	// název cílového adresáøe
	CString			m_sInstallationSubdirectory;
	// start menu group directory
	CString			m_sStartMenuGroupDir;
  // name of the check file
  CString     m_sCheckFileName;
  // name of the file that should be started after update
  CString     m_sRunAfterUpdate;
	// uninstall display names and uninstall strings
  CStringArray			m_sRegistryRoot;
  CString           m_sRegistryRootDefault;
  // name of the file that has to be in target directory 
  CString m_sTargetFileCheck;
  // if true, target file is requred to proceed installation
  bool m_bTargetFileRequired;

#if _ENABLE_PBO_PROTECTION
  // registry path root for parent application
  CString			m_sParentAppRegistryRoot;
  // projectId of the parent application
  int         m_iParentProjectId;
  // parent testing serial number for header protection
  CString			m_sProtTestParentSerial;
  // testing serial number for header protection
  CString			m_sProtTestSerial;
#endif

  // contains target file definitions which are requred to proceed installation
  AutoArray<RequiredVersion> m_requiredVersions;

	// název aplikace
	static CString	m_sApplicationName;
	CStringArray	m_arApplLogFileName;
	// instance aplikace
	CPosSetupApp	*m_pPosSetupApp;
	// seznam akci na zaver
	CPtrArray		m_arActions;
	// 
	int				m_arActionChecks[10];
	// current uninstaller log file
	CStringArray	m_arCurrentLogFile;
	// CD key options
	CStringArray	m_delFilesAfterInstallEnd,m_delDirsAfterInstallEnd;
	// CD key options
	CKeyOptions		m_cdKeyOptions;
	// CD ROMs
	CArray<CCdRom*, CCdRom*>	m_cds;
	// current CD-ROM
	CCdRom						*m_currentCdRom;
	// current drive with CD-ROM
	CString						m_currentDrive;
	// CRC files
	CArray<CCrcFile*, CCrcFile*>	m_arCrcFiles;
	// language extension string for command %L
	static CString	m_sFileLanguageExtension;
  // the way to install DirectX
  enum {DRX_DIALOG, DRX_SILENT, DRX_NO} m_processDirectX;
  // Game Explorer resource location
  CString m_geResource;
  // Game Explorer play tasks
  CStringArray m_gePlayTasks;
  // Game Explorer support tasks
  CStringArray m_geSupportTasks;
  // File type associations
  CStringArray m_fileTypes;
  /// Extended file type associations
  CStringArray m_fileTypesEx;
  // Registry keys
  AutoArray<SetupRegistryKey> m_registryKeys;
  /// List of applications ta add to the firewall exclude list
  CStringArray _firewall;
  // True iff Temporary folder was created
  bool temporaryFolderActive;
  // True when label of cdRom drive should not be tested (used for installation from multiple CD from HD as test)
  static bool m_testVolumeLabel;
  // True when openAL should be installed
  bool m_installOpenAL;
  // True when license dialog should be shown
  bool m_processLicense;
  // True when info dialog should be shown
  bool m_processInfo;
  // disable or enable ALL Users checkbox 
  bool m_allUsers;
  // creates text file with current version number
  bool m_saveVersionFile;
  // disable or enable first dialog "this program will install blabla..." 
  bool m_showWelcome;
  // when true then setup will leave msg "this aplication is alredy instaled..."
  // when silent and application is already installed skip wizard
  enum {FI_NO,FI_YES,FI_SILENT} m_forceInstall;
  // hide, show last dialog "applycation was succesfully installed"
  bool m_showResultOK;
  // BankPosition is used, when setupNN.bin banks are appended to setup.exe
  BankPosition m_bankPos;
  // When setup.exe is run with another setup.exe as parameter - then everything is taken from this executable
  CString m_patchingSetup;
  bool m_debugMode;

  // when enabled, setup acts as updater and launches the OA in the end
  bool m_updaterMode;
  // when setup to true, updater will start defined application when ended
  bool m_runAfterUpdateFlag;

  // debug option to show message box with information about mode in which the setup is running
  bool m_showSetupMode;

  // no files to copy
  bool m_doNotAskToDestFolder;
  // When licence agreement is not checked agreed, this condition disables next button in propertySheet
  bool m_licenceButtonDisabled;
  int  m_licencePageIndex;

  bool m_CDKeyButtonDisabled;
  int  m_CDKeyPageIndex;
  // Applications to be run at the end of installation (command Q:runApp:appExeFileName)
  CObArray m_runAppFiles;

	// aktuální prùbìh kopírování - proces kopírování
	CRITICAL_SECTION m_CSection_CopyFiles; 

  // aktuální prùbìh kopírování - proces kopírování
	CRITICAL_SECTION m_CSection_RUNAPP; 
	// TODO: use events instead of bool

	HANDLE _copyThread;
	HANDLE _RunAppThread;

	BOOL			m_bRunningCopy;		// pøíznak kopírování
	BOOL			m_bCancelCopy;		// pokyn k ukoncení procesu
  bool      m_bRunningRUNAPP;
	__int64			m_nCurrCopySize;	// pocet zkopírovanýcn bytù
	CString			m_sCopyDestination;	// kopírované soubory
	CString			m_sCopySource;
	unsigned int m_nCopyCRC;
	bool m_bTestCRC;
  char        m_cWhereSaved;
  int         m_nCopySavedSize;
  CString     m_sCopySourceRaw;
  /// install for all users
  BOOL m_bAllUsers;
  AutoArray<ResFileItem> m_SetupFileList;
  bool m_SetupFileListLoaded;

	CString			m_sCopyError;		// chyba pøi kopírování
  bool        m_canIgnoreError;

	// Direct X
	DWORD			m_dwVersion;
	DWORD			m_dwRevision;
	BOOL			m_bSetupDirectX;


  // list of all files and their's sizes
  CStringArray m_arCheckFiles;

	// SETUP-LOG
	CPtrArray		m_arSetupLogFiles;
};

////////////////////////////////////
// CSetupLog

class CSetupLogFileItem : public CObject
{
public:
	CSetupLogFileItem(CStringArray &curLogFiles, LPCTSTR logFile, LPCTSTR uninst, LPCTSTR appl, LPCTSTR disp, bool addDir)	
		: m_sLogFileName (logFile), m_sUninstaller (uninst), m_sApplication (appl), m_sMessage (disp), m_addDirectoryToUninstaller(addDir)
	{
		for (int i = 0; i < curLogFiles.GetSize (); ++i)
		{
			m_arCurrentLogFileName.Add (curLogFiles[i]);
		}
		m_arCurrentLogFileName.Add (logFile);
	};

// data
public:
	CStringArray	m_arCurrentLogFileName;
	CString			m_sLogFileName;
  CStringArray	m_arSetupLog;
  bool        m_addDirectoryToUninstaller;
  CString			m_sUninstaller;
	CString			m_sApplication;
	CString			m_sMessage;
};

////////////////////////////////////
// CSetupPictureItem

class CPictureItem : public CObject
{
public:
	CPictureItem() {};

// data
public:
	CString m_sPictureName;
	HBITMAP m_hPicture;
	double m_len;
	__int64 m_nFrom;
};

#endif // _SETUP_OBJECT_INCLUDE

void *FindAndLoadResource(int &size, const void *ctx, const char *source);
RString CString2RString(const CString &str, int codePage = CP_ACP);
RString LPCTSTR2RString(LPCTSTR str, int codePage=CP_ACP);
void SetupSetWindowText(CWnd *wnd, CString text);
BOOL SetupSetWindowText(HWND hWnd, CString text);

/////////////////////////////////////////////////////////////////////////////
