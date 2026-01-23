#version 330 core

in vec2 fragUV;

uniform vec3 lightColor;
uniform float lightIntensity;
uniform float starIntensity;

out vec4 FragColor;


void main()
{
    vec2 centeredUV = fragUV - vec2(0.5);
    float dist = length(centeredUV);
    
    // Soft Gaussian halo
    float halo = exp(-dist * dist * 40.0);

    // Polar angle
    float angle = atan(centeredUV.y, centeredUV.x);

    // 8-pointed star
    float star = pow(max(0.0, sin(8.0 * angle)), 6.0);

    float intensity = lightIntensity * (halo + starIntensity * star);
    if (intensity < 0.2)
        discard;

    FragColor = vec4(lightColor * intensity, intensity);
}
