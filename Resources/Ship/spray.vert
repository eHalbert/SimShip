#version 330 core
layout (location = 0) in vec3   aPos;
layout (location = 1) in vec3   aVelocity;
layout (location = 2) in float  aLife;
layout (location = 3) in vec4   aColor;

uniform mat4    projection;
uniform mat4    view;
uniform float   density;
uniform float   lifeSpan;
uniform float   exposure;

out vec4 ParticleColor;


void main() 
{
    gl_Position = projection * view * vec4(aPos, 1.0);
    gl_PointSize = 600.0 / gl_Position.w;

    float alpha = 2.0 * density * aLife / lifeSpan;
    ParticleColor = vec4(aColor.rgb * exposure, alpha);
}
