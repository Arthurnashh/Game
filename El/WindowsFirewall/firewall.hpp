#ifdef _MSC_VER
#pragma once
#endif

#ifndef _FIREWALL_HPP
#define _FIREWALL_HPP

bool WindowsFirewallAddApplication(const wchar_t *filename, const wchar_t *appname);
bool WindowsFirewallRemoveApplication(const wchar_t *filename);

#endif
