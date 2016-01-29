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
#ifndef __AD__HUD_H__
#define __AD__HUD_H__

struct ad_hud_render_task_s {
  bool drawing = false;

  enum {
    TASK_MINIMAP       = 0x1, // Minimap
    TASK_NAMETAGS      = 0x2, // Names and Quest Markers
    TASK_CONTROL_HINTS = 0x4  // Button Icons
  } type;

  virtual void reset (void) = 0;
};

struct ad_hud_state_s {
  bool                  visible;
  ad_hud_render_task_s* task;    // Changed during rendering to reflect the
                                 //   current part of the HUD being rendered.
};

#endif /* __AD__HUD_H__ */
