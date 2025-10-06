#version 330 core

in vec2 FragPos;
in vec2 TexCoords;

uniform vec3 color;
uniform sampler2D texture0;

out vec4 FragColor;


void main()
{
    FragColor = vec4(color, 1.0) + texture(texture0, TexCoords);
}
