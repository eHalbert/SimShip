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

class Sky
{
public:
	Sky(int width, int height);
	~Sky();

	void	Render(Camera& camera);
	GLuint	GetTexture() { return mTextureSky; }
	void	SetPositionFromDistance();
	void	SetSunAngles(vec2 pos);
	void	SetSunAngles(vec2 pos, float hour);
	void	ExpositionFromAngle();
	void	ColorOfTheSun();
	float	GetHourFromNow();

	float		SunAzimut = 0.0f;					// 0 = z-, 180 = z+ (anticlockwise)
	float		SunElevation = 30.0f;				// 0 = y+, 180 = y- (clockwise)
	float		SunDistance = 20000.0f;

	vec3		SunPosition = vec3(0.f, 0.f, 0.f);
	vec3		SunDirection = vec3(0.f, 0.f, 0.f);
	vec3		SunDirectionEB = vec3(0.f, 0.f, 0.f);

	vec3		SunEmissive = vec3(1.f, 0.98f, 0.92f);
	vec3		SunAmbient = vec3(0.75f, 0.75f, 0.75f);
	vec3		SunDiffuse = vec3(1.f, 0.98f, 0.92f);
	vec3		SunSpecular = vec3(1.f, 0.98f, 0.92f);

	bool		bAbsorbance		= true;
	vec3		AbsorbanceColor = { 0.7f, 0.8f, 0.9f };
	float		AbsorbanceCoeff = 0.00017f;

	vec3		FogColor		= vec3(1.0f);
	float		FogDensity		= 0.0f;
	float		MistDensity		= 0.00015f;

	float		Exposure		= 1.0f;

private:
	int			mWidth, mHeight;

	GLuint		mTexIrradiance;
	GLuint		mTexInscatter;
	GLuint		mTexTransmittance;
	GLuint		mTexNoise;
	
	GLuint		FRAMEBUFFER_SKY;
	GLuint		mTextureSky;

	Shader	  * mShader = nullptr;
	Quad	  * mQuad = nullptr;
};

