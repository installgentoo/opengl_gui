#include "objects.h"
#include "base_classes/font.h"
#include "base_classes/texture_atlas.h"
#include "base_classes/policies/window.h"
#include "base_classes/gl/shader.h"
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/packing.hpp>
#include <utfcpp/utf8.h>

using namespace GUI;
using glm::packHalf1x16;

SHADER(gui__pos_col_vs,
R"(#version 330 core
layout(location = 0)in vec3 Position;
layout(location = 1)in vec4 Color;
out vec4 glColor;

void main()
{
gl_Position = vec4(Position, 1.);
glColor = Color;
})")

SHADER(gui__pos_col_tex_z_vs,
R"(#version 330 core
layout(location = 0)in vec4 Position;
layout(location = 1)in vec4 Color;
layout(location = 2)in vec2 TexCoord;
out vec4 glColor;
out vec3 glTexCoord;

void main()
{
gl_Position = vec4(Position.xyz, 1.);
glColor = Color;
glTexCoord = vec3(TexCoord, Position.a);
})")

SHADER(gui__pos_col_tex_vs,
R"(#version 330 core
layout(location = 0)in vec3 Position;
layout(location = 1)in vec4 Color;
layout(location = 2)in vec2 TexCoord;
out vec4 glColor;
out vec2 glTexCoord;

void main()
{
gl_Position = vec4(Position, 1.);
glColor = Color;
glTexCoord = TexCoord;
})")

SHADER(gui__col_ps,
R"(#version 330 core
in vec4 glColor;
layout(location = 0)out vec4 glFragColor;

void main()
{
glFragColor = glColor;
})")

SHADER(gui__col_tex_ps,
R"(#version 330 core
in vec4 glColor;
in vec2 glTexCoord;
layout(location = 0)out vec4 glFragColor;
uniform sampler2D src;

void main()
{
glFragColor = glColor * texture(src, glTexCoord);
})")

SHADER(gui__frame_ps,
R"(#version 330 core
in vec4 glColor;
in vec3 glTexCoord;
layout(location = 0)out vec4 glFragColor;
uniform sampler2D theme;

void main()
{
float d = length(glTexCoord.xy);
vec4 c = texture(theme, vec2(d, glTexCoord.z));
glFragColor = glColor * c;
})")

SHADER(gui_sdf_ps,
R"(#version 330 core
in vec4 glColor;
in vec2 glTexCoord;
layout(location = 0)out vec4 glFragColor;
uniform sampler2D src;

void main()
{
ivec2 sz = textureSize(src, 0);

float dx = dFdx( glTexCoord.x ) * sz.x;
float dy = dFdy( glTexCoord.y ) * sz.y;

float toPixels = 8 * inversesqrt( dx * dx + dy * dy );

vec2 step = vec2(dFdx(glTexCoord.x) * 0.5, 0.);

float pix_l = texture(src, glTexCoord.xy - step).r - 0.5;
float pix_r = texture(src, glTexCoord.xy + step).r - 0.5;
float pix_n = texture(src, glTexCoord.xy + step * 2.).r - 0.5;

float pix = clamp((texture(src, glTexCoord.xy).r - 0.5) * 8 * toPixels + 0.5 , 0., 1.);

pix_l = clamp(pix_l * toPixels + 1, 0., 1.);
pix_r = clamp(pix_r * toPixels + 1, 0., 1.);
pix_n = clamp(pix_n * toPixels + 1, 0., 1.);

pix = ( pix_l + pix_r + pix ) / 3.;

vec4 correction = vec4(vec3(pix_l, pix_r, pix_n), pix);

/*// Antialias
float center = texture(src, glTexCoord.xy).r;
float dscale = 0.354; // half of 1/sqrt2
float friends = 0.5;  // scale value to apply to neighbours

vec2 duv = dscale * (dFdx(v_uv) + dFdy(v_uv));
vec4 box = vec4(v_uv-duv, v_uv+duv);

vec4 c = samp( box.xy ) + samp( box.zw ) + samp( box.xw ) + samp( box.zy );
float sum = 4.; // 4 neighbouring samples

rgbaOut = fontColor * (center + friends * c) / (1. + sum*friends);
*/

glFragColor = glColor * correction;
})")

template<class T, class A> static T copy(T it, A arr)
{
  std::copy(arr.begin(), arr.end(), it);
  return it + arr.size();
}


vector<GLushort> Obj::genIdx(uint start, uint size)const
{
  vector<GLushort> v;
  v.reserve((size * 3) / 2);

  for(uint i=start; i<start+size; i+=4)
    v.insert(v.cend(), { GLushort(i), GLushort(i+1), GLushort(i+3), GLushort(i+3), GLushort(i+1), GLushort(i+2) });

  return v;
}
/*
vector<GLushort> 9Sprite::genIdx(uint start, uint size)const
{
  vector<GLushort> v;
  v.reserve((size * 8) / 27);

  for(uint i=start; i<start+size; i+=16)
  {
    Val s = [i](uint j){ return GLushort(i + j); };
    v.insert(v.cend(), { s(0), s(1), s(4), s(4), s(1), s(5),  s(5), s(1), s(2), s(2), s(5), s(6),  s(6), s(2), s(3), s(3), s(6), s(7),  s(7), s(6), s(11), s(11), s(6), s(10),  s(10), s(6), s(5), s(5), s(10), s(9),  s(9), s(5), s(4), s(4), s(9), s(8),  s(8), s(9), s(12), s(12), s(9), s(13),  s(13), s(9), s(10), s(10), s(13), s(14),  s(14), s(10), s(11), s(11), s(14), s(15) });
  }

  return v;
}*/


void Obj::Draw(GLbindingVao const&b, GLushort num, GLushort offset)const
{
  static const GLshader s_s = { "gui__pos_col_vs", "gui__col_ps" };
  GLbind(s_s);
  b.DrawOffset(num, offset);
}

void Sprite::Draw(GLbindingVao const&b, GLushort num, GLushort offset)const
{
  static const GLshader s_s = []{ GLshader s = { "gui__pos_col_tex_vs", "gui__col_tex_ps" }; GLbind(s).Uniform("src", 0); return s; }();
  GLbind(s_s);
  GLbind(*m_tex->tex, 0);
  b.DrawOffset(num, offset);
}

/*void 9Sprite::Draw(GLbindingVao const&b, GLushort num, GLushort offset)const
{
  static const GLshader s_s = { "gui__pos_col_tex_z_vs", "gui__frame_ps" };
  static const GLtex theme = []{ GLtex t = GLtex::FromResource(ResourceLoader::Load(themes_file.c_str()), 4); GLbind(t).Parameters(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); return t; }();
  GLbind(s_s);
  GLbind(theme, 0);
  b.DrawOffset(num, offset);
}*/

void Text::Draw(GLbindingVao const&b, GLushort num, GLushort offset)const
{
  static const GLshader s_s = []{ GLshader s = { "gui__pos_col_tex_vs", "gui_sdf_ps" }; GLbind(s).Uniform("src", 0); return s; }();
  GLbind(s_s);
  GLbind(m_font->tex(), 0);
  b.DrawOffset(num, offset);
}


vec4 Obj::bounding_box()const
{
  Val crop1 = vec2(m_crop.x, m_crop.y)
      , crop2 = vec2(m_crop.z, m_crop.w)
      , xy1 = glm::clamp(m_pos, crop1, crop2)
      , xy2 = glm::clamp(m_pos + m_size, crop1, crop2);

  return { xy1, xy2 };
}

bool Obj::intersect(Obj const&r)const
{
  Val bb = this->bounding_box()
      , r_bb = r.bounding_box();

  return !(bb.z <= r_bb.x || bb.x >= r_bb.z ||
           bb.w <= r_bb.y || bb.y >= r_bb.w);
}

Obj::Obj(Vec4 crop, Vec2 pos, Vec2 size, Vec4 color)
  : m_pos(pos)
  , m_size(glm::max(vec2(0), size))
  , m_color(color)
  , m_crop(crop)
{ }


static bool equalColor(Vec4 l, Vec4 r)
{
  return glm::all(glm::epsilonEqual(l, r, vec4(1. / 256)));
}

uint Rect::compare(Vec4 crop, Vec2 pos, Vec2 size, Vec4 color)const
{
  if(!opaque(color) != this->ordered())
    return State::mismatch;

  Val window = Window::Get();
  return (window.equalPos(vec4(m_pos, m_size), vec4(pos, size)) &&
          window.equalPos(m_crop, crop) ? 0u : State::xyzw)
      | (equalColor(m_color, color) ? 0u : State::rgba);
}

uint Sprite::compare(Vec4 crop, Vec2 pos, Vec2 size, struct Vtex const*tex, Vec4 color)const
{
  if(m_atlas_idx != tex->tex->obj())
    return State::mismatch;

  Val window = Window::Get();
  Val same_crop = window.equalPos(m_crop, crop);
  return (same_crop &&
          window.equalPos(vec4(m_pos, m_size), vec4(pos, size)) ? 0u : State::xyzw)
      | (same_crop &&
         m_tex == tex ? 0u : State::uv)
      | (equalColor(m_color, color) ? 0u : State::rgba);
}

/*uint 9Sprite::compare(Vec4 crop, Vec2 pos, Vec2 size, float corner, uint theme, Vec4 color)const
{
  return (equalPos(vec4(m_pos, m_size), vec4(pos, size)) &&
          equalSize(m_corner, corner) &&
          m_theme == theme &&
          equalPos(m_crop, crop) ? 0u : State::xyzw)//coord too
      | (equalColor(m_color, color) ? 0u : State::rgba);
}*/

uint Text::compare(Vec4 crop, Vec2 pos, String text, Font const*font, float scale, Vec4 color)const
{
  Val window = Window::Get();
  return (window.equalPos(m_pos, pos) &&
          m_text == text &&
          m_font == font &&
          window.equalPos(m_scale, scale) &&
          window.equalPos(m_crop, crop) ? 0u : State::xyzw | State::uv)
      | (equalColor(m_color, color) ? 0u : State::rgba);
}


void Rect::genMesh(float level, uint state, it<uint16> xyzw, it<ubyte> rgba, it<uint16> uv)const
{
  if(state & State::xyzw)
  {
    Val aspect = Window::Get().aspect();
    Val bb = this->bounding_box() * vec4(aspect, aspect);

    Val z = packHalf1x16(level)
        , x1 = packHalf1x16(bb.x)
        , x2 = packHalf1x16(bb.z)
        , y1 = packHalf1x16(bb.y)
        , y2 = packHalf1x16(bb.w);

    copy(xyzw, array<uint16, 16>{ x1, y1, z, 0,  x2, y1, z, 0,
                                  x2, y2, z, 0,  x1, y2, z, 0 });
  }

  if(state & State::rgba)
  {
    Val color = glm::round(glm::clamp(m_color, vec4(0), vec4(1)) * 255.f);
    const ubyte r = color.r
        , g = color.g
        , b = color.b
        , a = color.a;

    copy(rgba, array<ubyte, 16>{ r, g, b, a,  r, g, b, a,
                                 r, g, b, a,  r, g, b, a });
  }

  if(state & State::uv)
    copy(uv, array<uint16, 8>{ 0, 0, 0, 0,  0, 0, 0, 0 });
}


bool Sprite::ordered() const
{
  return Obj::ordered() ||
      m_tex->tex->stats().channels > 3;
}

void Sprite::genMesh(float level, uint state, it<uint16> xyzw, it<ubyte> rgba, it<uint16> uv) const
{
  if(state & State::xyzw)
  {
    Val aspect = Window::Get().aspect();
    Val bb = this->bounding_box() * vec4(aspect, aspect);

    Val z = packHalf1x16(level)
        , x1 = packHalf1x16(bb.x)
        , x2 = packHalf1x16(bb.z)
        , y1 = packHalf1x16(bb.y)
        , y2 = packHalf1x16(bb.w);

    copy(xyzw, array<uint16, 16>{ x1, y1, z, 0,  x2, y1, z, 0,
                                  x2, y2, z, 0,  x1, y2, z, 0 });
  }

  if(state & State::rgba)
  {
    Val color = glm::round(glm::clamp(m_color, vec4(0), vec4(1)) * 255.f);
    const ubyte r = color.r
        , g = color.g
        , b = color.b
        , a = color.a;

    copy(rgba, array<ubyte, 16>{ r, g, b, a,  r, g, b, a,
                                 r, g, b, a,  r, g, b, a });
  }

  if(state & State::uv)
  {
    Val coord = m_tex->coord;
    Val u1 = packHalf1x16(coord.x)
        , u2 = packHalf1x16(coord.z)
        , v1 = packHalf1x16(coord.y)
        , v2 = packHalf1x16(coord.w);
    copy(uv, array<uint16, 8>{ u1, v1,  u2, v1,
                               u2, v2,  u1, v2 });//scale
  }
}

Sprite::Sprite(Vec4 crop, Vec2 pos, Vec2 size, Vtex const*tex, Vec4 color)
  : Obj(crop, pos, size, color)
  , m_atlas_idx(tex->tex->obj())
  , m_tex(tex)
{ }


/*void 9Sprite::genMesh(float level, uint state, it<uint16> xyzw, it<ubyte> rgba, it<uint16> uv)const //reimplement as unified sdf shader
{
  CASSERT(!themes_file.empty(), "9Sprite component needs themes_file");
  static uint total_themes = []{ return GLtex::FromResource(ResourceLoader::Load(themes_file.c_str()), 4).height(); }();

  if(state & State::xyzw)
  {
    Val m1 = vec2(glm::min(m_size.x, m_size.y) * glm::min(0.5f, m_corner))
        , m2 =  m_size - m1
        , crop1 = vec2(m_crop.x, m_crop.y)
        , crop2 = vec2(m_crop.z, m_crop.w)
        , size = m_pos + m_size;

    Val wh = vec2(1) / m_size
        , uv1 = wh * (crop1 - m_pos) * vec2(glm::greaterThan(crop1, m_pos))
        , uv2 = wh * (size - crop2) * vec2(glm::lessThan(crop2, size));

    Val xy1 = glm::clamp(m_pos, crop1, crop2)
        , xy2 = glm::clamp(m_pos + m_size, crop1, crop2)
        , m1xy = glm::clamp(m_pos + m1, crop1, crop2)
        , m2xy = glm::clamp(m_pos + m2, crop1, crop2);

    Val z = packHalf1x16(level)
        , x1 = packHalf1x16(xy1.x)
        , y1 = packHalf1x16(xy1.y)
        , x2 = packHalf1x16(xy2.x)
        , y2 = packHalf1x16(xy2.y)
        , m1x = packHalf1x16(m1xy.x)
        , m1y = packHalf1x16(m1xy.y)
        , m2x = packHalf1x16(m2xy.x)
        , m2y = packHalf1x16(m2xy.y)
        , t = packHalf1x16(glm::clamp((double(m_theme) + 0.5) / total_themes, 0., 1.));

    copy(xyzw, array<uint16, 64>{ x1, y1,  z, t,  m1x, y1,  z, t,  m2x, y1,  z, t,  x2, y1,  z, t,
                                  x1, m1y, z, t,  m1x, m1y, z, t,  m2x, m1y, z, t,  x2, m1y, z, t,
                                  x1, m2y, z, t,  m1x, m2y, z, t,  m2x, m2y, z, t,  x2, m2y, z, t,
                                  x1, y2,  z, t,  m1x, y2,  z, t,  m2x, y2,  z, t,  x2, y2,  z, t });

    static const GLushort n = packHalf1x16(0), o = packHalf1x16(1);
    copy(uv, array<uint16, 32>{ o, o,  n, o,  n, o,  o, o,
                                o, n,  n, n,  n, n,  o, n,
                                o, n,  n, n,  n, n,  o, n,
                                o, o,  n, o,  n, o,  o, o });
  }

  if(state & State::rgba)
  {
    Val color = glm::round(glm::clamp(m_color, vec4(0), vec4(1)) * 255.f);
    const ubyte r = color.r
        , g = color.g
        , b = color.b
        , a = color.a;

    copy(rgba, array<ubyte, 64>{ r, g, b, a,  r, g, b, a,  r, g, b, a,  r, g, b, a,
                                 r, g, b, a,  r, g, b, a,  r, g, b, a,  r, g, b, a,
                                 r, g, b, a,  r, g, b, a,  r, g, b, a,  r, g, b, a,
                                 r, g, b, a,  r, g, b, a,  r, g, b, a,  r, g, b, a });
  }
}*/


void Text::genMesh(float level, uint state, it<uint16> xyzw, it<ubyte> rgba, it<uint16> uv)const
{
  if(state & State::xyzw ||
     state & State::uv)
  {
    Val s = m_scale / (m_font->topline() - m_font->bottomline());
    Val z = packHalf1x16(level);
    Val pos = m_pos + vec2(0, -m_font->bottomline()) * s
        , crop1 = vec2(m_crop.x, m_crop.y)
        , crop2 = vec2(m_crop.z, m_crop.w);

    float x = !m_text.empty() ? -m_font->charData(utf8::unchecked::peek_next(m_text.cbegin())).x1 : 0;

    uint last_char = 0;
    for(auto i=m_text.begin(); i!=m_text.cend();)
    {
      Val code = utf8::unchecked::next(i);
      Val c = m_font->charData(code);
      x += m_font->kerning(last_char, code);
      last_char = code;

      if(!c.empty)
      {
        vec2 xy1 = pos + s * vec2(x + c.x1, c.y1)
            , xy2 = pos + s * vec2(x + c.x2, c.y2);

        Val wh = vec2(c.u2 - c.u1, c.v2 - c.v1) / (xy2 - xy1)
            , uv1 = wh * (crop1 - xy1) * vec2(glm::greaterThan(crop1, xy1))
            , uv2 = wh * (xy2 - crop2) * vec2(glm::lessThan(crop2, xy2));

        Val aspect = Window::Get().aspect();

        xy1 = glm::clamp(xy1, crop1, crop2) * aspect;
        xy2 = glm::clamp(xy2, crop1, crop2) * aspect;

        Val x1 = packHalf1x16(xy1.x)
            , x2 = packHalf1x16(xy2.x)
            , y1 = packHalf1x16(xy1.y)
            , y2 = packHalf1x16(xy2.y)
            , u1 = packHalf1x16(c.u1 + uv1.x)
            , u2 = packHalf1x16(c.u2 - uv2.x)
            , v1 = packHalf1x16(c.v1 + uv1.y)
            , v2 = packHalf1x16(c.v2 - uv2.y);

        xyzw = copy(xyzw, array<uint16, 16>{ x1, y1, z, 0,  x2, y1, z, 0,
                                             x2, y2, z, 0,  x1, y2, z, 0 });
        uv = copy(uv, array<uint16, 8>{ u1, v1,  u2, v1,  u2, v2,  u1, v2 });
      }

      x += c.adv;
    }
  }

  if(state & State::rgba)
  {
    Val color = glm::round(glm::clamp(m_color, vec4(0), vec4(1)) * 255.f);
    Val a = array<GLubyte, 4>{{ GLubyte(color.r), GLubyte(color.g), GLubyte(color.b), GLubyte(color.a) }};

    for(uint i=0; i<m_vert_c; ++i)
      rgba = copy(rgba, a);
  }
}

pair<vec2, uint> Text::GetSizeFor(String text, Font const*font, float scale, float max_width, int max_glyphs)
{
  if(text.empty())
    return { vec2(0), 0 };

  CASSERT(utf8::is_valid(text.cbegin(), text.cend()), "Non-utf8 string");

  Val with_empty = max_width > -0.5 || max_glyphs > -1;
  Val s = scale / (font->topline() - font->bottomline());
  float w = -font->charData(utf8::unchecked::peek_next(text.cbegin())).x1;
  uint g = 0;

  uint last_char = 0;
  for(auto i=text.begin(); i!=text.cend();)
  {
    Val code = utf8::unchecked::next(i);
    Val c = font->charData(code);
    Val a = c.adv + font->kerning(last_char, code);

    if((max_width > -0.5 &&
        (w + a) * s > max_width) ||
       (max_glyphs > -1 &&
        g >= cast<uint>(max_glyphs)))
      break;

    g += !c.empty || with_empty;
    w += a;
    last_char = code;
  }

  if(last_char)
  {
    Val c = font->charData(last_char);
    if(!c.empty)
      w += c.x2 - c.adv;
  }

  return { vec2(w * s, scale), g };
}
