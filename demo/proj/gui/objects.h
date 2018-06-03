#pragma once
#include "base_classes/policies/logging.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace code_policy { struct GLbindingVao; struct Vtex; struct Font; }

namespace GUI
{

typedef string const& String;
typedef vec2 const&   Vec2;
typedef vec4 const&   Vec4;

struct State { enum : uint { resized = 0x1, mismatch = 0x2,
                             xyzw = 0x10, rgba = 0x20, uv = 0x30,
                             full = xyzw | rgba | uv | resized }; };

struct Obj
{
  struct Anchor { enum : uint { H = 0xf, Left = 0x0, Middle = 0x1, Right = 0x2,
                                V = 0xf0, Bottom = 0x0, Center = 0x10, Top = 0x20 }; };

  virtual ~Obj() = default;

  Val size()const { return m_size; }
  vec4 bounding_box()const;
  bool intersect(Obj const&)const;

  static bool opaque(Vec4 color) { return color.a >= 0.996; }
  virtual uint vert_count()const { return 4;                }
  virtual bool ordered()const    { return !opaque(m_color); }
  virtual vector<uint16> genIdx(uint, uint)const;

  virtual void Draw(GLbindingVao const&, uint16, uint16)const;

  template<class T> using it = typename vector<T>::iterator;
  virtual void genMesh(float, uint, it<uint16>, it<ubyte>, it<uint16>)const = 0;

  virtual uint compare(Vec4, Vec2, Vec2,                       Vec4=vec4(1))const { return State::mismatch; }
  virtual uint compare(Vec4, Vec2, Vec2, String,               Vec4=vec4(1))const { return State::mismatch; }
  virtual uint compare(Vec4, Vec2, Vec2, Vtex const*,          Vec4=vec4(1))const { return State::mismatch; }
  //  virtual uint compare(Vec4, Vec2, Vec2, float, uint,          Vec4=vec4(1))const { return State::mismatch; }
  virtual uint compare(Vec4, Vec2, String, Font const*, float, Vec4=vec4(1))const { return State::mismatch; }

  virtual bool batchable(struct Rect const&)const   { return false; }
  virtual bool batchable(struct Sprite const&)const { return false; }
  //virtual bool batchable(struct 9Sprite const&)const  { return false; }
  virtual bool batchable(struct Text const&)const   { return false; }
  virtual bool check_batchable(Obj const&)const = 0;

protected:
  Obj(Vec4 crop, Vec2 pos, Vec2 size, Vec4 color);
  vec2 m_pos, m_size;
  vec4 m_color, m_crop;
};


struct Rect : Obj
{
  void genMesh(float, uint, it<uint16>, it<ubyte>, it<uint16>)const;

  uint compare(Vec4, Vec2, Vec2, Vec4=vec4(1))const;

  bool batchable(Rect const&r)const { return r.ordered() == this->ordered(); }
  bool check_batchable(Obj const&r)const { return r.batchable(*this); }

  static Rect Make(Vec4 crop, Vec2 pos, Vec2 size, Vec4 color=vec4(1))
  { return { crop, pos, size, color }; }
private:
  using Obj::Obj;
};


struct Sprite : Obj
{
  bool ordered()const;
  void Draw(GLbindingVao const&, uint16, uint16)const;

  void genMesh(float, uint, it<uint16>, it<ubyte>, it<uint16>)const;

  uint compare(Vec4, Vec2, Vec2, Vtex const*, Vec4=vec4(1))const;

  bool batchable(Sprite const&r)const { return r.m_atlas_idx == m_atlas_idx; }
  bool check_batchable(Obj const&r)const { return r.batchable(*this); }

  static Sprite Make(Vec4 crop, Vec2 pos, Vec2 size, Vtex const*tex, Vec4 color=vec4(1))
  { return { crop, pos, size, tex, color }; }
private:
  Sprite(Vec4 crop, Vec2 pos, Vec2 size, Vtex const*tex, Vec4 color);
  uint m_atlas_idx;
  Vtex const*m_tex;
};


/*struct 9Sprite : Obj
{
  uint vert_count()const { return 16;   }
  bool ordered()const    { return true; }
  vector<uint16> genIdx(uint, uint)const;
  void Draw(GLbindingVao const&, uint16, uint16)const;

  void genMesh(float, uint, it<uint16>, it<ubyte>, it<uint16>)const;

  uint compare(Vec4, Vec2, Vec2, float, uint, Vec4 =vec4(1))const;

  bool batchable(9Sprite const&)const { return true; }
  bool check_batchable(Obj const&r)const { return r.batchable(*this); }

  static 9Sprite Make(vec4 crop, vec2 pos, vec2 size, float corner, uint theme, vec4 color=vec4(1))
  { return { move(crop), move(pos), move(size), corner, theme, move(color) }; }

  static string themes_file;
private:
  9Sprite(vec4 crop, vec2 pos, vec2 size, float corner, uint theme, vec4 color)
    : Obj(move(crop), move(pos), move(size), move(color))
    , m_theme(theme)
    , m_corner(corner)
  { }
  uint m_theme;
  float m_corner;
};*/


struct Text : Obj
{
  uint vert_count()const { return m_vert_c; }
  bool ordered()const    { return true;     }
  void Draw(GLbindingVao const&, uint16, uint16)const;

  void genMesh(float, uint, it<uint16>, it<ubyte>, it<uint16>)const;

  uint compare(Vec4, Vec2, String, Font const*, float, Vec4=vec4(1))const;

  bool batchable(Text const&)const { return true; }
  bool check_batchable(Obj const&r)const { return r.batchable(*this); }

  static pair<vec2, uint> GetSizeFor(String text, Font const*font, float scale, float max_width=-1., int max_glyphs=-1);
  static Text Make(Vec4 crop, Vec2 pos, String text, Font const*font, float scale, Vec4 color=vec4(1)) {
    Val size_and_count = GetSizeFor(text, font, scale);
    return { crop, pos, size_and_count.first, text, font, scale, color, size_and_count.second * 4 };
  }
private:
  Text(Vec4 crop, Vec2 pos, Vec2 size, String text, Font const*font, float scale, Vec4 color, uint vert)
    : Obj(crop, pos, size, color)
    , m_vert_c(vert)
    , m_scale(scale)
    , m_font(font)
    , m_text(move(text))
  { }
  uint m_vert_c;
  float m_scale;
  Font const*m_font;
  string8 m_text;
};

}
