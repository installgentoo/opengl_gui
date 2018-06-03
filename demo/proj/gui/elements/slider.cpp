#include "slider.h"
#include "../resource_control.h"
#include "base_classes/policies/events.h"

using namespace GUI;

void VerticalSlider::Draw(Renderer &r, Theme const&t, vec2 pos, vec2 size, float bar_height)
{
  r.Clip(pos, size);

  r.Draw<Rect>(pos, size, t.foreground);
  r.Logic([this, &r, pos, size, bar_height](Val e){
    Val set_bar = [&](Val y){ bar = glm::clamp((y - pos.y) / size.y - bar_height / 2, 0.f, 1.f - bar_height); };

    switch(e.type())
    {
      case Event::Type::MouseButton:
        if(e.mouse_button().s & Event::State::Release)
          m_pressed = false;
        else
        {
          m_pressed = true;
          set_bar(r.mouse_pos().y);
        }
        return true;

      case Event::Type::MouseMove:
        if(m_pressed)
        {
          set_bar(e.mouse_move().y);
          return true;
        }
        break;
    }
    return false;
  }, this + sizeof(*this));

  r.Draw<Rect>(pos + vec2(0, bar * size.y), size * vec2(1, bar_height), t.highlight);
}


void HorizontalSlider::Draw(Renderer &r, Theme const&t, vec2 pos, vec2 size, float bar_width)
{
  r.Clip(pos, size);

  r.Draw<Rect>(pos, size, t.foreground);
  r.Logic([this, &r, pos, size, bar_width](Val e){
    Val set_bar = [&](Val x){ bar = glm::clamp((x - pos.x) / size.x - bar_width / 2, 0.f, 1.f - bar_width); };

    switch(e.type())
    {
      case Event::Type::MouseButton:
        if(e.mouse_button().s & Event::State::Release)
          m_pressed = false;
        else
        {
          m_pressed = true;
          set_bar(r.mouse_pos().x);
        }
        return true;

      case Event::Type::MouseMove:
        if(m_pressed)
        {
          set_bar(e.mouse_move().x);
          return true;
        }
        break;
    }
    return false;
  }, this + sizeof(*this));

  r.Draw<Rect>(pos + vec2(bar * size.x, 0), size * vec2(bar_width, 1), t.highlight);
}
