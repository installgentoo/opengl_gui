#pragma once
#include "base_classes/policies/code.h"
#include <glm/vec2.hpp>

namespace GUI
{

struct LineEdit
{
  void Draw(struct Renderer &r, struct Theme const&t, vec2 pos, vec2 size);

  string8 text;
private:
  bool m_hovered = false, m_focused = false;
  float m_scale;
  int m_cursor;
  vec2 m_offset, m_size;
  string8 m_old_text;
};

}
