/**
 * This file is part of Agnostic Dragon.
 *
 * Agnostic Dragon is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Agnostic Dragon is distributed in the hope that it will be
 * useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Agnostic Dragon.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * MODIFIED in 2026 by EveryNightLive (unofficial 32:9 update):
 *   32:9 support, compatibility with the Dec/2017 DDDA.exe and recent
 *   Special K versions, full map / menu / HUD fixes, build fixes.
 *   Changes are marked with "FIX" comments. See README.md.
**/
#ifndef __AD__HOOK_H__
#define __AD__HOOK_H__

// FIX: lib removed - built with /GL in VS2015, does not link with current VS.
//   No MH_* function is called (hooks come from the d3d9.dll injector).
#include "MinHook/include/MinHook.h"

void
AD_DrawCommandConsole (void);

MH_STATUS
WINAPI
AD_CreateFuncHook ( LPCWSTR pwszFuncName,
                    LPVOID  pTarget,
                    LPVOID  pDetour,
                    LPVOID *ppOriginal );

MH_STATUS
WINAPI
AD_CreateDLLHook ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                   LPVOID  pDetour,    LPVOID *ppOriginal,
                   LPVOID* ppFuncAddr = nullptr );

MH_STATUS
WINAPI
AD_EnableHook (LPVOID pTarget);

MH_STATUS
WINAPI
AD_DisableHook (LPVOID pTarget);

MH_STATUS
WINAPI
AD_RemoveHook (LPVOID pTarget);

MH_STATUS
WINAPI
AD_Init_MinHook (void);

MH_STATUS
WINAPI
AD_UnInit_MinHook (void);

#endif __AD__HOOK_H__