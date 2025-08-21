#version 330 core

layout(location = 0) in vec2 aPos;      // vertex position (-1..+1) clip space
layout(location = 1) in vec2 aTexCoord; // texture coordinate (0..1)

out vec2 TexCoords; 


void main()
{
    TexCoords = aTexCoord;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
