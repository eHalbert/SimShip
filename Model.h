/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#pragma once

#define NOMINMAX
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

// glad
#include <glad/glad.h>
// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
// stb_image
#include <stb/stb_image.h>
// assimp 5.4.3
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "Shader.h"
#include "Camera.h"

using namespace std;
using namespace glm;


struct sBBvec3
{
    vec3 min;
    vec3 max;
};


#include <glm/gtc/random.hpp>
class Model 
{
public:
    bool            bVisible;
    unsigned int    NbVertices = 0;
    unsigned int    NbFaces = 0;

    Model(string const &path)
    {
        loadModel(path);
        bVisible = true;
    }
    
    void Bind()
    {
        if (!bVisible)
            return;

        for (unsigned int i = 0; i < mvMeshes.size(); i++)
            mvMeshes[i].Bind();
    }
    sBBvec3 GetBoundingBox()
    {
        sBBvec3 bbox;
        bbox.min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
        bbox.max = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };

        for (const auto& mesh : mvMeshes)
        {
            for (const auto& vertex : mesh.vVertices)
            {
                bbox.min.x = std::min(bbox.min.x, vertex.Position.x);
                bbox.min.y = std::min(bbox.min.y, vertex.Position.y);
                bbox.min.z = std::min(bbox.min.z, vertex.Position.z);

                bbox.max.x = std::max(bbox.max.x, vertex.Position.x);
                bbox.max.y = std::max(bbox.max.y, vertex.Position.y);
                bbox.max.z = std::max(bbox.max.z, vertex.Position.z);
            }
        }

        return bbox;
    }
    vector<Mesh>& GetMesh() { return mvMeshes; }

    void Render(Shader& shader)
    {
        if (!bVisible)
            return;

        for (unsigned int i = 0; i < mvMeshes.size(); i++)
            mvMeshes[i].Render(shader);
    }
    void RenderWithTransparency(Shader& shader, Camera& camera, mat4 model)
    {
        if (!bVisible)
            return;

        vec3 camPos = camera.GetPosition();

        // Actualiser les centers avec la matrice model
        for (auto& mesh : mvMeshes)
        {
            vec4 newCenter = model * vec4(mesh.Center, 1.0f);
            mesh.TransformedCenter = vec3(newCenter);
        }

        // Séparer opaque / transparent
        vector<Mesh*> opaqueMeshes;
        vector<Mesh*> transparentMeshes;

        for (auto& mesh : mvMeshes)
        {
            if (mesh.HasTransparency)
                transparentMeshes.push_back(&mesh);
            else
                opaqueMeshes.push_back(&mesh);
        }

        // Dessiner opaque (ordre non important)
        for (auto* mesh : opaqueMeshes)
            mesh->Render(shader);

        // Trier transparent par distance décroissante
        std::sort(transparentMeshes.begin(), transparentMeshes.end(),
            [&](Mesh* a, Mesh* b) {
                float distA = glm::distance(camPos, a->TransformedCenter);
                float distB = glm::distance(camPos, b->TransformedCenter);
                return distA > distB; // plus éloigné en premier
            }
        );

        // Dessiner transparent dans l’ordre trié
        for (auto* mesh : transparentMeshes)
            mesh->Render(shader);
    }
    void RenderTransparentMeshes(Shader& shader, Camera& camera, mat4 model)
    {
        if (!bVisible)
            return;

        vec3 camPos = camera.GetPosition();

        // Actualiser les centers avec la matrice model
        for (auto& mesh : mvMeshes)
        {
            vec4 newCenter = model * vec4(mesh.Center, 1.0f);
            mesh.TransformedCenter = vec3(newCenter);
        }

        // Séparer opaque / transparent
        vector<Mesh*> transparentMeshes;

        for (auto& mesh : mvMeshes)
            if (mesh.HasTransparency)
                transparentMeshes.push_back(&mesh);

        // Trier transparent par distance décroissante
        std::sort(transparentMeshes.begin(), transparentMeshes.end(),
            [&](Mesh* a, Mesh* b) {
                float distA = glm::distance(camPos, a->TransformedCenter);
                float distB = glm::distance(camPos, b->TransformedCenter);
                return distA > distB; // plus éloigné en premier
            }
        );

        // Dessiner transparent dans l’ordre trié
        for (auto* mesh : transparentMeshes)
            mesh->Render(shader);
    }

private:
    vector<sTexture> mvTextures;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once
    vector<Mesh>    mvMeshes;
    string          directory;
    
    void loadModel(string const &path)
    {
        // load a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector

        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));
        if(directory.length() == 0)
            directory = path.substr(0, path.find_last_of('\\'));
        
        // Number of vertices and faces
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
        {
            NbVertices += scene->mMeshes[i]->mNumVertices;
            NbFaces += scene->mMeshes[i]->mNumFaces;
        }

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode *node, const aiScene *scene)
    {
        // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any)
  
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains mvIndices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            mvMeshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        // data to fill
        vector<sVertex>         vVertices;
        vector<unsigned int>    vIndices;
        vector<sTexture>        vTextures;

        // walk through each of the mesh's mvVertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            sVertex vertex;
            vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x; 
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
                vertex.TexCoords = vec2(0.0f, 0.0f);

            vVertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex mvIndices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all mvIndices of the face and store them in the mvIndices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                vIndices.push_back(face.mIndices[j]);        
        }
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        sMaterial mat;
        aiColor4D color(0.0f, 0.0f, 0.0f, 1.0f);

        // Ambient
        mat.ambient = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_AMBIENT, color))
            mat.ambient = vec4(color.r, color.g, color.b, color.a);

        // Diffuse
        mat.diffuse = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color))
            mat.diffuse = vec4(color.r, color.g, color.b, color.a);
        
        // Specular
        mat.specular = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, color))
            mat.specular = vec4(color.r, color.g, color.b, color.a);
        
        // Emission
        mat.emission = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_EMISSIVE, color))
            mat.emission = vec4(color.r, color.g, color.b, color.a);

        // Shininess
        float shininess = 0.0f;
        if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, shininess))
        {
            float strength;
            if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS_STRENGTH, strength))
                mat.shininess = shininess * strength;
            else
                mat.shininess = shininess;
        }
        else
            mat.shininess = 0.0f;

        // Roughness
        float roughness = 0.1f;
        if (AI_SUCCESS == material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness))
            mat.roughness = roughness;
        else
            mat.roughness = 0.1f;

        // Metallic
        float metallic = 0.9f;
        if (AI_SUCCESS == material->Get(AI_MATKEY_METALLIC_FACTOR, metallic))
            mat.metallic = metallic;
        else
            mat.metallic = 0.0f;

        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN

        // 1. diffuse maps
        vector<sTexture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        vTextures.insert(vTextures.end(), diffuseMaps.begin(), diffuseMaps.end());
        bool hasTransparency = false;
        for (const auto& texture : vTextures)
            if (texture.hasTransparency)
                hasTransparency = true;

        // 2. specular maps
        vector<sTexture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        vTextures.insert(vTextures.end(), specularMaps.begin(), specularMaps.end());
        
        // 3. normal maps
        vector<sTexture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        vTextures.insert(vTextures.end(), normalMaps.begin(), normalMaps.end());
        
        // 4. height maps
        vector<sTexture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        vTextures.insert(vTextures.end(), heightMaps.begin(), heightMaps.end());
        
        // return a mesh object created from the extracted mesh data
        return Mesh(vVertices, vIndices, vTextures, mat, hasTransparency);
    }

    unsigned int textureFromFile(const char* path, const string& directory, bool& hasTransparency)
    {
        string filename = string(path);
        filename = directory + '/' + filename;

        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            GLenum format = GL_RGBA;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            // Alpha channel and transparent pixel check
            hasTransparency = false;
            if (nrComponents == 4)
            {
                for (int i = 0; i < width * height; ++i)
                {
                    unsigned char alpha = data[i * 4 + 3];
                    if (alpha < 255)
                    {
                        hasTransparency = true;
                        break;
                    }
                }
            }

            glBindTexture(GL_TEXTURE_2D, textureID);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Essential addition for RGB textures
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else
        {
            cout << "Texture failed to load at path: " << path << endl;
            stbi_image_free(data);
        }

        return textureID;
    }

    vector<sTexture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        // checks all material textures of a given type and loads the textures if they're not loaded yet. The required info is returned as a Texture struct.
        
        vector<sTexture> vTextures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for (unsigned int j = 0; j < mvTextures.size(); j++)
            {
                if (strcmp(mvTextures[j].path.data(), str.C_Str()) == 0)
                {
                    vTextures.push_back(mvTextures[j]);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if (!skip)
            {   // if texture hasn't been loaded already, load it
                sTexture texture;
                texture.id = textureFromFile(str.C_Str(), this->directory, texture.hasTransparency);
                texture.type = typeName;
                texture.path = str.C_Str();
                vTextures.push_back(texture);
                mvTextures.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
            }
        }
        return vTextures;
    }
};

