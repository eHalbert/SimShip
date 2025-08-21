#version 430

layout (location = 0) in vec3   aPosition;
layout (location = 1) in vec2   aTexCoords;
layout (location = 2) in mat4   instanceMatrix;
layout (location = 6) in int    instanceLod;
layout (location = 7) in int    instanceFoam;

layout (binding = 0) uniform sampler2D displacement;

uniform mat4 matViewProj;
uniform vec3 eyePos;

out vec3        vdir;
out vec2        tex;
out vec3        vertex;
flat out int    lod;
flat out int    iFoam;


void main()
{
	// Transform to world space
	vec4 posLocal = instanceMatrix * vec4(aPosition, 1.0);
	vec3 disp = texture(displacement, aTexCoords).xyz;
    vec3 finalPos = posLocal.xyz + disp;

    // Outputs
    vdir = eyePos - finalPos;
    tex = aTexCoords;
    vertex = finalPos;
    lod = instanceLod;
    iFoam = instanceFoam;

    gl_Position = matViewProj * vec4(finalPos, 1.0);
}

