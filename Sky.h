/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#pragma once

#define NOMINMAX
#include <complex>
#include <vector>

// glad
#include <glad/glad.h>
// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "Structures.h"
#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "Shapes.h"
#include "Utility.h"
#include "mat4.h"

using namespace std;
using namespace glm;

struct sHM
{
	int hour;
	int minute;
};

class Sky
{
public:
	Sky(vec2 pos, int width, int height);
	~Sky();

	void	Render(Camera& camera);
	GLuint	GetTexture() { return mTextureSky; }
	void	SetObserver(vec2 pos);
	
	void	SetNow();
	sHM		GetNow();
	void	SetTime(int hour, int minute);
	sHM		GetTime();

	float	SunAzimut		= 0.0f;					// 0 = z-, 180 = z+ (anticlockwise)
	float	SunElevation	= 30.0f;				// 0 = y+, 180 = y- (clockwise)
	float	SunDistance		= 20000.0f;
	
	int		SunHour			= 12;					// Store the hours
	int		SunMinute		= 0;					// Store the minutes
	tm		tmTimeStored;

	float	LongitudeObserver = 0.0f;
	float	LatitudeObserver = 45.0f;

	vec3	SunPosition		= vec3(0.f, 0.f, 0.f);
	vec3	SunDirection	= vec3(0.f, 0.f, 0.f);
	vec3	SunDirectionEB	= vec3(0.f, 0.f, 0.f);

	vec3	SunEmissive		= vec3(1.f, 0.98f, 0.92f);
	vec3	SunAmbient		= vec3(0.75f, 0.75f, 0.75f);
	vec3	SunDiffuse		= vec3(1.f, 0.98f, 0.92f);
	vec3	SunSpecular		= vec3(1.f, 0.98f, 0.92f);

	bool	bAbsorbance		= true;
	vec3	AbsorbanceColor = { 0.7f, 0.8f, 0.9f };
	float	AbsorbanceCoeff = 0.00017f;

	vec3	FogColor		= vec3(1.0f);
	float	FogDensity		= 0.0f;
	float	MistDensity		= 0.00015f;

	bool	bRain			= false;
	bool	bRainDropsTrails = false;
	bool	bRainBlurDrips		= false;

	float	Exposure		= 1.0f;

private:
	void	SetPositionFromDistance();
	void	ExpositionFromAngle();
	void	ColorOfTheSun();

	int		mWidth, mHeight;

	GLuint	mTexIrradiance;
	GLuint	mTexInscatter;
	GLuint	mTexTransmittance;
	GLuint	mTexNoise;
	
	GLuint	FRAMEBUFFER_SKY;
	GLuint	mTextureSky;

	Shader* mShader = nullptr;
	Quad  * mQuad	= nullptr;
};

