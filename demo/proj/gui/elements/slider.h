#pragma once
#include "base_classes/policies/code.h"
#include <glm/vec2.hpp>

namespace GUI
{

struct VerticalSlider
{
  void Draw(struct Renderer &r, struct Theme const&t, vec2 pos, vec2 size, float bar_height);

  float bar = 0;
private:
  bool m_pressed = false;
};


struct HorizontalSlider
{
  void Draw(struct Renderer &r, struct Theme const&t, vec2 pos, vec2 size, float bar_width);

  float bar = 0;
private:
  bool m_pressed = false;
};

}
