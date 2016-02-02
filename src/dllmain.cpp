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
**/

#include <Windows.h>

#include "config.h"
#include "log.h"

#include "render.h"
#include "gamestate.h"

#include "command.h"
#include "hook.h"
#include "input.h"

#pragma comment (lib, "kernel32.lib")

HMODULE hDLLMod      = { 0 }; // Handle to SELF
HMODULE hInjectorDLL = { 0 }; // Handle to Special K

typedef void (__stdcall *SK_SetPluginName_pfn)(std::wstring name);
SK_SetPluginName_pfn SK_SetPluginName = nullptr;

DWORD
WINAPI
DllThread (LPVOID user)
{
  std::wstring plugin_name = L"Agnostic Dragon v " + AD_VER_STR;

  hInjectorDLL =
    GetModuleHandle (config.system.injector.c_str ());

  SK_SetPluginName = 
    (SK_SetPluginName_pfn)GetProcAddress (hInjectorDLL, "SK_SetPluginName");
  SK_GetCommandProcessor =
    (SK_GetCommandProcessor_pfn)GetProcAddress (hInjectorDLL, "SK_GetCommandProcessor");

  //
  // If this is NULL, the injector system isn't working right!!!
  //
  if (SK_SetPluginName != nullptr)
    SK_SetPluginName (plugin_name);


  // Game State
  ad_gamestate_s::init ();


  // Plugin State
  if (AD_Init_MinHook () == MH_OK) {
    ad::InputManager::Init ();
    ad::RenderFix::Init    ();
  }

  return 0;
}

BOOL
APIENTRY
DllMain (HMODULE hModule,
         DWORD    ul_reason_for_call,
         LPVOID  /* lpReserved */)
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
  {
    hDLLMod = hModule;

    dll_log.init ("logs/AgDrag.log", "w");
    dll_log.Log  (L"AgDrag.log created");

    if (! AD_LoadConfig ()) {
      config.render.aspect_correction = true;
      config.render.center_ui         = true;
      config.render.fix_minimap       = true;

      config.nametags.always_on_top   = true;
      config.nametags.aspect_correct  = false;

      // Save a new config if none exists
      AD_SaveConfig ();
    }

    HANDLE hThread = CreateThread (NULL, NULL, DllThread, 0, 0, NULL);

    // Initialization delay
    if (hThread != 0)
      WaitForSingleObject (hThread, 250UL);
  } break;

  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
    break;

  case DLL_PROCESS_DETACH:
    ad::RenderFix::Shutdown    ();
    ad::InputManager::Shutdown ();

    AD_UnInit_MinHook ();
    AD_SaveConfig     ();

    dll_log.LogEx      (true,  L"Closing log file... ");
    dll_log.close      ();
    break;
  }

  return TRUE;
}