#pragma once
#include "base_classes/gl/shader.h"
#include "base_classes/gl/texture.h"
#include "base_classes/mesh.h"
#include "base_classes/policies/window.h"

namespace code_policy
{

inline void ActivateTexturesByArg(uint) { }

template<class...P> void ActivateTexturesByArg(uint &n, GLtex2d const&a, P const&...p) {
  GLbind(a, n);
  ++n;
  ActivateTexturesByArg(n, p...);
}

template<class...P> void ActivateTexturesByArg(uint &n, GLfbo const&a, P const&...p) {
  GLbind(a.tex(), n);
  ++n;
  ActivateTexturesByArg(n, p...);
}

template<class...P> void ComputeShader(GLbindingShader const&, GLfbo const&tgt, P const&...p) {
  uint texindex = 0;
  ActivateTexturesByArg(texindex, p...);
  GLbind(tgt).Clear();
  Screen::Draw();
}

template<class...P> void ComputeShader(GLbindingShader const&, Window &w, P const&...p) {
  uint texindex = 0;
  ActivateTexturesByArg(texindex, p...);
  w.DrawToScreen(true);
  Screen::Draw();
}

template<class...P> void ComputeShader(GLbindingShader const&s, Slab &slab, P const&...p) {
  ComputeShader(s, slab.tgt, slab.src, p...);
  slab.Swap();
}

}
