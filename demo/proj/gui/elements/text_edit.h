#pragma once
#include "slider.h"

namespace GUI
{

struct TextEdit
{
  void Draw(struct Renderer &r, struct Theme const&t, vec2 pos, vec2 size, float scale, bool readonly=false);

  void write_history() {
    cursor = selection = ivec2(-1);
    m_history.increment(text, cursor);
  }

  string8 text;
  ivec2 cursor, selection;
private:
  struct History {
    void restore(string8 &text, ivec2 &cursor);
    void repeat_increment(string8 &text, ivec2 &cursor);
    void increment(string8 const&text, ivec2 const&cursor);

  private:
    uint m_at = 0;
    deque<pair<string8, ivec2>> m_history;
  };

  bool m_hovered = false, m_pressed = false, m_focused = false;
  float m_scale = 0;
  ivec2 m_old_cursor;
  vec2 m_size;
  string8 m_old_text;
  set<int> m_wraps;
  vector<string8> m_lines;
  History m_history;
  VerticalSlider m_scrollbar;
};

}
