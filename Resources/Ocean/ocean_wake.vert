#version 430

layout (location = 0) in vec3 my_Position;
layout (location = 1) in vec2 my_TexCoords;

layout (binding = 0) uniform sampler2D displacement;

uniform mat4 matLocal;
uniform mat4 matViewProj;
uniform vec3 eyePos;
uniform mat4 lightSpaceMatrix;

out vec3 vdir;
out vec2 tex;
out vec3 vertex;
out vec4 FragPosLightSpace;

void main()
{
	// transform to world space
	vec4 pos_local = matLocal * vec4(my_Position, 1.0);
	vec3 disp = texture(displacement, my_TexCoords).xyz;

	// outputs
	vdir = eyePos - pos_local.xyz;
	tex = my_TexCoords;
	vertex = vec3(pos_local.xyz + disp);

	gl_Position = matViewProj * vec4(pos_local.xyz + disp, 1.0);

	// Position dans l’espace lumière pour shadow mapping
    FragPosLightSpace = lightSpaceMatrix * pos_local;
}
