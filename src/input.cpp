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

#include <Windows.h>

#include <comdef.h>

#include <dinput.h>
#pragma comment (lib, "dxguid.lib")

#include <cstdint>

#include "log.h"
#include "config.h"
#include "window.h"
#include "render.h"
#include "hook.h"

#include <mmsystem.h>
#pragma comment (lib, "winmm.lib")

#include "input.h"

ClipCursor_pfn ClipCursor_Original = nullptr;

void
AD_ComputeAspectCoeffsEx (float& x, float& y, float& xoff, float& yoff, bool force=false)
{
  yoff = 0.0f;
  xoff = 0.0f;

  x = 1.0f;
  y = 1.0f;

  if (! (config.render.aspect_correction || force))
    return;

  config.render.aspect_ratio = ad::RenderFix::width / ad::RenderFix::height;
  float rescale = ((16.0f / 9.0f) / config.render.aspect_ratio);

  // Wider
  if (config.render.aspect_ratio > (16.0f / 9.0f)) {
    int width = (16.0f / 9.0f) * ad::RenderFix::height;
    int x_off = (ad::RenderFix::width - width) / 2;

    x    = (float)ad::RenderFix::width / (float)width;
    xoff = x_off;

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
ad::InputManager::CalcCursorPos (LPPOINT pPoint, bool reverse)
{
  // Bail-out early if aspect ratio correction is disabled, or if the
  //   aspect ratio is less than or equal to 16:9.
  if  (! ( config.render.aspect_correction &&
           config.render.aspect_ratio > (16.0f / 9.0f) ) )
    return *pPoint;

  float xscale, yscale;
  float xoff,   yoff;

  AD_ComputeAspectCoeffsEx (xscale, yscale, xoff, yoff);

  if (! config.render.center_ui) {
    xscale = 1.0f;
    xoff   = 0.0f;
  }

  // Adjust system coordinates to game's (broken aspect ratio) coordinates
  if (! reverse) {
    pPoint->x = ((float)pPoint->x - xoff) * xscale;
    pPoint->y = ((float)pPoint->y - yoff) * xscale;
  }

  // Adjust game's (broken aspect ratio) coordinates to system coordinates
  else {
    pPoint->x = ((float)pPoint->x / xscale) + xoff;
    pPoint->y = ((float)pPoint->y / xscale) + yoff;
  }

  return *pPoint;
}

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
      if (cbData == sizeof (DIMOUSESTATE) || cbData == sizeof (DIMOUSESTATE2)) {
//
// This is only for mouselook, etc. That stuff works fine without aspect ratio correction.
//
//#define FIX_DINPUT8_MOUSE
#ifdef FIX_DINPUT8_MOUSE
        POINT mouse_pos { ((DIMOUSESTATE *)lpvData)->lX,
                          ((DIMOUSESTATE *)lpvData)->lY };

        ad::InputManager::CalcCursorPos (&mouse_pos);

        ((DIMOUSESTATE *)lpvData)->lX = mouse_pos.x;
        ((DIMOUSESTATE *)lpvData)->lY = mouse_pos.y;
#endif
      }

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
  if (ad::InputManager::Hooker::getInstance ()->isVisible ())
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
  if (ad::InputManager::Hooker::getInstance ()->isVisible ()) {
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

    ad::InputManager::CalcCursorPos (&pt);

    pci->ptScreenPos.x = pt.x;
    pci->ptScreenPos.y = pt.y;
  }

  return ret;
}

typedef BOOL (WINAPI *SetCursorPos_pfn)
  (_In_ int X,
   _In_ int Y);

SetCursorPos_pfn SetCursorPos_Original = nullptr;

BOOL
WINAPI
SetCursorPos_Detour (_In_ int X, _In_ int Y)
{
  POINT pt { X, Y };
  ad::InputManager::CalcCursorPos (&pt);

  return SetCursorPos_Original (pt.x, pt.y);
}

typedef BOOL (WINAPI *GetCursorPos_pfn)
  (_Out_ LPPOINT lpPoint);

GetCursorPos_pfn GetCursorPos_Original = nullptr;

BOOL
WINAPI
GetCursorPos_Detour (LPPOINT lpPoint)
{
  BOOL ret = GetCursorPos_Original (lpPoint);

  // Correct the cursor position for Aspect Ratio
  if (config.render.aspect_correction && config.render.aspect_ratio > (16.0f / 9.0f))
    ad::InputManager::CalcCursorPos (lpPoint);

  return ret;
}


void
ad::InputManager::Init (void)
{
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

  AD_CreateDLLHook ( L"user32.dll", "GetCursorInfo",
                        GetCursorInfo_Detour,
              (LPVOID*)&GetCursorInfo_Original );

  AD_CreateDLLHook ( L"user32.dll", "GetCursorPos",
                        GetCursorPos_Detour,
              (LPVOID*)&GetCursorPos_Original );

  AD_CreateDLLHook ( L"user32.dll", "SetCursorPos",
                        SetCursorPos_Detour,
              (LPVOID*)&SetCursorPos_Original );

  ad::InputManager::Hooker* pHook = ad::InputManager::Hooker::getInstance ();

  pHook->Start ();
}

void
ad::InputManager::Shutdown (void)
{
  ad::InputManager::Hooker* pHook = ad::InputManager::Hooker::getInstance ();

  pHook->End ();
}


void
ad::InputManager::Hooker::Start (void)
{
  hMsgPump =
    CreateThread ( NULL,
                     NULL,
                       Hooker::MessagePump,
                         &hooks,
                           NULL,
                             NULL );
}

void
ad::InputManager::Hooker::End (void)
{
  TerminateThread     (hMsgPump, 0);
  UnhookWindowsHookEx (hooks.keyboard);
  UnhookWindowsHookEx (hooks.mouse);
}

void
ad::InputManager::Hooker::Draw (void)
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

DWORD
WINAPI
ad::InputManager::Hooker::MessagePump (LPVOID hook_ptr)
{
  hooks_s* pHooks = (hooks_s *)hook_ptr;

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
  //   we have an actual window!
  ad::WindowManager::Init ();

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

LRESULT
CALLBACK
ad::InputManager::Hooker::MouseProc (int nCode, WPARAM wParam, LPARAM lParam)
{
  MOUSEHOOKSTRUCT* pmh = (MOUSEHOOKSTRUCT *)lParam;

  return CallNextHookEx (Hooker::getInstance ()->hooks.mouse, nCode, wParam, lParam);
}

LRESULT
CALLBACK
ad::InputManager::Hooker::KeyboardProc (int nCode, WPARAM wParam, LPARAM lParam)
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
          eTB_CommandResult result = SK_GetCommandProcessor ()->ProcessCommandLine (text+1);

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
      eTB_CommandProcessor* pCommandProc = SK_GetCommandProcessor ();

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
          pCommandProc->ProcessCommandLine ("AspectCorrection toggle");
        } else if (keys_ [VK_MENU] && vkCode == 'Z' && new_press) {
          pCommandProc->ProcessCommandLine ("CenterUI toggle");
        } else if (vkCode == VK_OEM_COMMA) {
          if (! config.scaling.locked) {
            if (keys_ [VK_MENU])
              config.scaling.mouse_y_offset -= 0.1f;
            else
              config.scaling.hud_x_offset -= 0.01f;
          }
        } else if (vkCode == VK_OEM_PERIOD) {
          if (! config.scaling.locked) {
            if (keys_ [VK_MENU])
              config.scaling.mouse_y_offset += 0.1f;
            else
              config.scaling.hud_x_offset += 0.01f;
          }
        } else if (keys_ [VK_MENU] && vkCode == VK_SPACE && new_press) {
          config.scaling.locked = (! config.scaling.locked);

          if (! config.scaling.locked)
            config.scaling.auto_calc = false;
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
          pCommandProc->ProcessCommandLine ("FramesToTrace 1");
          pCommandProc->ProcessCommandLine ("TraceFrame true");
        } else if (keys_ [VK_MENU] && vkCode == 'D' && new_press) {
          pCommandProc->ProcessCommandLine ("FixDOF toggle");
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
          pCommandProc->ProcessCommandLine ("Render.AllowBG toggle");
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

  return CallNextHookEx (Hooker::getInstance ()->hooks.keyboard, nCode, wParam, lParam);
};


void
AD_DrawCommandConsole (void)
{
  static int draws = 0;

  // Skip the first frame, so that the console appears below the
  //  other OSD.
  if (draws++ > 20) {
    ad::InputManager::Hooker* pHook = ad::InputManager::Hooker::getInstance ();
    pHook->Draw ();
  }
}


ad::InputManager::Hooker* ad::InputManager::Hooker::pInputHook;

char                      ad::InputManager::Hooker::text [4096];

BYTE                      ad::InputManager::Hooker::keys_ [256]    = { 0 };
bool                      ad::InputManager::Hooker::visible        = false;

bool                      ad::InputManager::Hooker::command_issued = false;
std::string               ad::InputManager::Hooker::result_str;

ad::InputManager::Hooker::command_history_s
                          ad::InputManager::Hooker::commands;