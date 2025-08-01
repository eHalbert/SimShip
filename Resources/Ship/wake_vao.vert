#version 330 core

layout(location = 0) in vec3 inPos;   // position (x,y,z)
layout(location = 1) in vec2 inUV;    // coordonnées UV éventuelles (facultatif)
layout(location = 2) in float inAlpha; // nouvel attribut alpha

out vec2 fragUV;
out float fragAlpha;  // sortie vers fragment shader

uniform float scaleX;  
uniform float scaleZ;  
uniform float offsetX; 
uniform float offsetZ;  

uniform float originX; 
uniform float originZ; 

void main()
{
    // On soustrait l'origine pour que ce point soit au centre
    float x = (inPos.x - originX) * scaleX + offsetX;
    float y = (inPos.z - originZ) * scaleZ + offsetZ;

    gl_Position = vec4(x, y, 0.0, 1.0);

    fragUV = inUV;
    fragAlpha = inAlpha;  // passe alpha au fragment shader
}


