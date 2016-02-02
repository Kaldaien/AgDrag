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

#include "config.h"
#include "parameter.h"
#include "ini.h"
#include "log.h"

std::wstring AD_VER_STR = L"0.2.0";

static ad::INI::File*  dll_ini = nullptr;

ad_config_s config;

ad::ParameterFactory g_ParameterFactory;

struct {
  ad::ParameterBool*    aspect_correction;
  ad::ParameterBool*    center_ui;
  ad::ParameterBool*    allow_background;
  ad::ParameterFloat*   foreground_fps;
  ad::ParameterFloat*   background_fps;
} render;

struct {
  ad::ParameterFloat*   mouse_y_offset;
  ad::ParameterFloat*   hud_x_offset;
  ad::ParameterBool*    locked;
  ad::ParameterBool*    auto_calc;
} scaling;

struct {
  ad::ParameterInt*     always_on_top;
  ad::ParameterBool*    aspect_correct;
  ad::ParameterStringW* hold_on_top_key;
  ad::ParameterStringW* toggle_on_top_key;
} nametags;

struct {
  ad::ParameterBool*    block_left_alt;
  ad::ParameterBool*    block_left_ctrl;
} keyboard;

struct {
  ad::ParameterStringW* injector;
  ad::ParameterStringW* version;
} sys;


bool
AD_LoadConfig (std::wstring name) {
  // Load INI File
  std::wstring full_name = name + L".ini";
  dll_ini = new ad::INI::File ((wchar_t *)full_name.c_str ());

  bool empty = dll_ini->get_sections ().empty ();

  //
  // Create Parameters
  //
  render.aspect_correction =
    static_cast <ad::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Aspect Ratio Correct EVERYTHING")
      );
  render.aspect_correction->register_to_ini (
    dll_ini,
      L"AgDrag.Render",
        L"AspectCorrection" );

  render.center_ui =
    static_cast <ad::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Center the UI")
      );
  render.center_ui->register_to_ini (
    dll_ini,
      L"AgDrag.Render",
        L"CenterUI" );

  render.allow_background =
    static_cast <ad::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Allow background rendering")
      );
  render.allow_background->register_to_ini (
    dll_ini,
      L"AgDrag.Render",
        L"AllowBackground" );

  render.foreground_fps =
    static_cast <ad::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Foreground Framerate Limit")
      );
  render.foreground_fps->register_to_ini (
    dll_ini,
      L"AgDrag.Render",
        L"ForegroundFPS" );

  render.background_fps =
    static_cast <ad::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Background Framerate Limit")
      );
  render.background_fps->register_to_ini (
    dll_ini,
      L"AgDrag.Render",
        L"BackgroundFPS" );


  scaling.mouse_y_offset = 
    static_cast <ad::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Mouse Coordinate Y Offset")
      );
  scaling.mouse_y_offset->register_to_ini (
    dll_ini,
      L"AgDrag.Scaling",
        L"MouseYOffset" );

  scaling.hud_x_offset = 
    static_cast <ad::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"HUD Coordinate X Offset")
      );
  scaling.hud_x_offset->register_to_ini (
    dll_ini,
      L"AgDrag.Scaling",
        L"HUDXOffset" );

  scaling.locked = 
    static_cast <ad::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Scale Lock")
      );
  scaling.locked->register_to_ini (
    dll_ini,
      L"AgDrag.Scaling",
        L"Locked" );

  scaling.auto_calc = 
    static_cast <ad::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Scale Lock")
      );
  scaling.auto_calc->register_to_ini (
    dll_ini,
      L"AgDrag.Scaling",
        L"AutoCalc" );


  nametags.aspect_correct =
    static_cast <ad::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Aspect Ratio Correct Nametags")
      );
  nametags.aspect_correct->register_to_ini (
    dll_ini,
      L"AgDrag.Nametags",
        L"AspectCorrect" );

  nametags.always_on_top =
    static_cast <ad::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Don't let the world occlude names")
      );
  nametags.always_on_top->register_to_ini (
    dll_ini,
      L"AgDrag.Nametags",
        L"AlwaysOnTop" );


  keyboard.block_left_alt =
    static_cast <ad::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Block Left Alt Key")
      );
  keyboard.block_left_alt->register_to_ini (
    dll_ini,
      L"AgDrag.Keyboard",
        L"BlockLeftAlt" );

  keyboard.block_left_ctrl =
    static_cast <ad::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Block Left Ctrl Key")
      );
  keyboard.block_left_ctrl->register_to_ini (
    dll_ini,
      L"AgDrag.Keyboard",
        L"BlockLeftCtrl" );


  sys.version =
    static_cast <ad::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Software Version")
      );
  sys.version->register_to_ini (
    dll_ini,
      L"AgDrag.System",
        L"Version" );

  sys.injector =
    static_cast <ad::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Injector DLL")
      );
  sys.injector->register_to_ini (
    dll_ini,
      L"AgDrag.System",
        L"Injector" );


  //
  // Load Parameters
  //
  if (render.aspect_correction->load ())
    config.render.aspect_correction = render.aspect_correction->get_value ();

  if (render.center_ui->load ())
    config.render.center_ui = render.center_ui->get_value ();

  if (render.allow_background->load ())
    config.render.allow_background = render.allow_background->get_value ();

  if (render.foreground_fps->load ())
    config.render.foreground_fps = render.foreground_fps->get_value ();

  if (render.background_fps->load ())
    config.render.background_fps = render.background_fps->get_value ();


  if (scaling.hud_x_offset->load ())
    config.scaling.hud_x_offset = scaling.hud_x_offset->get_value ();

  if (scaling.mouse_y_offset->load ())
    config.scaling.mouse_y_offset = scaling.mouse_y_offset->get_value ();

  if (scaling.locked->load ())
    config.scaling.locked = scaling.locked->get_value ();

  if (scaling.auto_calc->load ())
    config.scaling.auto_calc = scaling.auto_calc->get_value ();


  if (nametags.aspect_correct->load ())
    config.nametags.aspect_correct = nametags.aspect_correct->get_value ();

  if (nametags.always_on_top->load ())
    config.nametags.always_on_top = nametags.always_on_top->get_value ();


  if (keyboard.block_left_alt->load ())
    config.keyboard.block_left_alt = keyboard.block_left_alt->get_value ();

  if (keyboard.block_left_ctrl->load ())
    config.keyboard.block_left_ctrl = keyboard.block_left_ctrl->get_value ();


  if (sys.version->load ())
    config.system.version = sys.version->get_value ();

  if (sys.injector->load ())
    config.system.injector = sys.injector->get_value ();

  if (empty)
    return false;

  return true;
}

void
AD_SaveConfig (std::wstring name, bool close_config) {
  render.aspect_correction->set_value (config.render.aspect_correction);
  render.aspect_correction->store     ();

  render.center_ui->set_value         (config.render.center_ui);
  render.center_ui->store             ();

  render.allow_background->set_value  (config.render.allow_background);
  render.allow_background->store      ();

  render.foreground_fps->set_value    (config.render.foreground_fps);
  render.foreground_fps->store        ();

  render.background_fps->set_value    (config.render.background_fps);
  render.background_fps->store        ();


  if (! config.scaling.locked) {
    scaling.mouse_y_offset->set_value   (config.scaling.mouse_y_offset);
    scaling.mouse_y_offset->store       ();

    scaling.hud_x_offset->set_value     (config.scaling.hud_x_offset);
    scaling.hud_x_offset->store         ();
  }

  scaling.locked->set_value           (config.scaling.locked);
  scaling.locked->store               ();

  scaling.auto_calc->set_value        (config.scaling.auto_calc);
  scaling.auto_calc->store            ();


  nametags.aspect_correct->set_value  (config.nametags.aspect_correct);
  nametags.aspect_correct->store      ();

  nametags.always_on_top->set_value   (config.nametags.always_on_top);
  nametags.always_on_top->store       ();


  keyboard.block_left_alt->set_value  (config.keyboard.block_left_alt);
  keyboard.block_left_alt->store      ();

  keyboard.block_left_ctrl->set_value (config.keyboard.block_left_ctrl);
  keyboard.block_left_ctrl->store     ();


  sys.version->set_value       (AD_VER_STR);
  sys.version->store           ();

  sys.injector->set_value (config.system.injector);
  sys.injector->store     ();

  dll_ini->write (name + L".ini");

  if (close_config) {
    if (dll_ini != nullptr) {
      delete dll_ini;
      dll_ini = nullptr;
    }
  }
}