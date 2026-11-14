#ifdef _MSC_VER
#pragma once
#endif

#ifndef _GAME_DIRS_HPP
#define _GAME_DIRS_HPP

#include <Es/Strings/rString.hpp>

//! encode player name into directory name
RString EncodeFileName(RString src);
//! decode player name from directory name
RString DecodeFileName(RString src);

/// conversion Unicode -> UTF-8
RString UnicodeToUTF(RWString text);
/// conversion UTF-8 - > Unicode
RWString UTFToUnicode(RString text);


#if defined(_WIN32) 
RString GetLoginName();

typedef bool ForEachUserCallback(RString userName, void *context);
bool ForEachUser(ForEachUserCallback func, void *context);

bool IsValidPlayerName(RString name);
bool IsEmptyPlayerName(RString name);

#endif

void SetExplicitRootDir(RString path);
RString GetUserRootDir(RString name);
RString GetDefaultUserRootDir();
bool GetLocalSettingsDir(char *dir);

#endif
