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
#include "Clouds.h"

#pragma warning( push )
#pragma warning( disable : 4244 ) // conversion de 'x' en 'y', perte possible de données
#pragma warning( disable : 6011 ) // référencement du pointeur

using namespace std;
using namespace glm;
namespace fs = std::filesystem;


GLFWwindow        * g_hWindow		= nullptr;
HWND				g_hWnd			= nullptr;
uint32_t			g_WindowW		= 1600;
uint32_t			g_WindowH		= 1000;
uint32_t			g_WindowW_2		= 800;
uint32_t			g_WindowH_2		= 500;
uint32_t			g_WindowX		= 0;
uint32_t			g_WindowY		= 0;
bool                g_IsFullScreen	= false;
wstring				g_DirExecutable;
HANDLE				g_hConsole;					// Handle of the console (can be shown and can be hidden)
NVGcontext        * g_Nvg			= nullptr;
float				g_DevicePixelRatio = 1.0f;

// ImGui windows
bool                g_bShowSceneWindow			= true;		// [ F1 ]
bool                g_bShowShipWindow			= true;     // [ F2 ]
bool                g_bShowStatusBar			= false;	// [ F3 ]
bool                g_bShowOceanAnalysisWindow	= false;
bool                g_bShowSphereMeasure		= false;
bool                g_bShowTerrainWindow		= false;
bool                g_bShowAutopilotWindow		= false;

SoundManager* SoundManager::instance	= nullptr; // Initialisation du pointeur statique (sinon, à placer dans un fichier Sound.cpp)
SoundManager      * g_SoundMgr			= nullptr;
unique_ptr<Sound>	g_SoundSeagull[8];
unique_ptr<Sound>	g_SoundHorn;
bool                g_bSoundSeagull		= false;

// POSITION //////////////////////////////////////
vec2                g_InitialPosition	= vec2(-5.09732676, 48.47529221);
vector<sPositions>	g_vPositions;					// List of positions to be used by the ship
int					g_NoPosition		= 0;		// The number of the position in the list		
Camera				g_Camera;
bool				g_bZoomCamera		= false;

// TIME //////////////////////////////////////////
LARGE_INTEGER		g_Frequency;
LARGE_INTEGER		g_LastTime, g_CurrentTime;
int					g_FrameCount		= 0;
int					g_Fps				= 0;
eh::Timer			g_Timer;
float				g_TimeSpeed			= 1.0f;
bool				g_bPause			= false;
bool				g_bVsync			= false;

// SHADERS //////////////////////////////////////////
unique_ptr<Shader>  g_ShaderSun;
unique_ptr<Shader>	g_ShaderCamera;
unique_ptr<Shader>  g_ShaderPostProcessing;
unique_ptr<Shader>  g_ShaderShadow;

// FRAMEBUFFERS ////////////////////////////////////
GLuint              FBO_REFLECTION		= 0;        // Used only for the ship reflection 
GLuint              TexReflectionColor	= 0;		// Used by the ocean rendering
GLuint              TexReflectionDepth	= 0;		// To be deleted?

GLuint              msFBO_SCENE			= 0;        // Used to Draw the whole scene (including the ship reflection) - Multisample
GLuint              msTexSceneColor		= 0;        // Multisample
GLuint				msTexSceneDepth		= 0;        // Multisample

GLuint              FBO_SCENE			= 0;        // Used to Draw the postprocessing (scene + blur + fog + underwater)
GLuint              TexSceneColor		= 0;
GLuint              TexSceneDepth		= 0;

GLuint				FBO_SHADOWMAP		= 0;		// Shadow map
GLuint				TexShadowMapDepth	= 0;
int					shadowWidth			= 1024;
int					shadowHeight		= 1024;
mat4				lightSpaceMatrix;

unique_ptr<ScreenQuad> g_ScreenQuadPost;

// OBJECTS ///////////////////////////////////////
bool				g_bWireframe		= false;   // For the entire scene
unique_ptr<Grid>    g_Grid;
bool				g_bGridVisible		= false;
unique_ptr<Cube>    g_Axe;
unique_ptr<Sphere>  g_FloatingBall;
unique_ptr<Cube>    g_MobileWall;
unique_ptr<Sphere>  g_SphereMeasure;
vec3                g_SphereMeasurePos	= vec3(0.0f);
unique_ptr<Model>   g_ArrowWind;
unique_ptr<Model>   g_Pier;

// WIND //////////////////////////////////////////
vec2				g_Wind				= vec2(0.0f);
float				g_WindDirectionDEG	= 0.0f;
float				g_WindSpeedKN		= 1.0f;

// SHIP //////////////////////////////////////////
unique_ptr<Ship>    g_Ship;						// The ship selected
vector<sShip>       g_vShips;					// The list of ships
int					g_NoShip			= 4;    // The number of the ship in the list
bool                g_bShipWake			= true; // Display the wake (texture around the ship)
int                 g_LowMass			= 0;	// Half of the mass (for ImGui selection purpose)
int					g_HighMass			= 0;	// Double of the mass (for ImGui selection purpose)

vec4                g_CtrlThrottle		= vec4(0.0f);
float               g_CtrlThrottleHigh	= 0.0f;
float				g_CtrlThrottleLow	= 0.0f;
vec4                g_CtrlRudder		= vec4(0.0f);
float               g_CtrlRudderLeft	= 0.0f;
float				g_CtrlRudderRight	= 0.0f;
vec4                g_CtrlAutopilotCMD	= vec4(0.0f);
vec4                g_CtrlAutopilotM1	= vec4(0.0f);
vec4                g_CtrlAutopilotM10	= vec4(0.0f);
vec4                g_CtrlAutopilotP1	= vec4(0.0f);
vec4                g_CtrlAutopilotP10	= vec4(0.0f);
vec4                g_CtrlAutopilotDynAdjust = vec4(0.0f);

// SKY ///////////////////////////////////////////
unique_ptr<Sky>    g_Sky;
unique_ptr<ScreenQuad> g_ScreenQuadCloud;   // Used with the background shader to Draw the clouds
unique_ptr<VolumetricClouds> g_Clouds;
unique_ptr<Shader>  g_ShaderBackground;
bool				g_bSkyVisible			= true;

// OCEANS ////////////////////////////////////////
unique_ptr<Ocean>   g_Ocean;
bool				g_bOceanVisible			= true;
bool				g_bOceanWireframe		= false;
bool				g_bTextureDisplay		= false;	// Display the 2D textures Displacement, Gradients, FoamBuffer
bool				g_bTextureWakeDisplay	= false;	// Display the 2D texture Trail of the ship
bool                g_bShowOceanCut;					// Display a cut of a patch of ocean
unique_ptr<QuadTexture> g_QuadTexture;					// For the rendering of the 2D textures on the screen (debug)

// TERRAIN ///////////////////////////////////////
bool                g_bShowTerrain			= false;
vector<sTerrain>    g_vTerrains;
unique_ptr<Markup>  g_Markup;

//////////////////////////////////////////////////

void	InitScene();
void    InitFBO();
void	InitImGUI();
void	InitNanoVg();
void	InitFPSCounter();
void	LoadModels();
void    LoadTerrains();
void    LoadShips();
void    LoadSounds();
void    UpdateSounds();
void	UpdateFPS();
void	Render();
