#version 430

in vec4         ParticleColor;
uniform float   exposure;

out vec4        FragColor;

void main() 
{
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    float dist = length(circCoord);

    // Smoothing and softening the edge
    float alpha = smoothstep(1.0, 0.0, dist);
    alpha = pow(alpha, 2.0);

    FragColor = vec4(ParticleColor.rgb * exposure, ParticleColor.a * alpha);

    if (FragColor.a < 0.01)
        discard; // optimization and better transparency
}
