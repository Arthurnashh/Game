#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SHAPE_MODEL_CONFIG_HPP
#define _SHAPE_MODEL_CONFIG_HPP

#include <El/ParamFile/paramFile.hpp>
/*! \file 
Header file for model config operations
*/

/// describe model config location
class ModelConfig
{
  /// file with the config (may be unused when Pars>>"CfgModels" is used
  ParamFile _file;
  /// CfgModels class - may point to _file or to Pars>>"CfgModels"
  ConstParamEntryPtr _cfgModels;
  /// CfgSkeletons class - may point to _file or to Pars>>"CfgSkeletons"
  ConstParamEntryPtr _cfgSkeletons;
  /// class describing this particular model
  ConstParamEntryPtr _model;
  
  
  void ParseLevelUp(ParamFile &merged, const char *path, IEvaluatorNamespace *globalVariables);
  /// parse all configs in the given path
  void Parse(const char *path, IEvaluatorNamespace *globalVariables);
  
  public:
  ModelConfig();
  ModelConfig(const char *path, IEvaluatorNamespace *globalVariables = NULL);
  /// find config class
  void FindClass(const char *path, IEvaluatorNamespace *globalVariables = NULL);

  template <class Type>
  ParamEntryVal operator >> (Type *name) const {return (*_model)>>name;}
  
  ParamEntryVal GetCfgSkeletons() const {return *_cfgSkeletons;}
  ParamEntryVal GetCfgModels() const {return *_cfgModels;}
  ParamEntryVal GetModel() const {return *_model;}
  
  bool IsDefined() const {return _cfgModels.NotNull();}
  ConstParamEntryPtr operator ->() const {return _model;}
  
  // diagnostics
  const RStringB &GetName() const {return _file.GetName();}
};

#endif
