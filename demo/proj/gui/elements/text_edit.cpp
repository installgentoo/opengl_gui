#include "text_edit.h"
#include "../resource_control.h"
#include "base_classes/policies/window.h"
#include <utfcpp/utf8.h>
#include <GLFW/glfw3.h>
#include <sstream>

using namespace GUI;

static void parse_text(vector<string8> &lines, set<int> &wraps, string8 const&text, Font const*font, float scale, float max_w)
{
  wraps.clear();
  lines.clear();
  std::stringstream ss(text);
  string8 line;
  while(std::getline(ss, line, '\n'))
  {
    if(line.empty())
      lines.emplace_back("");
    else
      while(!line.empty())
      {
        Val last_fit = glm::max(1u, Text::GetSizeFor(line, font, scale, max_w).second);
        auto start = line.cbegin();
        utf8::unchecked::advance(start, last_fit);
        lines.emplace_back(line.cbegin(), start);
        line.erase(line.cbegin(), start);
        if(!line.empty())
          wraps.emplace(cast<int>(lines.size()) - 1);
      }
  }
  lines.emplace_back("");
}


void TextEdit::Draw(Renderer &r, Theme const&t, vec2 pos, vec2 size, float scale, bool readonly)
{
  Val scroll_padding = 0.02
      , cursor_padding = 0.01;

  r.Clip(pos, size);

  Val numbers_bar_w = glm::min(double(size.x), t.font->charData('0').adv * (m_scale / (t.font->topline() - t.font->bottomline())) * (glm::floor(std::log10(glm::max(cast<uint>(m_lines.size() - m_wraps.size()), 1u))) + 2));

  pos += vec2(numbers_bar_w, 0);
  size = glm::max(vec2(0), size - vec2(scroll_padding + numbers_bar_w, 0));

  Val update_text = [this, &t]{
    m_old_text = text;
    parse_text(m_lines, m_wraps, text, t.font, m_scale, m_size.x);
  };

  Val window = Window::Get();
  if(m_old_text != text ||
     !window.equalPos(m_scale, scale) ||
     !window.equalPos(m_size, size))
  {
    m_scale = scale;
    m_size = size;
    update_text();
  }

  Val max_line = [this]{ return glm::max(cast<int>(m_lines.size()) - 1, 0); };
  Val line = [this, max_line](Val at){ return m_lines.empty() ? string8{} : m_lines[cast<uint>(glm::clamp(at, 0, max_line()))]; };

  Val whole_text_size = m_scale * m_lines.size()
      , visible_part = m_size.y / whole_text_size;

  if(m_old_cursor != cursor)
  {
    m_old_cursor = cursor;
    cursor = glm::max(cursor, ivec2(0));
    Val screen_y = (float(m_lines.size()) - (cursor.y + 1)) / m_lines.size();
    m_scrollbar.bar = glm::clamp(m_scrollbar.bar, screen_y - visible_part + 1.f / m_lines.size(), screen_y);
  }
  selection = glm::max(selection, ivec2(0));
  m_scrollbar.bar = glm::clamp(m_scrollbar.bar, 0.f, 1.f - visible_part);

  r.Draw<Rect>(pos - vec2(numbers_bar_w, 0), size + vec2(scroll_padding + numbers_bar_w, 0), t.background);
  r.Logic([this, update_text, pos, &t, &r, max_line, line, whole_text_size, readonly](Val e){

    Val length = [](Val s){ return cast<int>(utf8::unchecked::distance(s.cbegin(), s.cend())); };

    Val calc_cursor = [&](Val click){
      Val p = click - pos + vec2(0, m_scrollbar.bar * whole_text_size);
      Val line_idx = cast<int>(m_lines.size()) - 1 - cast<int>(p.y / m_scale)
          , count = cast<int>(Text::GetSizeFor(line(line_idx), t.font, m_scale, glm::max(p.x, 0.f)).second);
      return ivec2(count, line_idx);
    };

    Val set_cursor = [&](Val c){
      Val h = glm::clamp(c.y, 0, max_line())
          , count = cast<int>(Text::GetSizeFor(line(c.y), t.font, m_scale, -1, glm::max(c.x, 0)).second);
      return ivec2(c.x == cursor.x ? cursor.x : count, h);
    };

    Val find_text_iter = [&](Val c){
      auto adv = text.begin();
      Val h = glm::min(c.y, max_line());
      for(int i=0; i<h; ++i)
      {
        adv += cast<int64>(line(i).size());
        adv += (*adv == '\n' ? 1 : 0);
      }
      utf8::unchecked::advance(adv, glm::min(c.x, length(line(c.y))));
      return adv;
    };

    Val selection_begin = [&]{
      Val seq = cursor.y != selection.y ? cursor.y > selection.y : cursor.x > selection.x;
      return seq ? selection : cursor;
    };

    Val selection_end = [&]{
      Val seq = cursor.y != selection.y ? cursor.y > selection.y : cursor.x > selection.x;
      return seq ? cursor : selection;
    };

    Val erase_selected = [&]{
      if(selection == cursor)
        return false;

      text.erase(find_text_iter(selection_begin()), find_text_iter(selection_end()));
      return true;
    };

    switch(e.type())
    {
      case Event::Type::Defocus:
      {
        m_focused = m_pressed = false;
        return false;
      }

      case Event::Type::MouseButton:
      {
        if(e.mouse_button().s & Event::State::Release)
          m_pressed = false;
        else
        {
          m_pressed = m_focused = true;
          cursor = calc_cursor(r.mouse_pos());

          if(!(e.mouse_button().s & Event::State::Shift))
            selection = cursor;
        }
        return true;
      }

      case Event::Type::MouseMove:
      {
        if(!m_pressed)
          return false;

        cursor = calc_cursor(e.mouse_move());
        return true;
      }

      case Event::Type::Scroll:
      {
        if(m_hovered)
          m_scrollbar.bar += e.scroll().y * m_scale * 0.5f;
        return true;
      }

      case Event::Type::Key:
      {
        if(!m_focused)
          return false;

        if(e.key().s & Event::State::Release)
          return true;

        Val shift_held = e.key().s & Event::State::Shift
            , ctrl_held =  e.key().s & Event::State::Ctrl;

        switch(e.key().c)
        {
          case GLFW_KEY_UP:
          {
            cursor = set_cursor(cursor + ivec2(0, -1));
            selection = shift_held ? selection : cursor;
            return true;
          }

          case GLFW_KEY_PAGE_UP:
          {
            cursor = set_cursor(cursor + ivec2(0, -40));
            selection = shift_held ? selection : cursor;
            return true;
          }

          case GLFW_KEY_DOWN:
          {
            cursor = set_cursor(cursor + ivec2(0, 1));
            selection = shift_held ? selection : cursor;
            return true;
          }

          case GLFW_KEY_PAGE_DOWN:
          {
            cursor = set_cursor(cursor + ivec2(0, 40));
            selection = shift_held ? selection : cursor;
            return true;
          }

          case GLFW_KEY_RIGHT:
          {
            cursor = set_cursor(cursor.x < length(line(cursor.y)) ? cursor + ivec2(1, 0) : ivec2(0, cursor.y + 1));
            selection = shift_held ? selection : cursor;
            return true;
          }

          case GLFW_KEY_LEFT:
          {
            cursor.x = glm::min(cursor.x, length(line(cursor.y)));
            cursor = set_cursor(cursor.x > 0 ? cursor + ivec2(-1, 0) :
                                               cursor.y > 0 ? ivec2(std::numeric_limits<int>::max(), cursor.y - 1) : cursor);
            selection = shift_held ? selection : cursor;
            return true;
          }

          case GLFW_KEY_A:
          {
            if(!ctrl_held)
              return true;

            selection = { 0, 0 };
            cursor = { length(line(max_line())), max_line() };
            return true;
          }

          case GLFW_KEY_V:
          {
            if(readonly ||
               !ctrl_held)
              return true;

            Val str = Window::Get().clipboard();
            if(str.empty())
              return true;

            erase_selected();
            selection = cursor = set_cursor(selection_begin());

            text.insert(find_text_iter(selection_begin()), str.cbegin(), str.cend());

            m_history.increment(text, cursor);
            return true;
          }

          case GLFW_KEY_C:
          {
            if(!ctrl_held ||
               selection == cursor)
              return true;

            Window::Get().setClipboard({ find_text_iter(selection_begin()), find_text_iter(selection_end()) });
            return true;
          }

          case GLFW_KEY_X:
          {
            if(readonly ||
               !ctrl_held ||
               selection == cursor)
              return true;

            Window::Get().setClipboard({ find_text_iter(selection_begin()), find_text_iter(selection_end()) });

            erase_selected();
            selection = cursor = set_cursor(selection_begin());

            m_history.increment(text, cursor);
            return true;
          }

          case GLFW_KEY_Z:
          {
            if(readonly ||
               !ctrl_held)
              return true;

            if(shift_held)
              m_history.repeat_increment(text, cursor);
            else
              m_history.restore(text, cursor);

            cursor = selection = cursor;
            return true;
          }

          case GLFW_KEY_ENTER:
          {
            if(readonly)
              return true;

            Val c = selection_begin();
            Val after_wrap = c.x == 0 && m_wraps.cend() != m_wraps.find(c.y - 1);
            erase_selected();
            text.insert(find_text_iter(c), '\n');

            cursor = selection = set_cursor(after_wrap ? c : ivec2(0, c.y + 1));

            m_history.increment(text, cursor);
            return true;
          }

          case GLFW_KEY_DELETE:
          {
            if(readonly)
              return true;

            Val c = selection_begin();
            auto adv = find_text_iter(c);

            if(erase_selected())
              cursor = selection = set_cursor(c);
            else
            {
              if(adv == text.cend())
                return true;

              Val w = glm::min(c.x, length(line(c.y)));

              const auto begin = adv;
              utf8::unchecked::next(adv);
              text.erase(begin, adv);
              cursor = selection = set_cursor(ivec2(w, c.y));
            }

            m_history.increment(text, cursor);
            return true;
          }

          case GLFW_KEY_BACKSPACE:
          {
            if(readonly)
              return true;

            Val c = selection_begin();
            auto adv = find_text_iter(c);

            if(erase_selected())
              cursor = selection = set_cursor(c);
            else
            {
              if(adv == text.cbegin())
                return true;

              const auto end = adv;
              Val erased_char = utf8::unchecked::previous(adv);
              text.erase(adv, end);

              Val up_line_idx = glm::max(0, c.y - 1);
              Val up_line_w = length(line(up_line_idx));

              Val x = up_line_w != length(line(up_line_idx)) || erased_char == '\n' ? up_line_w : up_line_w - 1;
              cursor = selection = set_cursor(c.x > 0 ? c + ivec2(-1, 0) : ivec2(x, c.y - 1));
            }

            m_history.increment(text, cursor);
            return true;
          }
        }
        return false;
      }

      case Event::Type::Unichar:
      {
        if(readonly ||
           !t.font->exists(e.unichar()))
          return true;

        Val c = selection_begin();
        erase_selected();
        utf8::unchecked::append(e.unichar(), std::inserter(text, find_text_iter(c)));

        update_text();

        Val wrap = m_wraps.cend() == m_wraps.find(c.y);
        cursor = selection = set_cursor(wrap ? c + ivec2(1, 0) : ivec2(1, c.y + 1));

        m_history.increment(text, cursor);
        return true;
      }
    }
    return false;
  }, this + sizeof(*this));
  m_hovered = r.hovered();

  Val line_pos = [&](Val n){ return (1. - m_scrollbar.bar) * whole_text_size - scale * (n + 1); };
  Val visible =  [&](Val p){ return (p.y < pos.y + size.y) && (p.y + scale > pos.y); };

  for(Val i: m_wraps)
  {
    Val p = pos + vec2(size.x - cursor_padding, line_pos(i)) - vec2(cursor_padding);
    if(visible(p))
      r.Draw<Rect>(p, vec2(cursor_padding), t.highlight);
  }

  if(cursor != selection)
  {
    Val seq = cursor.y != selection.y ? cursor.y > selection.y : cursor.x > selection.x;
    Val w_selection = Text::GetSizeFor(line(selection.y), t.font, m_scale, -1, selection.x).first.x
        , w_cursor = Text::GetSizeFor(line(cursor.y), t.font, m_scale, -1, cursor.x).first.x;
    Val w_begin = seq ? w_selection : w_cursor;
    Val w_end =   seq ? w_cursor : w_selection;

    Val begin = glm::min(cursor.y, selection.y)
        , end = glm::max(cursor.y, selection.y);

    for(int i=begin; i<=end; ++i)
    {
      Val w = i != begin ? 0 : w_begin;
      Val p = pos + vec2(w, line_pos(i));
      if(visible(p))
        r.Draw<Rect>(p, vec2(i != end ? size.x : w_end - w, scale), t.highlight);
    }
  }
  else
    if(m_focused)
    {
      Val w_cursor = Text::GetSizeFor(line(cursor.y), t.font, m_scale, -1, cursor.x).first.x;

      Val p = pos + vec2(w_cursor, line_pos(cursor.y));
      if(visible(p))
        r.Draw<Rect>(p, vec2(cursor_padding, scale), t.highlight);
    }

  if(whole_text_size > size.y)
    m_scrollbar.Draw(r, t, pos + vec2(size.x, 0), vec2(scroll_padding, size.y), visible_part);

  r.Clip(pos - vec2(numbers_bar_w, 0), size + vec2(numbers_bar_w, 0));

  r.Draw<Rect>(pos - vec2(numbers_bar_w, 0), vec2(numbers_bar_w, size.y), t.foreground);

  for(size_t i=0, j=1; i<m_lines.size(); ++i)
  {
    Val p = pos - vec2(numbers_bar_w, 0) + vec2(0, line_pos(i));
    if(m_wraps.cend() == m_wraps.find(i))
    {
      if(visible(p))
        r.Draw<Text>(p, std::to_string(j), t.font, scale, t.highlight);
      ++j;
    }
  }

  for(size_t i=0; i<m_lines.size(); ++i)
  {
    Val p = pos + vec2(0, line_pos(i));
    if(visible(p))
      r.Draw<Text>(p, m_lines[i], t.font, scale, t.text);
  }
}


void TextEdit::History::restore(string8 &text, ivec2 &cursor)
{
  if(m_history.empty())
    return;

  if(m_at)
    --m_at;

  Val h = m_history[m_at];
  text = h.first;
  cursor = h.second;
}

void TextEdit::History::repeat_increment(string8 &text, ivec2 &cursor)
{
  if(m_at + 1 >= m_history.size())
    return;

  ++m_at;
  Val h = m_history[m_at];
  text = h.first;
  cursor = h.second;
}

void TextEdit::History::increment(string8 const&text, ivec2 const&cursor)
{
  if(m_history.empty())
    m_history.emplace_back(text, cursor);

  if(m_history.back().first == text)
    return;

  m_history.resize(m_at + 1);

  if(m_history.size() >= 500)
    m_history.pop_front();

  m_at = m_history.size();

  m_history.emplace_back(text, cursor);
}
