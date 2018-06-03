//--GLOBAL:
#version 330 core


//--PIX nn__relu_activation


layout(location = 0)out vec4 glFragColor;
uniform sampler2D weights, inputs;

float leaky_relu(in float sum)
{
float bigger = float(sum >= 0);
return bigger * sum + (1. - bigger) * sum * 0.01;
}

void main()
{
int height = textureSize(weights, 0).y;
int x = int(gl_FragCoord.x);

float sum = texelFetch(weights, ivec2(x, 0), 0).x;

for(int i=1; i<height; ++i)
{
float w = texelFetch(weights, ivec2(x, i), 0).x;
float v = texelFetch(inputs, ivec2(i, 1), 0).x;
sum += w * v;
}

float activation = leaky_relu(sum);

glFragColor = vec4(vec3(activation), 1.);
}


//--PIX nn__relu_error_gradient


layout(location = 0)out vec4 glFragColor;
uniform sampler2D weights, errors, outputs;

float leaky_relu_derivative(in float v)
{
float bigger = float(v >= 0);
return bigger + (1. - bigger) * v;
}

void main()
{
int width = textureSize(outputs, 0).x;
int x = int(gl_FragCoord.x);

float error = 0;

for(int i=1; i<width; ++i)
{
float w = texelFetch(weights, ivec2(i, x), 0).x;
float o = texelFetch(outputs, ivec2(i, 1), 0).x;
float e = texelFetch(errors, ivec2(i, 1), 0).x;
error += w * leaky_relu_derivative(o) * e;
}

glFragColor = vec4(vec3(error), 1.);
}


//--PIX nn__relu_backpropagate


layout(location = 0)out vec4 glFragColor;
uniform sampler2D weights, errors, outputs, inputs;

float leaky_relu_derivative(in float v)
{
float bigger = float(v >= 0);
return bigger + (1. - bigger) * v;
}

void main()
{
int x = int(gl_FragCoord.x);
int y = int(gl_FragCoord.y);

const float learning_rate = 0.0001;

float w = texelFetch(weights, ivec2(x, y), 0).x;
float o = texelFetch(outputs, ivec2(x, 1), 0).x;
float e = texelFetch(errors,  ivec2(x, 1), 0).x;
float i = texelFetch(intputs, ivec2(y, 1), 0).x;

float weight = w + learning_rate * e * leaky_relu_derivative(o) * i;

glFragColor = vec4(vec3(error), 1.);
}
