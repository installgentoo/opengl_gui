#include "resource_control.h"
#include "base_classes/policies/resource.h"
#include "base_classes/policies/serialization.h"
#include <numeric>
#include <regex>

using namespace GUI;
using std::regex;

Vtex const* TextureManager::Register(string filename)
{
  Val p = m_vtex_objects.emplace(filename, Vtex{ });
  if(p.second)
    m_filenames.emplace_back(move(filename));

  return &p.first->second;
}

void TextureManager::LoadRegisteredTextures(uint channels)
{
  map<string, uImage> images;
  for(auto &i: m_filenames)
    images.emplace(i, ImageCodec::Decode<ubyte>(Resource::Load(i), channels));

  Val max_tex_size = cast<uint>(GLState::Iconst<GL_MAX_TEXTURE_SIZE>());

  map<string, Vtex> textures;

  while(!images.empty())
  {
    auto r = MakeAtlas(max_tex_size, max_tex_size, channels, move(images));
    auto texture_batch = move(r.first);

    textures.insert(std::make_move_iterator(texture_batch.begin()), std::make_move_iterator(texture_batch.end()));
    images = move(r.second);
  }

  for(auto &i: m_vtex_objects)
  {
    Val found = textures.find(i.first);
    CASSERT(found != textures.cend(), "Broken font atlas");
    i.second = found->second;
  }
}


const Font *FontManager::Register(pair<string, string> font_description)
{
  CASSERT(!font_description.second.empty(), "No character set specified");
  Val p = m_font_objects.emplace(font_description.first, Font{ });
  CASSERT(p.second, "Don't register the same font twice");
  m_font_descriptions.emplace(move(font_description));
  return &p.first->second;
}

void FontManager::LoadRegisteredFonts(bool cache)
{
  static const char c_m[] = "resources/fonts_cache_map.bin", c_t[] = "resources/font_cache_atlas.png";
  Val glyph_size = 28u, border_size = 2u, supersample_mult = 16u;

  Val signature = std::hash<string>{}(std::to_string(glyph_size) + "_" + std::to_string(border_size) + "_" + std::to_string(supersample_mult) + "_" + std::accumulate(m_font_descriptions.cbegin(), m_font_descriptions.cend(), string{}, [](string v, Val d){ return v += d.first + d.second; }));

  if(cache)
  {
    vector<char> atl_data(Resource::Load(c_m))
        , tex_data(Resource::Load(c_t));
    if(atl_data.size() > sizeof(signature) &&
       signature == reinterpret_cast<size_t const*>(atl_data.data())[0] &&
       !tex_data.empty())
    {
      atl_data.erase(atl_data.cbegin(), atl_data.cbegin() + sizeof(size_t));
      unordered_map<string, Font> fonts_data;
      code_policy::deserialize_from_vec(atl_data, fonts_data);
      Val atl_tex = make_shared<GLtex2d>(ImageCodec::Decode<ubyte>(tex_data, 1), 1);

      for(auto &i: m_font_objects)
      {
        Val found = fonts_data.find(i.first);
        CASSERT(found != fonts_data.cend(), "Broken font atlas");

        i.second = found->second;
        i.second.m_tex = atl_tex;
      }

      return;
    }
  }

  unordered_map<string, Font> font_objects = MakeFonts(m_font_descriptions, glyph_size, border_size, supersample_mult);
  for(auto &i: m_font_objects)
  {
    Val found = font_objects.find(i.first);
    CASSERT(found != font_objects.cend(), "Broken font atlas");
    i.second = found->second;
  }

  Val tex = font_objects.begin()->second.tex();
  Resource::Save(c_t, ImageCodec::Encode(uImage{ tex.width(), tex.height(), 1, GLbind(tex).Save<ubyte>(1) }));

  Val serialized = code_policy::serialize_into_vec(font_objects);
  vector<char> data(reinterpret_cast<char const*>(&signature), reinterpret_cast<char const*>(&signature) + sizeof(size_t));
  data.insert(data.cend(), serialized.cbegin(), serialized.cend());
  Resource::Save(c_m, data);
}


Animation::Animation(TextureManager &manager, char const*filename)
{
  Val data = Resource::LoadText(filename);
  Val match_delay = regex("d [0-9]*")
      , match_frame = regex(".*\\.png");

  uint delay = 0
      , total_delay = 0;
  for(size_t start=0,c_line=data.find('\n', start); c_line!=string::npos; c_line=[&]{ start = c_line + 1; return data.find('\n', start); }())
  {
    Val line = data.substr(start, c_line - start);

    if(std::regex_match(line, match_delay))
    {
      delay = cast<uint>(std::stoi(line.substr(2, string::npos)));
      continue;
    }

    if(std::regex_match(line, match_frame))
    {
      total_delay += delay;
      m_frames.emplace_back(manager.Register(line), delay);
      continue;
    }

    CINFO("Unrecognised line'"<<line<<"' in file "<<filename);
  }

  for(auto &i: m_frames)
    i.second /= total_delay;
}

Vtex const* Animation::currentFrame(double time)
{
  time = glm::clamp(time, 0., 1.);
  double at = 0;

  for(Val i: m_frames)
  {
    at += i.second;
    if(at >= time)
      return i.first;
  }

  return m_frames.back().first;
}
