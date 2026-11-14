// Functions declaration



/* WinMain */
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int);

/* Do Uninstall User files */
void DoUninstallFinal();

/* Get help string for msg dialog to list directories to be removed recursively */
RString GetUserDirectoriesString();

/* DoCopyFile */
BOOL DoCopyFile(LPTSTR FileSrc,LPTSTR FileDst,BOOL bShowErr);

/* DoDeleteFile */
BOOL DoDeleteFile(LPCTSTR pFile);

/* DoDeleteDir */
void DoDeleteDir(LPCTSTR pDir);

/* DoUnInstallCmd */
void DoUnInstallCmd(LPCTSTR pCmd);

/* PosLoadString */
void PosLoadString (const char *uID, LPTSTR lpBuffer, int nBufferMax);

RString LPCTSTR2RString(LPCTSTR str, int codePage=CP_ACP);

void UninstallProtect();

