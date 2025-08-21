#version 430

#define ONE_OVER_4PI	0.0795774715459476

layout(binding = 1) uniform samplerCube	envmap;
layout(binding = 2) uniform sampler2D	gradients;
layout(binding = 3) uniform sampler2D	foamBuffer;
layout(binding = 4) uniform sampler2D	foamDesign;
layout(binding = 5) uniform sampler2D	foamBubbles;
layout(binding = 6) uniform sampler2D	foamTexture;
layout(binding = 7) uniform sampler2D	contourShip;
layout(binding = 8) uniform sampler2D	reflectionTexture;
layout(binding = 9) uniform sampler2D	waterDUDV;
layout(binding = 10) uniform sampler2D	wakeBuffer;

in vec3		vdir;
in vec2		tex;
in vec3		vertex;
in float	vFoamIntensity;

uniform vec3	oceanColor;
uniform float	transparency;
uniform vec3	sunColor;
uniform vec3	sunDir;
uniform int		bEnvmap;
uniform float	exposure;
uniform bool	bAbsorbance;
uniform vec3	absorbanceColor;
uniform float	absorbanceCoeff;
uniform vec3	eyePos;
uniform vec2	shipPivot;
uniform float	shipRotation;
uniform vec2	shipSize;
uniform vec2	wakeSize;
uniform bool	bShowPatch;

uniform float	time;
uniform bool	bWake;

out vec4 FragColor;


vec2 RotatePoint(vec2 point, vec2 pivot, float angle) 
{
    float s = sin(angle);
    float c = cos(angle);
    vec2 translated = point - pivot;
    vec2 rotated = vec2( translated.x * c - translated.y * s, translated.x * s + translated.y * c );
    return rotated + pivot;
}
vec4 ComputeFoam(vec2 uv, float factor)
{
	vec2 dx = dFdx(uv);
	vec2 dy = dFdy(uv);

	// Extract only the white component of the foam texture
	vec4 foamTexture = textureGrad(foamTexture, tex * 20.0, dx, dy);
	float foamWhiteness = max(foamTexture.r, max(foamTexture.g, foamTexture.b));
			
	// Extract the drawing from the foam
	vec4 foamBubblesTexture = textureGrad(foamBubbles, tex * 20.0, dx, dy);
	vec4 foamColor = vec4(1.0) * (1.0 + factor * foamBubblesTexture);
			
	// Mix the original color with the white foam
	return mix(FragColor, foamColor, foamWhiteness * factor);
}

void main()
{
	// Vectors
	vec4 grad = texture(gradients, tex);	// xyz = position, w = jacobian
	vec3 N = normalize(grad.xzy);
	vec3 V = normalize(vdir);
	vec3 R = reflect(-V, N);

	// Fresnel
	float F0 = 0.020018673;
	float F = F0 + (1.0 - F0) * pow(1.0 - dot(N, R), 5.0);
	
	// Less Fresnel effect if far away
	float dist = length(eyePos - vertex);
	F *= exp(-0.001 * dist);

	// No Fresnel effect if underwater
	if (eyePos.y < 0.0) 
	 F = 0.0;

	// Reflection of the environment map (hdr texture)
	vec3 reflection = 0.5 * texture(envmap, R).rgb;
	if (bEnvmap == 0)
	{
		reflection = mix(oceanColor, sunColor, R.y * 0.5 + 0.5) * 2;	// Simulate an hdr environment map
		float sunInfluence = max(dot(R, sunDir), 0.0);
		reflection += sunColor * pow(sunInfluence, 32.0);
	}

	// Reflection of the ship
	vec2 screenCoords = gl_FragCoord.xy / textureSize(reflectionTexture, 0);
	
	// Noise for the reflection of the ship
	float grain = 10.0;
	vec2 distortion1 = texture(waterDUDV, vec2(tex.x + time, tex.y) * grain).rg * 2.0 - 1.0;
	vec2 distortion2 = texture(waterDUDV, vec2(tex.x + time, tex.y - time) * grain).rg * 2.0 - 1.0;
	vec2 totalDistortion = distortion1 + distortion2;
	screenCoords += 0.01 * totalDistortion;
	
	vec3 reflectionShip = 0.25 * texture(reflectionTexture, screenCoords).rgb;
	if (reflectionShip != vec3(0.0))	// Apply only the part of the ship
		reflection = mix(reflection, reflectionShip, 0.5); // 1.0 = full reflection of the ship, 0.0 = none
	
	// Tweaked from ARM/Mali's sample
	float turbulence = max(grad.w + 0.8, 0.0);  // 0.0 = dark, 2.0 = shining
	float color_mod = 1.0 + 3.0 * smoothstep(1.2, 1.8, turbulence);

	// Some additional specular (Ward model)
	float spec = 0.0;
	if (sunDir.y > 0.0) // // Checks if the sun is above the horizon
	{	
		const float rho = 0.3;
		const float ax = 0.2;
		const float ay = 0.1;

		vec3 h = sunDir + V;
		vec3 x = cross(sunDir, N);
		vec3 y = cross(x, N);

		float mult = (ONE_OVER_4PI * rho / (ax * ay * sqrt(max(1e-5, dot(sunDir, N) * dot(V, N)))));
		float hdotx = dot(h, x) / ax;
		float hdoty = dot(h, y) / ay;
		float hdotn = dot(h, N);

		spec = mult * exp(-((hdotx * hdotx) + (hdoty * hdoty)) / (hdotn * hdotn));
	}

	// Final color
	FragColor = vec4(mix(oceanColor, reflection * color_mod, F) + sunColor * spec, 0.75);

	// Transparent above, opaque at the edge
	FragColor.a = 1.0 - abs(dot(N, R)) * transparency;

	// The highest waves are greener and more transparent
	vec3 greenHighlight = vec3(0.0, 0.6, 0.4);	// Green
	const float sprayThresholdLower = 0.0;
	const float sprayThresholdUpper = 4.0;
	float heightFactor = smoothstep(sprayThresholdLower, sprayThresholdUpper, vertex.y);
	FragColor.rgb = mix(FragColor.rgb, greenHighlight, heightFactor);
	FragColor.a *= (1.0 - 0.5 * heightFactor);

	// Foam
	float combinedFoamFactor = max(texture(foamBuffer, tex).r, vFoamIntensity);
	if (combinedFoamFactor > 0.0)
		FragColor = ComputeFoam(tex, combinedFoamFactor);

	// Wake around the ship (texture: contourShip)
	if (bWake)
	{
		// Calculate the position of the fragment in the decal coordinate system
		vec2 decalSpacePos = RotatePoint(vertex.xz, shipPivot, -shipRotation);
    
		// Calculate the UV coordinates of the decal
		vec2 wakeUV = (decalSpacePos - shipPivot + shipSize * 0.5) / shipSize;
		
		// Check that the sampling is in the valid region [0,1]
		if (wakeUV.x >= 0.0 && wakeUV.x <= 1.0 && wakeUV.y >= 0.0 && wakeUV.y <= 1.0) 
		{
			float wakefactor = texture(contourShip, wakeUV).r;
			if (wakefactor > 0.0)
				FragColor = ComputeFoam(wakeUV, wakefactor);
		}
	}

	// Wake fater the ship (texture: wakeBuffer)
	if (bWake)
	{
		// Calculate the UV coordinates of the decal
		vec2 wakeUV = (vertex.xz - shipPivot + wakeSize * 0.5) / wakeSize;
		
		// Check that the sampling is in the valid region [0,1]
		if (wakeUV.x >= 0.0 && wakeUV.x <= 1.0 && wakeUV.y >= 0.0 && wakeUV.y <= 1.0)
		{
			float wakefactor = texture(wakeBuffer, wakeUV).r;
			if (wakefactor > 0.0)
				FragColor = ComputeFoam(wakeUV, wakefactor);
		}
	}

	if (bAbsorbance)
	{
		// Calculation of logarithmic fog
		float distanceToCamera = length(eyePos - vertex);
		float absorbanceFactor = exp(-absorbanceCoeff * distanceToCamera);
		absorbanceFactor = clamp(absorbanceFactor, 0.0, 1.0);
		vec3 finalColor = mix(absorbanceColor * exposure, FragColor.rgb, absorbanceFactor);
		FragColor = vec4(finalColor, FragColor.a);
	}

	if (bShowPatch)
	{
		float borderWidth = 0.05;
		float edge = max( smoothstep(1.0 - borderWidth, 1.0, tex.x), smoothstep(1.0 - borderWidth, 1.0, tex.y) );
		vec3 borderColor = vec3(1.0, 0.0, 0.0);
		FragColor.rgb = mix(FragColor.rgb, borderColor, edge);
	}

	// Underwater effect: dark blue-green tint if the camera is below level 0
	if (eyePos.y < 0.0) 
	{
		vec3 underwaterColor = vec3(0.0, 0.18, 0.22);		// Dark blue-green
		float depth = clamp(-eyePos.y / 20.0, 0.0, 1.0);	// Deeper = more saturated
		FragColor.rgb = mix(FragColor.rgb, underwaterColor, depth);
	}

    // Apply dynamic exposure
    FragColor.rgb = vec3(1.0) - exp(-FragColor.rgb * exposure);
}
