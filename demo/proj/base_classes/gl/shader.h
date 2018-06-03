#pragma once
#include "objects.h"
#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>

namespace code_policy
{

struct GLshader : GLobject<ShaderProgramPolicy>
{
  GLshader(bool &valid, string &errors, string vert, string pix, string geom={});
  GLshader(string vert, string pix, string geom={});

  GLint GetUniform(char const*name)const;

  const string m_name;
private:
  mutable unordered_map<string, const GLint> m_uniforms;
};

struct GLbindingShader : GLbinding<GLshader>
{
  GLbindingShader(GLshader const&s)
    : GLbinding<GLshader>::GLbinding(s)
    , r_shader(s)
  { }

  void Uniform(char const*name, int u)          { GLCHECK(glUniform1i(r_shader.GetUniform(name), u));                                                                }
  void Uniform(char const*name, uint u)         { GLCHECK(glUniform1i(r_shader.GetUniform(name), cast<GLint>(u)));                                                   }
  void Uniform(char const*name, double u)       { GLCHECK(glUniform1f(r_shader.GetUniform(name), u));                                                                }
  void Uniform(char const*name, vec2 const&u)   { GLCHECK(glUniform2f(r_shader.GetUniform(name), u.x, u.y));                                                         }
  void Uniform(char const*name, vec3 const&u)   { GLCHECK(glUniform3f(r_shader.GetUniform(name), u.x, u.y, u.z));                                                    }
  void Uniform(char const*name, vec4 const&u)   { GLCHECK(glUniform4f(r_shader.GetUniform(name), u.r, u.g, u.b, u.a));                                               }
  void Uniform(char const*name, mat2 const&m)   { GLCHECK(glUniformMatrix2fv(r_shader.GetUniform(name), 1, GL_FALSE, glm::value_ptr(m)));                            }
  void Uniform(char const*name, mat3 const&m)   { GLCHECK(glUniformMatrix3fv(r_shader.GetUniform(name), 1, GL_FALSE, glm::value_ptr(m)));                            }
  void Uniform(char const*name, mat4 const&m)   { GLCHECK(glUniformMatrix4fv(r_shader.GetUniform(name), 1, GL_FALSE, glm::value_ptr(m)));                            }
  void Uniform(char const*name, mat4x3 const&m) { GLCHECK(glUniformMatrix4x3fv(r_shader.GetUniform(name), 1, GL_FALSE, glm::value_ptr(m)));                          }
  void Uniform(char const*name, vector<int> const&u)   { GLCHECK(glUniform1iv(r_shader.GetUniform(name), u.size(), u.data()));                                       }
  void Uniform(char const*name, vector<float> const&u) { GLCHECK(glUniform1fv(r_shader.GetUniform(name), u.size(), u.data()));                                       }
  void Uniform(char const*name, vector<vec2> const&u)  { GLCHECK(glUniform2fv(r_shader.GetUniform(name), u.size() * 2, reinterpret_cast<GLfloat const*>(u.data()))); }
  void Uniform(char const*name, vector<vec3> const&u)  { GLCHECK(glUniform3fv(r_shader.GetUniform(name), u.size() * 3, reinterpret_cast<GLfloat const*>(u.data()))); }
  void Uniform(char const*name, vector<vec4> const&u)  { GLCHECK(glUniform4fv(r_shader.GetUniform(name), u.size() * 4, reinterpret_cast<GLfloat const*>(u.data()))); }
  template<class T, class...P> void Uniforms(char const*name, T value, P &&...p) {
    this->Uniform(name, value);
    this->Uniforms(forward<P>(p)...);
  }

private:
  void Uniforms()const { }
  GLshader const&r_shader;
};
inline GLbindingShader GLbind(GLshader const&s) { return { s }; }


struct ShaderManager : CUNIQUE
{
  enum ShaderType { VERTEX=0, FRAGMENT=1, GEOMETRY=2 };

  static ShaderManager& Get();

  void LoadShaderSource(string, string);
  void ForceShaderSource(string const&, string);
  string CompileProgram(string const&, GLuint, string, string, string);

  void LoadAllShadersFromFile(char const*filename);
  void ClearCache();

private:
  unordered_map<string, GLuint> m_shader_objects;
  unordered_map<string, string> m_shader_sources;
};


map<string, string> ParseShaderSources(string text);


struct ShaderText
{
  template<class...P> ShaderText(string &&name, P &&...strings) {
    ShaderManager::Get().LoadShaderSource(move(name), ConcatLine(move(strings)...));
  }

  template<class...P> string ConcatLine(string &&a, P &&...p)const {
    return a + "\n" + ConcatLine(move(p)...);
  }
  string ConcatLine()const { return ""; }
};

template<class T> string GLSLUnwindedLoop(string command, T start, T end, T step = 1)
{
  string text;
  CASSERT(step > 0, "Can't unwind into zero steps");
  for(T i=start; i<=end; i+=step)
  {
    for(size_t found; (found = command.find("$$iter")) != command.npos;)
      command.replace(found, found + 6, to_string(i));

    text += command + "; ";
  }
  return text;
}

#define SHADER(name, ...)const ShaderText shadertext__##name(#name, __VA_ARGS__);

}
