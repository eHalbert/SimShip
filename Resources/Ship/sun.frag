#version 330 core

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

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3        viewPos;
uniform float       exposure;
uniform Material    material;
uniform Light       light;
uniform bool	    bAbsorbance;
uniform vec3	    absorbanceColor;
uniform float	    absorbanceCoeff;
uniform float       specularIntensity = 0.3;

uniform bool        has_texture;
uniform sampler2D   texture_diffuse1;

out vec4 FragColor;


vec3 ACESFilm(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    vec3 mapped = clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
    return mapped * 1.0;
}
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265359 * denom * denom;

    return nom / denom;
}
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    vec3 L = normalize(light.position - FragPos);
    vec3 H = normalize(L + V);
    
    vec4 materialColor = has_texture ? texture(texture_diffuse1, TexCoords) : material.diffuse;
    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, materialColor.rgb, material.specular.r);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, material.roughness);   
    float G   = GeometrySmith(N, V, L, material.roughness);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
       
    vec3 nominator    = NDF * G * F;
    float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular     = nominator / max(denominator, 0.001);  
        
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - material.metallic;
    
    float NdotL = max(dot(N, L), 0.0);        

    vec3 ambient = light.ambient * materialColor.rgb * 0.5;   
    vec3 diffuse = light.diffuse * kD * materialColor.rgb * NdotL * 0.7;
    specular = light.specular * specular * specularIntensity;
    
    vec3 emission = material.emission.rgb;
    
    vec3 hdrColor = ambient + diffuse + specular + emission;
    hdrColor = pow(hdrColor, vec3(2.0));    // Increases contrast
    hdrColor = max(hdrColor, vec3(0.001));  // Prevents completely flat blacks

    // Tone mapping
    vec3 mapped = ACESFilm(hdrColor * exposure);
    
    float alpha = has_texture ? materialColor.a : material.diffuse.a;
   
  	if (bAbsorbance)
	{
		// Calculation of logarithmic fog
		float distanceToCamera = length(viewPos - FragPos);
		float absorbanceFactor = exp(-absorbanceCoeff * distanceToCamera);
		absorbanceFactor = clamp(absorbanceFactor, 0.0, 1.0);
		mapped = mix(absorbanceColor * exposure, mapped, absorbanceFactor);
	}

    FragColor = vec4(mapped, alpha);
}
