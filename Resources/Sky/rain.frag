// Heartfelt - by Martijn Steinrucken aka BigWings - 2017
// Email:countfrolic@gmail.com Twitter:@The_ArtOfCode
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// I revisited the rain effect I did for another shader. This one is better in multiple ways:
// 1. The glass gets foggy.
// 2. Drops cut trails in the fog on the glass.
// 3. The amount of rain is adjustable (with Mouse.y)

// To have full control over the rain, uncomment the HAS_HEART define 

// A video of the effect can be found here:
// https://www.youtube.com/watch?v=uiF5Tlw22PI&feature=youtu.be

// Music - Alone In The Dark - Vadim Kiselev
// https://soundcloud.com/ahmed-gado-1/sad-piano-alone-in-the-dark
// Rain sounds:
// https://soundcloud.com/elirtmusic/sleeping-sound-rain-and-thunder-1-hours

#version 450

in vec2 TexCoord;

uniform sampler2D   texColor; // Color texture
uniform float       uTime;        // Time in second
uniform vec2        screenSize;   // Size of the window in pixels
uniform bool        bBinoculars;
uniform bool        bRainDropsTrails;
uniform bool        bRainBlurDrips;

out vec4 FragColor;

// DROPLETS ========================================

#define S(a, b, t) smoothstep(a, b, t)

vec3 N13(float p) {
    vec3 p3 = fract(vec3(p) * vec3(0.1031, 0.11369, 0.13787));
    p3 += dot(p3, p3.yzx + 19.19);
    return fract(vec3((p3.x + p3.y) * p3.z, (p3.x + p3.z) * p3.y, (p3.y + p3.z) * p3.x));
}
float N(float t) {
    return fract(sin(t * 12345.564) * 7658.76);
}
float Saw(float b, float t) {
    return S(0.0, b, t) * S(1.0, b, t);
}
vec2 DropLayer(vec2 uv, float t) {
    vec2 UV = uv;
    uv.y += t * 0.75;
    vec2 a = vec2(6.0, 1.0);
    vec2 grid = a * 3.5;
    vec2 id = floor(uv * grid);
    
    float colShift = N(id.x); 
    uv.y += colShift;
    
    id = floor(uv * grid);
    vec3 n = N13(id.x * 35.2 + id.y * 2376.1);
    vec2 st = fract(uv * grid) - vec2(.5, 0);
    
    float x = n.x - 0.5;
    
    float y = UV.y * 20.0;
    float wiggle = sin(y + sin(y));
    x += wiggle * (0.5 - abs(x)) * (n.z - 0.5);
    x *= 0.7;
    float ti = fract(t + n.z);
    y = (Saw(0.85, ti) -0.5) * 0.9 + 0.5;
    vec2 p = vec2(x, y);
    
    float d = length((st - p) * a.yx);
    float mainDrop = S(0.4, 0.0, d);
    
    float r = sqrt(S(1.0, y, st.y));
    float cd = abs(st.x - x);
    float trail = S(0.23 * r, 0.15 * r * r, cd);
    float trailFront = S(-0.02, 0.02, st.y - y);
    trail *= trailFront * r * r;
    
    y = UV.y;
    float trail2 = S(0.2 * r, .0, cd);
    float droplets = max(0.0, (sin(y * (1.0 - y) * 120.0) - st.y)) * trail2 * trailFront * n.z;
    y = fract(y * 10.0) + (st.y - .5);
    float dd = length(st - vec2(x, y));
    droplets = S(0.3, 0.0, dd);
    float m = mainDrop + droplets * r * trailFront;
    
    return vec2(m, trail);
}
float StaticDrops(vec2 uv, float t) {
    uv *= 40.0;
    
    vec2 id = floor(uv);
    uv = fract(uv) - 0.5;
    vec3 n = N13(id.x * 107.45 + id.y * 3543.654);
    vec2 p = (n.xy - 0.5) * 0.7;
    float d = length(uv - p);
    
    float fade = Saw(0.025, fract(t + n.z));
    float c = S(0.2, 0.0, d) * fract(n.z * 10.0) * fade;
    return c;
}

vec2 Drops(vec2 uv, float t, float l0, float l1, float l2) {
    float s = StaticDrops(uv, t) * l0; 
    vec2 m1 = DropLayer(uv, t) * l1;
    vec2 m2 = DropLayer(uv * 1.85, t) * l2;
    
    float c = s + m1.x + m2.x;
    c = S(0.3, 1.0, c);
    return vec2(c, max(m1.y * l0, m2.y * l1));
}

// TRAILS ==================================================

float hash(float n) {
    return fract(sin(n) * 687.3123);
}
float noise(in vec2 x) {
    vec2 p = floor(x);
    vec2 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    float n = p.x + p.y * 157.0;
    return mix(mix(hash(n), hash(n + 1.0), f.x), mix(hash(n + 157.0), hash(n + 158.0), f.x), f.y);
}
float rainEffect(vec2 uv, float time) {
    vec2 st = 256.0 * (uv * vec2(3.0, 0.1) + vec2(time * 0.13 - uv.y * 0.01, time * 0.13));
    float f = noise(st) * noise(st * 0.773) * 1.55;
    f = 0.25 + clamp(pow(abs(f), 13.0) * 13.0, 0.0, uv.y * 0.14);
    return f;
}

// BLUR + DRIPS ==============================================

#define size 0.2

vec4 N14(float t) {
	return fract(sin(t * vec4(123.0, 1024.0, 1456.0, 264.0)) * vec4(6547.0, 345.0, 8799.0, 1564.0));
}
vec2 Drops(vec2 uv, float t) {
    vec2 UV = uv;
    
    // DEFINE GRID
    uv.y += t * 0.8;
    vec2 a = vec2(6.0, 1.0);
    vec2 grid = a * 2.0;
    vec2 id = floor(uv * grid);
    
    // RANDOM SHIFT Y
    float colShift = N(id.x); 
    uv.y += colShift;
    
    // DEFINE SPACES
    id = floor(uv * grid);
    vec3 n = N13(id.x * 35.2 + id.y * 2376.1);
    vec2 st = fract(uv*grid) - vec2(0.5, 0);
    
    // POSITION DROPS
    float x = n.x - 0.5;
    
    float y = UV.y * 20.0;
    
    float distort = sin(y + sin(y));
    x += distort * (0.5 - abs(x)) * (n.z - 0.5);
    x *= 0.7;
    float ti = fract(t + n.z);
    y = (Saw(0.85, ti) - 0.5) * 0.9 + 0.5;
    vec2 p = vec2(x, y);
    
    // DROPS
    float d = length((st - p) * a.yx);
    float dSize = size; 
    float Drop = S(dSize, 0.0, d);
    
    float r = sqrt(S(1.0, y, st.y));
    float cd = abs(st.x - x);
    
    // TRAILS
    float trail = S((dSize * 0.5 + 0.03) * r, (dSize * 0.5 - 0.05) * r, cd);
    float trailFront = S(-0.02, 0.02, st.y - y);
    trail *= trailFront;
    
    // DROPLETS
    y = UV.y;
    y += N(id.x);
    float trail2 = S(dSize * r, .0, cd);
    float droplets = max(0.0, (sin(y * (1.0 - y) * 120.0) - st.y)) * trail2 * trailFront * n.z;
    y = fract(y * 10.0) + (st.y - 0.5);
    float dd = length(st - vec2(x, y));
    droplets = S(dSize * N(id.x), 0.0, dd);
    float m = Drop + droplets * r * trailFront;
    
    return vec2(m, trail);
}
float StaticDrops2(vec2 uv, float t) {
	uv *= 30.0;
    
    vec2 id = floor(uv);
    uv = fract(uv) - 0.5;
    vec3 n = N13(id.x * 107.45 + id.y * 3543.654);
    vec2 p = (n.xy - 0.5) * 0.5;
    float d = length(uv-p);
    
    float fade = Saw(0.025, fract(t + n.z));
    float c = S(size, 0.0, d) * fract(n.z * 10.0) * fade;

    return c;
}
vec2 Rain(vec2 uv, float t) {
    float s = StaticDrops2(uv, t); 
    vec2 r1 = Drops(uv, t);
    vec2 r2 = Drops(uv * 1.8, t);
    
    float c = s + r1.x + r2.x;
    c = S(0.3, 1.0, c);
    
    return vec2(c, max(r1.y, r2.y));
}

void main() 
{
    vec4 color = texture(texColor, TexCoord);

    float mask = 1.0;
    if (bBinoculars)
    {
        // Centers of the two circles in UV coordinates
        vec2 centerLeft = vec2(0.35, 0.5);
        vec2 centerRight = vec2(0.65, 0.5);
        float radius = 0.45;

        // Calculating the width/height ratio
        float aspect = screenSize.x / screenSize.y;

        // Adjusting coordinates to compensate for aspect ratio
        vec2 coordLeft = TexCoord - centerLeft;
        coordLeft.x *= aspect;

        vec2 coordRight = TexCoord - centerRight;
        coordRight.x *= aspect;

        float distLeft = length(coordLeft);
        float distRight = length(coordRight);

        float edgeWidth = 0.02; // edge gradient width

        float maskLeft = smoothstep(radius, radius - edgeWidth, distLeft);
        float maskRight = smoothstep(radius, radius - edgeWidth, distRight);


        float mask = max(maskLeft, maskRight);

        if (mask == 0.0)
            discard;
    }

    // Drops + Trails
    if (bRainDropsTrails)
    {
        // Center and moderate zoom
        float zoom = 1.0;
        vec2 uv = (TexCoord - 0.5) * zoom;

        // Slight extension to avoid cuts
        uv = uv * 1.1; 

        float t = uTime * 0.2;  // speed rain
        float rainAmount = 0.8;
        vec2 drops = Drops(uv, t, rainAmount, rainAmount * 0.6, rainAmount * 0.4);

        // Cheap normal calculation for refraction
        vec2 n = vec2(dFdx(drops.x), dFdy(drops.x));
        float blurAmount = mix(3.0, 6.0, rainAmount);
        float focus = mix(blurAmount, 2.0, smoothstep(0.1, 0.2, drops.x));
        color.rgb = textureLod(texColor, TexCoord + n, focus).rgb;

        // Modulate color with drops for wet effect
        color.rgb = mix(color.rgb, vec3(0.8, 0.9, 1.0), drops.x * 0.3);

        // Rain Trails
        float fRain = rainEffect(TexCoord, uTime * 0.5);
        vec3 rainCol = vec3(0.7, 0.8, 0.9); 
        float maskTrail = smoothstep(0.01, 1.0, fRain);
        color.rgb += fRain * rainCol * maskTrail * 1.5;
    }

    //Droplets + Drips
    if (bRainBlurDrips)
    {
        float zoom = 4.0;   // Bigger is the zoom, smaller are the drops
	    vec2 uv = (TexCoord.xy - 0.5) * zoom;
        vec2 UV = TexCoord.xy;
        float T = uTime;
        float t = T * 0.2;
    
        float rainAmount = 0.8;
    
        UV = (UV - 0.5) * 0.9 + 0.5;
    
        vec2 c = Rain(uv, t);

   	    vec2 e = vec2(0.001, 0.0);          // pixel offset
   	    float cx = Rain(uv + e, t).x;
   	    float cy = Rain(uv + e.yx, t).x;
   	    vec2 n = vec2(cx - c.x, cy - c.x);  // normals
    
        // BLUR derived from existical https://www.shadertoy.com/view/Xltfzj
        float Pi = 6.28318530718;           // Pi*2
    
        // GAUSSIAN BLUR SETTINGS
        float Directions = 16.0;            // BLUR DIRECTIONS (Default 16.0 - More is better but slower)
        float Quality = 4.0;                // BLUR QUALITY (Default 4.0 - More is better but slower)
        float Size = 8.0;                   // BLUR SIZE (Radius)

        vec2 Radius = Size / screenSize.xy;

        vec3 col = texture(texColor, UV).rgb;
        
        // Blur calculations
        for( float d = 0.0; d < Pi; d += Pi / Directions)
        {
            for(float i = 1.0 / Quality; i <= 1.0; i += 1.0 / Quality)
            {
                vec3 tex = texture( texColor, UV+n+vec2(cos(d),sin(d))*Radius*i).rgb;
                col += tex;            
            }
        }
        col /= Quality * Directions;

        vec3 tex = texture(texColor, UV + n).rgb;
        c.y = clamp(c.y, 0.0, 1.);
        col -= c.y;
        col += c.y * (tex + 0.6);
        color.rgb = col;
    }

    if (bBinoculars)
        FragColor = vec4(color.rgb * mask, 1.0);
    else
        FragColor = vec4(color.rgb, 1.0);
}
