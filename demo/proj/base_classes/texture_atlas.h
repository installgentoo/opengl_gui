#pragma once
#include "gl/texture.h"
#include <glm/vec4.hpp>

namespace code_policy
{

struct Vtex
{
  vec4 coord;
  shared_ptr<GLtex2d> tex;
};


template<class m_key, class T> pair<unordered_map<m_key, Vtex>, map<m_key, T>> MakeAtlas(uint max_w, uint max_h, uint channels, map<m_key, T> images);

}
