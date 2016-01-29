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

std::wstring AD_VER_STR = L"0.1.0";

static ad::INI::File*  dll_ini = nullptr;

ad_config_s config;

ad::ParameterFactory g_ParameterFactory;

struct {
  ad::ParameterBool*    aspect_correction;
  ad::ParameterBool*    center_ui;
} render;

struct {
  ad::ParameterInt*     always_on_top;
  ad::ParameterBool*    aspect_correct;
  ad::ParameterStringW* hold_on_top_key;
  ad::ParameterStringW* toggle_on_top_key;
} nametags;

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


  if (nametags.aspect_correct->load ())
    config.nametags.aspect_correct = nametags.aspect_correct->get_value ();

  if (nametags.always_on_top->load ())
    config.nametags.always_on_top = nametags.always_on_top->get_value ();


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


  nametags.aspect_correct->set_value  (config.nametags.aspect_correct);
  nametags.aspect_correct->store      ();

  nametags.always_on_top->set_value   (config.nametags.always_on_top);
  nametags.always_on_top->store       ();


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