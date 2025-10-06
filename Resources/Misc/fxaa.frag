#version 450

in vec2 texCoord;

uniform sampler2D   texInput;
uniform vec2        invScreenSize;  // 1.0 / texture resolution (ex : vec2(1.0/width, 1.0/height))

out vec4 fragColor;


vec3 apply_fxaa(sampler2D tex, vec2 uv, vec2 inverseResolution) {
    vec3 rgbNW = texture(tex, uv + vec2(-inverseResolution.x, -inverseResolution.y)).rgb;
    vec3 rgbNE = texture(tex, uv + vec2( inverseResolution.x, -inverseResolution.y)).rgb;
    vec3 rgbSW = texture(tex, uv + vec2(-inverseResolution.x,  inverseResolution.y)).rgb;
    vec3 rgbSE = texture(tex, uv + vec2( inverseResolution.x,  inverseResolution.y)).rgb;
    vec3 rgbM  = texture(tex, uv).rgb;

    vec3 lumaWeights = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, lumaWeights);
    float lumaNE = dot(rgbNE, lumaWeights);
    float lumaSW = dot(rgbSW, lumaWeights);
    float lumaSE = dot(rgbSE, lumaWeights);
    float lumaM  = dot(rgbM,  lumaWeights);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * (1.0/8.0)), 1.0/128.0);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, vec2(-2.0), vec2(2.0)) * inverseResolution;

    vec3 rgbA = 0.5 * ( texture(tex, uv + dir * (1.0/3.0 - 0.5)).rgb + texture(tex, uv + dir * (2.0/3.0 - 0.5)).rgb );
    vec3 rgbB = rgbA * 0.5 + 0.25 * ( texture(tex, uv + dir * (0.0/3.0 - 0.5)).rgb + texture(tex, uv + dir * (3.0/3.0 - 0.5)).rgb );

    float lumaB = dot(rgbB, lumaWeights);
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
        return rgbA;
    else
        return rgbB;
}

void main() 
{
    vec3 color = apply_fxaa(texInput, texCoord, invScreenSize);
    fragColor = vec4(color, 1.0);
}
