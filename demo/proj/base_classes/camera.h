#pragma once
#include "policies/code.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace code_policy
{

struct Camera
{
  Camera(mat4 const&proj, mat4 const&view)
  {
    this->setProj(proj);
    this->setView(view);
  }

  void setProj(mat4 const&p);
  void setView(mat4 const&v);

  Val  V()const             { return m_view;              }
  Val  MVP()const           { return m_view_proj;         }
  mat4 MV(mat4 model)const  { return m_view * model;      }
  mat4 MVP(mat4 model)const { return m_view_proj * model; }
  mat3 N(mat4 model)const;
  mat3 NV(mat4 model)const;

private:
  mat3 m_normal;
  mat4 m_view, m_proj, m_view_proj;
};

}
