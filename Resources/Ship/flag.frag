#version 330 core

uniform sampler2D flagTexture;
uniform float exposure;

in vec2 TexCoord;

out vec4 FragColor;


void main()
{
    FragColor = texture(flagTexture, TexCoord);
    FragColor.rgb *= exposure;
}
