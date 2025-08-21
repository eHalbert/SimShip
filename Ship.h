#pragma once

#define NOMINMAX
#include <windows.h>
#include <stdlib.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include <thread>
#include <variant>
#include <random>

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
// libigl
#include <igl/readOBJ.h>
#include <igl/opengl/glfw/Viewer.h>
#include <igl/read_triangle_mesh.h>
#include <igl/write_triangle_mesh.h>
#include <igl/boundary_loop.h> 
#include <igl/slice.h>
// Eigen
#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Sparse>

using namespace std;
using namespace glm;

#include "Structures.h"
#include "Utility.h"
#include "Ocean.h"
#include "Shader.h"
#include "Camera.h"
#include "Shapes.h"
#include "Model.h"
#include "Sound.h"
#include "Timer.h"
#include "Particles.h"

struct sTriangle
{
	int			I[3];								// Indices of the face
	int			bUnder[3];							// Status relative to water height
	int			WaterStatus;						// 0 = under, 3 = above, 1 or 2 = new triangles
	vec3		Color	= vec3(0.0f, 0.0f, 0.0f);	// Color of the triangle in debug mode
	float		Area	= 0.0f;						// Total area
	vec3		CoG		= vec3(0.0f, 0.0f, 0.0f);	// Centre of gravity
	vec3		Normal	= vec3(0.0f, 0.0f, 0.0f);	// Normal vector

	float		Depth;								// Profondeur
	vec3		vPressure;							// Vecteur force de pression
	float		fPressure;							// Norme de la force de pression
};
struct sSegment 
{
	vec3 a, b;
};
struct sForce
{
	float		Magnitude	= 0.0f;
	vec3		Vector		= vec3(0.0f);
	vec3		Position	= vec3(0.0f);
};
enum eRendering { TRIANGLES = 0, BASIC_LIGHT, SUN };

struct sFoamPts
{
	vec3		pos;		// Series of points for the wake	
	float		time;		// Time of creation (for alpha fading)
};
struct sFoamVertex
{
	vec3		pos;		// Position 3D du sommet
	vec2		uv;			// Coordonnée dans la texture
	float		alpha;
};
struct sSprayPt { vec3 p; vec3 n; };

#define TRACE

class Ship
{
public:
	Ship() {};
	~Ship();

	void	Init(sShip& ship, Camera& camera);
	void	SetOcean(Ocean* ocean);
	void	SetMass() { mMass = ship.Mass_t * 1000.0f; }
	float	GetLength() { return mLength; }
	float	GetWidth() { return mWidth; }

	void	ResetVelocities();
	vec3	TransformPosition(vec3 v);
	vec3	TransformVector(vec3 v);
	void	SetYawFromHDG(float hdg);
	void	Update(float time);

	void	RenderSmoke(Camera& camera, Sky* sky);
	void	RenderSpray(Camera& camera, Sky* sky);
	void	RenderWakeVao(Camera& camera);
	void	RenderContour(Camera& camera);
	void	RenderReflexion(Camera& camera, Sky* sky);
	void	Render(Camera& camera, Sky* sky);

	GLuint	GetTraceID();

	sShip				ship;
		
	string				InfoHull;
	string				InfoFull;

	// Motion
	float				AreaWetted			= 0.0f;
	float				LWL					= 0.0f;
	float				Yaw					= 0.0f;
	float				Pitch				= 0.0f;
	float				Roll				= 0.0f;
	float				YawRate				= 0.0f;
	float				HDG					= 0.0f;
	float				SOG					= 0.0f;
	float				COG					= 0.0f;
	float				SOGbow				= 0.0f;
	float				SOGstern			= 0.0f;
	vec2				vCOG				= vec2(0.0f);

	float				PitchCouple			= 0.0f;
	float				RollCouple			= 0.0f;
	float				PitchAcceleration	= 0.0f;
	float				RollAcceleration	= 0.0f;
	float				PitchVelocity		= 0.0f;
	float				RollVelocity		= 0.0f;
	float				HeaveAcceleration	= 0.0f;
	float				HeaveVelocity		= 0.0f;
	float				SurgeAcceleration	= 0.0f;
	float				SurgeVelocity		= 0.0f;
	float				YawAcceleration		= 0.0f;
	float				YawVelocity			= 0.0f;
	float				WindAcceleration	= 0.0f;
	float				WindVelocity		= 0.0f;
	float				VariationYawSigned	= 0.0f;
	float				LinearVelocity		= 0.0f;
	float				DriftVelocity		= 0.0f;
	float				Velocity			= 0.0f;

	// Engine
	int					PowerCurrentStep	= 0;
	float				PowerApplied		= 0.0f;		// kW
	float				PowerRpm			= 0.0f;

	// Rudder
	float				RudderCurrentStep	= 0.0f;
	float				RudderAngleDeg		= 0.0f;		// Deg
	
	// Bow Thruster
	int					BowThrusterCurrentStep = 0;
	float				BowThrusterApplied	= 0.0f;		// kW
	float				BowThrusterRpm		= 0.0f;
	
	// Autopilot
	bool				bAutopilot			= false;
	int					HDGInstruction		= 0;
	bool				bDynamicAdjustment	= false;

	// Switches
	bool				bVisible			= true;
	bool				bMotion				= true;
	bool				bModel				= true;
	bool				bWireframe			= false;
	bool				bOutline			= false;
	bool				bAxis				= false;
	bool				bForces				= false;
	bool				bPressure			= false;
	eRendering			Rendering			= eRendering::SUN;
	int					WaterSearch			= 0;
	bool				bSound				= true;
	bool				bLights				= false;
	bool				bSmoke				= true;
	bool				bSpray				= true;
	bool				bRadar				= true;
	bool				bWaves				= true;
	bool				bWakeVao			= false;
	bool				bContour			= false;

	unique_ptr<BBox>	BBoxShape;


private:
	void	InitBoundingBox();
	void	InitDimensions();
	void	InitTriangles();
	void	InitCentroid();
	void	InitSurfaces();
	void	InitVolume();
	void	InitInertia();
	void	InitWaterVertices();
	void	InitVaoHull();
	void	InitContours();
	void	InitShaders();
	void	InitTextures();
	void	InitVaoWake();
	void	InitModels();
	void	InitSounds(Camera& camera);
	void	InitSpray(vector<vec3>& contour);
	void	InitSmoke();


	// Contour
	vector<vec3> ComputeContour();
	vector<vec3> ArrangeContour(const vector<vec3>& contourUnordered); 
	vector<vec3> OffsetContour(const vector<vec3>& contour, float offset);
	void	CreateTexWake(const vector<vec3>& contour);
	void	CreateContourVAO1(vector<vec3>& contour);
	void	CreateContourVAO2(vector<vec3>& contour);

	vec3	GetVerticeAtMeshIndex(int x, int z);
	int		GetHeightFast(vec3& pos);
	int		GetHeightSlow(vec3& pos);
	void	UpdateWorldMatrix();
	void	TransformVertices();
	void	GetHeightOfAllVertices();
	void	GetTrisUnderWater();
	void	CreateKelvinImages();

	// Wake by vao
	void	UpdateWakeVao();

	// SYSTEM OF FORCES
	void	ComputeArchimede();
	void    ComputeGravity();
	void	ComputeHeave(float dt);
	void	ComputeThrust(float dt);
	void	ComputeResistanceViscous(float dt);
	void	ComputeResistanceWaves(float dt);
	void	ComputeResistanceResidual(float dt);
	void	ComputeBowThrust(float dt);
	void	ComputeRudder(float dt);
	void	ComputeWind(float dt);
	void	ComputeCentrifugal(float dt);
	void	ComputeForces(float dt);
	void	UpdateAutopilot(float dt);
	void	UpdateVaoPressureLines();

	void	UpdateSounds();
	void	UpdateSmoke(float dt);
	void	UpdateSpray(float dt);
	void	UpdateWakeBuffer();
	void	UpdateTextureWakeVao();

	void	RenderForceRefBody(Camera& camera, vec3 P, vec3 V, float scale, vec3 color, bool bRenderOrigin);
	void	RenderForceRefWorld(Camera& camera, sForce& f, float scale, vec3 color, bool bRenderOrigin);
	void	RenderNavLight(Camera& camera, int i, float distance);
	void	RenderPropellers(Camera& camera, Sky* sky);
	void	RenderRudders(Camera& camera, Sky* sky);
	void	RenderRadars(Camera& camera, Sky* sky, bool bReflexion);

	// Hull for physics
	string				mPathnameHull;
	Eigen::MatrixXd		mV;
	Eigen::MatrixXi		mF;
	vector<vec3>		mvVertices;
	vector<float>		mvVertexColored;
	vector<int>			mvVertSubmerged;
	vector<float>		mvVertWaterHeight;
	vector<sTriangle>	mvTris;
	GLuint				mVaoHull = 0;
	GLuint				mVboHull = 0;
	GLuint				mEboHull = 0;
	int					mIndicesFull;

	// Full model
	string				mPathnameFull;
	sBBvec3				mBbox;					// Bounding box

	// Hydrostatic pressure forces
	GLuint				mVaoLines	= 0;
	int					mLinesCount = 0;

	// Forces
	sForce				Archimede;
	sForce				Gravity;
	sForce				ResistanceHeave;
	sForce				Thrust;
	sForce				ResistanceViscous;
	sForce				ResistanceWaves;
	sForce				ResistanceResidual;
	sForce				BowThrust;
	sForce				RudderLift;
	sForce				RudderDrag;
	sForce				WindRotation;
	sForce				WindFront;
	sForce				WindRear;
	sForce				Centrifugal;
	sForce				COGSOG;

	// Models
	unique_ptr<Model>	mModelFull;
	unique_ptr<Model>	mPropeller;
	unique_ptr<Model>	mRudder;
	unique_ptr<Sphere>	mLight;
	unique_ptr<Model>	mRadar1;
	unique_ptr<Model>	mRadar2;

	// Smoke
	const int			mSmokeMaxParticles	= 5000;
	GLuint				mSSBO_SMOKE			= 0;
	GLuint				mVaoSmoke			= 0;
	uint				mFrameSmokeCount	= 0;
	GLuint				SSBO_LIVE_COUNTER	= 0;

	// Shaders
	unique_ptr<Shader>	mShaderHullColored;
	unique_ptr<Shader>	mShaderWireframe;
	unique_ptr<Shader>	mShaderPressure;
	unique_ptr<Shader>	mShaderCamera;
	unique_ptr<Shader>	mShaderShip;
	unique_ptr<Shader>	mShaderUnicolor;
	unique_ptr<Shader>	mShaderNavLight;
	unique_ptr<Shader>	mShaderSmokeCompute;
	unique_ptr<Shader>	mShaderSmokeRender;

	// Environment
	unique_ptr<Texture>	mTexEnvironment;			// Environment texture (shaderSky)
	float				mEnvMapfactor = 0.0f;

	Ocean			  * mOcean = nullptr;			// Reference to the ocean object
	float			  * pDisplacement = nullptr;	// Map of the displacement of the vertices of the ocean mesh
	vector<vector<vec3>>mvWaterPos;

	// Physical characteristics
	float				mMass			= 1.0f;			// kg
	float				mPowerW			= 1.0f;			// watt
	float				mLength			= 0.0f;			// m
	float				mWidth			= 0.0f;			// m
	float				mHeight			= 0.0f;			// m
	float				mVolume			= 0.0f;			// m3
	float				mDraft			= 0.0f;			// m
	float				mAirDraft		= 0.0f;			// m
	float				AreaWettedMax	= 0.0f;			// m2
	float				AreaXZ			= 0.0f;			// m2
	float				AreaXZ_RacCub	= 0.0f;
	vec3				mCentroid		= vec3(0.0f);
	float				Ixx				= 0.0f;
	float				Iyy				= 0.0f;
	float				Izz				= 0.0f;
	float				Ixy				= 0.0f;
	float				Ixz				= 0.0f;
	float				Iyz				= 0.0f;
	vec3				mBow			= vec3(0.0f);	// From centre to bow
	vec3				mStern			= vec3(0.0f);	// From centre to stern
	vec3				mWakePivot		= vec3(0.0f);	// Close to the stern;
	float				mRudderArea		= 0.0f;

	// World matrice and time
	mat4				World			= mat4(1.0f);
	float               mDt				= 0.0f;			// Elapsed time since last frame

	// Constants
	const float			mGRAVITY				= 9.81f;
	const float			mWATER_DENSITY			= 1027.f;	// SI = kg / m3
	const double		mKINEMATIC_VISCOSITY	= 1.30e-6;	// Viscosité cinématique de l'eau de mer (m^2/s)
	const float			mAIR_DENSITY			= 1.225f;	// SI = kg / m3
	const float			mPLATE_DRAG_COEFF		= 1.28f;

	unique_ptr<Cube>	mForceVector;
	unique_ptr<Sphere>	mForceApplication;
	unique_ptr<Cube>	mAxis;

	unique_ptr<Sound>	mSoundPower;
	unique_ptr<Sound>	mSoundBowThruster;
	bool				mbSoundBowThrusterPlaying = false;

	//= C O N T O U R =======================================

	GLuint				mVaoContour1		= 0;
	GLuint				mVboContour1		= 0;
	int					mIndicesContour1	= 0;
	GLuint				mVaoContour2		= 0;
	GLuint				mVboContour2		= 0;
	int					mIndicesContour2	= 0;

	//= T R A I L 1 (Accumulation buffer) ===================
	
	unique_ptr<Shader>	mShaderWakeBuffer;
	unique_ptr<Texture>	mTexWake;

	GLuint				FBO_BUFFER[2]	= { 0 };
	GLuint				mTexBuffer[2]	= { 0 };
	int					mCurrentIdx		= 0;		// Ping-pong index
	unique_ptr<Shader>	mShaderBuffer;
	unique_ptr<ScreenQuad> mScreenQuadWakeBuffer;	// Used with the background shader to Draw the clouds
	vec2				mPreviousShipPosition;

	//= T R A I L  2 (Projection of VAO on texture) =========

	vector<sFoamPts>	vWakePoints;				// Points taken every second to mark the wake
	vector<sFoamVertex>	vWakeVertices;				// From the points create a vao with vertices making triangles
	GLuint				mVaoWake		= 0;
	GLuint				mVboWake		= 0;
	unique_ptr<Shader>	mShaderWakeVao;
	
	GLuint				msFBO_WAKE		= 0;
	GLuint				msTexWakeVAO	= 0;
	GLuint				FBO_BLIT_WAKE	= 0;
	GLuint              TexBlit			= 0;

	GLuint				FBO_GAUSS1		= 0;
	GLuint				mTexGauss1		= 0;
	unique_ptr<Shader>	mShaderGaussH;
	GLuint				FBO_GAUSS2		= 0;
	GLuint				mTexGauss2		= 0;
	unique_ptr<Shader>	mShaderGaussV;

	unique_ptr<Shader>	mShaderWakeVaoToTex;

	//= S P R A Y ==================================
	
	vector<sSprayPt>	mLeft;
	vector<sSprayPt>	mRight;
	float				mRandomOffsetRange = 0.1f;
	unique_ptr<Spray>	mSpray;

	//== TRACE =====================================
	
	const int			TEX_SIZE = 512;
	GLuint			  * mTexTrace;
	int					mTraceIdx = 0;		// Ping-pong index
	unique_ptr<Shader>	mShaderTrace;
	void				InitTrace();
	void				UpdateTrace();

	//==============================================

	vec3 Red			= vec3(1.0f, 0.0f, 0.0f);
	vec3 Green			= vec3(0.0f, 1.0f, 0.0f);
	vec3 Blue			= vec3(0.0f, 0.0f, 1.0f);
	
	vec3 White			= vec3(1.0f, 1.0f, 1.0f);
	vec3 Gray			= vec3(0.5f, 0.5f, 0.5f);
	vec3 Black			= vec3(0.0f, 0.0f, 0.0f);
	
	vec3 Yellow			= vec3(1.0f, 1.0f, 0.0f);
	vec3 Cyan			= vec3(0.0f, 1.0f, 1.0f);
	vec3 Magenta		= vec3(1.0f, 0.0f, 1.0f);
	
	vec3 Orange			= vec3(1.0f, 0.5f, 0.0f);
	vec3 Violet			= vec3(0.5f, 0.0f, 0.5f);
	vec3 Pink			= vec3(1.0f, 0.5f, 0.5f);
	vec3 Marron			= vec3(0.5f, 0.25f, 0.25f);
	vec3 Turquoise		= vec3(0.25f, 0.75f, 0.75f);
	vec3 Gold			= vec3(1.0f, 0.843f, 0.0f);
};

