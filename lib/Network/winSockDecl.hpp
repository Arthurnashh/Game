#ifdef _MSC_VER
#pragma once
#endif

/**
@file
Avoid having to use WinSock2 headers only because of sockaddr_in declaration - this would pull windows.h as well
*/

#if !defined _WIN_SOCK_DECL_HPP && !defined _WIN_SOCK_IMPL_HPP

#define _WIN_SOCK_DECL_HPP

#if defined _XBOX
  // forward declaration not possible on Xbox - missing #ifndef s_addr test in Xbox headers
# include <Es/Common/win.h>
# if _XBOX_VER>=2
#   include <winsockx.h>
# endif

#elif _GAMES_FOR_WINDOWS

  // similar to Xbox - avoid forward declaration
# include <Es/Common/win.h>

#elif defined _WIN32

#ifndef WINSOCK_VERSION

#ifndef s_addr
/*
 * Internet address (old style... should be updated)
 */
typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned long   u_long;

struct in_addr {
        union {
                struct { u_char s_b1,s_b2,s_b3,s_b4; } S_un_b;
                struct { u_short s_w1,s_w2; } S_un_w;
                u_long S_addr;
        } S_un;
#define s_addr  S_un.S_addr
                                /* can be used for most tcp & ip code */
#define s_host  S_un.S_un_b.s_b2
                                /* host on imp */
#define s_net   S_un.S_un_b.s_b1
                                /* network */
#define s_imp   S_un.S_un_w.s_w2
                                /* imp */
#define s_impno S_un.S_un_b.s_b4
                                /* imp # */
#define s_lh    S_un.S_un_b.s_b3
                                /* logical host */
};
#endif

struct sockaddr_in {
        short   sin_family;
        u_short sin_port;
        struct  in_addr sin_addr;
        char    sin_zero[8];
};
#endif

#else
// Linux sockets
#  include <sys/socket.h>
#  include <netinet/in.h>
#endif

#endif


