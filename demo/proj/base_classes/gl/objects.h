#pragma once
#include "state.h"
#include <glm/gtc/type_ptr.hpp>

namespace code_policy
{

template<class T> GLenum getGlType();
template<>inline GLenum getGlType<GLubyte>()  { return GL_UNSIGNED_BYTE;  }
template<>inline GLenum getGlType<GLushort>() { return GL_UNSIGNED_SHORT; }
template<>inline GLenum getGlType<GLuint>()   { return GL_UNSIGNED_INT;   }

template<class m_policy>
struct GLobject
{
  GLobject()
    : m_obj(StateControl<m_policy>::Create())
  { }
  ~GLobject()
  {
    StateControl<m_policy>::Delete(m_obj);
  }
  GLobject(GLobject &&r)
    : m_obj(r.m_obj)
  {
    r.m_obj = 0;
  }
  GLobject& operator=(GLobject &&r)
  {
    StateControl<m_policy>::Delete(m_obj);
    m_obj = r.m_obj;
    r.m_obj = 0;
    return *this;
  }

  GLuint obj()const {
    return m_obj;
  }

  typedef m_policy policy;
protected:
  GLuint m_obj;
};


template<class m_type>
struct GLbinding
{
  GLbinding(m_type const&u)
  CDEBUGBLOCK( : o(u.obj()) )
  {
    StateControl<typename m_type::policy>::Bind(u.obj());
  }
  ~GLbinding()
  {
    CASSERT(o == StateControl<typename m_type::policy>::m_bound_object, "Binding order error");
  }

  CDEBUGBLOCK( const GLuint o; )
};
template<class T> GLbinding<GLobject<T>> GLbind(GLobject<T> const&o) { return { o }; }


template<GLenum m_type> using GLbuffer = GLobject<BuffPolicy<m_type>>;

template<GLenum m_type>
struct GLbindingBuffer : GLbinding<GLbuffer<m_type>>
{
  struct Mapping {
    Mapping(void *p, GLbindingBuffer &buff)
      : ptr(p)
      , m_buff(buff)
    { }
    ~Mapping()
    {
      m_buff.UnmapBuffer();
    }

    void *ptr;
  private:
    GLbindingBuffer &m_buff;
  };

  friend struct GLbindingBuffer::Mapping;

  using GLbinding<GLbuffer<m_type>>::GLbinding;

  template<class T> void AllocateBuffer(vector<T> const&data, GLenum USAGE=GL_STATIC_DRAW) {
    this->AllocateBuffer(data.data(), sizeof(data[0]) * data.size(), USAGE);
  }

  void AllocateBuffer(void const*data, size_t size, GLenum USAGE=GL_STATIC_DRAW) {
    GLCHECK(glBufferData(m_type, cast<GLsizeiptr>(size), data, USAGE));
  }

  template<class T> void UpdateBuffer(vector<T> const&data, size_t offset) {
    GLCHECK(glBufferSubData(m_type, sizeof(data[0]) * offset, sizeof(data[0]) * data.size(), data.data()));
  }

  void UpdateBuffer(void const*data, size_t size, size_t offset) {
    GLCHECK(glBufferSubData(m_type, offset, size, data));
  }

  Mapping MapBuffer(GLenum ACCESS) {
    void *buff_ptr = GLCHECK_RET(glMapBuffer(m_type, ACCESS));
    CASSERT(buff_ptr, "Passed nullptr as target ptr");
    return { buff_ptr, *this };
  }

private:
  void UnmapBuffer() {
    CDEBUGBLOCK( Val valid = ) GLCHECK_RET(glUnmapBuffer(m_type));
    CASSERT(valid == GL_TRUE, "Buffer memory was corrupted by OS");
  }
};
template<GLenum T> GLbindingBuffer<T> GLbind(GLbuffer<T> const&o) { return { o }; }


typedef GLobject<VaoPolicy> GLvao;

struct GLbindingVao : GLbinding<GLvao>
{
  using GLbinding<GLvao>::GLbinding;

  void Draw(GLenum type, uint num, GLenum MODE=GL_TRIANGLES)const {
    GLCHECK(glDrawElements(MODE, cast<GLsizei>(num), type, nullptr));
  }

  template<class T>void DrawOffset(T num, T offset, GLenum MODE=GL_TRIANGLES)const {
    GLCHECK(glDrawElements(MODE, cast<GLsizei>(num), getGlType<T>(), reinterpret_cast<void*>(cast<intptr_t>(offset * sizeof(T)))));
  }

  void AttribFormat(GLbindingBuffer<GL_ARRAY_BUFFER> const&, GLuint idx, GLint size, GLenum TYPE=GL_FLOAT, GLboolean NORMALIZED=GL_FALSE, GLsizei stride=0, void const*first=nullptr) {
    CASSERT((size > 0) && (size < 5), "Attribute size only range from 1 to 4");
    GLCHECK(glEnableVertexAttribArray(idx));
    GLCHECK(glVertexAttribPointer(idx, size, TYPE, NORMALIZED, stride, first));
  }
};
inline GLbindingVao GLbind(GLvao const&o) { return { o }; }

}
