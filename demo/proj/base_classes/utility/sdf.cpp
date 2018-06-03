#include "sdf.h"
#include "base_classes/mesh.h"
#include "base_classes/gl/texture.h"

using namespace code_policy;

SHADER(font__distance_transform_v_ps,
R"(#version 330 core
in vec2 glTexCoord;
layout(location = 0)out vec4 glFragColor;
uniform sampler2D tex;
uniform int r;
uniform vec2 step;
uniform float side;

void main()
{
for(int i=0; i<r; ++i)
{
vec2 o = step * float(i);
float t = side * 0.5;
if((side * texture(tex, glTexCoord + o).r > t) || (side * texture(tex, glTexCoord - o).r > t))
{
glFragColor = vec4(vec3(float(i) / r), 1.);
return;
}
}

glFragColor = vec4(1);
})")

SHADER(font__distance_transform_ps,
R"(#version 330 core
in vec2 glTexCoord;
layout(location = 0)out vec4 glFragColor;
uniform sampler2D tex_i, tex_o;
uniform int r;
uniform vec2 step;

void main()
{
float d_i = texture(tex_i, glTexCoord).r;
float d_o = texture(tex_o, glTexCoord).r;

for(int i=1; i<r; ++i)
{
float v = float(i) / r;
vec2 o = step * float(i);
d_o = min(d_o, min(length(vec2(v, texture(tex_o, glTexCoord + o).r)), length(vec2(v, texture(tex_o, glTexCoord - o).r))));
d_i = min(d_i, min(length(vec2(v, texture(tex_i, glTexCoord + o).r)), length(vec2(v, texture(tex_i, glTexCoord - o).r))));
}

d_o = 0.5 - d_o * 0.5;
d_i = 0.5 + d_i * 0.5;

glFragColor = vec4(vec3(mix(d_o, d_i, float(d_i > 0.5))), 1.);
})")

GLtex2d SdfGenerator::generate(GLtex2d tex, uint scale, uint border)const
{
  Val width = tex.width()
      , height = tex.height();
  Val surf_out = GLfbo(width, height, 1)
      , surf_inner = GLfbo(width, height, 1);
  {
    auto bs = GLbind(m_dst_t);
    bs.Uniforms("tex", 0,
                "r", border,
                "step", vec2(0., 1. / height));
    GLbind(tex, 0).Parameters(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLState::Save<GL_BLEND, GL_MULTISAMPLE, GL_DEPTH_WRITEMASK>();
    GLState::Disable<GL_BLEND, GL_MULTISAMPLE, GL_DEPTH_WRITEMASK>();

    bs.Uniform("side", 1.);
    GLbind(surf_out);
    Screen::Draw();

    bs.Uniform("side", -1.);
    GLbind(surf_inner);
    Screen::Draw();
  }

  Val surf_res = GLfbo(width, height, 1);
  {
    GLbind(m_dt_h).Uniforms("tex_o", 0,
                          "tex_i", 1,
                          "r", border,
                          "step", vec2(1. / width, 0.));
    GLbind(surf_res);
    GLbind(surf_out.tex(), 0);
    GLbind(surf_inner.tex(), 1);
    Screen::Draw();
  }

  GLfbo surf = { width / scale, height / scale, 1 };
  GLbind(m_render).Uniform("tex", 0);
  GLbind(surf);
  GLbind(surf_res.tex(), 0);
  Screen::Draw();

  GLState::Restore<GL_BLEND, GL_MULTISAMPLE, GL_DEPTH_WRITEMASK>();

  return surf.TakeTexture();
}
