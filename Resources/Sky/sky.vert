#version 430

layout (location = 0) in vec3 aPos;

uniform mat4 screenToCamera;
uniform mat4 cameraToWorld;
uniform vec3 worldCamera;
uniform vec3 worldSunDir;

out vec3 ViewDir;


void main() 
{
    ViewDir = (cameraToWorld * vec4((screenToCamera * vec4(aPos, 1.0)).xyz, 0.0)).xyz;
    gl_Position = vec4(aPos.xy, 0.9999999, 1.0);
}