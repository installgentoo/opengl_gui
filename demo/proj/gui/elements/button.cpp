#include "button.h"
#include "../resource_control.h"
#include "base_classes/policies/window.h"

using namespace GUI;

void Button::Draw(Renderer &r, Theme const&t, vec2 pos, vec2 size, string8 const&text)
{
  Val text_padding = 1.f / 10;

  r.Clip(pos, size);

  if(m_text != text ||
     !Window::Get().equalPos(m_size, size))
  {
    m_text = text;
    m_size = size;

    Val padding = size * text_padding;
    Val text_size = Text::GetSizeFor(text, t.font, size.y).first + padding;
    Val scale_coeff = size / text_size;
    Val scale = glm::min(scale_coeff.x, scale_coeff.y);
    m_scale = size.y * scale;
    m_offset = vec2(size - (text_size - padding) * scale) / 2.f;
  }

  Val delta = (1. / (t.easing * glm::abs(m_easing - 2.f)));
  m_easing += hovered ? delta : -delta;
  m_easing = glm::clamp(m_easing, 0.f, 1.f);

  Val color = glm::mix(t.foreground, t.foreground_focus, m_easing);

  r.Draw<Rect>(pos, size, pressed ? t.highlight : color);
  r.Logic([this](Val e){
    if(e.type() != Event::Type::MouseButton)
      return false;

    pressed = !(e.mouse_button().s & Event::State::Release);

    return true;
  });
  hovered = r.hovered();
  pressed &= hovered;

  r.Draw<Text>(pos + m_offset, text, t.font, m_scale, pressed ? t.text_highlight : hovered ? t.text_focus : t.text);
}
