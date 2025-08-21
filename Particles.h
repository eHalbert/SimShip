#pragma once

#include "Shader.h"
#include "Camera.h"

#include <glm/glm.hpp>
#include <vector>

using namespace std;
using namespace glm;


struct ParticleGPU 
{
    vec3    position;
    vec3    velocity;
    float   life;
    vec4    color;
};

class Spray
{
public:
    bool bVisible = true;

    Spray()
    {
        lifeSpan = (int)((longLife - shortLife) * 10.0f) + 1;

        mvParticles.resize(mMaxParticles);

        // Initializing the MultiMesh
        glGenBuffers(1, &mVBO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferData(GL_ARRAY_BUFFER, mMaxParticles * sizeof(ParticleGPU), nullptr, GL_DYNAMIC_DRAW);

        glGenVertexArrays(1, &mVAO);
        glBindVertexArray(mVAO);

        // Position (attribute 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleGPU), (void*)0);
        glVertexAttribDivisor(0, 1);

        // Velocity (attribute 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleGPU), (void*)(3 * sizeof(float)));
        glVertexAttribDivisor(1, 1);

        // Life (attribute 2)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleGPU), (void*)(6 * sizeof(float)));
        glVertexAttribDivisor(2, 1);

        // Color (attribute 3)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleGPU), (void*)(7 * sizeof(float)));
        glVertexAttribDivisor(3, 1);

        glBindVertexArray(0);

        glDepthMask(GL_FALSE);
        mShader = make_unique<Shader>("Resources/Ship/spray.vert", "Resources/Ship/spray.frag");
        glDepthMask(GL_TRUE);
    }

    ~Spray()
    {
        glDeleteBuffers(1, &mVBO);
        glDeleteVertexArrays(1, &mVAO);
    }

    void Emit(vec3 position, vec3 velocity)
    {
        if (mActiveParticles >= mMaxParticles)
            return;

        vec3 randomOffset = vec3( (rand() % 100 - 50) / 1000.0f, (rand() % 100 - 50) / 1000.0f, (rand() % 100 - 50) / 1000.0f );
        mvParticles[mActiveParticles].position = position + randomOffset;

        // Ajout de variation aléatoire pour un mouvement naturel
        vec3 randomVelocity = vec3( (rand() % 100 - 50) / 500.0f, (rand() % 100 - 50) / 500.0f, (rand() % 100 - 50) / 500.0f );
        mvParticles[mActiveParticles].velocity = velocity + randomVelocity;
        
        mvParticles[mActiveParticles].life = shortLife + (rand() % lifeSpan) / 10.0f;

        float gray = 0.8f + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 0.2f;
        mvParticles[mActiveParticles].color = vec4(gray, gray, gray, 0.8f);

        mActiveParticles++;
    }
    void Update(float deltaTime)
    {
        const float gravity = 9.81f;

        for (int i = 0; i < mActiveParticles; ++i)
        {
            mvParticles[i].life -= deltaTime;
            if (mvParticles[i].life < 0)   // Underwater
            {
                // Remplace la particule morte par la dernière
                mvParticles[i] = mvParticles[mActiveParticles - 1];
                mActiveParticles--;
                i--;
                continue;
            }

            // Applique la gravité (descente verticale)
            mvParticles[i].velocity.y -= gravity * deltaTime;

            // Turbulence légère
            float turbulenceStrength = 50.0f;
            mvParticles[i].velocity += turbulenceStrength * vec3( (rand() % 100 - 50) / 100.0f, (rand() % 100 - 50) / 100.0f, (rand() % 100 - 50) / 100.0f ) * deltaTime;

            // Mise à jour position
            mvParticles[i].position += mvParticles[i].velocity * deltaTime;
        }

        UpdateParticleBuffers();
    }

    void UpdateParticleBuffers()
    {
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, mActiveParticles * sizeof(ParticleGPU), mvParticles.data());
    }
    float GetNbActiveParticles()
    {
        return mActiveParticles;
    }
    void Render(Camera& camera, float density, float exposure)
    {
        if (!bVisible)
            return;

        mShader->use();
        mShader->setMat4("view", camera.GetView());
        mShader->setMat4("projection", camera.GetProjection());
        mShader->setFloat("density", density);
        mShader->setFloat("lifeSpan", lifeSpan);
        mShader->setFloat("exposure", exposure);

        glBindVertexArray(mVAO);
        glDrawArraysInstanced(GL_POINTS, 0, 1, mActiveParticles);
        glBindVertexArray(0);
    }

private:
    static const int    mMaxParticles = 50000;

    vector<ParticleGPU> mvParticles;
    GLuint              mVBO;
    GLuint              mVAO;
    unique_ptr<Shader>  mShader;
    int                 mActiveParticles = 0;
    const float         shortLife = 1.0f;
    const float         longLife = 2.0f;
    int                 lifeSpan;
};
