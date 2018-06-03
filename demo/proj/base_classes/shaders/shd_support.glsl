//--GLOBAL:
#version 330 core


//--VER vs_base_3d


layout(location = 0)in vec3 Position;
layout(location = 1)in vec2 TexCoord;
layout(location = 2)in vec3 Normal;
uniform mat4 MVPMat;
uniform mat4 ModelViewMat;
uniform mat3 NormalViewMat;
out vec3 glPos;
out vec2 glTexCoord;
out vec3 glNormal;

void main()
{
  vec4 pos = vec4(Position, 1.);
  gl_Position = MVPMat * pos;
  glPos = (ModelViewMat * pos).xyz;
  glTexCoord = TexCoord;
  glNormal = normalize(NormalViewMat * Normal);
}


//--PIX ps_base_3d


in vec3 glPos;
in vec2 glTexCoord;
in vec3 glNormal;
layout(location = 0)out vec4 glFragColor;
uniform vec3 light_pos[4];
uniform vec4 light_color[4];

void main()
{
  vec3 lightDir = normalize(glPos - light_pos[0]);
  vec3 viewDir = -normalize(-glPos);
  vec3 reflectDir = reflect(-lightDir, glNormal);
  float diff = max(dot(normalize(glNormal), -lightDir), 0.);
  float spec = pow(max(dot(viewDir, reflectDir), 0.), 64);
  glFragColor = vec4(vec3(0.1 + spec + diff * 0.3), 1.);
}


//--PIX ps_render_dist


in vec4 glColor;
in vec2 glTexCoord;
layout(location = 0)out vec4 glFragColor;
uniform sampler2D src;

float contour(in float d, in float w)
{
return smoothstep(0.5 - w, 0.5 + w, d);
}

float samp(in vec2 uv, float w)
{
return contour(texture2D(src, uv).a, w);
}

void main()
{
float d = texture(src, glTexCoord).r;
float w = fwidth(d);
float a = smoothstep(0.5 - w, 0.5 + w, d);
float ds = 0.354;
vec2 duv = ds * (dFdx(glTexCoord) + dFdy(glTexCoord));
vec4 box = vec4(glTexCoord - duv, glTexCoord + duv);
float ass = samp(box.xy, w) + samp(box.zw, w) + samp(box.xw, w) + samp(box.zy, w);
a = (a + 0.5 * ass) / 3.0;
glFragColor = vec4(a, a, a, 1.);
}


//--PIX ps_proc_spheregen


in vec2 glTexCoord;
layout(location = 0)out vec4 glFragColor;
uniform float r;
const float i_PI = 1. / 3.14159265358979323846;

void main()
{
  vec2 pos = (glTexCoord.xy - vec2(0.5)) * 2;
  float sin_theta = sqrt(1. - pos.y * pos.y);

  float v = acos(pos.y) * i_PI;
  float z = max(sqrt(1. - pos.y * pos.y - pos.x * pos.x), 0.);
  float u = atan(z, pos.x) * 0.5 * i_PI;

  glFragColor = vec4(normalize(vec3(1.-u, 1.-v, z)), 1.);
}
