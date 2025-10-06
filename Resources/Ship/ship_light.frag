#version 430

in vec3 FragPos;

uniform vec3 lightColor;
uniform vec3 viewPos;

out vec4 FragColor;

const float maxDistance = 10000.0;  // maximum range in meters
const float minI = 1.0;             // minimum intensity at maximum range
const float maxI = 100.0;           // maximum intensity

void main()
{
    float dist = length(viewPos - FragPos);
    
    // Intensity
    float intensity = maxI - ( (maxI - minI) * (dist/maxDistance) );
    intensity = clamp(intensity, minI, maxI);
    // Attenuation
    float attenuation = (dist < maxDistance) ? 1.0 / (1.0 + 0.000001 * dist + 0.000001 * dist * dist) : 0.0;    
    vec3 finalColor = lightColor * intensity * attenuation;
    
    FragColor = vec4(finalColor, 1.0);
}

