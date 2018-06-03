#include "camera.h"
#include <glm/gtc/matrix_inverse.hpp>

using namespace code_policy;

void Camera::setProj(mat4 const&p)
{
  m_proj = p;
  m_view_proj = m_proj * m_view;
}

void Camera::setView(mat4 const&v)
{
  m_view = v;
  m_view_proj = m_proj * m_view;
  m_normal = glm::inverseTranspose(mat3(m_view));
}

mat3 Camera::N(mat4 model)const
{
  return glm::inverseTranspose(mat3(model));
}

mat3 Camera::NV(mat4 model)const
{
  return glm::inverseTranspose(mat3(MV(model)));
}
