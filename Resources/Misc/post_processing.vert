#version 450 core

precision highp float;

layout(location = 0) in vec3 my_Position; 
layout(location = 1) in vec2 my_TexCoords;

out vec2 tex;


void main()
{
    gl_Position = vec4(my_Position, 1.0);
    tex = my_TexCoords;
}