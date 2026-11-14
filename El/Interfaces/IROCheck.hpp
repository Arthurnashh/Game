#ifndef __READONLYCHECK_INTERFACE_HEADER_
#define __READONLYCHECK_INTERFACE_HEADER_

#include <tchar.h>

enum ROCheckResult {ROCHK_FileOK,ROCHK_FileRO,ROCHK_FileSaveAs,ROCHK_GetLatestVersion,ROCHK_FileOKUpdated, ROCHK_GetLatestVersionUpdated};
enum ROCheckFlags  {ROCHF_DisableSaveAs=1,ROCHF_DisableExploreButton=2,ROCHF_GetLatestVersion=4,ROCHF_TrackFileUpdate=8};


class SccFunctions;

class IROCheck
{
protected:
  SccFunctions *_scc;
public:
  IROCheck():_scc(NULL) {}

  ///Tests file for R/O flags and if it, opens dialog to ask user next action
  /** 
  @param szFilename file name tested
  @param flags additional flags "ROCheckFlags"
  @return ROCHK_FileOK - file is R/W. Function can return this state, if file was R/O and
  user set file's state to R/W. ROCHK_FileRO - file is R/O and user cancels the dialog.
  ROCHK_FileSaveAs - file is R/O and user wish to open Save As dialog.
  */
  virtual ROCheckResult TestFileRO(const _TCHAR *szFilename, int flags=0)=0;

  ///Function sets current SCC project. 
  /** If project is set and file is controlled, Check Out button is enabled.
  Ensure pointer is valid before you call TestFileRO*/

  void SetScc(SccFunctions *scc) {_scc=scc;}
  ///Retrieves current SCC project
  SccFunctions *GetScc() {return _scc;}
};
#endif