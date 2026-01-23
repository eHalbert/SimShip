#version 330 core

in float v_t;
uniform vec3 color;
uniform float intensity;

out vec4 FragColor;


void main()
{
    float localIntensity = intensity * v_t; // intense near the lighthouse, none at the far end
    if (localIntensity < 0.05)
        discard;
    FragColor = vec4(color, localIntensity);
}
