/* SimSail by Edouard Halbert
http://www.simsail.net

This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/
*/
#pragma once

#include <string>
#include <Windows.h>
#include <sstream>
#include <vector>

#include <glm/glm.hpp>

using namespace std;
using namespace glm;

class Ini
{
public:

	Ini(void);
	~Ini(void);

	bool Load(wchar_t * file);

	// Get
	wstring			GetString(const wchar_t * section, const wchar_t * key, wchar_t * defaultValue);
	wchar_t *		GetChars(const wchar_t * section, const wchar_t * key, wchar_t * defaultValue);
	int				GetInt(const wchar_t* section, const wchar_t* key, int defaultValue);
	float			GetFloat(const wchar_t* section, const wchar_t* key, float defaultValue);
	double			GetDouble(const wchar_t* section, const wchar_t* key, double defaultValue);
	bool			GetBoolean(const wchar_t* section, const wchar_t* key, bool defaultValue);
	unsigned long	GetColor(const wchar_t* section, const wchar_t* key, unsigned long defaultValue);
	vec3			GetVec3(const wchar_t* section, const wchar_t* key, const vec3& defaultValue);
	dvec3			GetdVec3(const wchar_t* section, const wchar_t* key, const dvec3& defaultValue);
	vector<vec3>	GetVec3Array(const wchar_t* section, const wchar_t* key, const vector<vec3>& defaultValue);
	vector<dvec3>	GetdVec3Array(const wchar_t* section, const wchar_t* key, const vector<dvec3>& defaultValue);

	// Set
	bool			SetString(const wchar_t* section, const wchar_t* key, wchar_t * value);
	bool			SetString(const wchar_t * section, const wchar_t * key, wstring value);
	bool			SetInt(const wchar_t* section, const wchar_t* key, int value);
	bool			SetFloat(const wchar_t* section, const wchar_t* key, float value);
	bool			SetDouble(const wchar_t* section, const wchar_t* key, double value);
	bool			SetBoolean(const wchar_t* section, const wchar_t* key, bool value);
	bool			SetColor(const wchar_t* section, const wchar_t* key, unsigned long value);
	bool			SetVec3(const wchar_t* section, const wchar_t* key, const vec3& value);
	bool			SetdVec3(const wchar_t* section, const wchar_t* key, const dvec3& value);
	bool			SetVec3Array(const wchar_t* section, const wchar_t* key, const vector<vec3>& vecArray);
	bool			SetdVec3Array(const wchar_t* section, const wchar_t* key, const vector<dvec3>& vecArray);

private:

	wchar_t m_Pathname[256];
};

