#include "minimap.h"

ad_minimap_s* minimap = nullptr;

void
ad_minimap_s::reset (void)
{
  drawing        = false;
  finished       = false;
  shader_changes = 0;
  prims_drawn    = 0;
  prim_xpos      = 0.0f;
  prim_ypos      = 0.0f;
  prim_zpos      = 0.0f;
  ps23           = 0.0f;
  ps43           = 0.0f;
  center_prim    = false;
}