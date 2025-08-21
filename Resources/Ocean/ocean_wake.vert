#version 430

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoords;

layout (binding = 0) uniform sampler2D displacement;
layout (binding = 1) uniform sampler2DArray kelvinArray;
uniform int     texLayer;

uniform mat4    matLocal;
uniform mat4    matViewProj;
uniform vec3    eyePos;

uniform vec3    shipPosition;   // Ship position (world)
uniform float   shipRotation;   // Ship heading (radians, 0 = X)
uniform bool    bWaves;
uniform float   amplitude;      // Height of the waves
uniform float   kelvinScale;
uniform float   centerFore;

out vec3        vdir;
out vec2        tex;
out vec3        vertex;
out float       vFoamIntensity;

const float PI_2 = 1.57079632;
const int kelvin_width_2 = 512;
const int kelvin_height = 1024;

void main()
{
    // Transform to world space
    vec4 posLocal = matLocal * vec4(aPosition, 1.0);
    vec3 disp = texture(displacement, aTexCoords).xyz;
    vec3 posWorld = posLocal.xyz + disp;
    vFoamIntensity = 0.0;   // Out for the fragment sshader

    if (bWaves)
    {
        // Relative position of the vertex to the position of the ship (in world space)
        vec2 relativePos = posWorld.xz - shipPosition.xz;

        // Apply inverse boat rotation to bring coordinates into the ship's local frame
        float cosR = cos(-shipRotation - PI_2);
        float sinR = sin(-shipRotation - PI_2);
        
        // Rotate the relative position vector
        vec2 rotatedPos;
        rotatedPos.x = relativePos.x * cosR - relativePos.y * sinR;
        rotatedPos.y = relativePos.x * sinR + relativePos.y * cosR;

        // Compute texture coordinates for the Kelvin wake texture
        float texX = 0.5 + rotatedPos.x * kelvinScale / kelvin_width_2;
        float texY = (rotatedPos.y * kelvinScale / kelvin_height) + (centerFore / kelvin_height);
        
        // Only apply the wake effect if within the bounds of the Kelvin texture
        if (texX >= 0.0 && texX <= 1.0 && texY >= 0.0 && texY <= 1.0)
        {
            // Sample the grayscale value from the Kelvin wake texture array
            float kelvinGray = texture(kelvinArray, vec3(texX, texY, texLayer)).r;
            
            // Convert grayscale value [0,1] to vertical offset: 0.5 → 0, 0 → -amplitude, 1 → +amplitude
            float kelvinYoffset = (kelvinGray - 0.5) * 2.0 * amplitude;

             // Optional: fade the effect near the edges for a soft transition
            float border = 0.04;
            float mask = smoothstep(0.0, border, texX) * smoothstep(0.0, border, 1.0-texX) * smoothstep(0.0, border, texY) * smoothstep(0.0, border, 1.0-texY);
            kelvinYoffset *= mask;

            // Apply the vertical offset to the vertex position and foam intensity
            posWorld.y += kelvinYoffset;
            vFoamIntensity = kelvinYoffset;
        }
    }

    // Outputs
    vertex = posWorld;
    vdir = eyePos - vertex;
    tex = aTexCoords;
    gl_Position = matViewProj * vec4(vertex, 1.0);
}