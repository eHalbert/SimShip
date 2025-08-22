/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely used Windows headers
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <CommDlg.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>		// ofstream
#include <filesystem>
#include <iomanip>
#include <limits>
#include <corecrt_io.h>	// Console
#include <fcntl.h>		// Console
#include <vector>
#include <string>
#define _USE_MATH_DEFINES
#include <math.h>

#include <glad/glad.h>
#include <glfw/glfw3.h>
// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace std;
using namespace glm;

#define opengl_check {check_opengl_error(__FILE__, __func__, __LINE__);}


void	InitConsole();
void	ConsoleClose();
void	ConsoleClear();
void	PrintGlmMatrix(mat4& mat, string name = "");
void	PrintGlmVec3(vec3 vec, string name);
void	PrintGlmVec3(vec3 vec);
void	SetVsync(int interval);
void	GetLimitsGPU();

string	opengl_info_display(); 
void	gl_check_error();
void	check_opengl_error(string const& file, string const& function, int line);

wstring GetExecutablePath();
string	ReplaceBackSlash(string& chaine);
string	RemoveExtensionAndPath(const string pathname);

uint32_t Log2OfPow2(uint32_t x);

float	MsToKnots(float speedMS);
float	KnotsToMS(float speedKnots);
float	WindVec_Dir(vec2 windVector);
vec2	WindDirSpeed_Vec(float directionDEG, float speedKN);

quat	RotationBetweenVectors(vec3 A, vec3 B);
float	Sign(float value);
double	InterpolateAValue(const double start_1, const double end_1, const double start_2, const double end_2, double value_between_start_1_and_end_1);
bool	IsInRect(vec4& rect, vec2& point);

inline int MSToBeaufort(double v)
{
	int bf = (int)pow(v * v * 1.44, 0.33333);
	if (bf > 12)
		bf = 12;
	return bf;
}
template <class T> inline T WrapDEG(T angle)
{
	while (angle >= 360) angle -= (T)360;
	while (angle < -0) angle += (T)360;
	return angle;
}
template <class T> inline T	DifferenceDEG(T angle1, T angle2)
{
	angle1 = WrapDEG(angle1);
	angle2 = WrapDEG(angle2);
	if (abs(angle1 - angle2) >= 180.0)
		return WrapDEG(abs(360.0 - abs(angle1 - angle2)));
	else
		return WrapDEG(abs(angle1 - angle2));
}
template <class T> inline T MinDEG(T angle1, T angle2)
{
	angle1 = WrapDEG(angle1);
	angle2 = WrapDEG(angle2);
	if (abs(angle1 - angle2) >= 180.0)
		return max(angle1, angle2);
	else
		return min(angle1, angle2);
}
template <class T> inline T MaxDEG(T angle1, T angle2)
{
	angle1 = WrapDEG(angle1);
	angle2 = WrapDEG(angle2);
	if (abs(angle1 - angle2) >= 180.0)
		return min(angle1, angle2);
	else
		return max(angle1, angle2);
}
template <class T> inline int AreAnglesSorted(T angle1, T angle2)
{
	// return 
	// -1 : false (angle1 is superior to angle2)
	//  0 : anges are equal
	// +1 : angles are in the right order

	angle1 = WrapDEG(angle1);
	angle2 = WrapDEG(angle2);
	if (angle1 == angle2)
		return 0;

	T diff = DifferenceDEG(angle1, angle2);
	if (WrapDEG(angle1 + diff) == angle2)
		return 1;
	else
		return -1;
}

vec2 LonLatToOpenGL(float lon, float lat);
vec2 OpenGLToLonLat(float x, float z);

bool InterpolateTriangle(const vec3& p1, const vec3& p2, const vec3& p3, vec3& pos);
vector<string> ListFiles(const string& folder, const string& ext);

vec3 ConvertToFloat(vec3 v);
void RGBtoHSL(const vec3& rgb, float& h, float& s, float& l);
wstring SaveClientArea(HWND hwnd);