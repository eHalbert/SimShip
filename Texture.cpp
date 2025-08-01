#include "Texture.h"

#include <iostream>

#include <stb_image.h>
#include "stb_image_write.h"

using namespace std;

Texture::Texture(unsigned int width_, unsigned int height_, GLint internal_format_, GLenum format, GLenum type, GLint min_filter, GLint max_filter, GLint wrap_r, GLint wrap_s, const GLvoid* data) 
                    : width(width_), height(height_), internal_format(internal_format_)
{
    glGenTextures(1, &id);

    Bind();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, max_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, wrap_r);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);

    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, data);

    Unbind();
}
Texture::~Texture()
{
    glDeleteTextures(1, &id);
}
void Texture::Init(unsigned int width_, unsigned int height_, GLint internal_format_, GLenum format, GLenum type, GLint min_filter, GLint max_filter, GLint wrap_r, GLint wrap_s, const GLvoid* data)
{
    width = width_;
    height = height_;
    internal_format = internal_format_;

    glGenTextures(1, &id);

    Bind();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, max_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, wrap_r);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);

    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, data);

    Unbind();
}

void Texture::CreateFromFile(const char* path, bool flip_vertically)
{
    stbi_set_flip_vertically_on_load(flip_vertically);

    int width, height, channel_count;
    stbi_uc* data = stbi_load(path, &width, &height, &channel_count, 0);

    if (data)
    {
        GLint internal_format(-1);
        GLenum format(-1);

        switch (channel_count)
        {
            case 1:
            {
                internal_format = GL_R8;
                format = GL_RED;
            } 
            break;

            case 3:
            {
                internal_format = GL_RGB8;
                format = GL_RGB;
            } 
            break;

            case 4:
            {
                internal_format = GL_RGBA8;
                format = GL_RGBA;
            } 
            break;

            default:
            {
                cout << "File format not supported yet!" << endl;
            } 
            break;
        }

        Init(width, height, internal_format, format, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, data);
    }
    else
    {
        cout << "Failed to load image at path: " << path << endl;
    }

    stbi_image_free(data);
}
void Texture::CreateFromDDSFile(const char* path)
{
    gli::texture texture = gli::load(path);
    if (texture.empty())
    {
        std::cerr << "Erreur : Impossible de charger la texture DDS " << path << std::endl;
        return;
    }

    gli::gl gl(gli::gl::PROFILE_GL33);
    gli::gl::format const format = gl.translate(texture.format(), texture.swizzles());
    GLenum target = gl.translate(texture.target());

    glGenTextures(1, &id);
    glBindTexture(target, id);

    glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(texture.levels() - 1));
    glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, format.Swizzles[0]);
    glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, format.Swizzles[1]);
    glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, format.Swizzles[2]);
    glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, format.Swizzles[3]);

    glm::tvec3<GLsizei> const extent(texture.extent());
    GLsizei const face_total = static_cast<GLsizei>(texture.layers() * texture.faces());

    switch (texture.target())
    {
    case gli::TARGET_2D:
        glTexStorage2D(target, static_cast<GLint>(texture.levels()), format.Internal, extent.x, extent.y);
        break;
    case gli::TARGET_3D:
        glTexStorage3D(target, static_cast<GLint>(texture.levels()), format.Internal, extent.x, extent.y, extent.z);
        break;
    case gli::TARGET_CUBE:
        glTexStorage2D(target, static_cast<GLint>(texture.levels()), format.Internal, extent.x, extent.y);
        break;
    default:
        assert(0);
        break;
    }

    for (std::size_t layer = 0; layer < texture.layers(); ++layer)
    {
        for (std::size_t face = 0; face < texture.faces(); ++face)
        {
            for (std::size_t level = 0; level < texture.levels(); ++level)
            {
                GLsizei const layer_gl = static_cast<GLsizei>(layer);
                glm::tvec3<GLsizei> extent(texture.extent(level));
                GLenum target_face = gli::is_target_cube(texture.target()) ? static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face) : target;

                switch (texture.target())
                {
                case gli::TARGET_2D:
                case gli::TARGET_CUBE:
                {
                    if (gli::is_compressed(texture.format()))
                        glCompressedTexSubImage2D(target_face, static_cast<GLint>(level), 0, 0, extent.x, extent.y, format.Internal, static_cast<GLsizei>(texture.size(level)), texture.data(layer, face, level));
                    else
                        glTexSubImage2D(target_face, static_cast<GLint>(level), 0, 0, extent.x, extent.y, format.External, format.Type, texture.data(layer, face, level));
                }
                break;
                case gli::TARGET_3D:
                {
                    if (gli::is_compressed(texture.format()))
                        glCompressedTexSubImage3D(target, static_cast<GLint>(level), 0, 0, 0, extent.x, extent.y, extent.z, format.Internal, static_cast<GLsizei>(texture.size(level)), texture.data(layer, face, level));
                    else
                        glTexSubImage3D(target, static_cast<GLint>(level), 0, 0, 0, extent.x, extent.y, extent.z, format.External, format.Type, texture.data(layer, face, level));
                }
                break;
                default: assert(0); break;
                }
            }
        }
    }
    width = extent.x;
    height = extent.y;
    depth = (texture.target() == gli::TARGET_3D) ? extent.z : 1;
    internal_format = format.Internal;
    is_cubemap = (texture.target() == gli::TARGET_CUBE);
}
void Texture::SetWrappingParams(GLint wrap_r, GLint wrap_s)
{
    glTextureParameteri(id, GL_TEXTURE_WRAP_R, wrap_r);
    glTextureParameteri(id, GL_TEXTURE_WRAP_S, wrap_s);
}
unsigned int LoadCubemapWith6Faces(vector<string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    stbi_set_flip_vertically_on_load(false);

    int width, height, nrChannels;
    unsigned char* data;
    for (unsigned int i = 0; i < 6; i++)
    {
        data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    }

    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
void Texture::BindImage(GLuint unit, GLenum access, GLenum format) const
{
    glBindImageTexture(unit, id, 0, GL_FALSE, 0, access, format);
}

void SaveTexture2D(GLuint texture, int width, int height, int channels, int format, std::string name)
{
    float* pixels = new float[width * height * channels];

    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, format, GL_FLOAT, pixels);

    unsigned char* imageData = new unsigned char[width * height * channels];
    for (int i = 0; i < width * height * channels; ++i)
        imageData[i] = static_cast<unsigned char>(std::min(std::max(pixels[i] * 255.0f, 0.0f), 255.0f));

    int stride = width * channels;
    unsigned char* flippedData = new unsigned char[width * height * channels];
    for (int y = 0; y < height; ++y) 
        memcpy( flippedData + y * stride, imageData + (height - 1 - y) * stride, stride );

    stbi_write_png(name.c_str(), width, height, channels, flippedData, stride);

    delete[] pixels;
    delete[] imageData;
    delete[] flippedData;
}
void SaveDepthTexture2D(GLuint texture, int width, int height, std::string name)
{
    float* pixels = new float[width * height];

    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, pixels);

    unsigned char* imageData = new unsigned char[width * height];
    
    // First pass: find min and max
    float dmin = pixels[0], dmax = pixels[0];
    for (int i = 1; i < width * height; ++i) {
        if (pixels[i] < dmin) dmin = pixels[i];
        if (pixels[i] > dmax) dmax = pixels[i];
    }

    // Second pass: normalize and map [min, max] --> [0.255]
    for (int i = 0; i < width * height; ++i) {
        float depthNorm = (pixels[i] - dmin) / (dmax - dmin); // ∈ [0,1]
        imageData[i] = static_cast<unsigned char>(std::clamp(depthNorm * 255.0f, 0.0f, 255.0f));
    }

    int stride = width;
    unsigned char* flippedData = new unsigned char[width * height];
    for (int y = 0; y < height; ++y)
        memcpy(flippedData + y * stride, imageData + (height - 1 - y) * stride, stride);

    // stbi_write_png, 1 canal = grayscale
    stbi_write_png(name.c_str(), width, height, 1, flippedData, stride);

    delete[] pixels;
    delete[] imageData;
    delete[] flippedData;
}
void SaveTexture3D(GLuint texture, string name)
{
    glBindTexture(GL_TEXTURE_3D, texture);

    int width, height, depth; 
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, &height);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_DEPTH, &depth);

    vector<unsigned char> buffer(width * height * depth * 4);
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

    for (int z = 0; z < depth; z++) 
    {
        string filename = name + "_slice_" + to_string(z) + ".png";
        stbi_write_png(filename.c_str(), width, height, 4, buffer.data() + z * width * height * 4, width * 4);
    }
    glBindTexture(GL_TEXTURE_3D, 0);

}