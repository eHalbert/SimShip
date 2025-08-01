#version 330 core

in vec2 fragUV;

uniform sampler2D tex;
uniform float texelHeight; // 1.0 / 1024.0

out vec4 outColor;

const float noiseStrength = 0.1;

float rand(vec2 co) {
    return fract(cos(dot(co, vec2(4.898, 7.23))) * 23421.631);
}

void main() 
{
    float kernel[5] = float[](0.204164, 0.304005, 0.093913, 0.010381, 0.000229);
    float sum = 0.0;
    for (int i = -4; i <= 4; ++i)
        sum += texture(tex, fragUV + vec2(0.0, texelHeight * i)).r * kernel[abs(i)];
    if (sum != 0.0) 
    {
        float noise = (rand(fragUV) - 0.5) * noiseStrength;
        sum += noise;
    }    
    outColor = vec4(clamp(sum, 0.0, 1.0), 0.0, 0.0, 1.0);
}
