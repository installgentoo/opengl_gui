//--GLOBAL:
#version 330 core

//--PIX vs_skybox


layout(location = 0)in vec3 Position;
uniform mat4 MVPMat;
uniform mat4 ModelViewMat;
out vec3 glTexCoord;

void main()
{
  vec4 pos = vec4(Position, 1.);
  gl_Position = MVPMat * pos;
  glTexCoord = Position;
}


//--PIX ps_skybox


in vec3 glTexCoord;
layout(location = 0)out vec4 glFragColor;
uniform samplerCube skybox_tex;
uniform float exposure;

const float gamma = 2.2;

void main()
{
  vec3 c =  textureLod(skybox_tex, glTexCoord, 0.).rgb;
  c = vec3(1.) - exp(-c * exposure);
  c = pow(c, vec3(1. / gamma));
  glFragColor = vec4(c, 1.);
}


//--VER vs_material_based_render


layout(location = 0)in vec3 Position;
layout(location = 1)in vec2 TexCoord;
layout(location = 2)in vec3 Normal;
uniform mat4 MVPMat;
uniform mat4 ModelViewMat;
uniform mat3 NormalViewMat;
uniform mat3 NormalMat;
out vec3 glPos;
out vec2 glTexCoord;
out vec3 glNormal;
out vec3 glNormalWorld;

void main()
{
  vec4 pos = vec4(Position, 1.);
  gl_Position = MVPMat * pos;
  glPos = (ModelViewMat * pos).xyz;
  glTexCoord = TexCoord;
  glNormal = NormalViewMat * Normal;
  glNormalWorld = NormalMat * Normal;
}


//--PIX ps_material_based_render


in vec3 glPos;
in vec2 glTexCoord;
in vec3 glNormal;
in vec3 glNormalWorld;
layout(location = 0)out vec4 glFragColor;
uniform samplerCube irradiance_cubetex;
uniform samplerCube specular_cubetex;
uniform sampler2D brdf_lut;
uniform vec3 camera_world;
uniform vec3 light_pos[4];
uniform vec4 light_color[4];

uniform vec3 albedo;
uniform float metallicity;
uniform float roughness;
uniform float exposure;
uniform float max_lod;

const float gamma = 2.2;
const float ao = 0.1;
const float refractive_index = 0.1;

const float M_PI = 3.14159265358979323846;

vec3 fresnelSchlick(float cos_theta, vec3 F0, float roughness)
{
  return F0 + (max(vec3(1. - roughness), F0) - F0) * pow(1. - cos_theta, 5.);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.);
  float NdotH2 = NdotH * NdotH;

  float num = a2;
  float denom = (NdotH2 * (a2 - 1.) + 1.);
  denom = M_PI * denom * denom;

  return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
  float r = (roughness + 1.);
  float k = (r * r) / 8.;

  float num   = NdotV;
  float denom = NdotV * (1. - k) + k;

  return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
  float NdotV = max(dot(N, V), 0.);
  float NdotL = max(dot(N, L), 0.);
  float ggx2  = GeometrySchlickGGX(NdotV, roughness);
  float ggx1  = GeometrySchlickGGX(NdotL, roughness);

  return ggx1 * ggx2;
}

void main()
{
  vec3 normal = normalize(glNormal);
  vec3 eye_vec = normalize(-glPos);

  vec3 F0 = mix(vec3(0.04), albedo, metallicity);

  vec3 Lo = vec3(0);
  for(int i=0; i<4; ++i)
  {
    vec3 light_vec = normalize(light_pos[i] - glPos);
    vec3 half_vec = normalize(eye_vec + light_vec);

    float dist = length(light_pos[i] - glPos);
    vec3 radiance = light_color[i].xyz * light_color[i].a / (dist * dist);

    float NDF = DistributionGGX(normal, half_vec, roughness);
    float G = GeometrySmith(normal, eye_vec, light_vec, roughness);
    vec3 F = fresnelSchlick(max(dot(half_vec, eye_vec), 0.), F0, roughness);

    vec3 kS = F;
    vec3 kD = vec3(1.) - kS;
    kD *= 1. - metallicity;

    vec3 numerator = NDF * G * F;
    float denominator = 4. * max(dot(normal, eye_vec), 0.) * max(dot(normal, light_vec), 0.);
    vec3 specular = numerator / max(denominator, 0.01);

    float NdotL = max(dot(normal, light_vec), 0.);
    Lo += (kD * albedo / M_PI + specular) * radiance * NdotL;
  }

  vec3 kS = fresnelSchlick(max(dot(normal, eye_vec), 0.), F0, roughness);
  vec3 kD = 1. - kS;
  kD *= 1. - metallicity;

  vec3 normal_world = normalize(glNormalWorld);

  vec3 irradiance = texture(irradiance_cubetex, normal_world).rgb;
  vec3 diffuse = irradiance * albedo;

  vec3 R = reflect(-camera_world, normal_world);
  vec3 prefiltered = textureLod(specular_cubetex, R, roughness * max_lod).rgb;;

  vec2 brdf = texture(brdf_lut, vec2(max(dot(normal, eye_vec), 0.), roughness)).rg;
  vec3 specular = prefiltered * (kS * brdf.x + brdf.y);

  vec3 ambient = (kD * diffuse + specular) * ao;
  vec3 c = ambient + Lo;

  c = vec3(1.) - exp(-c * exposure);
  c = pow(c, vec3(1. / gamma));

  glFragColor = vec4(c, 1.);
}
