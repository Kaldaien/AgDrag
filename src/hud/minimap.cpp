#include <stdint.h>

#include "minimap.h"
#include "../hook.h"

ad_minimap_s* minimap = nullptr;

typedef DWORD (__stdcall *uGUIMap_draw_pfn)(DWORD dwUnknown);
uGUIMap_draw_pfn uGUIMap_draw_Original = nullptr;

DWORD
__stdcall
uGUIMap_draw_Detour (DWORD dwUnknown)
{
  minimap->main_map = true;
  minimap->drawing  = true;

#if 0
  __asm {
    pushad
    pushfd
 

  //minimap->finished = false;
  //minimap->drawing  = true;

  //dll_log.Log (L" -- Begin uGUIMap::draw (...) [%08Xh] -- ", dwUnknown);

  __asm {
    popfd
    popad
  }
#endif

  return uGUIMap_draw_Original (dwUnknown);

  //minimap->drawing  = false;
  //minimap->main_map = false;
  //minimap->finished = true;
}

void
ad_minimap_s::notifyShaderChange ( uint32_t vs_crc32,
                                   uint32_t ps_crc32,
                                   bool     pixel )
{
  if (drawing && pixel) {
    ++shader_changes;
    if (! main_map) {
      if (shader_changes > 2) {
        finished = true;
        drawing  = false;
      }
    } else {
      if (ps_crc32 == 0xf88d8bcd) {
#if 0
        if (tracer.log_frame && config.trace.minimap) {
          dll_log.Log ( L" Ending map menu draw <%f,%f,%f> (vs: %x, ps: %x)",
                          minimap->prim_xpos,
                            minimap->prim_ypos,
                              minimap->prim_zpos,
                                vs_checksum,
                                  ps_checksum );
          }
#endif
        main_map = false;
        finished = true;
        drawing  = false;
      }
    }
  }
}

void
ad_minimap_s::init (void)
{
  //
  // TODO: Signature scan this
  //
  AD_CreateFuncHook (L"uGUIMap::draw (...)", (LPVOID)0x0068BA30, uGUIMap_draw_Detour, (LPVOID *)&uGUIMap_draw_Original );
  AD_EnableHook ((LPVOID)0x0068BA30);
}

void
ad_minimap_s::reset (void)
{
  drawing        = false;
  finished       = false;
  main_map       = false;
  shader_changes = 0;
  prims_drawn    = 0;
  prim_xpos      = 0.0f;
  prim_ypos      = 0.0f;
  prim_zpos      = 0.0f;
  ps23           = 0.0f;
  ps43           = 0.0f;
  center_prim    = false;
}