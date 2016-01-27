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

#ifndef __AD__RENDER_H__
#define __AD__RENDER_H__

#include "command.h"

struct IDirect3DDevice9;
struct IDirect3DSurface9;

#include <Windows.h>

namespace ad
{
  namespace RenderFix
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
      eTB_Variable* aspect_ratio_;
      eTB_Variable* center_ui_;

    private:
      static CommandProcessor* pCommProc;
    };

    extern uint32_t           width;
    extern uint32_t           height;

    extern IDirect3DDevice9*  pDevice;
    extern HWND               hWndDevice;

    extern uint32_t           dwRenderThreadID;

    extern HMODULE            d3dx9_43_dll;
    extern HMODULE            user32_dll;
  }
}

#endif /* __AD__RENDER_H__ */