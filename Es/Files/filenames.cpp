#include <Es/essencepch.hpp>
#include <Es/Common/fltopts.hpp>
#include "filenames.hpp"

/*!
  In the mask string you can use wilcards like '?' for a single char and '*'
  for an unknown number of chars.
  Matching is case sensitive.
  \param s String to be matched with the mask
  \param m Mask to be matched with the string
  \return true in case the string and the mask matches, false elsewhere
*/

__forceinline bool MatchChar(char s, char m, bool caseSens)
{
  return caseSens ? s==m : myLower(s)==myLower(m);
}

bool Matches(const char *s, const char *m, bool caseSens)
{
  if (s[0] == 0)
  {
    if (m[0] == 0)
    {
      return true;
    }
    else if (m[0] == '*')
    {
      return Matches(s, m+1);
    }
    else
    {
      return false;
    }
  }
  else
  {
    if (m[0] == 0)
    {
      return false;
    }
    else if ((m[0] == '?') || MatchChar(s[0],m[0],caseSens))
    {
      return Matches(s+1, m+1);
    }
    else if (m[0] == '*')
    {
      int slength = strlen(s);
      // Cycle through all lengths including empty string
      for (int i = 0; i < slength + 1; i++)
      {
        if (Matches(s+i, m+1))
        {
          return true;
        }
      }
      return false;
    }
    else
    {
      return false;
    }
  }
}

bool IsPathAbsolute(const char *name)
{
  const char *searchDColon = name;
  while (*searchDColon!=0)
  {
    // if double colon is found first, it is an absolute path
    if (*searchDColon==':') return true;
    // if backslash if found first, it is a relative path
    if (*searchDColon=='\\') break;
    searchDColon++;
  }
  return false;
}

void NormalizePath(char* path)
{
  if (!path)
    return;

  size_t length = strlen(path);
  if (0 == length)
    return;

  char* dst = path;
  bool lastWasSeparator = false;
  for (size_t i=0; i != length; ++i)
  {
    if (path[i] == '/' || path[i] == '\\')
    {
      if (lastWasSeparator)
        continue; //duplicate separator

      *dst++ = '\\';
      lastWasSeparator = true;
    }
    else
    {
      lastWasSeparator = false;
      *dst++ = tolower(path[i]);
    }

  }

  // check for last separator and remove if needed
  if (dst > path && dst[-1] == '\\')
    --dst;

  // finish the string
  *dst = 0;
}
