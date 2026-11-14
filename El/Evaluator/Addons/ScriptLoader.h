#ifndef _O2SCRIPTCLASS_HEADER_6760654981AC55A_BREDY_
#define _O2SCRIPTCLASS_HEADER_6760654981AC55A_BREDY_
#include <el\preprocc\preproc.h>
#include <el\pathname\pathname.h>




class ScriptLoaderStream: public QIFStream
{
public:
  Pathname restoreScriptName;
};


class ScriptLoaderClass: public GameVarSpace, private Preproc
{
  QOStrStream script;


public:
  Pathname curScriptName;
  AutoArray<RString> searchPaths;


  virtual QIStream *OnEnterInclude(const char *filename);
  virtual void OnExitInclude(QIStream *stream);
  ScriptLoaderStream *FindLibrary(const char *filename);

  static void PrepareGameState(GameState &gState);
  bool LoadScript(const char *filename);
  void LoadScriptDirect(const char *script);

  RString GetPreprocError();
  PreprocessError GetPreprocErrorCode() {return error;}
  void SetLibraryPaths(const char *libs); //paths is separated by ;

  GameValue RunScript(GameState &gState);

  const char *GetScript() const {return script.str();}  


};



#endif