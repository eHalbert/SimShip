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

struct sMark
{
	wstring name;
    vec3    pos;
	wstring colour;
	int     boyshp;
	int     bcnshp;
	int     cardinal;
	int     lateral;
	int     landmark;
	int     mooring;
    int     idxModel = -1;
    double  time;
    vec3    lightColor;
    bool    isInViewFrustum;
    vec3    waterPosition;
};

// File must have Lon and Lat with dots nor decimal

class Markup
{
public:
    Markup(wstring fullname)
    {
		// Load zones from XML file
        LoadMarksFromXML(fullname.c_str());

        // Initialiser le générateur aléatoire avec un seed unique (ici avec l'heure)
        std::mt19937 rng(static_cast<unsigned int>(time(nullptr)));
        std::uniform_real_distribution<double> dist(0.0, 8.0); // Cycle max = 8 secondes

        // Pour chaque bouée, initialiser un décalage de temps aléatoire
        for (auto& mark : mvMarks)
            mark.time = dist(rng); // décalage aléatoire en secondes

        // Load the different buoys
        mModel[0] = make_unique<Model>("Resources/Buoys/Boy-North.gltf");
        mModel[1] = make_unique<Model>("Resources/Buoys/Boy-East.gltf");
        mModel[2] = make_unique<Model>("Resources/Buoys/Boy-South.gltf");
        mModel[3] = make_unique<Model>("Resources/Buoys/Boy-West.gltf");
        mModel[4] = make_unique<Model>("Resources/Buoys/Boy-Portside.gltf");
        mModel[5] = make_unique<Model>("Resources/Buoys/Boy-Starboard.gltf");
        mModel[6] = make_unique<Model>("Resources/Buoys/Boy-Danger.gltf");
        mModel[7] = make_unique<Model>("Resources/Buoys/Bcn-North.gltf");
        mModel[8] = make_unique<Model>("Resources/Buoys/Bcn-East.gltf");
        mModel[9] = make_unique<Model>("Resources/Buoys/Bcn-South.gltf");
        mModel[10] = make_unique<Model>("Resources/Buoys/Bcn-West.gltf");
        mModel[11] = make_unique<Model>("Resources/Buoys/Bcn-Portside.gltf");
        mModel[12] = make_unique<Model>("Resources/Buoys/Bcn-Starboard.gltf");
        mModel[13] = make_unique<Model>("Resources/Buoys/Bcn-Danger.gltf");
        mModel[14] = make_unique<Model>("Resources/Buoys/Boy-Mooring.gltf");

        vvPatterns = {
        // Buoy
        {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0}, // N
        {1, 0, 1, 0, 1, 0, 0, 0},                                                       // E
        {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0},                                     // S
        {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0},                   // W
        {1, 1, 0, 0},
        {0, 1, 1, 0},
        {1, 0, 1, 0, 0, 0, 0, 0},
        // Beacon
        {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0}, // N
        {1, 0, 1, 0, 1, 0, 0, 0},                                                       // E
        {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0},                                     // S
        {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0},                   // W
        {1, 1, 0, 0},
        {0, 1, 1, 0},
        {1, 0, 1, 0, 0, 0, 0, 0},
        // Mooring
        {1, 0, 1, 0},
        };
		
        mShader = make_unique<Shader>("Resources/Misc/sun.vert", "Resources/Misc/sun.frag");
        mShaderLight = make_unique<Shader>("Resources/Misc/light.vert", "Resources/Misc/light.frag");
        
        InitBillboard();
	};
	~Markup() {};

	void Render(Camera& camera, Ocean* ocean, Sky* sky)
	{
		if (!bVisible)
			return;

        // Check the visibility of all the marks
        for (auto& mark : mvMarks)
        {
            if (camera.IsInViewFrustum(mark.pos))
            {
                mark.isInViewFrustum = true;
                if (mark.boyshp || mark.mooring)
                    ocean->GetVerticeXYZ(mark.pos, mark.waterPosition);
            }
            else
                mark.isInViewFrustum = false;
        }

        // Render the visible marks
        mShader->use();     // Misc/sun.vert, Misc/sun.frag
        mShader->setVec3("light.position", sky->SunPosition);
        mShader->setVec3("light.ambient", sky->SunAmbient);
        mShader->setVec3("light.diffuse", sky->SunDiffuse);
        mShader->setVec3("light.specular", sky->SunSpecular);
        mShader->setVec3("viewPos", camera.GetPosition());
        mShader->setFloat("exposure", sky->Exposure * 2.0f);
        mShader->setBool("bAbsorbance", sky->bAbsorbance);
        mShader->setVec3("absorbanceColor", sky->AbsorbanceColor);
        mShader->setFloat("absorbanceCoeff", sky->AbsorbanceCoeff);
        mShader->setMat4("view", camera.GetView());
        mShader->setMat4("projection", camera.GetProjection());
        for (auto& mark : mvMarks)
        {
            if (mark.isInViewFrustum)
            {
                vec3 p;
                if (mark.boyshp > 0 || mark.mooring > 0)
                    p = mark.waterPosition;
                else
                    p = mark.pos;
                mat4 model = glm::translate(mat4(1.0f), p);
                mShader->setMat4("model", model);
                mModel[mark.idxModel]->Render(*mShader);
            }
        }
	}
    void RenderLights(Camera& camera, bool bLights = false)
    {
        if (!bLights)
            return;

        glBindVertexArray(mQuadVAO);
        
        mShaderLight->use();        // Misc/light.vert, Misc/light.frag
        mShaderLight->setMat4("view", camera.GetView());
        mShaderLight->setMat4("projection", camera.GetProjection());

        for (auto& mark : mvMarks)
        {
            if (mark.isInViewFrustum)
            {
                int patternLength = static_cast<int>(vvPatterns[mark.idxModel].size());
                int currentIndex = static_cast<int>(floor((glfwGetTime() + mark.time) * 2)) % patternLength;
                bool lightOn = vvPatterns[mark.idxModel][currentIndex] == 1;
                if (!lightOn)
                    continue;

                vec3 p;
                if (mark.boyshp > 0 || mark.mooring > 0)
                    p = mark.waterPosition;
                else
                    p = mark.pos;

                if (mark.boyshp)
                    p.y += 4.2f;
                else if (mark.bcnshp)
                    p.y += 4.5f;
                else if (mark.mooring)
                    p.y += 1.4f;

                mat4 model = glm::translate(mat4(1.0f), p);

                // Cylindrical billboard - the quad is always facing the camera
                vec3 camRight = vec3(camera.GetView()[0][0], camera.GetView()[1][0], camera.GetView()[2][0]);
                vec3 camUp = vec3(camera.GetView()[0][1], camera.GetView()[1][1], camera.GetView()[2][1]);
                model[0] = vec4(camRight, 0.0f);
                model[1] = vec4(camUp, 0.0f);

                float scale = 5.0f;
                model = glm::scale(model, vec3(scale, scale, scale));

                mShaderLight->setMat4("model", model);
                mShaderLight->setVec3("lightColor", mark.lightColor);
                mShaderLight->setFloat("lightIntensity", 1.0f);
                mShaderLight->setFloat("starIntensity", 0.1f);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }

        glBindVertexArray(0);
    }
 
    bool bVisible = true;

private:

    void LoadMarksFromXML(const wstring filename)
    {
        mvMarks.clear();
        pugi::xml_document doc;

        // Load XML file
        pugi::xml_parse_result result = doc.load_file(filename.c_str());

        if (result)
        {
            // Get the root node
            pugi::xml_node root = doc.child(L"Marks");

            // Browse all "Mark" nodes
            for (pugi::xml_node markNode : root.children(L"Mark"))
            {
                sMark mark;

                // Read the data for each mark
                mark.name = markNode.child(L"Name").text().as_string();
                mark.colour = markNode.child(L"Colour").text().as_string();
                mark.boyshp = markNode.child(L"Boyshp").attribute(L"value").as_int();
                mark.bcnshp = markNode.child(L"Bcnshp").attribute(L"value").as_int();
                mark.cardinal = markNode.child(L"Cardinal").attribute(L"value").as_int();
                mark.lateral = markNode.child(L"Lateral").attribute(L"value").as_int();
                mark.landmark = markNode.child(L"Landmark").attribute(L"value").as_int();
                mark.mooring = markNode.child(L"Mooring").attribute(L"value").as_int();
                float lat = markNode.child(L"Latitude").attribute(L"value").as_float();
                float lon = markNode.child(L"Longitude").attribute(L"value").as_float();
                mark.pos = lonlat_to_opengl(lon, lat);

                if (mark.boyshp > 0)
                {
                    if (mark.cardinal == 1)         mark.idxModel = 0;  // Buoy - North
                    else if (mark.cardinal == 2)    mark.idxModel = 1;  // Buoy - East
                    else if (mark.cardinal == 3)    mark.idxModel = 2;  // Buoy - South
                    else if (mark.cardinal == 4)    mark.idxModel = 3;  // Buoy - West

                    else if (mark.lateral == 1)     mark.idxModel = 4;  // Buoy - Portside
                    else if (mark.lateral == 2)     mark.idxModel = 5;  // Buoy - Starboard

                    else if (mark.colour == L"2,3,2") mark.idxModel = 6;// Buoy - Danger
                }
                if (mark.bcnshp > 0)
                {
                    if (mark.cardinal == 1)         mark.idxModel = 7;  // Beacon - North
                    else if (mark.cardinal == 2)    mark.idxModel = 8;  // Beacon - East
                    else if (mark.cardinal == 3)    mark.idxModel = 9;  // Beacon - South
                    else if (mark.cardinal == 4)    mark.idxModel = 10; // Beacon - West

                    else if (mark.lateral == 1)     mark.idxModel = 11;  // Beacon - Portside
                    else if (mark.lateral == 2)     mark.idxModel = 12;  // Beacon - Starboard

                    else if (mark.colour == L"2,3,2") mark.idxModel = 13;// Buoy - Danger
                }
                if (mark.mooring > 0)
                    mark.idxModel = 14;// Buoy - Mooring

                switch (mark.idxModel)
                {
                case 4:     // Buoy-Portside
                case 11:    // Bcn-Portside
                    mark.lightColor = vec3(1.0f, 0.0f, 0.0f); break;    // Red
                case 5:     // Buoy-Starboard
                case 12:    // Bcn-Starboard
                    mark.lightColor = vec3(0.0f, 1.0f, 0.0f); break;    // Green
                case 6:     // Buoy-Danger
                case 13:    // Bcn-Danger
                case 14:    // Boy-Mooring
                    mark.lightColor = vec3(1.0f, 1.0f, 0.0f); break;    // Yellow
                default:    // Cardinals
                    mark.lightColor = vec3(1.0f, 1.0f, 1.0f); break;    // White
                }

                if(mark.idxModel != -1)
                    mvMarks.push_back(mark);
            }
        }
        else
        {
            // Handle file loading error
            std::wcerr << L"Error loading XML file: " << result.description() << std::endl;
        }
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

    unique_ptr<Shader>	mShader;
	unique_ptr<Model>	mModel[15];
    vector<sMark>       mvMarks;
    vector<vector<int>> vvPatterns;
    
    GLuint              mQuadVAO = 0;
    unique_ptr<Shader>	mShaderLight;
};

/*
File lut.json in BALISAGE.z
{
	"BCNSHP": {
		"0": "Forme de balise",
		"1": "Perche, jalon, pieu",
		"2": "Branche",
		"3": "Tourelle",
		"4": "Balise à treillis",
		"5": "Balise pilier",
		"6": "Cairn",
		"7": "Balise à flotteurs"
	},
	"BOYSHP": {
		"0": "Forme de bouée",
		"1": "Cônique",
		"2": "Cylindrique",
		"3": "Sphérique",
		"4": "Pylône, charpente",
		"5": "Espar, fuseau",
		"6": "Bouée tonne",
		"7": "Super-bouée/bouée géante",
		"8": "Pour région glaciaire"
	},
	"BUISHP": {
		"0": "Forme de bâtiment",
		"1": "{no specific shape},",
		"2": "{tower},",
		"3": "{spire},",
		"4": "{cupola (dome) },",
		"5": "Immeuble haut",
		"6": "Pyramide",
		"7": "Cylindrique",
		"8": "Sphérique",
		"9": "Cubique",
		"701": "UNKNOWN",
		"703": "Not Applicable",
		"704": "Autre"
	},
	"CATCAM": {
		"0": "Catégorie de marque cardinale",
		"1": "Marque cardinale nord",
		"2": "Marque cardinale est",
		"3": "Marque cardinale sud",
		"4": "Marque cardinale ouest",
		"701": "UNKNOWN",
		"703": "Not Applicable"
	},
	"CATLAM": {
		"0": "Catégorie de marque latérale",
		"1": "Latérale bâbord",
		"2": "Latérale tribord",
		"3": "Chenal préféré à tribord",
		"4": "Chenal préféré à bâbord",
		"701": "UNKNOWN",
		"703": "Not Applicable"
	},
	"CATLMK": {
		"0": "Catégorie d'amers",
		"1": "Cairn",
		"2": "Cimetière",
		"3": "Cheminée",
		"4": "Antenne à réflecteur",
		"5": "Mât de pavillon",
		"6": "Torchère",
		"7": "Mât, pilier",
		"8": "Manche à air",
		"9": "Monument",
		"10": "Colonne",
		"11": "Plaque commémorative",
		"12": "Obélisque",
		"13": "Statue",
		"14": "Croix",
		"15": "Dôme",
		"16": "Antenne radar",
		"17": "Tour",
		"18": "Moulin à vent",
		"19": "Eolienne",
		"20": "Flèche/minaret",
		"21": "Rocher/bloc rocheux à terre",
		"22": "rock pinnacle",
		"701": "UNKNOWN",
		"702": "Multiple",
		"703": "Not Applicable",
		"704": "Autre"
	},
	"CATMOR": {
		"0": "Catégorie de dispositif d'amarrage",
		"1": "Duc d'Albe/dauphin",
		"2": "Duc d'Albe pour la régulation des compas",
		"3": "Bollard",
		"4": "Mur d'amarrage",
		"5": "Poteau/pilier",
		"6": "Chaine/câble",
		"7": "Coffres ou bouées d'amarrage",
		"501": "fast patrol boat waiting position",
		"701": "UNKNOWN",
		"703": "Not Applicable",
		"704": "Autre"
	},
	"COLOUR": {
		"0": "Couleur",
		"1": "Blanc",
		"2": "Noir",
		"3": "Rouge",
		"4": "Vert",
		"5": "Bleu",
		"6": "Jaune",
		"7": "Gris",
		"8": "Marron",
		"9": "Ambre",
		"10": "Violet",
		"11": "Orange",
		"12": "Magenta ",
		"13": "Rose",
		"701": "UNKNOWN",
		"702": "Multiple",
		"703": "Not Applicable",
		"704": "Autre"
	},
}*/
