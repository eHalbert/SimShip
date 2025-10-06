/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#pragma once

#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include "Shader.h"

using namespace std;
using namespace glm;

struct sClothPt {
    vec3 pos;
    vec3 prevPos;
    vec3 acceleration;
    bool fixed;
};

class Flag 
{
public:
    Flag(int w, int h, float s, const char* path) : mWidth(w), mHeight(h), mSpacing(s)
    {
        initParticles();
        initBuffers();
        loadTexture(path);
        mShader = make_unique<Shader>("Resources/Ship/flag.vert", "Resources/Ship/flag.frag");
        mShader->use();
        mShader->setInt("flagTexture", 0);
    }
    ~Flag()
    {
        glDeleteBuffers(1, &mVBO);
        glDeleteBuffers(1, &mEBO);
        glDeleteVertexArrays(1, &mVAO);
        glDeleteTextures(1, &mTexture);
    }
 
    void Update(float deltaTime, const vec2& baseWind)
    {
        static int frameCount = 0;
        
        static float time = 0.0f;
        time += deltaTime;
       
        applyForces(time, 3.0f * baseWind);

        int iterations = (frameCount < 10) ? 20 : 3; // more iterations at the start
        for (int i = 0; i < iterations; ++i)
            satisfyConstraints();

        integrate(deltaTime);

        vec3 windDir = glm::normalize(glm::vec3(baseWind.x, 0.0f, baseWind.y));
        float windForce = glm::length(baseWind);
        float strengthAlignment = windForce * 0.002f;
        applyWindAlignmentConstraint(windDir, strengthAlignment);

        updateVerticesBuffer();
        frameCount++;
    }
    void Render(const mat4& model, const mat4& view, const mat4& projection, float exposure)
    {
        mShader->use();
        mShader->setMat4("model", model);
        mShader->setMat4("view", view);
        mShader->setMat4("projection", projection);
        mShader->setFloat("exposure", exposure);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mTexture);

        glBindVertexArray(mVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)mIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:
    void initParticles()
    {
        vClothPts.resize(mWidth * mHeight);
        for (int y = 0; y < mHeight; ++y)
        {
            for (int x = 0; x < mWidth; ++x)
            {
                sClothPt& p = vClothPts[getIndex(x, y)];
                p.pos = vec3(x * mSpacing, -y * mSpacing, 0.0f);
                p.prevPos = p.pos;
                p.acceleration = vec3(0.0f);
                p.fixed = (x == 0);
            }
        }
    }
    void initBuffers()
    {
        // Generate the indices to draw each square of the grid into 2 triangles
        mIndices.clear();
        for (int y = 0; y < mHeight - 1; ++y)
        {
            for (int x = 0; x < mWidth - 1; ++x)
            {
                int i0 = getIndex(x, y);
                int i1 = getIndex(x + 1, y);
                int i2 = getIndex(x, y + 1);
                int i3 = getIndex(x + 1, y + 1);

                // Triangle 1
                mIndices.push_back(i0);
                mIndices.push_back(i1);
                mIndices.push_back(i2);

                // Triangle 2
                mIndices.push_back(i2);
                mIndices.push_back(i1);
                mIndices.push_back(i3);
            }
        }

        // Generate the data table with positions + UVs
        vector<float> vertices;
        vertices.reserve(mWidth * mHeight * 5); // 3 for pos + 2 for UV
        for (int y = 0; y < mHeight; ++y)
        {
            for (int x = 0; x < mWidth; ++x)
            {
                sClothPt& p = vClothPts[getIndex(x, y)];
                vertices.push_back(p.pos.x);
                vertices.push_back(p.pos.y);
                vertices.push_back(p.pos.z);
                vertices.push_back(float(x) / (mWidth - 1));  // U
                vertices.push_back(float(y) / (mHeight - 1)); // V
            }
        }

        glGenVertexArrays(1, &mVAO);
        glGenBuffers(1, &mVBO);
        glGenBuffers(1, &mEBO);
        glBindVertexArray(mVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0); // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1); // uv
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(GLuint), mIndices.data(), GL_STATIC_DRAW);
        glBindVertexArray(0);
    }
    void loadTexture(const char* path)
    {
        glGenTextures(1, &mTexture);
        glBindTexture(GL_TEXTURE_2D, mTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        int width, height, nrChannels;
        unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
        if (data)
        {
            GLenum format = nrChannels == 4 ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else
        {
            std::cerr << "Failed to load texture at " << path << std::endl;
        }
        stbi_image_free(data);
    }
    void updateVerticesBuffer()
    {
        vector<float> vertices;
        vertices.reserve(vClothPts.size() * 5);
        for (int y = 0; y < mHeight; ++y)
        {
            for (int x = 0; x < mWidth; ++x)
            {
                sClothPt& p = vClothPts[getIndex(x, y)];
                vertices.push_back(p.pos.x);
                vertices.push_back(p.pos.y);
                vertices.push_back(p.pos.z);
                vertices.push_back(float(x) / (mWidth - 1));
                vertices.push_back(float(y) / (mHeight - 1));
            }
        }
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    }
    vec3 computeTriangleNormal(int idx0, int idx1, int idx2) const
    {
        const vec3& p0 = vClothPts[idx0].pos;
        const vec3& p1 = vClothPts[idx1].pos;
        const vec3& p2 = vClothPts[idx2].pos;
        vec3 edge1 = p1 - p0;
        vec3 edge2 = p2 - p0;
        return normalize(cross(edge1, edge2));
    }
    
    void applyForces(float time, const vec2& wind)
    {
        vec3 windForceBase = vec3(wind.x, 0.0f, wind.y);

        // Gravité
        for (auto& p : vClothPts)
            p.acceleration.y -= 9.81f;

        // Force du vent selon la normale
        for (size_t i = 0; i < mIndices.size(); i += 3)
        {
            int i0 = mIndices[i];
            int i1 = mIndices[i + 1];
            int i2 = mIndices[i + 2];
            vec3 normal = computeTriangleNormal(i0, i1, i2);
            float intensity = glm::max(dot(normal, windForceBase), 0.0f);
            vec3 force = normal * intensity;
            vClothPts[i0].acceleration += force;
            vClothPts[i1].acceleration += force;
            vClothPts[i2].acceleration += force;
        }
    }
    void integrate(float deltaTime)
    {
        float damping = 0.99f;
        for (auto& p : vClothPts)
        {
            if (!p.fixed)
            {
                vec3 temp = p.pos;
                p.pos += (p.pos - p.prevPos) * damping + p.acceleration * deltaTime * deltaTime;
                p.prevPos = temp;
                p.acceleration = vec3(0.0f);
            }
        }
    }
    int getIndex(int x, int y) const { return y * mWidth + x; }
    void applyConstraint(int idx1, int idx2, float restLength, float rigidity)
    {
        vec3 diff = vClothPts[idx2].pos - vClothPts[idx1].pos;
        float dist = glm::length(diff);
        vec3 correction = diff * (1.0f - restLength / dist) * 0.5f * rigidity;
        if (!vClothPts[idx1].fixed) 
            vClothPts[idx1].pos += correction;
        if (!vClothPts[idx2].fixed) 
            vClothPts[idx2].pos -= correction;
    }
    void satisfyConstraints()
    {
        float restLength = mSpacing;
        float rigidity = 0.5f;

        for (int y = 0; y < mHeight; ++y)
        {
            for (int x = 0; x < mWidth; ++x)
            {
                int idx = getIndex(x, y);

                // Structural springs (adjacent points)
                if (x < mWidth - 1)
                {
                    int right = getIndex(x + 1, y);
                    applyConstraint(idx, right, restLength, rigidity);
                }
                if (y < mHeight - 1)
                {
                    int down = getIndex(x, y + 1);
                    applyConstraint(idx, down, restLength, rigidity);
                }
                // Flexion springs (2-point skip)
                if (x < mWidth - 2)
                {
                    int right2 = getIndex(x + 2, y);
                    applyConstraint(idx, right2, restLength * 2.0f, rigidity);
                }
                if (y < mHeight - 2)
                {
                    int down2 = getIndex(x, y + 2);
                    applyConstraint(idx, down2, restLength * 2.0f, rigidity);
                }
                // Diagonal springs (shear constraints)
                if (x < mWidth - 1 && y < mHeight - 1) 
                {
                    int diag1 = getIndex(x + 1, y + 1);
                    applyConstraint(idx, diag1, restLength * sqrt(2.0f), rigidity);
                }
                if (x < mWidth - 1 && y > 0) 
                {
                    int diag2 = getIndex(x + 1, y - 1);
                    applyConstraint(idx, diag2, restLength * sqrt(2.0f), rigidity);
                }
            }
        }
    }
    void applyWindAlignmentConstraint(const vec3& windDir, float strength)
    {
        for (int y = 0; y < mHeight; ++y)
        {
            int idx_fixed = getIndex(0, y);     // Index du point fixé sur la colonne x=0 à la même hauteur
            vec3 origin = vClothPts[idx_fixed].pos;

            for (int x = 1; x < mWidth; ++x)    // Commence à x=1 (particules non fixes)
            {
                int idx = getIndex(x, y);
                sClothPt& p = vClothPts[idx];

                // Projette sur l’axe du vent partant de 'origin'
                vec3 delta = p.pos - origin;
                float projLen = glm::dot(delta, windDir);
                vec3 proj = origin + windDir * projLen;

                vec3 corr = proj - p.pos;
                p.pos += corr * strength;
            }
        }
    }
    
    int                 mWidth, mHeight;
    float               mSpacing;
    vector<sClothPt>    vClothPts;
    GLuint              mVAO = 0, mVBO = 0, mEBO = 0;
    vector<GLuint>      mIndices;
    GLuint              mTexture = 0;
    unique_ptr<Shader>  mShader;
};
