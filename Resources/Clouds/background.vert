#version 430

layout (location = 0) in vec2 aPos;          // Position (x, y) du sommet du quad
layout (location = 1) in vec2 aTexCoord;     // Coordonnée UV associée

out vec2 TexCoord;                           // Passage de l’UV au fragment shader

void main()
{
    gl_Position = vec4(aPos.xy, 0.0, 1.0);   // Projette le quad en 2D
    TexCoord = aTexCoord;                    // Passe les coordonnées UV
}
