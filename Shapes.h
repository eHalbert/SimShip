/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#pragma once

#define NOMINMAX
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>

#include <glad/glad.h>
#include <stb/stb_image.h>
#include <glm/glm.hpp>

#include "Shader.h"
#include "Camera.h"

using namespace std;
using namespace glm;

class BBox
{
public:
    BBox(glm::vec3 min, glm::vec3 max)
    {
        // Définir les sommets de la bounding box
        vector<GLfloat> vertices =
        {
            min.x, min.y, min.z,  // 0
            max.x, min.y, min.z,  // 1
            max.x, max.y, min.z,  // 2
            min.x, max.y, min.z,  // 3
            min.x, min.y, max.z,  // 4
            max.x, min.y, max.z,  // 5
            max.x, max.y, max.z,  // 6
            min.x, max.y, max.z   // 7
        };

        vector<GLuint> indices =
        {
            0, 1,  // Arête avant bas
            1, 2,  // Arête avant droite
            2, 3,  // Arête avant haut
            3, 0,  // Arête avant gauche
            4, 5,  // Arête arrière bas
            5, 6,  // Arête arrière droite
            6, 7,  // Arête arrière haut
            7, 4,  // Arête arrière gauche
            0, 4,  // Arête gauche bas
            1, 5,  // Arête droite bas
            2, 6,  // Arête droite haut
            3, 7   // Arête gauche haut        
        };
        GLuint VBO, EBO;
        glGenVertexArrays(1, &mVAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(mVAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        mShader = make_unique<Shader>("Resources/Shaders/unicolor.vert", "Resources/Shaders/unicolor.frag");

        bVisible = true;
    };
    ~BBox()
    {
        glDeleteVertexArrays(1, &mVAO);
    };
    void    Render(Camera& camera, glm::vec3 pos, float scale, glm::vec3 color)
    {
        if (mShader == nullptr)
            return;

        if (!bVisible)
            return;

        glUseProgram(mShader->ID);
        mShader->setMat4("view", camera.GetView());
        mShader->setMat4("projection", camera.GetProjection());
        mat4 model = glm::translate(mat4(1.0f), pos);
        model = glm::scale(model, glm::vec3(scale));
        mShader->setMat4("model", model);
        mShader->setVec3("lineColor", color);

        glBindVertexArray(mVAO);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    void    Bind()
    {
        glBindVertexArray(mVAO);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    };
    bool    bVisible;
private:
    GLuint              mVAO;
    unique_ptr<Shader>  mShader;
};

class Cube
{
public:
    Cube()
    {
        float vertices[] =
        {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
            1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };

        unsigned int VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glGenVertexArrays(1, &mVao);
        glBindVertexArray(mVao);
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // UV coordinates
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

        mShader = new Shader("Resources/Shaders/unicolor.vert", "Resources/Shaders/unicolor.frag");
        bVisible = true;
    };
    ~Cube()
    {
        glDeleteVertexArrays(1, &mVao);
    };
    void    Render(Camera& camera, vec3 pos, float scale, vec3 color)
    {
        if (mShader == nullptr)
            return;

        if (!bVisible)
            return;

        glUseProgram(mShader->ID);
        mShader->setMat4("view", camera.GetView());
        mShader->setMat4("projection", camera.GetProjection());
        mat4 model = mat4(1.0f);
        model = glm::translate(model, pos);
        model = glm::scale(model, vec3(scale));
        mShader->setMat4("model", model);
        mShader->setVec3("lineColor", color);

        glBindVertexArray(mVao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
    void    RenderDistorted(Camera& camera, vec3 pos, vec3 scale, vec3 color)
    {
        if (mShader == nullptr)
            return;

        if (!bVisible)
            return;

        glUseProgram(mShader->ID);
        mShader->setMat4("view", camera.GetView());
        mShader->setMat4("projection", camera.GetProjection());
        mat4 model = mat4(1.0f);
        model = glm::translate(model, pos);
        model = glm::scale(model, scale);
        mShader->setMat4("model", model);
        mShader->setVec3("lineColor", color);

        glBindVertexArray(mVao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
    void    RenderDistorted(Camera& camera, mat4& model, vec3 color)
    {
        if (mShader == nullptr)
            return;

        if (!bVisible)
            return;

        glUseProgram(mShader->ID);
        mShader->setMat4("view", camera.GetView());
        mShader->setMat4("projection", camera.GetProjection());
        mShader->setMat4("model", model);
        mShader->setVec3("lineColor", color);

        glBindVertexArray(mVao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    void    Bind()
    {
        glBindVertexArray(mVao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    };
    bool    bVisible;
private:
    unsigned int  mVao;
    Shader      * mShader = nullptr;
};

class Grid
{
public:
    Grid(unsigned int gridSize, unsigned int cellSize)
    {
        this->mGridSize = gridSize;
        this->mCellSize = cellSize;

        float halfSize = (gridSize * cellSize) / 2.0f;

        // Lines parallel to the X axis
        for (int i = 0; i <= gridSize; ++i)
        {
            float z = -halfSize + i * cellSize;
            mvVertices.push_back(-halfSize);  // x start
            mvVertices.push_back(0.0f);       // y (always 0 for a flat grid)
            mvVertices.push_back(z);          // z
            mvVertices.push_back(halfSize);   // x end
            mvVertices.push_back(0.0f);       // y
            mvVertices.push_back(z);          // z
        }

        // Lines parallel to the Z axis
        for (int i = 0; i <= gridSize; ++i)
        {
            float x = -halfSize + i * cellSize;
            mvVertices.push_back(x);          // x
            mvVertices.push_back(0.0f);       // y
            mvVertices.push_back(-halfSize);  // z start
            mvVertices.push_back(x);          // x
            mvVertices.push_back(0.0f);       // y
            mvVertices.push_back(halfSize);   // z end
        }

        unsigned int VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, mvVertices.size() * sizeof(float), mvVertices.data(), GL_STATIC_DRAW);

        glGenVertexArrays(1, &mVao);
        glBindVertexArray(mVao);
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        mShader = make_unique<Shader>("Resources/Shaders/unicolor.vert", "Resources/Shaders/unicolor.frag");

        bVisible = true;
    };
    ~Grid()
    {
        glDeleteVertexArrays(1, &mVao);
    };
    void    Render(Camera& camera, vec3 pos, float scale, vec3 color)
    {
        if (mShader == nullptr)
            return;

        if (!bVisible)
            return;

        glUseProgram(mShader->ID);
        mShader->setMat4("view", camera.GetView());
        mShader->setMat4("projection", camera.GetProjection());
        mat4 model = mat4(1.0f);
        model = glm::translate(model, pos);
        model = glm::scale(model, vec3(scale));
        mShader->setMat4("model", model);
        mShader->setVec3("lineColor", color);

        glBindVertexArray(mVao);
        glDrawArrays(GL_LINES, 0, mvVertices.size() / 3);

        glBindVertexArray(0);
    };
    void    Render(Camera& camera, mat4 model, vec3 color)
    {
        if (mShader == nullptr)
            return;

        if (!bVisible)
            return;

        glUseProgram(mShader->ID);
        mShader->setMat4("view", camera.GetView());
        mShader->setMat4("projection", camera.GetProjection());
        mShader->setMat4("model", model);
        mShader->setVec3("lineColor", color);

        glBindVertexArray(mVao);
        glDrawArrays(GL_LINES, 0, mvVertices.size() / 3);

        glBindVertexArray(0);
    };
    bool    bVisible;
private:
    vector<float>       mvVertices;
    int                 mGridSize = 10;      // taille de la grille
    float               mCellSize = 0.2f;    // taille d'une cellule
    unsigned int        mVao;
    unique_ptr<Shader>  mShader;
};

class Plane
{
public:
    Plane()
    {
        float vertices[] =
        {
            // positions + texcoords
             1.0f, -0.0f,  1.0f,  1.0f, 0.0f,
            -1.0f, -0.0f,  1.0f,  0.0f, 0.0f,
            -1.0f, -0.0f, -1.0f,  0.0f, 1.0f,

             1.0f, -0.0f,  1.0f,  1.0f, 0.0f,
            -1.0f, -0.0f, -1.0f,  0.0f, 1.0f,
             1.0f, -0.0f, -1.0f,  1.0f, 1.0f
        };

        unsigned int VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

        glGenVertexArrays(1, &mVao);
        glBindVertexArray(mVao);

        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        // Normal attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glBindVertexArray(0);

        mShader = make_unique<Shader>("Resources/Shaders/unicolor.vert", "Resources/Shaders/unicolor.frag");

        bVisible = true;
    };
    ~Plane()
    {
        glDeleteVertexArrays(1, &mVao);
    };
    void    Render(Camera& camera, vec3 pos, float scale, vec3 color)
    {
        if (mShader == nullptr)
            return;

        if (!bVisible)
            return;

        glUseProgram(mShader->ID);
        mShader->setMat4("view", camera.GetView());
        mShader->setMat4("projection", camera.GetProjection());
        mat4 model = glm::translate(mat4(1.0f), pos);
        model = glm::scale(model, vec3(scale));
        mShader->setMat4("model", model);
        mShader->setVec3("lineColor", color);

        glBindVertexArray(mVao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
    void    Bind()
    {
        glBindVertexArray(mVao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    };
    bool    bVisible;
private:
    unsigned int        mVao;
    unique_ptr<Shader>  mShader;
};

class Disk
{
public:
    Disk(int segments = 32)
    {
        vector<float> vertices;
        float radius = 1.0f;

        // Disk Center
        vertices.push_back(0.0f); // x
        vertices.push_back(0.0f); // y
        vertices.push_back(0.0f); // z
        vertices.push_back(0.5f); // u
        vertices.push_back(0.5f); // v

        // Generate Disk mvVertices
        for (int i = 0; i <= segments; ++i)
        {
            float theta = 2.0f * M_PI * float(i) / float(segments);
            float x = radius * cosf(theta);
            float z = radius * sinf(theta);

            vertices.push_back(x);    // x
            vertices.push_back(0.0f); // y
            vertices.push_back(z);    // z
            vertices.push_back((x + 1.0f) * 0.5f); // u
            vertices.push_back((z + 1.0f) * 0.5f); // v
        }

        glGenVertexArrays(1, &mVao);
        glBindVertexArray(mVao);

        unsigned int VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        // Texture coordinate attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        glBindVertexArray(0);

        mSegments = segments;
        mShader = make_unique<Shader>("Resources/Shaders/unicolor.vert", "Resources/Shaders/unicolor.frag");
        bVisible = true;
    }
    ~Disk()
    {
        glDeleteVertexArrays(1, &mVao);
    }
    void    Render(Camera& camera, vec3 pos, float scale, vec3 color)
    {
        if (mShader == nullptr || !bVisible)
            return;

        glUseProgram(mShader->ID);
        mShader->setMat4("view", camera.GetView());
        mShader->setMat4("projection", camera.GetProjection());
        mat4 model = glm::translate(mat4(1.0f), pos);
        model = glm::scale(model, vec3(scale));
        mShader->setMat4("model", model);
        mShader->setVec3("lineColor", color);

        glBindVertexArray(mVao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, mSegments + 2);
        glBindVertexArray(0);
    }
    void    Bind()
    {
        glBindVertexArray(mVao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, mSegments + 2);
        glBindVertexArray(0);
    };
    bool    bVisible;
private:
    unsigned int        mVao;
    unique_ptr<Shader>  mShader;
    int                 mSegments;
};

class Cylinder
{
public:
    Cylinder(float radius = 1.0f, float height = 2.0f, int segments = 32)
        : mRadius(radius), mHeight(height), mSegments(segments)
    {
        vector<float> vertices;

        // Generate the mvVertices for the sides of the cylinder
        for (int i = 0; i <= segments; ++i)
        {
            float theta = 2.0f * M_PI * float(i) / float(segments);
            float x = radius * cosf(theta);
            float z = radius * sinf(theta);

            // Lower vertex
            vertices.push_back(x);
            vertices.push_back(0.0f);
            vertices.push_back(z);
            vertices.push_back(float(i) / float(segments));
            vertices.push_back(0.0f);

            // Upper vertex
            vertices.push_back(x);
            vertices.push_back(height);
            vertices.push_back(z);
            vertices.push_back(float(i) / float(segments));
            vertices.push_back(1.0f);
        }

        // Generate the mvVertices for the cylinder bases
        for (int base = 0; base < 2; ++base)
        {
            float y = base * height;
            vertices.push_back(0.0f);
            vertices.push_back(y);
            vertices.push_back(0.0f);
            vertices.push_back(0.5f);
            vertices.push_back(0.5f);

            for (int i = 0; i <= segments; ++i)
            {
                float theta = 2.0f * M_PI * float(i) / float(segments);
                float x = radius * cosf(theta);
                float z = radius * sinf(theta);

                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);
                vertices.push_back((x / radius + 1.0f) * 0.5f);
                vertices.push_back((z / radius + 1.0f) * 0.5f);
            }
        }

        glGenVertexArrays(1, &mVao);
        glBindVertexArray(mVao);

        unsigned int VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        // Texture coordinate attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        glBindVertexArray(0);

        mShader = make_unique<Shader>("Resources/Shaders/unicolor.vert", "Resources/Shaders/unicolor.frag");
        bVisible = true;
    }
    ~Cylinder()
    {
        glDeleteVertexArrays(1, &mVao);
    }
    void    Render(Camera& camera, vec3 pos, float scale, vec3 color)
    {
        if (mShader == nullptr || !bVisible)
            return;

        glUseProgram(mShader->ID);
        mShader->setMat4("view", camera.GetView());
        mShader->setMat4("projection", camera.GetProjection());
        mat4 model = glm::translate(mat4(1.0f), pos);
        model = glm::scale(model, vec3(scale));
        mShader->setMat4("model", model);
        mShader->setVec3("lineColor", color);

        glBindVertexArray(mVao);

        // Draw the sides of the cylinder
        glDrawArrays(GL_TRIANGLE_STRIP, 0, (mSegments + 1) * 2);

        // Draw the bases of the cylinder
        int baseOffset = (mSegments + 1) * 2;
        glDrawArrays(GL_TRIANGLE_FAN, baseOffset, mSegments + 2);
        glDrawArrays(GL_TRIANGLE_FAN, baseOffset + mSegments + 2, mSegments + 2);

        glBindVertexArray(0);
    }
    bool    bVisible;
private:
    unsigned int        mVao;
    unique_ptr<Shader>  mShader;
    int                 mSegments;
    float               mRadius;
    float               mHeight;
};

class Sphere
{
public:
    Sphere(float radius, int segments)
    {
        vector<vec3> vertices;
        vector<vec3> normals;
        vector<vec2> texcoords;

        for (int y = 0; y <= segments; y++)
        {
            for (int x = 0; x <= segments; x++)
            {
                float xSegment = (float)x / (float)segments;
                float ySegment = (float)y / (float)segments;
                float xPos = cos(xSegment * 2.0f * glm::pi<float>()) * sin(ySegment * glm::pi<float>());
                float yPos = cos(ySegment * glm::pi<float>());
                float zPos = sin(xSegment * 2.0f * glm::pi<float>()) * sin(ySegment * glm::pi<float>());

                vec3 position = vec3(xPos, yPos, zPos) * radius;
                vertices.push_back(position);

                // The normal vector is simply the normalized position for a sphere
                normals.push_back(normalize(position));

                // Adding texture coordinates
                texcoords.push_back(vec2(xSegment, ySegment));
            }
        }

        for (int y = 0; y < segments; y++)
        {
            for (int x = 0; x < segments; x++)
            {
                mvIndices.push_back((y + 1) * (segments + 1) + x);
                mvIndices.push_back(y * (segments + 1) + x);
                mvIndices.push_back(y * (segments + 1) + x + 1);

                mvIndices.push_back((y + 1) * (segments + 1) + x);
                mvIndices.push_back(y * (segments + 1) + x + 1);
                mvIndices.push_back((y + 1) * (segments + 1) + x + 1);
            }
        }

        // Creating buffers
        GLuint VBO, NBO, TBO, EBO;

        glGenVertexArrays(1, &mVAO);
        glBindVertexArray(mVAO);

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), &vertices[0], GL_STATIC_DRAW);

        glGenBuffers(1, &NBO);
        glBindBuffer(GL_ARRAY_BUFFER, NBO);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), &normals[0], GL_STATIC_DRAW);

        glGenBuffers(1, &TBO);
        glBindBuffer(GL_ARRAY_BUFFER, TBO);
        glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(vec2), &texcoords[0], GL_STATIC_DRAW);

        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mvIndices.size() * sizeof(GLuint), &mvIndices[0], GL_STATIC_DRAW);

        // Position attribute
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
        glEnableVertexAttribArray(0);

        // Normal attribute
        glBindBuffer(GL_ARRAY_BUFFER, NBO);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
        glEnableVertexAttribArray(1);

        // Texture coordinate attribute
        glBindBuffer(GL_ARRAY_BUFFER, TBO);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)0);
        glEnableVertexAttribArray(2);

        mShader = make_unique<Shader>("Resources/Shaders/unicolor.vert", "Resources/Shaders/unicolor.frag");

        bVisible = true;
    };
    ~Sphere()
    {
        glDeleteVertexArrays(1, &mVAO);
    };
    void    Render(Camera& camera, vec3 pos, float scale, vec3 color)
    {
        if (mShader == nullptr)
            return;

        if (!bVisible)
            return;

        glUseProgram(mShader->ID);  // Shaders/unicolor.vert, Shaders/unicolor.frag
        mShader->setMat4("view", camera.GetView());
        mShader->setMat4("projection", camera.GetProjection());
        mat4 model = glm::translate(mat4(1.0f), pos);
        model = glm::scale(model, vec3(scale));
        mShader->setMat4("model", model);
        mShader->setVec3("lineColor", color);

        glBindVertexArray(mVAO);
        glDrawElements(GL_TRIANGLES, mvIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    };
    void    Bind()
    {
        glBindVertexArray(mVAO);
        glDrawElements(GL_TRIANGLES, mvIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    };
    bool    bVisible;
private:
    GLuint              mVAO;
    unique_ptr<Shader>  mShader;
    vector<GLuint>      mvIndices;
};

class Surface
{
public:
    Surface(string texture = "")
    {
        float vertices[] = {
            // positions    // texture coordinates
            -1.0f, -1.0f,   0.0f, 0.0f,
             1.0f, -1.0f,   1.0f, 0.0f,
            -1.0f,  1.0f,   0.0f, 1.0f,
             1.0f,  1.0f,   1.0f, 1.0f
        };

        GLuint VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glGenVertexArrays(1, &mVAO);
        glBindVertexArray(mVAO);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        mShader = make_unique<Shader>("Resources/Shaders/quad.vert", "Resources/Shaders/quad.frag");

        if (texture.length())
            mTextureID = LoadTexture(texture.c_str());

        bVisible = true;
    };
    ~Surface()
    {
        glDeleteVertexArrays(1, &mVAO);
    };
    unsigned int LoadTexture(const char* path)
    {
        string filename = string(path);

        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        stbi_set_flip_vertically_on_load(true);
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

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            stbi_set_flip_vertically_on_load(false);
        }
        else
        {
            cout << "Texture failed to load at path: " << path << endl;
            stbi_image_free(data);
        }

        return textureID;
    }
    void    Render(vec3 color = vec3(0.0, 0.0, 0.0))
    {
        if (mShader == nullptr)
            return;

        if (!bVisible)
            return;

        glUseProgram(mShader->ID);

        // if texture or color
        if (mTextureID)
        {
            glBindTexture(GL_TEXTURE_2D, mTextureID);
            mShader->setVec3("color", vec3(0.0, 0.0, 0.0));
        }
        else
            mShader->setVec3("color", color);

        glBindVertexArray(mVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    };
    void    Bind()
    {
        glBindVertexArray(mVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    };
    bool    bVisible;
private:
    GLuint              mVAO;
    unique_ptr<Shader>  mShader;
    GLuint              mTextureID = 0;
};
