/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclure les en-têtes Windows rarement utilisés
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <CommDlg.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <corecrt_io.h>	// Console
#include <fcntl.h>		// Console
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include <wchar.h>
#include <random>

// glad
#include <glad/glad.h>
#include <glad/glad_wgl.h>
#pragma comment(lib, "OpenGL32.lib")
// glfw
#include <glfw/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_EXPOSE_NATIVE_WIN32 
#include <glfw/glfw3native.h>
// stb_image
#include "stb_image.h"
#include "stb_image_write.h"
// nanovg
#include "nanovg.h"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"
// imgui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Structures.h"
#include "Utility.h"
#include "Camera.h"
#include "Sky.h"
#include "Shader.h"
#include "Shapes.h"
#include "Model.h"
#include "Ocean.h"
#include "Timer.h"
#include "Spectra.h"
#include "Texture.h"
#include "Ship.h"
#include "Markup.h"
#include "Lighthouse.h"
#include "Clouds.h"
#include "Ini.h"
#include "UdpSender.h"
#include "Traffic.h"

#pragma warning( push )
#pragma warning( disable : 4244 ) // conversion de 'x' en 'y', perte possible de données
#pragma warning( disable : 6011 ) // référencement du pointeur

using namespace std;
using namespace glm;
namespace fs = std::filesystem;

GLFWwindow        * g_hWindow			= nullptr;
HWND				g_hWnd				= nullptr;
uint32_t			g_WindowW			= 1920;
uint32_t			g_WindowH			= 1080;
uint32_t			g_WindowW_2			= 960;
uint32_t			g_WindowH_2			= 540;
uint32_t			g_WindowX			= 0;
uint32_t			g_WindowY			= 0;
bool                g_IsFullScreen		= false;
HANDLE				g_hConsole;					// Handle of the console (can be shown and can be hidden)
wstring				g_CaptureName;
eh::Timer			g_Chrono;
int					g_ChronoStep = -1;
int g_OldWindowX, g_OldWindowY, g_OldWindowW, g_OldWindowH, g_OldClientX = 0, g_OldClientY, g_OldClientW, g_OldClientH;


// NANOVG //////////////////////////////////////////
NVGcontext        * g_Nvg				= nullptr;
int					g_NvgImgClock		= 0;
int					g_NvgImgSoundOff	= 0;
int					g_NvgImgSoundUp		= 0;
int					g_NvgImgTimer		= 0;
int					g_NvgImgMto1		= 0;
int					g_NvgImgMto2		= 0;
int					g_NvgImgMto3		= 0;
float				g_DevicePixelRatio	= 1.0f;

// ImGui windows
bool                g_bShowSceneWindow			= false;	// [ F2 ]
bool                g_bShowShipWindow			= false;    // [ F3 ]
bool                g_bShowStatusBar			= false;	// [ F4 ]
bool                g_bShowAutopilotWindow		= false;	// [ F5 ]
bool                g_bShowOceanAnalysisWindow	= false;
bool				g_bShowShortcuts			= false;
bool                g_bShowShipForcesWindows	= false;	// [ F6 ]

// SOUNDS ////////////////////////////////////////
SoundManager* SoundManager::instance	= nullptr; // Initialisation du pointeur statique (sinon, à placer dans un fichier Sound.cpp)
SoundManager      * g_SoundMgr			= nullptr;
unique_ptr<Sound>	g_SoundSeagull[8];
unique_ptr<Sound>	g_SoundHorn;
bool                g_bSoundSeagull		= false;
unique_ptr<Sound>	g_SoundRain;
bool				g_bSoundRain = false;

// VIEWS //////////////////////////////////////////
vec2                g_InitialPosition	= vec2(-2.94097114, 47.38162231);
vector<sPositions>	g_vPositions;					// List of positions to be used by the ship
int					g_NoPosition		= 0;		// The number of the position in the list		
Camera				g_Camera;
eBridgeView			g_eBridgeView		= eBridgeView::WHEEL;	// Index of the view (1,2,3) for the BRIDGE mode of the camera
bool				g_bBinoculars		= false;
bool				g_bLowIntensity		= false;
bool				g_bNightVision		= false;

// TIME ////////////////////////////////////////////
LARGE_INTEGER		g_Frequency;
LARGE_INTEGER		g_LastTime, g_CurrentTime;
int					g_FrameCount		= 0;
int					g_Fps				= 0;
eh::Timer			g_TimerShipMotion;
bool				g_bChrono			= false;
float				g_TimeSpeed			= 1.0f;
bool				g_bPause			= false;
bool				g_bVsync			= false;

// SHADERS ///////////////////////////////////////////
unique_ptr<Shader>  g_ShaderSun;
unique_ptr<Shader>	g_ShaderCamera;
unique_ptr<Shader>  g_ShaderPostProcessing;
unique_ptr<Shader>  g_ShaderRain;
unique_ptr<Shader>  g_ShaderFXAA;
unique_ptr<Shader>  g_ShaderRainVolume;

// FRAMEBUFFERS //////////////////////////////////////
GLuint              FBO_REFLECTION		= 0;        // Used only for the ship reflection 
GLuint              TexReflectionColor	= 0;		// Used by the ocean rendering
GLuint              TexReflectionDepth	= 0;		// To be deleted?

GLuint              msFBO_SCENE			= 0;        // Used to Draw the whole scene (including the ship reflection) - Multisample
GLuint              msTexSceneColor		= 0;        // Multisample
GLuint				msTexSceneDepth		= 0;        // Multisample

GLuint              FBO_SCENE			= 0;        // Used to blit multisample for postprocessing
GLuint              TexSceneColor		= 0;		// The scene on a texture
GLuint              TexSceneDepth		= 0;

GLuint              FBO_POST			= 0;		// Used to Draw the postprocessing (in: TexSceneColor, TexSceneDepth)
GLuint              TexPostColor		= 0;
GLuint              TexPostDepth		= 0;

GLuint              FBO_RAIN			= 0;		// Used to Draw the rain and/or the binoculars (in: TexPostColor)
GLuint              TexRainColor		= 0;
GLuint              TexRainDepth		= 0;

unique_ptr<ScreenQuad> g_ScreenQuadPost;
unique_ptr<ScreenQuad> g_ScreenQuadRainVolume;

// For Rain volume effect
GLuint				FBO_OCEAN_TEMP		= 0;
GLuint				TexOceanTempColor	= 0;
GLuint				TexOceanTempDepth	= 0;

// OBJECTS //////////////////////////////////////////
bool				g_bWireframe		= false;   // For the entire scene
unique_ptr<Grid>    g_Grid;
bool				g_bGridVisible		= false;
unique_ptr<Cube>    g_Axe;
unique_ptr<Sphere>  g_StaticBall;
unique_ptr<Model>   g_ArrowWind;

// WIND /////////////////////////////////////////////
vec2				g_Wind				= vec2(0.0f);
float				g_TWS_Deg			= 0.0f;
float				g_TWS_Kn			= 1.0f;

// SHIP /////////////////////////////////////////////
unique_ptr<Ship>    g_Ship;						// The ship selected
vector<sShip>       g_vShips;					// The list of ships
int					g_NoShip = 0;				// The index in the list of the ships
unique_ptr<UdpSender> g_UdpSender;				// Class to send the NMEA sentences
bool                g_bShipWake			= true; // Display the wake (texture around the ship)
int                 g_LowMass			= 0;	// Half of the mass (for ImGui selection purpose)
int					g_HighMass			= 0;	// Double of the mass (for ImGui selection purpose)
bool				g_bReset			= true;	// Inhibit motion during a short period

// INTERFACE ////////////////////////////////////////
vec4                g_CtrlPanel			= vec4(0.0f);
vec4                g_CtrlThrottle1		= vec4(0.0f);
float               g_CtrlThrottleHigh1	= 0.0f;
float				g_CtrlThrottleLow1	= 0.0f;
vec4                g_CtrlThrottle2		= vec4(0.0f);
float               g_CtrlThrottleHigh2 = 0.0f;
float				g_CtrlThrottleLow2	= 0.0f;
vec4                g_CtrlRudder		= vec4(0.0f);
float               g_CtrlRudderLeft	= 0.0f;
float				g_CtrlRudderRight	= 0.0f;
vec4                g_CtrlBowThruster = vec4(0.0f);
float               g_CtrlBowThrusterLeft = 0.0f;
float				g_CtrlBowThrusterRight = 0.0f;
vec4                g_CtrlSternThruster = vec4(0.0f);
float               g_CtrlSternThrusterLeft = 0.0f;
float				g_CtrlSternThrusterRight = 0.0f;

vec4                g_CtrlAutopilotCMD	= vec4(0.0f);
vec4                g_CtrlAutopilotM1	= vec4(0.0f);
vec4                g_CtrlAutopilotM10	= vec4(0.0f);
vec4                g_CtrlAutopilotP1	= vec4(0.0f);
vec4                g_CtrlAutopilotP10	= vec4(0.0f);
vec4                g_CtrlTimeHour		= vec4(0.0f);
vec4				g_CtrlTimeMinute	= vec4(0.0f);
vec4                g_CtrlWind			= vec4(0.0f);
vec4                g_CtrlNow			= vec4(0.0f);
vec4                g_CtrlSound			= vec4(0.0f);
vec4                g_CtrlTimer			= vec4(0.0f);
vec4                g_CtrlMto1			= vec4(0.0f);
vec4                g_CtrlMto2			= vec4(0.0f);
vec4                g_CtrlMto3			= vec4(0.0f);

// SKY /////////////////////////////////////////////
unique_ptr<Sky>		g_Sky;
bool				g_bSkyVisible = true;
unique_ptr<ScreenQuad> g_ScreenQuadCloud;				// Used with the background shader to draw the clouds
unique_ptr<Clouds>	g_Clouds;
unique_ptr<Shader>  g_ShaderBackground;

// OCEANS //////////////////////////////////////////
unique_ptr<Ocean>   g_Ocean;
bool				g_bOceanVisible			= true;
bool				g_bOceanWireframe		= false;
bool				g_bTextureDisplay		= false;	// Display the 2D textures Displacement, Gradients, FoamBuffer
bool				g_bTextureWakeDisplay	= false;	// Display the 2D texture Trail of the ship
bool                g_bShowOceanCut;					// Display a cut of a patch of ocean
unique_ptr<QuadTexture> g_QuadTexture;					// For the rendering of the 2D textures on the screen (debug)
bool				g_bShipShadow			= true;

// TERRAIN //////////////////////////////////////////
bool                g_bShowTerrain			= true;
vector<sTerrain>    g_vTerrains;
int					g_idxHouat = 0;
unique_ptr<Model>   g_Port;
vector<sLine>		g_vPortLines;
unique_ptr<Markup>  g_Markup;
unique_ptr<LighthouseMgr> g_Lighthouses;

// TRAFFIC //////////////////////////////////////////
unique_ptr<Traffics> g_Traffics;

/////////////////////////////////////////////////////
NVGcolor			g_ColorBlack	= nvgRGB(0, 0, 0);
NVGcolor			g_ColorGray192	= nvgRGB(192, 192, 192);
NVGcolor			g_ColorWhite	= nvgRGB(255, 255, 255);

NVGcolor			g_ColorRed		= nvgRGB(200, 0, 0);
NVGcolor			g_ColorGreen	= nvgRGB(0, 200, 0);
NVGcolor			g_ColorBlue		= nvgRGB(0, 0, 200);
NVGcolor			g_ColorAmbre	= nvgRGB(255, 165, 0);
NVGcolor			g_ColorCyan		= nvgRGB(0, 255, 255);
NVGcolor			g_ColorMagenta	= nvgRGB(255, 0, 255);
NVGcolor			g_ColorYellow	= nvgRGB(255, 255, 0);

NVGcolor			g_ColorSimShip1 = nvgRGB(37, 87, 131);
NVGcolor			g_ColorSimShip2 = nvgRGB(44, 129, 169);
NVGcolor			g_ColorSimShip3 = nvgRGB(118, 150, 175);
NVGcolor			g_ColorSimShip4 = nvgRGB(184, 199, 208);

/////////////////////////////////////////////////////
vector<pair<string, string>> vShortcuts = {
	{ "C", "Orbital camera" },
	{ "B", "Bridge camera" },
	{ "WSADX", "Bridge views" },
	{ "F", "FPS camera" },
	{ "W", "Forward" },
	{ "S", "Back" },
	{ "A", "Left" },
	{ "D", "Right" },
	{ "Q", "Up" },
	{ "E", "Down" },
	{ "I", "Interpolation of camera" },
	{ "R click", "Binoculars" },
	{ "N", "Night vision" },
	{ "T", "Textures 1 debug" },
	{ "I", "Textures 2 debug" },
	{ "Space", "Pause" },
	{ "Esc", "Quit" },
	{ "F2", "Scene settings" },
	{ "F3", "Ship settings" },
	{ "F4", "Status bar" },
	{ "F5", "Autopilot settings" },
	{ "F6", "Ship Forces" },
	{ "F8", "Window capture" },
	{ "F11", "Full screen" },
	{ "1 ... 0", "Select ship #" },
	{ "NUM 7", "Engine Left +" },
	{ "NUM 4", "Engine Left stop" },
	{ "NUM 1", "Engine Left -" },
	{ "NUM 8", "Both engines +" },
	{ "NUM 5", "Both engines stop" },
	{ "NUM 2", "Both engines -" },
	{ "NUM 9", "Engine Right +" },
	{ "NUM 6", "Engine Right stop" },
	{ "NUM 6", "Engine Right -" },
	{ "Ins", "Bow Thruster left" },
	{ "Home", "Bow Thruster stop" },
	{ "Page up", "Bow Thruster right" },
	{ "Del", "Stern Thruster left" },
	{ "End", "Stern Thruster stop" },
	{ "Page dn", "Stern Thruster right" },
	{ "Left", "Rudder left" },
	{ "Down", "Rudder stop" },
	{ "Right", "Rudder right" },
	{ "L", "Ship lights" },
	{ "H", "Ship horn" },
	{ "NUM /", "Both engine 7/10" },
	{ "NUM +", "Increase speed" },
	{ "NUM -", "Decrease speed" },
};

void	InitScene();
void    InitFBO();
void	InitImGUI();
void	InitNanoVg();
void	InitFPSCounter();
int		LoadNvgIcon(string name);
void	LoadPositions();
void	SetPosition();
void	LoadModels();
void    LoadTerrains();
void	LoadPortContour();
bool	CheckCrossingPort();
void    LoadShips();
void	LoadShips();
void	SetShip(int n);
void	SetMeteo(int n);
void    LoadSounds();
void    UpdateSounds();
void	UpdateFPS();
void	SendNMEA(float time);
void	RenderSplashScreen(string subtitle);
void	Render();
