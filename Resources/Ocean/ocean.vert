#version 430

layout (location = 0) in vec3   my_Position;
layout (location = 1) in vec2   my_TexCoords;
layout (location = 2) in mat4   instanceMatrix;
layout (location = 6) in int    inLod;
layout (location = 7) in int    inFoam;

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
	vec4 pos_local = instanceMatrix * vec4(my_Position, 1.0);
	vec3 disp = texture(displacement, my_TexCoords).xyz;

    vec3 finalPos = pos_local.xyz + disp;

	// Outputs
    vdir = eyePos - finalPos;
    tex = my_TexCoords;
    vertex = finalPos;
    lod = inLod;
    iFoam = inFoam;

    gl_Position = matViewProj * vec4(finalPos, 1.0);
}

