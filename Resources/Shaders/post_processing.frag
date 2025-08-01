#version 450

uniform sampler2D texColor; // Color texture
uniform sampler2D texDepth; // Depth texture

uniform float	exposure;
uniform float   near;
uniform float   far;
uniform float   horizonHeight; // Height of the horizon in NDC screen coordinate [-1, +1] [bottom, top}
uniform vec3	eyePos;

// Colors
uniform vec3	oceanColor;
uniform vec3	fogColor;

// Mist
uniform float	mistDensity;

// Fog
uniform float	fogDensity;

// Particles
uniform float   uTime;        // Time in second
uniform vec2    screenSize;   // Size of the window in pixels

// Binoculars
uniform bool    bBinoculars;

in vec2 tex;

out vec4 my_FragColor;

const float f =  2.0 * sqrt(2.0);

// Simple noise function for particles
float hash(vec2 p) 
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main() 
{
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
        vec2 coordLeft = tex - centerLeft;
        coordLeft.x *= aspect;

        vec2 coordRight = tex - centerRight;
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

    vec4 color = texture(texColor, tex);

    float depth = texture(texDepth, tex).r;
    float linearDepth = (2.0 * near * far) / (far + near - depth * (far - near));
    linearDepth = clamp(linearDepth, near, far);

    // Underwater effect always active if the camera is underwater
    if (eyePos.y < 0.0) 
    {
        // Underwater fog: depends only on camera-pixel distance
        float fogFactor = 1.0 - exp(-0.01 * linearDepth); // The further you look, the denser it is.
        fogFactor = clamp(fogFactor, 0.0, 1.0);

        // Intensity depending on camera depth
        float depthBlend = clamp(-eyePos.y / 20.0, 0.0, 1.0);

        vec3 underwaterFogColor = oceanColor * 0.4;
        vec3 fogCol = mix(oceanColor, underwaterFogColor, depthBlend) * exposure;

        // Applies underwater fog regardless of viewing angle
        color = mix(vec4(fogCol, 1.0), color, 1.0 - fogFactor);

        // Underwater suspended particle effect, animated towards the camera
        vec2 uv = tex * screenSize;
        vec2 screenCenter = 0.5 * screenSize;

        float particleDensity = 0.8;    // Particle density
        float particleSize = 2.2;       // Size in pixels
        float speed = 6.0;              // Radial velocity
        float lifetime = 2.6;           // Lifespan of a particle (in seconds)

        float particleMask = 0.0;
        int numLayers = 2;              // Multiple layers for added depth

        for (int layer = 0; layer < numLayers; ++layer) 
        {
            // For each particle "slot"
            for (int i = 0; i < 20; ++i) 
            {
                // Generates a unique seed for each particle
                float seed = float(i) + float(layer) * 100.0;
                float angle = hash(vec2(seed, 0.0)) * 6.2831853; // [0, 2*PI]
                float radius = hash(vec2(seed, 1.0)) * 0.45 * min(screenSize.x, screenSize.y);

                // Starting position at center + radius
                vec2 dir = vec2(cos(angle), sin(angle));
                vec2 start = screenCenter + dir * radius * 0.3;

                // Animated radial shift (advance from center to outward)
                float t = mod(uTime + hash(vec2(seed, 2.0)) * 100.0, lifetime) / lifetime;
                float travel = t * (0.7 * min(screenSize.x, screenSize.y) - radius);

                vec2 pos = start + dir * travel;

                // Displays the particle if it exists (density)
                if (hash(vec2(seed, 3.0)) < particleDensity) 
                {
                    float dist = length(uv - pos);
                    float alpha = smoothstep(particleSize, 0.0, dist);
                    // Attenuation according to the particle's progression (soft appearance/disappearance)
                    alpha *= smoothstep(0.05, 0.15, t) * (1.0 - smoothstep(0.7, 1.0, t));
                    particleMask += alpha;
                }
            }
        }

        // Particle color (slightly opaque)
        vec3 particleColor = mix(oceanColor, vec3(1.0), 0.55);
        float particleAlpha = clamp(particleMask, 0.0, 0.17);   // Adjusts the overall intensity
        color.rgb = mix(color.rgb, particleColor, particleAlpha);
    }
    else if (mistDensity > 0.0)
    {
        // Adds a strip centered on horizonHeight, in screen space

        // Normalized screen vertical coordinate [-1, 1]
        float screenY = tex.y * 2.0 - 1.0;

        // Settings for band width
        float mistThicknessAbove = 0.1;  
        float mistThicknessBelow = 0.5;

        // Calculation of vertical mist factor — decreasing above and below
        float above = 1.0 - smoothstep(horizonHeight, horizonHeight + mistThicknessAbove, screenY);
        float below = 1.0 - smoothstep(horizonHeight - mistThicknessBelow, horizonHeight, screenY);
        float heightMistFactor = max(above, below);
        heightMistFactor = clamp(heightMistFactor, 0.0, 1.0);

        // Mist factor by linearized depth (distance)
        float mistFactorDepth = 1.0 - exp(-mistDensity * linearDepth);
        mistFactorDepth = clamp(mistFactorDepth, 0.0, 1.0);

        // Combination
        float mistFactor = mistFactorDepth * heightMistFactor;

        // Color mist (base on fog color)
        vec3 mistCol = fogColor * exposure;

        // Final Mix
        color = mix(vec4(mistCol, 1.0), color, 1.0 - mistFactor);

    }
    else if (fogDensity > 0.0)
    {
        float fogFactor = exp(-fogDensity * linearDepth);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        color = mix(vec4(fogColor * exposure, 1.0), color, fogFactor);
    }

    // Apply the gentle mask of the binoculars to the fragment
    if (bBinoculars)
        my_FragColor = vec4(color.rgb * mask, color.a * mask);
    else
        my_FragColor = color;
}