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

#include "gamestate.h"

#include "hud/minimap.h"
#include "hud/nametags.h"

ad_gamestate_s* game;

bool
ad_gamestate_s::init (void)
{
  if (game == nullptr) {
    game = new ad_gamestate_s ();

    game->hud.task    = nullptr;
    game->hud.visible = false;

    minimap       = new ad_minimap_s ();
    minimap->type = ad_hud_render_task_s::TASK_MINIMAP;
    minimap->init ();

    nametags       = new ad_nametags_s ();
    nametags->type = ad_hud_render_task_s::TASK_NAMETAGS;
    nametags->init ();

    game->menu = nullptr;

    return true;
  }

  return false;
}