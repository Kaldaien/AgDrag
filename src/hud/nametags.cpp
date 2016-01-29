#include "../config.h"

#include <d3d9.h>
#include <d3d9types.h>

#include "nametags.h"

ad_nametags_s* nametags = nullptr;

ad_nametag_phase_s quest = { ad_nametag_phase_s::PHASE_QUEST };
ad_nametag_phase_s names = { ad_nametag_phase_s::PHASE_NAMES };

void
ad_nametags_s::reset (void)
{
  drawing  = false;
  finished = false;
  phase    = nullptr;

  top_technique = config.nametags.always_on_top;
}

bool
ad_nametags_s::shouldAspectCorrect (void)
{
  return config.nametags.aspect_correct;
}

bool
ad_nametags_s::shouldDrawOnTop (void)
{
  return (config.nametags.always_on_top || config.nametags.temp_on_top);
}


DWORD dwLastZEnable  = 0;
DWORD dwLastZFunc    = 0;

DWORD dwLastAlphaTest = 0;
DWORD dwLastAlphaFunc = 0;
DWORD dwLastAlphaRef  = 0;

DWORD dwLastBlendOp  = 0;
DWORD dwLastSrcOp    = 0;
DWORD dwLastDstOp    = 0;
DWORD dwLastBlend    = 0;

void
ad_nametags_s::beginPrimitive (IDirect3DDevice9* pDev)
{
  pDev->GetRenderState (D3DRS_ZENABLE, &dwLastZEnable);
  pDev->GetRenderState (D3DRS_ZFUNC,   &dwLastZFunc);

  pDev->SetRenderState (D3DRS_ZENABLE, D3DZB_TRUE);

  if (top_technique > 1) {
    pDev->GetRenderState (D3DRS_ALPHATESTENABLE,  &dwLastAlphaTest);
    pDev->GetRenderState (D3DRS_ALPHAFUNC,        &dwLastAlphaFunc);
    pDev->GetRenderState (D3DRS_ALPHAREF,         &dwLastAlphaRef);

    pDev->GetRenderState (D3DRS_DESTBLEND,        &dwLastDstOp);
    pDev->GetRenderState (D3DRS_ALPHABLENDENABLE, &dwLastBlend);

    //pDev->GetRenderState (D3DRS_ALPHATESTENABLE, );

    pDev->GetRenderState (D3DRS_BLENDOP,          &dwLastBlendOp);
    pDev->GetRenderState (D3DRS_SRCBLEND,         &dwLastSrcOp);
    pDev->GetRenderState (D3DRS_DESTBLEND,        &dwLastDstOp);
    pDev->GetRenderState (D3DRS_ALPHABLENDENABLE, &dwLastBlend);

    //pDev->SetRenderState (D3DRS_BLENDOP,          D3DBLENDOP_REVSUBTRACT);

    pDev->SetRenderState (D3DRS_SRCBLEND,         D3DBLEND_INVSRCALPHA);
    pDev->SetRenderState (D3DRS_DESTBLEND,        D3DBLEND_SRCALPHA);
    pDev->SetRenderState (D3DRS_ALPHABLENDENABLE, TRUE);

    pDev->SetRenderState (D3DRS_ZFUNC,            D3DCMP_GREATER);

    // Avoid (most) fringing on translucent edges -- we should really be using
    //   pre-multiplied alpha, but accomplishing that is too much trouble.
    pDev->SetRenderState (D3DRS_ALPHATESTENABLE, TRUE);
    pDev->SetRenderState (D3DRS_ALPHAFUNC,       D3DCMP_GREATEREQUAL);

    pDev->SetRenderState (D3DRS_ALPHAREF,        0x0f);
  } else {
    pDev->SetRenderState (D3DRS_ZFUNC,            D3DCMP_ALWAYS);
  }
}

void
ad_nametags_s::endPrimitive (IDirect3DDevice9* pDev)
{
  pDev->SetRenderState (D3DRS_ZENABLE, dwLastZEnable);
  pDev->SetRenderState (D3DRS_ZFUNC,   dwLastZFunc);

  if (top_technique > 1) {
    pDev->SetRenderState (D3DRS_ALPHATESTENABLE,  dwLastAlphaTest);
    pDev->SetRenderState (D3DRS_ALPHAFUNC,        dwLastAlphaFunc);
    pDev->SetRenderState (D3DRS_ALPHAREF,         dwLastAlphaRef);

    pDev->SetRenderState (D3DRS_BLENDOP,          dwLastBlendOp);

    pDev->SetRenderState (D3DRS_SRCBLEND,         dwLastSrcOp);
    pDev->SetRenderState (D3DRS_DESTBLEND,        dwLastDstOp);

    pDev->SetRenderState (D3DRS_ALPHABLENDENABLE, dwLastBlend);
  }
}