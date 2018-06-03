#include "label.h"
#include "../resource_control.h"
#include "base_classes/policies/window.h"

using namespace GUI;

void Label::Draw(Renderer &r, Theme const&t, vec2 pos, vec2 size, string8 text)
{
  Val text_padding = 1.f / 10;

  r.Clip(pos, size);

  if(m_old_text != text ||
     !Window::Get().equalPos(m_size, size))
  {
    m_old_text = text;
    m_size = size;

    Val padding = size * text_padding;
    Val text_size = vec2(Text::GetSizeFor(text, t.font, size.y).first.x, size.y) + padding;
    Val scale_coeff = size / text_size;
    Val scale = glm::min(scale_coeff.x, scale_coeff.y);
    m_scale = size.y * scale;
    m_offset = vec2(size - (text_size - padding) * scale) / 2.f;
  }

  r.Draw<Rect>(pos, size, t.foreground);

  pos += m_offset;

  r.Draw<Text>(pos, move(text), t.font, m_scale, t.text);
}
