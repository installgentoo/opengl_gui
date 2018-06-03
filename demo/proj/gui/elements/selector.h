#pragma once
#include "line_edit.h"
#include "button.h"

namespace GUI
{

struct Selector
{
  void Draw(struct Renderer &r, struct Theme const&t, vec2 pos, vec2 size, vector<string8> const&choices);

  string8 text;
  bool hovered = false, active = false;
private:
  string8 m_old_text;
  Button m_button;
  LineEdit m_line_edit;
  vector<Button> m_choices;
};

}
