#version 330 core
precision highp float;

in vec2 TexCoords;

uniform sampler2D texPrevAccum;     // Previous accumulation texture (R8)
uniform sampler2D texWake;          // Ship silhouette texture (alpha)

uniform vec2    texWakeSize;        // UV size of the projected silhouette
uniform float   rotationShip;       // Ship rotation in radians
uniform vec2    shipPos;
uniform vec2    posDelta;           // Offset UV scroll accumulation
uniform float   rotDelta;
uniform vec2    worldMin;           // coin bas-gauche du plan monde couvert par la texture accumulée
uniform vec2    worldMax;           // coin haut-droit

out float FragAlpha;

// The ship is always placed in the middle, i.e. vec2(0.5)

vec2 texUVToWorld(vec2 uv) {
    // Convert UV coordinate in [0,1] to world coordinate
    return mix(worldMin, worldMax, uv);
}
vec2 worldToTexUV(vec2 worldPos) {
    // Convert world coordinate to UV in [0,1]
    return (worldPos - worldMin) / (worldMax - worldMin);
}

void main()
{
    // 1. Ship display in central and oriented position
    // ================================================

    // Remove the forced translation of uv = 0.5
    vec2 relativeCoord = TexCoords - vec2(0.5);

    // 2D rotation around the origin (0,0)
    float cosR = cos(-rotationShip);
    float sinR = sin(-rotationShip);

    vec2 rotatedCoord = vec2(
        relativeCoord.x * cosR - relativeCoord.y * sinR,
        relativeCoord.x * sinR + relativeCoord.y * cosR
    );
    
    // Then add the original translation uv = 0.5
    vec2 scaled = rotatedCoord / texWakeSize + vec2(0.5);

    // Sampling only if in valid interval
    float wakeAlpha = 0.0;
    if (scaled.x >= 0.0 && scaled.x <= 1.0 && scaled.y >= 0.0 && scaled.y <= 1.0)
        wakeAlpha = texture(texWake, scaled).r;

    // 2. Display previous pixels
    // ==========================

    // Current UV coordinates
    vec2 uv = TexCoords;

    // Convert to world position
    vec2 worldPos = texUVToWorld(uv);

    // Center the rotation around the ship
    worldPos -= shipPos;

    // Apply reverse rotation
    float cosRd = cos(-rotDelta);
    float sinRd = sin(-rotDelta);
    vec2 rotated = vec2(
        worldPos.x * cosRd - worldPos.y * sinRd,
        worldPos.x * sinRd + worldPos.y * cosRd
    );

    // Putting back in place in the world
    rotated += shipPos;

    // Apply reverse translation (go back in time)
    rotated += posDelta;

    // Convert back to UV to sample the previous texture
    vec2 shiftedUV = worldToTexUV(rotated);

    // Sample the previous texture
    float oldAlpha = texture(texPrevAccum, shiftedUV).r;

    // 3. Combined display with oldAlpha masked by the ship
    // ====================================================

   FragAlpha = clamp(oldAlpha * 0.995 + wakeAlpha, 0.0, 1.0);
}
