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
	int     pylone;
	int     silo;
	int     mooring;
    int     idxModel = -1;
    double  time;
    vec3    lightColor;
};

// File must have Lon and Lat with dots dor decimal

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
        mBuoy[0] = make_unique<Model>("Resources/Buoys/Buoy-North.glb");
        mBuoy[1] = make_unique<Model>("Resources/Buoys/Buoy-East.glb");
        mBuoy[2] = make_unique<Model>("Resources/Buoys/Buoy-South.glb");
        mBuoy[3] = make_unique<Model>("Resources/Buoys/Buoy-West.glb");
        mBuoy[4] = make_unique<Model>("Resources/Buoys/Buoy-Portside.glb");
        mBuoy[5] = make_unique<Model>("Resources/Buoys/Buoy-Starboard.glb");
        mBuoy[6] = make_unique<Model>("Resources/Buoys/Buoy-Danger.glb");

        vvPatterns = {
        {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0}, // N
        {1, 0, 1, 0, 1, 0, 0, 0},                                                       // E
        {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0},                                     // S
        {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0},                   // W
        {1, 1, 0, 0},
        {0, 1, 1, 0},
        {1, 0, 1, 0, 0, 0, 0, 0},
        };
		
        mShader = make_unique<Shader>("Resources/Misc/sun.vert", "Resources/Misc/sun.frag");
        mShaderNavLight = make_unique<Shader>("Resources/Ship/ship_light.vert", "Resources/Ship/ship_light.frag");  // For the navigation lights
        mLight = make_unique<Sphere>(1.0f, 8);
	};
	~Markup() {};

	void Render(Camera& camera, Ocean* ocean, Sky* sky)
	{
		if (!bVisible)
			return;

        // Models
        mShader->use();
        mShader->setVec3("light.position", sky->SunPosition);
        mShader->setVec3("light.ambient", sky->SunAmbient);
        mShader->setVec3("light.diffuse", sky->SunDiffuse);
        mShader->setVec3("light.specular", sky->SunSpecular);
        mShader->setVec3("viewPos", camera.GetPosition());
        mShader->setFloat("exposure", sky->Exposure);
        mShader->setBool("bAbsorbance", sky->bAbsorbance);
        mShader->setVec3("absorbanceColor", sky->AbsorbanceColor);
        mShader->setFloat("absorbanceCoeff", sky->AbsorbanceCoeff);
        mShader->setMat4("view", camera.GetView());
        mShader->setMat4("projection", camera.GetProjection());

        vec3 eye = camera.GetPosition();
        for (auto& mark : mvMarks)
        {
            mat4 model = glm::translate(mat4(1.0f), mark.pos);
            mShader->setMat4("model", model);
            mBuoy[mark.idxModel]->Render(*mShader);
        }

        // Lights
        if (sky->SunPosition.y < 0.0f)
        {
            for (auto& mark : mvMarks)
            {
                int patternLength = static_cast<int>(vvPatterns[mark.idxModel].size());
                int currentIndex = static_cast<int>(floor((glfwGetTime() + mark.time) * 2)) % patternLength;    // Each element of the pattern is 0.5 second (=> * 2)
                bool lightOn = vvPatterns[mark.idxModel][currentIndex] == 1;
                if (!lightOn)
                    continue;

                mShaderNavLight->use();     // Misc/ship_light.vert, Misc/ship_light.frag
                vec3 p = mark.pos;
                p.y = 4.5f;
                mat4 model = glm::translate(mat4(1.0f), p);
                mShaderNavLight->setMat4("model", model);
                mShaderNavLight->setMat4("view", camera.GetView());
                mShaderNavLight->setMat4("projection", camera.GetProjection());
                mShaderNavLight->setVec3("lightColor", mark.lightColor);
                mShaderNavLight->setVec3("viewPos", camera.GetPosition());
                mLight->Bind();
            }
        }
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
                mark.pylone = markNode.child(L"Pylone").attribute(L"value").as_int();
                mark.silo = markNode.child(L"Silo").attribute(L"value").as_int();
                mark.mooring = markNode.child(L"Mooring").attribute(L"value").as_int();
                float lat = markNode.child(L"Latitude").attribute(L"value").as_float();
                float lon = markNode.child(L"Longitude").attribute(L"value").as_float();
                mark.pos = LonLatToOpenGL(lon, lat);

                if (mark.cardinal == 1)         mark.idxModel = 0;  // Buoy-North
                else if (mark.cardinal == 2)    mark.idxModel = 1;  // Buoy-East
                else if (mark.cardinal == 3)    mark.idxModel = 2;  // Buoy-South
                else if (mark.cardinal == 4)    mark.idxModel = 3;  // Buoy-West
                
                else if (mark.lateral == 1)     mark.idxModel = 4;  // Buoy-Portside
                else if (mark.lateral == 2)     mark.idxModel = 5;  // Buoy-Starboard
                
                else if (mark.colour == L"2,3,2") mark.idxModel = 6;// Buoy-Danger

                switch (mark.idxModel)
                {
                case 0:
                case 1:
                case 2:
                case 3:
                    mark.lightColor = vec3(1.0f, 1.0f, 1.0f); break;
                case 4:
                    mark.lightColor = vec3(1.0f, 0.0f, 0.0f); break;
                case 5:
                    mark.lightColor = vec3(0.0f, 1.0f, 0.0f); break;
                case 6:
                    mark.lightColor = vec3(1.0f, 1.0f, 0.0f); break;
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

    unique_ptr<Shader>	mShader;
    unique_ptr<Shader>	mShaderNavLight;
	unique_ptr<Model>	mBuoy[7];
    vector<sMark>       mvMarks;
    unique_ptr<Sphere>	mLight = 0;
    vector<vector<int>> vvPatterns; 
};


/*
Dans la cartographie S-57, les codes des couleurs utilisés dans la description d'un objet, comme dans l'attribut COLOUR, sont les suivants :
Blanc (1)
Noir (2)
Rouge (3)
Vert (4)
Bleu (5)
Jaune (6)
Gris (7)
Brun (8)
Orange (9)
Violet (10)
Magenta (11)

Les différents codes de BOYSHP (forme de la bouée) dans la cartographie S-57 sont :
Conique (1)
Cylindrique (2)
Sphérique (3)
Pilier (4)
Espar (5)
Baril (6)
Superstructure (7)
Flotteur (8)

Dans la cartographie S-57, les différents codes de CATCAM (Catégorie de balise cardinale) sont :
Nord (1)
Est (2)
Sud (3)
Ouest (4)

Dans la cartographie S-57, les différents codes de COLPAT (motif de couleur) sont :
Horizontal (1)
Vertical (2)
Diagonal (3)
Carré (4)
Rayé (5)
Bordure (6)

Dans la cartographie S-57, les différents codes de CATLAM (Catégorie de marque latérale) sont :
Bâbord (1)
Tribord (2)
Chenal préféré à tribord (3)
Chenal préféré à bâbord (4)

Dans la cartographie S-57, les différents codes de CATMOR (Catégorie d'amarrage) sont :
Duc d'Albe (1)
Bitte d'amarrage (2)
Bollard (3)
Chaîne ou câble d'amarrage (4)
Bouée d'amarrage (5)
Mur de quai (6)
Pieu d'amarrage (7)

Dans la cartographie S-57, les différents codes de BCNSHP (forme de la balise) sont :
Tour (1)
Perche (2)
Espar (3)
Pylône (4)
Pile (5)
Superstructure (6)
Flotteur (7)

Dans la cartographie S-57, les différents codes de CATPYL (Catégorie de pylône) sont :
Pylône de pont (1)
Pylône de ligne électrique (2)
Pylône de téléphérique (3)
Pylône de remontée mécanique (4)

Dans la cartographie S-57, les différents codes de CATSIL (Catégorie de silo) sont :
Silo à grains (1)
Silo à ciment (2)
Silo à eau (3)
Silo à produits chimiques (4)

Dans la cartographie S-57, les différents codes de CATLMK (Catégorie d'amer) sont :
Cairn (1)
Tour de balise (2)
Cheminée (3)
Colonne (4)
Monument (5)
Obélisque (6)
Statue (7)
Croix (8)
Dôme (9)
Radar (10)
Tour (11)
Moulin à vent (12)
Mât (13)
Croix de grande dimension (14)
Minaret (15)
Réservoir (16)
Colonne (pilier) (17)
Croix de grande dimension lumineuse (18)
Sphère (19)
Château d'eau (20)
Hangar à dirigeables ou aéronefs (21)
Réservoir de gaz (gazomètre) (22)
Bâtiment (en général) (23)
Bâtiment remarquable (24)
Silo (25)
Flèche (26)


*/