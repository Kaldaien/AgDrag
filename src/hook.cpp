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
#include <string>

#include "hook.h"
#include "log.h"
#include "command.h"

#include "render.h"

#include <d3d9.h>

#include "config.h"

#include <mmsystem.h>
#pragma comment (lib, "winmm.lib")

#include <comdef.h>

struct window_t {
  DWORD proc_id;
  HWND  root;
};

BOOL
CALLBACK
AD_EnumWindows (HWND hWnd, LPARAM lParam)
{
  window_t& win = *(window_t*)lParam;

  DWORD proc_id = 0;

  GetWindowThreadProcessId (hWnd, &proc_id);

  if (win.proc_id != proc_id) {
    if (GetWindow (hWnd, GW_OWNER) != (HWND)nullptr ||
        GetWindowTextLength (hWnd) < 30             ||
     (! IsWindowVisible     (hWnd)))
      return TRUE;
  }

  win.root = hWnd;
  return FALSE;
}

HWND
AD_FindRootWindow (DWORD proc_id)
{
  window_t win;

  win.proc_id  = proc_id;
  win.root     = 0;

  EnumWindows (AD_EnumWindows, (LPARAM)&win);

  return win.root;
}

float mouse_y_scale;


void
AD_ComputeAspectCoeffsEx (float& x, float& y, float& xoff, float& yoff)
{
  yoff = 0.0f;
  xoff = 0.0f;

  x = 1.0f;
  y = 1.0f;

  if (! (config.render.aspect_correction))
    return;

  config.render.aspect_ratio = ad::RenderFix::width / ad::RenderFix::height;
  float rescale = (1.77777778f / config.render.aspect_ratio);

  // Wider
  if (config.render.aspect_ratio > 1.7777f) {
    int width = (16.0f / 9.0f) * ad::RenderFix::height;
    int x_off = (ad::RenderFix::width - width) / 2;

    x    = (float)ad::RenderFix::width / (float)width;
    xoff = x_off;
  } else {
    int height = (9.0f / 16.0f) * ad::RenderFix::width;
    int y_off  = (ad::RenderFix::height - height) / 2;

    y    = (float)ad::RenderFix::height / (float)height;
    yoff = y_off;
  }
}

// Returns the original cursor position and stores the new one in pPoint
POINT
CalcCursorPos (LPPOINT pPoint)
{
  float xscale, yscale;
  float xoff,   yoff;

  AD_ComputeAspectCoeffsEx (xscale, yscale, xoff, yoff);

  //
  // TODO: Factor this straight into xoff during AD_ComputeAspectCoeffs
  //
  yscale = xscale;

  if (! config.render.center_ui) {
    xscale = 1.0f;
    xoff   = 0.0f;
  }

  pPoint->x = ((float)pPoint->x - xoff) * xscale;
  pPoint->y = ((float)pPoint->y - yoff) * yscale;

  ///int width = ad::RenderFix::width;
  ///int height = (9.0f / 16.0f) * width;

  ////float scale = (float)height / ad::RenderFix::height;

  ////float y_ndc = 2.0f * ((float)pPoint->y / (float)ad::RenderFix::height) - 1.0f;

  /////pPoint->y = ((float)pPoint->y - (ad::RenderFix::height / xscale)) * (xscale);// ((y_ndc * ad::RenderFix::height * scale + ad::RenderFix::height * scale) / 3.0f);

  return *pPoint;
}


WNDPROC original_wndproc = nullptr;

LRESULT
CALLBACK
DetourWindowProc ( _In_  HWND   hWnd,
                   _In_  UINT   uMsg,
                   _In_  WPARAM wParam,
                   _In_  LPARAM lParam )
{
  if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) {
    static POINT last_p = { LONG_MIN, LONG_MIN };

    POINT p;

    p.x = MAKEPOINTS (lParam).x;
    p.y = MAKEPOINTS (lParam).y;

    if (/*game_state.needsFixedMouseCoords () &&*/config.render.aspect_correction) {
      // Only do this if cursor actually moved!
      //
      //   Otherwise, it tricks the game into thinking the input device changed
      //     from gamepad to mouse (and changes button icons).
      if (last_p.x != p.x || last_p.y != p.y) {
        CalcCursorPos (&p);

        last_p = p;
      }

      return CallWindowProc (original_wndproc, hWnd, uMsg, wParam, MAKELPARAM (p.x, p.y));
    }

    last_p = p;
  }

  return CallWindowProc (original_wndproc, hWnd, uMsg, wParam, lParam);
}


typedef BOOL (WINAPI *GetCursorInfo_t)
  (_Inout_ PCURSORINFO pci);

GetCursorInfo_t GetCursorInfo_Original = nullptr;

BOOL
WINAPI
GetCursorInfo_Detour (PCURSORINFO pci)
{
  BOOL ret = GetCursorInfo_Original (pci);

  // Correct the cursor position for Aspect Ratio
  if (config.render.aspect_correction) {
    POINT pt;

    pt.x = pci->ptScreenPos.x;
    pt.y = pci->ptScreenPos.y;

    CalcCursorPos (&pt);

    pci->ptScreenPos.x = pt.x;
    pci->ptScreenPos.y = pt.y;
  }

  return ret;
}

typedef BOOL (WINAPI *GetCursorPos_t)
  (_Out_ LPPOINT lpPoint);

GetCursorPos_t GetCursorPos_Original = nullptr;

BOOL
WINAPI
GetCursorPos_Detour (LPPOINT lpPoint)
{
  BOOL ret = GetCursorPos_Original (lpPoint);

  // Correct the cursor position for Aspect Ratio
  if (config.render.aspect_correction)
    CalcCursorPos (lpPoint);

  // Defer initialization of the Window Message redirection stuff until
  //   the first time the game calls GetCursorPos (...)
  if (original_wndproc == nullptr && ad::RenderFix::hWndDevice != NULL) {
    original_wndproc =
      (WNDPROC)GetWindowLong (ad::RenderFix::hWndDevice, GWL_WNDPROC);

    SetWindowLong ( ad::RenderFix::hWndDevice,
                      GWL_WNDPROC,
                        (LONG)DetourWindowProc );
  }

  return ret;
}


class AD_InputHooker
{
private:
  HANDLE                  hMsgPump;
  struct hooks_t {
    HHOOK                 keyboard;
    HHOOK                 mouse;
  } hooks;

  static AD_InputHooker*  pInputHook;

  static char             text [4096];

  static BYTE             keys_ [256];
  static bool             visible;

  static bool             command_issued;
  static std::string      result_str;

  struct command_history_t {
    std::vector <std::string> history;
    int_fast32_t              idx     = -1;
  } static commands;

protected:
  AD_InputHooker (void) { }

public:
  static AD_InputHooker* getInstance (void)
  {
    if (pInputHook == NULL)
      pInputHook = new AD_InputHooker ();

    return pInputHook;
  }

  void Start (void)
  {
    hMsgPump =
      CreateThread ( NULL,
                       NULL,
                         AD_InputHooker::MessagePump,
                           &hooks,
                             NULL,
                               NULL );

    AD_CreateDLLHook ( L"user32.dll", "GetCursorInfo",
                        GetCursorInfo_Detour,
              (LPVOID*)&GetCursorInfo_Original );

    AD_CreateDLLHook ( L"user32.dll", "GetCursorPos",
                        GetCursorPos_Detour,
              (LPVOID*)&GetCursorPos_Original );
  }

  void End (void)
  {
    TerminateThread     (hMsgPump, 0);
    UnhookWindowsHookEx (hooks.keyboard);
    UnhookWindowsHookEx (hooks.mouse);
  }

  void Draw (void)
  {
    typedef BOOL (__stdcall *BMF_DrawExternalOSD_t)(std::string app_name, std::string text);

    static HMODULE               hMod =
      GetModuleHandle (config.system.injector.c_str ());
    static BMF_DrawExternalOSD_t BMF_DrawExternalOSD
      =
      (BMF_DrawExternalOSD_t)GetProcAddress (hMod, "BMF_DrawExternalOSD");

    std::string output;

    static DWORD last_time = timeGetTime ();
    static bool  carret    = true;

    if (visible) {
      output += text;

      // Blink the Carret
      if (timeGetTime () - last_time > 333) {
        carret = ! carret;

        last_time = timeGetTime ();
      }

      if (carret)
        output += "-";

      // Show Command Results
      if (command_issued) {
        output += "\n";
        output += result_str;
      }
    }

    BMF_DrawExternalOSD ("ToZ Fix", output.c_str ());
  }

  HANDLE GetThread (void)
  {
    return hMsgPump;
  }

  static DWORD
  WINAPI
  MessagePump (LPVOID hook_ptr)
  {
    hooks_t* pHooks = (hooks_t *)hook_ptr;

    ZeroMemory (text, 4096);

    text [0] = '>';

    extern    HMODULE hDLLMod;

    DWORD dwThreadId;

    int hits = 0;

    DWORD dwTime = timeGetTime ();

    while (true) {
      // Spin until the game has a render window setup and various
      //   other resources loaded
      if (! ad::RenderFix::pDevice) {
        Sleep (83);
        continue;
      }

      DWORD dwProc;

      dwThreadId =
        GetWindowThreadProcessId (GetForegroundWindow (), &dwProc);

      // Ugly hack, but a different window might be in the foreground...
      if (dwProc != GetCurrentProcessId ()) {
        //dll_log.Log (L" *** Tried to hook the wrong process!!!");
        Sleep (83);
        continue;
      }

      break;
    }

    dll_log.Log ( L"  # Found window in %03.01f seconds, "
                     L"installing keyboard hook...",
                   (float)(timeGetTime () - dwTime) / 1000.0f );

    dwTime = timeGetTime ();
    hits   = 1;

    while (! (pHooks->keyboard = SetWindowsHookEx ( WH_KEYBOARD,
                                                      KeyboardProc,
                                                        hDLLMod,
                                                          dwThreadId ))) {
      _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

      dll_log.Log ( L"  @ SetWindowsHookEx failed: 0x%04X (%s)",
                    err.WCode (), err.ErrorMessage () );

      ++hits;

      if (hits >= 5) {
        dll_log.Log ( L"  * Failed to install keyboard hook after %lu tries... "
          L"bailing out!",
          hits );
        return 0;
      }

      Sleep (1);
    }

    while (! (pHooks->mouse = SetWindowsHookEx ( WH_MOUSE,
                                                   MouseProc,
                                                     hDLLMod,
                                                       dwThreadId ))) {
      _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

      dll_log.Log ( L"  @ SetWindowsHookEx failed: 0x%04X (%s)",
                    err.WCode (), err.ErrorMessage () );

      ++hits;

      if (hits >= 5) {
        dll_log.Log ( L"  * Failed to install mouse hook after %lu tries... "
          L"bailing out!",
          hits );
        return 0;
      }

      Sleep (1);
    }

    dll_log.Log ( L"  * Installed keyboard hook for command console... "
                        L"%lu %s (%lu ms!)",
                  hits,
                    hits > 1 ? L"tries" : L"try",
                      timeGetTime () - dwTime );

    while (true) {
      Sleep (10);
    }
    //193 - 199

    return 0;
  }

  static LRESULT
  CALLBACK
  MouseProc (int nCode, WPARAM wParam, LPARAM lParam)
  {
    MOUSEHOOKSTRUCT* pmh = (MOUSEHOOKSTRUCT *)lParam;

    return CallNextHookEx (AD_InputHooker::getInstance ()->hooks.mouse, nCode, wParam, lParam);
  }

  static LRESULT
  CALLBACK
  KeyboardProc (int nCode, WPARAM wParam, LPARAM lParam)
  {
    if (nCode >= 0) {
      BYTE    vkCode   = LOWORD (wParam) & 0xFF;
      BYTE    scanCode = HIWORD (lParam) & 0x7F;
      bool    repeated = LOWORD (lParam);
      bool    keyDown  = ! (lParam & 0x80000000);

      if (visible && vkCode == VK_BACK) {
        if (keyDown) {
          size_t len = strlen (text);
                 len--;

          if (len < 1)
            len = 1;

          text [len] = '\0';
        }
      }

      else if ((vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT)) {
        if (keyDown) keys_ [VK_SHIFT] = 0x81; else keys_ [VK_SHIFT] = 0x00;
      }

      else if ((!repeated) && vkCode == VK_CAPITAL) {
        if (keyDown) if (keys_ [VK_CAPITAL] == 0x00) keys_ [VK_CAPITAL] = 0x81; else keys_ [VK_CAPITAL] = 0x00;
      }

      else if ((vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL)) {
        if (keyDown) keys_ [VK_CONTROL] = 0x81; else keys_ [VK_CONTROL] = 0x00;
      }

      else if ((vkCode == VK_UP) || (vkCode == VK_DOWN)) {
        if (keyDown && visible) {
          if (vkCode == VK_UP)
            commands.idx--;
          else
            commands.idx++;

          // Clamp the index
          if (commands.idx < 0)
            commands.idx = 0;
          else if (commands.idx >= commands.history.size ())
            commands.idx = commands.history.size () - 1;

          if (commands.history.size ()) {
            strcpy (&text [1], commands.history [commands.idx].c_str ());
            command_issued = false;
          }
        }
      }

      else if (visible && vkCode == VK_RETURN) {
        if (keyDown && LOWORD (lParam) < 2) {
          size_t len = strlen (text+1);
          // Don't process empty or pure whitespace command lines
          if (len > 0 && strspn (text+1, " ") != len) {
            eTB_CommandResult result = command.ProcessCommandLine (text+1);

            if (result.getStatus ()) {
              // Don't repeat the same command over and over
              if (commands.history.size () == 0 ||
                  commands.history.back () != &text [1]) {
                commands.history.push_back (&text [1]);
              }

              commands.idx = commands.history.size ();

              text [1] = '\0';

              command_issued = true;
            }
            else {
              command_issued = false;
            }

            result_str = result.getWord   () + std::string (" ")   +
                         result.getArgs   () + std::string (":  ") +
                         result.getResult ();
          }
        }
      }

      else if (keyDown) {
        bool new_press = keys_ [vkCode] != 0x81;

        keys_ [vkCode] = 0x81;

        if (keys_ [VK_CONTROL] && keys_ [VK_SHIFT]) {
          if (keys_ [VK_TAB] && new_press) {
            visible = ! visible;

            // Avoid duplicating a BMF feature
            static HMODULE hD3D9 = GetModuleHandle (config.system.injector.c_str ());

            typedef void (__stdcall *BMF_SteamAPI_SetOverlayState_t)(bool);
            static BMF_SteamAPI_SetOverlayState_t BMF_SteamAPI_SetOverlayState =
                (BMF_SteamAPI_SetOverlayState_t)
                  GetProcAddress ( hD3D9,
                                     "BMF_SteamAPI_SetOverlayState" );

            BMF_SteamAPI_SetOverlayState (visible);
          }
          else if (keys_ [VK_MENU] && keys_ ['A'] && new_press) {
            command.ProcessCommandLine ("AspectCorrection toggle");
          } else if (keys_ [VK_MENU] && keys_ ['Z'] && new_press) {
            command.ProcessCommandLine ("CenterUI toggle");
          } else if (keys_ [VK_OEM_COMMA]) {
            extern float scale_coeff;
            scale_coeff -= 0.001f;
          } else if (keys_ [VK_OEM_PERIOD]) {
            extern float scale_coeff;
            scale_coeff += 0.001f;
          } else if (keys_ [VK_MENU] && keys_ ['V'] && new_press) {
            extern bool vert_fix_map;
            vert_fix_map = ! vert_fix_map;
          } else if (keys_ [VK_MENU] && keys_ ['L'] && new_press) {
            command.ProcessCommandLine ("FramesToLog 1");
            command.ProcessCommandLine ("LogFrame true");
          } else if (keys_ [VK_MENU] && keys_ ['D'] && new_press) {
            command.ProcessCommandLine ("FixDOF toggle");
          } else if (keys_ [VK_MENU] && keys_ [VK_BACK] && new_press) {
            float* pfUIAspect = (float *)0x01618ee8;
            DWORD dwOld;
            VirtualProtect (pfUIAspect, 4, PAGE_READWRITE, &dwOld);
            *pfUIAspect = -*pfUIAspect;
            VirtualProtect (pfUIAspect, 4, dwOld, &dwOld);
          }
        }

        if (visible) {
          char key_str [2];
          key_str [1] = '\0';

          if (1 == ToAsciiEx ( vkCode,
                                scanCode,
                                keys_,
                              (LPWORD)key_str,
                                0,
                                GetKeyboardLayout (0) )) {
            strncat (text, key_str, 1);
            command_issued = false;
          }
        }
      }

      else if ((! keyDown))
        keys_ [vkCode] = 0x00;

      if (visible) return 1;
    }

    return CallNextHookEx (AD_InputHooker::getInstance ()->hooks.keyboard, nCode, wParam, lParam);
  };
};


MH_STATUS
WINAPI
AD_CreateFuncHook ( LPCWSTR pwszFuncName,
                    LPVOID  pTarget,
                    LPVOID  pDetour,
                    LPVOID *ppOriginal )
{
  static HMODULE hParent = GetModuleHandle (config.system.injector.c_str ());

  typedef MH_STATUS (WINAPI *BMF_CreateFuncHook_t)
      ( LPCWSTR pwszFuncName, LPVOID  pTarget,
        LPVOID  pDetour,      LPVOID *ppOriginal );
  static BMF_CreateFuncHook_t BMF_CreateFuncHook =
    (BMF_CreateFuncHook_t)GetProcAddress (hParent, "BMF_CreateFuncHook");

  return
    BMF_CreateFuncHook (pwszFuncName, pTarget, pDetour, ppOriginal);
}

MH_STATUS
WINAPI
AD_CreateDLLHook ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                   LPVOID  pDetour,    LPVOID *ppOriginal,
                   LPVOID *ppFuncAddr )
{
  static HMODULE hParent = GetModuleHandle (config.system.injector.c_str ());

  typedef MH_STATUS (WINAPI *BMF_CreateDLLHook_t)(
        LPCWSTR pwszModule, LPCSTR  pszProcName,
        LPVOID  pDetour,    LPVOID *ppOriginal, 
        LPVOID *ppFuncAddr );
  static BMF_CreateDLLHook_t BMF_CreateDLLHook =
    (BMF_CreateDLLHook_t)GetProcAddress (hParent, "BMF_CreateDLLHook");

  return
    BMF_CreateDLLHook (pwszModule,pszProcName,pDetour,ppOriginal,ppFuncAddr);
}

MH_STATUS
WINAPI
AD_EnableHook (LPVOID pTarget)
{
  static HMODULE hParent = GetModuleHandle (config.system.injector.c_str ());

  typedef MH_STATUS (WINAPI *BMF_EnableHook_t)(LPVOID pTarget);
  static BMF_EnableHook_t BMF_EnableHook =
    (BMF_EnableHook_t)GetProcAddress (hParent, "BMF_EnableHook");

  return BMF_EnableHook (pTarget);
}

MH_STATUS
WINAPI
AD_DisableHook (LPVOID pTarget)
{
  static HMODULE hParent = GetModuleHandle (config.system.injector.c_str ());

  typedef MH_STATUS (WINAPI *BMF_DisableHook_t)(LPVOID pTarget);
  static BMF_DisableHook_t BMF_DisableHook =
    (BMF_DisableHook_t)GetProcAddress (hParent, "BMF_DisableHook");

  return BMF_DisableHook (pTarget);
}

MH_STATUS
WINAPI
AD_RemoveHook (LPVOID pTarget)
{
  static HMODULE hParent = GetModuleHandle (config.system.injector.c_str ());

  typedef MH_STATUS (WINAPI *BMF_RemoveHook_t)(LPVOID pTarget);
  static BMF_RemoveHook_t BMF_RemoveHook =
    (BMF_RemoveHook_t)GetProcAddress (hParent, "BMF_RemoveHook");

  return BMF_RemoveHook (pTarget);
}

MH_STATUS
WINAPI
AD_Init_MinHook (void)
{
  MH_STATUS status = MH_OK;

  AD_InputHooker* pHook = AD_InputHooker::getInstance ();
  pHook->Start ();

  return status;
}

MH_STATUS
WINAPI
AD_UnInit_MinHook (void)
{
  MH_STATUS status = MH_OK;

  AD_InputHooker* pHook = AD_InputHooker::getInstance ();
  pHook->End ();

  return status;
}

void
AD_DrawCommandConsole (void)
{
  static int draws = 0;

  // Skip the first frame, so that the console appears below the
  //  other OSD.
  if (draws++ > 20) {
    AD_InputHooker* pHook = AD_InputHooker::getInstance ();
    pHook->Draw ();
  }
}


AD_InputHooker* AD_InputHooker::pInputHook;
char            AD_InputHooker::text [4096];

BYTE        AD_InputHooker::keys_ [256]    = { 0 };
bool        AD_InputHooker::visible        = false;

bool        AD_InputHooker::command_issued = false;
std::string AD_InputHooker::result_str;

AD_InputHooker::command_history_t AD_InputHooker::commands;