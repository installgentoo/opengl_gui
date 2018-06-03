#include "layout.h"
#include "../resource_control.h"
#include "base_classes/policies/window.h"

using namespace GUI;

void Layout::Draw(Renderer &r, Theme const&t, function<void(vec2 const&, vec2 const&)> const&content)
{
  Val top_padding = 0.05
      , corner_padding = 0.04;

  Val win = Window::Get();
  size = glm::clamp(size, vec2(0), win.right_up() * 2.f - vec2(corner_padding));
  pos = glm::clamp(pos, win.left_bottom() + vec2(0, corner_padding), win.right_up() - size - vec2(corner_padding, 0));

  r.Clip(pos - vec2(0, corner_padding), size + vec2(corner_padding));

  r.Draw<Rect>(pos, size, t.background);
  r.Draw<Rect>(pos + vec2(0, size.y - top_padding), vec2(size.x, top_padding), t.highlight);
  r.Logic([this, &r](Val e){
    switch(e.type())
    {
      case Event::Type::MouseButton:
      {
        if(e.mouse_button().s & Event::State::Press)
        {
          m_pressed = true;
          m_click = r.mouse_pos() - pos;
        }

        if(e.mouse_button().s & Event::State::Release)
          m_pressed = false;

        return true;
      }

      case Event::Type::MouseMove:
      {
        if(m_pressed)
          pos = e.mouse_move() - m_click;
      }
    }
    return true;
  }, r.hovered() || m_pressed ? this : 0);

  r.Draw<Rect>(pos + vec2(size.x, -corner_padding), vec2(corner_padding), t.highlight);
  r.Logic([this, top_padding, corner_padding](Val e){
    switch(e.type())
    {
      case Event::Type::MouseButton:
      {
        if(e.mouse_button().s & Event::State::Press)
          m_corner_pressed = true;

        if(e.mouse_button().s & Event::State::Release)
          m_corner_pressed = false;

        return true;
      }

      case Event::Type::MouseMove:
      {
        if(m_corner_pressed)
        {
          Val mouse_pos = e.mouse_move();
          size = glm::max(vec2(top_padding), (mouse_pos - pos) * vec2(1, -1) + vec2(-corner_padding, size.y));
          pos.y = mouse_pos.y;
        }
      }
    }
    return true;
  }, r.hovered() || m_corner_pressed ? this : 0);

  content(pos, size - vec2(0, top_padding));
}
