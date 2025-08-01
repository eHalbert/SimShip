#pragma once

#define NOMINMAX
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <unordered_map>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace glm;

class Shader 
{
public:
    GLuint ID;

    Shader() : ID(0) {}
    Shader(const string& vertexPath, const string& fragmentPath, const string& geometryPath = "", const string& computePath = "", const string& tessControlPath = "", const string& tessEvaluationPath = "")
    {
        Load(vertexPath, fragmentPath, geometryPath, computePath, tessControlPath, tessEvaluationPath);
    }
    ~Shader() 
    {
        glDeleteProgram(ID);
    }

    void addDefines(const string& defines)
    {
        mDefines = defines + "\n";
    }
    void Load(const string& vertexPath, const string& fragmentPath, const string& geometryPath = "", const string& computePath = "", const string& tessControlPath = "", const string& tessEvaluationPath = "") 
    {
        string vertexCode           = vertexPath.empty() ?          "" : loadShaderFile(vertexPath);
        string fragmentCode         = fragmentPath.empty() ?        "" : loadShaderFile(fragmentPath);
        string geometryCode         = geometryPath.empty() ?        "" : loadShaderFile(geometryPath);
        string computeCode          = computePath.empty() ?         "" : loadShaderFile(computePath);
        string tessControlCode      = tessControlPath.empty() ?     "" : loadShaderFile(tessControlPath);
        string tessEvaluationCode   = tessEvaluationPath.empty() ?  "" : loadShaderFile(tessEvaluationPath);

        GLuint vertex           = vertexCode.empty() ?          0 : compileShader(GL_VERTEX_SHADER, vertexCode);
        GLuint fragment         = fragmentCode.empty() ?        0 : compileShader(GL_FRAGMENT_SHADER, fragmentCode);
        GLuint geometry         = geometryCode.empty() ?        0 : compileShader(GL_GEOMETRY_SHADER, geometryCode);
        GLuint compute          = computeCode.empty() ?         0 : compileShader(GL_COMPUTE_SHADER, computeCode);
        GLuint tessControl      = tessControlCode.empty() ?     0 : compileShader(GL_TESS_CONTROL_SHADER, tessControlCode);
        GLuint tessEvaluation   = tessEvaluationCode.empty() ?  0 : compileShader(GL_TESS_EVALUATION_SHADER, tessEvaluationCode);

        ID = glCreateProgram();
        if (vertex)         glAttachShader(ID, vertex);
        if (fragment)       glAttachShader(ID, fragment);
        if (geometry)       glAttachShader(ID, geometry);
        if (compute)        glAttachShader(ID, compute);
        if (tessControl)    glAttachShader(ID, tessControl);
        if (tessEvaluation) glAttachShader(ID, tessEvaluation);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        if (vertex)         glDeleteShader(vertex);
        if (fragment)       glDeleteShader(fragment);
        if (geometry)       glDeleteShader(geometry);
        if (compute)        glDeleteShader(compute);
        if (tessControl)    glDeleteShader(tessControl);
        if (tessEvaluation) glDeleteShader(tessEvaluation);
    }

    void LoadCombined(const string& shaderPath) 
    {
        string shaderCode = loadShaderFile(shaderPath);

        unordered_map<GLenum, string> shaderSources;
        parseShaderFile(shaderCode, shaderSources);

        vector<GLuint> shaderIDs;
        for (auto& [type, source] : shaderSources) 
        {
            shaderIDs.push_back(compileShader(type, source));
        }

        ID = glCreateProgram();
        for (auto& shader : shaderIDs) 
        {
            glAttachShader(ID, shader);
        }
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        for (auto& shader : shaderIDs) 
        {
            glDeleteShader(shader);
        }
    }
    void use() 
    {
        glUseProgram(ID);
    }
    void setBool(const string& name, bool value) const 
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    void setInt(const string& name, int value) const 
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    void setFloat(const string& name, float value) const 
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    void setVec2(const string& name, const vec2& value) const 
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec2(const string& name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }
    void setVec3(const string& name, const vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const string& name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }
    void setVec4(const string& name, const vec4& value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const string& name, float x, float y, float z, float w)
    {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }
    void setMat2(const string& name, const mat2& mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    void setMat3(const string& name, const mat3& mat) const 
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    void setMat4(const string& name, const mat4& mat) const 
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    void setSampler2D(const string& name, unsigned int texture, int id) const
    {
        glActiveTexture(GL_TEXTURE0 + id);
        glBindTexture(GL_TEXTURE_2D, texture);
        this->setInt(name, id);
    }
    void setSampler3D(const string& name, unsigned int texture, int id) const
    {
        glActiveTexture(GL_TEXTURE0 + id);
        glBindTexture(GL_TEXTURE_3D, texture);
        this->setInt(name, id);
    }
    void setSamplerCube(const string& name, unsigned int texture, int id) const
    {
        glActiveTexture(GL_TEXTURE0 + id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
        this->setInt(name, id);
    }


private:
    string loadShaderFile(const string& shaderPath) 
    {
        ifstream shaderFile;
        shaderFile.exceptions(ifstream::failbit | ifstream::badbit);
        try
        {
            shaderFile.open(shaderPath);
            stringstream shaderStream;
            shaderStream << shaderFile.rdbuf();
            shaderFile.close();
            return shaderStream.str();
        }
        catch (ifstream::failure& e) 
        {
            cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << endl;
            return "";
        }
    }
    GLuint compileShader(GLenum type, const string& source) 
    {
        GLuint shader = glCreateShader(type);

        // Search for the #version directive
        string finalSource = source;
        size_t versionPos = finalSource.find("#version");
        if (versionPos != string::npos) 
        {
            // Extract the #version line
            size_t endOfLine = finalSource.find_first_of("\r\n", versionPos);
            string versionLine = finalSource.substr(versionPos, endOfLine - versionPos);

            // Delete the #version line from its original position
            finalSource.erase(versionPos, endOfLine - versionPos + 1);

            // Add the line #version at the beginning, followed by a new line
            finalSource = versionLine + "\n" + mDefines + finalSource;
        }
        else
        {
            // If no #version, just add the defines
            finalSource = mDefines + finalSource;
        }

        const char* src = finalSource.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        checkCompileErrors(shader, "SHADER");
        return shader;
    }

    void checkCompileErrors(GLuint shader, string type) 
    {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM") 
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) 
            {
                glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) 
            {
                glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
                cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << endl;
            }
        }
    }

    GLenum shaderTypeFromString(const string& type) 
    {
        if (type == "vertex") return GL_VERTEX_SHADER;
        if (type == "fragment" || type == "pixel") return GL_FRAGMENT_SHADER;
        if (type == "geometry") return GL_GEOMETRY_SHADER;
        if (type == "compute") return GL_COMPUTE_SHADER;
        return 0;
    }
    void parseShaderFile(const string& shaderCode, unordered_map<GLenum, string>& shaderSources) 
    {
        const char* typeToken = "#type";
        size_t typeTokenLength = strlen(typeToken);
        size_t pos = shaderCode.find(typeToken, 0);
        while (pos != string::npos) 
        {
            size_t eol = shaderCode.find_first_of("\r\n", pos);
            string type = shaderCode.substr(pos + typeTokenLength + 1, eol - pos - typeTokenLength - 1);
            size_t nextLinePos = shaderCode.find_first_not_of("\r\n", eol);
            pos = shaderCode.find(typeToken, nextLinePos);
            shaderSources[shaderTypeFromString(type)] = (pos == string::npos) ? shaderCode.substr(nextLinePos) : shaderCode.substr(nextLinePos, pos - nextLinePos);
        }
    }

    string mDefines;
};
