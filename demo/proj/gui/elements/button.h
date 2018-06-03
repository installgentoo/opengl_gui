#pragma once
#include "base_classes/policies/code.h"
#include <glm/vec2.hpp>

namespace GUI
{

struct Button
{
  void Draw(struct Renderer &r, struct Theme const&t, vec2 pos, vec2 size, string8 const&text);

  bool pressed = false, hovered = false;
private:
  float m_scale, m_easing = 0.;
  vec2 m_offset, m_size;
  string8 m_text;
};

}
