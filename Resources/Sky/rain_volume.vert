#version 430
precision highp float;

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoords;

uniform mat4 matViewProj;
uniform mat4 matInvViewProj;     // AJOUTÉ
uniform vec3 cameraPos;
uniform float uTime;
uniform vec3 windDir;
uniform vec2 screenSize;

out vec2 uv;
out vec3 vRayDir;
out vec3 vCameraPos;
out vec3 vWind;
out float vTime;
out vec2 vInvScreenSize;

void main()
{
    gl_Position = vec4(aPosition.xy, 0.0, 1.0);  // Fullscreen quad
    uv = aTexCoords;
    vCameraPos = cameraPos;
    vWind = windDir;
    vTime = uTime;
    vInvScreenSize = 1.0 / screenSize;
}
