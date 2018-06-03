#pragma once
#include "policies/code.h"
#include "gl/texture.h"

namespace GUI { struct FontManager; }

namespace code_policy
{

struct Font
{
  friend struct GUI::FontManager;

  template<class A> void serialize(A &a) { a(m_topline, m_bottomline, m_font_map, m_kerning); }

  struct CharData {
    template<class A> void serialize(A &a) { a(empty, adv, x1, x2, y1, y2, u1, u2, v1, v2); }

    CharData() = default;
    CharData(float, float, float, float, float, float, float, float, float);

    bool empty;
    float adv, x1, x2, y1, y2, u1, u2, v1, v2;
  };

  Font() = default;
  Font(unordered_map<uint, CharData> font_map, unordered_map<uint, unordered_map<uint, float>> kerning, double topline, double bottomline, shared_ptr<GLtex2d> tex)
    : m_topline(topline)
    , m_bottomline(bottomline)
    , m_tex(move(tex))
    , m_font_map(move(font_map))
    , m_kerning(move(kerning))
  { }

  Val tex()const          { return *m_tex;       }
  float topline()const    { return m_topline;    }
  float bottomline()const { return m_bottomline; }
  bool exists(uint c)const;
  float kerning(uint c1, uint c2)const;
  Font::CharData const& charData(uint c)const;

private:
  float m_topline, m_bottomline;
  shared_ptr<GLtex2d> m_tex;
  unordered_map<uint, CharData> m_font_map;
  unordered_map<uint, unordered_map<uint, float>> m_kerning;
};


unordered_map<string, Font> MakeFonts(map<string, string> font_descriptions, uint glyph_size, uint border_size, uint supersample_mult);

}
