#version 430

#define ONE_OVER_4PI	0.0795774715459476

uniform samplerCube	envmap;
uniform sampler2D	gradients;
uniform sampler2D	foamBuffer;
uniform sampler2D	foamDesign;
uniform sampler2D	foamBubbles;
uniform sampler2D	foamTexture;

in vec3			vdir;
in vec2			tex;
in vec3			vertex;
flat in int		lod;
flat in int		iFoam;

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
uniform bool	bShowPatch;

out vec4 FragColor;


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
	if (iFoam == 1 && lod < 3.0)
	{
		vec3 greenHighlight = vec3(0.0, 0.6, 0.4);	// Green
		const float sprayThresholdLower = 0.0;
		const float sprayThresholdUpper = 4.0;
		float heightFactor = smoothstep(sprayThresholdLower, sprayThresholdUpper, vertex.y);
		FragColor.rgb = mix(FragColor.rgb, greenHighlight, heightFactor);
		FragColor.a *= (1.0 - 0.5 * heightFactor);
	}

	// Foam
	float foamFactor = texture(foamBuffer, tex).r;
	if (iFoam == 1 && foamFactor > 0.0 && lod < 3.0)
		FragColor = ComputeFoam(tex, foamFactor);

	if (bAbsorbance)
	{
		// Calculation of logarithmic fog
		float distanceToCamera = length(eyePos - vertex);
		float absorbanceFactor = exp(-absorbanceCoeff * distanceToCamera);
		absorbanceFactor = clamp(absorbanceFactor, 0.0, 1.0);
		vec3 finalColor = mix(absorbanceColor * exposure, FragColor.rgb, absorbanceFactor);
		FragColor = vec4(finalColor, FragColor.a);
	}

	// Color the left and bottom sides of the patch
	if (bShowPatch)
	{
		float borderWidth = 0.05;
		float edge = max( smoothstep(1.0 - borderWidth, 1.0, tex.x), smoothstep(1.0 - borderWidth, 1.0, tex.y) );
		vec3 borderColor = vec3(1.0, 0.0, 0.0);
		switch(int(floor(lod)))
		{
		case 0: borderColor = vec3(1.0, 0.0, 0.0); break;	// Red
		case 1: borderColor = vec3(0.0, 1.0, 0.0); break;	// Green
		case 2: borderColor = vec3(0.0, 0.0, 1.0); break;	// Blue
		case 3: borderColor = vec3(1.0, 0.0, 1.0); break;	// Magenta
		case 4: borderColor = vec3(0.0, 1.0, 1.0); break;	// Cyan
		}
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
