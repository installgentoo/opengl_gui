#pragma once
#include "objects.h"

namespace code_policy
{

struct GLsampler : GLobject<SamplerPolicy>
{
  template<class...P> GLsampler(P &&...p) {
    CASSERT(sizeof ...(p) > 0, "Sampler has no parameters");
    this->Parameters(forward<P>(p)...);
  }

  void Parameter(int PARAM, GLint value)       { GLCHECK(glSamplerParameteri(m_obj, cast<GLenum>(PARAM), value));                  }
  void Parameter(int PARAM, GLfloat value)     { GLCHECK(glSamplerParameterf(m_obj, cast<GLenum>(PARAM), value));                  }
  void Parameter(int PARAM, ivec4 const&value) { GLCHECK(glSamplerParameteriv(m_obj, cast<GLenum>(PARAM), glm::value_ptr(value))); }
  void Parameter(int PARAM, vec4 const&value)  { GLCHECK(glSamplerParameterfv(m_obj, cast<GLenum>(PARAM), glm::value_ptr(value))); }
  template<class T, class...P> void Parameters(int PARAM, T value, P &&...p) {
    this->Parameter(PARAM, value);
    this->Parameters(forward<P>(p)...);
  }

private:
  void Parameters()const { }
};

template<>
struct GLbinding<GLsampler>
{
  GLbinding(GLsampler const&s, GLuint unit)
  CDEBUGBLOCK( : o(s.obj()) CCOMMA u(unit) )
  {
    SamplerControl::Bind(s.obj(), unit);
  }
  ~GLbinding()
  {
    CASSERT(o == SamplerControl::m_texture_units[u], "Binding order error");
  }

  CDEBUGBLOCK( const GLuint o CCOMMA u; )
};
inline GLbinding<GLsampler> GLbind(GLsampler const&s, GLuint unit) { return { s, unit }; }


template<GLenum m_type>
struct GLtex : GLobject<TexturePolicy> { };

struct GLtexStats {
  uint64 Size(uint level=0)const;

  uint width, height, channels, precision;
};

using GLtex2d = GLtex<GL_TEXTURE_2D>;

template<>
struct GLtex<GL_TEXTURE_2D> : GLobject<TexturePolicy>
{
  friend struct GLbinding<GLtex2d>;

  GLtex(uint width, uint height, uint channels, GLenum PRECISION=GL_BYTE, void const*data=nullptr, GLenum FORMAT=GL_RGBA, GLenum TYPE=GL_UNSIGNED_BYTE, uint alignment=4);
  template<class T> GLtex(T const&img, uint channels, uint alignment=4);

  Val stats()const   { return m_stats;        }
  uint width()const  { return m_stats.width;  }
  uint height()const { return m_stats.height; }

private:
  mutable GLtexStats m_stats;
};

using GLtexCube = GLtex<GL_TEXTURE_CUBE_MAP>;

template<>
struct GLtex<GL_TEXTURE_CUBE_MAP> : GLobject<TexturePolicy>
{
  friend struct GLbinding<GLtexCube>;

  GLtex(uint width, uint height, uint channels, GLenum PRECISION=GL_BYTE, array<void const*, 6> data={0,0,0,0,0,0}, GLenum FORMAT=GL_RGBA, GLenum TYPE=GL_UNSIGNED_BYTE, uint alignment=4);
  GLtex(array<uImage, 6> const&img, uint channels, uint alignment=4);
  GLtex(array<fImage, 6> const&img, uint channels, uint alignment=4);

  Val stats()const   { return m_stats;        }
  uint width()const  { return m_stats.width;  }
  uint height()const { return m_stats.height; }

private:
  mutable GLtexStats m_stats;
};

template<GLenum m_type>
struct GLbinding<GLtex<m_type>>
{
  GLbinding(GLtex<m_type> const&t, GLuint unit)
    : r_stats(t.m_stats)
    CDEBUGBLOCK( CCOMMA o(t.obj()) CCOMMA u(unit) )
  {
    TextureControl::Bind(m_type, t.obj(), unit);
  }
  ~GLbinding()
  {
    CASSERT((u == TextureControl::m_bound_unit) && (o == TextureControl::m_texture_units[u * 2]), "Binding order error");
  }

  void GenMipmaps() {
    GLCHECK(glGenerateMipmap(m_type));
    Parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  }

  void Parameter(int PARAM, GLint value)       { GLCHECK(glTexParameteri(m_type, cast<GLenum>(PARAM), value));                  }
  void Parameter(int PARAM, GLfloat value)     { GLCHECK(glTexParameterf(m_type, cast<GLenum>(PARAM), value));                  }
  void Parameter(int PARAM, ivec4 const&value) { GLCHECK(glTexParameteriv(m_type, cast<GLenum>(PARAM), glm::value_ptr(value))); }
  void Parameter(int PARAM, vec4 const&value)  { GLCHECK(glTexParameterfv(m_type, cast<GLenum>(PARAM), glm::value_ptr(value))); }
  template<class T, class...P> void Parameters(int PARAM, T value, P &&...p) {
    this->Parameter(PARAM, value);
    this->Parameters(forward<P>(p)...);
  }

protected:
  GLtexStats &r_stats;
  CDEBUGBLOCK( const GLuint o CCOMMA u; )
  void Parameters()const { }
};

struct GLtex2dBinding : GLbinding<GLtex2d>
{
  using GLbinding<GLtex2d>::GLbinding;

  template<class T> vector<T> Save(uint channels)const;
  void Save(void *ptr, GLenum FORMAT, GLenum TYPE)const;
  void Load(uint lod, uint width, uint height, uint channels, GLenum PRECISION, void const*data, GLenum FORMAT, GLenum TYPE, uint alignment=4);
  void Load(uint lod, uImage const&img, uint channels, uint alignment=4);
  void Load(uint lod, fImage const&img, uint channels, uint alignment=4);
};
inline GLtex2dBinding GLbind(GLtex2d const&t, GLuint unit) { return { t, unit };                         }
inline GLtex2dBinding GLbind(GLtex2d const&t)              { return { t, TextureControl::m_bound_unit }; }

struct GLtexCubeBinding : GLbinding<GLtexCube>
{
  using GLbinding<GLtexCube>::GLbinding;

  void Load(uint lod, uint target, uint width, uint height, uint channels, GLenum PRECISION, void const*data, GLenum FORMAT, GLenum TYPE, uint alignment=4);
  void Load(uint lod, uint target, uImage const&img, uint channels, uint alignment=4);
  void Load(uint lod, uint target, fImage const&img, uint channels, uint alignment=4);
};
inline GLtexCubeBinding GLbind(GLtexCube const&t, GLuint unit) { return { t, unit };                         }
inline GLtexCubeBinding GLbind(GLtexCube const&t)              { return { t, TextureControl::m_bound_unit }; }


struct GLfbo : GLobject<FboPolicy>
{
  GLfbo(uint width, uint height, uint channels=4, GLenum PRECISION=GL_BYTE);

  Val tex()const     { CASSERT(m_tex.obj(), "Surface owns no texture"); return m_tex;                   }
  auto TakeTexture() { CASSERT(m_tex.obj(), "Surface owns no texture"); auto t = move(m_tex); return t; }

private:
  GLtex2d m_tex;
};

struct GLfboBinding : GLbinding<GLfbo>
{
  GLfboBinding(GLfbo const&f)
    : GLbinding<GLfbo>::GLbinding(f)
  {
    GLState::Viewport(f.tex().width(), f.tex().height());
  }

  void Clear(GLclampf v);
  void Clear() { GLState::Clear(GL_COLOR_BUFFER_BIT); }
};
inline GLfboBinding GLbind(GLfbo const&f) { return { f }; }


struct Slab
{
  Slab(uint width, uint height, uint channels=4, GLenum PRECISION=GL_BYTE)
    : src(width, height, channels, PRECISION)
    , tgt(width, height, channels, PRECISION)
  { }

  void Swap() { std::swap(src, tgt); }

  GLfbo src, tgt;
};

}
