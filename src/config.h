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
#ifndef __AD__CONFIG_H__
#define __AD__CONFIG_H__

#include <Windows.h>
#include <string>

extern std::wstring AD_VER_STR;

struct ad_config_s
{
  struct {
    bool     aspect_correction = true;
    float    aspect_ratio      = (16.0f / 9.0f);
    bool     center_ui         = true;
    bool     fix_minimap       = true;
    bool     allow_background  = true;
    float    foreground_fps    =  0.0f; // Unlimited
    float    background_fps    = 15.0f;
  } render;

  struct {
    float    mouse_y_offset    = 0.0f;
    float    hud_x_offset      = 0.0f;
    bool     auto_calc         = true;
    bool     locked            = true;
  } scaling;

  struct {
    bool     aspect_correct    = false;
    int      always_on_top     = 2;
    bool     temp_on_top       = false; // When the key combo is held
    uint16_t
             hold_on_top_key   = (uint16_t)'N';
    uint16_t
             toggle_on_top_key = (uint16_t)'N';
  } nametags;

  struct {
    int  num_frames = 0;
    bool shaders    = false;
    bool ui         = false; // General-purpose UI stuff
    bool hud        = false;
    bool menus      = false;
    bool minimap    = false;
    bool nametags   = false;
  } trace;

  struct {
    bool block_left_alt  = false;
    bool block_left_ctrl = false;
  } keyboard;

  struct {
    std::wstring
            version            = AD_VER_STR;
    std::wstring
            injector           = L"d3d9.dll";
  } system;
};

extern ad_config_s config;

bool AD_LoadConfig (std::wstring name         = L"AgDrag");
void AD_SaveConfig (std::wstring name         = L"AgDrag",
                    bool         close_config = false);

#endif __AD__CONFIG_H__