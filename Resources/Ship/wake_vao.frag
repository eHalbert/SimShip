// Fragment shader
#version 330 core

in vec2 fragUV;
in float fragAlpha;

out vec4 outColor;

void main()
{
	outColor = vec4(1.0, 0.0, 0.0, fragAlpha);
}
