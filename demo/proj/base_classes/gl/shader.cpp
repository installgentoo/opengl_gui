#include "shader.h"
#include "base_classes/policies/resource.h"
#include <algorithm>

using namespace code_policy;

static string printShaderLog(uint obj)
{
  GLint maxLength;
  vector<char> infoLog;
  if(glIsShader(obj))
  {
    GLCHECK(glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &maxLength));
    infoLog.resize(cast<size_t>(maxLength));
    GLCHECK(glGetShaderInfoLog(obj, maxLength, nullptr, infoLog.data()));
  }
  else
  {
    GLCHECK(glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &maxLength));
    infoLog.resize(cast<size_t>(maxLength));
    GLCHECK(glGetProgramInfoLog(obj, maxLength, nullptr, infoLog.data()));
  }

  if(!infoLog.empty())
    return { infoLog.data() };

  return "";
}


GLshader::GLshader(bool &valid, string &errors, string vert, string pix, string geom)
  : m_name("v:" + vert + "|p:" + pix + (geom.empty() ? "" : "|g:" + geom))
{
  errors = ShaderManager::Get().CompileProgram(m_name, m_obj, move(vert), move(pix), move(geom));
  valid = errors.empty();
}

GLshader::GLshader(string vert, string pix, string geom)
  : m_name("v:" + vert + "|p:" + pix + (geom.empty() ? "" : "|g:" + geom))
{
  Val errors = ShaderManager::Get().CompileProgram(m_name, m_obj, move(vert), move(pix), move(geom));
  if(!errors.empty())
    CERROR(errors);
}

GLint GLshader::GetUniform(char const*name)const
{
  Val found = m_uniforms.find(name);

  if(found != m_uniforms.cend())
    return found->second;

  Val addr = GLCHECK_RET(glGetUniformLocation(m_obj, name));
  if(-1 == addr)
    CINFO("No uniform named '"<<name<<"' in shader '"<<m_name<<"', or uniform was optimized out");
  m_uniforms.emplace(name, addr);
  return addr;
}


ShaderManager& ShaderManager::Get()
{
  static ShaderManager s_manager;
  return s_manager;
}

void ShaderManager::LoadShaderSource(string name, string source)
{
  CASSERT(m_shader_sources.find(name) == m_shader_sources.cend(), "Shader source '"<<name<<"' already exists");
  m_shader_sources.emplace(move(name), move(source));
}

void ShaderManager::ForceShaderSource(string const&name, string source)
{
  m_shader_sources[name] = move(source);
  auto found_obj = m_shader_objects.find(name);
  if(found_obj != m_shader_objects.cend())
  {
    GLCHECK(glDeleteShader(found_obj->second));
    m_shader_objects.erase(found_obj);
  }
}

string ShaderManager::CompileProgram(string const&prog, GLuint obj, string vert, string pix, string geom)
{
  static const string s_shader_strings[] = { "vert", "fragment", "geom" };

  string errors;

  Val get_object = [&](string name, ShaderType type){
    Val found = m_shader_objects.find(name);
    if(m_shader_objects.cend() != found)
      return found->second;

    Val source = m_shader_sources.find(name);
    if(source == m_shader_sources.cend())
    {
      errors += "No " + s_shader_strings[type] + " shader '" + name + "' loaded\n";
      return 0u;
    }

    GLuint id = 0;
    switch(type)
    {
      case VERTEX:   id = GLCHECK_RET(glCreateShader(GL_VERTEX_SHADER));   break;
      case FRAGMENT: id = GLCHECK_RET(glCreateShader(GL_FRAGMENT_SHADER)); break;
      case GEOMETRY: id = GLCHECK_RET(glCreateShader(GL_GEOMETRY_SHADER)); break;
    }
    CASSERT(id, "Failed to create object");

    char const* const s = source->second.c_str();
    GLCHECK(glShaderSource(id, 1, &s, nullptr));
    GLCHECK(glCompileShader(id));
    GLint status;
    GLCHECK(glGetShaderiv(id, GL_COMPILE_STATUS, &status));
    if(!status)
    {
      errors += "Error compiling " + s_shader_strings[type] + " shader '" + name + "'\n" + printShaderLog(id) + "\n";
      GLCHECK(glDeleteShader(id));
      return 0u;
    }

    return m_shader_objects.emplace(move(name), id).first->second;
  };

  Val vert_obj = get_object(move(vert), VERTEX)
      , pix_obj = get_object(move(pix), FRAGMENT)
      , geom_obj = geom.empty() ? 0 : get_object(move(geom), GEOMETRY);

  if(!errors.empty())
    return errors;

  GLCHECK(glAttachShader(obj, vert_obj));
  GLCHECK(glAttachShader(obj, pix_obj));
  if(geom_obj)
    GLCHECK(glAttachShader(obj, geom_obj));

  GLCHECK(glLinkProgram(obj));
  GLint status;
  GLCHECK(glGetProgramiv(obj, GL_LINK_STATUS, &status));

  GLCHECK(glDetachShader(obj, vert_obj));
  GLCHECK(glDetachShader(obj, pix_obj));
  if(geom_obj)
    GLCHECK(glDetachShader(obj, geom_obj));

  return status ? "" : "Error linking program '" + prog + "'\n" + printShaderLog(obj) + "\n";
}

void ShaderManager::LoadAllShadersFromFile(char const*filename)
{
  for(auto &i: ParseShaderSources(Resource::LoadText(filename)))
    this->LoadShaderSource(move(i.first), move(i.second));
}

void ShaderManager::ClearCache()
{
  for(Val o: m_shader_objects)
    GLCHECK(glDeleteShader(o.second));
  m_shader_objects.clear();
}


map<string, string> code_policy::ParseShaderSources(string text)
{
  map<string, string> parsed_sources;

  size_t start = 0;
  string header;
  if(text.find("//--GLOBAL:") != string::npos)
  {
    start = text.find("//--", 4);
    header = text.substr(11, start - 11);
  }

  for(size_t c_line=start; start!=string::npos; start=text.find("//--", start))
  {
    Val newline_count = cast<size_t>(std::count(text.cbegin() + cast<int64>(c_line), text.cbegin() + cast<int64>(start), '\n'));
    c_line = start;
    header.append(newline_count, '\n');

    if([&]{ Val l = text.substr(start + 4, 3); return l == "VER" || l == "PIX" || l == "GEO"; }())
    {
      start += 7;
      while(text[start] == ' ')
        ++start;
      size_t stop = text.find('\n', start);
      string name = text.substr(start, stop - start);

      start = stop;
      stop = text.find("//--", start);
      if(string::npos == stop)
        stop = text.size();

      Val emplaced = parsed_sources.emplace(name, header + text.substr(start, stop - start)).second;
      if(!emplaced)
        CINFO("Duplicate shader '"<<name<<"' in source file");

      start = stop;
    }
    else
      start += 4;
  }

  return parsed_sources;
}
