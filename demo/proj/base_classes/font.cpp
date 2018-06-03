#include "font.h"
#include "texture_atlas.h"
#include "utility/sdf.h"
#include "policies/resource.h"
#include <glm/gtc/epsilon.hpp>
#include <utfcpp/utf8.h>
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>

using namespace code_policy;

static_assert(sizeof(ubyte) == sizeof(unsigned char), "Platform uchar size not supported by STBTT");

Font::CharData::CharData(float _adv, float _u1, float _v1, float _u2, float _v2, float _x1, float _x2, float _y1, float _y2)
  : empty(glm::any(glm::epsilonEqual(vec2(_y2 - _y1, _x2 - _x1), vec2(0), std::numeric_limits<float>::epsilon() * 2)))
  , adv(_adv), x1(_x1), x2(_x2), y1(_y1), y2(_y2)
  , u1(_u1), u2(_u2), v1(_v1), v2(_v2)
{ }

Font::CharData const& Font::charData(uint c)const
{
  Val found = m_font_map.find(c);
  if(found != m_font_map.cend())
    return found->second;

  CASSERT(0, "No character "<<[c]{ string s; utf8::append(c, std::back_inserter(s)); return s; }()<<" code "<<c);
  return m_font_map.cbegin()->second;
}

bool Font::exists(uint c) const
{
  Val found = m_font_map.find(c);

  return found != m_font_map.cend();
}

float Font::kerning(uint c1, uint c2)const
{
  Val found = m_kerning.find(c1);
  if(found == m_kerning.cend())
    return 0;

  Val kern = found->second.find(c2);
  if(kern == found->second.cend())
    return 0;

  return kern->second;
}


struct glyph {
  glyph(uint _i, int _x1, int _x2, int _y1, int _y2, float _a, float _l, int b, unordered_map<uint, float> _kern)
    : i(_i), x1(_x1 - b), x2(_x2 + b), y1(_y1 - b), y2(_y2 + b), a(_a), l(_l)
    , kern(move(_kern))
  { }

  uint i;
  int x1, x2, y1, y2;
  float a, l;
  unordered_map<uint, float> kern;
};

unordered_map<string, Font> code_policy::MakeFonts(map<string, string> font_descriptions, uint glyph_size, uint border_size, uint supersample_mult)
{
  Val border = border_size * supersample_mult;
  Val s = 1. / (glyph_size * supersample_mult + border * 2);
  Val sdf = SdfGenerator{};
  map<string, vector<glyph>> fonts_map;
  map<uint, uImage> glyph_images;

  for(Val font: font_descriptions)
  {
    Val font_file = Resource::Load(font.first);
    if(font_file.empty())
      CERROR("No font file "<<font.first);

    Val font_info = [&]{
      stbtt_fontinfo info;
      if(!stbtt_InitFont(&info, reinterpret_cast<unsigned char const*>(font_file.data()), 0))
        CERROR("Error initializing freetype");
      return info;
    }();

    Val alphabet = [&]{
      set<uint> a;
      Val str = font.second;
      for(auto i=str.begin(); i!=str.cend();)
        a.emplace(utf8::next(i, str.end()));
      return a;
    }();

    Val scale = stbtt_ScaleForPixelHeight(&font_info, glyph_size * supersample_mult);

    vector<glyph> glyph_set;
    for(Val i: alphabet)
    {
      Val g = stbtt_FindGlyphIndex(&font_info, cast<int>(i));
      int a, l, x1, y1, x2, y2;
      stbtt_GetGlyphHMetrics(&font_info, g, &a, &l);
      stbtt_GetGlyphBitmapBox(&font_info, g, scale, scale, &x1, &y1, &x2, &y2);
      Val w = x2 - x1, h = y2 - y1;
      Val adv = s * scale * a;
      CASSERT(w >= 0 && h >= 0, "Negative glyph size");

      if(w == 0 || h == 0)
      {
        glyph_set.emplace_back(i, 0, 0, 0, 0, adv, 0, 0, unordered_map<uint, float>{ });
        continue;
      }

      vector<ubyte> data(cast<size_t>(w) * cast<size_t>(h), 0);
      stbtt_MakeGlyphBitmap(&font_info, data.data(), cast<int>(w), cast<int>(h), cast<int>(w), scale, scale, g);

      Val _w = cast<uint>(w) + border * 2, _h = cast<uint>(h) + border * 2;
      uImage img = { _w, _h, 1, vector<ubyte>( size_t(_w) * _h, 0 ) };
      for(ptrdiff_t j=0; j<h; ++j)
        std::copy(data.cbegin() + j * w, data.cbegin() + (j+1) * w, img.data.begin() + (j + border) * img.width + border);

      Val tex = sdf.generate({ img, 1, 1 }, supersample_mult, border * 2);
      img = { tex.width(), tex.height(), 1, GLbind(tex).Save<ubyte>(1) };

      unordered_map<uint, float> kern;
      for(Val j: alphabet)
      {
        Val k = stbtt_GetGlyphKernAdvance(&font_info, g, stbtt_FindGlyphIndex(&font_info, cast<int>(j)));
        if(k != 0)
          kern.emplace(j, s * scale * k);
      }

      glyph_images.emplace(i, move(img));
      glyph_set.emplace_back(i, x1, x2, y1, y2, adv, scale * l, border, move(kern));
    }

    fonts_map.emplace(font.first, move(glyph_set));
  }

  Val max_tex_size = cast<uint>(GLState::Iconst<GL_MAX_TEXTURE_SIZE>());
  Val p = MakeAtlas(max_tex_size, max_tex_size, 1, move(glyph_images));
  if(!p.second.empty())
    CERROR("Sdf atlas is too big; reduce size or implement batching by atlas");

  Val atlas = p.first;
  Val texture = atlas.begin()->second.tex;

  unordered_map<string, Font> font_objects;
  for(Val font: fonts_map)
  {
    unordered_map<uint, unordered_map<uint, float>> kerning;
    unordered_map<uint, Font::CharData> font_obj;
    double topline = 0, bottomline = 0;
    for(Val g: font.second)
    {
      Val found = atlas.find(g.i);
      Val c = found != atlas.cend() ? found->second.coord : vec4(0);
      Val y1 = -s * g.y2
          , y2 = -s * g.y1;
      topline = glm::max(topline, y2);
      bottomline = glm::min(bottomline, y1);
      font_obj.emplace(g.i, Font::CharData(g.a, c.x, c.w, c.z, c.y, s * (-g.l + g.x1), s * (-g.l + g.x2), y1, y2));
      if(!g.kern.empty())
        kerning.emplace(g.i, g.kern);
    }

    font_objects[font.first] = { font_obj, kerning, topline, bottomline, texture };
  }

  return font_objects;
}
