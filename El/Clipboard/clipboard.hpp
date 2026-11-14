#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CLIPBOARD_HPP
#define _CLIPBOARD_HPP

#include "Es/Strings/rString.hpp"

void ExportToClipboard(const char *data, int size);
void ExportToClipboardAndFile(const char *data, int size, const char *filename);

RString ImportFromClipboard();

void ExportUnicodeToClipboard(const wchar_t *data, int size);
RStringCT<wchar_t> ImportUnicodeFromClipboard();

#endif
