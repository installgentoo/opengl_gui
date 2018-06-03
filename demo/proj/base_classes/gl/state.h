#pragma once
#include "base_classes/policies/logging.h"
#include <GL/gl3w.h>
#include <glm/vec4.hpp>
#include <glm/common.hpp>

namespace code_policy
{

string glCodeToError(GLenum);

struct GLState
{
  static void EnableDebugContext(uint level=0);
  static void BindScreenFramebuffer();
  static void ClearColor(vec4 const&color);
  static void ClearColor(float b)    { ClearColor(vec4(b, b, b, 1)); }
  static void Clear(GLbitfield mask) { GLCHECK(glClear(mask));       }

  static void Viewport(uint w, uint h, uint x=0, uint y=0) {
    static uvec4 s_fbo_size = uvec4(0);
    if((s_fbo_size.x != w) || (s_fbo_size.y != h) || (s_fbo_size.z != x) || (s_fbo_size.a != y))
    {
      s_fbo_size = uvec4(w, h, x, y);
      GLCHECK(glViewport(cast<GLsizei>(x), cast<GLsizei>(y), cast<GLsizei>(w), cast<GLsizei>(h)));
    }
  }

  template<GLenum PARAM>static GLboolean Bconst() { static const auto s_v = []{ GLboolean r; GLCHECK(glGetBooleanv(PARAM, &r)) return r; }(); return s_v; }
  template<GLenum PARAM>static GLdouble Dconst()  { static const auto s_v = []{ GLdouble r; GLCHECK(glGetDoublev(PARAM, &r))   return r; }(); return s_v; }
  template<GLenum PARAM>static GLfloat Fconst()   { static const auto s_v = []{ GLfloat r; GLCHECK(glGetFloatv(PARAM, &r))     return r; }(); return s_v; }
  template<GLenum PARAM>static GLint Iconst()     { static const auto s_v = []{ GLint r; GLCHECK(glGetIntegerv(PARAM, &r))     return r; }(); return s_v; }

  template<GLenum...> struct typelist { };
  template<GLenum...P>static void Enable()  { EnableCall(typelist<P...>());  }
  template<GLenum...P>static void Disable() { DisableCall(typelist<P...>()); }
  template<GLenum...P>static void Save()    { SaveCall(typelist<P...>());    }
  template<GLenum...P>static void Restore() { RestoreCall(typelist<P...>()); }

private:
  template<class F, F m_func, class...P>
  struct Func {
    static void Set(P ...p) {
      if(!Valid())
      {
        Valid() = true;
        tuple<P...> state(p...);
        PrevState() = state;
        State() = move(state);
        GLCHECK((*m_func)(p...));
      }

      auto &state = State();
      tuple<P...> new_state(p...);

      if(new_state == state)
        return;

      state = move(new_state);
      GLCHECK((*m_func)(p...));
    }

    static void Restore() {
      Val prev_state = PrevState();
      auto &state = State();

      if(state == prev_state)
        return;

      state = prev_state;
      Call(state, std::index_sequence_for<P...>{});
    }

    static void Save() {
      PrevState() = State();
    }

  private:
    template<class T, size_t...I>static void Call(T &t, std::index_sequence<I...>) {
      GLCHECK((*m_func)(std::get<I>(t)...));
    }

    static bool& Valid() {
      static bool s_valid = false;
      return s_valid;
    }

    static tuple<P...>& PrevState() {
      CASSERT(Valid(), "State wasn't set");
      static tuple<P...> s_prev_state;
      return s_prev_state;
    }

    static tuple<P...>& State() {
      CASSERT(Valid(), "State wasn't set");
      static tuple<P...> s_state;
      return s_state;
    }
  };

  static void PixelStoreiPack(GLint v) { glPixelStorei(GL_PACK_ALIGNMENT, v); }
  static void PixelStoreiUnpack(GLint v) { glPixelStorei(GL_UNPACK_ALIGNMENT, v); }
public:

  typedef Func<decltype(&glBlendFunc), &glBlendFunc, GLenum, GLenum> BlendFunc;
  typedef Func<decltype(&glBlendFuncSeparate), &glBlendFuncSeparate, GLenum, GLenum, GLenum, GLenum> BlendFuncSeparate;
  typedef Func<decltype(&glBlendEquation), &glBlendEquation, GLenum> BlendEquation;
  typedef Func<decltype(&glDepthFunc), &glDepthFunc, GLenum> DepthFunc;
  typedef Func<decltype(&GLState::PixelStoreiPack), &GLState::PixelStoreiPack, GLint> PixelStoreSave;
  typedef Func<decltype(&GLState::PixelStoreiUnpack), &GLState::PixelStoreiUnpack, GLint> PixelStoreLoad;

private:
  static map<int, int> m_overflow;

  template<GLenum PARAM>static uint64& State() {
    static uint64 s_state = []{ GLDisable<PARAM>(); return 0u; }();
    return s_state;
  }

  template<GLenum PARAM>static void EnableImpl() {
    uint64 &state = State<PARAM>();
    if((state & 1u) == 1u)
      return;

    GLEnable<PARAM>();
    state |= 1u;
  }

  template<GLenum PARAM>static void DisableImpl() {
    uint64 &state = State<PARAM>();
    if((state & 1u) == 0u)
      return;

    GLDisable<PARAM>();
    state &= ~1u;
  }

  template<GLenum PARAM>static void SaveImpl() {
    CDEBUGBLOCK( auto &v = m_overflow[PARAM]; v = glm::min(v + 1, 64); )
    uint64 &state = State<PARAM>();
    state = (state & 1u) | (state << 1);
  }

  template<GLenum PARAM>static void RestoreImpl() {
    CASSERT((--m_overflow[PARAM]) >= 0, "GLState has no value in stack");
    uint64 &state = State<PARAM>();
    const uint64 s = (state & 1u);
    state >>= 1;
    if(s == (state & 1u))
      return;

    if(state == 0u)
      GLDisable<PARAM>();
    else
      GLEnable<PARAM>();
  }

  template<GLenum PARAM, GLenum...P>static void EnableCall(typelist<PARAM, P...>)  { EnableImpl<PARAM>();  EnableCall(typelist<P...>());  }
  template<GLenum PARAM, GLenum...P>static void DisableCall(typelist<PARAM, P...>) { DisableImpl<PARAM>(); DisableCall(typelist<P...>()); }
  template<GLenum PARAM, GLenum...P>static void SaveCall(typelist<PARAM, P...>)    { SaveImpl<PARAM>();    SaveCall(typelist<P...>());    }
  template<GLenum PARAM, GLenum...P>static void RestoreCall(typelist<PARAM, P...>) { RestoreImpl<PARAM>(); RestoreCall(typelist<P...>()); }
  static void EnableCall(typelist<>)  { }
  static void DisableCall(typelist<>) { }
  static void SaveCall(typelist<>)    { }
  static void RestoreCall(typelist<>) { }

  template<GLenum PARAM>static void GLEnable()  { GLCHECK(glEnable(PARAM));  }
  template<GLenum PARAM>static void GLDisable() { GLCHECK(glDisable(PARAM)); }
};
template<>inline void GLState::GLEnable<GL_DEPTH_WRITEMASK>()  { GLCHECK(glDepthMask(GL_TRUE));  }
template<>inline void GLState::GLDisable<GL_DEPTH_WRITEMASK>() { GLCHECK(glDepthMask(GL_FALSE)); }


struct TextureControl
{
  static void Bind(GLenum TYPE, GLuint tex, GLuint n) {
    if(!(n * 2 < m_texture_units.size()))
    {
      if(!(cast<GLint>(n) < GLState::Iconst<GL_MAX_TEXTURE_IMAGE_UNITS>()))
        CERROR("Not enough texture units avaliable, "<<n<<" units in use");

      m_texture_units.resize(n * 2 + 2, 0);
    }

    if(n != m_bound_unit)
    {
      GLCHECK(glActiveTexture(GL_TEXTURE0 + n));
      m_bound_unit = n;
    }

    if(m_texture_units[m_bound_unit * 2] != tex ||
       m_texture_units[m_bound_unit * 2 + 1] != TYPE)
    {
      GLCHECK(glBindTexture(TYPE, tex));
      m_texture_units[m_bound_unit * 2] = tex;
      m_texture_units[m_bound_unit * 2 + 1] = TYPE;
    }
  }

  static void Delete(GLuint tex) {
    for(size_t i=0; i<m_texture_units.size(); i+=2)
      if(m_texture_units[i] == tex)
      {
        m_texture_units[i] = 0;
        m_texture_units[i + 1] = 0;
      }
  }

  static GLuint m_bound_unit;
  static vector<GLuint> m_texture_units;
};


struct SamplerControl
{
  static void Bind(GLuint obj, GLuint n) {
    if(!(n < m_texture_units.size()))
    {
      if(!(cast<GLint>(n) < GLState::Iconst<GL_MAX_TEXTURE_IMAGE_UNITS>()))
        CERROR("Not enough texture units avaliable, "<<n<<" units in use");

      m_texture_units.resize(n + 1, 0);
    }

    if(m_texture_units[n] != obj)
    {
      GLCHECK(glBindSampler(n, obj));
      m_texture_units[n] = obj;
    }
  }

  static void Delete(GLuint obj) {
    for(auto &i: m_texture_units)
      if(i == obj)
        i = 0;
  }

  static vector<GLuint> m_texture_units;
};


struct TexturePolicy {
  static void gen(GLuint *obj) { glGenTextures(1, obj);                                  }
  static void del(GLuint *obj) { TextureControl::Delete(*obj); glDeleteTextures(1, obj); }
};

struct SamplerPolicy {
  static void gen(GLuint *obj) { glGenSamplers(1, obj);                                  }
  static void del(GLuint *obj) { SamplerControl::Delete(*obj); glDeleteSamplers(1, obj); }
};

struct ShaderProgramPolicy {
  static void bind(GLuint obj) { glUseProgram(obj);        }
  static void gen(GLuint *obj) { *obj = glCreateProgram(); }
  static void del(GLuint *obj) { glDeleteProgram(*obj);    }
};

struct FboPolicy {
  static void bind(GLuint obj) { glBindFramebuffer(GL_DRAW_FRAMEBUFFER, obj); }
  static void gen(GLuint *obj) { glGenFramebuffers(1, obj);                   }
  static void del(GLuint *obj) { glDeleteFramebuffers(1, obj);                }
};

struct RenderbufferPolicy {
  static void bind(GLuint obj) { glBindRenderbuffer(GL_RENDERBUFFER, obj); }
  static void gen(GLuint *obj) { glGenRenderbuffers(1, obj);               }
  static void del(GLuint *obj) { glDeleteRenderbuffers(1, obj);            }
};

struct VaoPolicy {
  static void bind(GLuint obj) { glBindVertexArray(obj);       }
  static void gen(GLuint *obj) { glGenVertexArrays(1, obj);    }
  static void del(GLuint *obj) { glDeleteVertexArrays(1, obj); }
};

template<GLenum m_type> struct BuffPolicy {
  static void bind(GLuint obj) { glBindBuffer(m_type, obj); }
  static void gen(GLuint *obj) { glGenBuffers(1, obj);      }
  static void del(GLuint *obj) { glDeleteBuffers(1, obj);   }
};

struct QueryPolicy {
  static void gen(GLuint *obj) { glGenQueries(1, obj);      }
  static void del(GLuint *obj) { glDeleteQueries(1, obj);   }
};


template<class m_policy>
struct StateControl
{
  static void Bind(GLuint obj) {
    if(m_bound_object != obj)
    {
      GLCHECK(m_policy::bind(obj));
      m_bound_object = obj;
    }
  }

  static GLuint Create() {
    GLuint obj = 0;
    GLCHECK(m_policy::gen(&obj));
    CASSERT(obj, "OpenGL variable not initilized");
    return obj;
  }

  static void Delete(GLuint obj) {
    if(obj)
    {
      if(m_bound_object == obj)
        m_bound_object = 0;
      GLCHECK(m_policy::del(&obj));
    }
  }

  static GLuint m_bound_object;
};
template<class m_policy> GLuint StateControl<m_policy>::m_bound_object = 0;

}
