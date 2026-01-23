#version 330 core
layout(location = 0) in vec2 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 fragUV;

void main()
{
    fragUV = position * 0.5 + 0.5; // passage de [-1,1] à [0,1] pour UV
    vec4 worldPos = model * vec4(position, 0.0, 1.0);
    gl_Position = projection * view * worldPos;
}
