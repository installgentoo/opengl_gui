//--GLOBAL:
#version 330 core


//--PIX ps_proc_spheregen

in vec2 glTexCoord;
layout(location = 0)out vec4 glFragColor;
const float i_PI = 1. / 3.14159265358979323846;

void main()
{
  vec2 pos = (glTexCoord.xy - vec2(0.5)) * 2.;

  float z = 1. - pos.x * pos.x - pos.y * pos.y;

  if(z < 0.)
  {
    glFragColor = vec4(0.);
    return;
  }

  z = sqrt(z);

  float u = 1. - atan(z, pos.y) * i_PI;
  float v = 1. - acos(pos.x) * i_PI;

  glFragColor = vec4(v, u, z, 1.);
}


//--PIX ps_reverse_fisheye

in vec2 glTexCoord;
layout(location = 0)out vec4 glFragColor;
const float PI = 3.14159265358979323846;
uniform sampler2D tex;

void main()
{
  vec2 pos = (glTexCoord.xy - vec2(0.5)) * 2.;
  float the = pos.x * PI / 2.;
  float phi = pos.y * PI / 2.;

  float x = cos(phi) * sin(the);
  float y = cos(phi) * cos(the);
  float z = sin(phi);

  the = atan(z, x);
  phi = atan(sqrt(x * x + z * z), y);

  float r = phi / PI * 2.;
  vec2 fish = vec2(r * cos(the), r * sin(the));

  fish = (fish + vec2(1.)) / 2.;

  glFragColor = texture(tex, fish);
}


//--PIX ps_reverse_spheregen

in vec2 glTexCoord;
layout(location = 0)out vec4 glFragColor;
const float PI = 3.14159265358979323846;
uniform sampler2D tex;

void main()
{
  vec2 pos = (glTexCoord.xy - vec2(0.5)) * 2.;
  float z = 1. - pos.x * pos.x - pos.y * pos.y;

  float x = cos((1. - glTexCoord.x) * PI);
  float t = tan((1. - glTexCoord.y) * PI * 0.5);
  float y = 0.5 * (z - z * t * t) / t;

  vec2 uv = (vec2(x, y) * 0.5 + vec2(0.5)) * float(glTexCoord.y > 0.15 && glTexCoord.y < 0.85);

  glFragColor = texture(tex, uv);
}


//--VER vs_render_earth

layout(location = 0)in vec3 Position;
layout(location = 1)in vec2 TexCoord;
uniform mat4 ProjViewModelMat;
out vec2 glTexCoord;

void main()
{
  gl_Position = ProjViewModelMat * vec4(Position, 1.);
  glTexCoord = TexCoord;
}


//--PIX ps_render_earth

in vec2 glTexCoord;
layout(location = 0)out vec4 glFragColor;
uniform sampler2D uvz_tex, tex;
uniform mat3 NormalMat;
uniform vec2 shift;

void main()
{
  vec2 uv = texture(uvz_tex, glTexCoord).rg;
  vec4 color = texture(tex, uv + shift);
  vec3 r = mix(vec3(1., 0.8, 0.1), color.rgb, color.a);

  float d = 0.5 - distance(glTexCoord, vec2(0.5));
  vec4 o = vec4(r, min(1., d * 50.));
  glFragColor = o;
}


//--PIX ps_glint

in vec2 glTexCoord;
layout(location = 0)out vec4 glFragColor;
uniform sampler2D tex;
uniform float offset;

void main()
{
  vec4 color = texture(tex, glTexCoord);
  float g = max((1. - abs(glTexCoord.x - glTexCoord.y * 0.5 - offset) * 5.), 0.) * 0.5 + 1.;

  vec4 o = color * g;
  glFragColor = o;
}


//--PIX ramka

in vec2 glTexCoord;
layout(location = 0)out vec4 glFragColor;
uniform sampler2D tex;
uniform float offset;

void main()
{
float d = texture(tex, glTexCoord).r;
float w = fwidth(d);
float t = 0.08;
float a = smoothstep(0.5 - t * 2. - w, 0.5 - t + w, d);
glFragColor = vec4(1., 0.8, 0.2, a * offset);
}
