/*
 * MODIFIED in 2026 by EveryNightLive (unofficial 32:9 update):
 *   32:9 support, compatibility with the Dec/2017 DDDA.exe and recent
 *   Special K versions, full map / menu / HUD fixes, build fixes.
 *   Changes are marked with "FIX" comments. See README.md.
 */

#include <stdint.h>
#include <Windows.h>
#include <cstring>

#include "minimap.h"
#include "../hook.h"
#include "../config.h"
#include "../log.h"
#include "../render.h"

ad_minimap_s* minimap = nullptr;

// FIX (2026): Full map detection without a fixed address.
//
//   The original hook used the fixed address 0x0068BA30 (2016 DDDA.exe). In
//   the current executable (Dec 2017) that address lands in the MIDDLE of an
//   instruction and the game crashed when opening the map.
//
//   We now locate the uGUIMap class at runtime through the MT Framework type
//   system (MtDTI) and hook entry 55 of its vtable: a __thiscall method
//   with no parameters that runs every frame ONLY while the map menu is
//   open. The hook only records "the map is open"; the rendering code
//   decides whether the current draw is the full map when it detects the
//   minimap shader (see render.cpp).
//
//   __fastcall receives ECX ("this") and EDX as the first two parameters and
//   passes them untouched to the original (compatible with a no-arg __thiscall).
static const int AD_UGUIMAP_UPDATE_SLOT = 55;

typedef void (__fastcall *uGUIMap_update_pfn)(void* This, void* edx);
static uGUIMap_update_pfn uGUIMap_update_Original = nullptr;

static void
__fastcall
uGUIMap_update_Detour (void* This, void* edx)
{
  if (minimap != nullptr)
    minimap->map_menu_tick = GetTickCount ();

  uGUIMap_update_Original (This, edx);
}

bool
ad_minimap_s::isMapMenuActive (void)
{
  // The update runs in the game loop and drawing right after; a 250 ms margin
  //   covers FPS drops and the frame in which the menu closes.
  return map_menu_tick != 0 && (GetTickCount () - map_menu_tick) < 250;
}

//
// Simple byte search within a memory range of the module itself
//
static uint8_t*
AD_FindBytes (uint8_t* begin, size_t len, const uint8_t* pat, size_t pat_len)
{
  if (len < pat_len)
    return nullptr;

  for (size_t i = 0; i <= len - pat_len; i++) {
    if (begin [i] == pat [0] && memcmp (begin + i, pat, pat_len) == 0)
      return begin + i;
  }

  return nullptr;
}

static bool
AD_GetSection (HMODULE mod, const char* name, uint8_t** start, size_t* size)
{
  PIMAGE_DOS_HEADER     dos = (PIMAGE_DOS_HEADER)mod;
  PIMAGE_NT_HEADERS     nt  = (PIMAGE_NT_HEADERS)((uint8_t *)mod + dos->e_lfanew);
  PIMAGE_SECTION_HEADER sec = IMAGE_FIRST_SECTION (nt);

  for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
    if (strncmp ((const char *)sec->Name, name, IMAGE_SIZEOF_SHORT_NAME) == 0) {
      *start = (uint8_t *)mod + sec->VirtualAddress;
      *size  = sec->Misc.VirtualSize;
      return true;
    }
  }

  return false;
}

//
// Locates the uGUIMap vtable:
//   1. string "uGUIMap\0" in .rdata (name registered in the MtDTI)
//   2. in .text: "push <string>" followed by "mov ecx, <MtDTI object>"
//      (static constructor of the class's MtDTI)
//   3. in .text: "mov eax, <MtDTI>; ret" = virtual method getDTI()
//   4. in .rdata: pointer to getDTI() = vtable entry 4
//
void**
AD_FindUGUIMapVTable (HMODULE exe)
{
  uint8_t *text,  *rdata;
  size_t   text_len, rdata_len;

  if (! (AD_GetSection (exe, ".text",  &text,  &text_len) &&
         AD_GetSection (exe, ".rdata", &rdata, &rdata_len))) {
    dll_log.Log (L" [Minimap] Could not read executable sections");
    return nullptr;
  }

  static const uint8_t name [] = "uGUIMap"; // includes the trailing '\0'

  uint32_t dti = 0;

  for (size_t off = 1; off + sizeof (name) <= rdata_len && dti == 0; off++) {
    uint8_t* str = AD_FindBytes (rdata + off, rdata_len - off, name, sizeof (name));

    if (str == nullptr)
      break;

    off = (size_t)(str - rdata);

    if (str [-1] != 0) // must be the start of a string
      continue;

    uint8_t push_str [5] = { 0x68 };
    uint32_t str_va      = (uint32_t)(uintptr_t)str;
    memcpy (&push_str [1], &str_va, 4);

    uint8_t* scan     = text;
    size_t   scan_len = text_len;

    while (dti == 0) {
      uint8_t* hit = AD_FindBytes (scan, scan_len, push_str, 5);

      if (hit == nullptr)
        break;

      for (int k = 5; k < 5 + 16 && (hit + k + 5) <= (text + text_len); k++) {
        if (hit [k] == 0xB9) { // mov ecx, imm32
          memcpy (&dti, &hit [k + 1], 4);
          break;
        }
      }

      scan_len -= (size_t)(hit + 1 - scan);
      scan      = hit + 1;
    }
  }

  if (dti == 0) {
    dll_log.Log (L" [Minimap] uGUIMap type descriptor (MtDTI) not found");
    return nullptr;
  }

  uint8_t getter_code [6] = { 0xB8, 0, 0, 0, 0, 0xC3 };
  memcpy (&getter_code [1], &dti, 4);

  uint8_t* getter = AD_FindBytes (text, text_len, getter_code, 6);

  if (getter == nullptr) {
    dll_log.Log (L" [Minimap] uGUIMap::getDTI not found (MtDTI=%08X)", dti);
    return nullptr;
  }

  uint32_t getter_va = (uint32_t)(uintptr_t)getter;

  for (size_t off = 0; off + 4 <= rdata_len; off += 4) {
    if (memcmp (rdata + off, &getter_va, 4) == 0) {
      void** vtable = (void **)(rdata + off) - 4; // getDTI is entry 4

      dll_log.Log ( L" [Minimap] uGUIMap: MtDTI=%08X getDTI=%08X vtable=%08X",
                      dti, getter_va, (uint32_t)(uintptr_t)vtable );
      return vtable;
    }
  }

  dll_log.Log (L" [Minimap] uGUIMap vtable not found (getDTI=%08X)", getter_va);
  return nullptr;
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

//
// FIX (2026): "Screen height" as seen by the full map code.
//
//   uGUIMap reads the screen height (field +0xBC of a global object) in 8
//   places and computes, for example, (height - width * 9/16) / 2 to fit
//   a 16:9 layout. On 32:9 the map ends up with half the height relative
//   to the rest of the UI. We make the drawing-related reads (and ONLY those)
//   read this variable, which is width * 9/16 in MapMode=1 - so the map
//   behaves as on a 16:9 screen, like the rest of the UI, and fills the whole
//   central area with the cursor and icons aligned.
//
//   Addresses are from the Dec 2017 DDDA.exe; every instruction is verified
//   byte by byte before patching (if the game changes, nothing is patched).
//
volatile int ad_map_screen_height = 0; // 0 = not computed yet
static   bool ad_map_height_patched = false;

// With the patch active the map itself already fits the central 16:9 band;
//   the mod must not shrink the viewport width again (see render.cpp).
bool
AD_IsMapHeightPatchActive (void)
{
  return ad_map_height_patched &&
         ad_map_screen_height > 0 &&
         ad_map_screen_height != (int)ad::RenderFix::height;
}

void
AD_UpdateMapScreenHeight (void)
{
  int w = (int)ad::RenderFix::width;
  int h = (int)ad::RenderFix::height;

  if (w <= 0 || h <= 0)
    return;

  int fake = h;

  if (config.render.map_height_patch && config.render.map_mode == 1 &&
      (float)w / (float)h > (16.0f / 9.0f) + 0.01f)
    fake = (w * 9) / 16;

  ad_map_screen_height = fake;
}

struct ad_map_height_site_s {
  uint32_t addr;
  uint8_t  orig [8];
  int      len;     // 6 = mov r32,[r+0BCh] / 8 = movd xmm,[r+0BCh]
  bool     patch;   // false = verified only (cursor-related read)
};

// The 3 reads marked "false" convert the cursor position between the screen
//   and the map (and the "cursor inside the map" rectangle). They need the
//   REAL height, the same one the game's input system uses; with the fake
//   height the cursor was offset and could not reach the bottom edge (no scrolling).
static const ad_map_height_site_s ad_map_height_sites [] = {
  { 0x00681597, { 0x8B, 0x80, 0xBC, 0x00, 0x00, 0x00             }, 6, true  },
  { 0x0068230B, { 0x8B, 0x80, 0xBC, 0x00, 0x00, 0x00             }, 6, true  },
  { 0x00686FA3, { 0x66, 0x0F, 0x6E, 0x80, 0xBC, 0x00, 0x00, 0x00 }, 8, false }, // cursor vs. window
  { 0x0068769D, { 0x8B, 0x80, 0xBC, 0x00, 0x00, 0x00             }, 6, true  },
  { 0x0068EA4F, { 0x8B, 0x89, 0xBC, 0x00, 0x00, 0x00             }, 6, true  },
  { 0x0068F30B, { 0x8B, 0x92, 0xBC, 0x00, 0x00, 0x00             }, 6, true  },
  { 0x006927A1, { 0x66, 0x0F, 0x6E, 0x81, 0xBC, 0x00, 0x00, 0x00 }, 8, false }, // map -> screen (cursor)
  { 0x006928FE, { 0x8B, 0x92, 0xBC, 0x00, 0x00, 0x00             }, 6, false }, // screen -> map (cursor)
};

static void
AD_PatchMapScreenHeight (void)
{
  if (! config.render.map_height_patch) {
    dll_log.Log (L" [Minimap] Map height patch DISABLED by INI (MapHeightPatch=false)");
    return;
  }

  const int count = sizeof (ad_map_height_sites) / sizeof (ad_map_height_sites [0]);

  // Verify ALL of them before patching any
  for (int i = 0; i < count; i++) {
    const ad_map_height_site_s& s = ad_map_height_sites [i];

    if (memcmp ((void *)(uintptr_t)s.addr, s.orig, s.len) != 0) {
      dll_log.Log ( L" [Minimap] Map height patch: unexpected code at %08X; "
                    L"game version differs, NOT patching.", s.addr );
      return;
    }
  }

  // Safe initial value until the swapchain is captured: the real height
  if (ad_map_screen_height == 0) {
    uint32_t* pScreen = *(uint32_t **)(uintptr_t)0x018D08F4;

    if (pScreen != nullptr)
      ad_map_screen_height = *(int *)((uint8_t *)pScreen + 0xBC);

    if (ad_map_screen_height <= 0)
      ad_map_screen_height = 720;
  }

  uint32_t var_addr = (uint32_t)(uintptr_t)&ad_map_screen_height;

  int patched = 0;

  for (int i = 0; i < count; i++) {
    const ad_map_height_site_s& s = ad_map_height_sites [i];

    if (! s.patch)
      continue;

    ++patched;

    uint8_t  code [8];
    memcpy (code, s.orig, s.len);

    int modrm = (s.len == 6) ? 1 : 3;
    code [modrm] = (uint8_t)((code [modrm] & 0x38) | 0x05); // [disp32]
    memcpy (&code [modrm + 1], &var_addr, 4);

    DWORD dwOld;
    VirtualProtect ((void *)(uintptr_t)s.addr, s.len, PAGE_EXECUTE_READWRITE, &dwOld);
    memcpy ((void *)(uintptr_t)s.addr, code, s.len);
    VirtualProtect ((void *)(uintptr_t)s.addr, s.len, dwOld, &dwOld);
  }

  FlushInstructionCache (GetCurrentProcess (), nullptr, 0);

  ad_map_height_patched = true;

  dll_log.Log ( L" [Minimap] Map height patch applied (%d of %d sites)",
                  patched, count );
}

void
ad_minimap_s::init (void)
{
  AD_PatchMapScreenHeight ();

  // INI: [AgDrag.Render] HookMap=false disables map detection.
  if (! config.render.hook_map) {
    dll_log.Log (L" [Minimap] Map detection hook DISABLED by INI (HookMap=false)");
    return;
  }

  void** vtable = AD_FindUGUIMapVTable (GetModuleHandle (nullptr));

  if (vtable == nullptr) {
    dll_log.Log (L" [Minimap] Map detection unavailable; the full map will not be corrected.");
    return;
  }

  uint8_t* target = (uint8_t *)vtable [AD_UGUIMAP_UPDATE_SLOT];

  // Sanity check: the target must be inside the executable's code and start
  //   with a normal prologue (push ebp; mov ebp, esp). If the game changes, we
  //   fail safely instead of corrupting code.
  HMODULE  exe = GetModuleHandle (nullptr);
  uint8_t* text;
  size_t   text_len;

  if ( (! AD_GetSection (exe, ".text", &text, &text_len)) ||
       target < text || target >= text + text_len         ||
       target [0] != 0x55 || target [1] != 0x8B || target [2] != 0xEC ) {
    dll_log.Log ( L" [Minimap] vtable[%d] = %08X does not look like the expected function; not hooking.",
                    AD_UGUIMAP_UPDATE_SLOT, (uint32_t)(uintptr_t)target );
    return;
  }

  MH_STATUS st =
    AD_CreateFuncHook ( L"uGUIMap::update (vtable[55])", target,
                          uGUIMap_update_Detour,
                (LPVOID *)&uGUIMap_update_Original );

  dll_log.Log ( L" [Minimap] uGUIMap vtable[%d] = %08X, hook create status: %d",
                  AD_UGUIMAP_UPDATE_SLOT, (uint32_t)(uintptr_t)target, (int)st );

  if (st == MH_OK)
    AD_EnableHook (target);
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