#include "renderer.h"
#include "base_classes/policies/window.h"
#include <glm/gtc/epsilon.hpp>
#include <GLFW/glfw3.h>
#include <numeric>
#include <algorithm>

using namespace GUI;

static bool contains(Vec4 bb, Vec2 p)
{
  return !(p.x < bb.x || p.x > bb.z ||
           p.y < bb.y || p.y > bb.w);
}


struct Renderer::LogicStorage {
  vec4 box;
  ObjectId id;
  function<bool(Event const&)> func;
};


struct Renderer::Batch {
  using Objs = vector<Object> const&;

  Batch(uint z)
    : indices({ z })
  { }

  Val front(Objs objs)const {
    return *objs[indices.front()].obj;
  }

  bool contains(Objs objs, Obj const&o, uint z)const {
    return front(objs).check_batchable(o) && std::binary_search(indices.cbegin(), indices.cend(), z);
  }

  bool covered(Objs objs, Obj const&o)const {
    return o.ordered() && front(objs).ordered() &&
        indices.cend() != std::find_if(indices.cbegin(), indices.cend(), [&](uint i){ return objs[i].obj->intersect(o); });
  }

  bool covered(Objs objs, Obj const&o, uint z)const {
    if(!front(objs).ordered() ||
       front(objs).check_batchable(o))
      return false;

    Val begin = std::find_if(indices.crbegin(), indices.crend(), [&](uint i){ return i < z; }).base();
    return indices.cend() != std::find_if(begin, indices.cend(), [&](uint i){ return objs[i].obj->intersect(o); });
  }

  bool covers(Objs objs, Obj const&o, uint z)const {
    if(!front(objs).ordered() ||
       front(objs).check_batchable(o))
      return false;

    Val end = std::find_if(indices.cbegin(), indices.cend(), [&](uint i){ return i > z; });
    return end != std::find_if(indices.cbegin(), end, [&](uint i){ return objs[i].obj->intersect(o); });
  }

  bool try_to_add(Objs objs, Obj const&o, uint z) {
    if(!front(objs).check_batchable(o))
      return false;

    indices.emplace_back(z);
    return true;
  }

  bool shrink(uint z) {
    Val begin = std::find_if(indices.crbegin(), indices.crend(), [&](uint i){ return i < z; }).base();
    indices.erase(begin, indices.cend());
    return indices.empty();
  }

  auto redraw(vector<Object> &objs, uint first_invalid_index) {
    Val expand = [](auto &v, uint b, uint s){ v.insert(v.cbegin() + b, s, 0);          };
    Val erase =  [](auto &v, uint s, uint e){ v.erase(v.cbegin() + s, v.cbegin() + e); };

    uint flush = 0, start = 0;

    for(Val i: indices)
    {
      auto &obj = objs[i];
      auto state = i < first_invalid_index ? obj.state : State::mismatch;

      if(!state)
      {
        start += obj.last_size;
        continue;
      }

      Val o = *obj.obj;
      Val size = o.vert_count();

      if(state & State::mismatch)
      {
        Val to = start + size;
        xyzw.resize(to * 4);
        rgba.resize(to * 4);
        uv.resize(to * 2);
        state = State::full;
      }
      else
      {
        Val old_size = obj.last_size;

        if(size > old_size)
        {
          Val at = start + old_size
              , s = size - old_size;
          expand(xyzw, at * 4, s * 4);
          expand(rgba, at * 4, s * 4);
          expand(uv,   at * 2, s * 2);
          state = State::full;
        }

        if(size < old_size)
        {
          Val from = start + size
              , to = start + old_size;
          erase(xyzw, from * 4, to * 4);
          erase(rgba, from * 4, to * 4);
          erase(uv,   from * 2, to * 2);
          state = State::full;
        }
      }

      flush |= state;
      o.genMesh(1. - double(i) / 1000, state, xyzw.begin() + start * 4, rgba.begin() + start * 4, uv.begin() + start * 2);
      obj.last_size = size;

      start += size;
    }

    if(xyzw.size() != start * 4)
    {
      xyzw.resize(start * 4);
      rgba.resize(start * 4);
      uv.resize(start * 2);

      flush = State::full;
    }

    return std::make_pair(start, flush);
  }

  uint idx_start = 0, idx_size = 0;
  vector<uint> indices;
  vector<GLushort> xyzw, uv;
  vector<GLubyte> rgba;
};


Renderer::Renderer()
{
  auto b = GLbind(m_vao);
  GLbind(m_idx.vbo);
  b.AttribFormat(m_xyzw.vbo, 0, 4, GL_HALF_FLOAT);
  b.AttribFormat(m_rgba.vbo, 1, 4, GL_UNSIGNED_BYTE, GL_TRUE);
  b.AttribFormat(m_uv.vbo,   2, 2, GL_HALF_FLOAT);
}

Renderer::~Renderer()
{ }

bool Renderer::hovered()
{
  CASSERT(m_num > 0, "No object, can't check hover");

  return contains(m_objects[m_num - 1].obj->bounding_box(), this->mouse_pos());
}

bool Renderer::hovered(Vec4 bb)
{
  return contains(bb, this->mouse_pos());
}

void Renderer::ConsumeEvents(vector<Event> e)
{
  m_events = move(e);

  if(m_events.empty())
    return;

  m_interactions.emplace_back(m_mouse_pos);

  for(Val e: m_events)
    if(e.type() == Event::Type::MouseMove)
      m_interactions.emplace_back(e.mouse_move());
}

vector<Event> Renderer::ProcessEvents()
{
  Val refocus = [&](ObjectId id){
    for(Val i: m_logics)
      if(focused_id == i.id)
      {
        i.func(Event{ });
        break;
      }

    focused_id = id;
  };

  CASSERT(!m_objects.empty(), "No object, can't check focus");

  Val offer_event = [&](Val e){
    if(e.type() == Event::Type::Key &&
       e.key().c == GLFW_KEY_ESCAPE)
      return false;

    if(e.type() == Event::Type::MouseMove)
      m_mouse_pos = e.mouse_move();

    Val needs_refocus = (e.type() == Event::Type::MouseButton) && (e.mouse_button().s & Event::State::Press);

    if(!needs_refocus && focused_id)
      for(auto i=m_logics.crbegin(); i!=m_logics.crend(); ++i)
        if(focused_id == i->id &&
           i->func(e))
          return true;

    for(auto i=m_logics.crbegin(); i!=m_logics.crend(); ++i)
      if(contains(i->box, m_mouse_pos))
      {
        if(needs_refocus)
          refocus(i->id);

        if(i->func(e))
          return true;
      }

    if(needs_refocus && focused_id)
      refocus(0);

    return false;
  };

  m_events.erase(std::remove_if(m_events.begin(), m_events.end(), offer_event), m_events.cend());
  m_logics.clear();
  m_interactions.clear();
  return move(m_events);
}

void Renderer::Logic(function<bool(Event const&)> func, ObjectId id)
{
  CASSERT(m_num > 0, "No object, can't check focus");
  Val bb = m_objects[m_num - 1].obj->bounding_box();
  Logic(bb, move(func), id);
}

void Renderer::Logic(Vec4 bb, function<bool(Event const&)> func, ObjectId id)
{
  if((!id || id != focused_id) &&
     m_interactions.cend() == std::find_if(m_interactions.cbegin(), m_interactions.cend(), [&](Val i){ return contains(bb, i); }))
    return;

  m_logics.emplace_back(LogicStorage{ bb, id, move(func) });
}

void Renderer::Clip(Vec2 pos, Vec2 size) {
  const vec2 is_neg = glm::lessThan(size, vec2(0));
  m_clip = { pos + size * is_neg, pos + glm::abs(size) };
}

void Renderer::Render()
{
  if(m_flush)
  {
    Val get_index = [&](Val i){ return cast<uint>(std::distance(m_objects.cbegin(), i)); };

    Val last_valid = m_objects.cbegin() + m_num
        , first_invalid = [&]{
      for(auto i=m_objects.cbegin(); i!=last_valid; ++i)
      {
        Val state = i->state;
        Val overlap = [&]{
          Val o = *i->obj;
          Val z = get_index(i);
          Val batch = std::find_if(m_batches.cbegin(), m_batches.cend(),    [&](Val b){ return b.contains(m_objects, o, z); });
          CASSERT(batch != m_batches.cend(), "Batch out of bounds");
          return (batch != std::find_if(m_batches.cbegin(), batch,          [&](Val b){ return b.covered(m_objects, o, z); }) ||
              m_batches.cend() != std::find_if(batch + 1, m_batches.cend(), [&](Val b){ return b.covers(m_objects, o, z); }));
        };

        if((state & State::mismatch) ||
           ((state & State::xyzw) &&
            i->obj->ordered() &&
            overlap()))
          return i;
      }

      return last_valid;
    }();
    Val first_invalid_index = get_index(first_invalid);

    if(first_invalid != m_objects.cend())
    {
      m_batches.erase(std::remove_if(m_batches.begin(), m_batches.end(), [&](auto &i){ return i.shrink(first_invalid_index); }), m_batches.cend());
      m_objects.erase(last_valid, m_objects.cend());
    }

    for(auto j=first_invalid; j!=m_objects.cend(); ++j)
      [&]{
      Val o = *j->obj;
      Val z = get_index(j);
      for(auto i=m_batches.rbegin(); i!=m_batches.crend(); ++i)
      {
        if(i->try_to_add(m_objects, o, z))
          return;

        if(i->covered(m_objects, o))
          break;
      }

      if(o.ordered())
        m_batches.emplace_back(z);
      else
        m_batches.emplace(m_batches.cbegin(), Batch{ z });
    }();

    m_flush = 0;
    uint index_start = 0, batch_start = 0;
    Val insert = [](bool ordered, uint dim, auto &to, size_t at, Val v) {
      to.resize(at * dim);
      if(ordered)
        to.insert(to.cend(), v.cbegin(), v.cend());
      else
        for(auto i=v.crbegin(); i!=v.crend(); i+=dim)
          to.insert(to.cend(), std::next(i, dim).base(), i.base());
    };

    for(auto &i: m_batches)
    {
      Val batch = i.redraw(m_objects, first_invalid_index);
      Val batch_size = batch.first;
      m_flush |= batch.second;

      if(m_flush & State::resized)
      {
        Val indices = i.front(m_objects).genIdx(batch_start, batch_size);
        i.idx_start = index_start;
        i.idx_size = indices.size();
        insert(true, 1, m_idx.buff, index_start, indices);
      }
      Val ordered = i.front(m_objects).ordered();
      if(m_flush & State::xyzw) insert(ordered, 4, m_xyzw.buff, batch_start, i.xyzw);
      if(m_flush & State::rgba) insert(ordered, 4, m_rgba.buff, batch_start, i.rgba);
      if(m_flush & State::uv)   insert(ordered, 2, m_uv.buff,   batch_start, i.uv);

      index_start += i.idx_size;
      batch_start += batch_size;
    }
  }

  Val b = GLbind(m_vao);
  if(m_flush & State::resized) m_idx.flush();
  if(m_flush & State::xyzw)    m_xyzw.flush();
  if(m_flush & State::rgba)    m_rgba.flush();
  if(m_flush & State::uv)      m_uv.flush();

  GLState::Clear(GL_DEPTH_BUFFER_BIT);

  GLState::BlendFunc::Save();
  GLState::DepthFunc::Save();
  GLState::Save<GL_CULL_FACE, GL_DEPTH_WRITEMASK, GL_BLEND, GL_DEPTH_TEST>();
  GLState::Disable<GL_CULL_FACE>();
  GLState::Enable<GL_DEPTH_TEST, GL_DEPTH_WRITEMASK>();
  GLState::BlendFunc::Set(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  GLState::DepthFunc::Set(GL_LEQUAL);

  Val first_ordered = std::find_if(m_batches.cbegin(), m_batches.cend(), [&](Val i){ return i.front(m_objects).ordered(); });

  GLState::Disable<GL_BLEND>();
  std::for_each(m_batches.cbegin(), first_ordered, [&](Val i){ i.front(m_objects).Draw(b, cast<GLushort>(i.idx_size), cast<GLushort>(i.idx_start)); });

  GLState::Enable<GL_BLEND>();
  std::for_each(first_ordered, m_batches.cend(), [&](Val i){ i.front(m_objects).Draw(b, cast<GLushort>(i.idx_size), cast<GLushort>(i.idx_start)); });

  GLState::Restore<GL_CULL_FACE, GL_DEPTH_WRITEMASK, GL_BLEND, GL_DEPTH_TEST>();
  GLState::DepthFunc::Restore();
  GLState::BlendFunc::Restore();

  m_num = 0;
  m_flush = 0;

  Val window = Window::Get();
  if(!window.equalPos(m_aspect, window.aspect()))
  {
    m_objects.clear();
    m_aspect = window.aspect();
  }
}
