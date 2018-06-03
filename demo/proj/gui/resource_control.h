#pragma once
#include "renderer.h"
#include "base_classes/font.h"
#include "base_classes/texture_atlas.h"

namespace GUI
{

inline vec4 to_rgba(uint c)
{
  return vec4((c & 0xff000000) >> 24, (c & 0xff0000) >> 16, (c & 0xff00) >> 8, c & 0xff) / vec4(255);
}

struct Theme
{
  uint easing = 200;

  vec4 background = to_rgba(0x596475A0)
      , background_focus = to_rgba(0x596475A0)
      , foreground = to_rgba(0x211C31FF)
      , foreground_focus = to_rgba(0x461E5CFF)
      , highlight = to_rgba(0xEDEDEDFF)
      , text_highlight = to_rgba(0x461E5CFF)
      , text = to_rgba(0xFBFFDBFF)
      , text_focus = to_rgba(0xFFFFFFFF);

  Font const*font;
};


struct G
{
  template<class T> static auto& Get(uint id)
  {
    static unordered_map<uint, T> storage;
    return storage[id];
  }

  static auto& theme()
  {
    static Theme s_theme;
    return s_theme;
  }

  static auto& renderer()
  {
    static Renderer s_renderer;
    return s_renderer;
  }

  template<class T, class...P> static Val Draw(uint id, P ...p)
  {
    auto &e = Get<T>(id);
    e.Draw(renderer(), theme(), move(p)...);
    return e;
  };

  template<class T, class...P> static void Draw(P ...p)
  {
    T::Draw(renderer(), theme(), move(p)...);
  };
};


struct TextureManager
{
  Vtex const* Register(string filename);
  void LoadRegisteredTextures(uint channels);

private:
  unordered_map<string, Vtex> m_vtex_objects;
  vector<string> m_filenames;
};


struct FontManager
{
  Font const* Register(pair<string, string> font_description);
  void LoadRegisteredFonts(bool cache=true);

private:
  unordered_map<string, Font> m_font_objects;
  map<string, string> m_font_descriptions;
};


struct Animation
{
  Animation(TextureManager &manager, char const*filename);

  Vtex const* currentFrame(double time);

private:
  vector<pair<Vtex const*, double>> m_frames;
};

}
