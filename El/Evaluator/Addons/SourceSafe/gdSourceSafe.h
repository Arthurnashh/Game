#pragma once

#pragma once
#include <el\evaluator\express.hpp>

class SccFunctions;

class GdSourceSafe
{
public:

  /// Hostle SS interface. Script can use application settings for SS
  /** Set valid interface, or set NULL, if no interface available */
  static SccFunctions *GScc;
  /// True, if GScc is active for scripts
  /** When this variable is false, script was not used source control. 
  The variable is changed to true on first SS command. With this command,
  Script SS settings dialog is displayed, to enable control/debug processing
  SS commands. 

  Application should reset the variable before it starts script. If script is
  started for each item in list, the application can reset the variable only before
  first item is processed.
  */
  static bool GSccActive;
  static RString lastComment;
  static void RegisterToGameState(GameState *gState);
  static RString GetLastComment() {RString res=lastComment; lastComment=RString();return res;}

};
