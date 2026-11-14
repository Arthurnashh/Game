#include "../wpch.hpp"
#include "modelConfig.hpp"
#include "../paramFileExt.hpp"

void ModelConfig::ParseLevelUp(ParamFile &config, const char *path, IEvaluatorNamespace *globalVariables)
{
  // check config for given path
  // try to derive from the file name by appending .cfg extension
  char filename[1024];
  char shortname[512];
  strcpy(filename,path);
  
  ParamFile localConfig;
  // check if the name is short only
  char *name = strrchr(filename,'\\');
  if (!name)
  {
    // if there is nowhere to proceed to, load and return
    config.Parse("model.cfg", NULL, NULL, globalVariables);
// fprintf(stderr, "<modelCfg path=\"%s\" count=%d>\n", "model.cfg", config.GetEntryCount());
    return;
  }
  // path+name - auto generate filename
  *name = 0;
  GetFilename(shortname,filename);
  strcpy(name,"\\");
  strcat(filename,shortname);
  strcat(filename,".cfg");
  localConfig.Parse(filename, NULL, NULL, globalVariables);
// fprintf(stderr, "<modelCfg path=\"%s\" count=%d>\n", cc_cast(filename), localConfig.GetEntryCount());
  // remove appended name
  // if not found, try model.cfg instead of filename
  if (localConfig.GetEntryCount()==0)
  {
    strcpy(name,"\\model.cfg");
    localConfig.Parse(filename, NULL, NULL, globalVariables);
// fprintf(stderr, "<modelCfg path=\"%s\" count=%d>\n", cc_cast(filename), localConfig.GetEntryCount());
  }
  *name = 0;

  // current level processed
  // if config claims it is a root, or there is no possible parent,
  // do not process any further  
  if (!localConfig.FindEntry("root"))
  {
    char *levelUp = strrchr(filename,'\\');
    if (levelUp)
    {
      levelUp[1] = 0;
      ParseLevelUp(config, filename, globalVariables);
    }
    else
    {
      ParseLevelUp(config, "", globalVariables);
    }
  }
  // merge config from the path with configs from higher levels
  config.Update(localConfig);
  
}

void ModelConfig::Parse(const char *path, IEvaluatorNamespace *globalVariables)
{
  // TODO: change locality of variables defined by enum and EXEC / EVAL
  // TODO: evaluator should load / save global variables, not local ones

  // parse folders recursively
  _file.CreateEvaluator(NULL, globalVariables, true);
  // check config corresponding to the model name
  char filename[1024];
  strcpy(filename,path);
  char *name = GetFilenameExt(filename);
  strcpy(GetFileExt(name),".cfg");
  
  ParamFile localConfig;
  // TODO: if folder name and model name are the same, do not load one of them
  localConfig.Parse(filename, NULL, NULL, globalVariables);
// fprintf(stderr, "<modelCfg path=\"%s\" count=%d>\n", cc_cast(filename), localConfig.GetEntryCount());

  // if model specific config is not found, use only folder based config

  if (name>filename && name[-1]=='\\')
  {
    *name = 0;
    ParseLevelUp(_file, filename, globalVariables);
  }  
  _file.Update(localConfig);
  
  _file.SetFile(&_file);
}

ModelConfig::ModelConfig()
{
}

ModelConfig::ModelConfig(const char *path, IEvaluatorNamespace *globalVariables)
{
  FindClass(path, globalVariables);
}

void ModelConfig::FindClass(const char *path, IEvaluatorNamespace *globalVariables)
{
  // if there is no name, how can we know what to parse?
  if (*path==0) return;
  // first try to parse specific configs
  Parse(path, globalVariables);
  
  // note: shape name may contain spaces
  char shortName[256];
  GetFilename(shortName,path);
  while (strpbrk(shortName," -/()"))
  {
    *strpbrk(shortName," -/()")='_';
  }
  // Get the model class name
  RStringB modelName = shortName;

  if (_file.FindEntry("CfgModels"))
  {
    // valid model config is parsed - check it
    ParamEntryVal fileNotes = _file>>"CfgModels";
    // instead of using default from model.cfg prefer CfgModels specific
    _model = fileNotes.FindEntry(modelName);
    if (_model)
    {
      _cfgModels = &fileNotes;
    }
    
  }

  _cfgSkeletons = _file.FindEntry("CfgSkeletons");

  if (!_cfgModels && Pars.FindEntry("CfgModels"))
  {
    // Get the CfgModels entry
    
    ParamEntryVal notes = Pars>>"CfgModels";
    if (!notes.FindEntry(modelName))
    {
      if (_file.FindEntry("CfgModels"))
      {
        ParamEntryVal fileNotes = _file>>"CfgModels";
        ParamEntryVal modelEntry = fileNotes>>"default";
        _cfgModels = &fileNotes;
        _model = &modelEntry;
      }
      else
      {
        ParamEntryVal modelEntry = notes>>"default";
        _cfgModels = &notes;
        _model = &modelEntry;
      }
    }
    else
    {
      ParamEntryVal modelEntry = notes>>modelName;
      _cfgModels = &notes;
      _model = &modelEntry;
    }
  }
}
