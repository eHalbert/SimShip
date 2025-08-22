/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#include "Ocean.h"

#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <numeric>
#include <random>

// For spectrum study
bool bStats = true;
static float Min = FLT_MAX;
static float Max = FLT_MIN;
static float Sum = 0.0f;
static int nSum = 0;
void MinMax(float length)
{
    if (length < Min)
        Min = length;
    if (length > Max)
        Max = length;
    Sum += length;
    nSum++;
}
void getKMinMax()
{
    cout << "= V E C T O R    K =============================" << endl;
    cout << "Min = " << Min << endl;
    cout << "Max = " << Max << endl;
    cout << "Avg = " << Sum / nSum << endl;
    cout << "n = " << nSum << endl;
}

extern GLuint   TexContourShip;                     // Texture of the contour of the ship
extern int      TexContourShipW, TexContourShipH;   // Size of the contour of the ship
extern GLuint   TexReflectionColor;
extern bool     g_bShipWake;
extern GLuint   TexWakeBuffer;                      // Buffer of wake
extern int      TexWakeBufferSize;
extern GLuint   TexWakeVao;                         // Texture of wake made by a projection of vao
extern int      TexWakeVaoSize;
extern bool     bTexWakeByVAO;
extern 
int SPECTRUM = 0;
struct InstanceData 
{
    mat4    modelMatrix;
    float   seed;
};

Ocean::Ocean(vec2 wind, Sky* sky)
{
    mSky = sky;

    GetWind(wind);
    EvaluatePersistence(PersistenceSec);

    Init();
}
Ocean::~Ocean()
{
    if (mTexInitialSpectrum)
        glDeleteTextures(1, &mTexInitialSpectrum);
    if (mTextFrequencies)
        glDeleteTextures(1, &mTextFrequencies);
    if (mTexUpdatedSpectra[0] && mTexUpdatedSpectra[1])
        glDeleteTextures(2, &mTexUpdatedSpectra[0]);
    if (mTexTempData)
        glDeleteTextures(1, &mTexTempData);	
    if (mTexDisplacements)
        glDeleteTextures(1, &mTexDisplacements);	
    if (mTexGradients)
        glDeleteTextures(1, &mTexGradients);	
    if (mTexFoamAcc1)
        glDeleteTextures(1, &mTexFoamAcc1);
    if (mTexFoamAcc2)
        glDeleteTextures(1, &mTexFoamAcc2);
    if (mTexFoamBuffer)
        glDeleteTextures(1, &mTexFoamBuffer);
    if (mTexFoamBuffer)
        glDeleteTextures(1, &TexWakeBuffer);

    if (mVbo)
        glDeleteBuffers(1, &mVbo);
    if (mVao)
        glDeleteVertexArrays(1, &mVao);
    if (mIbo)
        glDeleteBuffers(1, &mIbo);

    for (GLuint vao : mvVAOs)
        if (vao)
            glDeleteVertexArrays(1, &vao);
}

// Init
void Ocean::Init()
{
    // Sort the ocean colors by their hue
    //int color = 0;
    //for (auto& c : vOceanColors)
    //{
    //    float h, s, l;
    //    RGBtoHSL(c, h, s, l);
    //    cout << color++ << " : " << h << endl;
    //}
    
    GLint maxanisotropy = 5;

    //glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

    glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxanisotropy);
    maxanisotropy = std::max(maxanisotropy, 2);

    // Generate initial spectrum and frequencies
    glGenTextures(1, &mTexInitialSpectrum);
    glGenTextures(1, &mTextFrequencies);

    glBindTexture(GL_TEXTURE_2D, mTexInitialSpectrum);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG32F, FFT_SIZE_1, FFT_SIZE_1);

    glBindTexture(GL_TEXTURE_2D, mTextFrequencies);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, FFT_SIZE_1, FFT_SIZE_1);

    // Fill in the 2 textures with data
    InitFrequencies();

    // Create other spectrum textures
    glGenTextures(2, mTexUpdatedSpectra);
    glBindTexture(GL_TEXTURE_2D, mTexUpdatedSpectra[0]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG32F, FFT_SIZE, FFT_SIZE);

    glBindTexture(GL_TEXTURE_2D, mTexUpdatedSpectra[1]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG32F, FFT_SIZE, FFT_SIZE);

    glGenTextures(1, &mTexTempData);
    glBindTexture(GL_TEXTURE_2D, mTexTempData);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG32F, FFT_SIZE, FFT_SIZE);

    // Create displacement map
    glGenTextures(1, &mTexDisplacements);
    glBindTexture(GL_TEXTURE_2D, mTexDisplacements);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, FFT_SIZE, FFT_SIZE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // For displacement pixels
    mPixelsDisplacement = make_unique<float[]>(FFT_SIZE * FFT_SIZE * 4);

    // Create gradient & folding map
    glGenTextures(1, &mTexGradients);
    glBindTexture(GL_TEXTURE_2D, mTexGradients);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, FFT_SIZE, FFT_SIZE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Create accumulation buffer for the foam
    glGenTextures(1, &mTexFoamAcc1);
    glBindTexture(GL_TEXTURE_2D, mTexFoamAcc1);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, FFT_SIZE, FFT_SIZE);
    glGenTextures(1, &mTexFoamAcc2);
    glBindTexture(GL_TEXTURE_2D, mTexFoamAcc2);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, FFT_SIZE, FFT_SIZE);

    // Create environment map
    mTexEnvironment = make_unique<Texture>();
    mTexEnvironment->CreateFromDDSFile("Resources/Ocean/ocean_env.dds");

    maxanisotropy = 8;

    // Create the texture of the foam
    mTexFoamDesign = make_unique<Texture>();
    mTexFoamDesign->CreateFromFile("Resources/Ocean/foam.png");
    glBindTexture(GL_TEXTURE_2D, mTexFoamDesign->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxanisotropy);
   
    mTexFoamBubbles = make_unique<Texture>();
    mTexFoamBubbles->CreateFromFile("Resources/Ocean/foam_bubbles.png");
    glBindTexture(GL_TEXTURE_2D, mTexFoamBubbles->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxanisotropy);

    // Create the texture of the wake
    mTexFoam = make_unique<Texture>();
    mTexFoam->CreateFromFile("Resources/Ocean/seamless-seawater-with-foam-1.jpg");
    glBindTexture(GL_TEXTURE_2D, mTexFoam->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxanisotropy);

    mTexWaterDuDv = make_unique<Texture>();
    mTexWaterDuDv->CreateFromFile("Resources/Ocean/waterDUDV.png");
    glBindTexture(GL_TEXTURE_2D, mTexWaterDuDv->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxanisotropy);

    mTexKelvinArray = InitTexture2DArray();
    glBindTextureUnit(0, mTexKelvinArray);

    glBindTexture(GL_TEXTURE_2D, 0);

    CreateMesh();
    CreateLODMeshes();
    GetPatchVertices();

    // Load shaders
    char defines[256];
    sprintf_s(defines, "#define FFT_SIZE %d\n#define GRID_SIZE %d\n#define LOG2_N_SIZE %d\n#define PATCH_SIZE_X2_N %.4f\n",
        FFT_SIZE,
        MESH_SIZE,
        Log2OfPow2(FFT_SIZE),
        (float)PATCH_SIZE * 2.0f / (float)FFT_SIZE);

    mShaderSpectrum = make_unique<Shader>();
    mShaderSpectrum->addDefines(defines);
    mShaderSpectrum->Load("", "", "", "Resources/Ocean/updatespectrum.comp");
    mShaderSpectrum->use();
    mShaderSpectrum->setInt("tilde_h0", 0);
    mShaderSpectrum->setInt("frequencies", 1);
    mShaderSpectrum->setInt("tilde_h", 2);
    mShaderSpectrum->setInt("tilde_D", 3);

    mShaderFft = make_unique<Shader>();
    mShaderFft->addDefines(defines);
    mShaderFft->Load("", "", "", "Resources/Ocean/fourier_fft.comp");
    mShaderFft->use();
    mShaderFft->setInt("readbuff", 0);
    mShaderFft->setInt("writebuff", 1);

    mShaderDisplacements = make_unique<Shader>();
    mShaderDisplacements->addDefines(defines);
    mShaderDisplacements->Load("", "", "", "Resources/Ocean/createdisplacement.comp");
    mShaderDisplacements->use();
    mShaderDisplacements->setInt("heightmap", 0);
    mShaderDisplacements->setInt("choppyfield", 1);
    mShaderDisplacements->setInt("displacement", 2);

    mShaderGradients = make_unique<Shader>();
    mShaderGradients->addDefines(defines);
    mShaderGradients->Load("", "", "", "Resources/Ocean/creategradients.comp");
    mShaderGradients->use();
    mShaderGradients->setInt("displacement", 0);
    mShaderGradients->setInt("gradients", 1);
    mShaderGradients->setInt("accfoam1", 2);
    mShaderGradients->setInt("accfoam2", 3);

    mShaderOcean = make_unique<Shader>();
    mShaderOcean->addDefines(defines);
    mShaderOcean->Load("Resources/Ocean/ocean.vert", "Resources/Ocean/ocean.frag");
    mShaderOcean->use();
    mShaderOcean->setInt("displacement", 0);        // dx, dy, dz
    mShaderOcean->setInt("envmap", 1);              // cubemap texture
    mShaderOcean->setInt("gradients", 2);           // jacobians for the foam
    mShaderOcean->setInt("foamBuffer", 3);          // buffer accumulation of the foam which progressively disappear
    mShaderOcean->setInt("foamTexture", 4);         // texture of the foam
    mShaderOcean->setInt("foamBubbles", 5);         // texture to add bubbles
    mShaderOcean->setInt("foamTexture", 6);         // texture of the foam

    mShaderOceanWake = make_unique<Shader>();
    mShaderOceanWake->addDefines(defines);
    mShaderOceanWake->Load("Resources/Ocean/ocean_wake.vert", "Resources/Ocean/ocean_wake.frag");
    mShaderOceanWake->use();
    mShaderOceanWake->setInt("displacement", 0);    // dx, dy, dz
    mShaderOceanWake->setInt("kelvinArray", 1);
    mShaderOceanWake->setInt("envmap", 2);          // cubemap texture
    mShaderOceanWake->setInt("gradients", 3);       // jacobians for the foam
    mShaderOceanWake->setInt("foamBuffer", 4);      // buffer accumulation of the foam which progressively disappear
    mShaderOceanWake->setInt("foamDesign", 5);      // texture of the foam
    mShaderOceanWake->setInt("foamBubbles", 6);     // texture to add bubbles
    mShaderOceanWake->setInt("foamTexture", 7);     // texture of the foam
    mShaderOceanWake->setInt("contourShip", 8);     // texture of foam around the ship
    mShaderOceanWake->setInt("reflectionTexture", 9);// reflection texture
    mShaderOceanWake->setInt("waterDUDV", 10);       // texture to add vibrations of the water for the reflection
    mShaderOceanWake->setInt("wakeBuffer", 11);     // wake of the ship

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxanisotropy / 2);

    OceanColor = ConvertToFloat(vOceanColors[iOceanColor]);
}
void Ocean::InitFrequencies()
{
    Lambda = EvaluateLambda(Wind);

    mt19937 gen(static_cast<unsigned int>(std::time(0)));
    normal_distribution<> gaussian(0.0, 1.0);
    
    complex<float>* h0data = new complex<float>[(FFT_SIZE_1) * (FFT_SIZE_1)];
    vec2 k;
    float sqrt_S;
    //vector<float> vS(FFT_SIZE_1 * FFT_SIZE_1);

    float* wdata = new float[(FFT_SIZE_1) * (FFT_SIZE_1)];
    {
        for (int m = 0; m <= FFT_SIZE; ++m)
        {
            for (int n = 0; n <= FFT_SIZE; ++n)
            {
                // n & m are bound from -FFT_SIZE/2 to FFT_SIZE/2
                k.x = 2.0 * M_PI * (n - FFT_SIZE / 2) / LengthWave;
                k.y = 2.0 * M_PI * (m - FFT_SIZE / 2) / LengthWave;
                
                switch (SPECTRUM)
                {
                case 0: sqrt_S = sqrtf(Phillips(k)); break;
                case 1: sqrt_S = sqrtf(JONSWAP(k)); break;
                case 2: sqrt_S = sqrtf(PiersonMoskowitz(k)); break;
                case 3: sqrt_S = sqrtf(DonelanBanner(k)); break;
                case 4: sqrt_S = sqrtf(Elfouhaily(k)); break;
                case 5: sqrt_S = sqrtf(Elfouhaily2(k)); break;
                case 6: sqrt_S = sqrtf(TexelMarsenArsloe(k)); break;
                case 7: sqrt_S = sqrtf(TexelMarsenArsloe2(k)); break;
                }
                sqrt_S *= Amplitude;
                //vS.push_back(sqrt_S);

                int index = m * (FFT_SIZE_1) + n;
                h0data[index].real(gaussian(gen) * sqrt_S);
                h0data[index].imag(gaussian(gen) * sqrt_S);

                // Dispersion relation \omega^2(k) = gk
                wdata[index] = sqrtf(mGravity * glm::length(k));
            }
        }
    }
    //getKMinMax();
    //GetSpectrumStats(vS);

    glBindTexture(GL_TEXTURE_2D, mTextFrequencies);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FFT_SIZE_1, FFT_SIZE_1, GL_RED, GL_FLOAT, wdata);

    glBindTexture(GL_TEXTURE_2D, mTexInitialSpectrum);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FFT_SIZE_1, FFT_SIZE_1, GL_RG, GL_FLOAT, h0data);

    delete[] wdata;
    delete[] h0data;
}
GLuint Ocean::InitTexture2DArray()
{
    const int texCount = 100;
    const int width = 1024;
    const int height = 1024;

    stbi_set_flip_vertically_on_load(true);

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);

    // Allouer espace pour la texture 2D array
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R8, width, height, texCount);

    for (int i = 0; i < texCount; i++) 
    {
        string filename;
        if (i < 9)
            filename = "Resources/Kelvin/Kelvin-1024_Fr-00" + std::to_string(i + 1) + ".png";
        else if (i < 99)
            filename = "Resources/Kelvin/Kelvin-1024_Fr-0" + std::to_string(i + 1) + ".png";
        else
            filename = "Resources/Kelvin/Kelvin-1024_Fr-" + std::to_string(i + 1) + ".png";

        int w, h, nrChannels;
        unsigned char* data = stbi_load(filename.c_str(), &w, &h, &nrChannels, 1); // Charger en un canal (GL_RED)
        if (!data) {
            std::cerr << "Error loadind texture " << filename << std::endl;
            continue;
        }

        if (w != width || h != height) {
            std::cerr << "Incorrect size in " << filename << std::endl;
            stbi_image_free(data);
            continue;
        }

        // Copier les données dans la couche i de la 2D texture array
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, GL_RED, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
    }

    // Paramètres de filtrage / wrapping
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
   
    stbi_set_flip_vertically_on_load(false);

    return textureID;
}
void Ocean::GetWind(vec2 wind) 
{ 
    Wind = wind; 
    Lambda = EvaluateLambda(Wind); 
};
void Ocean::CreateMesh()
{
    // Create mesh of PATCH_SIZE meters (MESH_SIZE cells x MESH_SIZE cells)
    GridVertex* vdata = new GridVertex[MESH_SIZE_1 * MESH_SIZE_1];
    for (int z = 0; z <= MESH_SIZE; ++z)
    {
        for (int x = 0; x <= MESH_SIZE; ++x)
        {
            int index = z * MESH_SIZE_1 + x;

            vdata[index].position.x = (x - MESH_SIZE / 2.0f) * PATCH_SIZE / MESH_SIZE;
            vdata[index].position.y = 0.0f;
            vdata[index].position.z = (z - MESH_SIZE / 2.0f) * PATCH_SIZE / MESH_SIZE;

            vdata[index].texCoord.x = (float)x / (float)MESH_SIZE;
            vdata[index].texCoord.y = (float)z / (float)MESH_SIZE;
        }
    }

    unsigned int* idata = new unsigned int[MESH_SIZE_1 * MESH_SIZE_1 * 6];
    int index;
    mIndicesCount = 0;
    for (int z = 0; z < MESH_SIZE; ++z)
    {
        for (int x = 0; x < MESH_SIZE; ++x)
        {
            index = z * MESH_SIZE_1 + x;

            // two triangles
            idata[mIndicesCount++] = index;
            idata[mIndicesCount++] = index + MESH_SIZE_1;
            idata[mIndicesCount++] = index + MESH_SIZE_1 + 1;

            idata[mIndicesCount++] = index;
            idata[mIndicesCount++] = index + MESH_SIZE_1 + 1;
            idata[mIndicesCount++] = index + 1;
        }
    }

    // Générer et lier le mVAO
    glGenVertexArrays(1, &mVao);
    glBindVertexArray(mVao);

    // Générer et lier le VBO
    glGenBuffers(1, &mVbo);
    glBindBuffer(GL_ARRAY_BUFFER, mVbo);

    // Allouer l'espace pour le buffer, mais sans y mettre de données pour l'instant
    glBufferData(GL_ARRAY_BUFFER, MESH_SIZE_1 * MESH_SIZE_1 * sizeof(GridVertex), vdata, GL_STATIC_DRAW);

    // Activez les attributs
    glEnableVertexAttribArray(0);	// position
    glEnableVertexAttribArray(1);	// texCoord

    // Configurez les pointeurs d'attributs
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GridVertex), 0);	// position
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void*)offsetof(GridVertex, texCoord));	// texCoord

    // Générer et lier l'IBO
    glGenBuffers(1, &mIbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndicesCount * sizeof(unsigned int), idata, GL_STATIC_DRAW);

    glBindVertexArray(0);
}
void Ocean::CreateLODMesh(int meshSize, vector<GridVertex>& vertices, vector<unsigned int>& indices)
{
    int meshSize1 = meshSize + 1;
    vertices.resize(meshSize1 * meshSize1);

    // Création des mvVertices
    for (int z = 0; z <= meshSize; ++z)
    {
        for (int x = 0; x <= meshSize; ++x)
        {
            int index = z * meshSize1 + x;
            vertices[index].position.x = (x - meshSize / 2.0f) * PATCH_SIZE / meshSize;
            vertices[index].position.y = 0.0f;
            vertices[index].position.z = (z - meshSize / 2.0f) * PATCH_SIZE / meshSize;
            vertices[index].texCoord.x = (float)x / (float)meshSize;
            vertices[index].texCoord.y = (float)z / (float)meshSize;
        }
    }

    // Création des mvIndices
    indices.clear();
    for (int z = 0; z < meshSize; ++z)
    {
        for (int x = 0; x < meshSize; ++x)
        {
            int index = z * meshSize1 + x;
            indices.push_back(index);
            indices.push_back(index + meshSize1);
            indices.push_back(index + meshSize1 + 1);
            indices.push_back(index);
            indices.push_back(index + meshSize1 + 1);
            indices.push_back(index + 1);
        }
    }
}
void Ocean::CreateLODMeshes()
{
    // Create multiple LOD patches 
    for (int meshSize : mvMeshSizes)
    {
        vector<GridVertex> vertices;
        vector<unsigned int> indices;
        CreateLODMesh(meshSize, vertices, indices);

        GLuint vao, vbo, ibo;

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GridVertex), vertices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);   // layout (location = 0) in vec3 aPosition;
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void*)0);
        
        glEnableVertexAttribArray(1);   // layout (location = 1) in vec2 aTexCoords;
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void*)offsetof(GridVertex, texCoord));

        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        mvVAOs.push_back(vao);
        mvIndicesCounts.push_back(indices.size());

        glBindVertexArray(0);

        // Libération des buffers (optionnel, car le mVAO les référence toujours)
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ibo);
    }
}
void Ocean::GetPatchVertices()
{
    // Positions
    for (int z = 0; z <= MESH_SIZE; ++z)
    {
        vector<vec3> vPos;
        for (int x = 0; x <= MESH_SIZE; ++x)
        {
            int index = z * MESH_SIZE_1 + x;
            vec3 v;
            v.x = (x - MESH_SIZE / 2.0f) * PATCH_SIZE / MESH_SIZE;
            v.y = 0.0f;
            v.z = (z - MESH_SIZE / 2.0f) * PATCH_SIZE / MESH_SIZE;
            vPos.push_back(v);
        }
        mvPatchVertices.push_back(vPos);
    }
}
float Ocean::EvaluateLambda(vec2 wind)
{
    float lambdaLow = -0.6f;
    float lambdaHigh = -0.9f;

    // in Knots
    float windLow = 12.0f;
    float windHigh = 28.0f;

    return glm::mix(lambdaLow, lambdaHigh, (glm::length(wind) * 1.852f - windLow) / (windHigh - windLow)); // [ -1f @ 12 kn, -2.f @ 60 kn ]
}
void Ocean::EvaluatePersistence(float seconds)
{
    PersistenceSec = seconds;
    PersistenceFactor = -std::log(0.01f) / PersistenceSec;
}

// Spectra
float Ocean::Phillips(vec2 k)
{
    /*
    *   For FFT + 1 = 513 => there are 269169 calls to this function (513 x 513).
    *   Due to 2 possibilities to exit before the result, there are only 131854 calls which succeed.
    *   Wind force is divided by 2 (more realistic)
    */

    float k_length = glm::length(k);
    if (k_length < 0.000001f)
        return 0.0f;
    
    // k^2 & k^4
    float k_length2 = k_length * k_length;
    float k_length4 = k_length2 * k_length2;

    float k_dot_w = glm::dot(glm::normalize(k), glm::normalize(Wind * 0.7f));   

    // If wave is moving against wind direction
    if (k_dot_w < 0.0f)	
        return 0.0f;

    // Directional distribution (added to Phillips spectrum)
    k_dot_w = pow(cos(1.0f * acos(k_dot_w)), 3);

    float k_dot_w2 = k_dot_w * k_dot_w;	// The higher the exponent in (k_dot_w)exp will be set (2 in this case), the more the waves will be aligned with the wind direction

    float L = glm::length2(Wind * 0.7f) / mGravity;	// Largest possible wave for wind speed V. L = V^2 / g
    float L2 = L * L;

    // Suppress waves smaller than 1 / 1000
    float damping = 0.0001f;
    float l2 = L2 * damping * damping;
    float S = exp(-1.0f / (k_length2 * L2)) / k_length4 * k_dot_w2 * exp(-k_length2 * l2);
    return S * 0.0000375f;      // Acceptable factor to have the Amplitude around 1.0f
}
float Ocean::JONSWAP(vec2 k)
{
    // Le spectre JONSWAP est plus complexe et prend en compte plus de paramètres que le spectre de Phillips. Il produit généralement des vagues plus prononcées et plus réalistes, en particulier pour les mers en développement.
 
    k *= 6.0f;

    if (k.x == 0.0f && k.y == 0.0f)
        return 0.0f;

    float k_length = glm::length(k);
    //MinMax(k_length);

    float w_length = glm::length(Wind);
    float fetch = 1000.0f; // Longueur du fetch en mètres (à ajuster selon vos besoins)
    float g = mGravity;

    // Paramètres JONSWAP
    float alpha = 0.076f * pow(w_length * w_length / (fetch * g), -0.22f);
    float omega_p = 22.0f * pow(g * g / (w_length * fetch), 1.0f / 3.0f); // Fréquence de pic
    float gamma = 3.3f; // Facteur de pic (typiquement entre 1 et 7)
    float sigma = (k_length <= omega_p) ? 0.07f : 0.09f;

    // Calcul du spectre
    float omega = sqrt(g * k_length);
    float r = exp(-(omega - omega_p) * (omega - omega_p) / (2.0f * sigma * sigma * omega_p * omega_p));
    float S_pm = (alpha * g * g / pow(omega, 5)) * exp(-1.25f * pow(omega_p / omega, 4));
    float S_j = S_pm * pow(gamma, r);

    // Directionnalité
    float k_dot_w = glm::dot(glm::normalize(k), glm::normalize(Wind));
    float D = pow(cos(0.5f * acos(k_dot_w)), 2); // Distribution directionnelle

    float S = S_j * D / (k_length * k_length * k_length * k_length);
    return S * 1.0f;
}
float Ocean::JONSWAP2(vec2 k)
{
    float k_length = glm::length(k * 0.001f);
    if (k_length < 0.000001f)
        return 0.0f;

    float omega = sqrt(mGravity * k_length);
    float omega_p = 0.855f * mGravity / glm::length(Wind);  // Fréquence de pic

    float alpha = 0.0081f;  // Constante de Phillips
    float gamma = 3.3f;     // Facteur de pic
    float sigma = (omega <= omega_p) ? 0.07f : 0.09f;

    float r = exp(-(omega - omega_p) * (omega - omega_p) / (2.0f * sigma * sigma * omega_p * omega_p));

    float S = (alpha * mGravity * mGravity / (omega * omega * omega * omega * omega)) * exp(-1.25f * pow(omega_p / omega, 4)) * pow(gamma, r);

    // Distribution directionnelle
    float theta = atan2(k.y, k.x);
    float cos_theta = cos(theta - atan2(Wind.y, Wind.x));
    float D = pow(cos_theta, 2);  // Distribution cosinus carré

    return S * D * 0.0375f * Amplitude;  // Facteur d'échelle pour ajuster l'amplitude
}
float Ocean::PiersonMoskowitz(vec2 k)
{
    // Ce spectre est souvent utilisé pour modéliser des mers complètement développées. Il est plus simple que le spectre JONSWAP et ne prend pas en compte le fetch limité.

    k *= 5.5f;

    if (k.x == 0.0f && k.y == 0.0f)
        return 0.0f;

    float k_length = glm::length(k);

    float g = mGravity;
    float w_length = glm::length(Wind);

    // Paramètres du spectre Pierson-Moskowitz
    float alpha = 0.0081f; // Constante de Phillips qui contrôle l'amplitude globale du spectre.
    float omega_p = g / w_length; // Fréquence de pic

    // Calcul du spectre
    float omega = sqrt(g * k_length);
    float S_pm = (alpha * g * g / pow(omega, 5)) * exp(-5.0f / 4.0f * pow(omega_p / omega, 4));

    // Directionnalité
    float k_dot_w = glm::dot(glm::normalize(k), glm::normalize(Wind));
    float D = pow(cos(0.5f * acos(k_dot_w)), 2); // Distribution directionnelle

    float S = S_pm * D / (k_length * k_length * k_length * k_length);
    return S * 1.0f;
}
float Ocean::DonelanBanner(vec2 k)
{
    // Ce spectre est une amélioration du spectre JONSWAP, particulièrement adapté pour les vents forts et les vagues courtes.

    k *= 3.0f;
    
    if (k.x == 0.0f && k.y == 0.0f)
        return 0.0f;

    float k_length = glm::length(k);

    float g = mGravity;
    float w_length = glm::length(Wind);

    // Paramètres du spectre Donelan-Banner
    float alpha = 0.006f * sqrt(w_length / g);
    float omega_p = 0.877f * g / w_length;
    float gamma = 1.7f;
    float sigma = 0.08f * (1.0f + 4.0f / pow(omega_p * w_length / g, 3));

    // Calcul du spectre
    float omega = sqrt(g * k_length);
    float r = exp(-pow(omega - omega_p, 2) / (2.0f * sigma * sigma * omega_p * omega_p));
    float S_db = alpha * g * g / pow(omega, 4) * exp(-pow(omega_p / omega, 4)) * pow(gamma, r);

    // Directionnalité
    float k_dot_w = glm::dot(glm::normalize(k), glm::normalize(Wind));
    float theta = acos(k_dot_w);
    float beta = 2.61f * pow(omega / omega_p, 0.65f);
    float sech = 1.0f / cosh(beta * theta);
    float D = pow(sech, 2);

    float S = S_db * D / (k_length * k_length * k_length * k_length);
    return S * 0.1f;
}
float Ocean::Elfouhaily(vec2 k)
{
    const float KM = 370.0;
    const float CM = 0.23;
    
    vec2 wave_vector = k;
    
    wave_vector *= 80.f;
    
    float k_length = length(wave_vector);

    float U10 = length(Wind);

    float Omega = 0.84f;
    float kp = mGravity * (Omega / U10) * (Omega / U10);

    float c = sqrt(mGravity * k_length * (1.f + ((k_length * k_length) / (KM * KM)))) / k_length;
    float cp = sqrt(mGravity * kp * (1.f + ((kp * kp) / (KM * KM)))) / kp;

    float Lpm = exp(-1.25 * (kp / k_length) * (kp / k_length));
    float gamma = 1.7;
    float sigma = 0.08 * (1.0 + 4.0 * pow(Omega, -3.0));
    float Gamma = exp(-(sqrt(k_length / kp) - 1.0) * (sqrtf(k_length / kp) - 1.0) / 2.0 * (sigma * sigma));
    float Jp = pow(gamma, Gamma);
    float Fp = Lpm * Jp * exp(-Omega / sqrt(10.0) * (sqrt(k_length / kp) - 1.0));
    float alphap = 0.006 * sqrt(Omega);
    float Bl = 0.5 * alphap * cp / c * Fp;

    float z0 = 0.000037 * U10 * U10 / mGravity * pow(U10 / cp, 0.9);
    float uStar = 0.41 * U10 / log(10.0 / z0);
    float alpham = 0.01 * ((uStar < CM) ? (1.0 + log(uStar / CM)) : (1.0 + 3.0 * log(uStar / CM)));
    float Fm = exp(-0.25 * (k_length / KM - 1.0) * (k_length / KM - 1.0));
    float Bh = 0.5 * alpham * CM / c * Fm * Lpm;

    float a0 = log(2.0) / 4.0;
    float am = 0.13 * uStar / CM;
    float Delta = tanh(a0 + 4.0 * pow(c / cp, 2.5) + am * pow(CM / c, 2.5));

    float cosPhi = glm::dot(glm::normalize(Wind), glm::normalize(wave_vector));

    float S = (1.0 / (2.0 * M_PI)) * pow(k_length, -4.0) * (Bl + Bh) * (1.0 + Delta * (2.0 * cosPhi * cosPhi - 1.0));

    float dk = 2.0 * M_PI / MESH_SIZE;
    float h = sqrt(S / 2.0) * dk;

    if (wave_vector.x == 0.0 && wave_vector.y == 0.0) h = 0.f;
    return h;
}
float Ocean::Elfouhaily2(vec2 k)
{
    float Hs = 2.0f;
    float U10 = glm::length(Wind);
    float fetch = 100.0f;

    float g = 9.81f; // Accélération due à la gravité
    float kp = (g / U10) * std::pow(fetch, -0.33f); // Nombre d'onde pic
    float alpha = 0.0074f; // Coefficient de Phillips

    float k_length = glm::length(k);

    if (k_length == 0.0f)
        return 0.0f;

    // Spectre d'Elfouhaily
    float S = alpha * Hs * Hs * std::pow(kp, 2) * std::pow(k_length, -3) * std::exp(-5.0f / 4.0f * std::pow(k_length / kp, -2)) * std::exp(-0.5f * std::pow(k_length / kp - 1, 2));
    return S * 0.01f;
}
float Ocean::TexelMarsenArsloe(vec2 k)
{
    // Ce spectre de Texel-MARSEN-ARSLOE (TMA) est une modification du spectre JONSWAP pour les eaux peu profondes
    
    k *= 6.f;

    float windSpeed = glm::length(Wind);    // Vitesse du vent en m/s
    float fetchLength = 100000.0f;          // 100 km
    float peakFrequency = 0.1f;             // 0.1 Hz
    float depth = 10.0f;

    if (k.x == 0.0f && k.y == 0.0f)
        return 0.0f;

    float k_length = glm::length(k);

    float g = mGravity;
    float omega = sqrt(g * k_length * tanh(k_length * depth));
    float omega_p = 2.0f * M_PI * peakFrequency;

    // Paramètres JONSWAP
    float alpha = 0.076f * pow(windSpeed * windSpeed / (fetchLength * g), 0.22f);
    float gamma = 3.3f;
    float sigma = (omega <= omega_p) ? 0.07f : 0.09f;

    // Calcul du spectre JONSWAP
    float r = exp(-pow(omega - omega_p, 2) / (2 * sigma * sigma * omega_p * omega_p));
    float S_j = alpha * g * g / pow(omega, 5) * exp(-5.0f / 4.0f * pow(omega_p / omega, 4)) * pow(gamma, r);

    // Facteur de profondeur limitée
    float k_h = k_length * depth;
    float phi = 0.5f + 0.5f * tanh(2.0f * k_h);

    // Directionnalité
    float k_dot_w = glm::dot(glm::normalize(k), glm::normalize(Wind));
    float D = pow(cos(0.5f * acos(k_dot_w)), 2);

    float S = S_j * phi * D / (k_length * k_length * k_length * k_length);
    return S * 1.0f;
}
float Ocean::TexelMarsenArsloe2(vec2 k)
{
    float omega = glm::length(k);
    if (omega == 0.0f)
        return 0.0f;

    float Hs = 2.0f;    // Hauteur significative
    float Tp = 10.0f;   // Période de pic
    float g = 9.81f;    // Accélération due à la gravité
    float omega_p = 2 * M_PI / Tp;  // Fréquence de pic
    float alpha = 0.0081f;  // Coefficient empirique

    // Spectre de Texel Marsen Arsloe
    float S = (alpha * Hs * Hs) / std::pow(omega, 5) * std::exp(-1.25f * std::pow(omega_p / omega, 4)) * std::exp(-0.5f * std::pow((omega - omega_p) / (0.07f * omega_p), 2));
    return S * 0.01f;
}

// Update
void Ocean::Update(float t)
{
    // Update spectra
    mShaderSpectrum->use();     // Ocean/updatespectrum.comp
    mShaderSpectrum->setFloat("time", t);                                                   // t * 0.6 might be a adhoc parameter to slow down the speed of the waves
    glBindImageTexture(0, mTexInitialSpectrum, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RG32F);      // tilde_h0
    glBindImageTexture(1, mTextFrequencies, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32F);          // frequencies
    glBindImageTexture(2, mTexUpdatedSpectra[0], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG32F);   // tilde_h
    glBindImageTexture(3, mTexUpdatedSpectra[1], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG32F);   // tilde_D
    glDispatchCompute(FFT_SIZE / 16, FFT_SIZE / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Transform spectra to spatial/time domain
    FourierTransform(mTexUpdatedSpectra[0]);   // readbuff
    FourierTransform(mTexUpdatedSpectra[1]);   // writebuff

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Calculate displacement map
    mShaderDisplacements->use();    // Ocean/createdisplacement.comp
    glBindImageTexture(0, mTexUpdatedSpectra[0], 0, GL_TRUE, 0, GL_READ_ONLY, GL_RG32F);    // heightmap
    glBindImageTexture(1, mTexUpdatedSpectra[1], 0, GL_TRUE, 0, GL_READ_ONLY, GL_RG32F);    // choppyfield
    glBindImageTexture(2, mTexDisplacements, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);     // displacement
    mShaderDisplacements->setFloat("lambda", Lambda);
    glDispatchCompute(FFT_SIZE / 16, FFT_SIZE / 16, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Calculate normal & folding map
    swap(mTexFoamAcc1, mTexFoamAcc2);
    mTexFoamBuffer = mTexFoamAcc1;

    mShaderGradients->use();        // Ocean/creategradients.comp
    glBindImageTexture(0, mTexDisplacements, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);      // displacements
    glBindImageTexture(1, mTexGradients, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);         // gradients
    glBindImageTexture(2, mTexFoamAcc1, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32F);              // accumulation of foam (alternate read/write)
    glBindImageTexture(3, mTexFoamAcc2, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);             // accumulation of foam (alternate read/write)

    static float tOld = t;
    mShaderGradients->setFloat("t", t - tOld);
    mShaderGradients->setFloat("persistenceFactor", PersistenceFactor);
    tOld = t;
    glDispatchCompute(FFT_SIZE / 16, FFT_SIZE / 16, 1);

    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    // Get data of displacement (x, y, z)
    glBindTexture(GL_TEXTURE_2D, mTexDisplacements);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, mPixelsDisplacement.get());
    
    glBindTexture(GL_TEXTURE_2D, 0);
}
void Ocean::FourierTransform(GLuint spectrum)
{
    // horizontal pass
    glBindImageTexture(0, spectrum, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, mTexTempData, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG32F);

    mShaderFft->use();
    glDispatchCompute(FFT_SIZE, 1, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // vertical pass
    glBindImageTexture(0, mTexTempData, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, spectrum, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG32F);

    mShaderFft->use();
    glDispatchCompute(FFT_SIZE, 1, 1);
}
bool Ocean::GetVertice(vec2 pos, vec3& output)
{
    // Return a valid vertice
    int x = pos.x * FFT_SIZE / PATCH_SIZE + FFT_SIZE / 2;
    int z = pos.y * FFT_SIZE / PATCH_SIZE + FFT_SIZE / 2;

    int index = 4 * (z * FFT_SIZE + x);
    
    // Outside the map
    if (index < 0 || index >= FFT_SIZE * FFT_SIZE * 4)
        return false;

    output = { pos.x + mPixelsDisplacement[index + 0], mPixelsDisplacement[index + 1] , pos.y + mPixelsDisplacement[index + 2] };
    return true;
}

// Analysis
void Ocean::GetAllJacobians()
{
    glBindTexture(GL_TEXTURE_2D, mTexGradients);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, mPixelsDisplacement.get());

    float Max = FLT_MIN;
    float Min = FLT_MAX;

    float J;
    for (int i = 0; i < FFT_SIZE * FFT_SIZE; i += 4)
    {
        J = mPixelsDisplacement[i + 3];
        if (J < Min)
            Min = J;
        if (J > Max)
            Max = J;
    }
    cout << Min << "   " << Max << endl;
}
vector<vec2> Ocean::GetCut(int xN)
{
    int index;
    vector<vec2> vHeights(FFT_SIZE_1);
    int count = FFT_SIZE_1 - 1;
    for (int m_prime = 0; m_prime < FFT_SIZE; m_prime++)
    {
        index = 4 * (m_prime * FFT_SIZE + xN);
        vHeights[count--] = vec2(mPixelsDisplacement[index + 2], mPixelsDisplacement[index + 1]);
    }
    return vHeights;
}
void Ocean::GetRecordFromBuoy(vec2 pos, float t)
{
    // Get index
    int x = (MESH_SIZE / 2 + pos.x) * FFT_SIZE / MESH_SIZE;
    int z = (MESH_SIZE / 2 + pos.y) * FFT_SIZE / MESH_SIZE;
    int index = 4 * (z * FFT_SIZE + x);

    // Store data (time, dx, dy, dz)
    WaveData wd;
    wd.time = t;
    wd.dx = mPixelsDisplacement[index + 0];
    wd.dy = mPixelsDisplacement[index + 1];
    wd.dz = mPixelsDisplacement[index + 2];
    vWaveData.push_back(wd);

    // Tells the other functions that new data has been added
    bNewData = true;
}
bool Ocean::GetWaveByWaveAnalysis(float& waves1_3, float& waveMax, int& nWaves, float& average_period)
{
    if (!bNewData)
        return false;

    const size_t size = vWaveData.size();
    if (size < 3)
        return false;

    // Temporary vectors
    vector<size_t> crest_indices;
    vector<size_t> trough_indices;

    static vector<float> vWaves;
    static vector<float> vPeriods;

    // Peak and trough detection
    for (size_t i = 1; i < size - 1; ++i)
    {
        float prev = vWaveData[i - 1].dy;
        float curr = vWaveData[i].dy;
        float next = vWaveData[i + 1].dy;

        if (curr > prev && curr > next)
            crest_indices.push_back(i);
        else if (curr < prev && curr < next)
            trough_indices.push_back(i);
    }

    if (crest_indices.size() < 2 || trough_indices.empty())
        return false;

    // Wave calculation: associate each wave with a neighboring trough
    vWaves.clear();
    vPeriods.clear();

    // To simplify, we will go through ridges in pairs and look for hollows between these ridges
    for (size_t i = 0; i < crest_indices.size() - 1; ++i)
    {
        size_t crest1 = crest_indices[i];
        size_t crest2 = crest_indices[i + 1];

        // Find the minimum trough between these two peaks
        auto trough_it = std::min_element(trough_indices.begin(), trough_indices.end(),
            [&](size_t a, size_t b)
            {
                return (vWaveData[a].dy < vWaveData[b].dy) &&
                    (a > crest1 && a < crest2);
            });

        if (trough_it != trough_indices.end() && *trough_it > crest1 && *trough_it < crest2)
        {
            float height = vWaveData[crest1].dy - vWaveData[*trough_it].dy;
            if (height > 0)
            {
                vWaves.push_back(height);
                // Period between 2 crests
                float period = vWaveData[crest2].time - vWaveData[crest1].time;
                vPeriods.push_back(period);
            }
        }
    }

    nWaves = (int)vWaves.size();
    if (nWaves == 0)
        return false;

    average_period = std::accumulate(vPeriods.begin(), vPeriods.end(), 0.0f) / vPeriods.size();

    std::sort(vWaves.begin(), vWaves.end(), std::greater<float>());
    waveMax = vWaves[0];

    size_t nbSignificantWaves = nWaves / 3;
    if (nbSignificantWaves == 0)
        return false;

    waves1_3 = std::accumulate(vWaves.begin(), vWaves.begin() + nbSignificantWaves, 0.0f) / nbSignificantWaves;

    bNewData = false;
    return true;
}
void Ocean::GetSpectrumStats(vector<float>& vS)
{
    float Sum = 0.0f;
    for (const auto& valeur : vS)
        Sum += valeur;

    float moyenne = Sum / (FFT_SIZE_1 * FFT_SIZE_1);

    std::sort(vS.begin(), vS.end());
    float mediane;
    if (vS.size() % 2 == 0)
        mediane = (vS[vS.size() / 2 - 1] + vS[vS.size() / 2]) / 2.0f;
    else
        mediane = vS[vS.size() / 2];

    float ecart_type = 0.0f;
    for (const auto& valeur : vS)
        ecart_type += std::pow(valeur - moyenne, 2);

    ecart_type = std::sqrt(ecart_type / vS.size());

    float minimum = *std::min_element(vS.begin(), vS.end());
    float maximum = *std::max_element(vS.begin(), vS.end());

    std::map<int, int> frequences;
    for (const auto& valeur : vS)
        frequences[static_cast<int>(valeur)]++;

    cout << "= S P E C T R E  I N I T I A L =================" << endl;
    std::cout << "Moyenne : " << moyenne << std::endl;
    std::cout << "Mediane : " << mediane << std::endl;
    std::cout << "Ecart-type : " << ecart_type << std::endl;
    std::cout << "Minimum : " << minimum << std::endl;
    std::cout << "Maximum : " << maximum << std::endl;

    std::cout << "Frequences :" << std::endl;
    for (const auto& paire : frequences)
        std::cout << "Valeur " << paire.first << " : " << paire.second << " occurrences" << std::endl;
}
double GetPeriodPeak(const vector<double>& densiteSpectrale, double df)
{
    // Find the index of the energy peak in the spectrum
    auto it_max = std::max_element(densiteSpectrale.begin(), densiteSpectrale.end());
    size_t index_pic = std::distance(densiteSpectrale.begin(), it_max);

    // Calculate the corresponding frequency
    double frequence_pic = index_pic * df;

    // Convert frequency to period
    double Tp = 0.0;
    if (frequence_pic > 0)
    {
        Tp = 1.0 / frequence_pic;
    }
    else
    {
        // Handle the case where the peak frequency would be zero (unlikely but worth considering)
        std::cerr << "Warning: zero peak frequency detected." << std::endl;
        Tp = 0.0;
    }

    return Tp;
}
double GetSpectralMoment(const vector<double>& densiteSpectrale, double df, int ordre)
{
    double moment = 0.0;
    for (size_t i = 0; i < densiteSpectrale.size(); i++)
    {
        double f = i * df;
        moment += std::pow(f, ordre) * densiteSpectrale[i] * df;
    }
    return moment;
}
pair<vector<double>, vector<double>> Ocean::GetFrequencies()
{
    int N = vWaveData.size();

    // Calculate the average time interval
    double dt = 0;
    for (int i = 1; i < N; ++i)
        dt += vWaveData[i].time - vWaveData[i - 1].time;
    dt /= (N - 1);

    // Prepare data for FFT
    vector<double> heights(N);
    for (int i = 0; i < N; ++i)
        heights[i] = vWaveData[i].dy;

    // Application of Hann windowing - Hann : w[n] = 0.5 * (1 - cos(2*pi*n/(N-1))) for n=0..N-1
    for (int n = 0; n < N; ++n)
    {
        double w = 0.5 * (1 - cos(2 * M_PI * n / (N - 1)));
        heights[n] *= w;
    }

    // Allocate input and output arrays for FFTW
    fftwf_complex* in = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * N);
    fftwf_complex* out = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * N);

    // Copy input data
    for (int i = 0; i < N; ++i)
    {
        in[i][0] = heights[i];
        in[i][1] = 0.0;
    }

    // Create and execute the FFT plan
    fftwf_plan p = fftwf_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwf_execute(p);

    // Calculate frequencies and spectral densities
    vector<double> frequences(N / 2 + 1);
    vector<double> densiteSpectrale(N / 2 + 1);
    double df = 1.0 / (N * dt);

    // Multiply by 2 to conserve total energy (except for DC and Nyquist)
    for (int i = 0; i <= N / 2; ++i)
    {
        frequences[i] = i * df;
        double re = out[i][0];
        double im = out[i][1];
        densiteSpectrale[i] = (re * re + im * im) / (N * N);
    }

    for (int i = 1; i < N / 2; ++i)
        densiteSpectrale[i] *= 2.0;

    // Cleaning
    fftwf_destroy_plan(p);
    fftwf_free(in);
    fftwf_free(out);

    return make_pair(frequences, densiteSpectrale);
}
vector<sResultData> Ocean::SpectralAnalysis()
{
    // Calculate the average time interval
    double dt = 0;
    for (size_t i = 1; i < vWaveData.size(); i++)
        dt += vWaveData[i].time - vWaveData[i - 1].time;

    dt /= (vWaveData.size() - 1);

    auto [frequences, densiteSpectrale] = GetFrequencies();

    double df = 1.0 / (dt * vWaveData.size());  // Frequency resolution

    bool bFiltragePasseHaut = false;
    double m0, m1, m2;
    if (!bFiltragePasseHaut)
    {
        m0 = GetSpectralMoment(densiteSpectrale, df, 0);
        m1 = GetSpectralMoment(densiteSpectrale, df, 1);
        m2 = GetSpectralMoment(densiteSpectrale, df, 2);
    }
    else
    {
        // Apply a high-pass filter
        double fMin = 0.1; // Low cutoff frequency in Hz
        vector<double> densiteSpectraleFiltree = densiteSpectrale;
        for (size_t i = 0; i < frequences.size(); ++i)
            if (frequences[i] < fMin)
                densiteSpectraleFiltree[i] = 0.0;
        m0 = GetSpectralMoment(densiteSpectraleFiltree, df, 0);
        m1 = GetSpectralMoment(densiteSpectraleFiltree, df, 1);
        m2 = GetSpectralMoment(densiteSpectraleFiltree, df, 2);
    }

    vector<sResultData> vResults;
    sResultData rd;
    rd.variable = "Set of";
    rd.value = vWaveData.size();
    rd.decimal = 0;
    rd.unit = "data";
    vResults.push_back(rd);

    double Tm = sqrt(m0 / m2);
    rd.variable = "Average period (Tm)";
    rd.value = Tm;
    rd.decimal = 4;
    rd.unit = "s";
    vResults.push_back(rd);

    double Hm0 = 4.0 * sqrt(m0);
    rd.variable = "Hm0";
    rd.value = Hm0 * 10.0f;
    rd.decimal = 4;
    rd.unit = "m";
    vResults.push_back(rd);

    double Hmax = 1.86 * Hm0;
    rd.variable = "Hmax";
    rd.value = Hmax * 10.0f;
    rd.decimal = 4;
    rd.unit = "m";
    vResults.push_back(rd);

    double L = (9.81 * Tm * Tm) / (2 * M_PI);
    rd.variable = "Wavelength";
    rd.value = L;
    rd.decimal = 2;
    rd.unit = "m";
    vResults.push_back(rd);

    double cambrure = Hm0 / L;
    rd.variable = "Camber";
    rd.value = cambrure;
    rd.decimal = 4;
    rd.unit = "";
    vResults.push_back(rd);

    // After calculating the spectral density and df (frequency resolution). This is the period corresponding to the energy peak of the spectrum.
    double Tp = GetPeriodPeak(densiteSpectrale, df);
    rd.variable = "Peak period (Tp)";
    rd.value = Tp;
    rd.decimal = 4;
    rd.unit = "s";
    vResults.push_back(rd);

    double energie_totale = m0;
    rd.variable = "Total energy";
    rd.value = m0;
    rd.decimal = 4;
    rd.unit = "J";
    vResults.push_back(rd);

    // Power of the waves
    double rho = 1025; // densité de l'eau de mer
    double P = (rho * 9.81 * 9.81 * m0 * Tm) / (64 * M_PI);
    rd.variable = "Power of the waves";
    rd.value = P;
    rd.decimal = 4;
    rd.unit = "kW/m front";
    vResults.push_back(rd);

    return vResults;
}
vector<sResultData> Ocean::DirectionalAnalysis()
{
    int N = vWaveData.size();

    // Allocate tables for FFTW
    fftwf_complex* in_z, * out_z, * in_x, * out_x, * in_y, * out_y;
    fftwf_plan p_z, p_x, p_y;

    in_z = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * N);
    out_z = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * N);
    in_x = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * N);
    out_x = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * N);
    in_y = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * N);
    out_y = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * N);

    // Copy input vWaveData
    for (int i = 0; i < N; i++) {
        in_z[i][0] = vWaveData[i].dz; in_z[i][1] = 0.0;
        in_x[i][0] = vWaveData[i].dx; in_x[i][1] = 0.0;
        in_y[i][0] = vWaveData[i].dy; in_y[i][1] = 0.0;
    }

    // Create and execute FFT plans
    p_z = fftwf_plan_dft_1d(N, in_z, out_z, FFTW_FORWARD, FFTW_ESTIMATE);
    p_x = fftwf_plan_dft_1d(N, in_x, out_x, FFTW_FORWARD, FFTW_ESTIMATE);
    p_y = fftwf_plan_dft_1d(N, in_y, out_y, FFTW_FORWARD, FFTW_ESTIMATE);

    fftwf_execute(p_z);
    fftwf_execute(p_x);
    fftwf_execute(p_y);

    // Calculate the directional spectrum
    vector<double> frequences(N / 2 + 1);
    vector<double> densiteSpectrale(N / 2 + 1);
    vector<double> directions(N / 2 + 1);
    vector<double> etalement(N / 2 + 1);

    double dt = vWaveData[1].time - vWaveData[0].time;
    double df = 1.0 / (N * dt);

    for (int i = 0; i <= N / 2; i++)
    {
        frequences[i] = i * df;
        double Czz = out_z[i][0] * out_z[i][0] + out_z[i][1] * out_z[i][1];
        double Cxx = out_x[i][0] * out_x[i][0] + out_x[i][1] * out_x[i][1];
        double Cyy = out_y[i][0] * out_y[i][0] + out_y[i][1] * out_y[i][1];
        double Qxz = out_x[i][0] * out_z[i][1] - out_x[i][1] * out_z[i][0];
        double Qyz = out_y[i][0] * out_z[i][1] - out_y[i][1] * out_z[i][0];

        densiteSpectrale[i] = Czz / (N * N);

        // Corrected direction calculation for OpenGL
        double dir = atan2(Qxz, Qyz);
        dir = fmod(dir + 2 * M_PI, 2 * M_PI);  // Ensure that dir is between 0 and 2π
        dir = dir * 180 / M_PI;  // Convert to degrees

        // Adjust so that 0° is south and 90° is east
        directions[i] = fmod(540 - dir, 360);  // 450 = 360 + 90, to shift the 0 to the south

        // Calculation of the spread
        double r = sqrt((Qxz * Qxz + Qyz * Qyz) / (Czz * (Cxx + Cyy)));
        etalement[i] = sqrt(2 * (1 - r)) * 180 / M_PI;  // Convert to degrees
    }

    // Store the vectors for rendering
    a_Frequences = frequences;
    a_DensiteSpectrale = densiteSpectrale;
    a_Directions = directions;

    // Compute Hm0
    double m0 = 0.0;
    for (int i = 0; i <= N / 2; i++)
        m0 += densiteSpectrale[i] * df;

    double Hm0 = 4 * sqrt(m0);

    // Find corresponding Tp and Dir
    auto it_max = std::max_element(densiteSpectrale.begin(), densiteSpectrale.end());
    int index_pic = std::distance(densiteSpectrale.begin(), it_max);
    double Tp = 1.0 / frequences[index_pic];
    double Dir = directions[index_pic];

    // Calculate Energy Weighted Average
    double Etal = 0.0;
    double totalEnergy = 0.0;
    for (int i = 0; i <= N / 2; i++)
    {
        Etal += etalement[i] * densiteSpectrale[i];
        totalEnergy += densiteSpectrale[i];
    }
    Etal /= totalEnergy;

    // Cleaning
    fftwf_destroy_plan(p_z);
    fftwf_destroy_plan(p_x);
    fftwf_destroy_plan(p_y);
    fftwf_free(in_z); fftwf_free(out_z);
    fftwf_free(in_x); fftwf_free(out_x);
    fftwf_free(in_y); fftwf_free(out_y);

    vector<sResultData> vResults;
    sResultData rd;
    //rd.variable = "Hm0";
    //rd.value = Hm0;
    //rd.decimal = 4;
    //rd.unit = "m";
    //vResults.push_back(rd);

    //rd.variable = "Tp";
    //rd.value = Tp;
    //rd.decimal = 4;
    //rd.unit = "s";
    //vResults.push_back(rd);

    rd.variable = "Dir";
    Dir = fmod(450.0f - Dir, 360.0f);
    if (Dir < 0.0f)
        Dir += 360.0f;
    rd.value = Dir;
    rd.decimal = 2;
    rd.unit = "°";
    vResults.push_back(rd);

    rd.variable = "Spread";
    rd.value = Etal;
    rd.decimal = 2;
    rd.unit = "°";
    vResults.push_back(rd);

    return vResults;
}

// Render
void Ocean::GetPatchesDecal(vec2 Position, float w, float h, float Yaw, vector<pair<int, int>>& vPatches)
{
    // Get the corners of the decal
    float cosYaw = cos(Yaw);
    float sinYaw = sin(Yaw);
    vec2 coins[4] = {
        vec2(-w / 2 * cosYaw - h / 2 * sinYaw, -w / 2 * sinYaw + h / 2 * cosYaw),
        vec2( w / 2 * cosYaw - h / 2 * sinYaw,  w / 2 * sinYaw + h / 2 * cosYaw),
        vec2( w / 2 * cosYaw + h / 2 * sinYaw,  w / 2 * sinYaw - h / 2 * cosYaw),
        vec2(-w / 2 * cosYaw + h / 2 * sinYaw, -w / 2 * sinYaw - h / 2 * cosYaw)
    };

    // Initialize the limits
    float xMin = FLT_MAX, xMax = -FLT_MAX, zMin = FLT_MAX, zMax = -FLT_MAX;

    // Find the limits of the decal
    for (int k = 0; k < 4; k++)
    {
        vec2 coinGlobal = Position + coins[k];
        xMin = std::min(xMin, coinGlobal.x);
        xMax = std::max(xMax, coinGlobal.x);
        zMin = std::min(zMin, coinGlobal.y);
        zMax = std::max(zMax, coinGlobal.y);
    }

    // Convert limits to mvIndices of patches
    int iMin = floor((xMin + PATCH_SIZE / 2) / PATCH_SIZE);
    int iMax = ceil((xMax + PATCH_SIZE / 2) / PATCH_SIZE) - 1;
    int jMin = floor((zMin + PATCH_SIZE / 2) / PATCH_SIZE);
    int jMax = ceil((zMax + PATCH_SIZE / 2) / PATCH_SIZE) - 1;

	vPatches.clear();
    for (int i = iMin; i <= iMax; i++)
		for (int j = jMin; j <= jMax; j++)
            vPatches.push_back(std::make_pair(i, j));
}
void Ocean::WorldToPatch(float x, float z, float& xLocal, float& zLocal, int& i, int& j)
{
    // For a known position (x, z), give the patch (i, j) and the relative position on this patch (xLocal, zLocal)
    i = static_cast<int>(std::floor(x / PATCH_SIZE));
    j = static_cast<int>(std::floor(z / PATCH_SIZE));

    xLocal = x - i * PATCH_SIZE;
    zLocal = z - j * PATCH_SIZE;
}
bool IsPatchInFrustum(const mat4& viewProjMatrix, const vec3& center, float radius)
{
    vec4 clipSpacePos = viewProjMatrix * vec4(center, 1.0f);
    float w = clipSpacePos.w;

    // Clip coordinates in [-w, w] positive for visible (with margin for radius)
    // A margin (e.g. 1.1 * radius) can be added to avoid popping at the edges
    float margin = 1.1f * radius;

    vec3 absClipPos = glm::abs(glm::vec3(clipSpacePos));
    return (absClipPos.x <= w + margin) && (absClipPos.y <= w + margin) && (absClipPos.z <= w + margin);
}
void Ocean::Render(const float t, Camera& camera, vec3& ShipPosition, float ShipRotation, bool bWaves, float LWL, float kelvinScale, float shipVelocity, float centerFore)
{
    int sumPatches = 0;
#pragma region Instances
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Shader activation
    mShaderOcean->use();
    mShaderOcean->setMat4("matViewProj", camera.GetProjection() * camera.GetView());
    mShaderOcean->setVec3("eyePos", camera.GetPosition());
    mShaderOcean->setVec3("oceanColor", OceanColor);
    mShaderOcean->setFloat("transparency", Transparency);
    mShaderOcean->setVec3("sunColor", mSky->SunDiffuse);
    mShaderOcean->setInt("bEnvmap", bEnvmap);
    mShaderOcean->setFloat("exposure", mSky->Exposure);
    mShaderOcean->setBool("bAbsorbance", mSky->bAbsorbance);
    mShaderOcean->setVec3("absorbanceColor", mSky->AbsorbanceColor);
    mShaderOcean->setFloat("absorbanceCoeff", mSky->AbsorbanceCoeff);
    mShaderOcean->setVec3("sunDir", glm::normalize(mSky->SunPosition));
    mShaderOcean->setBool("bShowPatch", bShowPatch);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexDisplacements);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mTexEnvironment->id);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mTexGradients);
    
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTexFoamBuffer);
    
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, mTexFoamDesign->id);
    
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, mTexFoamBubbles->id);
    
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, mTexFoam->id);

    // Search for patches with wake (excluding instancing)
    vector<pair<int, int>> vPatches;
    if (bTexWakeByVAO)
        GetPatchesDecal(vec2(ShipPosition.x, ShipPosition.z), TexWakeVaoSize, TexWakeVaoSize, ShipRotation, vPatches);
    else
        GetPatchesDecal(vec2(ShipPosition.x, ShipPosition.z), TexWakeBufferSize, TexWakeBufferSize, ShipRotation, vPatches);

    // Definition of the InstanceData structure
    struct InstanceData {
        mat4 modelMatrix;   // Model matrix per instance
        int lod;            // 0, 1, 2, 3, 4 : level of detail per instance = 0 (near), 4 (far)
        int foamSwitch;     // 0, 1: random value per instance
    };

    // Vector by LOD
    vector<InstanceData> instanceData[5];

    // Central patches
    int cameraPatchX = static_cast<int>(std::round(camera.GetPosition().x / PATCH_SIZE));
    int cameraPatchZ = static_cast<int>(std::round(camera.GetPosition().z / PATCH_SIZE));

#ifdef _DEBUG
    int nGrids = 50;
#else
    int nGrids = NbPatches;
#endif

    const float sphereRadius = PATCH_SIZE * 1.414f;
    const mat4 viewProj = camera.GetViewProjection();

    for (int j = -nGrids / 2; j <= nGrids / 2; j++)
    {
        for (int i = -nGrids / 2; i <= nGrids / 2; i++)
        {
            vec3 center = vec3(PATCH_SIZE * (i + cameraPatchX), 0.f, PATCH_SIZE * (j + cameraPatchZ));

            // FRUSTUM CULLING test
            if (!IsPatchInFrustum(viewProj, center, sphereRadius))
                continue;  // Out of the frustum, we skip

            // Calculate distance to camera center (for LOD only)
            float distanceToCamera = glm::distance(center, camera.GetPosition());

            int lodLevel = 0;
            if (distanceToCamera < 600.f)       { lodLevel = 0; }
            else if (distanceToCamera < 1000.f) { lodLevel = 1; }
            else if (distanceToCamera < 1500.f) { lodLevel = 2; }
            else if (distanceToCamera < 2000.f) { lodLevel = 3; }
            else                                { lodLevel = 4; }

            // Checks if patch wake is absent (not instantiated)
            pair<int, int> recherche(i + cameraPatchX, j + cameraPatchZ);
            if (std::find(vPatches.begin(), vPatches.end(), recherche) != vPatches.end())
                continue;  // Patch wake present, skip

            // Fills in the instance data
            InstanceData data;
            data.modelMatrix = glm::translate(mat4(1.0f), center);
            data.lod = lodLevel;
            data.foamSwitch = 1;// dist(gen);    // 0 or 1
            instanceData[lodLevel].push_back(data);
        }
    }

    // Create and use a single VBO instance for all LODs
    GLuint instanceVBO;
    glGenBuffers(1, &instanceVBO);
    for (int lodLevel = 0; lodLevel < 5; lodLevel++)
    {
        if (instanceData[lodLevel].empty())
            continue;

        glBindVertexArray(mvVAOs[lodLevel]);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceData[lodLevel].size() * sizeof(InstanceData), &instanceData[lodLevel][0], GL_DYNAMIC_DRAW);

        // Mat4 attributes (occupy locations 2,3,4,5)
        for (unsigned int i = 0; i < 4; i++)
        {
            glEnableVertexAttribArray(2 + i);
            glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(sizeof(vec4) * i));
            glVertexAttribDivisor(2 + i, 1);
        }
        // Lod attribute (location 6)
        glEnableVertexAttribArray(6);
        glVertexAttribIPointer(6, 1, GL_INT, sizeof(InstanceData), (void*)offsetof(InstanceData, lod));
        glVertexAttribDivisor(6, 1);

        // foamSwitch attribute (location 7)
        glEnableVertexAttribArray(7);
        glVertexAttribIPointer(7, 1, GL_INT, sizeof(InstanceData), (void*)offsetof(InstanceData, foamSwitch));
        glVertexAttribDivisor(7, 1);

        // Instanced drawing
        mShaderOcean->use();
        glDrawElementsInstanced(GL_TRIANGLES, mvIndicesCounts[lodLevel], GL_UNSIGNED_INT, 0, instanceData[lodLevel].size());
    }
    
    for (int lodLevel = 0; lodLevel < 5; lodLevel++)
        sumPatches += instanceData[lodLevel].size();

    glDeleteBuffers(1, &instanceVBO);
#pragma endregion

#pragma region Patches with wake
    mShaderOceanWake->use();
    mShaderOceanWake->setMat4("matViewProj", camera.GetProjection()* camera.GetView());
    mShaderOceanWake->setVec3("eyePos", camera.GetPosition());
    mShaderOceanWake->setVec3("oceanColor", OceanColor);
    
    mShaderOceanWake->setVec3("shipPosition", ShipPosition);
    mShaderOceanWake->setBool("bWaves", bWaves);
    mShaderOceanWake->setFloat("amplitude", 0.15f * shipVelocity);
    mShaderOceanWake->setFloat("kelvinScale", kelvinScale);
    mShaderOceanWake->setFloat("centerFore", centerFore);
    int layer = int(100.0f * fabs(shipVelocity) / sqrt(9.81f * LWL)) + 20;   // Froude is (layer + 1) / 100
    layer = glm::clamp(layer, 0, 99);
    //cout << layer << endl;
    mShaderOceanWake->setInt("texLayer", layer);
    mShaderOceanWake->setFloat("transparency", Transparency);
    mShaderOceanWake->setVec3("sunColor", mSky->SunDiffuse);
    mShaderOceanWake->setInt("bEnvmap", bEnvmap);
    mShaderOceanWake->setFloat("exposure", mSky->Exposure);
    mShaderOceanWake->setBool("bAbsorbance", mSky->bAbsorbance);
    mShaderOceanWake->setVec3("absorbanceColor", mSky->AbsorbanceColor);
    mShaderOceanWake->setFloat("absorbanceCoeff", mSky->AbsorbanceCoeff);
    mShaderOceanWake->setVec3("sunDir", glm::normalize(mSky->SunPosition));
    mShaderOceanWake->setVec2("shipPivot", vec2(ShipPosition.x, ShipPosition.z));
    mShaderOceanWake->setFloat("shipRotation", -ShipRotation);
    mShaderOceanWake->setVec2("shipSize", vec2(TexContourShipW, TexContourShipH));
    if(bTexWakeByVAO)
        mShaderOceanWake->setVec2("wakeSize", vec2(TexWakeVaoSize));
    else
        mShaderOceanWake->setVec2("wakeSize", vec2(TexWakeBufferSize));
    mShaderOceanWake->setFloat("time", 0.001f * t);
    mShaderOceanWake->setBool("bWake", g_bShipWake);
    mShaderOceanWake->setBool("bShowPatch", bShowPatch);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexDisplacements);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, mTexKelvinArray);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mTexEnvironment->id);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTexGradients);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, mTexFoamBuffer);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, mTexFoamDesign->id);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, mTexFoamBubbles->id);

    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, mTexFoam->id);

    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, TexContourShip);

    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_2D, TexReflectionColor);

    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, mTexWaterDuDv->id);

    glActiveTexture(GL_TEXTURE11);
    if (bTexWakeByVAO)
        glBindTexture(GL_TEXTURE_2D, TexWakeVao);
    else
        glBindTexture(GL_TEXTURE_2D, TexWakeBuffer);
    glBindVertexArray(mVao);

    for (auto& patch : vPatches)
    {
        int boatPatchX = patch.first;
        int boatPatchZ = patch.second;
        vec3 center = vec3(PATCH_SIZE * boatPatchX, 0, PATCH_SIZE * boatPatchZ);
        mShaderOceanWake->setMat4("matLocal", glm::translate(mat4(1.0f), center));
        glDrawElements(GL_TRIANGLES, mIndicesCount, GL_UNSIGNED_INT, 0);
    }
    
    sumPatches += vPatches.size();
    //cout << sumPatches << endl;

    glActiveTexture(GL_TEXTURE0);
#pragma endregion
}

