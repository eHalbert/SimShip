/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <vector>
#include <limits>

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "pugixml/pugixml.hpp"
#ifdef _DEBUG
#pragma comment(lib, "pugixml/Debug/pugixml.lib")
#else
#pragma comment(lib, "pugixml/Release/pugixml.lib")
#endif

#include "Utility.h"
#include "Camera.h"
#include "Shader.h"
#include "Model.h"
#include "Ocean.h"

enum eLightType { Flash = 0, Occultation };
struct sLighthouse
{
	wstring			name;
	vec3			pos;
	float			range;
	eLightType		type;
	vector<float>	vAngles;
	vector<vec3>	vColors;
	float			durationOfTurn;
	double			angleStart;
	bool			isInViewFrustum;
};

class Lighthouse
{
public:
	Lighthouse(sLighthouse& sLh)
	{
		mLH = sLh;
		
		CreateConeOfLight();
		InitBillboard();

		mShaderFlash = make_unique<Shader>("Resources/Misc/lighthouse.vert", "Resources/Misc/lighthouse.frag");
		mShaderOccultation = make_unique<Shader>("Resources/Misc/light.vert", "Resources/Misc/light.frag");
	}
	void RenderBeamLight(Camera& camera)
	{
		// Color
		vec3 toCamera = camera.GetPosition() - mLH.pos;
		float dirToLighthouse = get_angle_from_north(-toCamera);
		vec3 color = GetSectorColor(dirToLighthouse);
		if (color == vec3(0.0f))	// No light
			return;

		if (mLH.type == eLightType::Flash)
		{ 
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);

			mShaderFlash->use(); // Misc/lighthouse.vert, Misc/lighthouse.frag

			// Rotation
			mat4 model = mat4(1.0f);
			model = glm::translate(model, mLH.pos);
			float angle = mLH.angleStart + (float)(glfwGetTime() * 2.0f * glm::pi<float>() / mLH.durationOfTurn);
			model = glm::rotate(model, angle, vec3(0.0f, 1.0f, 0.0f));
			mShaderFlash->setMat4("model", model);
			mShaderFlash->setMat4("view", camera.GetView());
			mShaderFlash->setMat4("projection", camera.GetProjection());
			mShaderFlash->setVec3("color", color);

			// Intensity function of the direction of the cone
			mat4 rotMat = glm::rotate(mat4(1), angle, vec3(0, 1, 0));
			vec3 coneDirLocal = vec3(1, 0, 0);
			vec3 coneDirWorld = vec3(rotMat * vec4(coneDirLocal, 0.0));
			float dotInt = glm::clamp(glm::dot(glm::normalize(coneDirWorld), glm::normalize(toCamera)), 0.0f, 1.0f);
			float intensity = 0.5f * pow(dotInt, 10.0f);
			mShaderFlash->setFloat("intensity", intensity);

			glBindVertexArray(mVAO);
			glDrawElements(GL_TRIANGLES, mIndicesCount, GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);

			glDisable(GL_CULL_FACE);
		}
	}
	void RenderLight(Camera& camera)
	{
		// Color
		vec3 toCamera = camera.GetPosition() - mLH.pos;
		float dirToLighthouse = get_angle_from_north(-toCamera);
		vec3 color = GetSectorColor(dirToLighthouse);
		if (color == vec3(0.0f))	// No light
			return;

		if (mLH.type == eLightType::Occultation)
		{
			if (camera.IsInViewFrustum(mLH.pos))
			{
				// Determination of the occulted phase
				double t = mLH.angleStart + glfwGetTime();
				double tInTurn = fmod(t, mLH.durationOfTurn);
				const double occultedDuration = 1.0; // duration in seconds during which the fire is extinguished

				if (tInTurn < (mLH.durationOfTurn - occultedDuration)) // On most of the time
				{
					mShaderOccultation->use();
					mShaderOccultation->setMat4("view", camera.GetView());
					mShaderOccultation->setMat4("projection", camera.GetProjection());

					glBindVertexArray(mQuadVAO);

					mat4 model = glm::translate(mat4(1.0f), mLH.pos);

					// Cylindrical billboard - the quad is always facing the camera
					vec3 camRight = vec3(camera.GetView()[0][0], camera.GetView()[1][0], camera.GetView()[2][0]);
					vec3 camUp = vec3(camera.GetView()[0][1], camera.GetView()[1][1], camera.GetView()[2][1]);
					model[0] = vec4(camRight, 0.0f);
					model[1] = vec4(camUp, 0.0f);

					float dCameraToLight = glm::length(camera.GetPosition() - mLH.pos);
					// Distance limits and scales
					const float dMin = 0.5f * 1852.0f;      // 1/2 NM
					const float dMax = 9.0f * 1852.0f;      // 9 NM
					const float sMin = 10.0f;
					const float sMax = 30.0f;
					// Linear interpolation with clamp
					float t = (dCameraToLight - dMin) / (dMax - dMin);
					t = glm::clamp(t, 0.0f, 1.0f);          // t in [0,1]
					float scale = sMin + t * (sMax - sMin); // scale in [2,20]
					model = glm::scale(model, glm::vec3(scale));

					mShaderOccultation->setMat4("model", model);
					mShaderOccultation->setVec3("lightColor", color);
					mShaderOccultation->setFloat("lightIntensity", 1.0f);
					mShaderOccultation->setFloat("starIntensity", 0.1f);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
					
					glBindVertexArray(0);
				}
			}
		}
	}

private:

	void CreateConeOfLight()
	{
		struct Vertex {
			vec3 position;
			vec3 normal;
			float t; // [1 = base, 0 = tip]
		};

		float angle_deg = 0.5f;
		int slices = 8;

		vector<Vertex> vertices;
		vector<unsigned int> indices;

		float angle_rad = glm::radians(angle_deg);
		float ConeRadiusSmall = 1.0f;
		float ConeRadiusLarge = ConeRadiusSmall + mLH.range * tan(angle_rad / 2.0f);

		vec3 base_center = vec3(0.0f);
		vec3 tip_center = base_center + vec3(mLH.range, 0, 0);

		// Analytic lateral normal
		float slope = (ConeRadiusLarge - ConeRadiusSmall) / mLH.range;
		float normal_x = 1.0f / sqrt(slope * slope + 1.0f);
		float normal_yrz = slope * normal_x;

		// Generation of vertices + normals
		for (int i = 0; i < slices; ++i)
		{
			float theta = 2.0f * glm::pi<float>() * float(i) / float(slices);
			float y = cos(theta);
			float z = sin(theta);

			vec3 pos0 = base_center + vec3(0, ConeRadiusSmall * y, ConeRadiusSmall * z);
			vec3 pos1 = tip_center + vec3(0, ConeRadiusLarge * y, ConeRadiusLarge * z);
			vec3 normal = glm::normalize(vec3(normal_x, normal_yrz * y, normal_yrz * z));

			vertices.push_back({ pos0, normal, 1.0f }); // base
			vertices.push_back({ pos1, normal, 0.0f }); // tip
		}

		// Lateral surface indices
		for (int i = 0; i < slices; ++i)
		{
			int next = (i + 1) % slices;
			int i0 = i * 2 + 0;
			int i1 = i * 2 + 1;
			int j0 = next * 2 + 0;
			int j1 = next * 2 + 1;

			// Two triangles for the quad
			indices.push_back(i0); indices.push_back(j1); indices.push_back(i1);
			indices.push_back(i0); indices.push_back(j0); indices.push_back(j1);
		}
		mIndicesCount = indices.size();

		// Creating OpenGL Buffers
		GLuint vbo, ebo;
		glGenVertexArrays(1, &mVAO);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		glBindVertexArray(mVAO);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, t));
		glEnableVertexAttribArray(2);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

		glBindVertexArray(0);
	}
	void InitBillboard()
	{
		float quadVertices[] = {
			// positions   // UVs
			-0.5f,  0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f,  0.0f, 0.0f,
			 0.5f,  0.5f,  1.0f, 1.0f,
			 0.5f, -0.5f,  1.0f, 0.0f,
		};

		GLuint quadVBO = 0;

		glGenVertexArrays(1, &mQuadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(mQuadVAO);

		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

		// position attribute
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

		// UV attribute
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

		glBindVertexArray(0);
	}
	vec3 GetSectorColor(float angleDeg)
	{
		size_t n = mLH.vAngles.size();

		if (n == 0)
			return mLH.vColors[0];

		for (size_t i = 0; i < n; ++i)
		{
			float a0 = mLH.vAngles[i];
			float a1 = mLH.vAngles[(i + 1) % n];
			// Wrap-around 360°
			if (a0 < a1)
			{
				if (angleDeg >= a0 && angleDeg < a1)
					return mLH.vColors[i];
			}
			else
			{
				if (angleDeg >= a0 || angleDeg < a1)
					return mLH.vColors[i];
			}
		}
		// Default (no sector)
		return vec3(0.0f, 0.0f, 0.0f);
	}

	sLighthouse			mLH;
	// Flash
	GLuint				mVAO = 0;
	int					mIndicesCount = 0;
	unique_ptr<Shader>  mShaderFlash = 0;
	// Occultation
	GLuint              mQuadVAO = 0;
	unique_ptr<Shader>  mShaderOccultation = 0;
};

class LighthouseMgr
{
public:
	LighthouseMgr(wstring fullname)
	{
		LoadFromXML(fullname.c_str());
	}
	~LighthouseMgr() {};

	void RenderBeamLights(Camera& camera, bool bLights = false)
	{
		if (!bVisible)
			return;
		
		if (!bLights)
			return;

		for (auto& lh : mvLighthouses)
			lh->RenderBeamLight(camera);
	}

	void RenderLights(Camera& camera, bool bLights = false)
	{
		if (!bVisible)
			return;

		if (!bLights)
			return;

		for (auto& lh : mvLighthouses)
			lh->RenderLight(camera);
	}

	bool bVisible = true;

private:
	void LoadFromXML(const wstring filename)
	{
		std::mt19937 rng(static_cast<unsigned int>(time(nullptr)));
		std::uniform_real_distribution<double> dist(0.0, 2.0 * glm::pi<double>());
		
		mvLighthouses.clear();

		pugi::xml_document doc;

		// Load XML file
		pugi::xml_parse_result result = doc.load_file(filename.c_str());

		if (result)
		{
			// Get the root node
			pugi::xml_node root = doc.child(L"Lighthouses");

			// Browse all "Lighthouse" nodes
			for (pugi::xml_node lhNode : root.children(L"Lighthouse"))
			{
				sLighthouse lh;

				lh.name = lhNode.child(L"Name").text().as_string();
				float lat = lhNode.child(L"Latitude").attribute(L"value").as_float();
				float lon = lhNode.child(L"Longitude").attribute(L"value").as_float();
				float height = lhNode.child(L"Height").attribute(L"value").as_float();
				lh.pos = lonlat_to_opengl(lon, lat);
				lh.pos.y = height;
				lh.range = lhNode.child(L"Range").attribute(L"value").as_float() * 1852.0f * 0.25f;

				wstring t = lhNode.child(L"Type").text().as_string();
				if (t == L"Flash")
					lh.type = eLightType::Flash;
				else if (t == L"Occultation")
					lh.type = eLightType::Occultation;

				// vAngles
				pugi::xml_node anglesNode = lhNode.child(L"Angles");
				if (anglesNode) 
				{
					std::wstringstream ss(anglesNode.text().as_string());
					float angle;
					while (ss >> angle)
						lh.vAngles.push_back(angle);
				}

				// vColors
				for (pugi::xml_node colorNode : lhNode.child(L"Colors").children(L"color")) 
				{
					float r = colorNode.attribute(L"r").as_float();
					float g = colorNode.attribute(L"g").as_float();
					float b = colorNode.attribute(L"b").as_float();
					lh.vColors.push_back(vec3(r, g, b));
				}
			
				lh.durationOfTurn = lhNode.child(L"DurationOfTurn").attribute(L"value").as_float();

				lh.angleStart = dist(rng);

				if (lh.vColors.size() > 0)
				{
					Lighthouse* lighthouse = new Lighthouse(lh);
					mvLighthouses.push_back(lighthouse);
				}
			}
		}
		else
		{
			// Handle file loading error
			std::wcerr << L"Error loading XML file: " << result.description() << std::endl;
		}
	}
	
	vector<Lighthouse*> mvLighthouses;
};