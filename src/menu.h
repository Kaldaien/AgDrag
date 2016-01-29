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
#ifndef __AD__MENU_H__
#define __AD__MENU_H__

struct ad_menu_state_s {
  enum {
    MENU_TITLE         =   0x1,
    MENU_PAUSE         =   0x2,
    MENU_SAVE          =   0x4,
    MENU_MAP           =   0x8,
    MENU_OPTIONS       =  0x10,
    MENU_STATUS        =  0x20,
    MENU_HISTORY       =  0x40,
    MENU_EQUIP         =  0x80,
    MENU_INVENTORY     = 0x100,
    MENU_PAWN_RECRUIT  = 0x200
  } type;
};

#endif /* __AD__MENU_H__ */