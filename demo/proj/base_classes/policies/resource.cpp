#include "resource.h"
#include "logging.h"
#include <fstream>

using namespace code_policy;
using namespace std;

static_assert(sizeof(ubyte) == sizeof(unsigned char), "Platform char size not supported by lodepng");

static vector<ubyte> invertImage(vector<ubyte> image, uint height, uint stride)
{
  for(uint i=0; i<height/2; ++i)
  {
    auto top = image.begin() + stride * i
        , bottom = image.end() - stride * (i + 1);

    std::swap_ranges(top, top + stride, bottom);
  }

  auto ret = move(image);
  return ret;
}


string OnDemandResourcePolicy::LoadText(string const&name)
{
  ifstream file(name, ifstream::in);
  string str;
  if(file.is_open())
  {
    file.exceptions(ifstream::failbit | ifstream::badbit);
    try
    {
      str = string{ istreambuf_iterator<char>(file), istreambuf_iterator<char>() };
      file.close();
    }
    catch(ifstream::failure const&) { CERROR("Error accessing file "<<name); }
  }
  else
    CINFO("No file "<<name);

  return str;
}

std::vector<char> OnDemandResourcePolicy::Load(string const&name)
{
  ifstream file(name, ifstream::binary | ifstream::in);
  vector<char> data;
  if(file.is_open())
  {
    file.exceptions(ifstream::failbit | ifstream::badbit);
    try
    {
      streamsize size = 0;
      if(file.seekg(0, ios::end).good())
        size = file.tellg();
      if(file.seekg(0, ios::beg).good())
        size -= file.tellg();
      data.resize(cast<size_t>(size));
      if(size > 0)
        file.read(data.data(), size);
      file.close();
    }
    catch(ifstream::failure const&) { CERROR("Error accessing file "<<name); }
  }
  else
    CINFO("No file "<<name);

  return data;
}

bool OnDemandResourcePolicy::Save(string const&name, vector<char> const&data)
{
  ofstream file(name, ofstream::binary | ofstream::out);
  if(file.is_open())
  {
    file.exceptions(ofstream::failbit | ofstream::badbit);
    try
    {
      file.write(data.data(), cast<streamsize>(data.size()));
      file.close();
      return true;
    }
    catch(ofstream::failure const&) { CINFO("Error writing to file "<<name); }
    file.close();
  }
  else
    CINFO("Can't create/open file "<<name);

  return false;
}


#ifdef USE_STB
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_JPEG
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_STDIO
#define STBI_FAILURE_USERMSG
#include <stb_image.h>
#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <cstring>

template<class T> T* imgAdpt(ubyte const*, size_t, int*, int*, int);
template<> ubyte* imgAdpt(ubyte const*img, size_t s, int *w, int *h, int c) { int _c; return stbi_load_from_memory(img, s, w, h, &_c, c); }
template<> float* imgAdpt(ubyte const*img, size_t s, int *w, int *h, int c) { int _c; return stbi_loadf_from_memory(img, s, w, h, &_c, c); }

template<class T> Image<T> StbDecodePolicy::Decode(vector<char> const&data, uint channels, bool flip)
{
  int _w, _h;
  stbi_set_flip_vertically_on_load(flip);
  T *buff = imgAdpt<T>(reinterpret_cast<ubyte const*>(data.data()), data.size(), &_w, &_h, cast<int>(channels));
  Val w = cast<uint>(_w), h = cast<uint>(_h);

  if(!buff)
    CERROR(stbi_failure_reason());

  vector<T> img(buff, buff + w * h * channels);
  free(buff);

  return { w, h, channels, move(img) };
}
template uImage StbDecodePolicy::Decode(vector<char> const&, uint, bool);
template fImage StbDecodePolicy::Decode(vector<char> const&, uint, bool);

vector<char> StbDecodePolicy::Encode(uImage img, bool flip)
{
  int _l;
  stbi_flip_vertically_on_write(flip);
  ubyte *buff = stbi_write_png_to_mem(img.data.data(), cast<int>(img.width * img.channels), cast<int>(img.width), cast<int>(img.height), cast<int>(img.channels), &_l);

  if(!buff)
    CERROR(stbi_failure_reason());

  vector<char> data(buff, buff + _l);
  free(buff);

  return data;
}

static void vec_write(void *context, void *data, int s)
{
  auto &buff = *reinterpret_cast<vector<char>*>(context);
  Val last_size = buff.size();
  Val size = cast<uint>(s);
  buff.resize(last_size + size);
  std::memcpy(reinterpret_cast<char*>(buff.data()), data, size);
}

vector<char> StbDecodePolicy::Encode(fImage img, bool flip)
{
  vector<char> buff;

  CASSERT(0, "stbi_write_hdr_to_func is broken, check out update");

  stbi_flip_vertically_on_write(flip);
  if(!stbi_write_hdr_to_func(vec_write, &buff, cast<int>(img.width), cast<int>(img.height), cast<int>(img.channels), img.data.data()))
    CERROR(stbi_failure_reason());

  return buff;
}
#endif


#ifdef USE_LODEPNG
#define LODEPNG_NO_COMPILE_DISK
#include <lodepng.h>

namespace code_policy {
template<> uImage LodepngDecodePolicy::Decode(vector<char> const&data, uint channels, bool flip)
{
  LodePNGColorType colortype = LCT_RGB;
  switch(channels)
  {
    case 1: colortype = LCT_GREY; break;
    case 3: colortype = LCT_RGB;  break;
    case 4: colortype = LCT_RGBA; break;
    default: CASSERT(0, "Invalid channels for image loading");
  }

  uint w, h;
  vector<ubyte> img;
  if(Val error = lodepng::decode(img, w, h, reinterpret_cast<ubyte const*>(data.data()), data.size(), colortype, 8))
    CERROR("Lodepng error: "<<lodepng_error_text(error));

  return {  w, h, channels, flip ? invertImage(move(img), h, w * channels) : move(img) };
  }
}

vector<char> LodepngDecodePolicy::Encode(uImage img, bool flip)
{
  Val data = flip ? invertImage(move(img.data), img.height, img.width * img.channels) : move(img.data);
  LodePNGColorType colortype = LCT_RGB;
  switch(img.channels)
  {
    case 1: colortype = LCT_GREY; break;
    case 3: colortype = LCT_RGB;  break;
    case 4: colortype = LCT_RGBA; break;
    default: CASSERT(0, "Invalid channels for image saving");
  }

  ubyte* buff = nullptr;
  size_t buff_s = 0;
  if(Val error = lodepng_encode_memory(&buff, &buff_s, data.data(), img.width, img.height, colortype, 8))
    CERROR("Lodepng error: "<<lodepng_error_text(error));

  vector<char> image(buff, buff + buff_s);
  free(buff);
  return image;
}
#endif


#ifdef USE_LZ4
#include <lz4.h>

Archive LZ4CompressionPolicy::Compress(vector<char> const&data)
{
  vector<char> buff(cast<uint>(LZ4_compressBound(data.size())));
  Val written = cast<uint>(LZ4_compress_default(data.data(), buff.data(), data.size(), buff.size()));
  if(!written)
    CERROR("Compression failed");

  buff.resize(written);
  return { cast<uint>(data.size()), move(buff) };
}

vector<char> LZ4CompressionPolicy::Extract(Archive const&a)
{
  vector<char> buff(a.orig_size);
  Val written = LZ4_decompress_safe(a.data.data(), buff.data(), a.data.size(), cast<int>(buff.size()));
  if(written < 0)
    CERROR("Malformed archive");

  buff.resize(cast<uint>(written));
  return buff;
}
#endif
