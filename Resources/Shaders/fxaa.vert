#version 450

layout(location = 0) in vec2 position; // quad vertices (-1,-1),(1,-1),(1,1),(-1,1)
layout(location = 1) in vec2 inTexCoord; // coordonnées UV correspondantes (0,0)-(1,1)

out vec2 texCoord;

void main() 
{
    gl_Position = vec4(position, 0.0, 1.0);
    texCoord = inTexCoord;
}

