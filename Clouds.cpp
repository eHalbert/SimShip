// Copyright (c) 2018 Federico Vaccaro

#include "Clouds.h"
#include <random>

#define INT_CEIL(n,d) (int)ceil((float)n/d)

VolumetricClouds::VolumetricClouds(int width, int height): SCR_WIDTH(width), SCR_HEIGHT(height)
{
	mVolumetricCloudsShader = make_unique<Shader>("", "", "", "Resources/Clouds/volumetric_clouds.comp");
	mWeatherShader = make_unique<Shader>("", "", "", "Resources/Clouds/weather.comp");

	mPostProcessingShader = make_unique<Shader>("Resources/Clouds/screen.vert", "Resources/Clouds/clouds_post.frag");
	mPostProcessingScreenQuad = make_unique<ScreenQuad>();

	InitVariables();

	//------------------------------------------------

	// 0	fragColor
	// 1	bloom
	// 2	alphaness
	// 3	cloudDistance

	TexClouds = new unsigned int[4];			// Generates an array of 4 textures (TexCloudsColor), each in 32-bit RGBA format
	glGenTextures(4, TexClouds);
	for (unsigned int i = 0; i < 4; i++) 
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TexClouds[i]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

		glBindImageTexture(0, TexClouds[i], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	}

	//------------------------------------------------

	glGenFramebuffers(1, &mCloudsPostProcessingFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, mCloudsPostProcessingFBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

	glGenTextures(1, &depthTex);
	glBindTexture(GL_TEXTURE_2D, depthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTex, 0);

	// FBO Verification
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "Error creating FBO for the post processing of the clouds" << endl;
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GenerateModelTextures();
}
VolumetricClouds::~VolumetricClouds()
{
	glDeleteTextures(1, &mPerlinTex);
	glDeleteTextures(1, &mWorley32);
	glDeleteTextures(1, &mWeatherTex);
}

void VolumetricClouds::GenerateModelTextures()
{
	// PERLIN texture
	glGenTextures(1, &mPerlinTex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, mPerlinTex);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 128, 128, 128, 0, GL_RGBA, GL_FLOAT, NULL);
	glGenerateMipmap(GL_TEXTURE_3D);
	glBindImageTexture(0, mPerlinTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

	Shader comp("", "", "", "Resources/Clouds/perlinworley.comp");
	comp.use();
	comp.setVec3("u_resolution", vec3(128, 128, 128));
	glActiveTexture(GL_TEXTURE0);
	comp.setInt("outVolTex", 0);
	glBindTexture(GL_TEXTURE_3D, this->mPerlinTex);
	glBindImageTexture(0, this->mPerlinTex, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
	glDispatchCompute(INT_CEIL(128, 4), INT_CEIL(128, 4), INT_CEIL(128, 4));
	glGenerateMipmap(GL_TEXTURE_3D);

	// WORLEY texture
	glGenTextures(1, &mWorley32);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, mWorley32);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 32, 32, 32, 0, GL_RGBA, GL_FLOAT, NULL);
	glGenerateMipmap(GL_TEXTURE_3D);
	glBindImageTexture(0, mWorley32, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

	Shader worley_git("", "", "", "Resources/Clouds/worley.comp");
	worley_git.use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, this->mWorley32);
	glBindImageTexture(0, this->mWorley32, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
	glDispatchCompute(INT_CEIL(32, 4), INT_CEIL(32, 4), INT_CEIL(32, 4));
	glGenerateMipmap(GL_TEXTURE_3D);

	// WEATHER texture
	glGenTextures(1, &mWeatherTex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mWeatherTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1024, 1024, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindImageTexture(0, mWeatherTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	Seed = vec3(0.0f);

	Shader weather("", "", "", "Resources/Clouds/weather.comp");
	weather.use();
	weather.setVec3("seed", Seed);
	weather.setFloat("perlinFrequency", PerlinFrequency);
	glDispatchCompute(INT_CEIL(1024, 8), INT_CEIL(1024, 8), 1);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
void VolumetricClouds::GenerateWeatherMap()
{
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_real_distribution<> dis(.0, 100.);

	float x, y, z;
	x = dis(gen);
	y = dis(gen);
	z = dis(gen);

	Seed = vec3(x, y, z);

	glBindImageTexture(0, mWeatherTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	Shader weather("", "", "", "Resources/Clouds/weather.comp");
	weather.use();
	weather.setVec3("seed", Seed);
	weather.setFloat("perlinFrequency", PerlinFrequency);
	glDispatchCompute(INT_CEIL(1024, 8), INT_CEIL(1024, 8), 1);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
void VolumetricClouds::InitVariables()
{
	CloudSpeed			= 600.0f;
	Coverage			= 0.3f;
	Crispiness			= 40.f;
	Curliness			= 0.3f;
	Density				= 0.02f;
	Absorption			= 0.5f;
	Illumination		= 5.0f;

	EarthRadius			= 600000.0f;
	SphereInnerRadius	= 8000.0f;
	SphereOuterRadius	= 17000.0f;

	PerlinFrequency		= 0.8f;

	bEnableGodRays		= true;
	bPostProcess		= true;

	Seed				= vec3(0.0, 0.0, 0.0);

	CloudColorTop		= vec3(0.994, 0.876, 0.876);		// 253, 223, 223
	CloudColorBottom	= vec3(0.04, 0.04, 0.04);		//vec3(0.243, 0.263, 0.298);		// 62, 67, 76

	mWeatherTex			= 0;
	mPerlinTex			= 0;
	mWorley32			= 0;
}

void VolumetricClouds::Render(Camera& camera, Sky* sky, vec2 wind)
{
	for (int i = 0; i < 4; ++i) 
		glBindImageTexture(i, TexClouds[i], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	mVolumetricCloudsShader->use();	// volumetric_clouds.comp

	mVolumetricCloudsShader->setVec2("iResolution", vec2(SCR_WIDTH, SCR_HEIGHT));
	mVolumetricCloudsShader->setFloat("iTime", glfwGetTime());
	mVolumetricCloudsShader->setMat4("inv_proj", glm::inverse(camera.GetProjection()));
	mVolumetricCloudsShader->setMat4("inv_view", glm::inverse(camera.GetView()));
	mVolumetricCloudsShader->setVec3("cameraPosition", camera.GetPosition());
	mVolumetricCloudsShader->setVec3("wind", vec3(wind.x, 0.0, wind.y));
	mVolumetricCloudsShader->setFloat("FOV", camera.GetZoom());
	mVolumetricCloudsShader->setVec3("lightDirection", glm::normalize(sky->SunPosition - camera.GetPosition()));
	mVolumetricCloudsShader->setVec3("lightColor", sky->SunDiffuse);
	mVolumetricCloudsShader->setFloat("coverage_multiplier", Coverage);
	mVolumetricCloudsShader->setFloat("cloudSpeed", CloudSpeed);
	mVolumetricCloudsShader->setFloat("crispiness", Crispiness);
	mVolumetricCloudsShader->setFloat("curliness", Curliness);
	mVolumetricCloudsShader->setFloat("illumination", Illumination);
	mVolumetricCloudsShader->setFloat("absorption", Absorption * 0.01);
	mVolumetricCloudsShader->setFloat("densityFactor", Density);

	mVolumetricCloudsShader->setFloat("earthRadius", EarthRadius);
	mVolumetricCloudsShader->setFloat("sphereInnerRadius", SphereInnerRadius);
	mVolumetricCloudsShader->setFloat("sphereOuterRadius", SphereOuterRadius);

	mVolumetricCloudsShader->setVec3("cloudColorTop", CloudColorTop);
	mVolumetricCloudsShader->setVec3("cloudColorBottom", CloudColorBottom);

	mat4 vp = camera.GetViewProjection();
	mVolumetricCloudsShader->setMat4("invViewProj", glm::inverse(vp));
	mVolumetricCloudsShader->setMat4("gVP", vp);

	mVolumetricCloudsShader->setSampler3D("cloud", mPerlinTex, 0);
	mVolumetricCloudsShader->setSampler3D("worley32", mWorley32, 1);
	mVolumetricCloudsShader->setSampler2D("weatherTex", mWeatherTex, 2);
	mVolumetricCloudsShader->setSampler2D("sky", sky->GetTexture(), 3);

	glDispatchCompute(INT_CEIL(SCR_WIDTH, 16), INT_CEIL(SCR_HEIGHT, 16), 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	if (bPostProcess) 
	{
		// Cloud post processing filtering
		
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, mCloudsPostProcessingFBO);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

		mPostProcessingShader->use();	// Clouds/screen.vert Clouds/clouds_post.frag

		mPostProcessingShader->setSampler2D("clouds", TexClouds[0], 0);		// Color
		mPostProcessingShader->setSampler2D("emissions", TexClouds[1], 1);	// Bloom
		mPostProcessingShader->setVec2("cloudRenderResolution", vec2(SCR_WIDTH, SCR_HEIGHT));
		mPostProcessingShader->setVec2("resolution", vec2(SCR_WIDTH, SCR_HEIGHT));

		mat4 lightModel;
		lightModel = glm::translate(lightModel, sky->SunPosition);
		vec4 pos = vp * lightModel * vec4(0.0, 60.0, 0.0, 1.0);
		pos = pos / pos.w;
		pos = pos * 0.5f + 0.5f;
		mPostProcessingShader->setVec4("lightPos", pos);

		bool isLightInFront = false;
		float lightDotCameraFront = glm::dot(glm::normalize(sky->SunPosition - camera.GetPosition()), glm::normalize(camera.GetAt()));
		if (lightDotCameraFront > 0.2)
			isLightInFront = true;

		mPostProcessingShader->setBool("isLightInFront", isLightInFront);
		mPostProcessingShader->setBool("enableGodRays", bEnableGodRays);
		mPostProcessingShader->setFloat("lightDotCameraFront", lightDotCameraFront);
		mPostProcessingShader->setFloat("time", glfwGetTime());
		mPostProcessingShader->setFloat("exposure", sky->Exposure);

		mPostProcessingScreenQuad->Render();
	}
}


