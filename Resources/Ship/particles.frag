#version 330 core

out vec4 My_FragColor;

in vec4 particleColor;

void main()
{
    // Calculating the distance from the center of the point
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    float dist = length(circCoord);

    // Using a more gradual smoothing function
    float alpha = smoothstep(1.0, 0.0, dist);

    // Applying a softer fade effect
    alpha = pow(alpha, 2.0);

    // Mixing with the particle color
    My_FragColor = vec4(particleColor.rgb, particleColor.a * alpha);
}
