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

class Smoke 
{
public:
    bool bVisible = true;

    Smoke() 
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
        mShader = make_unique<Shader>("Resources/Ship/particles.vert", "Resources/Ship/particles.frag");
        glDepthMask(GL_TRUE);
    }

    ~Smoke() 
    {
        glDeleteBuffers(1, &mVBO);
        glDeleteVertexArrays(1, &mVAO);
    }

    void Emit(vec3 position, vec3 direction)
    {
        if (mActiveParticles < mMaxParticles)
        {
            // Added a slight random variation to the emission position
            vec3 randomOffset = vec3(
                (rand() % 100 - 50) / 1000.0f,
                (rand() % 100 - 50) / 1000.0f,
                (rand() % 100 - 50) / 1000.0f
            );

            mvParticles[mActiveParticles].position = position + randomOffset;

            // Added random variation to direction and speed
            vec3 randomVelocity = vec3(
                (rand() % 100 - 50) / 500.0f,
                (rand() % 200) / 500.0f,  // Tendency to rise
                (rand() % 100 - 50) / 500.0f
            );
            mvParticles[mActiveParticles].velocity = direction + randomVelocity;

            mvParticles[mActiveParticles].life = shortLife + (rand() % lifeSpan) / 10.0f;

            // Gray color with slight variation
            float grayShade = 0.3f + (rand() % 10) / 1000.0f;
            mvParticles[mActiveParticles].color = vec4(grayShade, grayShade, grayShade, 0.8f);

            mActiveParticles++;
        }
    }

    void Update(float deltaTime, vec3& windDirection, float windStrength)
    {
        for (int i = 0; i < mActiveParticles; ++i)
        {
            mvParticles[i].life -= deltaTime;
            if (mvParticles[i].life < 0)
            {
                // Replace the dead particle with the last active one
                mvParticles[i] = mvParticles[mActiveParticles - 1];
                mActiveParticles--;
                i--; // Check this position again
                continue;
            }

            // Add thermal lift force
            mvParticles[i].velocity.y += 0.5f * deltaTime;

            // Add a gentle wind effect
            mvParticles[i].velocity += windDirection * (windStrength * 0.5f) * deltaTime;

            // Add light turbulence
            float turbulenceStrength = 20.0f;
            mvParticles[i].velocity += turbulenceStrength * glm::vec3(
                (rand() % 100 - 50) / 100.0f,
                (rand() % 100 - 50) / 100.0f,
                (rand() % 100 - 50) / 100.0f
            ) * deltaTime;

            // Position update
            mvParticles[i].position += mvParticles[i].velocity * deltaTime;
        }

        UpdateParticleBuffers();
    }

    void UpdateParticleBuffers() 
    {
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, mActiveParticles * sizeof(ParticleGPU), mvParticles.data());
    }

    void Render(Camera& camera, float density)
    {
        if (!bVisible)
            return;

        mShader->use();
        mShader->setMat4("view", camera.GetView());
        mShader->setMat4("projection", camera.GetProjection());
        mShader->setFloat("density", density);
        mShader->setFloat("lifeSpan", lifeSpan);

        glBindVertexArray(mVAO);
        glDrawArraysInstanced(GL_POINTS, 0, 1, mActiveParticles);
        glBindVertexArray(0);
    }

private:
    static const int    mMaxParticles = 10000;
    
    vector<ParticleGPU> mvParticles;
    GLuint              mVBO;
    GLuint              mVAO;
    unique_ptr<Shader>  mShader;
    int                 mActiveParticles = 0;
    const float         shortLife = 5.0f;
    const float         longLife = 10.0f;
    int                 lifeSpan;
};
