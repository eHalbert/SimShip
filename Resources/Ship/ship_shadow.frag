#version 430
precision highp float;

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
in vec4 FragPosLightSpace;

uniform sampler2D   texture_diffuse1;
uniform samplerCube envmap;
uniform sampler2D   shadowMap;

uniform vec3        viewPos;
uniform float       exposure;
uniform Material    material;
uniform Light       light;
uniform bool        has_texture;
uniform float		envmapFactor;
uniform bool        bShipShadow;

const float specularIntensity = 0.5;

out vec4 FragColor;


vec3 ACESFilm(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    vec3 mapped = clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
    return mapped; 
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
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
vec3 AdjustContrast(vec3 color, float contrast) {
    return clamp(mix(vec3(0.5), color, contrast), 0.0, 1.0);
}
float PCF_TextureGather(vec3 projCoords, float bias)
{
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float shadow = 0.0;

    // Récupère directement 4 profondeurs autour de la coordonnée donnée
    // textureGather retourne un vec4 avec 4 échantillons voisins (rouge, vert, bleu, alpha)
    // On effectue la comparaison parallèle en utilisant le ref, qui est projCoords.z dans sampler2DShadow
    vec4 depthSamples = textureGather(shadowMap, projCoords.xy);

    for (int i = 0; i < 4; ++i)
    {
        float compare = step(depthSamples[i], projCoords.z - bias); // 0.005 = biais
        shadow += compare;
    }

    shadow /= 4.0;

    return shadow;
}

float ShadowCalculation(vec3 N, vec3 L)
{
    vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    // Dynamic calculation of bias as a function of light-surface angle
    float bias = max(0.005 * (1.0 - dot(N, L)), 0.005);
    return PCF_TextureGather(projCoords, bias);
}

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);          // Direction towards the camera
    vec3 L = normalize(light.position - FragPos);   // Direction towards light (point)
    vec3 H = normalize(V + L);                      // Midway vector

    vec4 materialColor = has_texture ? texture(texture_diffuse1, TexCoords) : material.diffuse;
    
    // F0 (reflectivity at normal incidence)
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, materialColor.rgb, material.specular.r);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, material.roughness);      // Normal Distribution Function
    float G   = GeometrySmith(N, V, L, material.roughness);     // Geometric Shadowing/Masking Function
    vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);        // Fresnel
       
    vec3 nominator    = NDF * G * F;
    float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular     = nominator / max(denominator, 0.001);  
    specular = light.specular * specular * specularIntensity;
   
    // Ambient
    vec3 ambient = light.ambient * materialColor.rgb;

    // Diffuse
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - material.metallic;
    float NdotL = max(dot(N, L), 0.0);        
    vec3 diffuse = light.diffuse * kD * materialColor.rgb * NdotL;
    
    // Emission
    vec3 emission = material.emission.rgb;
    
    // Réflexion environnementale via HDRI
    vec3 R = reflect(-V, N);
    vec3 envColor = texture(envmap, R).rgb;
    float envReflect = envmapFactor * clamp(material.metallic + (1.0 - material.roughness), 0.0, 1.0);
    
    // Shadow
    float shadow = 0.0f;
    if (bShipShadow)
        shadow = ShadowCalculation(N, L);                      

    // Result
    vec3 hdrColor = ambient + (1.0 - shadow) * (diffuse + specular + emission);
    
    // Résultat HDR
    hdrColor += envColor * envReflect; // Ajout de la réflexion HDRI

    // Increases contrast
    hdrColor = pow(hdrColor, vec3(1.3));    

    // Prevents completely flat blacks
    hdrColor = max(hdrColor, vec3(0.001));  

    // Tone mapping (approximation of the ACES tonemapping curve that allows transforming an HDR image into LDR)
    vec3 mapped = ACESFilm(hdrColor * exposure);

    float alpha = has_texture ? materialColor.a : material.diffuse.a;

    FragColor = vec4(mapped, alpha);
}
