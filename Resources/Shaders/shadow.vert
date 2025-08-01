#version 330 core

layout(location = 0) in vec3 aPos;  // position du sommet en espace objet

uniform mat4 model;
uniform mat4 lightSpaceMatrix;
uniform mat4 view;
uniform mat4 projection;

out vec4 FragPosLightSpace;

void main()
{
    // Position en espace monde
    vec4 worldPos = model * vec4(aPos, 1.0);

    // Position dans l'espace lumi�re (projection orthographique ou perspective de la lumi�re)
    FragPosLightSpace = lightSpaceMatrix * worldPos;

    // Position finale en espace �cran (camera)
    gl_Position = projection * view * worldPos;
}
