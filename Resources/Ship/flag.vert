#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;

out vec2 TexCoord;

// Simple hash pseudo-bruit 1D
float rand(float x)
{
    return fract(sin(x) * 43758.5453);
}

void main()
{
    float noise = rand(aPos.x * 10.0 + time * 5.0) * 0.03;
    float wave = sin(10.0 * aPos.x + time * 5.0) * 0.05;
    vec3 displacedPos = aPos + vec3(0.0, wave + noise, 0.0);
    gl_Position = projection * view * model * vec4(displacedPos, 1.0);
    TexCoord = aTexCoord;
}
