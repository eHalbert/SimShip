#pragma once

#define NOMINMAX
#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>
#include <time.h>

// glm
#include <glm/glm.hpp>

#include "Model.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p) = NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p) = NULL; } }
#endif

using namespace std;
using namespace glm;

struct sResultData
{
	string	variable;
	double	value;
	int		decimal;
	string	unit;
};

struct sShip
{
	string		ShortName;						// Name
	string		PathnameHull;					// Pathname of the hull for simulation
	string		PathnameFull;					// Pathname of the model for rendering
	string		PathnamePropeller;				// Pathname of the propeller (moving piece)
	string		PathnameRudder;					// Pathname of the rudder (moving piece)
	string		PathnameRadar1;					// Pathname of the radar (moving piece)
	string		PathnameRadar2;					// Pathname of the radar (moving piece)

	vec3		Position = vec3(0.0f);			// Position of the ship (at the center)
	vec3		Rotation = vec3(0.0f);			// Rotation on the 3 axis
	vec3		View1 = vec3(0.0f);				// View on the bridge
	vec3		View2 = vec3(0.0f);				// View 2 somewhere
	vec3		View3 = vec3(0.0f);				// View 3 somewhere

	float		Length = 10.0f;					// Final length after rescaling
	float		Mass_t = 1.0f;					// Tons
	vec3		PosGravity = vec3(0.0f);		// Offset of the center of gravity (relative to Position)

	float		EnvMapfactor = 0.0f;			// Factor of environment reflexion (between 0.0 and 1.0)

	// Rudder
	vec3		PosRudder = vec3(0.0f);			// Offset of the center of the rudder (relative to Position)
	float		RudderIncrement = 1.0f;			// Degrees
	int			RudderStepMax = 35;				// Number of increments
	float		RudderRotSpeed = 10.0f;			// Degrees / sec
	float		RudderLiftPerf = 4.0f;			// Performance of efficiency of the rudder
	float		RudderDragPerf = 2.0f;			// Performance of efficiency of the rudder
	float		TurningDragPerf = 4.0f;			// Performance of efficiency of the counter drift
	float		CentrifugalPerf = 10.0f;		// Performance of the centrifugal force
	vec3		RudderPivotFwd = vec3(0.0f);	// Pivot point in forward motion (from Position to Bow, on X axis)
	vec3		RudderPivotBwd = vec3(0.0f);	// Pivot point in backward motion (from Position to Stern, on X axis)
	int			nRudder = 2;					// Number of rudders
	vec3		Rudder1 = vec3(0.0f);			// Left
	vec3		Rudder2 = vec3(0.0f);			// Right
	vec3		Rudder3 = vec3(0.0f);			// Centre

	// Power
	vec3		PosPower = vec3(0.0f);			// Offset of the center of the propeller where the power is applied (relative to Position)
	float		PowerPerf = 0.4f;				// Percentage of the efficency of the system Engine - Propeller
	float		PowerkW = 100000.0f;			// kiloWatts
	float		MaxStaticThrust;				// Initial thrust
	int			PowerStepMax = 10;				// Number of steps on the throttle lever
	float		PowerRpmMin = 0.0f;				// Minimum RPM of the propeller
	float		PowerRpmMax = 200.0f;			// Maximum RPM of the propeller
	float		PowerRpmIncrement = 20.f;		// Rate of increase/decrease RPM of the propeller
	int			nChimney = 2;					// Number of chimneys
	vec3		Chimney1 = vec3(0.0f);			// Left chimney
	vec3		Chimney2 = vec3(0.0f);			// Right chimney
	int			nPropeller = 2;					// Number of propellers
	vec3		Propeller1 = vec3(0.0f);		// Left propeller
	vec3		Propeller2 = vec3(0.0f);		// Right propeller
	vec3		Propeller3 = vec3(0.0f);		// Centre propeller
	float		WakeWidth = 1.0f;				// Real wake width is mWidth x WakeWidth
	string		ThrustSound = "";				// Name of the sound file

	// Bow Thruster
	bool		HasBowThruster = true;
	vec3		PosBowThruster = vec3(0.0f);	// Offset of the center of the center of the bow thruster (relative to Position)
	float		BowThrusterPerf = 0.4f;			// // Performance of efficiency of the system Engine - Propeller
	float		BowThrusterPowerW = 10000.0f;	// Watts
	int			BowThrusterStepMax = 5;			// Number of steps on the throttle lever
	float		BowThrusterRpmMin = 0.0f;		// Minimum RPM of the propeller
	float		BowThrusterRpmMax = 500.0f;		// Maximum RPM of the propeller
	float		BowThrusterRpmIncrement = 10.f;	// Rate of increase/decrease RPM of the propeller
	string		BowThrusterSound = "";

	// Lights
	vector<vec3>LightPositions;					// Offset of the center of the lights (relative to Position)
	vector<vec3>LightColors;					// Color the lights

	// Radar
	int			nRadar = 0;
	vec3		Radar1 = vec3(0.0f);
	float		RotationRadar1 = 40.0f;					// Tours/min
	vec3		Radar2 = vec3(0.0f);
	float		RotationRadar2 = 40.0f;					// Tours/min

	// Autopilot
	float		BaseP = 2.0f;
	float		BaseI = 0.3f;
	float		BaseD = 20.0f;
	float		MaxIntegral = 10.0f;	// Limit of the integral to avoid runaway
	float		SpeedFactor = 5.0f;		// Factor to adjust the influence of speed
	float		MinSpeed = 1.0f;		// Minimum speed to avoid division by zero
	float		LowSpeedBoost = 2.0f;	// Low speed amplification factor
	float		SeaSateFactor = 1.0f;	// Increases responsiveness in difficult conditions (Pitch and Roll)

};
struct sTerrain
{
	string	file;
	float	xMin			= 0.0f;
	float	xMax			= 0.0f;
	float	zMin			= 0.0f;
	float	zMax			= 0.0f;
	vec2	center			= vec2(0.0f);
	float	widthMeters		= 0.0f;
	float	heightMeters	= 0.0f;
	int		zoom			= 14;
	unique_ptr<Model> model;
	vec3	pos				= vec3(0.0f);
	vec3	scale			= vec3(1.0f);
};
struct sPositions 
{
	string	name;
	vec2	pos;
	float	heading; // in degrees
};