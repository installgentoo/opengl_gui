#pragma once
#include "objects.h"
#include "base_classes/gl/objects.h"

namespace code_policy { struct Event; }

namespace GUI
{

struct Renderer
{
  typedef void const* ObjectId;

  Renderer();
  ~Renderer();

  template<class T, class...P> void Draw(P ...p) {
    if(m_num < m_objects.size())
    {
      auto &curr = m_objects[m_num];
      Val state = curr.obj->compare(m_clip, p...);
      m_flush |= state;

      if(state)
        curr.obj = make_unique<T>(T::Make(m_clip, move(p)...));

      curr.state = state;
    }
    else
    {
      m_flush = State::full;
      m_objects.emplace_back(Object{ make_unique<T>(T::Make(m_clip, move(p)...)), State::mismatch, 0 });
    }

    ++m_num;
  }

  Val mouse_pos()const { return m_mouse_pos; }
  bool hovered();
  bool hovered(Vec4 bb);

  void ConsumeEvents(vector<Event> e);
  vector<Event> ProcessEvents();
  void Logic(function<bool(Event const&)> func, ObjectId id=0);
  void Logic(Vec4 bb, function<bool(Event const&)> func, ObjectId id=0);

  void Clip(Vec2 pos, Vec2 size);

  void Render();

  ObjectId focused_id = 0;
private:
  uint m_num = 0, m_flush = 0;
  vec2 m_mouse_pos = vec2(0), m_aspect = vec2(1);
  vec4 m_clip = vec4(-1, -1, 2, 2);
  GLvao m_vao;
  vector<vec2> m_interactions;
  vector<Event> m_events;

  struct LogicStorage;
  vector<LogicStorage> m_logics;

  template<GLenum m_type, class T>
  struct BufferStorage {
    void flush() {
      auto b = GLbind(vbo);
      b.AllocateBuffer(0, last_size);
      b.AllocateBuffer(buff);
      last_size = buff.size();
    }

    GLbuffer<m_type> vbo;
    uint last_size = 0;
    vector<T> buff;
  };
  BufferStorage<GL_ELEMENT_ARRAY_BUFFER, GLushort> m_idx;
  BufferStorage<GL_ARRAY_BUFFER, GLushort> m_xyzw, m_uv;
  BufferStorage<GL_ARRAY_BUFFER, GLubyte> m_rgba;

  struct Object {
    unique_ptr<Obj> obj;
    uint state, last_size;
  };
  vector<Object> m_objects;

  struct Batch;
  vector<Batch> m_batches;
};

}
