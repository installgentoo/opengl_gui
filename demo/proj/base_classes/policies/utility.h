#pragma once
#include "code.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace code_policy
{

class CUNIQUE
{
  CUNIQUE(CUNIQUE const&) = delete;
  CUNIQUE& operator=(CUNIQUE const&) = delete;
protected:
  CUNIQUE() = default;
  ~CUNIQUE() = default;
};


template<class T> struct Image
{
  template<class A> void serialize(A &a) { a(width, height, channels, data); }

  bool operator==(Image<T> const&r)const {
    return width == r.width && height == r.height && channels == r.channels && std::equal(data.cbegin(), data.cend(), r.data.cbegin());
  }

  uint width, height, channels;
  vector<T> data;
};
using uImage = Image<ubyte>;
using fImage = Image<float>;


struct Archive
{
  template<class A> void serialize(A &a) { a(orig_size, data); }

  uint orig_size;
  vector<char> data;
};

}
