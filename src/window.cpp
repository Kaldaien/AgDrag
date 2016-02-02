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

#include "window.h"
#include "input.h"
#include "render.h"

#include "config.h"
#include "log.h"
#include "hook.h"

ad_window_state_s window;

#if 0
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
#endif

LRESULT
CALLBACK
DetourWindowProc ( _In_  HWND   hWnd,
                   _In_  UINT   uMsg,
                   _In_  WPARAM wParam,
                   _In_  LPARAM lParam );

WNDPROC original_wndproc = nullptr;

typedef LONG
(WINAPI *SetWindowLongA_pfn)(
    _In_ HWND hWnd,
    _In_ int nIndex,
    _In_ LONG dwNewLong);

SetWindowLongA_pfn SetWindowLongA_Original = nullptr;

LONG
WINAPI
SetWindowLongA_Detour (
  _In_ HWND hWnd,
  _In_ int nIndex,
  _In_ LONG dwNewLong
)
{
  ad::RenderFix::hWndDevice = hWnd;
  window.hwnd               = hWnd;

  // Setup window message detouring as soon as a window is created..
  if (original_wndproc == nullptr) {
    original_wndproc =
      (WNDPROC)GetWindowLong (ad::RenderFix::hWndDevice, GWL_WNDPROC);

    SetWindowLongA_Original ( ad::RenderFix::hWndDevice,
                                GWL_WNDPROC,
                                  (LONG)DetourWindowProc );
  }

  // Borderless baby!
  if (config.render.allow_background) {
    if (nIndex == GWL_STYLE) {
      window.borderless = true;
      window.style = dwNewLong;
      dwNewLong &= ~( WS_BORDER           | WS_CAPTION  | WS_THICKFRAME |
                      WS_OVERLAPPEDWINDOW | WS_MINIMIZE | WS_MAXIMIZE   |
                      WS_SYSMENU          | WS_GROUP );
    }

    if (nIndex == GWL_EXSTYLE) {
      window.borderless = true;
      window.style_ex = dwNewLong;
      dwNewLong &= ~( WS_EX_DLGMODALFRAME    | WS_EX_CLIENTEDGE    |
                      WS_EX_STATICEDGE       | WS_EX_WINDOWEDGE    |
                      WS_EX_OVERLAPPEDWINDOW | WS_EX_PALETTEWINDOW |
                      WS_EX_MDICHILD );
    }

    LONG ret = SetWindowLongA_Original (hWnd, nIndex, dwNewLong);

    SetWindowPos  ( ad::RenderFix::hWndDevice,
                        NULL,
                          0,0,0,0,
                            SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
                            SWP_NOZORDER     | SWP_NOOWNERZORDER );

    return ret;
  } else {
    window.borderless = false;
    return SetWindowLongA_Original (hWnd, nIndex, dwNewLong);
  }
}

void
ad::WindowManager::BorderManager::Enable (void)
{
  if (! window.borderless)
    Toggle ();
}

void
ad::WindowManager::BorderManager::Disable (void)
{
  if (! window.borderless)
    Toggle ();
}

void
ad::WindowManager::BorderManager::Toggle (void)
{
  window.borderless = (! window.borderless);

  if (! window.borderless) {
    SetWindowLongW (window.hwnd, GWL_STYLE,   window.style);
    SetWindowLongW (window.hwnd, GWL_EXSTYLE, window.style_ex);
  } else {
    DWORD dwNewLong = window.style;

    dwNewLong &= ~( WS_BORDER           | WS_CAPTION  | WS_THICKFRAME |
                    WS_OVERLAPPEDWINDOW | WS_MINIMIZE | WS_MAXIMIZE   |
                    WS_SYSMENU          | WS_GROUP );

    SetWindowLongW (window.hwnd, GWL_STYLE,   dwNewLong);

    dwNewLong = window.style_ex;

    dwNewLong &= ~( WS_EX_DLGMODALFRAME    | WS_EX_CLIENTEDGE    |
                    WS_EX_STATICEDGE       | WS_EX_WINDOWEDGE    |
                    WS_EX_OVERLAPPEDWINDOW | WS_EX_PALETTEWINDOW |
                    WS_EX_MDICHILD );

    SetWindowLongW (window.hwnd, GWL_EXSTYLE, dwNewLong);
  }

  SetWindowPos  ( ad::RenderFix::hWndDevice,
                    NULL,
                      0,0,0,0,
                        SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
                        SWP_NOZORDER     | SWP_NOOWNERZORDER );
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


LRESULT
CALLBACK
DetourWindowProc ( _In_  HWND   hWnd,
                   _In_  UINT   uMsg,
                   _In_  WPARAM wParam,
                   _In_  LPARAM lParam )
{
  // Block keyboard input to the game while the console is visible
  if (uMsg == WM_INPUT && ad::InputManager::Hooker::getInstance ()->isVisible ())
    return 0;


  // Allow the game to run in the background
  if (uMsg == WM_ACTIVATEAPP) {
    bool last_active = window.active;

    window.active = wParam;

    //
    // The window activation state is changing, among other things we can take
    //   this opportunity to setup a special framerate limit.
    //
    if (window.active != last_active) {
      eTB_CommandProcessor* pCommandProc =
        SK_GetCommandProcessor           ();

#if 0
      eTB_CommandResult     result       =
        pCommandProc->ProcessCommandLine ("TargetFPS");

      eTB_VarStub <float>* pOriginalLimit = (eTB_VarStub <float>*)result.getVariable ();
#endif
      // Went from active to inactive (enforce background limit)
      if (! window.active)
        pCommandProc->ProcessCommandFormatted ("TargetFPS %f", config.render.background_fps);

      // Went from inactive to active (restore foreground limit)
      else
        pCommandProc->ProcessCommandFormatted ("TargetFPS %f", config.render.foreground_fps);
    }

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
        ad::InputManager::CalcCursorPos (&p);

        last_p = p;
      }

      return CallWindowProc (original_wndproc, hWnd, uMsg, wParam, MAKELPARAM (p.x, p.y));
    }

    last_p = p;
  }

  return CallWindowProc (original_wndproc, hWnd, uMsg, wParam, lParam);
}

void
ad::WindowManager::Init (void)
{
  // Stupid game is using the old ANSI API
  AD_CreateDLLHook ( L"user32.dll", "SetWindowLongA",
                      SetWindowLongA_Detour,
            (LPVOID*)&SetWindowLongA_Original );

  AD_CreateDLLHook ( L"user32.dll", "GetForegroundWindow",
                      GetForegroundWindow_Detour,
            (LPVOID*)&GetForegroundWindow_Original );

#if 0
  AD_CreateDLLHook ( L"user32.dll", "IsIconic",
                      IsIconic_Detour,
            (LPVOID*)&IsIconic_Original );
#endif

  CommandProcessor* comm_proc = CommandProcessor::getInstance ();

  
}

void
ad::WindowManager::Shutdown (void)
{
}


ad::WindowManager::CommandProcessor::CommandProcessor (void)
{
  foreground_fps_ = new eTB_VarStub <float> (&config.render.foreground_fps, this);
  background_fps_ = new eTB_VarStub <float> (&config.render.background_fps, this);

  eTB_CommandProcessor* pCommandProc = SK_GetCommandProcessor ();

  pCommandProc->AddVariable ("Window.BackgroundFPS", background_fps_);
  pCommandProc->AddVariable ("Window.ForegroundFPS", foreground_fps_);

  // If the user has an FPS limit preference, set it up now...
  pCommandProc->ProcessCommandFormatted ("TargetFPS %f", config.render.foreground_fps);
}

bool
ad::WindowManager::CommandProcessor::OnVarChange (eTB_Variable* var, void* val)
{
  eTB_CommandProcessor* pCommandProc = SK_GetCommandProcessor ();

  bool known = false;

  if (var == background_fps_) {
    known = true;

    // Range validation
    if (val != nullptr && *(float *)val >= 0.0f) {
      config.render.background_fps = *(float *)val;

      // How this was changed while the window was inactive is a bit of a
      //   mystery, but whatever :P
      if ((! window.active))
        pCommandProc->ProcessCommandFormatted ("TargetFPS %f", *(float *)val);

      return true;
    }
  }

  if (var == foreground_fps_) {
    known = true;

    // Range validation
    if (val != nullptr && *(float *)val >= 0.0f) {
      config.render.foreground_fps = *(float *)val;

      // Immediately apply limiter changes
      if (window.active)
        pCommandProc->ProcessCommandFormatted ("TargetFPS %f", *(float *)val);

      return true;
    }
  }

  if (! known) {
    dll_log.Log ( L" [Window Manager]: UNKNOWN Variable Changed (%p --> %p)",
                    var,
                      val );
  }

  return false;
}

ad::WindowManager::CommandProcessor* ad::WindowManager::CommandProcessor::pCommProc;