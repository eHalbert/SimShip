#version 330 core

in vec2 FragUV;

uniform sampler2D   tex;
uniform float       texelWidth; // 1.0 / 1024.0

out vec4 FragColor;

const float noiseStrength = 0.1;

float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    float kernel[5] = float[](0.204164, 0.304005, 0.093913, 0.010381, 0.000229);
    float sum = 0.0;
    for (int i = -4; i <= 4; ++i)
        sum += texture(tex, FragUV + vec2(texelWidth * i, 0.0)).r * kernel[abs(i)];
    if (sum != 0.0) 
    {
        float noise = (rand(FragUV) - 0.5) * noiseStrength;
        sum += noise;
    }    
    FragColor = vec4(clamp(sum, 0.0, 1.0), 0.0, 0.0, 1.0);
}

