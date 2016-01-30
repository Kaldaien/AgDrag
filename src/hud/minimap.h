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
#ifndef __AD__HUD_MINIMAP_H__
#define __AD__HUD_MINIMAP_H__

#include "../hud.h"

// The minimap is 3 shaders, basically... so once triggered
//   we will continue to modify the viewport coordinates until
//     2 shader changes elapse.
struct ad_minimap_s : ad_hud_render_task_s {
  int   shader_changes = 0;
  int   prims_drawn    = 0;
  float prim_xpos      = 0.0f;
  float prim_ypos      = 0.0f;
  float prim_zpos      = 0.0f;

  float ps23           = 0.0f;
  float ps43           = 0.0f;

  bool  center_prim    = false; // The triangle in the middle

  void reset (void);
} extern *minimap;

#endif /* __AD__HUD_MINIMAP_H__ */
