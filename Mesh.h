/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#pragma once

#include <string>
#include <vector>

#include <glad/glad.h> // holds all OpenGL type declarations
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"


using namespace std;
using namespace glm;

#define MAX_BONE_INFLUENCE 4

struct sVertex 
{
    vec3    Position;                       // position
    vec3    Normal;                         // normal
    vec2    TexCoords;                      // texCoords
    vec3    Tangent;                        // tangent
    vec3    Bitangent;                      // bitangent
    int     m_BoneIDs[MAX_BONE_INFLUENCE];	// bone indexes which will influence this vertex
	float   m_Weights[MAX_BONE_INFLUENCE];  // weights from each bone
};
struct sTexture 
{
    unsigned int    id;
    string          type;
    string          path;
    bool            hasTransparency;
};
struct sMaterial 
{
    vec4    ambient;
    vec4    diffuse;
    vec4    specular;
    vec4    emission;
    float   shininess;
    float   roughness;
    float   metallic;
};

class Mesh 
{
public:
    vector<sVertex>         vVertices;
    vector<unsigned int>    vIndices;
    bool                    HasTransparency;
    vec3                    Center;
    vec3                    TransformedCenter;

    Mesh(vector<sVertex> vertices, vector<unsigned int> indices, vector<sTexture> textures, sMaterial material, bool hasTransparency)
    {
        this->vVertices  = vertices;
        this->vIndices   = indices;
        this->mvTextures  = textures;
        this->material  = material;
        this->HasTransparency = hasTransparency;

        // now that we have all the required data, set the vertex buffers and its attribute pointers
        setupMesh();
        setCenter();
    }
    ~Mesh() { }

    void Render(Shader &shader)
    {
        bool has_texture = (bool)mvTextures.size();
        shader.setBool("has_texture", has_texture);

        shader.setVec4("material.ambient", material.ambient);
        shader.setVec4("material.diffuse", material.diffuse);
        shader.setVec4("material.specular", material.specular);
        shader.setVec4("material.emission", material.emission);
        shader.setFloat("material.shininess", material.shininess);
        shader.setFloat("material.roughness", material.roughness);
        shader.setFloat("material.metallic", material.metallic);

        // bind appropriate textures
        unsigned int diffuseNr  = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr   = 1;
        unsigned int heightNr   = 1;
        for (unsigned int i = 0; i < mvTextures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
            
            // retrieve texture number (the N in diffuse_textureN)
            string number;
            string name = mvTextures[i].type;
            if      (name == "texture_diffuse")     number = to_string(diffuseNr++);
            else if (name == "texture_specular")    number = to_string(specularNr++);   // transfer unsigned int to string
            else if (name == "texture_normal")      number = to_string(normalNr++);     // transfer unsigned int to string
            else if (name == "texture_height")      number = to_string(heightNr++);     // transfer unsigned int to string

            // now set the sampler to the correct texture unit
            glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
            // and finally bind the texture
            glBindTexture(GL_TEXTURE_2D, mvTextures[i].id);
        }

        // For the transparent meshes
        bool isTransparent = (material.diffuse.a < 1.0f);
        if (isTransparent) glDepthMask(GL_FALSE);

        // Draw mesh
        glBindVertexArray(mVAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(vIndices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Reset to default state
        if (isTransparent) glDepthMask(GL_TRUE);

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(0);
    }
    void Bind()
    {
        glBindVertexArray(mVAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(vIndices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:
    vector<sTexture>        mvTextures;
    sMaterial               material;
    unsigned int            mVAO = 0;
    unsigned int            mVBO = 0, mEBO = 0;

    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        // create buffers/arrays
        glGenVertexArrays(1, &mVAO);
        glGenBuffers(1, &mVBO);
        glGenBuffers(1, &mEBO);

        glBindVertexArray(mVAO);
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        glBufferData(GL_ARRAY_BUFFER, vVertices.size() * sizeof(sVertex), &vVertices[0], GL_STATIC_DRAW);  

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, vIndices.size() * sizeof(unsigned int), &vIndices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        
        // vertex Positions
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(sVertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);	
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(sVertex), (void*)offsetof(sVertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);	
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(sVertex), (void*)offsetof(sVertex, TexCoords));
        // vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(sVertex), (void*)offsetof(sVertex, Tangent));
        // vertex bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(sVertex), (void*)offsetof(sVertex, Bitangent));
		// ids
		glEnableVertexAttribArray(5);
		glVertexAttribIPointer(5, 4, GL_INT, sizeof(sVertex), (void*)offsetof(sVertex, m_BoneIDs));
		// weights
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(sVertex), (void*)offsetof(sVertex, m_Weights));
        glBindVertexArray(0);
    }
    void setCenter()
    {
        if (vVertices.empty())
            Center = vec3(0.0f);
        for (const auto& v : vVertices)
            Center += v.Position;
        Center /= static_cast<float>(vVertices.size());
    }
};

