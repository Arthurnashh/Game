#ifdef _MSC_VER
#pragma once
#endif

#ifndef _WIN_SOCK_IMPL_HPP
#define _WIN_SOCK_IMPL_HPP

#ifdef _WIN_SOCK_DECL_HPP
#pragma message("winSockImpl.hpp needs to be included before winSockDecl.hpp")
#endif

#ifdef _WIN32
#  include <Es/Common/win.h>
#  if _GAMES_FOR_WINDOWS // using special Winsock library
#  else
#    include <winsock2.h>
#  endif
#endif

#endif
