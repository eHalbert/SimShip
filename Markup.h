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
#pragma comment(lib, "pugixml/Debug/x64/pugixml.lib")
#else
#pragma comment(lib, "pugixml/Release/x64/pugixml.lib")
#endif

#include "Utility.h"
#include "Camera.h"
#include "Shader.h"
#include "Model.h"
#include "Ocean.h"

struct sMark
{
	wstring name;
	wstring colour;
	int     boyshp;
	int     bcnshp;
	int     cardinal;
	int     lateral;
	int     landmark;
	int     pylone;
	int     silo;
	int     mooring;
    int     N = -1;
	double  lat, lon;
};

// File must have Lon and Lat with dots dor decimal

class Markup
{
public:
    Markup(wstring fullname)
    {
		// Load zones from XML file
        LoadMarksFromXML(fullname.c_str());

        // Load the different buoys
        mBuoy[0] = make_unique<Model>("Resources/Models/Buoys/Buoy-North.glb");
        mBuoy[1] = make_unique<Model>("Resources/Models/Buoys/Buoy-East.glb");
        mBuoy[2] = make_unique<Model>("Resources/Models/Buoys/Buoy-South.glb");
        mBuoy[3] = make_unique<Model>("Resources/Models/Buoys/Buoy-West.glb");
        mBuoy[4] = make_unique<Model>("Resources/Models/Buoys/Buoy-Port.glb");
        mBuoy[5] = make_unique<Model>("Resources/Models/Buoys/Buoy-Starboard.glb");
        mBuoy[6] = make_unique<Model>("Resources/Models/Buoys/Buoy-Danger.glb");

		mShader = make_unique<Shader>("Resources/Shaders/sun.vert", "Resources/Shaders/sun.frag");
	};
	~Markup() {};


	void Render(Camera& camera, Ocean* ocean, Sky* sky)
	{
		if (!bVisible)
			return;

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
        for (auto& m : mvMarks)
        {
            if (m.name == L"Men er Roué")
                cout << "";
            vec2 p = LonLatToOpenGL(m.lon, m.lat);
            vec3 pos = vec3(p.x, 0.0f, p.y);
            if (m.boyshp > 0 && glm::length2(pos - eye) < 10000.0f)
                ocean->GetVertice(pos, pos);
            mat4 model = glm::translate(mat4(1.0f), pos);
            //model = glm::scale(model, vec3(10.0f));
            mShader->setMat4("model", model);
            if (m.N != -1)
                mBuoy[m.N]->Render(*mShader); continue;
        }
        vec2 p = OpenGLToLonLat(17914, 8764);
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
                mark.lat = markNode.child(L"Latitude").attribute(L"value").as_double();
                mark.lon = markNode.child(L"Longitude").attribute(L"value").as_double();

                if (mark.cardinal == 1)
                    mark.N = 0;
                else if (mark.cardinal == 2)
                    mark.N = 1;
                else if (mark.cardinal == 3)
                    mark.N = 2;
                else if (mark.cardinal == 4)
                    mark.N = 3;
                else if (mark.lateral == 1)
                    mark.N = 4;
                else if (mark.lateral == 2)
                    mark.N = 5;
                else if (mark.colour == L"2,3,2")
                    mark.N = 6;

                mvMarks.push_back(mark);
            }
        }
        else
        {
            // Handle file loading error
            std::wcerr << L"Erreur de chargement du fichier XML : " << result.description() << std::endl;
        }
    }

	unique_ptr<Shader>	mShader;
	unique_ptr<Model>	mBuoy[7];
    vector<sMark>       mvMarks;
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