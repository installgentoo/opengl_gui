#include "mesh.h"
#include "gl/shader.h"

using namespace code_policy;

SHADER(mesh__2d_screen_vs,
R"(#version 330 core
layout(location = 0)in vec4 Position;
out vec2 glTexCoord;

void main()
{
gl_Position = vec4(Position.xy, 0., 1.);
glTexCoord = Position.zw;
})")

SHADER(mesh__2d_screen_ps,
R"(#version 330 core
in vec2 glTexCoord;
layout(location = 0)out vec4 glFragColor;
uniform sampler2D tex;

void main()
{
glFragColor = texture(tex, glTexCoord);
})")

Plane::Plane(float x)
{
  auto b = GLbind(m_obj);
  GLbind(m_vbo_idx).AllocateBuffer(vector<GLubyte>{ 0, 1, 3, 3, 1, 2 });
  GLbind(m_vbo_xy).AllocateBuffer(vector<GLfloat>{ -x, -x, 0.,  x, -x, 0.,  x, x, 0.,  -x, x, 0. });
  b.AttribFormat(m_vbo_xy, 0, 3);

  GLbind(m_vbo_uv).AllocateBuffer(vector<GLbyte>{ 0, 0,  1, 0,  1, 1,  0, 1 });
  b.AttribFormat(m_vbo_uv, 1, 2, GL_BYTE);

  GLbind(m_vbo_norm).AllocateBuffer(vector<GLbyte>{ 0, 0, 1,  0, 0, 1,  0, 0, 1,  0, 0, 1, });
  b.AttribFormat(m_vbo_norm, 2, 3, GL_BYTE);
}


Skybox::Skybox(byte s)
{
  auto b = GLbind(m_obj);
  GLbind(m_vbo_idx).AllocateBuffer(vector<GLubyte>{ 0, 1, 3,  3, 1, 2,
                                                    4, 5, 7,  7, 5, 6,

                                                    0, 1, 4,  4, 1, 5,
                                                    3, 2, 7,  7, 2, 6,

                                                    2, 1, 6,  6, 1, 5,
                                                    3, 7, 0,  0, 7, 4 });

  byte n = cast<byte>(-s);
  GLbind(m_vbo_xyz).AllocateBuffer(vector<GLbyte>{ n, s, s,  s, s, s,  s, s, n,  n, s, n,
                                                   n, n, s,  s, n, s,  s, n, n,  n, n, n });
  GLbind(m_obj).AttribFormat(m_vbo_xyz, 0, 3, GL_BYTE);
}


Mesh::Mesh(Model const&model)
{
  m_n_verts = model.indices.size();

  auto b = GLbind(m_obj);
  GLbind(m_vbo_idx).AllocateBuffer(model.indices);

  GLbind(m_vbo_xyz).AllocateBuffer(model.vertices);
  b.AttribFormat(m_vbo_xyz, 0, 3);

  GLbind(m_vbo_norm).AllocateBuffer(model.normals);
  b.AttribFormat(m_vbo_norm, 2, 3);
}
