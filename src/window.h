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

#ifndef __AD__WINDOW_H__
#define __AD__WINDOW_H__

#include "command.h"

// State for window activation faking
struct ad_window_state_s {
  bool  active          = true;
  bool  activating      = false;

  DWORD proc_id;
  HWND  hwnd;

  RECT  cursor_clip;

  DWORD style;    // Style before we removed the border
  DWORD style_ex; // StyleEX before removing the border

  bool  borderless = false;
} extern window;

namespace ad
{
  namespace WindowManager
  {
    void Init     ();
    void Shutdown ();

    class CommandProcessor : public eTB_iVariableListener {
    public:
      CommandProcessor (void);

      virtual bool OnVarChange (eTB_Variable* var, void* val = NULL);

      static CommandProcessor* getInstance (void)
      {
        if (pCommProc == NULL)
          pCommProc = new CommandProcessor ();

        return pCommProc;
      }

    protected:
      eTB_Variable* foreground_fps_;
      eTB_VarStub <float>* background_fps_;

    private:
      static CommandProcessor* pCommProc;
    };

    class BorderManager {
    public:
      void Enable  (void);
      void Disable (void);
      void Toggle  (void);
    } static border;
  }
}

#endif /* __AD__WINDOW_H__ */