#pragma once
#include "base_classes/gl/shader.h"
#include "base_classes/gl/texture.h"

namespace code_policy
{

struct SdfGenerator
{
  GLtex2d generate(GLtex2d tex, uint scale, uint border)const;

private:
  const GLshader
  m_dst_t =  { "mesh__2d_screen_vs", "font__distance_transform_v_ps" },
  m_dt_h =   { "mesh__2d_screen_vs", "font__distance_transform_ps" },
  m_render = { "mesh__2d_screen_vs", "mesh__2d_screen_ps" };
};

}
