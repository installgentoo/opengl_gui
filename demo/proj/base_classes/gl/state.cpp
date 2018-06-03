#include "state.h"
#include <glm/gtc/epsilon.hpp>

using namespace code_policy;
using std::is_same;

static_assert(is_same<byte, GLbyte>::value,     "Platform char size not supported by GL");
static_assert(is_same<ubyte, GLubyte>::value,   "Platform uchar size not supported by GL");
static_assert(is_same<int16, GLshort>::value,   "Platform short size not supported by GL");
static_assert(is_same<uint16, GLushort>::value, "Platform ushort size not supported by GL");
static_assert(is_same<int, GLsizei>::value,     "Platform int size not supported by GL");
static_assert(is_same<uint, GLuint>::value,     "Platform uint size not supported by GL");
static_assert(is_same<uint, GLenum>::value,     "Platform uint size not supported by GL");
static_assert(is_same<int64, GLint64>::value,   "Platform long size not supported by GL");
static_assert(is_same<uint64, GLuint64>::value, "Platform ulong size not supported by GL");

map<int, int> GLState::m_overflow;

GLuint         TextureControl::m_bound_unit = 0;
vector<GLuint> TextureControl::m_texture_units;

vector<GLuint> SamplerControl::m_texture_units;

string code_policy::glCodeToError(GLenum code)
{
  switch(code)
  {
    case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
    case GL_STACK_OVERFLOW:                return "GL_STACK_OVERFLOW";
    case GL_STACK_UNDERFLOW:               return "GL_STACK_UNDERFLOW";
    case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_CONTEXT_LOST:                  return "GL_CONTEXT_LOST";
  }
  return "GL_?_" + std::to_string(code);
}


static void
#ifdef WIN32
CALLBACK
#endif
debug_gl_printer(GLenum src, GLenum type, GLuint id, GLenum lvl, GLsizei, GLchar const*msg, void const*filter)
{
  Val l = [lvl, f = *reinterpret_cast<uint const*>(filter)]()->string{
    switch(lvl)
    {
      case GL_DEBUG_SEVERITY_HIGH:                              return "HIG";
      case GL_DEBUG_SEVERITY_MEDIUM:       if(f > 2) return ""; return "MED";
      case GL_DEBUG_SEVERITY_LOW:          if(f > 1) return ""; return "LOW";
      case GL_DEBUG_SEVERITY_NOTIFICATION: if(f > 0) return ""; return "PRT";
    }
    return "SEVERITY_?_" + std::to_string(lvl);
  }();

  if(l.empty())
    return;

  Val s = [src]()->string{
      switch(src){
      case GL_DEBUG_SOURCE_API:             return "SOURCE_API";
      case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   return "SOURCE_WINDOW_SYSTEM";
      case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SOURCE_SHADER_COMPILER";
      case GL_DEBUG_SOURCE_THIRD_PARTY:     return "SOURCE_THIRD_PARTY";
      case GL_DEBUG_SOURCE_APPLICATION:     return "SOURCE_APPLICATION";
      case GL_DEBUG_SOURCE_OTHER:           return "SOURCE_OTHER"; }
      return "SOURCE_?_" + std::to_string(src); }()
      , t = [type]()->string{
      switch(type) {
      case GL_DEBUG_TYPE_ERROR:               return "TYPE_ERROR";
      case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "TYPE_DEPRECATED_BEHAVIOR";
      case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  return "TYPE_UNDEFINED_BEHAVIOR";
      case GL_DEBUG_TYPE_PORTABILITY:         return "TYPE_PORTABILITY";
      case GL_DEBUG_TYPE_PERFORMANCE:         return "TYPE_PERFORMANCE";
      case GL_DEBUG_TYPE_MARKER:              return "TYPE_MARKER";
      case GL_DEBUG_TYPE_PUSH_GROUP:          return "TYPE_PUSH_GROUP";
      case GL_DEBUG_TYPE_POP_GROUP:           return "TYPE_POP_GROUP";
      case GL_DEBUG_TYPE_OTHER:               return "TYPE_OTHER"; }
      return "TYPE_?_" + std::to_string(type); }();

  CINFO("GL_DEBUG_"<<id<<", "<<l<<": "<<t<<" "<<s<<" "<<msg);
}

void GLState::EnableDebugContext(uint level)
{
  GLState::Enable<GL_DEBUG_OUTPUT>();
  GLState::Enable<GL_DEBUG_OUTPUT_SYNCHRONOUS>();

  static uint debug_level = level;
  GLCHECK(glDebugMessageCallback(debug_gl_printer, &debug_level));
}


void GLState::BindScreenFramebuffer()
{
  StateControl<FboPolicy>::Bind(0);
}

void GLState::ClearColor(vec4 const&color)
{
  static vec4 clear_color = vec4(0);

  if(glm::all(glm::epsilonEqual(clear_color, color, vec4(1. / 256))))
    return;

  GLCHECK(glClearColor(color.r, color.g, color.b, color.a));
  clear_color = color;
}
