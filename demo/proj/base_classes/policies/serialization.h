#pragma once
#include "code.h"
#include <cereal/archives/binary.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>
#include <sstream>

namespace code_policy
{

template<class...P> vector<char> serialize_into_vec(P &&...p)
{
  std::stringstream s;
  {
    cereal::BinaryOutputArchive a(s);
    a(forward<P>(p)...);
  }

  std::streamsize size = 0;
  if(s.seekg(0, std::ios::end).good())
    size = s.tellg();
  if(s.seekg(0, std::ios::beg).good())
    size -= s.tellg();
  vector<char> data(size_t(size), 0);
  if(size > 0)
    s.read(data.data(), size);
  return data;
}

template<class...P> void deserialize_from_vec(vector<char> const&data, P &&...p)
{
  std::stringstream s;

  s.write(data.data(), static_cast<std::streamsize>(data.size()));
  {
    cereal::BinaryInputArchive a(s);
    a(forward<P>(p)...);
  }
}

}
