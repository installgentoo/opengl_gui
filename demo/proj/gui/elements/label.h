#pragma once
#include "base_classes/policies/code.h"
#include <glm/vec2.hpp>

namespace GUI
{

struct Label
{
  void Draw(struct Renderer &r, struct Theme const&t, vec2 pos, vec2 size, string8 text);

private:
  float m_scale;
  vec2 m_offset, m_size;
  string8 m_old_text;
};

}
