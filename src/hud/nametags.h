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
#ifndef __AD__HUD_NAMETAGS_H__
#define __AD__HUD_NAMETAGS_H__

#include <stdint.h>

#include "../hud.h"

struct IDirect3DDevice9;

struct ad_nametag_phase_s {
  enum {
    PHASE_QUEST = 0x1,
    PHASE_NAMES = 0x2,
    PHASE_OTHER = 0x4 // Escort Missions ?
  } type;
};

struct ad_nametags_s : ad_hud_render_task_s {
  ad_nametag_phase_s* phase;
  bool                finished;      // Finished drawing names for the current frame
  int                 top_technique; // Mode for drawing nametags on top

  struct {
    uint32_t ps_crc32;
    uint32_t vs_crc32;
  } shader;

  struct {
    float x, y, z;
  } location;

  void beginPrimitive (IDirect3DDevice9* pDev);
  void endPrimitive   (IDirect3DDevice9* pDev);

  bool shouldDrawOnTop     (void);
  bool shouldAspectCorrect (void);

  enum test_result {
    NAMETAGS_BEGIN,
    NAMETAGS_END,
    NAMETAGS_UNKNOWN
  };

  test_result trigger (float last_z, float y, float z, float w, float zz);

  void reset (void);
} extern *nametags;

#endif /* __AD__HUD_NAMETAGS_H__ */
