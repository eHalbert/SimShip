#version 330 core

in vec3 FragPos;

uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float intensity;

out vec4 FragColor;

void main()
{
    float dist = length(viewPos - FragPos);
    
    // Base color
    vec3 baseColor = lightColor * intensity;
    
    // Attenuation
    float attenuation = 1.0 / (1.0 + 0.000001 * dist + 0.00000001 * dist * dist);
    vec3 finalColor = baseColor * attenuation;
    
    FragColor = vec4(finalColor, 1.0);
}

