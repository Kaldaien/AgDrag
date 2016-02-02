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

#ifndef __AD__INPUT_H__
#define __AD__INPUT_H__

#include "command.h"

namespace ad
{
  namespace InputManager
  {
    void Init     ();
    void Shutdown ();

    class Hooker {
    private:
      HANDLE                  hMsgPump;

      struct hooks_s {
        HHOOK                 keyboard;
        HHOOK                 mouse;
      } hooks;

      static Hooker*  pInputHook;

      static char             text [4096];

      static BYTE             keys_ [256];
      static bool             visible;

      static bool             command_issued;
      static std::string      result_str;

      struct command_history_s {
        std::vector <std::string> history;
        int_fast32_t              idx     = -1;
      } static commands;

    protected:
      Hooker (void) { }

    public:
      static Hooker* getInstance (void)
      {
        if (pInputHook == NULL)
          pInputHook = new Hooker ();

        return pInputHook;
      }

      bool isVisible (void) { return visible; }

      void Start (void);
      void End   (void);

      void Draw  (void);

      HANDLE GetThread (void) {
        return hMsgPump;
      }

      static DWORD
        WINAPI
        MessagePump (LPVOID hook_ptr);

      static LRESULT
        CALLBACK
        MouseProc (int nCode, WPARAM wParam, LPARAM lParam);

      static LRESULT
        CALLBACK
        KeyboardProc (int nCode, WPARAM wParam, LPARAM lParam);
    };

    //
    // Returns the original cursor position and stores the new one in pPoint
    //
    //   * If reverse is true, this will translate the game's coordinates to system
    POINT
    CalcCursorPos (LPPOINT pPoint, bool reverse = false);
  }
}

typedef BOOL (WINAPI *ClipCursor_pfn)(
  _In_opt_ const RECT *lpRect
);

extern ClipCursor_pfn ClipCursor_Original;

#endif /* __AD__INPUT_H__ */