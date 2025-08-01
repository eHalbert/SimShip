#version 330 core

in vec3 fragmentColor;

out vec4 FragColor;

void main()
{
    FragColor = vec4(fragmentColor, 1.0);
}