#include "selector.h"
#include "../resource_control.h"
#include "base_classes/policies/window.h"
#include <GLFW/glfw3.h>

using namespace GUI;

void Selector::Draw(Renderer &r, Theme const&t, vec2 pos, vec2 size, vector<string8> const&choices)
{
  Val total_size = size * vec2(1, active ? 1 + choices.size() : 1);
  hovered = r.hovered({ pos, pos + total_size });
  active &= hovered;

  if(!active)
  {
    m_button.Draw(r, t, pos, size, text);
    if(!m_button.pressed)
      return;

    m_button.pressed = false;
    active = true;
    m_line_edit.text = text;
  }
  else
  {
    m_choices.resize(choices.size());
    for(uint i=0; i<m_choices.size(); ++i)
    {
      m_choices[i].Draw(r, t, pos + size * vec2(0, 1 + i), size, choices[i]);
      if(m_choices[i].pressed)
      {
        text = choices[i];
        active = false;
        return;
      }
    }

    m_line_edit.Draw(r, t, pos, size);
    r.Logic({ pos, pos + size }, [this](Val e){
      if(e.type() == Event::Type::Key &&
         e.key().c == GLFW_KEY_ENTER)
      {
        text = m_line_edit.text;
        active = false;
        return true;
      }

      return false;
    });
  }
}
