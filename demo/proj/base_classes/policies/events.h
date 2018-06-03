#pragma once
#include "code.h"
#include <glm/vec2.hpp>

namespace code_policy
{

struct Event
{
  struct Type { enum : uint { MouseMove = 0, MouseButton = 1, Key = 2, Scroll = 3, Unichar = 4, Defocus = 5 }; };
  struct State { enum : uint { Mod = 0xf, Shift = 0x1, Ctrl = 0x2, Alt = 0x4, Win = 0x8,
                               Action = 0xf0, Release = 0x10, Press = 0x20, Repeat = 0x40 }; };

  struct MouseMove   { vec2 p;    };
  struct MouseButton { uint c, s; };
  struct Key         { uint c, s; };
  struct Scroll      { vec2 p;    };
  struct Unichar     { uint c;    };

  Event(MouseMove data)   : m_type(Type::MouseMove), m_data(move(data)) { }
  Event(MouseButton data) : m_type(Type::MouseButton), m_data(move(data)) { }
  Event(Key data)         : m_type(Type::Key), m_data(move(data)) { }
  Event(Scroll data)      : m_type(Type::Scroll), m_data(move(data)) { }
  Event(Unichar data)     : m_type(Type::Unichar), m_data(move(data)) { }
  Event()                 : m_type(Type::Defocus) { }

  Val mouse_move()const   { CASSERT(m_type == Type::MouseMove, "Wrong event type");   return m_data.mouse_move.p; }
  Val mouse_button()const { CASSERT(m_type == Type::MouseButton, "Wrong event type"); return m_data.mouse_button; }
  Val key()const          { CASSERT(m_type == Type::Key, "Wrong event type");         return m_data.key;          }
  Val scroll()const       { CASSERT(m_type == Type::Scroll, "Wrong event type");      return m_data.scroll.p;     }
  uint unichar()const     { CASSERT(m_type == Type::Unichar, "Wrong event type");     return m_data.unichar.c;    }

  uint type()const { return m_type; }

private:
  uint m_type;
  union Data {
    MouseMove mouse_move;
    MouseButton mouse_button;
    Key key;
    Scroll scroll;
    Unichar unichar;
    Data(MouseMove d)   : mouse_move(move(d)) { }
    Data(MouseButton d) : mouse_button(move(d)) { }
    Data(Key d)         : key(move(d)) { }
    Data(Scroll d)      : scroll(move(d)) { }
    Data(Unichar d)     : unichar(move(d)) { }
    Data() { }
  } m_data;
};

}
