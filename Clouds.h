#pragma once

// Copyright (c) 2018 Federico Vaccaro

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

#include "Shader.h"
#include "Camera.h"
#include "texture.h"
#include "Sky.h"

using namespace std;
using namespace glm;

class VolumetricClouds
{
public:
	VolumetricClouds(int width, int height);
	~VolumetricClouds();
	
	void GenerateWeatherMap();
	void GenerateModelTextures();
	void InitVariables();

	void Render(Camera& camera, Sky* sky, vec2 wind);
	GLuint GetTexture() { return tex; }

	float	Coverage			= 0.0f;
	float	CloudSpeed			= 0.0f;
	float	Crispiness			= 0.0f;
	float	Curliness			= 0.0f;
	float	Density				= 0.0f;
	float	Illumination		= 1.8f;
	float	Absorption = 0.0f;
	float	EarthRadius			= 0.0f;
	float	SphereInnerRadius	= 0.0f;
	float	SphereOuterRadius	= 0.0f;
	float	PerlinFrequency		= 0.0f;
	bool	bEnableGodRays		= true;
	bool	bPostProcess		= true;

	vec3	CloudColorTop		= vec3(0.0f);
	vec3	CloudColorBottom	= vec3(0.0f);
	
	vec3	Seed				= vec3(0.0f);

private:
	int SCR_WIDTH, SCR_HEIGHT;
	
	unique_ptr<Shader>		mVolumetricCloudsShader;
	unique_ptr<Shader>		mWeatherShader;
	
	unique_ptr<Shader>		mPostProcessingShader;
	unique_ptr<ScreenQuad>	mPostProcessingScreenQuad;

	GLuint				  * TexClouds;

	GLuint					mCloudsPostProcessingFBO = 0;
	GLuint					tex				= 0;
	GLuint					depthTex		= 0;
	
	GLuint					mPerlinTex		= 0;
	GLuint					mWorley32		= 0;
	GLuint					mWeatherTex		= 0;
};

