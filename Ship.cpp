#include "Ship.h"
#include <omp.h>
#include "MeshPlaneIntersect.hpp"

#include "clipper/clipper.h"
#ifdef _DEBUG
#pragma comment(lib, "clipper/Debug/x64/clipper.lib")
#else
#pragma comment(lib, "clipper/Release/x64/clipper.lib")
#endif
using namespace Clipper2Lib;

extern float            g_WindSpeedKN;
extern vec2             g_Wind;
extern SoundManager   * g_SoundMgr;
extern bool             g_bPause;
extern Camera           g_Camera;

GLuint                  TexContourShip;                // Texture of the contour of the ship
int                     TexContourShipW;
int                     TexContourShipH;
GLuint                  TexWakeBuffer;
int					    TexWakeBufferSize = 512;
GLuint                  TexWakeVao;
int					    TexWakeVaoSize = 1024;

bool bVAO = true;

Ship::~Ship()
{
    BBoxShape.reset();
    mSmokeLeft.reset();
    mSmokeRight.reset();

    glDeleteVertexArrays(1, &mVao);
    glDeleteBuffers(1, &mVbo);
    glDeleteVertexArrays(1, &mVaoLines);

    mModelFull.reset();
    mPropeller.reset();
    mRudder.reset();
    mLight.reset();
    mRadar1.reset();
    mRadar2.reset();

    mShaderHullColored.reset();
    mShaderWireframe.reset();
    mShaderPressure.reset();
    mShaderCamera.reset();
    mShaderShip.reset();
    mShaderUnicolor.reset();
    mShaderNavLight.reset();

    mTexEnvironment.reset();

    mForceVector.reset();
    mForceApplication.reset();
    mAxis.reset();

    mSoundPower.reset();
    mSoundBowThruster.reset();

    glDeleteVertexArrays(1, &mVaoWake);
    glDeleteBuffers(1, &mVboWake);

    mShaderWakeVao.reset();
    mTexWake.reset();

    mvVertices.clear();
    mvVertices.resize(0);
    mvTris.clear();
    mvTris.resize(0);
    mvVertexColored.clear();
    mvVertexColored.resize(0);
    mvVertSubmerged.clear();
    mvVertSubmerged.resize(0);
    mvVertWaterHeight.clear();
    mvVertWaterHeight.resize(0);

    mvWaterPos.clear();
    mvWaterPos.resize(0);

    glDeleteVertexArrays(1, &mVaoContour1);
    glDeleteBuffers(1, &mVboContour1);
}

void Ship::Init(sShip& ship, Camera& camera)
{
    this->ship = ship;
    
    // Read the mvVertices and the faces
    igl::readOBJ(ship.PathnameHull.c_str(), mV, mF);
    stringstream ssHull;
    ssHull << mV.rows() << " vertices & " << mF.rows() << " faces" << endl;

    // Vertices
    mvVertices.resize(mV.rows());
    mvVertSubmerged.resize(mV.rows());
    mvVertWaterHeight.resize(mV.rows());
    World = mat4(1.0f);
    TransformVertices();

    // Get data
    GetBoundingBox();
    mMass = ship.Mass_t * 1000.0f;              // t -> kg for all physical calculations
    mPowerW = ship.PowerkW * 1000.0f;           // kW -> W for all physical calculations
    mLength = fabs(mBbox.max.x - mBbox.min.x);  // Overall length
    mWidth = fabs(mBbox.max.z - mBbox.min.z);   // Overall width
    mHeight = fabs(mBbox.max.y - mBbox.min.y);  // Overall height
    if (mLength < mWidth) std::swap(mLength, mWidth);
    mDraft = -mBbox.min.y;                      // Below the water level
    mAirDraft = mBbox.max.y;                    // Above the water level
	mBow = vec3(mBbox.max.x, 0.0f, 0.0f);       // Distance to the centre
	mStern = vec3(mBbox.min.x, 0.0f, 0.0f);     // Distance to the centre
    mWakePivot = vec3(mBbox.min.x + 0.5 * mWidth, 0.0f, 0.0f);
    mRudderArea = (mLength * mDraft * 0.01f) * (1.0f + 0.25f * (mWidth / mDraft) * (mWidth / mDraft));    // DNV2 formula for the area of the rudder in m²

    UpdateWorldMatrix();                        // Necessary for several calculations to come

    mvTris.resize(mF.rows());
    GetTriangles();                             // Create the list of the triangles
    GetCentroid();                              // Compute the centre of the volume
    GetSurfaces();                              // Certain surfaces
    GetVolume();                                // Total volume of the hull
    GetMomentsOfInertia();                      // Compute all moments of inertia (Ixx, Iyy, Izz, Ixy, Ixz, Iyz)
    
    // Info to display with interface
    ssHull << "Length/Width : " << std::fixed << std::setprecision(2) << mLength << " m x " << mWidth << " m " << endl;
    ssHull << "Draft : " << std::fixed << std::setprecision(2) << mDraft << endl;
    ssHull << "Mass : " << std::setprecision(0) << int(ship.Mass_t) << " t" << endl;
    InfoHull = ssHull.str();
   
    GetWaterVertices();                         // Create the list of water vertices in the reference patch
    CreateVAO();                                // Create the VAO of the colored hull
    
    // Contour
    vector<vec3> contour = ComputeContour();    // Intersect the mesh with the ocean (Y = 0)
    contour = ArrangeContour(contour);          // Sort the points
    CreateContourVAO1(contour);                 // Create the first contour which has the size of the ship
    contour = OffsetContour(contour, 1.0f);     // Expand the contour with a constant offset
    CreateContourVAO2(contour);                 // Create the second contour which is expanded
    CreateTexWake(contour);                     // Create the texture formed with foam inside the exapnded contour

    // Shaders for the ship
    mShaderHullColored = make_unique<Shader>("Resources/Ship/hull_colored.vert", "Resources/Ship/hull_colored.frag");       // For the hull (colored triangles for Archimede)
    mShaderWireframe = make_unique<Shader>("Resources/Shaders/unicolor.vert", "Resources/Shaders/unicolor.frag", "Resources/Shaders/unicolor.geom");
    mShaderPressure = make_unique<Shader>("Resources/Shaders/unicolor.vert", "Resources/Shaders/unicolor.frag");            // For the forces of pressure (lines)
    mShaderCamera = make_unique<Shader>("Resources/Shaders/camera.vert", "Resources/Shaders/camera.frag");                  // For the ship (the camera is the sun)
    mShaderShip = make_unique<Shader>("Resources/Ship/ship.vert", "Resources/Ship/ship.frag");                              // Enhanced shader for the ship model
    mShaderUnicolor = make_unique<Shader>("Resources/Shaders/unicolor.vert", "Resources/Shaders/unicolor.frag");            // For bounding box & contours
    mShaderNavLight = make_unique<Shader>("Resources/Ship/ship_light.vert", "Resources/Ship/ship_light.frag");              // For the navigation lights of the ship
    
    // Shaders for the wake
    mShaderBuffer = make_unique<Shader>("Resources/Ship/wake_buffer.vert", "Resources/Ship/wake_buffer.frag");              // Wake as an accumulation buffer
    
    mShaderWakeVaoToTex = make_unique<Shader>("Resources/Ship/wake_vao.vert", "Resources/Ship/wake_vao.frag");              // Wake as a projection of VAO on texture
    mShaderGaussH = make_unique<Shader>("Resources/Ship/wake_gauss.vert", "Resources/Ship/wake_gauss_h.frag");              // Gaussian blur - horizontal pass (1)
    mShaderGaussV = make_unique<Shader>("Resources/Ship/wake_gauss.vert", "Resources/Ship/wake_gauss_v.frag");              // Gaussian blur - vertical pass (2)
    
    mShaderWakeVao = make_unique<Shader>("Resources/Shaders/unicolor.vert", "Resources/Shaders/unicolor.frag");             // For the drawing of the vao (as a debug)

    // Wake
    CreateWakeVao();

    mTexWake = make_unique<Texture>();
    mTexWake->CreateFromFile("Resources/Ocean/seamless-seawater-with-foam-1.jpg");
    glBindTexture(GL_TEXTURE_2D, mTexWake->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);

    // W A K E ===========================================================

    // Create accumulation buffer for the wake
    glGenTextures(2, mTexBuffer);
    glGenFramebuffers(2, FBO_BUFFER);
    for (int i = 0; i < 2; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, mTexBuffer[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, TexWakeBufferSize, TexWakeBufferSize, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, FBO_BUFFER[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexBuffer[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            cout << "Error creating FBO for the wake drawing" << endl;
    }
    mScreenQuadWakeBuffer = make_unique<ScreenQuad>();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Creation of the multisample texture (no mipmaps or filters)
    glGenTextures(1, &msTexWakeVAO);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msTexWakeVAO);
    int samples = 4;
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_R8, TexWakeVaoSize, TexWakeVaoSize, GL_TRUE);

    // Multisample FBO creation
    glGenFramebuffers(1, &msFBO_WAKE);
    glBindFramebuffer(GL_FRAMEBUFFER, msFBO_WAKE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msTexWakeVAO, 0);

    // Creating a classic FBO with simple texture for resolution (blit)
    glGenFramebuffers(1, &FBO_BLIT_WAKE);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_BLIT_WAKE);

    glGenTextures(1, &TexBlit);
    glBindTexture(GL_TEXTURE_2D, TexBlit);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, TexWakeVaoSize, TexWakeVaoSize, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TexBlit, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "Error creating FBO for the wake post processing" << endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // FBO and texture for the 2 passes of the gaussian blur (horizontal and vertical)
    glGenTextures(1, &mTexGauss1);
    glBindTexture(GL_TEXTURE_2D, mTexGauss1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, TexWakeVaoSize, TexWakeVaoSize, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &FBO_GAUSS1);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_GAUSS1);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexGauss1, 0);

    glGenTextures(1, &TexWakeVao);
    glBindTexture(GL_TEXTURE_2D, TexWakeVao);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, TexWakeVaoSize, TexWakeVaoSize, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &FBO_GAUSS2);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_GAUSS2);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TexWakeVao, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "Error creating FBO for the Gauss passes of the wake" << endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //========================================================================
    
    // Create environment map
    mTexEnvironment = make_unique<Texture>();
    mTexEnvironment->CreateFromDDSFile("Resources/Ocean/ocean_env.dds");

    // Models
    mModelFull = make_unique<Model>(ship.PathnameFull);
    stringstream ssFull;
    ssFull << mModelFull->totalVertices << " vertices & " << mModelFull->totalFaces << " faces" << endl;
    InfoFull = ssFull.str();
    mPropeller = make_unique<Model>(ship.PathnamePropeller);
    mRudder = make_unique<Model>(ship.PathnameRudder);
    mRadar1 = make_unique<Model>(ship.PathnameRadar1);
    mRadar2 = make_unique<Model>(ship.PathnameRadar2);

    sBBvec3 bb = mModelFull->calculateBoundingBox();
    BBoxShape = make_unique<BBox>(bb.min, bb.max);
    BBoxShape->bVisible = false;
    mForceVector = make_unique<Cube>();
    mForceApplication = make_unique<Sphere>(0.1f, 16);
    mAxis = make_unique<Cube>();
    mLight = make_unique<Sphere>(0.1f, 16);

    // Sounds
    g_SoundMgr->setListenerPosition(camera.GetPosition());
    g_SoundMgr->setListenerOrientation(camera.GetAt(), camera.GetUp());
    mSoundPower = make_unique<Sound>(ship.ThrustSound);
    mSoundPower->setVolume(0.25f);
    mSoundPower->setPosition(ship.Position + ship.PosPower);
    mSoundPower->setLooping(true);
    mSoundPower->adjustDistances();
    if (bSound)
        mSoundPower->play();
    mSoundBowThruster = make_unique<Sound>(ship.BowThrusterSound);
    mSoundBowThruster->setVolume(0.25f);
    mSoundBowThruster->setPosition(ship.Position + ship.PosBowThruster);
    mSoundBowThruster->setLooping(true);
    mSoundBowThruster->adjustDistances();

    // Particles
	mSmokeLeft = make_unique<Smoke>();
    if (this->ship.nChimney == 2) mSmokeRight = make_unique<Smoke>();

    ResetVelocities();

    // Forces
    bMotion = false;
    SetForces(true);

    bSound = true;
}
void Ship::SetOcean(Ocean* ocean)
{ 
    mOcean = ocean; 
    pDisplacement = ocean->GetPixelsDisplacement(); 
};
void Ship::GetTriangles()
{
    sTriangle tri;
    for (int i = 0; i < mF.rows(); ++i)
    {
        tri.I[0] = mF(i, 0);
        tri.I[1] = mF(i, 1);
        tri.I[2] = mF(i, 2);
        vec3 u = mvVertices[tri.I[1]] - mvVertices[tri.I[0]];
        vec3 v = mvVertices[tri.I[2]] - mvVertices[tri.I[0]];
        vec3 a = glm::cross(v, u);
        tri.Area = 0.5f * sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
        tri.Normal = glm::normalize(a);
        mvTris[i] = tri;
    }
}
void Ship::GetCentroid()
{
    mCentroid = vec3(0.0f);
    for (const auto& tri : mvTris)
        mCentroid += mvVertices[tri.I[0]] + mvVertices[tri.I[1]] + mvVertices[tri.I[2]];
    
    if(mvTris.size())
        mCentroid /= (mvTris.size() * 3.0f);

#ifdef PROPERTIES
    cout << "Centroid : ( " << mCentroid.x << ", " << mCentroid.y << ", " << mCentroid.z << " )" << endl;
#endif
}
void Ship::GetSurfaces()
{
    // Area
    AreaXZ = mLength * mWidth;
    // Cube root of the area in the XZ plane
    AreaXZ_RacCub = pow(AreaXZ, 1.0f / 3.0f);
    // Wet area
    AreaWettedMax = 0.0f;
    for (auto& tri : mvTris)
        AreaWettedMax += tri.Area;

#ifdef PROPERTIES   
    cout << "Surface XZ : " << AreaXZ << " m2" << endl;
    cout << "Surface : " << AreaWettedMax << " m2" << endl;
#endif
}
void Ship::GetVolume()
{
    vec3 a, b, c;
    mVolume = 0.0f;
    for (auto& tri : mvTris)
    {
        a.x = mvVertices[tri.I[0]].x;
        a.y = mvVertices[tri.I[0]].y;
        a.z = mvVertices[tri.I[0]].z;

        b.x = mvVertices[tri.I[1]].x;
        b.y = mvVertices[tri.I[1]].y;
        b.z = mvVertices[tri.I[1]].z;

        c.x = mvVertices[tri.I[2]].x;
        c.y = mvVertices[tri.I[2]].y;
        c.z = mvVertices[tri.I[2]].z;

        // Signed volume of this tetrahedron
        mVolume += a.x * b.y * c.z + a.y * b.z * c.x + b.x * c.y * a.z - (c.x * b.y * a.z + b.x * a.y * c.z + c.y * b.z * a.x);
    }
    mVolume /= 6.0f;

#ifdef PROPERTIES
    cout << "Volume : " << mVolume << " m3" << endl;
#endif
}
void Ship::GetMomentsOfInertia()
{
    mVolume = 0.0f;
    Ixx = 0.0f;
    Iyy = 0.0f;
    Izz = 0.0f;
    Ixy = 0.0f;
    Ixz = 0.0f;
    Iyz = 0.0f;

    // Calculation of total volume and moments of inertia
    for (const auto& tri : mvTris)
    {
        vec3 p0 = mvVertices[tri.I[0]];
        vec3 p1 = mvVertices[tri.I[1]];
        vec3 p2 = mvVertices[tri.I[2]];

        float volume = std::abs(
            (mCentroid.x - p2.x) * ((p0.y - p2.y) * (p1.z - p2.z) - (p1.y - p2.y) * (p0.z - p2.z)) -
            (mCentroid.y - p2.y) * ((p0.x - p2.x) * (p1.z - p2.z) - (p1.x - p2.x) * (p0.z - p2.z)) +
            (mCentroid.z - p2.z) * ((p0.x - p2.x) * (p1.y - p2.y) - (p1.x - p2.x) * (p0.y - p2.y))
        ) / 6.0f;
        mVolume += volume;

        Ixx += (p0.y * p0.y + p1.y * p1.y + p2.y * p2.y + p0.y * p1.y + p1.y * p2.y + p2.y * p0.y + p0.z * p0.z + p1.z * p1.z + p2.z * p2.z + p0.z * p1.z + p1.z * p2.z + p2.z * p0.z) * volume / 10.0f;
        Iyy += (p0.x * p0.x + p1.x * p1.x + p2.x * p2.x + p0.x * p1.x + p1.x * p2.x + p2.x * p0.x + p0.z * p0.z + p1.z * p1.z + p2.z * p2.z + p0.z * p1.z + p1.z * p2.z + p2.z * p0.z) * volume / 10.0f;
        Izz += (p0.x * p0.x + p1.x * p1.x + p2.x * p2.x + p0.x * p1.x + p1.x * p2.x + p2.x * p0.x + p0.y * p0.y + p1.y * p1.y + p2.y * p2.y + p0.y * p1.y + p1.y * p2.y + p2.y * p0.y) * volume / 10.0f;

        Ixy += (2 * p0.x * p0.y + 2 * p1.x * p1.y + 2 * p2.x * p2.y + p0.x * p1.y + p0.x * p2.y + p1.x * p0.y + p1.x * p2.y + p2.x * p0.y + p2.x * p1.y) * volume / 20.0f;
        Ixz += (2 * p0.x * p0.z + 2 * p1.x * p1.z + 2 * p2.x * p2.z + p0.x * p1.z + p0.x * p2.z + p1.x * p0.z + p1.x * p2.z + p2.x * p0.z + p2.x * p1.z) * volume / 20.0f;
        Iyz += (2 * p0.y * p0.z + 2 * p1.y * p1.z + 2 * p2.y * p2.z + p0.y * p1.z + p0.y * p2.z + p1.y * p0.z + p1.y * p2.z + p2.y * p0.z + p2.y * p1.z) * volume / 20.0f;
    }

    if (mVolume != 0.0f)
    {
        // Calculation of density
        float densite = mMass / mVolume;

        // Adjustment of moments of inertia relative to the barycenter
        Ixx = densite * Ixx - mMass * (ship.PosGravity.y * ship.PosGravity.y + ship.PosGravity.z * ship.PosGravity.z);
        Iyy = densite * Iyy - mMass * (ship.PosGravity.x * ship.PosGravity.x + ship.PosGravity.z * ship.PosGravity.z);
        Izz = densite * Izz - mMass * (ship.PosGravity.x * ship.PosGravity.x + ship.PosGravity.y * ship.PosGravity.y);
        Ixy = densite * Ixy + mMass * ship.PosGravity.x * ship.PosGravity.y;
        Ixz = densite * Ixz + mMass * ship.PosGravity.x * ship.PosGravity.z;
        Iyz = densite * Iyz + mMass * ship.PosGravity.y * ship.PosGravity.z;
    }

#ifdef PROPERTIES
    // Displaying results
    cout << "============================" << endl;
    cout << "Moments d'inertie volumiques" << endl;
    cout << "Volume total : " << mVolume << " m3" << endl;
    cout << "Ixx = " << Ixx << " kg/m2" << endl;
    cout << "Iyy = " << Iyy << " kg/m2" << endl;
    cout << "Izz = " << Izz << " kg/m2" << endl;
    cout << "Ixy = " << Ixy << " kg/m2" << endl;
    cout << "Ixz = " << Ixz << " kg/m2" << endl;
    cout << "Iyz = " << Iyz << " kg/m2" << endl;
    cout << endl;
#endif
}
void Ship::GetWaterVertices()
{
    // Positions
    for (int z = 0; z <= mOcean->MESH_SIZE; ++z)
    {
        vector<vec3> vPos;
        for (int x = 0; x <= mOcean->MESH_SIZE; ++x)
        {
            int index = z * mOcean->MESH_SIZE_1 + x;
            vec3 v;
            v.x = (x - mOcean->MESH_SIZE / 2.0f) * mOcean->PATCH_SIZE / mOcean->MESH_SIZE;
            v.y = 0.0f;
            v.z = (z - mOcean->MESH_SIZE / 2.0f) * mOcean->PATCH_SIZE / mOcean->MESH_SIZE;
            vPos.push_back(v);
        }
        mvWaterPos.push_back(vPos);
    }
}
void Ship::CreateVAO()
{
    // Converting mvVertices, Normals and Colors
    mvVertexColored = vector<float>(mvTris.size() * 3 * 6);
    int index = 0;
    for (const auto& tri : mvTris)
    {
        for (int j = 0; j < 3; ++j)
        {
            // Position
            mvVertexColored[index++] = mvVertices[tri.I[j]].x;   // x
            mvVertexColored[index++] = mvVertices[tri.I[j]].y;   // y
            mvVertexColored[index++] = mvVertices[tri.I[j]].z;   // z

            // Color
            mvVertexColored[index++] = tri.Color.r;   // r
            mvVertexColored[index++] = tri.Color.g;   // g
            mvVertexColored[index++] = tri.Color.b;   // b
        }
    }

    // Generation of mvIndices
    vector<unsigned int> indices(mF.rows() * 3);
    for (unsigned int i = 0; i < indices.size(); ++i)
        indices[i] = i;
    mIndicesFull = indices.size();

    glGenVertexArrays(1, &mVao);
    glGenBuffers(1, &mVbo);
    glGenBuffers(1, &mEbo);

    glBindVertexArray(mVao);

    glBindBuffer(GL_ARRAY_BUFFER, mVbo);
    glBufferData(GL_ARRAY_BUFFER, mvVertexColored.size() * sizeof(float), mvVertexColored.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Configuring vertex attributes

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
void Ship::CreatePressureLinesVAO()
{
    float coeff = 0.001f * (200000.0f / mMass) * (6000.0 / mF.rows());

    vector<vec3> linePoints;
    for (auto& tri : mvTris)
    {
        if (tri.WaterStatus != 0) // at least, 1 pt under water
        {
            linePoints.push_back(tri.CoG);
            linePoints.push_back(tri.CoG - tri.Normal * tri.fPressure * coeff);
        }
    }
    mLinesCount = linePoints.size();

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, linePoints.size() * sizeof(vec3), linePoints.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &mVaoLines);
    glBindVertexArray(mVaoLines);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);

    glEnableVertexAttribArray(0);
}
void Ship::SetForces(bool bActive)
{
    // Buoyancy
    Archimede.bActive = bActive;
    Gravity.bActive = bActive;
    ResistanceHeave.bActive = bActive;
    // Power
    Thrust.bActive = bActive;
    ResistanceViscous.bActive = bActive;
    ResistanceWaves.bActive = bActive;
    ResistanceResidual.bActive = bActive;
    BowThrust.bActive = bActive;
    // Rudder
    RudderLift.bActive = bActive;
    RudderDrag.bActive = bActive;
    Centrifugal.bActive = bActive;
    // Wind
    WindRotation.bActive = bActive;
    WindFront.bActive = bActive;
    WindRear.bActive = bActive;
}

// Contour
vector<vec3> Ship::ComputeContour()
{
    using Intersector = MeshPlaneIntersect<float, int>;

    // Local lambdas for conversion
    auto toVec3D = [](const vec3& v) -> Intersector::Vec3D {
        return { static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z) };
        };

    auto toFace = [](const sTriangle& tri) -> Intersector::Face {
        return { tri.I[0], tri.I[1], tri.I[2] };
        };

    // Converting vertices
    vector<Intersector::Vec3D> vertices;
    vertices.reserve(mvVertices.size());
    for (const auto& v : mvVertices)
        vertices.push_back(toVec3D(v));

    // Face Conversion
    vector<Intersector::Face> faces;
    faces.reserve(mvTris.size());
    for (const auto& tri : mvTris)
        faces.push_back(toFace(tri));

    // Creating the mesh
    Intersector::Mesh mesh(vertices, faces);

    // Plan Y=0, normal (0,1,0)
    Intersector::Plane plane({ 0,0,0 }, { 0,1,0 });

    // Intersects the mesh with the plane
    auto result = mesh.Intersect(plane);

    // Each element of result is a contour
    vector<vec3> contour;
    for (const auto& path : result)
    {
        // path.points : vector<Vec3D> points of the contour, path.isClosed : boolean, true if the contour is closed
        for (const auto& p : path.points)
            contour.emplace_back(static_cast<float>(p[0]), static_cast<float>(p[1]), static_cast<float>(p[2]));
    }
    return contour;
}
vector<vec3> Ship::ArrangeContour(const vector<vec3>& contourUnordered)
{
    // CAUTION: Only use if the points form a simple path, without holes or intersections.
    vector<vec3> contourOrdered;
    contourOrdered.reserve(contourUnordered.size());

    vector<bool> used(contourUnordered.size(), false);

    // Démarre au 1er point
    size_t current = 0;
    contourOrdered.push_back(contourUnordered[current]);
    used[current] = true;

    for (size_t i = 1; i < contourUnordered.size(); ++i) 
    {
        float bestDist2 = std::numeric_limits<float>::max();
        size_t nextIdx = -1;
        // Find the nearest unused one
        for (size_t j = 0; j < contourUnordered.size(); ++j) 
        {
            if (used[j]) continue;
            float dx = contourUnordered[current].x - contourUnordered[j].x;
            float dy = contourUnordered[current].y - contourUnordered[j].y;
            float dz = contourUnordered[current].z - contourUnordered[j].z;
            float dist2 = dx * dx + dy * dy + dz * dz;
            if (dist2 < bestDist2) 
            {
                bestDist2 = dist2;
                nextIdx = j;
            }
        }
        if (nextIdx == -1) break; // Security
        contourOrdered.push_back(contourUnordered[nextIdx]);
        used[nextIdx] = true;
        current = nextIdx;
    }
    return contourOrdered;
}
vector<vector<vec2>> offsetContourWithClipper( const vector<vec2>& contour, float offset, JoinType joinType = JoinType::Round, EndType endType = EndType::Polygon)
{
    const double scaleClipper = 1e6; // scale factor to maintain accuracy of clipper (calculations on int64)
    
    // Convert contour to Clipper int64
    Path64 path;
    path.reserve(contour.size());
    for (const auto& pt : contour)
        path.emplace_back(static_cast<int64_t>(pt.x * scaleClipper), static_cast<int64_t>(pt.y * scaleClipper));

    // ClipperOffset manages multiple paths, we create a vector of paths
    Paths64 paths;
    paths.push_back(path);

    // Offset in integer units, we convert the float offset to int64_t
    int64_t intOffset = static_cast<int64_t>(offset * scaleClipper);

    ClipperOffset offsetter;
    offsetter.AddPaths(paths, joinType, endType);

    Paths64 solution;
    offsetter.Execute(intOffset, solution);

    // Convert each solution polygon to glm::vec2 (float)
    vector<vector<vec2>> result;
    result.reserve(solution.size());
    for (const Path64& p : solution)
    {
        vector<vec2> res;
        result.reserve(p.size());
        for (const auto& pt : p)
            res.emplace_back(static_cast<float>(pt.x) / static_cast<float>(scaleClipper), static_cast<float>(pt.y) / static_cast<float>(scaleClipper));
        result.push_back(res);
    }

    return result;
}
vector<vec3> Ship::OffsetContour(const vector<vec3>& contour, float offset)
{
    // Convert contour in 3D to contour 2D
    vector<vec2> contour2d;
    for (const auto& v : contour)
        contour2d.push_back(vec2(v.x, v.z));

    // Execute the offset with clipper library
    auto newContour = offsetContourWithClipper(contour2d, offset);

    // Convert the result to 3D
    vector<vec3> result;
    size_t totalSize = 0;
    for (const auto& contour : newContour) 
        totalSize += contour.size();
    result.reserve(totalSize);

    for (const auto& contour : newContour) 
        for (const vec2& p : contour) 
            result.emplace_back(p.x, 0.f, p.y); // Level of the water

    return result;
}
bool isPointInPolygon(const vec2& pt, const vector<vec2>& contour2D)
{
    // Tests if a point (x, z) is inside a polygon (ray algorithm)
    // contour2D : vector<vec2> (x, z)
    
    bool inside = false;
    size_t n = contour2D.size();
    for (size_t i = 0, j = n - 1; i < n; j = i++) 
    {
        const vec2& vi = contour2D[i];
        const vec2& vj = contour2D[j];
        if (((vi.y > pt.y) != (vj.y > pt.y)) && (pt.x < (vj.x - vi.x) * (pt.y - vi.y) / (vj.y - vi.y + 1e-8f) + vi.x))
            inside = !inside;
    }
    return inside;
}
float minDistanceToContour(const vec2& pt, const vector<vec2>& contour2D) 
{
    // Minimum distance between a point and a polyline (to draw the outline in white)
    
    float minDist = numeric_limits<float>::max();
    size_t n = contour2D.size();
    for (size_t i = 0; i < n; ++i) 
    {
        const vec2& a = contour2D[i];
        const vec2& b = contour2D[(i + 1) % n];
        vec2 ab = b - a;
        vec2 ap = pt - a;
        float t = glm::clamp(glm::dot(ap, ab) / glm::dot(ab, ab), 0.0f, 1.0f);
        vec2 proj = a + t * ab;
        float dist = glm::distance(pt, proj);
        if (dist < minDist) minDist = dist;
    }
    return minDist;
}
void Ship::CreateTexWake(const vector<vec3>& contour)
{
    TexContourShipW = std::ceil((mLength + 5.0f) / 10.0f) * 10.0f;
    TexContourShipH = std::ceil((mWidth + 5.0f) / 10.0f) * 10.0f;

    vector<float> mask(TexContourShipW * TexContourShipH, 0.0f);

    // Determines the actual boundaries of the contour
    float minX = contour[0].x, maxX = contour[0].x;
    float minZ = contour[0].z, maxZ = contour[0].z;
    for (const auto& v : contour) 
    {
        minX = std::min(minX, v.x); maxX = std::max(maxX, v.x);
        minZ = std::min(minZ, v.z); maxZ = std::max(maxZ, v.z);
    }

    // Calculating the offset to center the outline in the texture
    float spanX = maxX - minX;
    float spanZ = maxZ - minZ;
    float offsetX = ((float)TexContourShipW - spanX) * 0.5f - minX;
    float offsetZ = ((float)TexContourShipH - spanZ) * 0.5f - minZ;

    // Projects the contour in 2D XZ and applies centering
    vector<vec2> contour2D;
    for (const auto& v : contour)
        contour2D.emplace_back(v.x + offsetX, v.z + offsetZ);
    
    float edgeWidth = 1.0f; // Épaisseur du contour adouci, en pixels

    // For each pixel, determine whether it is within the outline or on the edge
    for (int j = 0; j < TexContourShipH; ++j) 
    {
        for (int i = 0; i < TexContourShipW; ++i)
        {
            vec2 pt(i + 0.5f, j + 0.5f);
            bool inside = isPointInPolygon(pt, contour2D);
            float dist = minDistanceToContour(pt, contour2D);
            float sd = inside ? -dist : dist; // Signed distance

            float bias = 0.0f;
            if (sd <= 0.0f)              // inside or on the outline
                bias = 1.0f;
            else if (sd < edgeWidth)     // degraded outside
                bias = 1.0f - (sd / edgeWidth);
            else                        // far outside
                bias = 0.0f;

            mask[j * TexContourShipW + i] = bias;
        }
    }

    glGenTextures(1, &TexContourShip);
    glBindTexture(GL_TEXTURE_2D, TexContourShip);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, TexContourShipW, TexContourShipH, 0, GL_RED, GL_FLOAT, mask.data());

    glBindTexture(GL_TEXTURE_2D, 0);
    //SaveTexture2D(TexContourShip, TexContourShipW, TexContourShipH, 1, GL_RED, "Outputs/TexContourShip.png");
}
void Ship::CreateContourVAO1(vector<vec3>& contour)
{
    glGenVertexArrays(1, &mVaoContour1);
    glGenBuffers(1, &mVboContour1);

    glBindVertexArray(mVaoContour1);

    glBindBuffer(GL_ARRAY_BUFFER, mVboContour1);
    glBufferData(GL_ARRAY_BUFFER, contour.size() * sizeof(vec3), contour.data(), GL_STATIC_DRAW );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0 );

    glBindVertexArray(0);

    mIndicesContour1 = contour.size();
}
void Ship::CreateContourVAO2(vector<vec3>& contour)
{
    glGenVertexArrays(1, &mVaoContour2);
    glGenBuffers(1, &mVboContour2);

    glBindVertexArray(mVaoContour2);

    glBindBuffer(GL_ARRAY_BUFFER, mVboContour2);
    glBufferData(GL_ARRAY_BUFFER, contour.size() * sizeof(vec3), contour.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);

    glBindVertexArray(0);

    mIndicesContour2 = contour.size();
}

// Support
vec3 Ship::GetVerticeAtMeshIndex(int x, int z)
{
    int xx = x;
    int zz = z;

    int i = 0;
    int j = 0;

    while (xx < 0)
    {
        xx += mOcean->MESH_SIZE;
        i--;
    }
    while (zz < 0)
    {
        zz += mOcean->MESH_SIZE;
        j--;
    }

    while (xx >= mOcean->MESH_SIZE)
    {
        xx -= mOcean->MESH_SIZE;
        i++;
    }
    while (zz >= mOcean->MESH_SIZE)
    {
        zz -= mOcean->MESH_SIZE;
        j++;
    }

    vec3 pos;
    // Correspondence between MESH coordinates and FFT coordinates
    int xFft = (xx - mOcean->MESH_SIZE / 2) * mOcean->FFT_SIZE / mOcean->MESH_SIZE + mOcean->FFT_SIZE / 2;
    int yFft = (zz - mOcean->MESH_SIZE / 2) * mOcean->FFT_SIZE / mOcean->MESH_SIZE + mOcean->FFT_SIZE / 2;
    int index = 4 * (yFft * mOcean->FFT_SIZE + xFft);

    pos.x = mvWaterPos[zz][xx].x + i * mOcean->PATCH_SIZE + pDisplacement[index + 0];
    pos.y = pDisplacement[index + 1];
    pos.z = mvWaterPos[zz][xx].z + j * mOcean->PATCH_SIZE + pDisplacement[index + 2];

    return pos;
}
int Ship::GetHeightFast(vec3& pos)
{
    // Update pos vector and return the number of computations (higher the lambda is, higher the computations are)

    int x = pos.x * mOcean->MESH_SIZE / mOcean->PATCH_SIZE + mOcean->MESH_SIZE / 2;
    int z = pos.z * mOcean->MESH_SIZE / mOcean->PATCH_SIZE + mOcean->MESH_SIZE / 2;

    vec3 posR = pos;

    while (x < 0)
    {
        x += mOcean->MESH_SIZE;
        posR.x += mOcean->PATCH_SIZE;
    }
    while (z < 0)
    {
        z += mOcean->MESH_SIZE;
        posR.z += mOcean->PATCH_SIZE;
    }

    while (x >= mOcean->MESH_SIZE)
    {
        x -= mOcean->MESH_SIZE;
        posR.x -= mOcean->PATCH_SIZE;
    }
    while (z >= mOcean->MESH_SIZE)
    {
        z -= mOcean->MESH_SIZE;
        posR.z -= mOcean->PATCH_SIZE;
    }

    int n = 0;
    int index;
    float xOrig, xReal;
    float zOrig, zReal;
    vec3 p1, p2, p3, p4;

    // First level of searching, step by step, x then z

    // X to the left
    int xGrid;
    for (xGrid = x; xGrid >= 0; xGrid--)
    {
        n++;
        xReal = GetVerticeAtMeshIndex(xGrid, z).x;
        if (xReal < posR.x)
            break;
    }
    int indexX1 = xGrid;
    if (x == indexX1)
    {
        // X to the right
        for (xGrid = x; xGrid < mOcean->MESH_SIZE_1; xGrid++)
        {
            n++;
            xReal = GetVerticeAtMeshIndex(xGrid, z).x;
            if (xReal > posR.x)
                break;
        }
        indexX1 = xGrid - 1;
    }


    // Z to the bottom
    int zGrid;
    for (zGrid = z; zGrid >= 0; zGrid--)
    {
        n++;
        zReal = GetVerticeAtMeshIndex(x, zGrid).z;
        if (zReal < posR.z)
            break;
    }
    int indexZ1 = zGrid;
    if (z == indexZ1)
    {
        // Z to the top
        for (zGrid = z; zGrid < mOcean->MESH_SIZE_1; zGrid++)
        {
            n++;
            zReal = GetVerticeAtMeshIndex(x, zGrid).z;
            if (zReal > posR.z)
                break;
        }
        indexZ1 = zGrid - 1;
    }
    if (indexZ1 == -1)  indexZ1 = 0;

    // Try with these indexes
    p1 = GetVerticeAtMeshIndex(indexX1, indexZ1);          // Bottom Left
    p2 = GetVerticeAtMeshIndex(indexX1 + 1, indexZ1);      // Bottom Right
    p3 = GetVerticeAtMeshIndex(indexX1 + 1, indexZ1 + 1);  // Top Right

    if (InterpolateTriangle(p1, p2, p3, posR))
    {
        pos.y = posR.y;
        return n;
    }

    p4 = GetVerticeAtMeshIndex(indexX1, indexZ1 + 1);      // Top Left
    if (InterpolateTriangle(p1, p3, p4, posR))
    {
        pos.y = posR.y;
        return n;
    }

    // Second level of searching. From the indexes, start around the last indexes on a 5 x 5 basis
    for (int i = -2; i <= 2; i++)
    {
        for (int j = -2; j <= 2; j++)
        {
            n++;
            p1 = GetVerticeAtMeshIndex(indexX1 + i, indexZ1 + j);          // Bottom Left
            p2 = GetVerticeAtMeshIndex(indexX1 + 1 + i, indexZ1 + j);      // Bottom Right
            p3 = GetVerticeAtMeshIndex(indexX1 + 1 + i, indexZ1 + 1 + j);  // Top Right

            if (InterpolateTriangle(p1, p2, p3, posR))
            {
                pos.y = posR.y;
                return n;
            }

            p4 = GetVerticeAtMeshIndex(indexX1 + i, indexZ1 + 1 + j);      // Top Left
            if (InterpolateTriangle(p1, p3, p4, posR))
            {
                pos.y = posR.y;
                return n;
            }
        }
    }

    // Second level of searching, extend the grid in another method
    n += GetHeightSlow(pos);

    return n;
}
int Ship::GetHeightSlow(vec3& pos)
{
    int x = pos.x * mOcean->MESH_SIZE / mOcean->PATCH_SIZE + mOcean->MESH_SIZE / 2;
    int z = pos.z * mOcean->MESH_SIZE / mOcean->PATCH_SIZE + mOcean->MESH_SIZE / 2;

    int n = 0;
    int index;
    float xOrig, xReal;
    float zOrig, zReal;

    // Find the index in pDisplacement

    vec3 p1, p2, p3, p4;
    int xGrid, zGrid;
    bool bAssert12, bAssert43, bAssert14, bAssert23;
    for (xGrid = x - 20; xGrid <= x + 20; xGrid++)
    {
        for (zGrid = z - 20; zGrid <= z + 20; zGrid++)
        {
            n++;
            p1 = GetVerticeAtMeshIndex(xGrid, zGrid);          // Bottom Left
            p2 = GetVerticeAtMeshIndex(xGrid + 1, zGrid);      // Bottom Right
            p3 = GetVerticeAtMeshIndex(xGrid + 1, zGrid + 1);  // Top Right

            if (InterpolateTriangle(p1, p2, p3, pos))
                return n;
            
            p4 = GetVerticeAtMeshIndex(xGrid, zGrid + 1);      // Top Left
            if (InterpolateTriangle(p1, p3, p4, pos))
                return n;
        }
    }
   
    // No triangle found
    pos.y = 0.0f;
    return 0;
}
void Ship::UpdateWorldMatrix()
{
    World = mat4(1.0f);
    World = glm::translate(World, ship.Position);
    World = glm::rotate(World, Yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    World = glm::rotate(World, Roll, glm::vec3(1.0f, 0.0f, 0.0f));
    World = glm::rotate(World, Pitch, glm::vec3(0.0f, 0.0f, 1.0f));
}
void Ship::ResetVelocities()
{
    YawRate = 0.0f;
    Pitch = 0.0f;
    Roll = 0.0f;
    
    HeaveVelocity = 0.0f;
    SurgeVelocity = 0.0f;
    PitchVelocity = 0.0f;
    RollVelocity = 0.0f;
    DriftVelocity = 0.0f;
}
void Ship::TransformVertices()
{
    vec3 p;
    for (int i = 0; i < mV.rows(); ++i)
    {
        p = { static_cast<float>(mV(i, 0)) , static_cast<float>(mV(i, 1)) ,static_cast<float>(mV(i, 2)) };
        p = vec3(World * vec4(p, 1.0f));
        mvVertices[i] = p;
    }
}
vec3 Ship::TransformPosition(vec3 v)
{
    return vec3(World * vec4(v, 1.0f));
}
vec3 Ship::TransformVector(vec3 v)
{
    return vec3(World * vec4(v, 0.0f));
}
void Ship::GetHDG()
{
    HDG = fmod(450.0f - glm::degrees(Yaw), 360.0f);
    while (HDG < 0.0f)      HDG += 360.0f;
    while (HDG > 360.0f)    HDG -= 360.0f;
}
void Ship::SetYawFromHDG(float hdg)
{
    float deg_Yaw = fmod(450.0f - hdg, 360.0f);
    if (deg_Yaw < 0.0f) 
        deg_Yaw += 360.0f;
    Yaw = glm::radians(deg_Yaw);
}
void Ship::GetBoundingBox()
{
    mBbox.min = vec3(FLT_MAX);
    mBbox.max = vec3(FLT_MIN);

    if (mvVertices.size() > 0)
    {
        for (auto& v : mvVertices)
        {
            mBbox.min = glm::min(mBbox.min, v);
            mBbox.max = glm::max(mBbox.max, v);
        }
	}
    else
    {
        mBbox.min = vec3(0.0f);
        mBbox.max = vec3(0.0f);
    }
}
void Ship::GetHeightOfAllVertices()
{
    int nSearch = 0;
    vec3 pWater;
    for (unsigned int i = 0; i < mvVertices.size(); i++)
    {
        pWater = mvVertices[i];
        nSearch += GetHeightFast(pWater);
        mvVertSubmerged[i] = (mvVertices[i].y < pWater.y) ? 1 : 0;  // 0 = under water, 1 = above
        mvVertWaterHeight[i] = mvVertices[i].y - pWater.y;
    }
    if (mvVertices.size())
        WaterSearch = int(nSearch / mvVertices.size());
}
void Ship::GetTrisUnderWater()
{
    int vertex = 0;
    for (auto& tri : mvTris)
    {
        tri.WaterStatus = 0;
        for (unsigned int i = 0; i < 3; i++)
        {
            tri.bUnder[i] = mvVertSubmerged[tri.I[i]];
            tri.WaterStatus += tri.bUnder[i];
        }
        switch (tri.WaterStatus)
        {
        case 0: tri.Color = vec3(0.5f, 0.5f, 0.5f); break;   // Above water 0/3
        case 1: tri.Color = vec3(0.6f, 0.6f, 1.0f); break;   // Under water 1/3
        case 2: tri.Color = vec3(0.3f, 0.3f, 1.0f); break;   // Under water 2/3
        case 3: tri.Color = vec3(0.0f, 0.0f, 1.0f); break;   // Under water 3/3
        }
    }

    // Update the vertex array object
    int index = 0;
    for (const auto& tri : mvTris)
    {
        for (int j = 0; j < 3; ++j)
        {
            index += 3;
            // Color
            mvVertexColored[index++] = tri.Color.r;   // r
            mvVertexColored[index++] = tri.Color.g;   // g
            mvVertexColored[index++] = tri.Color.b;   // b
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, mVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mvVertexColored.size() * sizeof(float), mvVertexColored.data());
}

// Compute forces
void Ship::Update(float time)
{
    if (g_bPause)
        return;

    static float prevTime = 0.0f;
    float dt = time - prevTime;
    mDt = dt;
    static float prevYaw = 0.0f;

    UpdateWorldMatrix();

    // Preparation
    TransformVertices();
    // Computation
    GetHeightOfAllVertices();
    GetTrisUnderWater();
    // Forces
    ComputeArchimede();
    ComputeGravity();
    ComputeHeave(dt);
    ComputeThrust(dt);
    ComputeResistanceViscous(dt);
    ComputeResistanceWaves(dt);
    ComputeResistanceResidual(dt);
    ComputeBowThrust(dt);
    ComputeRudder(dt);
    ComputeWind(dt);
    ComputeCentrifugal(dt);
    // Result
    ComputeForces(dt);
    // Sounds
    UpdateSounds();

    UpdateSmoke(dt);
    ComputeAutopilot(dt);

    if (bPressure)
        CreatePressureLinesVAO();

    UpdateWakeVao();
    if(bVAO)
        UpdateTextureWakeVao();
    else
        UpdateWakeBuffer();

    VariationYawSigned = (Yaw - prevYaw) / dt;
    YawRate = fabs(Yaw - prevYaw) / dt;   // rad/s
    prevYaw = Yaw;

    prevTime = time;
}
void Ship::ComputeArchimede()
{
    // Force calculated as the sum of the hydrostatic pressures acting on the submerged(or partially submerged) triangles of the hull
    
    AreaWetted = 0.0f;
    Archimede.Magnitude = 0.0f;
    Archimede.Vector = vec3(0.0f);
    Archimede.Position = vec3(0.0f);
    float tmpSumPressure = 0.0f;
    float intensity = 0.0f;

    vec3 Min = vec3(FLT_MAX);
    vec3 Max = -vec3(FLT_MAX);

    for (auto& tri : mvTris)
    {
        if (tri.WaterStatus != 0) // at least, 1 pt under water
        {
            vec3 u = mvVertices[tri.I[1]] - mvVertices[tri.I[0]];
            vec3 v = mvVertices[tri.I[2]] - mvVertices[tri.I[0]];
            vec3 a = glm::cross(v, u);
            tri.Normal = glm::normalize(a);
            tri.CoG = (mvVertices[tri.I[0]] + mvVertices[tri.I[1]] + mvVertices[tri.I[2]]) / 3.0f;
            
            if (tri.CoG.x < Min.x) Min.x = tri.CoG.x;
            if (tri.CoG.z < Min.z) Min.z = tri.CoG.z;
            if (tri.CoG.x > Max.x) Max.x = tri.CoG.x;
            if (tri.CoG.z > Max.z) Max.z = tri.CoG.z;
            
            switch (tri.WaterStatus)
            {
            case 3:
                tri.Depth = -(mvVertWaterHeight[tri.I[0]] + mvVertWaterHeight[tri.I[1]] + mvVertWaterHeight[tri.I[2]]) / 3.0f;
                break;
            case 2:
                if (tri.bUnder[0] == 0)
                    tri.Depth = -(mvVertWaterHeight[tri.I[1]] + mvVertWaterHeight[tri.I[2]]) / 2.0f;
                else if (tri.bUnder[1] == 0)
                    tri.Depth = -(mvVertWaterHeight[tri.I[0]] + mvVertWaterHeight[tri.I[2]]) / 2.0f;
                else
                    tri.Depth = -(mvVertWaterHeight[tri.I[0]] + mvVertWaterHeight[tri.I[1]]) / 2.0f;
                break;
            case 1:
                if (tri.bUnder[0] == 1)
                    tri.Depth = -mvVertWaterHeight[tri.I[0]];
                else if (tri.bUnder[1] == 1)
                    tri.Depth = -mvVertWaterHeight[tri.I[1]];
                else
                    tri.Depth = -mvVertWaterHeight[tri.I[2]];
                break;
            }
            intensity = (float)tri.WaterStatus / 3.0f;
            tri.fPressure = intensity * mWATER_DENSITY * mGRAVITY * tri.Depth * tri.Area;
            tri.vPressure = tri.Normal * tri.fPressure;

            Archimede.Vector += tri.vPressure;
            Archimede.Position += tri.CoG * tri.fPressure;  // Approximation because the 3 points do not have the same hydrostatic pressure (they are not at the same height)
            tmpSumPressure += tri.fPressure;

            AreaWetted += intensity * tri.Area;
        }
    }
    
    Archimede.Magnitude = std::max(Archimede.Vector.y, 0.0f);
    Archimede.Vector = { 0.0f, Archimede.Magnitude, 0.0f };     // Always vertical
    if (tmpSumPressure > 0) Archimede.Position /= tmpSumPressure;
    else                    Archimede.Position = vec3(0.0f);
    
    LWL = std::max(fabs(Max.x - Min.x), fabs(Max.z - Min.z));
}
void Ship::ComputeGravity()
{
    Gravity.Magnitude = mMass * mGRAVITY;
    Gravity.Vector = vec3(0.0f, -Gravity.Magnitude, 0.0f);  // Always vertical
    Gravity.Position = TransformPosition(ship.PosGravity);
}
void Ship::ComputeHeave(float dt)
{
    // Force which resist to the couple Archimede / Gravity
    ResistanceHeave.Magnitude = mMass * HeaveVelocity * AreaXZ_RacCub * AreaWetted / AreaWettedMax;
    if (Archimede.Magnitude > Gravity.Magnitude)    ResistanceHeave.Vector = TransformVector(vec3(0.0f, -ResistanceHeave.Magnitude, 0.0f));
    else                                            ResistanceHeave.Vector = TransformVector(vec3(0.0f, ResistanceHeave.Magnitude, 0.0f));
    ResistanceHeave.Position = Archimede.Position;
}
void Ship::ComputeThrust(float dt)
{
    // Force as a result of the engine and the propellers

    float limitRpm = 0.0f;
    if (PowerCurrentStep >= 0)  limitRpm =  ship.PowerRpmMin + ((float)PowerCurrentStep / (float)ship.PowerStepMax) * ((float)ship.PowerRpmMax - (float)ship.PowerRpmMin);
    else                        limitRpm = -ship.PowerRpmMin + ((float)PowerCurrentStep / (float)ship.PowerStepMax) * ((float)ship.PowerRpmMax - (float)ship.PowerRpmMin);

    if (PowerRpm < limitRpm - ship.PowerRpmIncrement * dt)
    {
        PowerRpm += ship.PowerRpmIncrement * dt;
        if (PowerRpm > -ship.PowerRpmMin && PowerRpm < ship.PowerRpmMin)
            PowerRpm = ship.PowerRpmMin;
    }
    else if (PowerRpm > limitRpm + ship.PowerRpmIncrement * dt)
    {
        PowerRpm -= ship.PowerRpmIncrement * dt;
        if (PowerRpm < ship.PowerRpmMin && PowerRpm > -ship.PowerRpmMin)
            PowerRpm = -ship.PowerRpmMin;
    }

    if (PowerRpm >= 0)  PowerApplied = mPowerW * (PowerRpm - ship.PowerRpmMin) / (ship.PowerRpmMax - ship.PowerRpmMin);
    else                PowerApplied = mPowerW * (PowerRpm + ship.PowerRpmMin) / (ship.PowerRpmMax - ship.PowerRpmMin);

    // Stop to 0 if close to zero and target is zero
    if (PowerRpm > -ship.PowerRpmIncrement * dt && PowerRpm < ship.PowerRpmIncrement * dt)
    {
        PowerRpm = 0.0f;
        PowerApplied = 0.0f;
    }

    Thrust.Magnitude = PowerApplied * ship.PowerPerf;
    Thrust.Vector = TransformVector(vec3(Thrust.Magnitude, 0.0f, 0.0f));
    Thrust.Position = TransformPosition(ship.PosPower);

    mSoundPower->setPitch(1.0f + 0.25f * fabs(PowerApplied) / mPowerW);
    if (g_Camera.GetPosition().y < 0.0f)
        mSoundPower->setVolume(0.01f + 0.25f * fabs(PowerApplied) / mPowerW);
    else
        mSoundPower->setVolume(0.25f + 0.25f * fabs(PowerApplied) / mPowerW);
}
void Ship::ComputeResistanceViscous(float dt) 
{
    // Reynolds number
    float Re = (fabs(SurgeVelocity) * LWL) / mKINEMATIC_VISCOSITY;

    // Viscous resistance coefficient
    float Cv = 0.075f / pow(log10(Re) - 2.0f, 2);

    // Viscous resistance (in Newtons)
    float resVisc = 0.5f * mWATER_DENSITY * AreaWetted * pow(fabs(SurgeVelocity), 2) * Cv;
    
    if (isnan(resVisc))
        resVisc = 0.0f;

    if (SurgeVelocity < 0.0f)
        resVisc *= 3.0f;

    ResistanceViscous.Magnitude = -Sign(SurgeVelocity) * resVisc;   // Against the speed
    ResistanceViscous.Vector = TransformVector(vec3(ResistanceViscous.Magnitude, 0.0f, 0.0f));
    ResistanceViscous.Position = Archimede.Position;
}
void Ship::ComputeResistanceWaves(float dt)
{
    // Froude number
    float Fn = fabs(SurgeVelocity) / sqrt(mGRAVITY * LWL);

	// Wave resistance coefficient (Cw) using a Gaussian distribution
	const float Fr0 = 0.45f;        // Peak centre (estimation, can be adjusted based on the ship type)
	const float Sigma = 0.085f;     // Peak width (estimation, can be adjusted based on the ship type)
    const float Cw0 = 0.005f;       // Maximal value of Cw
    float exponent = -pow(Fn - Fr0, 2) / (2.0f * pow(Sigma, 2));
    float Cw = Cw0 * exp(exponent);

    // Wave resistance (in Newtons)
    float resWav = 0.5f * mWATER_DENSITY * AreaWetted * pow(fabs(SurgeVelocity), 2) * Cw;
    if (isnan(resWav))
        resWav = 0.0f;

    ResistanceWaves.Magnitude = -Sign(SurgeVelocity) * resWav;   // Against the speed
    ResistanceWaves.Vector = TransformVector(vec3(ResistanceWaves.Magnitude, 0.0f, 0.0f));
    ResistanceWaves.Position = Archimede.Position;
}
void Ship::ComputeResistanceResidual(float dt)
{
    // Force at the start of a movement
    if(SurgeVelocity > 0.0001f || SurgeVelocity < 0.0001f)  ResistanceResidual.Magnitude = -Sign(SurgeVelocity) * (mMass / 150.f);   // Against the speed
    else                                                    ResistanceResidual.Magnitude = 0.0f; // No speed, no resistance
    ResistanceResidual.Vector = TransformVector(vec3(ResistanceResidual.Magnitude, 0.0f, 0.0f));
    ResistanceResidual.Position = Archimede.Position;
}
void Ship::ComputeBowThrust(float dt)
{
    // Force as a result of the bow thruster

    float limitRpm = 0.0f;
    if (BowThrusterCurrentStep >= 0)
        limitRpm = ship.BowThrusterRpmMin + ((float)BowThrusterCurrentStep / (float)ship.BowThrusterStepMax) * ((float)ship.BowThrusterRpmMax - (float)ship.BowThrusterRpmMin);
    else
        limitRpm = -ship.BowThrusterRpmMin + ((float)BowThrusterCurrentStep / (float)ship.BowThrusterStepMax) * ((float)ship.BowThrusterRpmMax - (float)ship.BowThrusterRpmMin);

    if (BowThrusterRpm < limitRpm - ship.BowThrusterRpmIncrement * dt)      BowThrusterRpm += ship.BowThrusterRpmIncrement * dt;
    else if (BowThrusterRpm > limitRpm + ship.BowThrusterRpmIncrement * dt) BowThrusterRpm -= ship.BowThrusterRpmIncrement * dt;

    if (BowThrusterRpm >= 0)    BowThrusterApplied = ship.BowThrusterPowerW * (BowThrusterRpm - ship.BowThrusterRpmMin) / (ship.BowThrusterRpmMax - ship.BowThrusterRpmMin);
    else                        BowThrusterApplied = ship.BowThrusterPowerW * (BowThrusterRpm + ship.BowThrusterRpmMin) / (ship.BowThrusterRpmMax - ship.BowThrusterRpmMin);

    // Stop to 0 if close to zero and target is zero
    if (BowThrusterRpm > -ship.BowThrusterRpmIncrement * dt && BowThrusterRpm < ship.BowThrusterRpmIncrement * dt)
        BowThrusterRpm = 0.0f;

    BowThrust.Magnitude = BowThrusterApplied * ship.BowThrusterPerf;
    BowThrust.Vector = TransformVector(vec3(0.0f, 0.0f, BowThrust.Magnitude));
    BowThrust.Position = TransformPosition(ship.PosBowThruster);

    mSoundBowThruster->setPitch(1.0f + 0.5f * fabs(BowThrusterApplied) / ship.BowThrusterPowerW);
    if (g_Camera.GetPosition().y < 0.0f)
        mSoundBowThruster->setVolume(0.01f + 0.25f * fabs(BowThrusterApplied) / ship.BowThrusterPowerW);
    else
        mSoundBowThruster->setVolume(0.25f + 0.25f * fabs(BowThrusterApplied) / ship.BowThrusterPowerW);
}
void Ship::ComputeRudder(float dt)
{
    // Rudder movement
    float maxAngleDeg = RudderCurrentStep * ship.RudderIncrement;
    // Ensure maxAngleDeg does not exceed ship.RudderStepMax
    if (fabs(maxAngleDeg) > ship.RudderStepMax)     maxAngleDeg = ship.RudderStepMax * Sign(RudderCurrentStep);

    float delta = ship.RudderRotSpeed * dt;

    // Move rudder toward target
    if (RudderAngleDeg < maxAngleDeg - delta)       RudderAngleDeg += delta;
    else if (RudderAngleDeg > maxAngleDeg + delta)  RudderAngleDeg -= delta;
    else                                            RudderAngleDeg = maxAngleDeg;

    // Limit movement to max angle (safety)
    if (fabs(RudderAngleDeg) > ship.RudderStepMax)  RudderAngleDeg = ship.RudderStepMax * Sign(RudderAngleDeg);

    // Stop to 0 if close to zero and target is zero
    if (maxAngleDeg == 0.0f && fabs(RudderAngleDeg) < delta) RudderAngleDeg = 0.0f;

    // Convert rudder angle to radians
    float angle = glm::radians(RudderAngleDeg);

    // Rudder force
    float velocity = 2.0f + fabs(SurgeVelocity);
    float forceLift = 0.5f * mWATER_DENSITY * pow(velocity, 2) * mRudderArea * sin(angle) * 3.0f;

    // Drag force
    float forceDrag = fabs(0.5f * mWATER_DENSITY * pow(SurgeVelocity, 2) * mRudderArea * sin(angle));

    // Lateral component (gyration of the ship)
    RudderLift.Magnitude = Sign(SurgeVelocity) * forceLift * ship.RudderLiftPerf;
    RudderLift.Vector = TransformVector(vec3(0.0f, 0.0f, RudderLift.Magnitude));
    RudderLift.Position = TransformPosition(ship.PosRudder);

    // Axial component (slow the ship)
    RudderDrag.Magnitude = -Sign(SurgeVelocity) * forceDrag * ship.RudderDragPerf;
    RudderDrag.Vector = TransformVector(vec3(RudderDrag.Magnitude, 0.0f, 0.0f));
    RudderDrag.Position = TransformPosition(ship.PosRudder);
}
void Ship::ComputeWind(float dt)
{
    // 2 forces, one on the forward half of the ship and the other on the aft half of the ship, 
    // both exerting a thrust that causes the ship to drift and a turning force that brings the ship across to windward

    float A = mLength * mAirDraft * 0.5f;

    // Angle between Wind and Heading (on horizontal plan XZ) (-0 to -180 = port wind, 0 to 180 = starboard wind)
    vec3 heading = TransformPosition(mBow) - ship.Position;
    vec2 windXZ(g_Wind.x, g_Wind.y);
    vec2 headingXZ(heading.x, heading.z);
    windXZ = glm::normalize(windXZ);
    headingXZ = glm::normalize(headingXZ);
    float angleWind = atan2(windXZ.y, windXZ.x);
    float angleHeading = atan2(headingXZ.y, headingXZ.x);
    float angle = angleWind - angleHeading;
    angle = atan2(sin(angle), cos(angle));
  
    // Between 0° and 90°: alpha is positive (from 0 to 1 then goes back down to 0), between 90° and 180°: alpha is negative (from 0 to -1 then goes back up to 0)
    float alpha = sin(2 * angle); 
    float windSpeedMS = glm::length(vec2(g_Wind.x - vCOG.x, g_Wind.y - vCOG.y));

    // WIND ROTATION
    WindRotation.Magnitude = 0.5f * mAIR_DENSITY * mPLATE_DRAG_COEFF * A * pow(windSpeedMS, 2) * alpha;
    WindRotation.Vector = TransformVector(vec3(0.0f, 0.0f, WindRotation.Magnitude));
    WindRotation.Position = TransformPosition(vec3(mLength * 0.5f, 0.0f, 0.0f));

    // Emerging surface exposed to the wind
    const float lateralArea = mLength * mAirDraft * 0.5f;
    const float frontalArea = mWidth * mAirDraft * 0.5f;
    // Angle decomposition
    const float cosPhi = std::cos(angle);
    const float sinPhi = std::sin(angle);
    const float absCosPhi = std::abs(cosPhi);
    const float absSinPhi = std::abs(sinPhi);
    // Directional factors
    const float frontFactor = (cosPhi >= 0.0f) ? 1.0f : 0.0f;
    const float rearFactor = (cosPhi <= 0.0f) ? 1.0f : 0.0f;
    // Projected areas
    float frontProjectedArea = (0.5f * lateralArea * absSinPhi) + (frontalArea * absCosPhi * frontFactor);
    float rearProjectedArea = (0.5f * lateralArea * absSinPhi) + (frontalArea * absCosPhi * rearFactor);

    // FRONT DRIFT
    WindFront.Magnitude = 0.5f * mAIR_DENSITY * mPLATE_DRAG_COEFF * frontProjectedArea * windSpeedMS * windSpeedMS * 0.5f;
    WindFront.Vector = -WindFront.Magnitude * glm::normalize(vec3(g_Wind.x, 0.0f, g_Wind.y));
    WindFront.Position = TransformPosition(vec3(mLength / 4.0f, mAirDraft * 0.25f, 0.0f));

    // REAR DRIFT
    WindRear.Magnitude = 0.5f * mAIR_DENSITY * mPLATE_DRAG_COEFF * rearProjectedArea * windSpeedMS * windSpeedMS * 0.5f;
    WindRear.Vector = -WindRear.Magnitude * glm::normalize(vec3(g_Wind.x, 0.0f, g_Wind.y));
    WindRear.Position = TransformPosition(vec3(-mLength / 4.0f, mAirDraft * 0.25f, 0.0f));
}
void Ship::ComputeCentrifugal(float dt)
{
    // Centrifugal force during a turn that causes the ship to roll outward

    float forceMagnitude = mMass * YawRate * ship.CentrifugalPerf;

    Centrifugal.Magnitude = forceMagnitude;
    Centrifugal.Vector = TransformVector(vec3(0.0f, 0.0f, forceMagnitude));
    Centrifugal.Position = TransformPosition(ship.PosGravity);
}
void Ship::ComputeForces(float dt)
{
    if (!bMotion)
        return;

    if (dt == 0.0f)
        return;

    // Previous position of the ship
    vec2 prevPosition = { ship.Position.x, ship.Position.z };

	// Previous position of the bow
    vec3 PrevPosBow = TransformPosition(mBow);
   
    // Previous position of the stern
    vec3 PrevPosStern = TransformPosition(mStern);
    
    // Heave (Archimede - Gravity - ResistanceHeave)
    float HeaveForce = Archimede.Magnitude - Gravity.Magnitude;
    if (Archimede.Magnitude != 0.0f)
        HeaveForce -= ResistanceHeave.Magnitude;
    HeaveAcceleration = HeaveForce / mMass;
    HeaveVelocity += HeaveAcceleration * dt;
    ship.Position.y += HeaveVelocity * dt;

    // Rotations / Inerties
    float dx = Archimede.Position.x - Gravity.Position.x;
    float dz = Archimede.Position.z - Gravity.Position.z;
    PitchCouple = dx * cos(Yaw) - dz * sin(Yaw);
    RollCouple = dx * sin(Yaw) + dz * cos(Yaw);
    float force = (Archimede.Magnitude + Gravity.Magnitude) * 0.5f;

    float damping = 1.0f;

    // CoupleZ (tangage) (AG.x positive = the bow lowers)
    PitchAcceleration = 0.5f * PitchCouple * force / Izz;
    PitchAcceleration -= PitchVelocity * damping;
    PitchVelocity += PitchAcceleration * dt;
    Pitch += PitchVelocity * dt;

    // CoupleX (roulis) (AG.z positive = list to starboard)
    RollAcceleration = 0.25f * -RollCouple * force / Ixx;
    if (SurgeVelocity != 0.0f)
        RollAcceleration += Sign(VariationYawSigned) * Centrifugal.Magnitude / Ixx;
    RollAcceleration -= RollVelocity * damping;
    RollVelocity += RollAcceleration * dt;
    Roll += RollVelocity * dt;

    // Normalization of Pitch and Roll Angles
    Pitch = fmod(Pitch, 2.0f * M_PI);
    Roll = fmod(Roll, 2.0f * M_PI);

    // Propulsive force
    float forceFWD = Thrust.Magnitude
        + ResistanceViscous.Magnitude
        + ResistanceWaves.Magnitude
        + ResistanceResidual.Magnitude
        + RudderDrag.Magnitude;

    // Mass of the ship is multiplied by 1.5 to simulate inertia (added displaced water)
	SurgeAcceleration = forceFWD / (mMass * 1.5f);  
    
    // Turning drag
    float turningDrag = ship.TurningDragPerf * pow(SurgeVelocity, 2) * fabs(YawVelocity);
    SurgeAcceleration -= turningDrag / mMass;

    // Slam and Surge
    if (AreaWetted > 0.0f)
    {
        // Slam decreases the speed
        //if (SurgeAcceleration > 0.0f && PitchAcceleration > 0.0f)
        //    SurgeAcceleration -= PitchAcceleration;

        // Surge: Positive pitch decreases the speed, negative pitch increases the speed
        SurgeAcceleration -= 0.1f * (AreaWetted / AreaWettedMax) * mGRAVITY * sin(Pitch);
    }

    // Forces of rotation
    YawAcceleration = 0.0f;
    YawAcceleration -= BowThrust.Magnitude * fabs(mStern.x) / Iyy;
    YawAcceleration += RudderLift.Magnitude * (fabs(ship.PosRudder.x)) / Iyy;
    YawAcceleration += WindRotation.Magnitude * (mLength * 0.5f) / Iyy;

    // Heading variation according to roll
    float roll_to_yaw_coeff = 0.2f;
    YawAcceleration += -roll_to_yaw_coeff * Roll;

    // Damping of yaw
    YawAcceleration -= YawVelocity * damping;
    YawVelocity += YawAcceleration * dt;
    Yaw += YawVelocity * dt;

    // New position
    mat4 rotationMatrix = glm::rotate(mat4(1.0f), Yaw, vec3(0.0f, 1.0f, 0.0f));
    vec3 xA(1.0f, 0.0f, 0.0f);
    vec3 forward = vec3(rotationMatrix * vec4(xA, 0.0f));
    SurgeVelocity += SurgeAcceleration * dt;
    ship.Position += forward * SurgeVelocity * dt;

    // Displacements (Bow thrust & Rudder lift)
    float dLat = 0.0f;
    if (BowThrust.Magnitude != 0.0f)
        dLat += mStern.x * sin(YawVelocity * dt); 
    if (RudderLift.Magnitude != 0.0f)
        dLat += -ship.PosRudder.x * sin(YawVelocity * dt) * 0.5f;

    vec3 zA(0.0f, 0.0f, 1.0f);
    vec3 perpend = vec3(rotationMatrix * vec4(zA, 0.0f));
    ship.Position += perpend * dLat;

    // Wind drift
    float forceDrift = 0.01f * (WindFront.Magnitude + WindRear.Magnitude);
    WindAcceleration = forceDrift / (0.5f * mLength * mLength) - 0.1 * WindVelocity;
    WindVelocity += WindAcceleration * dt;
    ship.Position += glm::normalize(-vec3(g_Wind.x, 0.0f, g_Wind.y)) * WindVelocity * dt;

    /////////////////////////

    // Velocities
    vec2 deltaPos = vec2(ship.Position.x, ship.Position.z) - prevPosition;
    LinearVelocity = dot(normalize(vec2(forward.x, forward.z)), deltaPos) / dt;
    DriftVelocity = dot(normalize(vec2(perpend.x, perpend.z)), deltaPos) / dt;
    vCOG = deltaPos;
    Velocity = glm::length(vCOG) / dt;

    // HDG, COG, SOG
    GetHDG();

    vec2 dPos = vec2(ship.Position.x, ship.Position.z) - prevPosition;
    if (dt != 0.0f)
        SOG = MsToKnots(glm::length(dPos) / dt);

    if (glm::length(dPos) == 0.0f)  COG = HDG;
    else                            COG = glm::degrees(std::atan2(dPos.y, dPos.x)) + 90;
    while (COG < 0.0f)      COG += 360.0f;
    while (COG > 360.0f)    COG -= 360.0f;

	// SOG at the bow and the stern
    mat4 world = mat4(1.0f);
    world = glm::translate(world, ship.Position);
    world = glm::rotate(world, Yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    world = glm::rotate(world, Roll, glm::vec3(1.0f, 0.0f, 0.0f));
    world = glm::rotate(world, Pitch, glm::vec3(0.0f, 0.0f, 1.0f));

    vec3 PosBow = mBow;
    PosBow = vec3(world * vec4(PosBow, 1.0f));
    float d = (PosBow.x - PrevPosBow.x) * sin(Yaw) + (PosBow.z - PrevPosBow.z) * cos(Yaw);
    if (dt != 0.0f)
        SOGbow = MsToKnots(d / dt);

    vec3 PosStern = mStern;
    PosStern = vec3(world * vec4(PosStern, 1.0f));
    d = (PosStern.x - PrevPosStern.x) * sin(Yaw) + (PosStern.z - PrevPosStern.z) * cos(Yaw);
    if (dt != 0.0f)
        SOGstern = MsToKnots(d / dt);
    
    // COG - SOG force drawn
    COGSOG.Position = TransformPosition(ship.PosPower);
    COGSOG.Vector = 1e6f * vec3(vCOG.x, 0.0f, vCOG.y);
}
void Ship::ComputeAutopilot(float dt)
{
    static float integral = 0.0f;
    static float lastError = 0.0f;

    static bool bPrevAutopilot;
    if (!bPrevAutopilot && bAutopilot)
    {
        // Reset
        integral = 0.0f;
        lastError = 0.0f;
    }
    bPrevAutopilot = bAutopilot;

    if (!bAutopilot)
        return;

    /*
    This implementation uses a PID (Proportional-Integral-Derivative) controller to calculate the optimal rudder angle.
    The function first calculates the heading error, taking into account that heading angles are cyclical (0-360°).
    The proportional (P) component responds directly to the current error.
    The integral (I) component accumulates the error over time, helping to correct persistent errors.
    The derivative (D) component anticipates future changes based on the rate of change of the error.
    The final rudder angle is the sum of these three components, limited to the maximum allowable angle.

    Rudder Gain – (P) This option allows you to change the amount of rudder angle used to resume steering and the speed at which it is applied.
    Increasing the gain allows the autopilot to respond quickly and aggressively to any input.
    This is the primary function used to ensure the autopilot responds appropriately to current conditions.
    "It's not unusual for people to think they need a high gain in all cases, but the reality is not that simple.
    If you compare it to steering a car, if you're going fast, you may want a smaller angle gently applied to the wheels to make a change of direction than when you're going slowly. Your gain preferences vary."

    Auto Trim – (I) This feature learns the amount of trim required to achieve a stable heading.
    Changing this setting adjusts how quickly the autopilot learns to cope with trim.
    "To evaluate and configure Auto Trim, turn off the autopilot, bring the boat into a crosswind, and sail manually until you are satisfied with the helm feel.
    Turn on the autopilot and wait to see if the heading remains the same. Then introduce some trim by sheeting in the mainsail.
    If the autopilot isn't trimming quickly enough, you should reduce Auto Trim to allow it to learn more quickly."

    Counter Rudder (D) – Counter rudder is the function that brings the rudder back in the opposite direction to prevent the boat from overshooting the required heading.
    "To check this function, set up the boat under power and note the angle you started at and the angle you want the driver to steer the boat at.
    Then, make large course changes, say 20-30 degrees at a time. This will help you see if the driver is under- or over-compensating.
    If they are over-compensating, increase the counter rudder. If they are under-compensating, decrease the counter rudder."    */

    /* Parameters
    ship.BaseP              Proportional
    ship.BaseI              Integral
    ship.BaseD              Derivative

    ship.MaxIntegral        Limit of the integral to avoid runaway
    ship.SpeedFactor        Factor to adjust the influence of speed
    ship.MinSpeed           Minimum speed to avoid division by zero
    ship.LowSpeedBoost      Low speed amplification factor
    ship.SeaSateFactor      Increases responsiveness in difficult conditions (Pitch and Roll)
    */

    // Calculate the heading error (the shortest difference between headings)
    float error = std::fmod(HDGInstruction - HDG + 540.0f, 360.0f) - 180.0f;

    // Reverse the error to correct in the right direction
    error = -error;

    auto adjustGain = [&](float baseGain, float shipSpeed) {
        float speedFactor = 1.0f / (1.0f + shipSpeed / ship.SpeedFactor);
        float seaStateFactor = 1.0f + (std::abs(Pitch) + std::abs(Roll)) * ship.SeaSateFactor;
        return baseGain * speedFactor * seaStateFactor;
        };

    // Gains
    float P_GAIN = ship.BaseP;           // Proportional gain
    float I_GAIN = ship.BaseI;           // Integral gain
    float D_GAIN = ship.BaseD;           // Derivative gain

    // Dynamically adjust gains
    if (bDynamicAdjustment)
    {
        P_GAIN = adjustGain(ship.BaseP, SurgeVelocity);
        I_GAIN = adjustGain(ship.BaseI, SurgeVelocity);
        D_GAIN = adjustGain(ship.BaseD, SurgeVelocity);
    }

    // Proportional component
    float p = P_GAIN * error;

    // Integral component
    integral += error * dt;
    integral = std::clamp(integral, -ship.MaxIntegral, ship.MaxIntegral);
    float i = I_GAIN * integral;

    // Derivative component
    float derivative = (error - lastError) * dt;
    float d = D_GAIN * derivative;

    // Calculate the total rudder angle
    float rudderAngle = p + i + d;

    // Adjust the rudder angle according to the speed
    float adjustedSpeed = std::max(SurgeVelocity, ship.MinSpeed);

    float speedFactor = (ship.SpeedFactor / adjustedSpeed) * ship.LowSpeedBoost;
    speedFactor = std::min(speedFactor, ship.LowSpeedBoost); // Limit the maximum factor

    rudderAngle *= speedFactor;

    // Limit the rudder angle to the maximum allowed
    rudderAngle = std::clamp(rudderAngle, -(float)ship.RudderStepMax, (float)ship.RudderStepMax);
    RudderCurrentStep = rudderAngle;

    // Update last error for next calculation
    lastError = error;
}

// Compute effects
void Ship::UpdateSounds()
{   
    mSoundPower->setPosition(TransformPosition(ship.PosPower));
    mSoundBowThruster->setPosition(TransformPosition(ship.PosBowThruster));
    if (!mbSoundBowThrusterPlaying)
    {
        if (BowThrusterRpm != ship.BowThrusterRpmMin)
        {
            alGetError(); // clear error state
            mSoundBowThruster->play();
            ALenum error;
            if ((error = alGetError()) != AL_NO_ERROR)
                cout << "alSourcef 0 AL_PITCH : " << error << endl;
            mbSoundBowThrusterPlaying = true;
        }
    }
    else
    {
        if (BowThrusterRpm == ship.BowThrusterRpmMin)
        {
            mSoundBowThruster->pause();
            mbSoundBowThrusterPlaying = false;
        }
    }

    static bool bPause = false;
    if (!bVisible || !bSound)
    {
        mSoundPower->pause();
        mSoundBowThruster->pause();
        bPause = true;
    }
    if (bVisible && bSound && bPause)
    {
        mSoundPower->play();
        if (BowThrusterRpm != ship.BowThrusterRpmMin)
            mSoundBowThruster->play();
        bPause = false;
    }
}
void Ship::UpdateSmoke(float dt)
{
    vec3 p = TransformPosition(ship.Chimney1);
    vec3 direction = 0.1f * vec3(-g_Wind.x, 0.1f * g_WindSpeedKN, -g_Wind.y);
    float windSpeed = KnotsToMS(g_WindSpeedKN);
    // 1st chimney  
    mSmokeLeft->Emit(p, direction);
    mSmokeLeft->Update(dt, direction, windSpeed);
    mSmokeLeft->UpdateParticleBuffers();
    // 2nd chimney
    if (ship.nChimney == 2)
    {
        p = TransformPosition(ship.Chimney2);
        mSmokeRight->Emit(p, direction);
        mSmokeRight->Update(dt, direction, windSpeed);
        mSmokeRight->UpdateParticleBuffers();
    }
}

// Wake by buffer
void Ship::UpdateWakeBuffer()
{
    // Calcul du déplacement en mètres
    vec2 posDelta = vec2(ship.Position.x, ship.Position.z) - mPreviousShipPosition;
    mPreviousShipPosition = vec2(ship.Position.x, ship.Position.z);

    // Bind FBO ping-pong cible
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_BUFFER[mCurrentIdx]);
    glViewport(0, 0, TexWakeBufferSize, TexWakeBufferSize);
    glClear(GL_COLOR_BUFFER_BIT);

    mShaderBuffer->use();   // Ship/wake_buffer.vert, Ship/wake_buffer.frag

    // Passage des textures + uniforms
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexBuffer[1 - mCurrentIdx]);
    mShaderBuffer->setInt("texPrevAccum", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, TexContourShip);
    mShaderBuffer->setInt("texWake", 1);

    mShaderBuffer->setVec2("texWakeSize", vec2(float(TexContourShipW) / (float)TexWakeBufferSize, float(TexContourShipH) / (float)TexWakeBufferSize));
    mShaderBuffer->setFloat("rotationShip", -Yaw);
    mShaderBuffer->setVec2("shipPos", ship.Position);
    mShaderBuffer->setVec2("posDelta", posDelta);
    static float prevYaw = Yaw;
    float rotDelta = Yaw - prevYaw;
    mShaderBuffer->setFloat("rotDelta", rotDelta);
    prevYaw = Yaw;
    mShaderBuffer->setVec2("worldMin", vec2(-256.0f, -256.0f));         // coin bas-gauche du plan monde couvert par la texture accumulée
    mShaderBuffer->setVec2("worldMax", vec2(256.0f, 256.0f));           // coin haut-droit

    //shaderAccum->setFloat("fadeSpeed", 0.001f);

    mScreenQuadWakeBuffer->Render();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    //static double startTime = glfwGetTime();
    //static bool bDone = false;
    //if (!bDone && (glfwGetTime() - startTime > 10.0))
    //{
    //    SaveTexture2D(texAccum[currentIdx], TexWakeBufferSize, TexWakeBufferSize, 1, GL_RED, "Outputs/texAccum0.png");
    //    bDone = true;
    //}

    // Swap ping-pong textures
    TexWakeBuffer = mTexBuffer[mCurrentIdx];
    mCurrentIdx = 1 - mCurrentIdx;
}

// Wake by vao
void Ship::CreateWakeVao()
{
    glGenVertexArrays(1, &mVaoWake);

    glGenBuffers(1, &mVboWake);
    glBindVertexArray(mVaoWake);
    glBindBuffer(GL_ARRAY_BUFFER, mVboWake);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    // position (3 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(sFoamVertex), (void*)offsetof(sFoamVertex, pos));
    // uv (2 floats)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(sFoamVertex), (void*)offsetof(sFoamVertex, uv));
    // alpha (1 float)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(sFoamVertex), (void*)offsetof(sFoamVertex, alpha));

    glBindVertexArray(0);
}
float calcAlpha(float pointTime, float now)
{
    const float start = 4.0f;
    const float end = 30.0f;

    float elapsed = now - pointTime;
    if (elapsed <= start)
        return 1.0f;
    if (elapsed >= end)
        return 0.0f;

    // logarithmic interpolation inverse: alpha=1 at start, alpha=0 at end
    float t = (elapsed - start) / (end - start); // t in [0,1]
    // small epsilon to avoid log(0)
    float eps = 1e-5f;
    t = glm::clamp(t, 0.0f, 1.0f - eps);
    // Compute
    float logAlpha = 1.0f - log(t + 1.0f) / log(2.0f);
    return glm::clamp(logAlpha, 0.0f, 1.0f);
}
void Ship::UpdateWakeVao()
{
    // Add point to wake every 100 frames
    static int compteurSillage = 0;
    compteurSillage++;
    if (compteurSillage % 100 == 0)
    {
        sFoamPts sfp;
        sfp.pos = TransformPosition(mWakePivot);
        sfp.time = glfwGetTime();
        vWakePoints.push_back(sfp);
        // Cleaning to not exceed a limit
        if (vWakePoints.size() > 100) vWakePoints.erase(vWakePoints.begin());
        compteurSillage = 0;
    }
    
    // Temporarily adds the current position
    sFoamPts sfp;
    sfp.pos = TransformPosition(mWakePivot);
    sfp.time = glfwGetTime();
    vWakePoints.push_back(sfp);

    size_t n = vWakePoints.size();
    if (n < 2) return;

    vector<vec3> sideLeft(n);
    vector<vec3> sideRight(n);

    float widthHalf = (mWidth * ship.WakeWidth) * 0.5f;

    // Calculation of the “joined” side ends
    for (size_t i = 0; i < n; ++i) 
    {
        vec3 p = vWakePoints[i].pos;

        // Forward (next), backward (prev)
        vec2 dirPrev, dirNext;

        if (i == 0)     dirPrev = glm::normalize(vec2(vWakePoints[i + 1].pos.x - p.x, vWakePoints[i + 1].pos.z - p.z));
        else            dirPrev = glm::normalize(vec2(p.x - vWakePoints[i - 1].pos.x, p.z - vWakePoints[i - 1].pos.z));
        if (i == n - 1) dirNext = glm::normalize(vec2(p.x - vWakePoints[i - 1].pos.x, p.z - vWakePoints[i - 1].pos.z));
        else            dirNext = glm::normalize(vec2(vWakePoints[i + 1].pos.x - p.x, vWakePoints[i + 1].pos.z - p.z));

        // Normals to the left of each segment
        vec2 nPrev(-dirPrev.y, dirPrev.x);
        vec2 nNext(-dirNext.y, dirNext.x);

        // Standard bisector (except in the case of very tight turns)
        vec2 bisec = glm::normalize(nPrev + nNext);
        float bisecLen = glm::length(nPrev + nNext);
        if (bisecLen < 1e-4f) // super acute angle, we take one of the normals
            bisec = nPrev;

        // Corrects the "big miter" if the turn is very tight (prevents crazy points)
        float dotDir = glm::dot(dirPrev, dirNext);

        // Calculates the distance to neighbors to adjust the width
        float dist = 0.0f;
        if (i + 1 < n) 
            dist = glm::distance(vec2(p.x, p.z), vec2(vWakePoints[i + 1].pos.x, vWakePoints[i + 1].pos.z));
        else if (i > 0) 
            dist = glm::distance(vec2(p.x, p.z), vec2(vWakePoints[i - 1].pos.x, vWakePoints[i - 1].pos.z));

        // Threshold for “too close” points
        float adjustedWidthHalf = widthHalf;
        float threshold = 0.25f;
        if (dist < threshold)
            adjustedWidthHalf = 0.05f; // Minimum width

        // Use adjustedWidthHalf for the following
        float miterLen = adjustedWidthHalf / glm::max(glm::dot(bisec, nPrev), 0.2f); // clamp min

        vec3 left = p + vec3(bisec.x, 0.0f, bisec.y) * miterLen;
        vec3 right = p - vec3(bisec.x, 0.0f, bisec.y) * miterLen;

        sideLeft[i] = left;
        sideRight[i] = right;
    }

    // Generation of triangles (2 per segment)
    vWakeVertices.clear();
    float uv_v = 0.0f, dv = 1.0f / n;

    float now = glfwGetTime();
    for (size_t i = 0; i + 1 < n; ++i)
    {
        float v0 = uv_v, v1 = uv_v + dv;

        float alpha0 = calcAlpha(vWakePoints[i].time, now);
        float alpha1 = calcAlpha(vWakePoints[i + 1].time, now);
      
        // Triangle 1
        vWakeVertices.push_back({ sideLeft[i],  {0, v0}, alpha0 });
        vWakeVertices.push_back({ sideRight[i], {1, v0}, alpha0 });
        vWakeVertices.push_back({ sideLeft[i + 1],  {0, v1}, alpha1 });
        // Triangle 2
        vWakeVertices.push_back({ sideLeft[i + 1],  {0, v1}, alpha1 });
        vWakeVertices.push_back({ sideRight[i], {1, v0}, alpha0 });
        vWakeVertices.push_back({ sideRight[i + 1], {1, v1}, alpha1 });

        uv_v += dv;
    }

    // Remove the temporary stitch after use
    vWakePoints.pop_back();

    // Update the vbo
    glBindBuffer(GL_ARRAY_BUFFER, mVboWake);
    glBufferData(GL_ARRAY_BUFFER, vWakeVertices.size() * sizeof(sFoamVertex), vWakeVertices.data(), GL_DYNAMIC_DRAW);
}
void Ship::UpdateTextureWakeVao()
{
    if (vWakeVertices.size() == 0)
        return;
    
    // Multisample
    glBindFramebuffer(GL_FRAMEBUFFER, msFBO_WAKE);
    glViewport(0, 0, TexWakeVaoSize, TexWakeVaoSize);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(mVaoWake);

    mShaderWakeVaoToTex->use();
    mShaderWakeVaoToTex->setFloat("scaleX", 2.0f / TexWakeVaoSize);
    mShaderWakeVaoToTex->setFloat("scaleZ", 2.0f / TexWakeVaoSize);
    mShaderWakeVaoToTex->setFloat("offsetX", 0.0f);
    mShaderWakeVaoToTex->setFloat("offsetZ", 0.0f);
    mShaderWakeVaoToTex->setFloat("originX", ship.Position.x);
    mShaderWakeVaoToTex->setFloat("originZ", ship.Position.z);

    glBindVertexArray(mVaoWake);
    glDrawArrays(GL_TRIANGLES, 0, vWakeVertices.size());

    // Blit to normal sample
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, msFBO_WAKE);      // multisample FBO
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO_BLIT_WAKE);   // FBO classique (résolution)
    glBlitFramebuffer( 0, 0, TexWakeVaoSize, TexWakeVaoSize, 0, 0, TexWakeVaoSize, TexWakeVaoSize, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Gauss pass (horizontal)
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_GAUSS1);
    glViewport(0, 0, TexWakeVaoSize, TexWakeVaoSize);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    mShaderGaussH->use();
    mShaderGaussH->setSampler2D("tex", TexBlit, 0);
    mShaderGaussH->setFloat("texelWidth", 1.0f / (float)TexWakeVaoSize);
    mScreenQuadWakeBuffer->Render();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Gauss pass (horizontal)
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_GAUSS2);
    glViewport(0, 0, TexWakeVaoSize, TexWakeVaoSize);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    mShaderGaussV->use();
    mShaderGaussV->setSampler2D("tex", mTexGauss1, 0);
    mShaderGaussV->setFloat("texelHeight", 1.0f / (float)TexWakeVaoSize);
    mScreenQuadWakeBuffer->Render();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    mScreenQuadWakeBuffer->Render();  // -> TexWakeVao
}

// Render
void Ship::RenderForceRefBody(Camera& camera, vec3 P, vec3 V, float scale, vec3 color, bool bRenderOrigin)
{
    // P & V are in the starting position

    // Transformations in World
    float longueur = scale * glm::length(V);
    mat4 model(1.0f);
    model = glm::translate(model, vec3(longueur, 0, 0));
    model = glm::scale(model, vec3(longueur, 0.025f, 0.025f));
    quat q = RotationBetweenVectors(vec3(1.0f, 0.0f, 0.0f), V);
    model = glm::mat4_cast(q) * model;
    model = glm::translate(mat4(1.0f), P) * model;
    model = World * model;
   
    // Display
    mShaderPressure->use();
    mShaderPressure->setMat4("model", model);
    mShaderPressure->setMat4("view", camera.GetView());
    mShaderPressure->setMat4("projection", camera.GetProjection());
    mShaderPressure->setVec3("lineColor", color);
    mForceVector->Bind();

    // Draw the origin
    vec3 newP = TransformPosition(P);
    if (bRenderOrigin)
        mForceApplication->Render(camera, newP, 1.0f, color);
}
void Ship::RenderForceRefWorld(Camera& camera, sForce& f, float scale, vec3 color, bool bRenderOrigin)
{
    vec3 pos = vec3(glm::inverse(World) * vec4(f.Position, 1.0f));
    vec3 vec = vec3(glm::inverse(World) * vec4(f.Vector, 1.0f));

    // Transformations in World
    float longueur = scale * glm::length(f.Vector);
    mat4 model(1.0f);
    model = glm::translate(model, vec3(longueur, 0, 0));
    model = glm::scale(model, vec3(longueur, 0.025f, 0.025f));
    quat q = RotationBetweenVectors(vec3(1.0f, 0.0f, 0.0f), vec);
    model = mat4_cast(q) * model;
    model = glm::translate(mat4(1.0f), pos) * model;
    model = World * model;

    // Display
    mShaderPressure->use();
    mShaderPressure->setMat4("model", model);
    mShaderPressure->setMat4("view", camera.GetView());
    mShaderPressure->setMat4("projection", camera.GetProjection());
    mShaderPressure->setVec3("lineColor", color);
    mForceVector->Bind();

    // Draw the origin
    if(bRenderOrigin)
        mForceApplication->Render(camera, f.Position, 1.0f, color);
}
void Ship::RenderWakeVao(Camera& camera)
{
    if (!bWakeVao)
        return;

    if (vWakeVertices.size() < 2)
        return;

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    mShaderWakeVao->use();    // Shaders/unicolor.vert, Shaders/unicolor.frag
    mShaderWakeVao->setVec3("lineColor", vec3(1.0f, 1.0f, 1.0f));
    mShaderWakeVao->setMat4("model", mat4(1.0f));
    mShaderWakeVao->setMat4("view", camera.GetView());
    mShaderWakeVao->setMat4("projection", camera.GetProjection());

    glBindVertexArray(mVaoWake);
    glDrawArrays(GL_TRIANGLES, 0, vWakeVertices.size());

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
void Ship::RenderContour(Camera& camera)
{
    if (!bContour)
        return;

    mShaderUnicolor->use();

    mShaderUnicolor->setMat4("view", camera.GetView());
    mShaderUnicolor->setMat4("projection", camera.GetProjection());
    mat4 model = glm::translate(World, vec3(0.0f, 0.5f, 0.0f));
    mShaderUnicolor->setMat4("model", model);
    mShaderUnicolor->setVec3("lineColor", vec3(1.0));

    glBindVertexArray(mVaoContour1);
    glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(mIndicesContour1));
    glBindVertexArray(0);

    mShaderUnicolor->setVec3("lineColor", vec3(1.0, 0.0, 0.0));

    glBindVertexArray(mVaoContour2);
    glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(mIndicesContour2));
    glBindVertexArray(0);

}

void Ship::RenderNavLight(Camera& camera, int i, float distance)
{
    mShaderNavLight->use();     // Shaders/ship_light.vert, Shaders/ship_light.frag
    mat4 model = glm::translate(mat4(1.0), TransformPosition(ship.LightPositions[i]));
    float scale = mix(1.0f, 20.0f, distance / 2000.0f);
    model = glm::scale(model, vec3(scale));
    mShaderNavLight->setMat4("model", model);
    mShaderNavLight->setMat4("view", camera.GetView());
    mShaderNavLight->setMat4("projection", camera.GetProjection());
    mShaderNavLight->setVec3("lightColor", ship.LightColors[i]);
    mShaderNavLight->setVec3("viewPos", camera.GetPosition());
    mShaderNavLight->setFloat("intensity", 100000.0f);
    mLight->Bind();
}
void Ship::RenderPropellers(Camera& camera,Sky* sky, bool bReflexion)
{
    // Light from sun
    mShaderShip->use();     // Shaders/ship.vert, Shaders/ship.frag
    mShaderShip->setVec3("light.position", sky->SunPosition);
    mShaderShip->setVec3("light.ambient", sky->SunAmbient);
    mShaderShip->setVec3("light.diffuse", sky->SunDiffuse);
    mShaderShip->setVec3("light.specular", sky->SunSpecular);
    mShaderShip->setVec3("viewPos", camera.GetPosition());
    mShaderShip->setFloat("exposure", sky->Exposure);
    // Matricies
    float omega = PowerRpm * (2.0f * M_PI) / 60.0f; // radians par seconde
    static float rotation = 0.0f;
    rotation += omega * mDt; // incrémente la rotation à chaque frame
    if (rotation > 360.0f)
        rotation -= 360.0f;
    mat4 matPropeller1 = mat4(1.0f);
    matPropeller1 = glm::translate(matPropeller1, ship.Propeller1);
    matPropeller1 = glm::rotate(matPropeller1, rotation, vec3(1.0f, 0.0f, 0.0f));
    mShaderShip->setMat4("model", World * matPropeller1);
    
    if (!bReflexion)    mShaderShip->setMat4("view", camera.GetView());
    else                mShaderShip->setMat4("view", camera.GetViewReflexion());
    mShaderShip->setMat4("projection", camera.GetProjection());
    mPropeller->Render(*mShaderShip);
 
    if (ship.nPropeller > 1)
    {
        mat4 matPropeller2 = mat4(1.0f);
        matPropeller2 = glm::translate(matPropeller2, ship.Propeller2);
        matPropeller2 = glm::rotate(matPropeller2, rotation, vec3(1.0f, 0.0f, 0.0f));

        mShaderShip->setMat4("model", World * matPropeller2);
        mPropeller->Render(*mShaderShip);
    }
    if (ship.nPropeller > 2)
    {
        mat4 matPropeller3 = mat4(1.0f);
        matPropeller3 = glm::translate(matPropeller3, ship.Propeller3);
        matPropeller3 = glm::rotate(matPropeller3, rotation, vec3(1.0f, 0.0f, 0.0f));

        mShaderShip->setMat4("model", World * matPropeller3);
        mPropeller->Render(*mShaderShip);
    }
}
void Ship::RenderRudders(Camera& camera, Sky* sky, bool bReflexion)
{
    // Light from sun
    mShaderShip->use();     // Shaders/ship.vert, Shaders/ship.frag
    mShaderShip->setVec3("light.position", sky->SunPosition);
    mShaderShip->setVec3("light.ambient", sky->SunAmbient);
    mShaderShip->setVec3("light.diffuse", sky->SunDiffuse);
    mShaderShip->setVec3("light.specular", sky->SunSpecular);
    mShaderShip->setVec3("viewPos", camera.GetPosition());
    mShaderShip->setFloat("exposure", sky->Exposure);
    // Matricies
    float rotation = -RudderAngleDeg * M_PI / 180.0f;
    mat4 matRudder1 = mat4(1.0f);
    matRudder1 = glm::translate(matRudder1, ship.Rudder1);
    matRudder1 = glm::rotate(matRudder1, rotation, vec3(0.0f, 1.0f, 0.0f));
    mShaderShip->setMat4("model", World * matRudder1);

    if (!bReflexion)    mShaderShip->setMat4("view", camera.GetView());
    else                mShaderShip->setMat4("view", camera.GetViewReflexion());
    mShaderShip->setMat4("projection", camera.GetProjection());
    mRudder->Render(*mShaderShip);

    if (ship.nRudder > 1)
    {
        mat4 matRudder2 = mat4(1.0f);
        matRudder2 = glm::translate(matRudder2, ship.Rudder2);
        matRudder2 = glm::rotate(matRudder2, rotation, vec3(0.0f, 1.0f, 0.0f));

        mShaderShip->setMat4("model", World * matRudder2);
        mRudder->Render(*mShaderShip);
    }
    if (ship.nRudder > 2)
    {
        mat4 matRudder3 = mat4(1.0f);
        matRudder3 = glm::translate(matRudder3, ship.Rudder3);
        matRudder3 = glm::rotate(matRudder3, rotation, vec3(0.0f, 1.0f, 0.0f));

        mShaderShip->setMat4("model", World * matRudder3);
        mRudder->Render(*mShaderShip);
    }
}
void Ship::RenderRadars(Camera& camera, Sky* sky, bool bReflexion)
{
    if (!bVisible)
        return;

    if (!bRadar)
        return;

    if (ship.nRadar == 0)
        return;

    if (Rendering == eRendering::TRIANGLES)
        return;

    // Light from sun
    mShaderShip->use();     // Shaders/ship.vert, Shaders/ship.frag
    mShaderShip->setVec3("light.position", sky->SunPosition);
    mShaderShip->setVec3("light.ambient", sky->SunAmbient);
    mShaderShip->setVec3("light.diffuse", sky->SunDiffuse);
    mShaderShip->setVec3("light.specular", sky->SunSpecular);
    mShaderShip->setVec3("viewPos", camera.GetPosition());
    mShaderShip->setFloat("exposure", sky->Exposure);
    // Matricies
    static float rot1 = 0.0f;
    rot1 -= ship.RotationRadar1 * 6.0f * mDt;
    rot1 = fmod(rot1, 360.0f);
    mat4 matRadar1 = mat4(1.0f);
    matRadar1 = glm::translate(matRadar1, ship.Radar1);
    matRadar1 = glm::rotate(matRadar1, glm::radians(rot1), vec3(0.0f, 1.0f, 0.0f));
    mShaderShip->setMat4("model", World * matRadar1);

    if (!bReflexion)    mShaderShip->setMat4("view", camera.GetView());
    else                mShaderShip->setMat4("view", camera.GetViewReflexion());
    mShaderShip->setMat4("projection", camera.GetProjection());
    mRadar1->Render(*mShaderShip);

    if (ship.nRadar > 1)
    {
        static float rot2 = 0.0f;
        rot2 -= ship.RotationRadar2 * 6.0f * mDt;
        rot2 = fmod(rot2, 360.0f);
        mat4 matRadar2 = mat4(1.0f);
        matRadar2 = glm::translate(matRadar2, ship.Radar2);
        matRadar2 = glm::rotate(matRadar2, glm::radians(rot2), vec3(0.0f, 1.0f, 0.0f));
        mShaderShip->setMat4("model", World * matRadar2);
        mRadar2->Render(*mShaderShip);
    }
}
void Ship::RenderSmoke(Camera& camera)
{
    if (!bVisible)
        return;

    if (!bSmoke)
        return;

    if (ship.nChimney == 0)
        return;

    glDepthMask(GL_FALSE);
    float density = InterpolateAValue(0.0f, mPowerW, 0.01f, 0.15f, fabs(PowerApplied));
    if (mSmokeLeft)
        mSmokeLeft->Render(camera, density);
    if (ship.nChimney > 1 && mSmokeRight)
        mSmokeRight->Render(camera, density);
    glDepthMask(GL_TRUE);
}

void Ship::Render(Camera& camera, Sky* sky, bool bReflexion)
{
    if (!bVisible)
        return;
    
    UpdateWorldMatrix();
    g_SoundMgr->setListenerPosition(camera.GetPosition());
    g_SoundMgr->setListenerOrientation(camera.GetAt(), camera.GetUp());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_CULL_FACE); // Necessary because the windows of the bridge are transparent

    if (bWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

#pragma region Ship
    switch (Rendering)
    {
    case eRendering:: TRIANGLES:
    {
        mShaderHullColored->use();     // Shaders/hull_colored.vert, Shaders/hull_colored.frag
        mShaderHullColored->setMat4("model", World);
        mShaderHullColored->setMat4("view", camera.GetView());
        mShaderHullColored->setMat4("projection", camera.GetProjection());
        
        glBindVertexArray(mVao);
        glDrawElements(GL_TRIANGLES, mIndicesFull, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    break;
    case eRendering::BASIC_LIGHT:
    {
        // Light from camera
        mShaderCamera->use();   // Shaders/camera.vert, Shaders/camera.frag
        mShaderCamera->setVec3("light.position", camera.GetPosition() - ship.Position);    
        mShaderCamera->setVec3("light.diffuse", vec3(1.0f));
        mShaderCamera->setMat4("model", World);
        if(!bReflexion)
            mShaderCamera->setMat4("view", camera.GetView());
        else
            mShaderCamera->setMat4("view", camera.GetViewReflexion());
        mShaderCamera->setMat4("projection", camera.GetProjection());
        mModelFull->Render(*mShaderCamera);
    }
    break;
    case eRendering::SUN:
    {
        // Light from sun
        mShaderShip->use();     // Shaders/ship.vert, Shaders/ship.frag
        mShaderShip->setVec3("light.position", sky->SunPosition);
        mShaderShip->setVec3("light.ambient", sky->SunAmbient);
        mShaderShip->setVec3("light.diffuse", sky->SunDiffuse);
        mShaderShip->setVec3("light.specular", sky->SunSpecular);
        mShaderShip->setVec3("viewPos", camera.GetPosition());
		mShaderShip->setFloat("exposure", sky->Exposure);
        if (camera.GetMode() == eCameraMode::ORBITAL || camera.GetMode() == eCameraMode::FPS)
            mShaderShip->setFloat("envmapFactor", ship.EnvMapfactor);
        else
            mShaderShip->setFloat("envmapFactor", 0.0f);
        mShaderShip->setInt("envmap", 1);
        // Matricies
        mShaderShip->setMat4("model", World);
        if (!bReflexion)
            mShaderShip->setMat4("view", camera.GetView());
        else
            mShaderShip->setMat4("view", camera.GetViewReflexion());
        mShaderShip->setMat4("projection", camera.GetProjection());
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTexEnvironment->id);

        mModelFull->Render(*mShaderShip);
    }
    break;
    }
#pragma endregion
   
    RenderPropellers(camera, sky, bReflexion);
    RenderRudders(camera, sky, bReflexion);
    RenderRadars(camera, sky, bReflexion);

    if (bWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#pragma region Navigation lights
    {
        vec3 lightDir = glm::normalize(sky->SunPosition);
        float dCamera_Ship = glm::length(camera.GetPosition() - ship.Position);
        if (bLights /*lightDir.y < 0.2*/)
        {
            // Visibility of navigation lights
            vec3 shipForward = TransformPosition(vec3(mLength * 0.5f, 0.0f, 0.0f)) - ship.Position;
            vec3 cameraToShip = camera.GetPosition() - ship.Position;
            vec3 up(0.0f, 1.0f, 0.0f);
            shipForward.y = 0.0f;
            cameraToShip.y = 0.0f;
            float angleDeg = degrees(orientedAngle(glm::normalize(shipForward), glm::normalize(cameraToShip), up));
            if (angleDeg > -112.5f && angleDeg < -3.0f)
                RenderNavLight(camera, 1, dCamera_Ship);  // Green
            else if (angleDeg >= -3.0f && angleDeg <= 3.0f)
            {
                RenderNavLight(camera, 0, dCamera_Ship);  // Red
                RenderNavLight(camera, 1, dCamera_Ship);  // Green
            }
            else if (angleDeg > 3.0f && angleDeg < 112.5f)
                RenderNavLight(camera, 0, dCamera_Ship);  // Red
            else
                RenderNavLight(camera, 2, dCamera_Ship);  // White
            RenderNavLight(camera, 3, dCamera_Ship);      // White high
            if (ship.LightPositions.size() > 4)
                RenderNavLight(camera, 4, dCamera_Ship);  // White high
        }
    }
#pragma endregion

#pragma region Wireframe
    if (bOutline)
    {
        mShaderWireframe->use();    // Shaders/unicolor.vert, Shaders/unicolor.frag, Shaders/unicolor.geom
        mShaderWireframe->setVec3("lineColor", vec3(1.0f, 1.0f, 1.0f));
        mShaderWireframe->setMat4("model", World);
        mShaderWireframe->setMat4("view", camera.GetView());
        mShaderWireframe->setMat4("projection", camera.GetProjection());

        switch (Rendering)
        {
        case eRendering::TRIANGLES:
            glBindVertexArray(mVao);
            glDrawElements(GL_TRIANGLES, mIndicesFull, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            break;
        case eRendering::BASIC_LIGHT:
        case eRendering::SUN:
            mModelFull->Bind();
            break;
        }
    }
#pragma endregion

#pragma region Pressure
    if (bPressure)
    {
        mShaderPressure->use();     // Shaders/unicolor.vert, Shaders/unicolor.frag
        mShaderPressure->setVec3("lineColor", vec3(0.3f, 0.5f, 1.0f));
        mat4 model(1.0f);
        mShaderPressure->setMat4("model", model);
        mShaderPressure->setMat4("view", camera.GetView());
        mShaderPressure->setMat4("projection", camera.GetProjection());

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBindVertexArray(mVaoLines);
        glDrawArrays(GL_LINES, 0, mLinesCount);
        glBindVertexArray(0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
#pragma endregion

#pragma region Bounding box
    if (BBoxShape->bVisible)
	{
		mShaderUnicolor->use();     // Shaders/unicolor.vert", Shaders/unicolor.frag
		mShaderUnicolor->setMat4("view", camera.GetView());
		mShaderUnicolor->setMat4("projection", camera.GetProjection());
		mShaderUnicolor->setMat4("model", World);
		mShaderUnicolor->setVec3("lineColor", vec3(1.0));
		BBoxShape->Bind();
	}
#pragma endregion

#pragma region Forces
    if (bForces)
    {
        float f = 5.0f / -Gravity.Vector.y;

        RenderForceRefWorld(camera, Archimede, f, Blue, true);
        RenderForceRefWorld(camera, Gravity, f, Gray, true);
        RenderForceRefWorld(camera, ResistanceHeave, f, Orange, true);
        RenderForceRefWorld(camera, Thrust, 50.0f * f, Red, true);
        RenderForceRefWorld(camera, ResistanceViscous, 50.0f * f, Pink, true);
        RenderForceRefWorld(camera, ResistanceWaves, 50.0f * f, Cyan, true);
        RenderForceRefWorld(camera, BowThrust, 50.0f * f, Yellow, true);
        RenderForceRefWorld(camera, RudderLift, 50.0f * f, Green, true);
        RenderForceRefWorld(camera, RudderDrag, 50.0f * f, Magenta, true);
        RenderForceRefWorld(camera, WindFront, 50.0f * f, Gold, true);
        RenderForceRefWorld(camera, WindRear, 50.0f * f, Gold, true);
    
        RenderForceRefWorld(camera, COGSOG, 50.0f * f, Yellow, true);
    }
#pragma endregion

#pragma region Axis
    if (bAxis)
    {
        mat4 modelX = glm::scale(World, vec3(2.0f + mLength / 2.0f, 0.025f, 0.025f));
        mAxis->RenderDistorted(camera, modelX, vec3(1.0f, 0.0f, 0.0f));

        mat4 modelY = glm::scale(World, vec3(0.025f, 2.0f + mHeight / 2.0f, 0.025f));
        mAxis->RenderDistorted(camera, modelY, vec3(0.0f, 1.0f, 0.0f));

        mat4 modelZ = glm::scale(World, vec3(0.025f, 0.025f, 2.0f + mWidth / 2.0f));
        mAxis->RenderDistorted(camera, modelZ, vec3(0.0f, 0.0f, 1.0f));
    }
#pragma endregion

}
void Ship::RenderShadow(Shader* shader, Camera& camera, mat4& lightSpaceMatrix)
{
    UpdateWorldMatrix();
    shader->use();
    // Matricies
    shader->setMat4("model", World);
    shader->setMat4("view", camera.GetView());
    shader->setMat4("projection", camera.GetProjection());
    shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    mModelFull->Render(*shader);
}