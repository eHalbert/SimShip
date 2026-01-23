#version 430
precision highp float;

in vec2 uv;
in vec3 vRayDir;
in vec3 vCameraPos;
in vec3 vWind;
in float vTime;
in vec2 vInvScreenSize;

uniform sampler2D sceneColor;    // Couleur scène (océan/bateau)
uniform sampler2D sceneDepth;    // Profondeur scène
//uniform sampler2D sceneDepthStencil; // Depth+stencil si besoin
uniform mat4 matInvViewProj;     // Pour unproject
uniform float rainDensity = 0.3;
uniform float rainSpeed = 20.0;
uniform float maxRainDist = 100.0;

out vec4 fragColor;

// Hash noise 3D simple (gouttes)
float hash(vec3 p) 
{
    p = fract(p * vec3(443.1, 397.3, 0.577));
    return fract(dot(p, vec3(0.577, 0.577, 0.577)) * 43758.5453);
}

// Noise animé pour gouttes
float noise(vec3 p) 
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f*f*(3.0-2.0*f);
    return mix(mix(mix(hash(i+vec3(0,0,0)), hash(i+vec3(1,0,0)), f.x), mix(hash(i+vec3(0,1,0)), hash(i+vec3(1,1,0)), f.x), f.y),
               mix(mix(hash(i+vec3(0,0,1)), hash(i+vec3(1,0,1)), f.x), mix(hash(i+vec3(0,1,1)), hash(i+vec3(1,1,1)), f.x), f.y), f.z);
}

// Unproject screen → world
vec3 unproject(vec2 screenUV, float depth) 
{
    vec4 clip = vec4(screenUV * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view = matInvViewProj * clip;
    return view.xyz / view.w;
}

float raymarchRain(vec3 rayOrigin, vec3 rayDir) 
{
    float accum = 0.0;
    float stepSize = 0.5;  // Taille pas (ajustez pour perf/qualité)
    
    for(int i = 0; i < 64; i++) 
    {
        vec3 pos = rayOrigin + rayDir * float(i) * stepSize;
        
        // Animation gouttes (gravité + vent bateau)
        vec3 dropPos = pos + vWind * vTime * 0.1;
        dropPos.y -= rainSpeed * vTime;
        
        // Densité gouttes (noise 3D)
        float drop = noise(dropPos * 40.0 + vTime * 2.0);
        drop = smoothstep(0.6, 0.8, drop);
        
        // Fade avec distance + hauteur (moins de pluie près sol)
        float distFade = exp(-float(i) * stepSize * 0.01);
        float heightFade = smoothstep(-10.0, 20.0, pos.y);
        
        accum += drop * rainDensity * distFade * heightFade * stepSize;
        
        // Early exit si trop loin
        if (accum > 0.9 || length(pos - rayOrigin) > maxRainDist) break;
    }
    
    return clamp(accum, 0.0, 1.0);
}

void main() 
{
    // Récupérer profondeur scène (océan/bateau)
    float sceneDepth = texture(sceneDepth, uv).r;
    vec3 scenePos = unproject(uv, sceneDepth);
    vec3 rayOrigin = vCameraPos;
    vec3 rayDir = normalize(scenePos - vCameraPos);
    
    // Limiter rayon à profondeur scène
    float maxDist = length(scenePos - vCameraPos);
    
    // Raymarch pluie
    float rainAmount = raymarchRain(rayOrigin, rayDir);
    
    // Couleur pluie (blanc bleuté + scattering)
    vec3 rainColor = mix(vec3(0.9, 0.95, 1.0), vec3(0.7, 0.8, 0.9), rainAmount * 0.5);
    
    // Mélange additif avec scène
    vec3 sceneCol = texture(sceneColor, uv).rgb;
    vec3 finalColor = sceneCol + rainColor * rainAmount * 0.8;
    
    // Contraste + saturation pluie
    finalColor = mix(finalColor, vec3(0.95), rainAmount * 0.1);
    
    fragColor = vec4(finalColor, 1.0);
}
