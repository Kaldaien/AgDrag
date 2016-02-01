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
#define _CRT_SECURE_NO_WARNINGS

#include <string>

#include "hook.h"
#include "log.h"
#include "command.h"

#include "render.h"

#include <d3d9.h>
#include <dinput.h>

#include "config.h"

#include <mmsystem.h>
#pragma comment (lib, "winmm.lib")

#include <comdef.h>


// State for window activation faking
struct ad_window_state_s {
  bool active          = true;
  bool activating      = false;
  RECT cursor_clip;
} window;


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
  float rescale = ((16.0f / 9.0f) / config.render.aspect_ratio);

  // Wider
  if (config.render.aspect_ratio > (16.0f / 9.0f)) {
    int width = (16.0f / 9.0f) * ad::RenderFix::height;
    int x_off = (ad::RenderFix::width - width) / 2;

    x    = (float)ad::RenderFix::width / (float)width;
    xoff = x_off;

    dll_log.Log (L"x (%f) : xoff (%f)", x, xoff);

#if 0
    // Calculated height will be greater than we started with, so work
    //  in the wrong direction here...
    int height = (9.0f / 16.0f) * ad::RenderFix::width;
    y          = (float)ad::RenderFix::height / (float)height;
#endif

    yoff = config.scaling.mouse_y_offset;
  } else {
// No fix is needed in this direction
#if 0
    int height = (9.0f / 16.0f) * ad::RenderFix::width;
    int y_off  = (ad::RenderFix::height - height) / 2;

    y    = (float)ad::RenderFix::height / (float)height;
    yoff = y_off;
#endif
  }
}

// Returns the original cursor position and stores the new one in pPoint
POINT
CalcCursorPos (LPPOINT pPoint)
{
  float xscale, yscale;
  float xoff,   yoff;

  AD_ComputeAspectCoeffsEx (xscale, yscale, xoff, yoff);

  if (! config.render.center_ui) {
    xscale = 1.0f;
    xoff   = 0.0f;
  }

  pPoint->x = ((float)pPoint->x - xoff) * xscale;
  pPoint->y = ((float)pPoint->y - yoff) * xscale;

  return *pPoint;
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
  if (config.render.aspect_correction && config.render.aspect_ratio > (16.0f / 9.0f)) {
    POINT pt;

    pt.x = pci->ptScreenPos.x;
    pt.y = pci->ptScreenPos.y;

    CalcCursorPos (&pt);

    pci->ptScreenPos.x = pt.x;
    pci->ptScreenPos.y = pt.y;
  }

  return ret;
}


LRESULT
CALLBACK
DetourWindowProc ( _In_  HWND   hWnd,
                   _In_  UINT   uMsg,
                   _In_  WPARAM wParam,
                   _In_  LPARAM lParam );

WNDPROC original_wndproc = nullptr;


typedef BOOL (WINAPI *GetCursorPos_t)
  (_Out_ LPPOINT lpPoint);

GetCursorPos_t GetCursorPos_Original = nullptr;

BOOL
WINAPI
GetCursorPos_Detour (LPPOINT lpPoint)
{
  BOOL ret = GetCursorPos_Original (lpPoint);

  // Correct the cursor position for Aspect Ratio
  if (config.render.aspect_correction && config.render.aspect_ratio > (16.0f / 9.0f))
    CalcCursorPos (lpPoint);

  return ret;
}

typedef BOOL (WINAPI *IsIconic_pfn)(HWND hWnd);
IsIconic_pfn IsIconic_Original = nullptr;

BOOL
WINAPI
IsIconic_Detour (HWND hWnd)
{
  if (config.render.allow_background)
    return FALSE;
  else
    return IsIconic_Original (hWnd);
}


typedef HWND (WINAPI *GetForegroundWindow_pfn)(void);
GetForegroundWindow_pfn GetForegroundWindow_Original = nullptr;

HWND
WINAPI
GetForegroundWindow_Detour (void)
{
  if (config.render.allow_background) {
    return ad::RenderFix::hWndDevice;
  }

  return GetForegroundWindow_Original ();
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

  bool isVisible (void) { return visible; }

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

    ad::RenderFix::hWndDevice = GetForegroundWindow ();

    // Defer initialization of the Window Message redirection stuff until
    //   the first time the game calls GetCursorPos (...)
    if (original_wndproc == nullptr && ad::RenderFix::hWndDevice != NULL) {
      original_wndproc =
        (WNDPROC)GetWindowLong (ad::RenderFix::hWndDevice, GWL_WNDPROC);

      SetWindowLong ( ad::RenderFix::hWndDevice,
                        GWL_WNDPROC,
                          (LONG)DetourWindowProc );
    }

    AD_CreateDLLHook ( L"user32.dll", "GetForegroundWindow",
                        GetForegroundWindow_Detour,
              (LPVOID*)&GetForegroundWindow_Original );

#if 0
    AD_CreateDLLHook ( L"user32.dll", "IsIconic",
                        IsIconic_Detour,
              (LPVOID*)&IsIconic_Original );
#endif

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

      if (config.render.allow_background) {
        LONG dwStyle   = GetWindowLong (ad::RenderFix::hWndDevice, GWL_STYLE);
        LONG dwStyleEx = GetWindowLong (ad::RenderFix::hWndDevice, GWL_EXSTYLE);

        dwStyle   &= ~(WS_BORDER | WS_CAPTION | WS_THICKFRAME | WS_OVERLAPPEDWINDOW | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU | WS_GROUP);
        dwStyleEx &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE | WS_EX_OVERLAPPEDWINDOW | WS_EX_PALETTEWINDOW | WS_EX_MDICHILD);

        SetWindowLong (ad::RenderFix::hWndDevice, GWL_STYLE,   dwStyle);
        SetWindowLong (ad::RenderFix::hWndDevice, GWL_EXSTYLE, dwStyleEx);

        SetWindowPos  ( ad::RenderFix::hWndDevice,
                          NULL,
                            0,0,ad::RenderFix::width,ad::RenderFix::height,
                              SWP_FRAMECHANGED |
                              SWP_NOZORDER     | SWP_NOOWNERZORDER );
    }

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
          else if (keys_ [VK_MENU] && vkCode == 'A' && new_press) {
            command.ProcessCommandLine ("AspectCorrection toggle");
          } else if (keys_ [VK_MENU] && vkCode == 'Z' && new_press) {
            command.ProcessCommandLine ("CenterUI toggle");
          } else if (vkCode == VK_OEM_COMMA) {
            if (! config.scaling.locked)
              config.scaling.hud_x_offset -= 0.01f;
          } else if (vkCode == VK_OEM_PERIOD) {
            if (! config.scaling.locked)
              config.scaling.hud_x_offset += 0.01f;
          } else if (vkCode == VK_OEM_6) {
            extern float name_shift_coeff;
            name_shift_coeff += 0.001f;
          } else if (vkCode == VK_OEM_4) {
            extern float name_shift_coeff;
            name_shift_coeff -= 0.001f;
          } else if (keys_ [VK_MENU] && vkCode == 'V' && new_press) {
            extern bool vert_fix_map;
            vert_fix_map = ! vert_fix_map;
          } else if (keys_ [VK_MENU] && vkCode == 'L' && new_press) {
            command.ProcessCommandLine ("FramesToTrace 1");
            command.ProcessCommandLine ("TraceFrame true");
          } else if (keys_ [VK_MENU] && vkCode == 'D' && new_press) {
            command.ProcessCommandLine ("FixDOF toggle");
          } else if (keys_ [VK_MENU] && vkCode == VK_BACK && new_press) {
            float* pfUIAspect = (float *)0x01618ee8;
            DWORD dwOld;
            VirtualProtect (pfUIAspect, 4, PAGE_READWRITE, &dwOld);
            *pfUIAspect = -*pfUIAspect;
            VirtualProtect (pfUIAspect, 4, dwOld, &dwOld);
          } else if (keys_ [VK_MENU] && vkCode == config.nametags.toggle_on_top_key && new_press) {
            if (config.nametags.always_on_top++ > 1)
              config.nametags.always_on_top = 0;
          } else if (keys_ [VK_MENU] && vkCode == 'M' && new_press) {
            config.nametags.aspect_correct = (! config.nametags.aspect_correct);
          } else if (keys_ [VK_MENU] && vkCode == 'B' && new_press) {
            command.ProcessCommandLine ("Render.AllowBG toggle");
          }
        }

        if (vkCode == config.nametags.hold_on_top_key && new_press) {
          config.nametags.temp_on_top = true;
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

      else if ((! keyDown)) {
        bool new_release = keys_ [vkCode] != 0x00;

        keys_ [vkCode] = 0x00;

        if (vkCode == 'N' && new_release) {
          config.nametags.temp_on_top = false;
        }
      }

      if (visible) return 1;
    }

    return CallNextHookEx (AD_InputHooker::getInstance ()->hooks.keyboard, nCode, wParam, lParam);
  };
};


typedef BOOL (WINAPI *ClipCursor_pfn)(
  _In_opt_ const RECT *lpRect
);

ClipCursor_pfn ClipCursor_Original = nullptr;

BOOL
WINAPI
ClipCursor_Detour (const RECT *lpRect)
{
  if (lpRect != nullptr)
    window.cursor_clip = *lpRect;

  if (window.active) {
    return ClipCursor_Original (lpRect);
  } else {
    return ClipCursor_Original (nullptr);
  }
}



LRESULT
CALLBACK
DetourWindowProc ( _In_  HWND   hWnd,
                   _In_  UINT   uMsg,
                   _In_  WPARAM wParam,
                   _In_  LPARAM lParam )
{
  // Block keyboard input to the game while the console is visible
  if (uMsg == WM_INPUT && AD_InputHooker::getInstance ()->isVisible ())
    return 0;


  // Allow the game to run in the background
  if (uMsg == WM_ACTIVATEAPP) {
    bool last_active = window.active;

    window.active = wParam;

    // Unrestrict the mouse when the app is deactivated
    if ((! window.active) && config.render.allow_background) {
      LONG dwStyle   = GetWindowLong (ad::RenderFix::hWndDevice, GWL_STYLE);
      LONG dwStyleEx = GetWindowLong (ad::RenderFix::hWndDevice, GWL_EXSTYLE);

      dwStyle   &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
      dwStyleEx &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);

      SetWindowLong (ad::RenderFix::hWndDevice, GWL_STYLE,   dwStyle);
      SetWindowLong (ad::RenderFix::hWndDevice, GWL_EXSTYLE, dwStyleEx);

      SetWindowPos  ( ad::RenderFix::hWndDevice,
                        NULL,
                          0,0,ad::RenderFix::width,ad::RenderFix::height,
                            SWP_FRAMECHANGED |
                            SWP_NOZORDER     | SWP_NOOWNERZORDER );

      ClipCursor_Original (nullptr);
    }

    // Restore it when the app is activated
    else {
      ClipCursor_Original (&window.cursor_clip);
    }

    if (config.render.allow_background) {
      CallWindowProc (original_wndproc, hWnd, uMsg, FALSE, ad::RenderFix::dwRenderThreadID);
      CallWindowProc (original_wndproc, hWnd, uMsg, TRUE,  ad::RenderFix::dwRenderThreadID);
      //window.activating = true;
      return 0;
    }
  }


  // Don't let the game do anything with the mouse or keyboard when
  //   the game is not active
  if (config.render.allow_background) {
    if ((! window.active) && uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
      return 0;

    if ((! window.active) && uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST)
      return 0;
  }


  if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) {
    static POINT last_p = { LONG_MIN, LONG_MIN };

    POINT p;

    p.x = MAKEPOINTS (lParam).x;
    p.y = MAKEPOINTS (lParam).y;

    if (/*game_state.needsFixedMouseCoords () &&*/config.render.aspect_correction) {
      // Only do this if cursor actually moved!
      //
      //   Otherwise, it tricks the game into thinking the input device changed
      //     from gamepad to mouse (and changes buessagetton icons).
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


#pragma comment (lib, "dxguid.lib")

#define DINPUT8_CALL(_Ret, _Call) {                                      \
  dll_log.LogEx (true, L"  Calling original function: ");                \
  (_Ret) = (_Call);                                                      \
  _com_error err ((_Ret));                                               \
  if ((_Ret) != S_OK)                                                    \
    dll_log.LogEx (false, L"(ret=0x%04x - %s)\n\n", err.WCode (),        \
                                                    err.ErrorMessage ());\
  else                                                                   \
    dll_log.LogEx (false, L"(ret=S_OK)\n\n");                            \
}

typedef HRESULT (WINAPI *IDirectInputDevice8_GetDeviceState_pfn)(IDirectInputDevice8* This,
                                                                 DWORD                cbData,
                                                                 LPVOID               lpvData);
IDirectInputDevice8_GetDeviceState_pfn IDirectInputDevice8_GetDeviceState_Original = nullptr;

HRESULT
WINAPI
IDirectInputDevice8_GetDeviceState_Detour ( IDirectInputDevice8       *This,
                                            DWORD                      cbData,
                                            LPVOID                     lpvData )
{
  static uint8_t mouse_state [128];

  // For input faking
  if ((! window.active) && config.render.allow_background) {
    memcpy (lpvData, mouse_state, cbData);
    return S_OK;
  }

  HRESULT hr;
  hr = IDirectInputDevice8_GetDeviceState_Original ( This,
                                                       cbData,
                                                         lpvData );

  if (SUCCEEDED (hr)) {
    if (window.active) {
      memcpy (mouse_state, lpvData, cbData);
    }
  }

  return hr;
}

typedef HRESULT (WINAPI *IDirectInput8_CreateDevice_pfn)(
  IDirectInput8       *This,
  REFGUID              rguid,
  LPDIRECTINPUTDEVICE *lplpDirectInputDevice,
  LPUNKNOWN            pUnkOuter);
IDirectInput8_CreateDevice_pfn IDirectInput8_CreateDevice_Original = nullptr;

HRESULT
WINAPI
IDirectInput8_CreateDevice_Detour ( IDirectInput8       *This,
                                    REFGUID              rguid,
                                    LPDIRECTINPUTDEVICE *lplpDirectInputDevice,
                                    LPUNKNOWN            pUnkOuter )
{
  const wchar_t* wszDevice = (rguid == GUID_SysKeyboard) ? L"Default System Keyboard" :
                                (rguid == GUID_SysMouse) ? L"Default System Mouse" :
                                                           L"Other Device";

  dll_log.Log ( L" [!] IDirectInput8::CreateDevice (%08Xh, %s, %08Xh, %08Xh)",
                  This,
                    wszDevice,
                      lplpDirectInputDevice,
                        pUnkOuter );

  HRESULT hr;
  DINPUT8_CALL ( hr,
                  IDirectInput8_CreateDevice_Original ( This,
                                                         rguid,
                                                          lplpDirectInputDevice,
                                                           pUnkOuter ) );

  if (SUCCEEDED (hr)) {
    if (rguid == GUID_SysMouse) {
      void** vftable = *(void***)*lplpDirectInputDevice;

      AD_CreateFuncHook ( L"IDirectInputDevice8::GetDeviceState",
                          vftable [9],
                          IDirectInputDevice8_GetDeviceState_Detour,
                (LPVOID*)&IDirectInputDevice8_GetDeviceState_Original );

      AD_EnableHook (vftable [9]);
    }
  }

  return hr;
}


typedef HRESULT (WINAPI *DirectInput8Create_pfn)( HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);
DirectInput8Create_pfn DirectInput8Create_Original = nullptr;

HRESULT
WINAPI
DirectInput8Create_Detour (HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter)
{
  dll_log.Log ( L" [!] DirectInput8Create (0x%X, %lu, ..., %08Xh, %08Xh)",
                  hinst, dwVersion, /*riidltf,*/ ppvOut, punkOuter );

  HRESULT hr;
  DINPUT8_CALL (hr,
    DirectInput8Create_Original ( hinst,
                                    dwVersion,
                                      riidltf,
                                        ppvOut,
                                          punkOuter ));

  // Avoid multiple hooks for third-party compatibility
  static bool hooked = false;

  if (SUCCEEDED (hr) && (! hooked)) {
    void** vftable = *(void***)*ppvOut;

    AD_CreateFuncHook ( L"IDirectInput8::CreateDevice",
                        vftable [3],
                        IDirectInput8_CreateDevice_Detour,
              (LPVOID*)&IDirectInput8_CreateDevice_Original );

    AD_EnableHook (vftable [3]);

    hooked = true;
  }

  return hr;
}


typedef UINT (WINAPI *GetRawInputData_pfn)(
  _In_      HRAWINPUT hRawInput,
  _In_      UINT      uiCommand,
  _Out_opt_ LPVOID    pData,
  _Inout_   PUINT     pcbSize,
  _In_      UINT      cbSizeHeader
);

GetRawInputData_pfn GetRawInputData_Original = nullptr;

UINT
WINAPI
GetRawInputData_Detour (_In_      HRAWINPUT hRawInput,
                        _In_      UINT      uiCommand,
                        _Out_opt_ LPVOID    pData,
                        _Inout_   PUINT     pcbSize,
                        _In_      UINT      cbSizeHeader)
{
  // Block keyboard input to the game while the console is active
  if (AD_InputHooker::getInstance ()->isVisible ())
    return 0;

  return GetRawInputData_Original (hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
}


typedef SHORT (WINAPI *GetAsyncKeyState_pfn)(
  _In_ int vKey
);

GetAsyncKeyState_pfn GetAsyncKeyState_Original = nullptr;

SHORT
WINAPI
GetAsyncKeyState_Detour (_In_ int vKey)
{
#define AD_ConsumeVKey(vKey) { GetAsyncKeyState_Original(vKey); return 0; }

  // Window is not active, but we are faking it...
  if ((! window.active) && config.render.allow_background)
    AD_ConsumeVKey (vKey);

  // Block keyboard input to the game while the console is active
  if (AD_InputHooker::getInstance ()->isVisible ()) {
    AD_ConsumeVKey (vKey);
  }

  // Block Left Alt
  if (vKey == VK_LMENU)
    if (config.keyboard.block_left_alt)
      AD_ConsumeVKey (vKey);

  // Block Left Ctrl
  if (vKey == VK_LCONTROL)
    if (config.keyboard.block_left_ctrl)
      AD_ConsumeVKey (vKey);

  return GetAsyncKeyState_Original (vKey);
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

  AD_CreateDLLHook ( L"dinput8.dll", "DirectInput8Create",
                     DirectInput8Create_Detour,
           (LPVOID*)&DirectInput8Create_Original );

  AD_CreateDLLHook ( L"user32.dll", "GetRawInputData",
                        GetRawInputData_Detour,
              (LPVOID*)&GetRawInputData_Original );

  AD_CreateDLLHook ( L"user32.dll", "GetAsyncKeyState",
                        GetAsyncKeyState_Detour,
              (LPVOID*)&GetAsyncKeyState_Original );

  AD_CreateDLLHook ( L"user32.dll", "ClipCursor",
                        ClipCursor_Detour,
              (LPVOID*)&ClipCursor_Original );

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