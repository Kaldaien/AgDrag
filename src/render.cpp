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

#include "render.h"
#include "config.h"
#include "log.h"

#include <stdint.h>

#include <comdef.h>

#include <d3d9.h>
#include <d3d9types.h>

bool needs_aspect       = false;
bool needs_ui_center    = false;
bool scissoring         = false;

// The minimap is 3 shaders, basically... so once triggered
//   we will continue to modify the viewport coordinates until
//     2 shader changes elapse.
struct dd_map_draw_t {
  bool  drawing        = false;
  int   shader_changes = 0;
  int   prims_drawn    = 0;
  float prim_ypos      = 0.0f;
  float prim_xpos      = 0.0f;

  float ps23           = 0.0f;
  float ps43           = 0.0f;

  bool  center_prim    = false; // The triangle in the middle
} map;

D3DVIEWPORT9 viewport;

// Debug hack
bool allow_scissor = true;

bool fix_rect = false;

int  minimap_shader = 0x9a78e585;
bool vert_fix_map   = false;

bool fix_dof        = true;
bool kill_dof       = false;
bool dof_active     = false;


bool log_frame       = false;
int  log_frame_count = 0;

bool fullscreen_blit = false;
int center_count     = 0;


void
AD_ComputeAspectCoeffs (float& x, float& y, float& xoff, float& yoff)
{
  yoff = 0.0f;
  xoff = 0.0f;

  x = 1.0f;
  y = 1.0f;

  if (! (config.render.aspect_correction))
    return;

  config.render.aspect_ratio = ad::RenderFix::width / ad::RenderFix::height;
  float rescale = (1.77777778f / config.render.aspect_ratio);

  // Wider
  if (config.render.aspect_ratio > 1.7777f) {
    int width = (16.0f / 9.0f) * viewport.Height;
    int x_off = (viewport.Width - width) / 2;

    x    = (float)viewport.Width / (float)width;
    xoff = x_off;
  } else {
    int height = (9.0f / 16.0f) * viewport.Width;
    int y_off  = (viewport.Height - height) / 2;

    y    = (float)viewport.Height / (float)height;
    yoff = y_off;
  }
}

#include "hook.h"

IDirect3DVertexShader9* g_pVS;
IDirect3DPixelShader9*  g_pPS;

static uint32_t crc32_tab[] = {
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
  0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
  0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
  0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
  0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
  0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
  0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
  0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
  0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
  0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
  0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
  0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
  0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
  0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
  0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
  0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
  0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
  0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
  0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
  0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
  0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
  0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
  0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
  0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
  0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
  0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
  0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
  0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
  0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
  0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
  0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t
crc32(uint32_t crc, const void *buf, size_t size)
{
  const uint8_t *p;

  p = (uint8_t *)buf;
  crc = crc ^ ~0U;

  while (size--)
    crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

  return crc ^ ~0U;
}

#include <map>
std::unordered_map <LPVOID, uint32_t> vs_checksums;
std::unordered_map <LPVOID, uint32_t> ps_checksums;

// Store the CURRENT shader's checksum instead of repeatedly
//   looking it up in the above hashmaps.
uint32_t vs_checksum = 0;
uint32_t ps_checksum = 0;

typedef HRESULT (STDMETHODCALLTYPE *SetVertexShader_t)
  (IDirect3DDevice9*       This,
   IDirect3DVertexShader9* pShader);

SetVertexShader_t D3D9SetVertexShader_Original = nullptr;

HRESULT
STDMETHODCALLTYPE
D3D9SetVertexShader_Detour (IDirect3DDevice9*       This,
                            IDirect3DVertexShader9* pShader)
{
  // Ignore anything that's not the primary render device.
  if (This != ad::RenderFix::pDevice)
    return D3D9SetVertexShader_Original (This, pShader);

  if (g_pVS != pShader) {
    if (pShader != nullptr) {
      if (vs_checksums.find (pShader) == vs_checksums.end ()) {
        UINT len;
        pShader->GetFunction (nullptr, &len);

        void* pbFunc = malloc (len);

        if (pbFunc != nullptr) {
          pShader->GetFunction (pbFunc, &len);

          vs_checksums [pShader] = crc32 (0, pbFunc, len);

          free (pbFunc);
        }
      }
    }
    else {
      vs_checksum = 0;
    }
  }

  if (vs_checksum != vs_checksums [pShader])
    needs_ui_center = false;

  vs_checksum = vs_checksums [pShader];

  g_pVS = pShader;
  return D3D9SetVertexShader_Original (This, pShader);
}


typedef HRESULT (STDMETHODCALLTYPE *SetPixelShader_t)
  (IDirect3DDevice9*      This,
   IDirect3DPixelShader9* pShader);

SetPixelShader_t D3D9SetPixelShader_Original = nullptr;

HRESULT
STDMETHODCALLTYPE
D3D9SetPixelShader_Detour (IDirect3DDevice9*      This,
                           IDirect3DPixelShader9* pShader)
{
  // Ignore anything that's not the primary render device.
  if (This != ad::RenderFix::pDevice)
    return D3D9SetPixelShader_Original (This, pShader);

  if (g_pPS != pShader) {
    if (pShader != nullptr) {
      if (ps_checksums.find (pShader) == ps_checksums.end ()) {
        UINT len;
        pShader->GetFunction (nullptr, &len);

        void* pbFunc = malloc (len);

        if (pbFunc != nullptr) {
          pShader->GetFunction (pbFunc, &len);

          ps_checksums [pShader] = crc32 (0, pbFunc, len);

          free (pbFunc);
        }
      }
    } else {
      ps_checksum = 0;
    }
  }

  // Pixel Shader Changed
  if (ps_checksum != ps_checksums [pShader]) {
    if (map.drawing) {
      ++map.shader_changes;
      if (map.shader_changes > 2)
        map.drawing = false;
    }

    needs_ui_center = false;
  }

  dof_active = false;

  ps_checksum = ps_checksums [pShader];

  g_pPS = pShader;
  return D3D9SetPixelShader_Original (This, pShader);
}


typedef void (STDMETHODCALLTYPE *BMF_BeginBufferSwap_t)(void);
BMF_BeginBufferSwap_t BMF_BeginBufferSwap = nullptr;

typedef HRESULT (STDMETHODCALLTYPE *BMF_EndBufferSwap_t)
  (HRESULT   hr,
   IUnknown* device);
BMF_EndBufferSwap_t BMF_EndBufferSwap = nullptr;

typedef HRESULT (STDMETHODCALLTYPE *SetScissorRect_t)(
  IDirect3DDevice9* This,
  const RECT*       pRect);

SetScissorRect_t D3D9SetScissorRect_Original = nullptr;

typedef HRESULT (STDMETHODCALLTYPE *EndScene_t)
(IDirect3DDevice9* This);

EndScene_t D3D9EndScene_Original = nullptr;

int scene_count = 0;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9EndScene_Detour (IDirect3DDevice9* This)
{
  // Ignore anything that's not the primary render device.
  if (This != ad::RenderFix::pDevice)
    return D3D9EndScene_Original (This);

  HRESULT hr = D3D9EndScene_Original (This);

  return hr;
}

COM_DECLSPEC_NOTHROW
void
STDMETHODCALLTYPE
D3D9EndFrame_Pre (void)
{
  return BMF_BeginBufferSwap ();
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9EndFrame_Post (HRESULT hr, IUnknown* device)
{
  // Ignore anything that's not the primary render device.
  if (device != ad::RenderFix::pDevice)
    return BMF_EndBufferSwap (hr, device);

  needs_aspect       = false;
  needs_ui_center    = false;

  map.drawing        = false;
  map.shader_changes = 0;
  map.prims_drawn    = 0;
  map.prim_ypos      = 0.0f;
  map.prim_xpos      = 0.0f;
  map.ps23           = 0.0f;
  map.ps43           = 0.0f;
  map.center_prim    = false;

  scissoring         = false;
  fix_rect           = false;
  dof_active         = false;

  center_count       = 0;
  fullscreen_blit    = false;

  g_pPS           = nullptr;
  g_pVS           = nullptr;
  vs_checksum     = 0;
  ps_checksum     = 0;

  ad::RenderFix::dwRenderThreadID = GetCurrentThreadId ();

  if (log_frame && log_frame_count > 0) {
    log_frame_count--;
    dll_log.Log (L" --- SwapChain Present ---");
    if (log_frame_count <= 0)
      log_frame = false;
  }

  AD_DrawCommandConsole ();

  hr = BMF_EndBufferSwap (hr, device);

  return hr;
}

typedef enum D3DXIMAGE_FILEFORMAT { 
  D3DXIFF_BMP          = 0,
  D3DXIFF_JPG          = 1,
  D3DXIFF_TGA          = 2,
  D3DXIFF_PNG          = 3,
  D3DXIFF_DDS          = 4,
  D3DXIFF_PPM          = 5,
  D3DXIFF_DIB          = 6,
  D3DXIFF_HDR          = 7,
  D3DXIFF_PFM          = 8,
  D3DXIFF_FORCE_DWORD  = 0x7fffffff
} D3DXIMAGE_FILEFORMAT, *LPD3DXIMAGE_FILEFORMAT;

typedef HRESULT (STDMETHODCALLTYPE *D3DXSaveTextureToFile_t)(
  _In_       LPCWSTR                pDestFile,
  _In_       D3DXIMAGE_FILEFORMAT   DestFormat,
  _In_       LPDIRECT3DBASETEXTURE9 pSrcTexture,
  _In_ const PALETTEENTRY           *pSrcPalette
  );

D3DXSaveTextureToFile_t
  D3DXSaveTextureToFile = nullptr;

typedef HRESULT (STDMETHODCALLTYPE *SetTexture_t)
  (     IDirect3DDevice9      *This,
   _In_ DWORD                  Sampler,
   _In_ IDirect3DBaseTexture9 *pTexture);

SetTexture_t D3D9SetTexture_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetTexture_Detour ( IDirect3DDevice9      *This,
                  _In_  DWORD                  Sampler,
                  _In_  IDirect3DBaseTexture9 *pTexture )
{
  return D3D9SetTexture_Original (This, Sampler, pTexture);
}

typedef HRESULT (STDMETHODCALLTYPE *UpdateSurface_t)
  ( _In_       IDirect3DDevice9  *This,
    _In_       IDirect3DSurface9 *pSourceSurface,
    _In_ const RECT              *pSourceRect,
    _In_       IDirect3DSurface9 *pDestinationSurface,
    _In_ const POINT             *pDestinationPoint );

UpdateSurface_t D3D9UpdateSurface_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9UpdateSurface_Detour ( IDirect3DDevice9  *This,
                _In_       IDirect3DSurface9 *pSourceSurface,
                _In_ const RECT              *pSourceRect,
                _In_       IDirect3DSurface9 *pDestinationSurface,
                _In_ const POINT             *pDestinationPoint )
{
  HRESULT hr =
    D3D9UpdateSurface_Original ( This,
                                   pSourceSurface,
                                     pSourceRect,
                                       pDestinationSurface,
                                         pDestinationPoint );

//#define DUMP_TEXTURES
  if (SUCCEEDED (hr)) {
#ifdef DUMP_TEXTURES
    IDirect3DTexture9 *pBase = nullptr;

    HRESULT hr2 =
      pDestinationSurface->GetContainer (
            __uuidof (IDirect3DTexture9),
              (void **)&pBase
          );

    if (SUCCEEDED (hr2) && pBase != nullptr) {
      if (D3DXSaveTextureToFile == nullptr) {
        D3DXSaveTextureToFile =
          (D3DXSaveTextureToFile_t)
          GetProcAddress ( ad::RenderFix::d3dx9_43_dll,
            "D3DXSaveTextureToFileW" );
      }

      if (D3DXSaveTextureToFile != nullptr) {
        wchar_t wszFileName [MAX_PATH] = { L'\0' };
        _swprintf ( wszFileName, L"textures\\UpdateSurface_%x.png",
          pBase );
        D3DXSaveTextureToFile (wszFileName, D3DXIFF_PNG, pBase, NULL);
      }

      pBase->Release ();
    }
#endif
  }

  return hr;
}

typedef HRESULT (STDMETHODCALLTYPE *UpdateTexture_t)
  (IDirect3DDevice9      *This,
   IDirect3DBaseTexture9 *pSourceTexture,
   IDirect3DBaseTexture9 *pDestinationTexture);

UpdateTexture_t D3D9UpdateTexture_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9UpdateTexture_Detour (IDirect3DDevice9      *This,
                          IDirect3DBaseTexture9 *pSourceTexture,
                          IDirect3DBaseTexture9 *pDestinationTexture)
{
  HRESULT hr = D3D9UpdateTexture_Original (This, pSourceTexture,
                                                 pDestinationTexture);

//#define DUMP_TEXTURES
  if (SUCCEEDED (hr)) {
#if 0
    if ( incomplete_textures.find (pDestinationTexture) != 
         incomplete_textures.end () ) {
      dll_log.Log (L" Generating Mipmap LODs for incomplete texture!");
      (pDestinationTexture->GenerateMipSubLevels ());
    }
#endif
#ifdef DUMP_TEXTURES
    if (SUCCEEDED (hr)) {
      if (D3DXSaveTextureToFile != nullptr) {
        wchar_t wszFileName [MAX_PATH] = { L'\0' };
        _swprintf ( wszFileName, L"textures\\UpdateTexture_%x.dds",
          pSourceTexture );
        D3DXSaveTextureToFile (wszFileName, D3DXIFF_DDS, pDestinationTexture, NULL);
      }
    }
#endif
  }

  return hr;
}

typedef HRESULT (STDMETHODCALLTYPE *CreateTexture_t)
  (IDirect3DDevice9   *This,
   UINT                Width,
   UINT                Height,
   UINT                Levels,
   DWORD               Usage,
   D3DFORMAT           Format,
   D3DPOOL             Pool,
   IDirect3DTexture9 **ppTexture,
   HANDLE             *pSharedHandle);

CreateTexture_t D3D9CreateTexture_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9CreateTexture_Detour (IDirect3DDevice9   *This,
                          UINT                Width,
                          UINT                Height,
                          UINT                Levels,
                          DWORD               Usage,
                          D3DFORMAT           Format,
                          D3DPOOL             Pool,
                          IDirect3DTexture9 **ppTexture,
                          HANDLE             *pSharedHandle)
{
  // Ignore anything that's not the primary render device.
  if (This != ad::RenderFix::pDevice)
    return D3D9CreateTexture_Original ( This, Width, Height, Levels, Usage,
                                          Format, Pool, ppTexture, pSharedHandle );

  int levels = Levels;

  HRESULT hr = 
    D3D9CreateTexture_Original (This, Width, Height, levels, Usage,
                                Format, Pool, ppTexture, pSharedHandle);

  return hr;
}

typedef HRESULT (STDMETHODCALLTYPE *CreateDepthStencilSurface_t)
  (IDirect3DDevice9     *This,
   UINT                  Width,
   UINT                  Height,
   D3DFORMAT             Format,
   D3DMULTISAMPLE_TYPE   MultiSample,
   DWORD                 MultisampleQuality,
   BOOL                  Discard,
   IDirect3DSurface9   **ppSurface,
   HANDLE               *pSharedHandle);

CreateDepthStencilSurface_t D3D9CreateDepthStencilSurface_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9CreateDepthStencilSurface_Detour (IDirect3DDevice9     *This,
                                      UINT                  Width,
                                      UINT                  Height,
                                      D3DFORMAT             Format,
                                      D3DMULTISAMPLE_TYPE   MultiSample,
                                      DWORD                 MultisampleQuality,
                                      BOOL                  Discard,
                                      IDirect3DSurface9   **ppSurface,
                                      HANDLE               *pSharedHandle)
{
  dll_log.Log (L" [!] IDirect3DDevice9::CreateDepthStencilSurface (%lu, %lu, "
                      L"%lu, %lu, %lu, %lu, %08Xh, %08Xh)",
                 Width, Height, Format, MultiSample, MultisampleQuality,
                 Discard, ppSurface, pSharedHandle);

  return D3D9CreateDepthStencilSurface_Original (This, Width, Height, Format,
                                                 MultiSample, MultisampleQuality,
                                                 Discard, ppSurface, pSharedHandle);
}

typedef HRESULT (STDMETHODCALLTYPE *CreateRenderTarget_t)
  (IDirect3DDevice9     *This,
   UINT                  Width,
   UINT                  Height,
   D3DFORMAT             Format,
   D3DMULTISAMPLE_TYPE   MultiSample,
   DWORD                 MultisampleQuality,
   BOOL                  Lockable,
   IDirect3DSurface9   **ppSurface,
   HANDLE               *pSharedHandle);

CreateRenderTarget_t D3D9CreateRenderTarget_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9CreateRenderTarget_Detour (IDirect3DDevice9     *This,
                               UINT                  Width,
                               UINT                  Height,
                               D3DFORMAT             Format,
                               D3DMULTISAMPLE_TYPE   MultiSample,
                               DWORD                 MultisampleQuality,
                               BOOL                  Lockable,
                               IDirect3DSurface9   **ppSurface,
                               HANDLE               *pSharedHandle)
{
  dll_log.Log (L" [!] IDirect3DDevice9::CreateRenderTarget (%lu, %lu, "
                      L"%lu, %lu, %lu, %lu, %08Xh, %08Xh)",
                 Width, Height, Format, MultiSample, MultisampleQuality,
                 Lockable, ppSurface, pSharedHandle);

  return D3D9CreateRenderTarget_Original (This, Width, Height, Format,
                                          MultiSample, MultisampleQuality,
                                          Lockable, ppSurface, pSharedHandle);
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetScissorRect_Detour (IDirect3DDevice9* This,
                     const RECT*             pRect)
{
  // Ignore anything that's not the primary render device.
  if (This != ad::RenderFix::pDevice)
    return D3D9SetScissorRect_Original (This, pRect);

  if (! allow_scissor) {
    RECT empty;
    empty.bottom = 0;
    empty.top = 0;
    empty.left = 0;
    empty.right = 0;
    This->SetRenderState (D3DRS_SCISSORTESTENABLE, 0);
    D3D9SetScissorRect_Original (This, &empty);
    return S_OK;
  }

  return D3D9SetScissorRect_Original (This, pRect);

  // If we don't care about aspect ratio, then just early-out
  if (! config.render.aspect_correction)
    return D3D9SetScissorRect_Original (This, pRect);

  //if (! needs_aspect)
    //dll_log.Log (L" ** Scissor rectangle forced aspect ratio correction!");

  //needs_aspect = true;

  // Otherwise, fix this because the UI's scissor rectangles are
  //   completely wrong after we start messing with viewport scaling.

  RECT fixed_scissor;
  fixed_scissor.bottom = pRect->bottom;
  fixed_scissor.top    = pRect->top;
  fixed_scissor.left   = pRect->left;
  fixed_scissor.right  = pRect->right;

  float x_scale, y_scale;
  float x_off,   y_off;
  AD_ComputeAspectCoeffs (x_scale, y_scale, x_off, y_off);

  float height = (9.0f / 16.0f) * (float)viewport.Width;

  dll_log.Log (L"Scissor Rectangle: [%lu,%lu / %lu,%lu] { %lu,%lu / %lu,%lu }",
               pRect->left, pRect->top, pRect->right, pRect->bottom,
                 viewport.X, viewport.Y, viewport.X + viewport.Width, viewport.Y + viewport.Height );

  // If the rectangle has a non-zero area, we are interested in reverse engineering
  // vertex shaders currently active...
  if (pRect->left < pRect->right && pRect->top < pRect->bottom)
    scissoring = true;
  else
    scissoring = false;

  float Width  = 0.0f;
  float Height = 0.0f;

  IDirect3DSurface9* pSurf = nullptr;
  if (SUCCEEDED (ad::RenderFix::pDevice->GetRenderTarget (0, &pSurf))) {
    pSurf->Release ();

    D3DSURFACE_DESC desc;
    if (SUCCEEDED (pSurf->GetDesc (&desc))) {
      float Width  = desc.Width;
      float Height = desc.Height;
    }
  }

  // Wider
  if (config.render.aspect_ratio > 1.7777f) {
    float left_ndc  = 2.0f * ((float)pRect->left  / ((float)viewport.X  + (float)viewport.Width)) - 1.0f;
    float right_ndc = 2.0f * ((float)pRect->right / ((float)viewport.X  + (float)viewport.Width)) - 1.0f;

    int width = (16.0f / 9.0f) * height;

    fixed_scissor.left  = (left_ndc  * width + width) / 2.0f + x_off;
    fixed_scissor.right = (right_ndc * width + width) / 2.0f + x_off;
  } else {
    float top_ndc    = 2.0f * ((float)pRect->top    / ((float)viewport.Y + (float)viewport.Height)) - 1.0f;
    float bottom_ndc = 2.0f * ((float)pRect->bottom / ((float)viewport.Y + (float)viewport.Height)) - 1.0f;

    int height = (9.0f / 16.0f) * viewport.Width;

    fixed_scissor.top    = (top_ndc    * height + height) / 2.0f + y_off;
    fixed_scissor.bottom = (bottom_ndc * height + height) / 2.0f + y_off;
  }

  return D3D9SetScissorRect_Original (This, &fixed_scissor);
}

typedef HRESULT (STDMETHODCALLTYPE *SetViewport_t)(
        IDirect3DDevice9* This,
  CONST D3DVIEWPORT9*     pViewport);

SetViewport_t D3D9SetViewport_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetViewport_Detour (IDirect3DDevice9* This,
                  CONST D3DVIEWPORT9*     pViewport)
{
  // Ignore anything that's not the primary render device.
  if (This != ad::RenderFix::pDevice)
    return D3D9SetViewport_Original (This, pViewport);

  HRESULT hr = D3D9SetViewport_Original (This, pViewport);

  // Detected tiled drawing, we need to handle this specially...
  //if (pViewport->Width == 64) {
    //fix_rect = true;
  //}

  if (SUCCEEDED (hr)) {
    viewport = *pViewport;
  }

  return hr;
}

void
AD_AdjustViewport (IDirect3DDevice9* This, bool UI)
{
  return;

  D3DVIEWPORT9 vp9_orig;
  This->GetViewport (&vp9_orig);

  if (! UI) {
    return;
    vp9_orig.MinZ = 0.0f;
    vp9_orig.MaxZ = 1.0f;
    vp9_orig.X = 0;
    vp9_orig.Y = 0;
    vp9_orig.Width  = ad::RenderFix::width;
    vp9_orig.Height = ad::RenderFix::height;
    D3D9SetViewport_Original (This, &vp9_orig);
    return;
  }

  DWORD width = vp9_orig.Width;
  DWORD height = (9.0f / 16.0f) * vp9_orig.Width;

  // We can't do this, so instead we need to sidebar the stuff
  if (height > vp9_orig.Height) {
    width = (16.0f / 9.0f) * vp9_orig.Height;
    height = vp9_orig.Height + (vp9_orig.Height - height);
  }

  if (height != vp9_orig.Height) {
    D3DVIEWPORT9 vp9;
    vp9.X     = vp9_orig.X;    vp9.Y      = vp9_orig.Y + (vp9_orig.Height - height) / 2;
    vp9.Width = width;         vp9.Height = height;
    vp9.MinZ  = vp9_orig.MinZ; vp9.MaxZ   = vp9_orig.MaxZ;

    D3D9SetViewport_Original (This, &vp9);
  }

  // Sidebar Videos
  if (width != vp9_orig.Width) {
    D3DVIEWPORT9 vp9;
    vp9.X     = vp9_orig.X + (vp9_orig.Width - width) / 2; vp9.Y = vp9_orig.Y;
    vp9.Width = width;                                     vp9.Height = height;
    vp9.MinZ  = vp9_orig.MinZ;                             vp9.MaxZ   = vp9_orig.MaxZ;

    D3D9SetViewport_Original (This, &vp9);
  }
}

typedef HRESULT (STDMETHODCALLTYPE *StretchRect_t)
  (      IDirect3DDevice9    *This,
         IDirect3DSurface9   *pSourceSurface,
   const RECT                *pSourceRect,
         IDirect3DSurface9   *pDestSurface,
   const RECT                *pDestRect,
         D3DTEXTUREFILTERTYPE Filter);

StretchRect_t D3D9StretchRect_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9StretchRect_Detour (      IDirect3DDevice9    *This,
                              IDirect3DSurface9   *pSourceSurface,
                        const RECT                *pSourceRect,
                              IDirect3DSurface9   *pDestSurface,
                        const RECT                *pDestRect,
                              D3DTEXTUREFILTERTYPE Filter )
{
  if (fix_rect && config.render.aspect_correction) {
    //fix_rect = false;
    RECT fake_dest;

    if (pDestRect == nullptr) {
      pDestRect = &fake_dest;

      fake_dest.left   = 0;
      fake_dest.right  = viewport.Width;
      fake_dest.top    = 0;
      fake_dest.bottom = viewport.Height;
    }

    int left = 0;
    int top  = 0;

    DWORD width = (pDestRect->right - pDestRect->left);
    DWORD height = (9.0f / 16.0f) * (pDestRect->right - pDestRect->left);

    if (height > pDestRect->bottom) {
      width  = (16.0f / 9.0f) * (pDestRect->bottom - pDestRect->top);
      height = (pDestRect->bottom - pDestRect->top);
      left   = ((pDestRect->right - pDestRect->left) - width) / 2;
    } else {
      top    = ((pDestRect->bottom - pDestRect->top) - height) / 2;
    }

    RECT fixed;

    fixed.left   = left;
    fixed.top    = top;
    fixed.right  = left + width;
    fixed.bottom = top  + height;

    return D3D9StretchRect_Original ( This,
                                        pSourceSurface,
                                        nullptr,
                                          pDestSurface,
                                          &fixed,
                                            Filter );
  }

  return D3D9StretchRect_Original ( This,
                                      pSourceSurface,
                                      pSourceRect,
                                        pDestSurface,
                                        pDestRect,
                                          Filter );
}

typedef HRESULT (STDMETHODCALLTYPE *DrawPrimitive_t)
                ( IDirect3DDevice9* This,
                  D3DPRIMITIVETYPE  PrimitiveType,
                  UINT              StartVertex,
                  UINT              PrimitiveCount );

DrawPrimitive_t D3D9DrawPrimitive_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9DrawPrimitive_Detour ( IDirect3DDevice9* This,
                           D3DPRIMITIVETYPE  PrimitiveType,
                           UINT              StartVertex,
                           UINT              PrimitiveCount )
{
  // Ignore anything that's not the primary render device.
  if (This != ad::RenderFix::pDevice) {
    return
      D3D9DrawPrimitive_Original ( This,
                                     PrimitiveType,
                                       StartVertex,
                                         PrimitiveCount );
  }

  //
  // Kill Depth of Field Pass
  //
  if (dof_active && kill_dof) {
    return S_OK;
  }

  if (map.drawing) {
    D3DVIEWPORT9 vp = viewport;
    float x, y;
    float x_off, y_off;
    AD_ComputeAspectCoeffs (x, y, x_off, y_off);

    if (config.render.center_ui) {
      vp.Width /= x;
      vp.X     += x_off;
    }

    if (log_frame) {
      dll_log.Log ( L" Minimap Item %d: (%f, %f) [vs: %x, ps: %x]", map.prims_drawn,
                                                                    map.prim_xpos,
                                                                    map.prim_ypos,
                                                                    vs_checksum,
                                                                    ps_checksum );
    }

    if ((map.ps23 == 1.0f && map.ps43 == 1.0f && vert_fix_map) || map.center_prim) {
      //if (! vert_fix_map)
        //vp.Height -= ((float)viewport.Height - vp.Height / x) / 2.0f;
    }

    else {
      vp.Y  += ((float)viewport.Height - vp.Height / x) / 2.0f;
    }

    D3D9SetViewport_Original (This, &vp);

    map.prims_drawn++;
    map.center_prim = false;
  }

#if 0
  if (needs_ui_center && needs_aspect) {
    D3DVIEWPORT9 vp = viewport;
    float x, y;
    float x_off, y_off;
    AD_ComputeAspectCoeffs (x, y, x_off, y_off);

    if (config.render.center_ui) {
      //vp.Width /= x;
      vp.X     += x_off;
    }

    D3D9SetViewport_Original (This, &vp);
  }
#endif

  HRESULT
    hr =
      D3D9DrawPrimitive_Original ( This,
                                     PrimitiveType,
                                       StartVertex,
                                         PrimitiveCount );

  if (map.drawing || (needs_ui_center && needs_aspect)) {
    D3D9SetViewport_Original (This, &viewport);
  }

  return hr;
}

typedef HRESULT (STDMETHODCALLTYPE *DrawIndexedPrimitive_t)(
  IDirect3DDevice9* This,
  D3DPRIMITIVETYPE  Type,
  INT               BaseVertexIndex,
  UINT              MinVertexIndex,
  UINT              NumVertices,
  UINT              startIndex,
  UINT              primCount);

DrawIndexedPrimitive_t D3D9DrawIndexedPrimitive_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9DrawIndexedPrimitive_Detour (IDirect3DDevice9* This,
                                 D3DPRIMITIVETYPE  Type,
                                 INT               BaseVertexIndex,
                                 UINT              MinVertexIndex,
                                 UINT              NumVertices,
                                 UINT              startIndex,
                                 UINT              primCount)
{
  // Ignore anything that's not the primary render device.
  if (This != ad::RenderFix::pDevice) {
    return D3D9DrawIndexedPrimitive_Original ( This, Type,
                                                 BaseVertexIndex, MinVertexIndex,
                                                   NumVertices, startIndex,
                                                     primCount );


  }

  //
  // Kill Depth of Field Pass
  //
  if (dof_active && kill_dof) {
    return S_OK;
  }

  //
  // Minimap Indexed Primitives (The border and actual map only)
  //
  //  -- All of the orbiting blips on the map are non-indexed
  //
  if (map.drawing) {
    D3DVIEWPORT9 vp = viewport;
    float x, y;
    float x_off, y_off;
    AD_ComputeAspectCoeffs (x, y, x_off, y_off);

    if (config.render.center_ui) {
      vp.Width /= x;
      vp.X     += x_off;
    }

    vp.Y  += ((float)viewport.Height - vp.Height / x) / 2.0f;

    D3D9SetViewport_Original (This, &vp);

    map.prims_drawn++;
  }

#if 0
  if (needs_ui_center && needs_aspect) {
    D3DVIEWPORT9 vp = viewport;
    float x, y;
    float x_off, y_off;
    AD_ComputeAspectCoeffs (x, y, x_off, y_off);

    if (config.render.center_ui) {
      //vp.Width /= x;
      vp.X     += x_off;
    }

    D3D9SetViewport_Original (This, &vp);
  }
#endif

  HRESULT
    hr = D3D9DrawIndexedPrimitive_Original ( This, Type,
                                               BaseVertexIndex, MinVertexIndex,
                                                 NumVertices, startIndex,
                                                   primCount );

  if (map.drawing || (needs_ui_center && needs_aspect)) {
    D3D9SetViewport_Original (This, &viewport);
  }

  return hr;
}


typedef HRESULT (STDMETHODCALLTYPE *SetVertexShaderConstantF_t)(
  IDirect3DDevice9* This,
  UINT              StartRegister,
  CONST float*      pConstantData,
  UINT              Vector4fCount);

SetVertexShaderConstantF_t D3D9SetVertexShaderConstantF_Original = nullptr;

float scale_coeff = 0.5f;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetVertexShaderConstantF_Detour (IDirect3DDevice9* This,
                                     UINT              StartRegister,
                                     CONST float*      pConstantData,
                                     UINT              Vector4fCount)
{
  // Ignore anything that's not the primary render device.
  if (This != ad::RenderFix::pDevice) {
    return D3D9SetVertexShaderConstantF_Original ( This,
                                                     StartRegister,
                                                       pConstantData,
                                                         Vector4fCount );
  }

#if 0
  if (scissoring) {
    dll_log.Log ( L" SetVertexShaderConstantF (%x) - Start: %lu, Count: %lu",
                   vs_checksum, StartRegister, Vector4fCount );
    for (int i = 0; i < Vector4fCount; i++) {
      dll_log.Log ( L"   %9.5f, %9.5f, %9.5f, %9.5f",
                      pConstantData [i * 4 + 0], pConstantData [i * 4 + 1],
                      pConstantData [i * 4 + 2], pConstantData [i * 4 + 3] );
    }

#endif

  //
  // Post-Processing Fix (e.g. DoF)
  //
  if ((float)viewport.Width / (float)viewport.Height > 1.777777777777778f && fix_dof && StartRegister == 1 && Vector4fCount == 1 && pConstantData [2] == 0.0f && pConstantData [3] == 0.0f && needs_aspect) {
    float inv_x         = 1.0f / pConstantData [0];
    float inv_y         = 1.0f / pConstantData [1];
    const float aspect  = 16.0f / 9.0f;
    const float epsilon = 2.0f;
    if ( inv_y <= (inv_x / aspect + epsilon) &&
         inv_y >= (inv_x / aspect - epsilon) ) {
      dof_active = true;
      //dll_log.Log (L"DoF Vertex Shader: %x - ps: %x", vs_checksum, ps_checksum);
      //dll_log.Log (L"Fixed Depth Of Field...");

      float ar       = (float)viewport.Width / (float)viewport.Height;

      float pFixedConstants [4];

      pFixedConstants [0] = pConstantData [0];
      pFixedConstants [1] = 1.0f / (inv_x / ar);
      pFixedConstants [2] = 0.0f;
      pFixedConstants [3] = 0.0f;

      //dll_log.Log (L" Y Before: %f, Y After: %f", inv_y, 1.0f / pFixedConstants [1]);

      return D3D9SetVertexShaderConstantF_Original (This, StartRegister, pFixedConstants, Vector4fCount);
    }
  }


  //
  // Map and Mini-Map Fix
  //
  if (config.render.fix_minimap && config.render.aspect_correction && vs_checksum == minimap_shader) {
    if (StartRegister == 2 && pConstantData [1] >= -(1.0f / (float)ad::RenderFix::height/*(float)viewport.Height*/) - 0.000001 && pConstantData [1] <= -(1.0f / (float)ad::RenderFix::height/*(float)viewport.Height*/) + 0.000001) {
#if 0
        dll_log.Log ( L" SetVertexShaderConstantF (%li) - Start: %lu, Count: %lu",
                        vs_checksum, StartRegister, Vector4fCount );
        for (int i = 0; i < Vector4fCount; i++) {
          dll_log.Log ( L"   %9.5f, %9.5f, %9.5f, %9.5f",
                          pConstantData [i * 4 + 0], pConstantData [i * 4 + 1],
                          pConstantData [i * 4 + 2], pConstantData [i * 4 + 3] );
        }
#endif

      float pNotConstantData [16];

      float ar       = (float)viewport.Width / (float)viewport.Height;
      float ar_scale = ar / (16.0f / 9.0f);

      float x_scale, y_scale;
      float x_off,   y_off;
      AD_ComputeAspectCoeffs (x_scale, y_scale, x_off, y_off);

      // Vertical Fix
      for (int i = 1; i < Vector4fCount * 4; i += 2) {
        pNotConstantData [i] = pConstantData [i] / x_scale;
      }

      for (int i = 0; i < Vector4fCount * 4; i += 2) {
        pNotConstantData [i] = pConstantData [i] / y_scale;
      }

      map.drawing = true;

      return D3D9SetVertexShaderConstantF_Original (This, StartRegister, pNotConstantData, Vector4fCount);
    }
  }

#if 0
  if (needs_aspect && StartRegister == 9 && Vector4fCount == 1 && viewport.Width / viewport.Height == ad::RenderFix::width / ad::RenderFix::height) {
      float ar       = (float)viewport.Width / (float)viewport.Height;
      float ar_scale = ar / (16.0f / 9.0f);
      float pNotConstantData [4];
      pNotConstantData [0] = pConstantData [0] / ar_scale;
      pNotConstantData [1] = pConstantData [1] / ar_scale;
      //pNotConstantData [2] = pConstantData [2] / ar_scale;
      //pNotConstantData [3] = pConstantData [3] / ar_scale;
      return D3D9SetVertexShaderConstantF_Original (This, StartRegister, pNotConstantData, Vector4fCount);
  }
#endif

  if (needs_aspect && config.render.aspect_correction && viewport.Width / viewport.Height == ad::RenderFix::width / ad::RenderFix::height && (StartRegister == 1/* || StartRegister == 9*/)) {
    float ar       = (float)viewport.Width / (float)viewport.Height;
    float ar_scale = ar / (16.0f / 9.0f);

    //dll_log.Log (L"Vertex Shader: %x ", vs_checksum);

    if (Vector4fCount == 4 && (pConstantData [3] == 0.0f || pConstantData [7] == 0.0f)) {
      float pNotConstantData [16];

      for (int i = 0; i < Vector4fCount * 4; i++) {
        pNotConstantData [i] = pConstantData [i];
      }

      float width  = (float)viewport.Width;
      float height = (float)viewport.Height;

      float x_pos  = pNotConstantData [12];
      float y_pos  = pNotConstantData [13];

#if 0
        dll_log.Log ( L" SetVertexShaderConstantF (%x) - Start: %lu, Count: %lu",
                        vs_checksum, StartRegister, Vector4fCount );
        for (int i = 0; i < Vector4fCount; i++) {
          dll_log.Log ( L"   %9.5f, %9.5f, %9.5f, %9.5f",
                          pConstantData [i * 4 + 0], pConstantData [i * 4 + 1],
                          pConstantData [i * 4 + 2], pConstantData [i * 4 + 3] );
        }
#endif

      float x_scale, y_scale;
      float x_off,   y_off;
      AD_ComputeAspectCoeffs (x_scale, y_scale, x_off, y_off);

      height = (9.0f / 16.0f) * width;

      float x_ndc = 2.0f * ((x_pos / x_scale) /  width) - 1.0f;
      float y_ndc = 2.0f * ((y_pos / y_scale) / height) - 1.0f;

      //width = (16.0f / 9.0f) * viewport.Height;

      // Loading screens use a Z translation of 100
      // Most HUD items are either translated 16 or 0 units

      // 229.99997 has been seen on inventory screens, but feels like a fluke

      // Assume that all other Z translations are things like nametags,
      //   which must not be centered (translated horizontally).

      //
      // Loading screens tend to have depth = 700
      //

#define PS_CRC32_TEXT 0xd6c2e96

      bool needs_center = false;
      
      if (config.render.center_ui) {
        needs_center = true;

        if (needs_center) {
          if (! (pConstantData [14] == 16.0f || pConstantData [14] == 0.0f || pConstantData [14] == 100.0f || pConstantData [14] == 32.0f || pConstantData [14] == 700.0f /*|| vs_checksum == 0xf52212b8*/)) {
            //if (! (pConstantData [0] == 1.0f && pConstantData [5] == 1.0f && pConstantData [10] == 1.0f)) // Fix for Status screen
              needs_center = false;
          }
        }


        // pConstantData [4] not equal to zero only happens on rotating UI elements

        //
        // Handle save screens
        //
        if ((pConstantData [4] != 0.0f || pConstantData [12] == 640.0f) && pConstantData [14] > 220.0f)
          needs_center = true;
      }

      // Translucent backgrounds (only once per-frame)
      if (ps_checksum == 0x79b9d805) {
        fullscreen_blit = true;
        needs_center    = false;
      }

      // Map drawing is special, we will use a viewport to handle this,
      //   so do not mess with its X coordinate.
      if (map.drawing) {
        needs_center = false;
      }

      bool forbid_center = false;

      if (pConstantData [14] != 16.0f && pConstantData [14] > 0.0f && pConstantData [14] >= 100.0f) {
        //forbid_center = true;
#if 0
        dll_log.Log (L" **** Expected 0.0, 16.0 or 100.0 but got %f", pConstantData [14]);
#endif
        //needs_center = false;
      }

      //
      // These shaders are used on certain fullscreen effects
      //
      if (vs_checksum == 0x3130653c || vs_checksum == 0x29c8a5c7) {
#if 0
        if (log_frame) {
          dll_log.Log ( L" SetVertexShaderConstantF (%x) - Start: %lu, Count: %lu",
                          vs_checksum, StartRegister, Vector4fCount );
          for (int i = 0; i < Vector4fCount; i++) {
            dll_log.Log ( L"   %9.5f, %9.5f, %9.5f, %9.5f",
                            pConstantData [i * 4 + 0], pConstantData [i * 4 + 1],
                            pConstantData [i * 4 + 2], pConstantData [i * 4 + 3] );
          }
      }
#endif

        // A lot of things with these shaders have z=32
      }

      // All fullscreen effects have translation of 640 horizontally and 360 vertically...
      //   this is precisely 1/2 of the Xbox 360's native resolution.
      if (pConstantData [12] == 640.0f && pConstantData [13] == 360.0f && (pConstantData [14] < 100.0f) && (ps_checksum != 0xf88d8bcd)) {
        needs_center = false;
      }

      if (log_frame && (! needs_center) && (! map.drawing)) {
          dll_log.Log ( L" SetVertexShaderConstantF (vs: %x - [ps: %x]) - Start: %lu, Count: %lu",
                            vs_checksum, ps_checksum, StartRegister, Vector4fCount );
            for (int i = 0; i < Vector4fCount; i++) {
              dll_log.Log ( L"   %9.5f, %9.5f, %9.5f, %9.5f",
                              pConstantData [i * 4 + 0], pConstantData [i * 4 + 1],
                              pConstantData [i * 4 + 2], pConstantData [i * 4 + 3] );
            }
        }

      if (log_frame && vs_checksum == 0x5c8f22bc && ps_checksum == 0xbf9778a) {
        dll_log.Log (L"UI Element @ (%2.1f,%2.1f :: %2.1f <%2.1f>) [%lux%lu]", x_pos, y_pos, pConstantData [14], pConstantData [10], viewport.Width, viewport.Height);
        dll_log.Log (L"           # (%2.1f,%2.1f || %2.1f, %2.1f)",                          pConstantData [0], pConstantData [5], pConstantData [4], pConstantData [1]);
        dll_log.Log (L"           % (%2.1f,%2.1f <> %2.1f, %2.1f {%2.1f}",                   pConstantData [2], pConstantData [3], pConstantData [6], pConstantData [7], pConstantData [15]);
        dll_log.Log (L" --> (%2.1f,%2.1f) {%2.1f,%2.1f}", x_ndc, y_ndc, x_off, y_off);
        dll_log.Log (L" UI Scale: (%2.1f,%2.1f)", x_scale, y_scale);

        float xx, yy;

        xx = x_pos * pConstantData [0] + y_pos * pConstantData [1];
        yy = x_pos * pConstantData [4] + y_pos * pConstantData [5];

        dll_log.Log (L" Rotated: (%2.1f, %2.1f)", xx, yy);
      }

      if (needs_center)
        pNotConstantData [0] /= ar_scale;

      // Don't squish nametags :)
      if (! forbid_center)
        pNotConstantData [1] /= ar_scale;

      if (! forbid_center)
        pNotConstantData [4] /= ar_scale;

      if (! forbid_center)
        pNotConstantData [5] /= ar_scale;

      ////dll_log.Log (L"x_off, ar_scale, x_scale, maybe?, maybe? : %f, %f, %f, %f, %f", x_off, ar_scale, x_scale, x_off / ar_scale, x_off / x_scale);


      float scale_trans [16];
      float scale [16] = { x_scale, 0.0f,    0.0f, 0.0f,
                           0.0f,    y_scale, 0.0f, 0.0f,
                           0.0f,    0.0f,    1.0f, 0.0f,
                           0.0f,    0.0f,    0.0f, 1.0f };
      float trans [16] = { 1.0f, 0.0f, 0.0f, 0.0f,
                           0.0f, 1.0f, 0.0f, 0.0f,
                           0.0f, 0.0f, 1.0f, 0.0f,
                           (((x_ndc * width + width) / 2.0f)), ((y_ndc * viewport.Height + viewport.Height) / 2.0f), 0.0f, 1.0f };

      for (int i = 0; i < 16; i += 4)
        for (int j = 0; j < 4; j++)
          scale_trans [i+j] = scale [i] * trans [j] + scale [i+1] * trans [j+4] + scale [i+2] * trans [j+8] + scale [i+3] * trans [j+12];

      if (needs_center)
        pNotConstantData [12] = ((x_ndc * (float)viewport.Width + (float)viewport.Width) / 2.0f) + x_off * scale_coeff;

      pNotConstantData [13] = scale_trans [13];

      if (true) {
        float scale2 [16] = { 1.0f/ar_scale, 0.0f,         0.0f, 0.0f,
                              0.0f,         1.0f/ar_scale, 0.0f, 0.0f,
                              0.0f,         0.0f,          1.0f, 0.0f,
                              0.0f,         0.0f,          0.0f, 1.0f };
        float orig [16] = { pConstantData [ 0], pConstantData [ 1], pConstantData [ 2], pConstantData [ 3],
                            pConstantData [ 4], pConstantData [ 5], pConstantData [ 6], pConstantData [ 7],
                            pConstantData [ 8], pConstantData [ 9], pConstantData [10], pConstantData [11],
                            pConstantData [12], pConstantData [13], pConstantData [14], pConstantData [15] };

        float scaled [16];

        for (int i = 0; i < 16; i += 4)
          for (int j = 0; j < 4; j++)
            scaled [i+j] = scale2 [i] * orig [j] + scale2 [i+1] * orig [j+4] + scale2 [i+2] * orig [j+8] + scale2 [i+3] * orig [j+12];

        if (map.drawing || needs_center)
          pNotConstantData [0] = scaled [0];

        //if (needs_center)
          //needs_ui_center = true;
        //else
          //needs_ui_center = false;

        if (! needs_center)
          //pNotConstantData [12] = ((x_ndc * width + width) / 2.0f) / (scaled [0] / pNotConstantData [0]);
        //else
          pNotConstantData [12] = scaled [12];

        pNotConstantData [1] = scaled [1];
        pNotConstantData [4] = scaled [4];
        pNotConstantData [5] = scaled [5];


        pNotConstantData [0] * ar_scale;
        pNotConstantData [1] * ar_scale;
        pNotConstantData [4] * ar_scale;
        pNotConstantData [5] * ar_scale;
      }

      if (map.drawing) {
        if (log_frame) {
          dll_log.Log (L" After transformation: (%2.1f,%2.1f)", pNotConstantData [12], pNotConstantData [13]);
        }
        map.prim_ypos = y_pos;
        map.prim_xpos = x_pos;
      }

      ///////pNotConstantData [13] += ((float)viewport.Height - viewport.Height / x_scale) / 2.0f;

      return D3D9SetVertexShaderConstantF_Original (This, StartRegister, pNotConstantData, Vector4fCount);
    }

    //if (pConstantData [12] == 640.0 && pConstantData [13] == 420.0)
      //dll_log.Log (L"UI Shader Detected: (vs=%x,ps=%x)", vs_checksum, ps_checksum);
  }

  return D3D9SetVertexShaderConstantF_Original (This, StartRegister, pConstantData, Vector4fCount);
}



typedef HRESULT (STDMETHODCALLTYPE *SetPixelShaderConstantF_t)(
  IDirect3DDevice9* This,
  UINT              StartRegister,
  CONST float*      pConstantData,
  UINT              Vector4fCount);

SetVertexShaderConstantF_t D3D9SetPixelShaderConstantF_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetPixelShaderConstantF_Detour (IDirect3DDevice9* This,
  UINT              StartRegister,
  CONST float*      pConstantData,
  UINT              Vector4fCount)
{
  // Ignore anything that's not the primary render device.
  if (This != ad::RenderFix::pDevice) {
    return D3D9SetPixelShaderConstantF_Original ( This,
                                                    StartRegister,
                                                      pConstantData,
                                                        Vector4fCount );
  }

  if (scissoring) {
#if 0
    dll_log.Log ( L" SetPixelShaderConstantF (%x) - Start: %lu, Count: %lu",
                   ps_checksum, StartRegister, Vector4fCount );
    for (int i = 0; i < Vector4fCount; i++) {
      dll_log.Log ( L"   %9.5f, %9.5f, %9.5f, %9.5f",
                      pConstantData [i * 4 + 0], pConstantData [i * 4 + 1],
                      pConstantData [i * 4 + 2], pConstantData [i * 4 + 3] );
    }
#endif
  }

  // When the game switches from rendering the world to the UI, it's as simple as looking for the first
  //   occurence of this pattern.
  if (StartRegister == 1 && Vector4fCount == 1) {
    if (pConstantData [0] == 0.5f && pConstantData [1] == 2.0f &&
        pConstantData [2] == 1.0f && pConstantData [3] == 1.0f) {
      if (! needs_aspect) {
        if (log_frame)
          dll_log.Log (L"Forcing ARC On Because of Pixel Shader");
      }
      needs_aspect    = true;
      fullscreen_blit = true;
    } else {
      fullscreen_blit = false;
    }
  }

  if (map.drawing) {
    // Accurately detect only the center triangle on the mini-map
    if (StartRegister == 4 && Vector4fCount == 1 && pConstantData [0] == 1.0f && pConstantData [1] == 1.0f &&
                                                    pConstantData [2] == 1.0f && pConstantData [3] == 1.0f &&
        map.drawing) {
      if (map.prim_ypos > 575.0f && map.prim_ypos < 585.0f && map.prim_xpos > 165.0f && map.prim_xpos < 170.0f) {
        if (log_frame)
          dll_log.Log (L" Center Primitive: VS: %x, PS: %x", vs_checksum, ps_checksum);
        map.center_prim = true;
      }
    }

    if (StartRegister == 4 && Vector4fCount == 1) {
      map.ps43 = pConstantData [3];
    }

    if (StartRegister == 2 && Vector4fCount == 1)
      map.ps23 = pConstantData [3];
  }

  return D3D9SetPixelShaderConstantF_Original (This, StartRegister, pConstantData, Vector4fCount);
}



#define D3DX_DEFAULT ((UINT) -1)
typedef struct D3DXIMAGE_INFO {
  UINT                 Width;
  UINT                 Height;
  UINT                 Depth;
  UINT                 MipLevels;
  D3DFORMAT            Format;
  D3DRESOURCETYPE      ResourceType;
  D3DXIMAGE_FILEFORMAT ImageFileFormat;
} D3DXIMAGE_INFO, *LPD3DXIMAGE_INFO;

typedef HRESULT (WINAPI *D3DXCreateTexture_t)
(
  _In_  LPDIRECT3DDEVICE9  pDevice,
  _In_  UINT               Width,
  _In_  UINT               Height,
  _In_  UINT               MipLevels,
  _In_  DWORD              Usage,
  _In_  D3DFORMAT          Format,
  _In_  D3DPOOL            Pool,
  _Out_ LPDIRECT3DTEXTURE9 *ppTexture
);

D3DXCreateTexture_t
  D3DXCreateTexture = nullptr;

typedef HRESULT (WINAPI *D3DXCreateTextureFromFile_t)
(
  _In_  LPDIRECT3DDEVICE9  pDevice,
  _In_  LPCWSTR            pSrcFile,
  _Out_ LPDIRECT3DTEXTURE9 *ppTexture
);

LPVOID pfnBMF_SetPresentParamsD3D9 = nullptr;

typedef D3DPRESENT_PARAMETERS* (__stdcall *BMF_SetPresentParamsD3D9_t)
  (IDirect3DDevice9*      device,
   D3DPRESENT_PARAMETERS* pparams);
BMF_SetPresentParamsD3D9_t BMF_SetPresentParamsD3D9_Original = nullptr;

COM_DECLSPEC_NOTHROW
D3DPRESENT_PARAMETERS*
__stdcall
BMF_SetPresentParamsD3D9_Detour (IDirect3DDevice9*      device,
                                 D3DPRESENT_PARAMETERS* pparams)
{
  D3DPRESENT_PARAMETERS present_params;

  //
  // TODO: Figure out what the hell is doing this when RTSS is allowed to use
  //         custom D3D libs. 1x1@0Hz is obviously NOT for rendering!
  //
  if ( pparams->BackBufferWidth            == 1 &&
       pparams->BackBufferHeight           == 1 &&
       pparams->FullScreen_RefreshRateInHz == 0 ) {
    dll_log.Log (L" * Fake D3D9Ex Device Detected... Ignoring!");
    return BMF_SetPresentParamsD3D9_Original (device, pparams);
  }

  ad::RenderFix::pDevice             = device;

  if (pparams != nullptr) {
    memcpy (&present_params, pparams, sizeof D3DPRESENT_PARAMETERS);

    dll_log.Log ( L" %% Caught D3D9 Swapchain :: Fullscreen=%s "
                  L" (%lux%lu@%lu Hz) "
                  L" [Device Window: 0x%04X]",
                    pparams->Windowed ? L"False" :
                                        L"True",
                      pparams->BackBufferWidth,
                        pparams->BackBufferHeight,
                          pparams->FullScreen_RefreshRateInHz,
                            pparams->hDeviceWindow );

    ad::RenderFix::hWndDevice = pparams->hDeviceWindow;

    ad::RenderFix::width  = present_params.BackBufferWidth;
    ad::RenderFix::height = present_params.BackBufferHeight;
    //ad::FrameRateFix::fullscreen = (! pparams->Windowed);

    //
    // Optimized centers to avoid post-processing nois
    //

    float ar = (float)pparams->BackBufferWidth / (float)pparams->BackBufferHeight;

    if (ar == 2560.0f / 1080.0f) {
      scale_coeff = 0.497f;
    }

    // 21:9
    else if (ar >= (21.0f / 9.0f) - 0.25f && ar <= (21.0f / 9.0f) + 0.25f) {
      dll_log.Log (L" >> Aspect Ratio: 21:9");
      scale_coeff = 0.373f;

      if (pparams->BackBufferWidth == 1120 && pparams->BackBufferHeight == 480) {
        scale_coeff = 1.151f;
      }
    }

    // 16:5
    else if (ar == 16.0f / 5.0f) {
      dll_log.Log (L" >> Aspect Ratio: 16:5");
      scale_coeff = 0.4425f;
    }

    // 16:3
    else if (ar == 16.0f / 3.0f) {
      dll_log.Log (L" >> Aspect Ratio: 16:3");
      scale_coeff = 0.222;
    }

    else if (ar > 16.0f / 9.0f) {
      dll_log.Log (L" >> Aspect Ratio:  Non-Standard Widescreen (%f)", ar);
      //scale_coeff = 0.5f;
    } else {
      dll_log.Log (L" >> Aspect Ratio:  16:9 or narrower");
      scale_coeff = 0.0f;
    }
  }

  return BMF_SetPresentParamsD3D9_Original (device, pparams);
}



void
ad::RenderFix::Init (void)
{
  AD_CreateDLLHook ( config.system.injector.c_str (),
                     "D3D9SetViewport_Override",
                      D3D9SetViewport_Detour,
            (LPVOID*)&D3D9SetViewport_Original );

  AD_CreateDLLHook ( config.system.injector.c_str (),
                     "D3D9SetScissorRect_Override",
                      D3D9SetScissorRect_Detour,
            (LPVOID*)&D3D9SetScissorRect_Original );

  AD_CreateDLLHook ( config.system.injector.c_str (),
                     "D3D9StretchRect_Override",
                      D3D9StretchRect_Detour,
            (LPVOID*)&D3D9StretchRect_Original );

  AD_CreateDLLHook ( config.system.injector.c_str (),
                     "D3D9DrawPrimitive_Override",
                      D3D9DrawPrimitive_Detour,
            (LPVOID*)&D3D9DrawPrimitive_Original );

  AD_CreateDLLHook ( config.system.injector.c_str (),
                     "D3D9DrawIndexedPrimitive_Override",
                      D3D9DrawIndexedPrimitive_Detour,
            (LPVOID*)&D3D9DrawIndexedPrimitive_Original );

  AD_CreateDLLHook ( config.system.injector.c_str (),
                     "D3D9SetVertexShaderConstantF_Override",
                      D3D9SetVertexShaderConstantF_Detour,
            (LPVOID*)&D3D9SetVertexShaderConstantF_Original );

  AD_CreateDLLHook ( config.system.injector.c_str (),
                     "D3D9SetVertexShader_Override",
                      D3D9SetVertexShader_Detour,
            (LPVOID*)&D3D9SetVertexShader_Original );

  AD_CreateDLLHook ( config.system.injector.c_str (),
                     "D3D9SetPixelShader_Override",
                      D3D9SetPixelShader_Detour,
            (LPVOID*)&D3D9SetPixelShader_Original );

  AD_CreateDLLHook ( config.system.injector.c_str (),
                     "D3D9SetPixelShaderConstantF_Override",
                      D3D9SetPixelShaderConstantF_Detour,
            (LPVOID*)&D3D9SetPixelShaderConstantF_Original );


  AD_CreateDLLHook ( config.system.injector.c_str (),
                     "BMF_BeginBufferSwap",
                      D3D9EndFrame_Pre,
            (LPVOID*)&BMF_BeginBufferSwap );

  AD_CreateDLLHook ( config.system.injector.c_str (),
                     "BMF_EndBufferSwap",
                      D3D9EndFrame_Post,
            (LPVOID*)&BMF_EndBufferSwap );


  AD_CreateDLLHook ( config.system.injector.c_str (),
                     "D3D9EndScene_Override",
                      D3D9EndScene_Detour,
            (LPVOID*)&D3D9EndScene_Original );


  AD_CreateDLLHook ( config.system.injector.c_str (),
                     "BMF_SetPresentParamsD3D9",
                      BMF_SetPresentParamsD3D9_Detour, 
           (LPVOID *)&BMF_SetPresentParamsD3D9_Original );

  CommandProcessor* comm_proc = CommandProcessor::getInstance ();
}

void
ad::RenderFix::Shutdown (void)
{
}

ad::RenderFix::CommandProcessor::CommandProcessor (void)
{
  center_ui_    = new eTB_VarStub <bool>  (&config.render.center_ui);

  eTB_Variable* aspect_correction   = new eTB_VarStub <bool>  (&config.render.aspect_correction);

  command.AddVariable ("AspectCorrection", aspect_correction);
  command.AddVariable ("CenterUI",         center_ui_);
  command.AddVariable ("ScaleCoeff",       new eTB_VarStub <float> (&scale_coeff));
  command.AddVariable ("AllowScissor",     new eTB_VarStub <bool>  (&allow_scissor));
  command.AddVariable ("FixMinimap",       new eTB_VarStub <bool>  (&config.render.fix_minimap));
  command.AddVariable ("MinimapShader",    new eTB_VarStub <int>   (&minimap_shader));
  command.AddVariable ("VertFixMap",       new eTB_VarStub <bool>  (&vert_fix_map));

  command.AddVariable ("FixDOF",           new eTB_VarStub <bool>  (&fix_dof));
  command.AddVariable ("KillDOF",          new eTB_VarStub <bool>  (&kill_dof));

  command.AddVariable ("LogFrame",         new eTB_VarStub <bool>  (&log_frame));
  command.AddVariable ("FramesToLog",      new eTB_VarStub <int>   (&log_frame_count));

}

bool
ad::RenderFix::CommandProcessor::OnVarChange (eTB_Variable* var, void* val)
{
  return true;
}


ad::RenderFix::CommandProcessor* ad::RenderFix::CommandProcessor::pCommProc;

HWND              ad::RenderFix::hWndDevice = NULL;
IDirect3DDevice9* ad::RenderFix::pDevice    = nullptr;

uint32_t ad::RenderFix::width            = 0UL;
uint32_t ad::RenderFix::height           = 0UL;
uint32_t ad::RenderFix::dwRenderThreadID = 0UL;

HMODULE            ad::RenderFix::d3dx9_43_dll        = 0;
HMODULE            ad::RenderFix::user32_dll          = 0;