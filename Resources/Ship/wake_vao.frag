#version 330 core

in float	FragAlpha;

out vec4	FragColor;


void main()
{
	FragColor = vec4(1.0, 0.0, 0.0, FragAlpha);
}
