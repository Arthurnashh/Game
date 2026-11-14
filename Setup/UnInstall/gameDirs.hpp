#ifndef _UN_GAME_DIRS_HPP
#define _UN_GAME_DIRS_HPP

#include <Es/Strings/rString.hpp>
#include <Es/Containers/array.hpp>

RString GetLoginName();
bool GetLocalSettingsDir(char *dir);
AutoArray<RString> GetDefaultUserRootDirs();

#endif
