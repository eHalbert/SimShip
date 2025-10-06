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
	string		ShortName		= "";				// Name
	string		PathnameHull	= "";				// Pathname of the hull for simulation
	string		PathnameFull	= "";				// Pathname of the model for rendering
	string		PathnamePropeller1 = "";			// Pathname of the propeller (moving piece)
	string		PathnamePropeller2 = "";			// Pathname of the propeller (moving piece)
	string		PathnameRudder	= "";				// Pathname of the rudder (moving piece)
	string		PathnameRadar1	= "";				// Pathname of the radar (moving piece)
	string		PathnameRadar2	= "";				// Pathname of the radar (moving piece)
	string		PathnameFlag	= "";				// Pathname of the flag
	string		ThrustSound		= "";				// Name of the sound file
	string		BowThrusterSound = "";				// Name of the sound file
	string		SternThrusterSound = "";			// Name of the sound file

	// Positions
	vec3		Position		= vec3(0.0f);		// Position of the ship (at the center)
	vec3		Rotation		= vec3(0.0f);		// Rotation on the 3 axis
	vec3		ViewWheel		= vec3(0.0f);		// View on the bridge - center
	vec3		ViewLeft		= vec3(0.0f);		// View on the bridge - left
	vec3		ViewRight		= vec3(0.0f);		// View on the bridge - right
	vec3		ViewBow			= vec3(0.0f);		// View at the bow
	vec3		ViewStern		= vec3(0.0f);		// View at the stern

	// Dimensions
	eClass		Class			= eClass::Corvette;	// Class of ship that defines certain parameters
	float		Length			= 10.0f;			// Final length
	float		SpeedMaxKn		= 15.0f;			// Speed max after sea trials
	float		SpeedTestKn		= 14.0f;			// Speed for sea trials at notch 7/10
	float		Mass_t			= 1.0f;				// Tons
	vec3		PosGravity		= vec3(0.0f);		// Offset of the center of gravity (relative to Position)
	float		HeaveCoef		= 1.0f;				// Performance of heave (damping)
	float		FormFactor		= 0.15f;			// Form factor according to ITTC 1957
	float		EnvMapFactor	= 0.0f;				// Factor of environment reflexion (between 0.0 and 1.0)
	float		AreaFront		= 0.0f;
	vec3		AreaFrontCenter = vec3(0.0f);
	float		AreaLat			= 0.0f;
	vec3		AreaLatCenter	= vec3(0.0f);

	// Spray
	float		SprayVerticalPerf = 10.0f;			// Vertical spray performance
	int			SprayMultiplier = 1;				// Number of points between points of contour
	float		SprayLength		= 0.1f;				// % of length of the ship taken on the contour
	int			SprayType		= 0;				// 0 = sharp (like a frigate), 1 = rounded (like a cargo)

	// Rudder
	vec3		PosRudder		= vec3(0.0f);		// Offset of the center of the rudder (relative to Position)
	float		RudderIncrement = 1.0f;				// Degrees
	int			RudderStepMax	= 35;				// Number of increments
	float		RudderRotSpeed	= 10.0f;			// Degrees / sec
	int			nRudder			= 2;				// Number of rudders
	vec3		PosRudder1		= vec3(0.0f);		// Left
	vec3		PosRudder2		= vec3(0.0f);		// Right
	
	// Turning
	float		TurningPerf		= 10.0f;			// Performance of efficiency of the rudder
	float		TurningDragCoef = 4.0f;				// Performance of efficiency of the counter drift
	float		RoTMax			= 120.0f;			// Maximum rate of turn (°/min)
	float		HighSpeedCoeff	= 0.1f;				// Coefficient applied to the rate of turn at SpeedTestKn
	float		PivotFwd	= 0.2f;					// Pivot point in forward motion (from Bow to length (stern))
	float		PivotBwd	= 0.7f;					// Pivot point in backward motion (from Bow to length (stern))
	float		CentrifugalPerf = 10.0f;			// Performance of the centrifugal force

	// Power
	vec3		PosPower			= vec3(0.0f);	// Offset of the center of the propeller where the power is applied (relative to Position)
	float		PowerkW				= 1000.0f;		// kiloWatts
	int			PowerStepMax		= 10;			// Number of steps on the throttle lever

	// Propellers
	float		PropRpmMax			= 200.0f;		// Maximum RPM of the propeller
	float		PropRpmIncrement	= 20.f;			// Rate of increase/decrease RPM of the propeller
	int			nPropeller			= 2;			// Number of propellers
	vec3		PosPropeller1		= vec3(0.0f);	// Left propeller
	float		PropTorque1			= 0.0f;			// +1.0 for right propeller, -1.0 otherwise
	vec3		PosPropeller2		= vec3(0.0f);	// Right propeller
	float		PropTorque2			= 0.0f;			// +1.0 for right propeller, -1.0 otherwise
	float		PropDiameter		= 3.0f;			// Diameter of the propeller
	float		WakeWidth			= 1.0f;			// Real wake width is mWidth x WakeWidth
	
	// Chimneys
	int			nChimney = 2;			// Number of chimneys
	vec3		PosChimney1 = vec3(0.0f);	// Left chimney
	vec3		PosChimney2 = vec3(0.0f);	// Right chimney

	// Bow Thruster
	bool		HasBowThruster		= true;
	vec3		PosBowThruster		= vec3(0.0f);	// Offset of the center of the center of the bow thruster (relative to Position)
	float		BowThrusterPerf		= 0.4f;			// Performance of efficiency of the system Engine - Propeller
	float		BowThrusterPowerW	= 10000.0f;		// Watts
	int			BowThrusterStepMax	= 5;			// Number of steps on the throttle lever
	float		BowThrusterRpmMin	= 0.0f;			// Minimum RPM of the propeller
	float		BowThrusterRpmMax	= 500.0f;		// Maximum RPM of the propeller
	float		BowThrusterRpmIncrement = 10.f;		// Rate of increase/decrease RPM of the propeller

	// Stern Thruster
	bool		HasSternThruster	= true;
	vec3		PosSternThruster	= vec3(0.0f);	// Offset of the center of the center of the stern thruster (relative to Position)
	float		SternThrusterPerf	= 0.4f;			// Performance of efficiency of the system Engine - Propeller
	float		SternThrusterPowerW	= 10000.0f;		// Watts
	int			SternThrusterStepMax= 5;			// Number of steps on the throttle lever
	float		SternThrusterRpmMin	= 0.0f;			// Minimum RPM of the propeller
	float		SternThrusterRpmMax	= 500.0f;		// Maximum RPM of the propeller
	float		SternThrusterRpmIncrement = 10.f;	// Rate of increase/decrease RPM of the propeller

	// Lights
	vector<vec3>LightPositions;						// Offset of the center of the lights (relative to Position)
	vector<vec3>LightColors;						// Color the lights

	// Radar
	int			nRadar			= 0;
	vec3		PosRadar1		= vec3(0.0f);
	float		RotationRadar1	= 40.0f;			// Tours/min
	vec3		PosRadar2		= vec3(0.0f);
	float		RotationRadar2	= 40.0f;			// Tours/min

	// Autopilot
	float		BaseP			= 4.0f;				// Make the turn
	float		BaseI			= 2.0f;				// Correct a constant deviation
	float		BaseD			= 4.0f;				// Anticipate the end of the turn
	float		MaxIntegral		= 5.0f;				// Limit of the integral to avoid runaway
	float		MinSpeed		= 1.0f;				// Minimum speed to avoid division by zero
	float		LowSpeedBoost	= 2.0f;				// Low speed amplification factor
	float		HighSpeedLimit	= 5.0f;				// Speed beyond which the rudder angle is progressively reduced
	float		DynamicFactor = 5.0f;				// Factor to adjust the influence of speed
	float		SeaSateFactor	= 1.0f;				// Increases responsiveness in difficult conditions (Pitch and Roll)

	// Waves
	float		CenterFore		= 0.0f;				// Reference X for the position of the kelvin texture
	int			BaseFroude		= 0;				// Normally = 0, meaning that the real froude scheme is take, otherwise, take a higher Froude scheme

	// Flag
	bool		bFlag			= true;
	vec3		PosFlag			= vec3(0.0f);
	float		DimXFlag		= 1.0f;
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
struct sLine
{
	vec2 p1;
	vec2 p2;
};