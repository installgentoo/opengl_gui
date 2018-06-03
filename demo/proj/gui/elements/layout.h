#pragma once
#include "base_classes/policies/code.h"
#include <glm/vec2.hpp>

namespace GUI
{

struct Layout
{
  void Draw(struct Renderer &r, struct Theme const&t, function<void(vec2 const&, vec2 const&)> const&content);

  vec2 pos = vec2(0), size = vec2(1);
private:
  vec2 m_click;
  bool m_pressed = false, m_corner_pressed = false;
};

}
