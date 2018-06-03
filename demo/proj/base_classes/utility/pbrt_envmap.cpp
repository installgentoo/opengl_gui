#include "pbrt_envmap.h"
#include "base_classes/camera.h"
#include "base_classes/mesh.h"
#include "base_classes/gl/shader.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace code_policy;

SHADER(environment_gen_vs,
R"(#version 330 core
layout(location = 0)in vec3 Position;
uniform mat4 MVPMat;
out vec3 glTexCoord;

void main()
{
vec4 pos = vec4(Position, 1.);
gl_Position = MVPMat * pos;
glTexCoord = Position;
})")

SHADER(environment_unwrap_equirect_ps,
R"(#version 330 core
in vec3 glTexCoord;
layout(location = 0)out vec4 glFragColor;
uniform sampler2D equirect_tex;

void main()
{
  vec3 v = normalize(glTexCoord);
  vec2 uv = vec2(atan(v.z, v.x), asin(v.y)) * vec2(0.1591, 0.3183) + vec2(0.5);
  vec3 c = texture(equirect_tex, uv).rgb;
  glFragColor = vec4(c, 1.);
})")

SHADER(environment_gen_irradiance_ps,
R"(#version 330 core
in vec3 glTexCoord;
layout(location = 0)out vec4 glFragColor;
uniform samplerCube env_cubetex;
uniform float delta;

const float M_PI = 3.14159265358979323846;

void main()
{
vec3 normal = normalize(glTexCoord);
vec3 right = cross(vec3(0., 1., 0.), normal);
vec3 up = cross(normal, right);

vec3 irradiance = vec3(0.);
float n_samples = 0.;
for(float phi=0.; phi<2.*M_PI; phi+=delta)
{for(float theta=0.; theta<0.5*M_PI; theta+=delta)
{
vec3 tangent_sample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
vec3 sample_vec = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * normal;
irradiance += texture(env_cubetex, sample_vec).rgb * cos(theta) * sin(theta);
++n_samples;
}}

irradiance = M_PI * irradiance / n_samples;
glFragColor = vec4(irradiance, 1.);
})")

static const string shd_transforms = R"(
const float M_PI = 3.14159265358979323846;

float RadicalInverse_VdC(uint bits)
{
bits = (bits << 16u) | (bits >> 16u);
bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
float a = roughness * roughness;

float phi = 2. * M_PI * Xi.x;
float cosTheta = sqrt((1. - Xi.y) / (1. + (a * a - 1.) * Xi.y));
float sinTheta = sqrt(1. - cosTheta * cosTheta);

vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

vec3 up = abs(N.z) < 0.999 ? vec3(0., 0., 1.) : vec3(1., 0., 0.);
vec3 tangent = normalize(cross(up, N));
vec3 bitangent = cross(N, tangent);

vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
return normalize(sampleVec);
})";

SHADER(environment_gen_spec_ps,
R"(#version 330 core
in vec3 glTexCoord;
layout(location = 0)out vec4 glFragColor;
uniform samplerCube env_cubetex;
uniform float roughness;
uniform int samples;
)" + shd_transforms + R"(
void main()
{
vec3 N = normalize(glTexCoord);

float totalWeight = 0.;
vec3 prefilteredColor = vec3(0.);
uint SAMPLE_COUNT = uint(samples);
for(uint i=0u; i<SAMPLE_COUNT; ++i)
{
vec2 Xi = Hammersley(i, SAMPLE_COUNT);
vec3 H = ImportanceSampleGGX(Xi, N, roughness);
vec3 L = normalize(2. * dot(N, H) * H - N);

float NdotL = max(dot(N, L), 0.);
if(NdotL > 0.)
{
prefilteredColor += texture(env_cubetex, L).rgb * NdotL;
totalWeight += NdotL;
}
}
prefilteredColor /= totalWeight;

glFragColor = vec4(prefilteredColor, 1.);
})")

SHADER(environment_gen_lut_ps,
R"(#version 330 core
in vec2 glTexCoord;
layout(location = 0)out vec4 glFragColor;
uniform int samples;
)" + shd_transforms + R"(
float GeometrySchlickGGX(float NdotV, float roughness)
{
float k = (roughness * roughness) / 2.;
float denom = NdotV * (1. - k) + k;
return NdotV / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
float NdotV = max(dot(N, V), 0.);
float NdotL = max(dot(N, L), 0.);
float ggx2 = GeometrySchlickGGX(NdotV, roughness);
float ggx1 = GeometrySchlickGGX(NdotL, roughness);

return ggx1 * ggx2;
}

vec2 IntegrateBRDF(float NdotV, float roughness)
{
vec3 V = vec3(sqrt(1. - NdotV * NdotV), 0., NdotV);

float A = 0.;
float B = 0.;
vec3 N = vec3(0., 0., 1.);
uint SAMPLE_COUNT = uint(samples);
for(uint i=0u; i<SAMPLE_COUNT; ++i)
{
vec2 Xi = Hammersley(i, SAMPLE_COUNT);
vec3 H = ImportanceSampleGGX(Xi, N, roughness);
vec3 L = normalize(2. * dot(V, H) * H - V);

float NdotL = max(L.z, 0.);
if(NdotL > 0.)
{
float NdotH = max(H.z, 0.);
float VdotH = max(dot(V, H), 0.);

float G = GeometrySmith(N, V, L, roughness);
float G_Vis = (G * VdotH) / (NdotH * NdotV);
float Fc = pow(1. - VdotH, 5.);

A += (1. - Fc) * G_Vis;
B += Fc * G_Vis;
}
}
A /= float(SAMPLE_COUNT);
B /= float(SAMPLE_COUNT);
return vec2(A, B);
}

void main()
{
glFragColor = vec4(IntegrateBRDF(glTexCoord.x, glTexCoord.y), 0., 1.);
})")

EnvironmentGenerator::Environment EnvironmentGenerator::generate(GLtex2d const&equirect)const
{
  Val side = [](Val to, Val up){ return glm::lookAt(vec3(0), to, up); };
  static const mat4 proj = glm::perspective(glm::radians(90.f), 1.f, 0.1f, 10.f)
      , sides[] = { side(vec3( 1,  0,  0), vec3(0, -1,  0)),
                    side(vec3(-1,  0,  0), vec3(0, -1,  0)),
                    side(vec3( 0,  1,  0), vec3(0,  0,  1)),
                    side(vec3( 0, -1,  0), vec3(0,  0, -1)),
                    side(vec3( 0,  0,  1), vec3(0, -1,  0)),
                    side(vec3( 0,  0, -1), vec3(0, -1,  0)) };

  Val cube = Skybox(1);

  array<fImage, 6> color, diffuse;
  array<vector<fImage>, 6> specular;

  for(uint i=0; i<6; ++i)
  {
    Val cam = Camera(proj, sides[i]);

    auto b = GLbind(m_equirect);
    b.Uniforms("MVPMat", cam.MVP(),
               "equirect_tex", 0);

    Val surf = GLfbo(512, 512, 4, GL_FLOAT);
    GLbind(surf);

    GLbind(equirect, 0);
    cube.Draw();

    Val tex = surf.tex();
    color[i] = fImage{ tex.width(), tex.height(), 3, GLbind(tex).Save<float>(3) };
  }

  Val cubemap = GLtexCube(color, 4);

  for(uint i=0; i<6; ++i)
  {
    Val cam = Camera(proj, sides[i]);

    {
      auto b = GLbind(m_irradiance);
      b.Uniforms("MVPMat", cam.MVP(),
                 "env_cubetex", 0,
                 "delta", 0.025);

      Val surf = GLfbo(32, 32, 4, GL_FLOAT);
      GLbind(surf);

      GLbind(cubemap, 0);
      cube.Draw();

      Val tex = surf.tex();
      diffuse[i] = fImage{ tex.width(), tex.height(), 3, GLbind(tex).Save<float>(3) };
    }

    CASSERT(cubemap.width() == cubemap.height(), "Cubemap mipmaps require square images, see spec");
    Val max_mip = cast<uint>(glm::ceil(glm::log(cubemap.width()) / glm::log(2.) - 2.));

    specular[i].emplace_back(move(color[i]));

    for(uint l=1; l<=max_mip; ++l)
    {
      auto b = GLbind(m_specular);
      b.Uniforms("MVPMat", cam.MVP(),
                 "env_cubetex", 0,
                 "samples", 4096,
                 "roughness", double(l) / max_mip);

      Val side = cast<uint>(glm::pow(2u, 2u + max_mip - l));
      Val mip = GLfbo(side, side, 4, GL_FLOAT);
      GLbind(mip);

      GLbind(cubemap, 0);
      cube.Draw();

      Val tex = mip.tex();
      specular[i].emplace_back(fImage{ tex.width(), tex.height(), 3, GLbind(tex).Save<float>(3) });
    }
  }

  return { move(diffuse), move(specular) };
}

fImage EnvironmentGenerator::generate_lut() const
{
  auto b = GLbind(m_lut);
  b.Uniforms("samples", 4096);

  Val surf = GLfbo(512, 512, 4, GL_FLOAT);
  GLbind(surf);
  Screen::Draw();

  Val tex = surf.tex();
  return { tex.width(), tex.height(), 2, GLbind(tex).Save<float>(2) };
}

EnvTex EnvironmentGenerator::LoadEnvmap(Environment const&e)
{
  GLtexCube base({ e.specular[0][0], e.specular[1][0], e.specular[2][0],
                   e.specular[3][0], e.specular[4][0], e.specular[5][0] }, 3);

  Val max_mip = cast<uint>(glm::ceil(glm::log(base.width()) / glm::log(2.) - 2.));

  {
    auto b = GLbind(base);
    b.GenMipmaps();
    for(uint j=0; j<6; ++j)
      for(uint l=1; l<=max_mip; ++l)
        b.Load(l, j, e.specular[j][l], 3);
  }

  return { float(max_mip), move(base), GLtexCube(e.diffuse, 3) };
}
