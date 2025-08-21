#version 330 core

layout(location = 0) in vec3    inPos;
layout(location = 1) in float   inAlpha;

uniform float scaleX;  
uniform float scaleZ;  
uniform float offsetX; 
uniform float offsetZ;  

uniform float originX; 
uniform float originZ; 

out float   FragAlpha;


void main()
{
    // We subtract the origin so that this point is in the center
    float x = (inPos.x - originX) * scaleX + offsetX;
    float y = (inPos.z - originZ) * scaleZ + offsetZ;

    gl_Position = vec4(x, y, 0.0, 1.0);

    FragAlpha = inAlpha;  // alpha pass to fragment shader
}


