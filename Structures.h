/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

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

enum eClass { FastBoat = 0, Corvette, Frigate, Fishing, Submarine, Ferry, Tugboat, Cargo, Supertanker };

struct sShip
{
	// Files
	string		ShortName			= "";			// Name
	string		PathnameHull		= "";			// Pathname of the hull for simulation
	string		PathnameFull		= "";			// Pathname of the model for rendering
	string		PathnamePropeller1	= "";			// Pathname of the propeller (moving piece)
	string		PathnamePropeller2	= "";			// Pathname of the propeller (moving piece)
	string		PathnameRudder		= "";			// Pathname of the rudder (moving piece)
	string		PathnameRadar1		= "";			// Pathname of the radar (moving piece)
	string		PathnameRadar2		= "";			// Pathname of the radar (moving piece)
	string		PathnameFlag		= "";			// Pathname of the flag
	string		ThrustSound			= "";			// Name of the sound file
	string		BowThrusterSound	= "";			// Name of the sound file
	string		SternThrusterSound	= "";			// Name of the sound file

	// Positions
	vec3		Position			= vec3(0.0);		// Position of the ship (at the center)
	vec3		Rotation			= vec3(0.0);		// Rotation on the 3 axis
	vec3		ViewWheel			= vec3(0.0);		// View on the bridge - center
	vec3		ViewLeft			= vec3(0.0);		// View on the bridge - left
	vec3		ViewRight			= vec3(0.0);		// View on the bridge - right
	vec3		ViewBow				= vec3(0.0);		// View at the bow
	vec3		ViewStern			= vec3(0.0);		// View at the stern

	// Dimensions
	eClass		Class				= eClass::Corvette;	// Class of ship that defines certain parameters
	float		Length				= 10.0;				// Final length
	float		SpeedMaxKn			= 15.0;				// Speed max after sea trials
	float		SpeedTestKn			= 14.0;				// Speed for sea trials at notch 7/10
	float		Mass_t				= 1.0;				// Tons
	vec3		PosGravity			= vec3(0.0);		// Offset of the center of gravity (relative to Position)
	float		HeaveCoef			= 1.0;				// Performance of heave (damping)
	float		FormFactor			= 0.15;				// Form factor according to ITTC 1957
	float		EnvMapFactor		= 0.0;				// Factor of environment reflexion (between 0.0 and 1.0)
	float		mAreaFront			= 0.0;
	vec3		mAreaFrontCenter		= vec3(0.0);
	float		mAreaLat				= 0.0;
	vec3		mAreaLatCenter		= vec3(0.0);

	// Spray
	float		SprayVerticalPerf	= 10.0;				// Vertical spray performance
	int			SprayMultiplier		= 1;				// Number of points between points of contour
	float		SprayLength			= 0.1;				// % of length of the ship taken on the contour
	int			SprayType			= 0;				// 0 = sharp (like a frigate), 1 = rounded (like a cargo)

	// Rudder
	vec3		PosRudder			= vec3(0.0);		// Offset of the center of the rudder (relative to Position)
	int			RudderIncrement		= 1;				// Degrees
	int			RudderStepMax		= 35;				// Number of increments
	float		RudderRotSpeed		= 10.0;				// Degrees / sec
	int			nRudder				= 2;				// Number of rudders
	vec3		PosRudder1			= vec3(0.0);		// Left
	vec3		PosRudder2			= vec3(0.0);		// Right
	
	// Turning
	float		TurningPerf			= 10.0;				// Performance of efficiency of the rudder
	float		TurningDragCoef		= 4.0;				// Performance of efficiency of the counter drift
	float		RoTMax				= 120.0;			// Maximum rate of turn (°/min)
	float		HighSpeedCoeff		= 0.1;				// Coefficient applied to the rate of turn at SpeedTestKn
	float		PivotFwd			= 0.2;				// Pivot point in forward motion (from Bow to length (stern))
	float		PivotBwd			= 0.7;				// Pivot point in backward motion (from Bow to length (stern))
	float		CentrifugalPerf		= 10.0;				// Performance of the centrifugal force

	// Power
	vec3		PosPower			= vec3(0.0);	// Offset of the center of the propeller where the power is applied (relative to Position)
	float		PowerkW				= 1000.0;		// kiloWatts
	int			PowerStepMax		= 10;			// Number of steps on the throttle lever

	// Propellers
	float		PropRpmMax			= 200.0;		// Maximum RPM of the propeller
	float		PropRpmIncrement	= 20.0;			// Rate of increase/decrease RPM of the propeller
	int			nPropeller			= 2;			// Number of propellers
	vec3		PosPropeller1		= vec3(0.0);	// Left propeller
	float		mPropTorque1			= 0.0;			// +1.0 for right propeller, -1.0 otherwise
	vec3		PosPropeller2		= vec3(0.0);	// Right propeller
	float		mPropTorque2			= 0.0;			// +1.0 for right propeller, -1.0 otherwise
	float		PropDiameter		= 3.0;			// Diameter of the propeller
	float		WakeWidth			= 1.0;			// Real wake width is mWidth x WakeWidth
	
	// Chimneys
	int			nChimney			= 2;				// Number of chimneys
	vec3		PosChimney1			= vec3(0.0);	// Left chimney
	vec3		PosChimney2			= vec3(0.0);	// Right chimney

	// Bow Thruster
	bool		HasBowThruster		= true;
	vec3		PosBowThruster		= vec3(0.0);	// Offset of the center of the center of the bow thruster (relative to Position)
	float		BowThrusterPerf		= 0.4;			// Performance of efficiency of the system Engine - Propeller
	float		BowThrusterPowerW	= 10000.0;		// Watts
	int			BowThrusterStepMax	= 5;			// Number of steps on the throttle lever
	float		BowThrusterRpmMin	= 0.0;			// Minimum RPM of the propeller
	float		BowThrusterRpmMax	= 500.0;		// Maximum RPM of the propeller
	float		BowThrusterRpmIncrement = 10.0;		// Rate of increase/decrease RPM of the propeller

	// Stern Thruster
	bool		HasSternThruster	= true;
	vec3		PosSternThruster	= vec3(0.0);	// Offset of the center of the center of the stern thruster (relative to Position)
	float		SternThrusterPerf	= 0.4;			// Performance of efficiency of the system Engine - Propeller
	float		SternThrusterPowerW	= 10000.0;		// Watts
	int			SternThrusterStepMax= 5;			// Number of steps on the throttle lever
	float		SternThrusterRpmMin	= 0.0;			// Minimum RPM of the propeller
	float		SternThrusterRpmMax	= 500.0;		// Maximum RPM of the propeller
	float		SternThrusterRpmIncrement = 10.0;	// Rate of increase/decrease RPM of the propeller

	// Lights
	vector<vec3>LightPositions;						// Offset of the center of the lights (relative to Position)
	vector<vec3>LightColors;						// Color the lights

	// Radar
	int			nRadar			= 0;
	vec3		PosRadar1		= vec3(0.0);
	float		RotationRadar1	= 40.0;			// Tours/min
	vec3		PosRadar2		= vec3(0.0);
	float		RotationRadar2	= 40.0;			// Tours/min

	// Autopilot
	float		BaseP			= 4.0;				// Make the turn
	float		BaseI			= 2.0;				// Correct a constant deviation
	float		BaseD			= 4.0;				// Anticipate the end of the turn
	float		MaxIntegral		= 5.0;				// Limit of the integral to avoid runaway

	// Waves
	float		CenterFore		= 0.0;				// Reference X for the position of the kelvin texture
	int			BaseFroude		= 0;				// Normally = 0, meaning that the real froude scheme is take, otherwise, take a higher Froude scheme

	// Flag
	bool		bFlag			= true;
	vec3		PosFlag			= vec3(0.0);
	float		DimXFlag		= 1.0;
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

	sTerrain() noexcept = default;
};
struct sPositions 
{
	string	name;
	vec2	pos;
	float	heading; // in degrees
};
struct sLine
{
	vec2 p1;
	vec2 p2;
};