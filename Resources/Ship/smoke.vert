#version 430

layout(location = 0) in vec3 unused; // Just to draw (1 point)

struct Particle {
    vec3    position;
    vec3    velocity;
    float   life;
    vec4    color;
};

layout(std430, binding = 0) buffer Particles { Particle particles[]; };

uniform mat4    view;
uniform mat4    projection;
uniform float   density;
uniform float   lifeSpan;

out vec4 ParticleColor;

void main() 
{
    Particle p = particles[gl_InstanceID];
    if (p.life <= 0.0) 
    {
        gl_Position = vec4(-1000.0, -1000.0, -1000.0, 1.0); // out of the camera
        ParticleColor = vec4(0.0);
        gl_PointSize = 0.0;
        return;
    }
    
    gl_Position = projection * view * vec4(p.position, 1.0);
    gl_PointSize = 250.0 * p.life / gl_Position.w;

    float alpha = density * p.color.a * (p.life / lifeSpan) * 0.2;  // alpha decreases with time (as p.life)
    ParticleColor = vec4(p.color.rgb, alpha);
}
