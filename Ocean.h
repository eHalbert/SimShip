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
// fftw
#define FFTW_DLL
#include <fftw3/fftw3.h>

#include "Structures.h"
#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "Shapes.h"
#include "Utility.h"
#include "mat4.h"
#include "Sky.h"

using namespace std;
using namespace glm;

struct GridVertex
{
	vec3 position;
	vec2 texCoord;
};
struct WaveData
{
	double time;
	double dx, dy, dz;
};


class Ocean
{
public:

	Ocean(vec2 wind, Sky* sky);
	~Ocean();

	void		Init();
	void		InitFrequencies();
	void		GetWind(vec2 wind);
	float		EvaluateLambda(vec2 wind);
	void		EvaluatePersistence(float seconds);

	float		Phillips(vec2 k);
	float		JONSWAP(vec2 k);
	float		JONSWAP2(vec2 k);
	float		PiersonMoskowitz(vec2 k);
	float		DonelanBanner(vec2 k);
	float		Elfouhaily(vec2 k);
	float		Elfouhaily2(vec2 k);
	float		TexelMarsenArsloe(vec2 k);
	float		TexelMarsenArsloe2(vec2 k);

	void		Update(float t);
	void		FourierTransform(GLuint spectrum);

	bool		GetVertice(vec2 pos, vec3& output);
	void		GetRecordFromBuoy(vec2 pos, float t);
	bool		GetWaveByWaveAnalysis(float& waves1_3, float& waveMax, int& nWaves, float& average_period);
	vector<vec2>GetCut(int xN);

	pair<vector<double>, vector<double>> GetFrequencies();
	vector<sResultData> SpectralAnalysis();
	vector<sResultData> DirectionalAnalysis();

	GLuint		GetDisplacementID()	{ return mTexDisplacements; };
	GLuint		GetGradientsID()	{ return mTexGradients; };
	GLuint		GetFoamBufferID()	{ return mTexFoamBuffer; };

	float	  * GetPixelsDisplacement() { return mPixelsDisplacement.get(); };

	void		Render(const float t, Camera& camera, vec3& ShipPosition, float ShipRotation);

	// Dimensions
	const int			FFT_SIZE		= 512;				// Dimension of the FFT (1024 max)
	const int			FFT_SIZE_1		= FFT_SIZE + 1;		 
	const int			MESH_SIZE		= 256;				// Dimension of the mesh (number of cells on one side of the grid)
	const int			MESH_SIZE_1		= MESH_SIZE + 1;	 
	const int			PATCH_SIZE		= 100;				// Size of the mesh grid in meters
	const int			LengthWave		= 60;				// Dimension of the wave number
	
	// Parameters
	vec2				Wind			= { 0.0f, 1.0f };	// input of wind (no need to be normalized at this stage)
	float				Amplitude		= 1.0f;				// amplitude of the waves
	float				Lambda			= -1.0f;			// factor of choppiness (exagerate the displacements)
	
	vec3				OceanColor;
	int					iOceanColor = 6;
	vector<vec3>		vOceanColors = {
					vec3(122, 138,  92),		// Pornic
					vec3( 69, 134, 111),		// North sea light
					vec3( 37,  66,  57),		// North sea dark
					vec3(  1, 169, 193),		// Carribbean
					vec3( 34,  99, 110),		// Golf of Morbihan
					vec3(  0, 102, 133),		// Carribbean
					vec3(  1,  53,  75),		// MSFS 2024
					vec3(  0,  59,  92),		// Light blue
					vec3( 21,  92, 142),		// Channel	
					vec3(  3,  92, 175),		// Pacific
					vec3( 10,  33,  55),		// 
					vec3( 21,  61, 111)			// Iroise
	};

	float				PersistenceSec	= 4.0f;
	float				PersistenceFactor = 1.0f;
	float				Transparency = 0.05f;

	vector<WaveData>	vWaveData;						// time, dx, dy, dz
	bool				bNewData		= false;

	bool				bVisible		= true;			// rendering of the ocean or not
	bool				bEnvmap			= true;
	bool				bAttenuationFresnel = true;
	bool				bShowPatch = false;

private:
	void GetAllJacobians();
	void GetSpectrumStats(vector<float>& vS);
	void CreateMesh();
	void CreateLODMeshes();
	void CreateLODMesh(int meshSize, vector<GridVertex>& vertices, vector<unsigned int>& indices);
	void GetPatchesDecal(vec2 Position, float w, float h, float Yaw, vector<pair<int, int>>& vPatches);
	void GetPatchVertices();
	void WorldToPatch(float x, float z, float& xLocal, float& zLocal, int& i, int& j);

	// Compute shaders
	unique_ptr<Shader>		mShaderSpectrum;		// time-updated spectrum
	unique_ptr<Shader>		mShaderFft;				// fast fourier transform
	unique_ptr<Shader>		mShaderDisplacements;	// displacements
	unique_ptr<Shader>		mShaderGradients;		// normals & jacobians
	
	// Environment
	Sky					  * mSky = nullptr;

	// Rendering mShader
	unique_ptr<Shader>		mShaderOcean;			// mShader of ocean rendering with instanciated matrices
	unique_ptr<Shader>		mShaderOceanWake;		// mShader of ocean rendering with wake texture applied

	// Textures of storage (glTexStorage2D)
	GLuint					mTexInitialSpectrum = 0;		// initial spectrum \tilde{h}_0
	GLuint					mTextFrequencies = 0;			// frequency \omega_i per wave vector
	GLuint					mTexUpdatedSpectra[2] = { 0 };	// updated spectra \tilde{h}(\mathbf{k},t) and \tilde{\mathbf{D}}(\mathbf{k},t) [reused for FT result]
	GLuint					mTexTempData = 0;				// intermediate data for FFT
	GLuint					mTexDisplacements = 0;			// displacements map
	GLuint					mTexGradients = 0;				// normals & foldings map
	GLuint					mTexFoamAcc1 = 0;				// accumulation buffer of the foam (swap due to readonly and writeonly)
	GLuint					mTexFoamAcc2 = 0;				// accumulation buffer of the foam (swap due to readonly and writeonly)
	GLuint					mTexFoamBuffer = 0;				// either accfoam1 or accfoam2 (the last writeonly which is read for rendering purpose) 

	// Textures of rendering
	unique_ptr<Texture>		mTexEnvironment;		// environment texture (shaderSky)
	unique_ptr<Texture>		mTexFoamDesign;			// foam texture
	unique_ptr<Texture>		mTexFoamBubbles;		// foam bubbles
	unique_ptr<Texture>		mTexFoam;				// texture of the wake
	unique_ptr<Texture>		mTexWaterDuDv;			// texture to bring noise for the reflection of the ship on the water 

	// Miscellaneous
	float					mGravity = 9.81f;
	GLuint					mVbo, mIbo, mVao;
	int						mIndicesCount = 0;
	unique_ptr<float[]>		mPixelsDisplacement = nullptr;

	vector<vector<vec3>>	mvPatchVertices;
	vector<GLuint>			mvVAOs;
	vector<int>				mvMeshSizes = { 256, 128, 64, 32, 16 };	// LOD 0, 1, 2, 3, 4
	vector<int>				mvIndicesCounts;

	vector<double>      a_Frequences;
	vector<double>      a_DensiteSpectrale;
	vector<double>      a_Directions;

};

