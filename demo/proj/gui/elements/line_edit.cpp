#include "line_edit.h"
#include "../resource_control.h"
#include "base_classes/policies/window.h"
#include <utfcpp/utf8.h>
#include <GLFW/glfw3.h>

using namespace GUI;

void LineEdit::Draw(Renderer &r, Theme const&t, vec2 pos, vec2 size)
{
  Val text_padding = 1.f / 10
      , cursor_padding = 0.01f;

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

  r.Draw<Rect>(pos, size, t.background);

  pos += m_offset;

  r.Logic([this, pos, &t, &r](Val e){

    Val calc_cursor = [&](Val click){
      Val p = click - pos;
      return cast<int>(Text::GetSizeFor(text, t.font, m_scale, glm::max(p.x, 0.f)).second);
    };

    Val set_cursor = [&](Val c){
      return cast<int>(Text::GetSizeFor(text, t.font, m_scale, -1, glm::max(c, 0)).second);
    };

    Val find_text_iter = [&](Val c){
      auto adv = text.begin();
      utf8::unchecked::advance(adv, glm::min(c, cast<int>(text.size())));
      return adv;
    };

    switch(e.type())
    {
      case Event::Type::Defocus:
      {
        m_focused = false;
        return false;
      }

      case Event::Type::MouseButton:
      {
        if(!(e.mouse_button().s & Event::State::Release))
        {
          m_focused = true;
          m_cursor = calc_cursor(r.mouse_pos());
        }
        return true;
      }

      case Event::Type::Key:
      {
        if(!m_focused)
          return false;

        if(e.key().s & Event::State::Release)
          return true;

        switch(e.key().c)
        {
          case GLFW_KEY_RIGHT:
          {
            m_cursor = set_cursor(m_cursor + 1);
            return true;
          }

          case GLFW_KEY_LEFT:
          {
            m_cursor = set_cursor(m_cursor - 1);
            return true;
          }

          case GLFW_KEY_DELETE:
          {
            auto adv = find_text_iter(m_cursor);
            if(adv == text.cend())
              return true;

            const auto begin = adv;
            utf8::unchecked::next(adv);
            text.erase(begin, adv);
            return true;
          }

          case GLFW_KEY_BACKSPACE:
          {
            auto adv = find_text_iter(m_cursor);
            if(adv == text.cbegin())
              return true;

            const auto end = adv;
            utf8::unchecked::previous(adv);
            text.erase(adv, end);
            m_cursor = set_cursor(m_cursor - 1);
            return true;
          }
        }
        return false;
      }

      case Event::Type::Unichar:
      {
        if(!t.font->exists(e.unichar()))
          return true;

        utf8::unchecked::append(e.unichar(), std::inserter(text, find_text_iter(m_cursor)));
        m_cursor = set_cursor(m_cursor + 1);
        return true;
      }
    }
    return false;
  }, this + sizeof(*this));
  m_hovered = r.hovered();

  if(m_focused)
  {
    Val w_cursor = Text::GetSizeFor(text, t.font, m_scale, -1, m_cursor).first.x;
    r.Draw<Rect>(pos + vec2(w_cursor, 0), vec2(cursor_padding, m_scale), t.highlight);
  }

  r.Draw<Text>(pos, text, t.font, m_scale, t.text);
}
