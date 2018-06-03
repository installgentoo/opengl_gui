#pragma once
#include "base_classes/gl/shader.h"
#include "base_classes/gl/texture.h"

namespace code_policy
{

struct EnvTex
{
  float mip_levels;
  GLtexCube specular, irradiance;
};


struct EnvironmentGenerator
{
  struct Environment {
    template<class A> void serialize(A &a) { a(diffuse, specular); }

    array<fImage, 6> diffuse;
    array<vector<fImage>, 6> specular;
  };

  Environment generate(GLtex2d const&equirect)const;
  fImage generate_lut()const;

  static EnvTex LoadEnvmap(Environment const&e);

private:
  const GLshader
  m_equirect =   { "environment_gen_vs", "environment_unwrap_equirect_ps" },
  m_irradiance = { "environment_gen_vs", "environment_gen_irradiance_ps" },
  m_specular =   { "environment_gen_vs", "environment_gen_spec_ps" },
  m_lut =        { "mesh__2d_screen_vs", "environment_gen_lut_ps" };
};

}
