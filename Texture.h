#pragma once

#define NOMINMAX
#include <vector>

#include <glad/glad.h>
#include <gli/gli/gli.hpp>
#include <glm/glm.hpp>

#include "Shader.h"

using namespace std;
using namespace glm;


struct Texture
{
    Texture() { width = 256; height = 256; internal_format = GL_RGBA32F; };
    Texture( unsigned int width_, unsigned int height_, GLint internal_format_, GLenum format, GLenum type, GLint min_filter = GL_NEAREST, GLint max_filter = GL_NEAREST, GLint wrap_r = GL_CLAMP_TO_BORDER, GLint wrap_s = GL_CLAMP_TO_BORDER, const GLvoid* data = 0);

    void Init(unsigned int width_, unsigned int height_, GLint internal_format_, GLenum format, GLenum type, GLint min_filter, GLint max_filter, GLint wrap_r, GLint wrap_s, const GLvoid* data);
    void CreateFromFile(const char* path, bool flip_vertically = false);
    void CreateFromDDSFile(const char* path);
	void SetWrappingParams(GLint wrap_r, GLint wrap_s);
	unsigned int LoadCubemapWith6Faces(vector<string> faces);

    inline void Bind(const unsigned int unit = 0) const
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }
    inline void Unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }
    void BindImage(GLuint unit, GLenum access, GLenum format) const;

    ~Texture();

    GLuint          id = GLuint(-1);
    unsigned int    width;
    unsigned int    height;
    GLint           internal_format;

    int             depth = 0;  // texture 3D from dds
    bool            is_cubemap = false;
};

void SaveTexture2D(GLuint texture, int width, int height, int channels, int format, string name);
void SaveDepthTexture2D(GLuint texture, int width, int height, std::string name);
void SaveTexture3D(GLuint texture, string name);

class Quad {
private:
    unsigned int VAO, VBO;
    vector<float> vertices;

public:
    Quad()
    {
        float vertices[] =
        {
            -1.0f, -1.0f, 0.0f, 0.0f,  // Vertex 1
             1.0f, -1.0f, 1.0f, 0.0f,  // Vertex 2
            -1.0f,  1.0f, 0.0f, 1.0f,  // Vertex 3
             1.0f,  1.0f, 1.0f, 1.0f   // Vertex 4
        };

        // Generate and link the mVAO
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        // Generate and link the VBO
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        // Copy the data from the mvVertices into the VBO
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Configure the vertex attribute (position)
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Unbind the mVAO and VBO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    ~Quad()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    void Render()
    {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }
};

class QuadTexture
{
private:
	unique_ptr<Shader>	mShader;
	unsigned int		mVertexlayout;

public:
	static enum Channels
	{
		RGBA = 0,
		RGB,
		RG,
		R,
		G,
		B,
		A
	};
	QuadTexture()
	{
		mShader = nullptr;
		mVertexlayout = 0;

		glGenVertexArrays(1, &mVertexlayout);

		glBindVertexArray(mVertexlayout);
		{
			// empty (will be generated in mShader)
		}
		glBindVertexArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		mShader = make_unique<Shader>("Resources/Shaders/quad_texture.vert", "Resources/Shaders/quad_texture.frag");
	}
	~QuadTexture()
	{
		if (mVertexlayout != 0)
			glDeleteVertexArrays(1, &mVertexlayout);
	}

	void Render()
	{
		mShader->use();
		mShader->setInt("u_NumChannels", 0);

		glBindVertexArray(mVertexlayout);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}
	void Render(int numChannel)
	{
		mShader->use();
		mShader->setInt("u_NumChannels", numChannel);

		glBindVertexArray(mVertexlayout);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}
	void Render(Texture* texture, int x, int y, int width, int height)
	{
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);

		int xOld = viewport[0];
		int yOld = viewport[1];
		int wOld = viewport[2];
		int hOld = viewport[3];

		glDisable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glViewport(x, y, width, height);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture->id);

		int numChannel = 0;
		if (texture->internal_format == GL_RGBA)
			numChannel = 0;
		else if (texture->internal_format == GL_RGB)
			numChannel = 1;
		else if (texture->internal_format == GL_RG)
			numChannel = 2;
		else if (texture->internal_format == GL_RED)
			numChannel = 3;
		else if (texture->internal_format == GL_R8)
			numChannel = 3;

		mShader->use();
		mShader->setInt("u_NumChannels", numChannel);

		glBindVertexArray(mVertexlayout);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);

		glViewport(xOld, yOld, wOld, hOld);
	}
	void Render(Texture* texture, int x, int y, int width, int height, Channels numChannel)
	{
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);

		int xOld = viewport[0];
		int yOld = viewport[1];
		int wOld = viewport[2];
		int hOld = viewport[3];

		glDisable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glViewport(x, y, width, height);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture->id);

		mShader->use();
		mShader->setInt("u_NumChannels", (int)numChannel);

		glBindVertexArray(mVertexlayout);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);

		glViewport(xOld, yOld, wOld, hOld);
	}
	void Render(GLuint textureID, int x, int y, int width, int height, Channels numChannel)
	{
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);

		int xOld = viewport[0];
		int yOld = viewport[1];
		int wOld = viewport[2];
		int hOld = viewport[3];

		glDisable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glViewport(x, y, width, height);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);

		mShader->use();
		mShader->setInt("u_NumChannels", (int)numChannel);

		glBindVertexArray(mVertexlayout);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);

		glViewport(xOld, yOld, wOld, hOld);
	}
};

class ScreenQuad
{
public:
	ScreenQuad()
	{
		float vertices[] = {
			-1.0f, -1.0f, 0.0, 0.0,
			-1.0f,  1.0f, 0.0, 1.0,
			1.0f, -1.0f, 1.0, 0.0,
			1.0f, -1.0f, 1.0, 0.0,
			-1.0f,  1.0f, 0.0, 1.0,
			1.0f,  1.0f, 1.0, 1.0
		};

		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);

		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

	}
	~ScreenQuad() 
	{
		glDeleteVertexArrays(1, &quadVAO);
	};

	void Render()
	{
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	void DisableTests()
	{
		glDisable(GL_CLIP_DISTANCE0);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
	}

private:
	unsigned int quadVAO, quadVBO;
};
