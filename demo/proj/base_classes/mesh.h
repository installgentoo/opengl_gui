#pragma once
#include "gl/objects.h"

namespace Assimp { class Importer; }
namespace code_policy
{

struct Screen
{
  static void Draw() {
    static Screen s_q;
    GLbind(s_q.m_obj);
    GLCHECK(glDrawArrays(GL_TRIANGLES, 0, 3));
  }

private:
  GLvao m_obj;
  GLbuffer<GL_ARRAY_BUFFER> m_vbo_xyuv;

  Screen()
  {
    GLbind(m_vbo_xyuv).AllocateBuffer(vector<GLbyte>{ -1, -1, 0, 0,  3, -1, 2, 0,  -1, 3, 0, 2 });
    GLbind(m_obj).AttribFormat(m_vbo_xyuv, 0, 4, GL_BYTE);
  }
};


struct Plane
{
  Plane(float x=1.);

  void Draw()const { Val b = GLbind(m_obj); b.Draw(GL_UNSIGNED_BYTE, 6); }

private:
  GLvao m_obj;
  GLbuffer<GL_ELEMENT_ARRAY_BUFFER> m_vbo_idx;
  GLbuffer<GL_ARRAY_BUFFER> m_vbo_xy, m_vbo_uv, m_vbo_norm;
};


struct Skybox
{
  Skybox(byte scale);

  void Draw()const { Val b = GLbind(m_obj); b.Draw(GL_UNSIGNED_BYTE, 36); }

private:
  GLvao m_obj;
  GLbuffer<GL_ELEMENT_ARRAY_BUFFER> m_vbo_idx;
  GLbuffer<GL_ARRAY_BUFFER> m_vbo_xyz;
};


struct Mesh
{
  struct Model
  {
    template<class A> void serialize(A &a) { a(indices, vertices, normals); }

    vector<uint> indices;
    vector<float> vertices, normals;
  };

  Mesh() = default;
  Mesh(Model const&model);

  void Draw()const { Val b = GLbind(m_obj); b.Draw(GL_UNSIGNED_INT, cast<uint>(m_n_verts)); }

private:
  int m_n_verts = 0;
  GLvao m_obj;
  GLbuffer<GL_ELEMENT_ARRAY_BUFFER> m_vbo_idx;
  GLbuffer<GL_ARRAY_BUFFER> m_vbo_xyz, m_vbo_norm;
};

}
