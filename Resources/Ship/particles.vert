#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aVelocity;
layout (location = 2) in float aLife;
layout (location = 3) in vec4 aColor;

uniform mat4 projection;
uniform mat4 view;
uniform float density;
uniform float lifeSpan;

out vec4 particleColor;

void main() 
{
    gl_Position = projection * view * vec4(aPos, 1.0);
    float lifeRatio = aLife / lifeSpan; // Normalize aLife between 0 and 1

    float minSize = 5.0;
    float maxSize = 50.0;
    gl_PointSize = min(maxSize, minSize + (maxSize - minSize) * (1.0 - lifeRatio));
   
    float alpha = density * aColor.a * lifeRatio; // Alpha increases as aLife decreases
    particleColor = vec4(aColor.rgb, alpha);
}
