#version 330 core

layout(location = 0) in vec2 aPos;      // position du sommet (-1..+1) clip space
layout(location = 1) in vec2 aTexCoord; // coordonnée de texture (0..1)

out vec2 TexCoords; // vers fragment shader

void main()
{
    TexCoords = aTexCoord;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
