#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct Material 
{
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 emission;
    float shininess;
    float roughness;
    float metallic;
};
struct Light 
{
    vec3 position;
	vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform bool has_texture;
uniform sampler2D texture_diffuse1;
uniform Material material;
uniform Light light;

out vec4 FragColor;


void main()
{
	vec4 color;
	vec4 amb;
	
	vec3 lightDir = normalize(light.position);
	vec3 n = normalize(Normal);	
	float intensity = max(dot(lightDir, n), 0.0);
	
	if (has_texture) 
	{
		color = vec4(light.diffuse, 1.0) * texture(texture_diffuse1, TexCoords);
		amb = color * 0.33;
	}
	else 
	{
		color = vec4(light.diffuse, 1.0) * material.diffuse * 0.8;
		amb = material.ambient * 0.1;
	}
	vec3 result = ((color * intensity) + amb).rgb;

    vec4 materialColor = has_texture ? texture(texture_diffuse1, TexCoords) : material.diffuse;
	float alpha = has_texture ? materialColor.a : material.diffuse.a;

	FragColor = vec4(result, alpha);
}
