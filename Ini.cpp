/* SimSail by Edouard Halbert
http://www.simsail.net

This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/
*/

#include "Ini.h"

#include <Shlwapi.h>

Ini::Ini(void)
{
}
bool Ini::Load(wchar_t * file)
{
	wcscpy_s(m_Pathname, file);

	FILE * fp = _wfopen( m_Pathname, L"rb" );
	if( fp != NULL )
	{
		fclose( fp );
		return true;
	}
	return false;
}
Ini::~Ini(void)
{
}

wstring Ini::GetString(const wchar_t* section, const wchar_t* key, wchar_t * defaultValue)
{
	wchar_t value[256];

	if(GetPrivateProfileString(section, key, defaultValue, value, sizeof value, m_Pathname) != 0)
		return value;

	return defaultValue;
}
wchar_t * Ini::GetChars(const wchar_t* section, const wchar_t* key, wchar_t * defaultValue)
{
	static wchar_t value[256];

	if(GetPrivateProfileString(section, key, defaultValue, value, sizeof value, m_Pathname) != 0)
		return value;

	return defaultValue;
}
int Ini::GetInt(const wchar_t* section, const wchar_t* key, int defaultValue)
{
	return GetPrivateProfileInt(section, key, defaultValue, m_Pathname);
}
float Ini::GetFloat(const wchar_t* section, const wchar_t* key, float defaultValue)
{
	wchar_t value[256];

	if(GetPrivateProfileString(section, key, L"", value, sizeof value, m_Pathname) != 0)
		return (float)_wtof(value);

	return defaultValue;
}
double Ini::GetDouble(const wchar_t* section, const wchar_t* key, double defaultValue)
{
	wchar_t value[256];

	if(GetPrivateProfileString(section, key, L"", value, sizeof value, m_Pathname) != 0)
		return _wtof(value);

	return defaultValue;
}
bool Ini::GetBoolean(const wchar_t* section, const wchar_t* key, bool defaultValue)
{
	wchar_t value[6];

	if(GetPrivateProfileString(section, key, L"", value, sizeof value, m_Pathname) != 0)
	{
		if (value[0]=='y' || value[0]=='Y' || value[0]=='1' || value[0]=='t' || value[0]=='T') 
			return true;
		else 
			return false;
	}
    return defaultValue;
}
unsigned long Ini::GetColor(const wchar_t* section, const wchar_t* key, unsigned long defaultValue)
{
	wchar_t value[256];

	if(GetPrivateProfileString(section, key, L"", value, sizeof value, m_Pathname) != 0)
	{
		wchar_t * pch;
		wchar_t* context = nullptr;
		pch = wcstok(value, L" ", &context);
		int val[3];
		val[0] = (int)wcstol(pch, NULL, 0);
		pch = wcstok(NULL, L" ", &context);
		val[1] = (int)wcstol(pch, NULL, 0);
		pch = wcstok(NULL, L" ", &context);
		val[2] = (int)wcstol(pch, NULL, 0);

		return unsigned long (((0xff)&0xff)<<24)|(((val[0])&0xff)<<16)|(((val[1])&0xff)<<8)|((val[2])&0xff);
	}
	return defaultValue;
}
vec3 Ini::GetVec3(const wchar_t* section, const wchar_t* key, const vec3& defaultValue)
{
	wchar_t value[256];
	if (GetPrivateProfileString(section, key, L"", value, sizeof value / sizeof(wchar_t), m_Pathname) != 0)
	{
		wchar_t* pch;
		wchar_t* context = nullptr;
		pch = wcstok(value, L" ", &context);
		float val[3] = { defaultValue.x, defaultValue.y, defaultValue.z }; // Valeur par défaut
		int i = 0;
		while (pch != nullptr && i < 3)
		{
			val[i] = static_cast<float>(wcstod(pch, nullptr));
			pch = wcstok(nullptr, L" ", &context);
			++i;
		}
		return glm::vec3(val[0], val[1], val[2]);
	}
	return defaultValue;
}
dvec3 Ini::GetdVec3(const wchar_t* section, const wchar_t* key, const dvec3& defaultValue)
{
	wchar_t value[256];
	if (GetPrivateProfileString(section, key, L"", value, sizeof value / sizeof(wchar_t), m_Pathname) != 0)
	{
		wchar_t* pch;
		wchar_t* context = nullptr;
		pch = wcstok(value, L" ", &context);
		double val[3] = { defaultValue.x, defaultValue.y, defaultValue.z }; // Valeur par défaut
		int i = 0;
		while (pch != nullptr && i < 3)
		{
			val[i] = static_cast<double>(wcstod(pch, nullptr));
			pch = wcstok(nullptr, L" ", &context);
			++i;
		}
		return glm::dvec3(val[0], val[1], val[2]);
	}
	return defaultValue;
}
vector<vec3> Ini::GetVec3Array(const wchar_t* section, const wchar_t* key, const vector<vec3>& defaultValue)
{
	wchar_t value[2048];
	if (GetPrivateProfileString(section, key, L"", value, sizeof value / sizeof(wchar_t), m_Pathname) != 0)
	{
		vector<vec3> vecs;
		std::wstring s(value);
		std::wistringstream ss(s);
		std::wstring triplet;
		while (std::getline(ss, triplet, L',')) // Séparation chaque vec3
		{
			std::wistringstream tr(triplet);
			float x, y, z;
			tr >> x >> y >> z;
			if (tr) // vérifie si parsing ok
				vecs.emplace_back(x, y, z);
		}
		return vecs;
	}
	return defaultValue;
}
vector<dvec3> Ini::GetdVec3Array(const wchar_t* section, const wchar_t* key, const vector<dvec3>& defaultValue)
{
	wchar_t value[2048];
	if (GetPrivateProfileString(section, key, L"", value, sizeof value / sizeof(wchar_t), m_Pathname) != 0)
	{
		vector<dvec3> vecs;
		std::wstring s(value);
		std::wistringstream ss(s);
		std::wstring triplet;
		while (std::getline(ss, triplet, L',')) // Séparation chaque vec3
		{
			std::wistringstream tr(triplet);
			double x, y, z;
			tr >> x >> y >> z;
			if (tr) // vérifie si parsing ok
				vecs.emplace_back(x, y, z);
		}
		return vecs;
	}
	return defaultValue;
}

bool Ini::SetString(const wchar_t* section, const wchar_t* key, wchar_t * value)
{
	wostringstream S_Stream;
	S_Stream << value;
	wstring valueAsString = S_Stream.str();
	if(WritePrivateProfileString(section, key, valueAsString.c_str(), m_Pathname))
		return true;

	return false;
}
bool Ini::SetString(const wchar_t* section, const wchar_t* key, wstring value)
{
	if (WritePrivateProfileString(section, key, value.c_str(), m_Pathname))
		return true;

	return false;
}
bool Ini::SetInt(const wchar_t* section, const wchar_t* key, int value)
{
	wostringstream S_Stream;
	S_Stream << value;
	wstring valueAsString = S_Stream.str();
	if(WritePrivateProfileString(section, key, valueAsString.c_str(), m_Pathname))
		return true;
		
	return false;
}
bool Ini::SetFloat(const wchar_t* section, const wchar_t* key, float value)
{
	wostringstream S_Stream;
	S_Stream << value;
	wstring valueAsString = S_Stream.str();
	if(WritePrivateProfileString(section, key, valueAsString.c_str(), m_Pathname))
		return true;

	return false;
}
bool Ini::SetDouble(const wchar_t* section, const wchar_t* key, double value)
{
	wostringstream S_Stream;
	S_Stream << " " << value;
	wstring valueAsString = S_Stream.str();
	if(WritePrivateProfileString(section, key, valueAsString.c_str(), m_Pathname))
		return true;

	return false;
}
bool Ini::SetBoolean(const wchar_t* section, const wchar_t* key, bool value)
{
	wostringstream S_Stream;
	S_Stream << value;
	wstring valueAsString = S_Stream.str();
	if(WritePrivateProfileString(section, key, valueAsString.c_str(), m_Pathname))
		return true;

	return false;
}
bool Ini::SetColor(const wchar_t* section, const wchar_t* key, unsigned long value)
{
	int B = value & 0xFF; value >>= 8;
	int G = value & 0xFF; value >>= 8;
	int R = value & 0xFF; value >>= 8;
	//int A = value & 0xFF;

	wostringstream S_Stream;
	S_Stream << " " << R << " " << G << " " << B;
	wstring valueAsString = S_Stream.str();
	if(WritePrivateProfileString(section, key, valueAsString.c_str(), m_Pathname))
		return true;

	return false;
}
bool Ini::SetVec3(const wchar_t* section, const wchar_t* key, const vec3& value)
{
	wostringstream S_Stream;
	S_Stream << value.x << L" " << value.y << L" " << value.z;
	wstring valueAsString = S_Stream.str();
	if (WritePrivateProfileString(section, key, valueAsString.c_str(), m_Pathname))
		return true;
	
	return false;
}
bool Ini::SetdVec3(const wchar_t* section, const wchar_t* key, const dvec3& value)
{
	wostringstream S_Stream;
	S_Stream << value.x << L" " << value.y << L" " << value.z;
	wstring valueAsString = S_Stream.str();
	if (WritePrivateProfileString(section, key, valueAsString.c_str(), m_Pathname))
		return true;

	return false;
}
bool Ini::SetVec3Array(const wchar_t* section, const wchar_t* key, const vector<vec3>& vecArray)
{
	if (vecArray.empty()) return false;
	wostringstream ss;
	for (const auto& v : vecArray) {
		ss << v.x << L" " << v.y << L" " << v.z << L",";
	}
	wstring valueStr = ss.str();
	valueStr.pop_back();
	return WritePrivateProfileString(section, key, valueStr.c_str(), m_Pathname) ? true : false;
}
bool Ini::SetdVec3Array(const wchar_t* section, const wchar_t* key, const vector<dvec3>& vecArray)
{
	if (vecArray.empty()) return false;
	wostringstream ss;
	for (const auto& v : vecArray) {
		ss << v.x << L" " << v.y << L" " << v.z << L",";
	}
	wstring valueStr = ss.str();
	valueStr.pop_back();
	return WritePrivateProfileString(section, key, valueStr.c_str(), m_Pathname) ? true : false;
}
