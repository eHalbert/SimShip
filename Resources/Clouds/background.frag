#version 330 core

in vec2 TexCoord;

uniform sampler2D uTexture;             

out vec4 FragColor;


void main()
{
    FragColor = texture(uTexture, TexCoord);
    gl_FragDepth = 0.9999999;   // Don't add a '9'
}
