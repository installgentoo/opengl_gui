#include "texture.h"

using namespace code_policy;

template<typename T> GLenum getTypeName();
template<> GLenum getTypeName<ubyte>() { return GL_UNSIGNED_BYTE; }
template<> GLenum getTypeName<float>() { return GL_FLOAT;         }

static uint getTypeSize(GLenum PRECISION)
{
  switch(PRECISION)
  {
    case GL_UNSIGNED_BYTE:
    case GL_BYTE:   return 1;

    case GL_HALF_FLOAT:
    case GL_UNSIGNED_SHORT:
    case GL_SHORT:  return 2;

    case GL_FLOAT:
    case GL_UNSIGNED_INT:
    case GL_INT:    return 4;

    case GL_DOUBLE: return 8;
    default: CASSERT(0, "Invalid channels");
  }
  return 0;
}

static uint getChannelsName(uint channels)
{
  switch(channels)
  {
    case 1: return GL_RED;
    case 2: return GL_RG;
    case 3: return GL_RGB;
    case 4: return GL_RGBA;
    default: CASSERT(0, "Invalid channels");
  }
  return 0;
}

static GLint getFormat(uint channels, GLenum PRECISION)
{
  GLint internal_format = GL_RGBA8;
  switch(channels)
  {
    case 1:
      switch(PRECISION)
      {
        case GL_UNSIGNED_BYTE:
        case GL_BYTE: internal_format = GL_R8; break;
        case GL_HALF_FLOAT: internal_format = GL_R16F; break;
        case GL_FLOAT: internal_format = GL_R32F; break;
        default: CASSERT(0, "No such texture can be created");
      } break;
    case 2:
      switch(PRECISION)
      {
        case GL_UNSIGNED_BYTE:
        case GL_BYTE: internal_format = GL_RG8; break;
        case GL_HALF_FLOAT: internal_format = GL_RG16F; break;
        case GL_FLOAT: internal_format = GL_RG32F; break;
        default: CASSERT(0, "No such texture can be created");
      } break;
    case 3:
      switch(PRECISION)
      {
        case GL_UNSIGNED_BYTE:
        case GL_BYTE: internal_format = GL_RGB8; break;
        case GL_HALF_FLOAT: internal_format = GL_RGB9_E5; break;
        case GL_FLOAT: internal_format = GL_RGB32F; break;
        default: CASSERT(0, "No such texture can be created");
      } break;
    case 4:
      switch(PRECISION)
      {
        case GL_UNSIGNED_BYTE:
        case GL_BYTE: internal_format = GL_RGBA8; break;
        case GL_HALF_FLOAT: internal_format = GL_RGBA16F; break;
        case GL_FLOAT: internal_format = GL_RGBA32F; break;
        default: CASSERT(0, "No such texture can be created");
      } break;
    default: CASSERT(0, "Invalid channels");
  }

  return internal_format;
}

static void checkParameters(uint width, uint height, uint alignment)
{
  CASSERT(width > 0 && height > 0, "Texture of zero size");
  CASSERT(alignment == 1 || alignment == 2 || alignment == 4 || alignment == 8, "Incorrect texture data alignment");
  CASSERT((cast<int>(width) <= GLState::Iconst<GL_MAX_TEXTURE_SIZE>()) && (cast<int>(height) <= GLState::Iconst<GL_MAX_TEXTURE_SIZE>()), "Texture exceeds maximum size");
}


uint64 GLtexStats::Size(uint level)const
{
  if(!level)
    return cast<uint64>(width) * height;

  Val _width = cast<uint64>(glm::max(1., glm::floor(double(width) / glm::pow(2, level))))
      , _height = cast<uint64>(glm::max(1., glm::floor(double(height) / glm::pow(2, level))));

  return _width * _height;
}


GLtex2d::GLtex(uint width, uint height, uint channels, GLenum PRECISION, void const*data, GLenum FORMAT, GLenum TYPE, uint alignment)
{
  auto b = GLbind(*this);
  b.Load(0, width, height, channels, PRECISION, data, FORMAT, TYPE, alignment);
  b.Parameters(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

template<class T> GLtex2d::GLtex(T const&img, uint channels, uint alignment)
{
  auto b = GLbind(*this);
  b.Load(0, img, channels, alignment);
  b.Parameters(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}
template GLtex2d::GLtex(uImage const&, uint, uint);
template GLtex2d::GLtex(fImage const&, uint, uint);


GLtexCube::GLtex(uint width, uint height, uint channels, GLenum PRECISION, array<void const*, 6> data, GLenum FORMAT, GLenum TYPE, uint alignment)
  : m_stats({ width, height, channels, PRECISION })
{
  auto b = GLbind(*this);
  for(uint i=0; i<data.size(); ++i)
  {
    Val side = data[i];
    b.Load(0, i, width, height, channels, PRECISION, side, FORMAT, TYPE, alignment);
  }

  b.Parameters(GL_TEXTURE_MIN_FILTER, GL_LINEAR, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

GLtexCube::GLtex(array<uImage, 6> const&img, uint channels, uint alignment)
  : GLtex(img[0].width, img[0].height, channels, GL_BYTE, { img[0].data.data(), img[1].data.data(), img[2].data.data(), img[3].data.data(), img[4].data.data(), img[5].data.data() }, getChannelsName(img[0].channels), GL_UNSIGNED_BYTE, alignment)
{ }

GLtexCube::GLtex(array<fImage, 6> const&img, uint channels, uint alignment)
  : GLtex(img[0].width, img[0].height, channels, GL_HALF_FLOAT, { img[0].data.data(), img[1].data.data(), img[2].data.data(), img[3].data.data(), img[4].data.data(), img[5].data.data() }, getChannelsName(img[0].channels), GL_FLOAT, alignment)
{ }


template<class T> vector<T> GLtex2dBinding::Save(uint channels)const
{
  vector<T> img(r_stats.Size() * channels, 0);
  this->Save(img.data(), getChannelsName(channels), getTypeName<T>());
  return img;
}
template vector<ubyte> GLtex2dBinding::Save(uint)const;
template vector<float> GLtex2dBinding::Save(uint)const;

void GLtex2dBinding::Save(void *ptr, GLenum FORMAT, GLenum TYPE) const
{
  GLState::PixelStoreSave::Set(1);
  GLCHECK(glGetTexImage(GL_TEXTURE_2D, 0, FORMAT, TYPE, ptr));
}

void GLtex2dBinding::Load(uint lod, uint width, uint height, uint channels, GLenum PRECISION, void const*data, GLenum FORMAT, GLenum TYPE, uint alignment)
{
  checkParameters(width, height, alignment);
  r_stats = { width, height, channels, PRECISION };
  if(data)
    GLState::PixelStoreLoad::Set(cast<int>(alignment));
  GLCHECK(glTexImage2D(GL_TEXTURE_2D, cast<GLint>(lod), getFormat(channels, PRECISION), cast<GLint>(width), cast<GLint>(height), 0, FORMAT, TYPE, data));
}

void GLtex2dBinding::Load(uint lod, uImage const&img, uint channels, uint alignment)
{
  this->Load(lod, img.width, img.height, channels, GL_BYTE, img.data.empty() ? nullptr : img.data.data(), getChannelsName(img.channels), GL_UNSIGNED_BYTE, alignment);
}

void GLtex2dBinding::Load(uint lod, fImage const&img, uint channels, uint alignment)
{
  this->Load(lod, img.width, img.height, channels, GL_HALF_FLOAT, img.data.empty() ? nullptr : img.data.data(), getChannelsName(img.channels), GL_FLOAT, alignment);
}


void GLtexCubeBinding::Load(uint lod, uint target, uint width, uint height, uint channels, GLenum PRECISION, void const*data, GLenum FORMAT, GLenum TYPE, uint alignment)
{
  checkParameters(width, height, alignment);
  if(data)
    GLState::PixelStoreLoad::Set(cast<int>(alignment));
  GLCHECK(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + target, cast<GLint>(lod), getFormat(channels, PRECISION), cast<GLint>(width), cast<GLint>(height), 0, FORMAT, TYPE, data));
}

void GLtexCubeBinding::Load(uint lod, uint target, uImage const&img, uint channels, uint alignment)
{
  this->Load(lod, target, img.width, img.height, channels, GL_BYTE, img.data.empty() ? nullptr : img.data.data(), getChannelsName(img.channels), GL_UNSIGNED_BYTE, alignment);
}

void GLtexCubeBinding::Load(uint lod, uint target, fImage const&img, uint channels, uint alignment)
{
  this->Load(lod, target, img.width, img.height, channels, GL_HALF_FLOAT, img.data.empty() ? nullptr : img.data.data(), getChannelsName(img.channels), GL_FLOAT, alignment);
}



GLfbo::GLfbo(uint width, uint height, uint channels, GLenum PRECISION)
  : m_tex(width, height, channels, PRECISION, nullptr)
{
  GLbind(m_tex).Parameters(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  auto b = GLbind(*this);
  GLCHECK(glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex.obj(), 0));
  b.Clear();
}

void GLfboBinding::Clear(GLclampf v)
{
  GLState::ClearColor(v);
  GLState::Clear(GL_COLOR_BUFFER_BIT);
  GLState::ClearColor(0);
}
