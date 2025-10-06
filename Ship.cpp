/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#include "Ship.h"
#include "MeshPlaneIntersect.hpp"   // Used to find a contour
#include <math.h>

#include "clipper/clipper.h"        // Used to offset a contour
#ifdef _DEBUG
#pragma comment(lib, "clipper/Debug/clipper.lib")
#else
#pragma comment(lib, "clipper/Release/clipper.lib")
#endif
using namespace Clipper2Lib;        

//#define DEBUG_SMOKE // Update smoke.comp with 2 lines: layout(std430, binding = 1) buffer CounterBuffer { int liveCounter; }; atomicAdd(liveCounter, 1);

extern float            g_WindSpeedKN;
extern vec2             g_Wind;
extern SoundManager   * g_SoundMgr;
extern bool             g_bPause;
extern Camera           g_Camera;
extern bool             g_bShipShadow;
extern bool             g_bShowShipForcesWindows;

GLuint                  TexContourShip      = 0;                // Texture of the contour of the ship
int                     TexContourShipW;
int                     TexContourShipH;
GLuint                  TexWakeBuffer       = 0;
int					    TexWakeBufferSize   = 512;
GLuint                  TexWakeVao          = 0;
int					    TexWakeVaoSize      = 1024;
bool                    bTexWakeByVAO       = true;
GLuint				    TexShadowMap        = 0;
mat4				    LightViewProjection;


Ship::~Ship()
{
    BBoxShape.reset();
    mSpray.reset();

    glDeleteVertexArrays(1, &mVaoHull);
    glDeleteBuffers(1, &mVboHull);
    glDeleteVertexArrays(1, &mVaoLines);

    mModelFull.reset();
    mPropeller1.reset();
    mRudder.reset();
    mLight.reset();
    mRadar1.reset();
    mRadar2.reset();

    mShaderHullColored.reset();
    mShaderWireframe.reset();
    mShaderPressure.reset();
    mShaderCamera.reset();
    mShaderShipShadow.reset();
    mShaderUnicolor.reset();
    mShaderNavLight.reset();

    mTexEnvironment.reset();

    mForceVector.reset();
    mForceApplication.reset();
    mAxis.reset();

    mSoundThrust1.reset();
    mSoundBowThruster.reset();
    mSoundSternThruster.reset();

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

void Ship::SetOcean(Ocean* ocean)
{
    mOcean = ocean;
    pDisplacement = ocean->GetPixelsDisplacement();
};
void Ship::Init(sShip& ship, Camera& camera)
{
    //CreateKelvinImages();

    this->ship = ship;
    
    // Read the mvVertices and the faces
    igl::readOBJ(ship.PathnameHull.c_str(), mV, mF);
    stringstream ssHull;
    ssHull << "Hull: " << mV.rows() << " vertices & " << mF.rows() << " faces" << endl;

    // Vertices
    mvVertices.resize(mV.rows());
    mvVertSubmerged.resize(mV.rows());
    mvVertWaterHeight.resize(mV.rows());
    World = mat4(1.0f);
    TransformVertices();

    // Get data
    InitDimensions();

    UpdateWorldMatrix();                        // Necessary for several calculations to come

    mvTris.resize(mF.rows());
    InitTriangles();                            // Create the list of the triangles
    InitCentroid();                             // Compute the centre of the volume
    InitSurfaces();                             // Certain surfaces
    InitInertia();                              // Compute volume & all moments of inertia (Ixx, Iyy, Izz, Ixy, Ixz, Iyz)
    
    // Info to display with interface
    ssHull << "Length/Width : " << std::fixed << std::setprecision(2) << mLength << " m x " << mWidth << " m " << endl;
    ssHull << "Draft : " << std::fixed << std::setprecision(2) << mDraft << " m" << endl;
    ssHull << "Mass : " << std::setprecision(0) << int(ship.Mass_t) << " t" << endl;
    InfoHull = ssHull.str();
   
    InitWaterVertices();                        // Create the list of water vertices in the reference patch
    InitVaoHull();                              // Create the VAO of the colored hull
    InitContours();
    InitShaders();
    InitTextures();
    InitVaoWake();
    InitModels();
    InitSounds(camera);
    InitSmoke();

    mSpray = make_unique<Spray>();

    ResetVelocities();
    bMotion = false;
    bSound = true;

#ifdef TRACE
    InitTrace();
#endif

    Archimede.Name = "Archimede";
    Gravity.Name = "Gravity";
    ResistanceHeave.Name = "ResistanceHeave";
    Thrust1.Name = "Thrust1";
    Thrust2.Name = "Thrust2";
    PropDrag1.Name = "PropDrag1";
    PropDrag2.Name = "PropDrag2";
    PropTorque1.Name = "PropTorque1";
    PropTorque2.Name = "PropTorque2";
    ResistanceViscous.Name = "ResistanceViscous";
    ResistanceWaves.Name = "ResistanceWaves";
    ResistanceResidual.Name = "ResistanceResidual";
    BowThrust.Name = "BowThrust";
    SternThrust.Name = "SternThrust";
    RudderLift.Name = "RudderLift";
    RudderDrag.Name = "RudderDrag";
    WindRotation.Name = "WindRotation";
    WindFront.Name = "WindFront";
    WindRear.Name = "WindRear";
    ResistanceAir.Name = "ResistanceAir";
    Centrifugal.Name = "Centrifugal";
}
void Ship::InitDimensions()
{
    mModelFull = make_unique<Model>(ship.PathnameFull);
    mBbox = mModelFull->GetBoundingBox();
    mMass = ship.Mass_t * 1000.0f;              // t -> kg for all physical calculations
    mPowerW = ship.PowerkW * 1000.0f;           // kW -> W for all physical calculations
    if (ship.nPropeller == 2)
        mPowerW *= 0.5f;
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
}
void Ship::InitTriangles()
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
void Ship::InitCentroid()
{
    mCentroid = vec3(0.0f);
    for (const auto& tri : mvTris)
        mCentroid += mvVertices[tri.I[0]] + mvVertices[tri.I[1]] + mvVertices[tri.I[2]];

    if (mvTris.size())
        mCentroid /= (mvTris.size() * 3.0f);

#ifdef PROPERTIES
    cout << "Centroid : ( " << mCentroid.x << ", " << mCentroid.y << ", " << mCentroid.z << " )" << endl;
#endif
}
bool IsPolygonClockwise(const Clipper2Lib::Path64& polygon)
{
    int64_t sum = 0;
    int n = (int)polygon.size();
    for (int i = 0; i < n; ++i)
    {
        int j = (i + 1) % n;
        sum += (polygon[j].x - polygon[i].x) * (polygon[j].y + polygon[i].y);
    }
    return sum > 0; // true si dans le sens horaire (clockwise)
}
void Ship::InitSurfaces()
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

    if (ship.AreaFront != 0.0f && ship.AreaLat != 0.0f)
    {
        AreaFront = ship.AreaFront;
        AreaFrontCenter = ship.AreaFrontCenter;
        AreaLat = ship.AreaLat;
        AreaLatCenter = ship.AreaLatCenter;
        return;
    }

    vector<Mesh>& vMeshes = mModelFull->GetMesh();

    // Get the vertices
    Clipper2Lib::Paths64 projectedFront;
    Clipper2Lib::Paths64 projectedLat;
    double scale = 1e4;
    double scale2 = scale * scale;

    for (const auto& mesh : vMeshes)
    {
        for (size_t i = 0; i < mesh.vIndices.size(); i += 3)
        {
            const sVertex& v0 = mesh.vVertices[mesh.vIndices[i]];
            const sVertex& v1 = mesh.vVertices[mesh.vIndices[i + 1]];
            const sVertex& v2 = mesh.vVertices[mesh.vIndices[i + 2]];

            Clipper2Lib::Path64 triFront;
            triFront.push_back({ (int64_t)(v0.Position.y * scale), (int64_t)(v0.Position.z * scale) });
            triFront.push_back({ (int64_t)(v1.Position.y * scale), (int64_t)(v1.Position.z * scale) });
            triFront.push_back({ (int64_t)(v2.Position.y * scale), (int64_t)(v2.Position.z * scale) });
            projectedFront.push_back(triFront);
        
            Clipper2Lib::Path64 triLat;
            triLat.push_back({ (int64_t)(v0.Position.x * scale), (int64_t)(v0.Position.y * scale) });
            triLat.push_back({ (int64_t)(v1.Position.x * scale), (int64_t)(v1.Position.y * scale) });
            triLat.push_back({ (int64_t)(v2.Position.x * scale), (int64_t)(v2.Position.y * scale) });
            projectedLat.push_back(triLat);
        }
    }

    // Reverse the clockwise polygons
    for (auto& poly : projectedFront)
        if (IsPolygonClockwise(poly))
            std::reverse(poly.begin(), poly.end());

    for (auto& poly : projectedLat)
        if (IsPolygonClockwise(poly))
            std::reverse(poly.begin(), poly.end());

    // Area front
    Clipper2Lib::Path64 clipPolygonFront;
    clipPolygonFront.push_back({ (int64_t)0, numeric_limits<int64_t>::min() / 2 });
    clipPolygonFront.push_back({ numeric_limits<int64_t>::max() / 2, numeric_limits<int64_t>::min() / 2 });
    clipPolygonFront.push_back({ numeric_limits<int64_t>::max() / 2, numeric_limits<int64_t>::max() / 2 });
    clipPolygonFront.push_back({ (int64_t)0, numeric_limits<int64_t>::max() / 2 });

    Clipper2Lib::Clipper64 clipperFront;
    clipperFront.AddSubject(projectedFront);
    clipperFront.AddClip(Clipper2Lib::Paths64{ clipPolygonFront });

    Clipper2Lib::Paths64 solutionFront;
    clipperFront.Execute(Clipper2Lib::ClipType::Intersection, Clipper2Lib::FillRule::NonZero, solutionFront);
    
    // Area lateral
    Clipper2Lib::Path64 clipPolygonLat;
    clipPolygonLat.push_back({ (int64_t)0, numeric_limits<int64_t>::min() / 2 });
    clipPolygonLat.push_back({ numeric_limits<int64_t>::max() / 2, numeric_limits<int64_t>::min() / 2 });
    clipPolygonLat.push_back({ numeric_limits<int64_t>::max() / 2, numeric_limits<int64_t>::max() / 2 });
    clipPolygonLat.push_back({ (int64_t)0, numeric_limits<int64_t>::max() / 2 });
    
    Clipper2Lib::Clipper64 clipperLat;
    clipperLat.AddSubject(projectedLat);
    clipperLat.AddClip(Clipper2Lib::Paths64{ clipPolygonLat });

    Clipper2Lib::Paths64 solutionLat;
    clipperLat.Execute(Clipper2Lib::ClipType::Intersection, Clipper2Lib::FillRule::NonZero, solutionLat);
    
    // Geometric front center
    for (const auto& poly : solutionFront)
    {
        double area_poly = Clipper2Lib::Area(poly) / scale2;
        if (area_poly < 1e-12) continue;

        double cx_poly = 0.0;
        double cy_poly = 0.0;

        int n = (int)poly.size();
        for (int i = 0; i < n; ++i)
        {
            int j = (i + 1) % n;
            double xi = (double)poly[i].x / scale;
            double yi = (double)poly[i].y / scale;
            double xj = (double)poly[j].x / scale;
            double yj = (double)poly[j].y / scale;

            double factor = (xi * yj - xj * yi);
            cx_poly += (xi + xj) * factor;
            cy_poly += (yi + yj) * factor;
        }

        cx_poly /= (6.0 * area_poly);
        cy_poly /= (6.0 * area_poly);

        AreaFrontCenter.y += cx_poly * area_poly;
        AreaFrontCenter.z += cy_poly * area_poly;
        AreaFront += area_poly;
    }
    if (AreaFront > 0)
    {
        AreaFrontCenter.y /= AreaFront;
        AreaFrontCenter.z /= AreaFront;
    }

    // Geometric lateral center
    for (const auto& poly : solutionLat)
    {
        double area_poly = Clipper2Lib::Area(poly) / (scale * scale);
        if (area_poly < 1e-12) continue;

        double cx_poly = 0.0;
        double cy_poly = 0.0;

        int n = (int)poly.size();
        for (int i = 0; i < n; ++i)
        {
            int j = (i + 1) % n;
            double xi = (double)poly[i].x / scale;
            double yi = (double)poly[i].y / scale;
            double xj = (double)poly[j].x / scale;
            double yj = (double)poly[j].y / scale;

            double factor = (xi * yj - xj * yi);
            cx_poly += (xi + xj) * factor;
            cy_poly += (yi + yj) * factor;
        }

        cx_poly /= (6.0 * area_poly);
        cy_poly /= (6.0 * area_poly);

        AreaLatCenter.x += cx_poly * area_poly;
        AreaLatCenter.y += cy_poly * area_poly;
        AreaLat += area_poly;
    }
    if (AreaLat > 0)
    {
        AreaLatCenter.x /= AreaLat;
        AreaLatCenter.y /= AreaLat;
    }
    //cout << "total_area_front : " << AreaFront << "  ";
    //PrintGlmVec3(AreaFrontCenter);
    //cout << "total_area_lat : " << AreaLat << "  ";
    //PrintGlmVec3(AreaLatCenter);
    //cout << endl;
}
void Ship::InitInertia()
{
    mVolume = 0.0f; // m3
    Ixx = 0.0f;     // kg.m2
    Iyy = 0.0f;
    Izz = 0.0f;
    Ixy = 0.0f;
    Ixz = 0.0f;
    Iyz = 0.0f;

    // Calculation of total volume and moments of inertia
    for (const auto& tri : mvTris)
    {
        vec3 a = mvVertices[tri.I[0]];
        vec3 b = mvVertices[tri.I[1]];
        vec3 c = mvVertices[tri.I[2]];

        // Calcul du volume signé de ce tétraèdre (méthode 1)
        float volume = ( a.x * b.y * c.z + a.y * b.z * c.x + b.x * c.y * a.z - c.x * b.y * a.z - b.x * a.y * c.z - c.y * b.z * a.x ) / 6.0f;
        mVolume += volume;

        // Calcul des moments d'inertie, comme précédemment
        Ixx += (a.y * a.y + b.y * b.y + c.y * c.y + a.y * b.y + b.y * c.y + c.y * a.y + a.z * a.z + b.z * b.z + c.z * c.z + a.z * b.z + b.z * c.z + c.z * a.z) * volume / 10.0f;
        Iyy += (a.x * a.x + b.x * b.x + c.x * c.x + a.x * b.x + b.x * c.x + c.x * a.x + a.z * a.z + b.z * b.z + c.z * c.z + a.z * b.z + b.z * c.z + c.z * a.z) * volume / 10.0f;
        Izz += (a.x * a.x + b.x * b.x + c.x * c.x + a.x * b.x + b.x * c.x + c.x * a.x + a.y * a.y + b.y * b.y + c.y * c.y + a.y * b.y + b.y * c.y + c.y * a.y) * volume / 10.0f;
        Ixy += (2 * a.x * a.y + 2 * b.x * b.y + 2 * c.x * c.y + a.x * b.y + a.x * c.y + b.x * a.y + b.x * c.y + c.x * a.y + c.x * b.y) * volume / 20.0f;
        Ixz += (2 * a.x * a.z + 2 * b.x * b.z + 2 * c.x * c.z + a.x * b.z + a.x * c.z + b.x * a.z + b.x * c.z + c.x * a.z + c.x * b.z) * volume / 20.0f;
        Iyz += (2 * a.y * a.z + 2 * b.y * b.z + 2 * c.y * c.z + a.y * b.z + a.y * c.z + b.y * a.z + b.y * c.z + c.y * a.z + c.y * b.z) * volume / 20.0f;
    }
   
    if (mVolume != 0.0f)
    {
        // Calculation of density (mass in kg, volume in m3)
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
    cout << "Volume : " << mVolume << " m3" << endl;
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
void Ship::InitWaterVertices()
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
void Ship::InitVaoHull()
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

    glGenVertexArrays(1, &mVaoHull);
    glGenBuffers(1, &mVboHull);
    glGenBuffers(1, &mEboHull);

    glBindVertexArray(mVaoHull);

    glBindBuffer(GL_ARRAY_BUFFER, mVboHull);
    glBufferData(GL_ARRAY_BUFFER, mvVertexColored.size() * sizeof(float), mvVertexColored.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEboHull);
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
void Ship::InitContours()
{
    // Contour
    vector<vec3> contour = ComputeContour();    // Intersect the mesh with the ocean (Y = 0)
    if (contour.size() == 0)
        return;

    contour = ArrangeContour(contour);          // Sort the points
    CreateContourVAO1(contour);                 // Create the first contour which has the size of the ship

    vector<vec3> contourSpray = OffsetContour(contour, 0.05f);
    InitSpray(contourSpray);

    contour = OffsetContour(contour, 1.0f);     // Expand the contour with a constant offset
    CreateContourVAO2(contour);                 // Create the second contour which is expanded
    CreateTexWake(contour);                     // Create the texture formed with foam inside the exapnded contour
}
void Ship::InitShaders()
{
    // Shaders for the ship
    mShaderHullColored = make_unique<Shader>("Resources/Ship/hull_colored.vert", "Resources/Ship/hull_colored.frag");       // For the hull (colored triangles for Archimede)
    mShaderWireframe = make_unique<Shader>("Resources/Misc/unicolor.vert", "Resources/Misc/unicolor.frag", "Resources/Misc/unicolor.geom");
    mShaderPressure = make_unique<Shader>("Resources/Misc/unicolor.vert", "Resources/Misc/unicolor.frag");                  // For the forces of pressure (lines)
    mShaderCamera = make_unique<Shader>("Resources/Misc/camera.vert", "Resources/Misc/camera.frag");                        // For the ship (the camera is the sun)
    mShaderShipShadow = make_unique<Shader>("Resources/Ship/ship_shadow.vert", "Resources/Ship/ship_shadow.frag");          // Enhanced shader for the ship model
    mShaderShip = make_unique<Shader>("Resources/Ship/ship.vert", "Resources/Ship/ship.frag");
    mShaderUnicolor = make_unique<Shader>("Resources/Misc/unicolor.vert", "Resources/Misc/unicolor.frag");                  // For bounding box & contours
    mShaderNavLight = make_unique<Shader>("Resources/Ship/ship_light.vert", "Resources/Ship/ship_light.frag");              // For the navigation lights of the ship
    mShaderShadow = make_unique<Shader>("Resources/Ship/shadow.vert", "Resources/Ship/shadow.frag");                        // For the shadow depth map

    mShaderShipShadow->use();
    mShaderShipShadow->setInt("texture_diffuse1", 1);
    mShaderShipShadow->setInt("shadowMap", 2);

    // Shaders for the wake
    mShaderBuffer = make_unique<Shader>("Resources/Ship/wake_buffer.vert", "Resources/Ship/wake_buffer.frag");              // Wake as an accumulation buffer
    mShaderWakeVaoToTex = make_unique<Shader>("Resources/Ship/wake_vao.vert", "Resources/Ship/wake_vao.frag");              // Wake as a projection of VAO on texture
    mShaderGaussH = make_unique<Shader>("Resources/Ship/wake_gauss.vert", "Resources/Ship/wake_gauss_h.frag");              // Gaussian blur - horizontal pass (1)
    mShaderGaussV = make_unique<Shader>("Resources/Ship/wake_gauss.vert", "Resources/Ship/wake_gauss_v.frag");              // Gaussian blur - vertical pass (2)
    mShaderWakeVao = make_unique<Shader>("Resources/Misc/unicolor.vert", "Resources/Misc/unicolor.frag");             // For the drawing of the vao (as a debug)
}
void Ship::InitTextures()
{
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

    // S H A D O W ===========================================================
    
    glGenFramebuffers(1, &FBO_SHADOW);
    glGenTextures(1, &TexShadowMap);
    glBindTexture(GL_TEXTURE_2D, TexShadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_SIZE, SHADOW_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, FBO_SHADOW);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, TexShadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "Error creating FBO for the Shadow pass" << endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create environment map
    mTexEnvironment = make_unique<Texture>();
    mTexEnvironment->CreateFromDDSFile("Resources/Ocean/ocean_env.dds");
}
void Ship::InitVaoWake()
{
    glGenVertexArrays(1, &mVaoWake);

    glGenBuffers(1, &mVboWake);
    glBindVertexArray(mVaoWake);
    glBindBuffer(GL_ARRAY_BUFFER, mVboWake);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    // position (3 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(sFoamVertex), (void*)offsetof(sFoamVertex, pos));
    // alpha (1 float)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(sFoamVertex), (void*)offsetof(sFoamVertex, alpha));

    glBindVertexArray(0);
}
void Ship::InitModels()
{
    // The ship
    stringstream ssFull;
    ssFull << mModelFull->NbVertices << " vertices & " << mModelFull->NbFaces << " faces" << endl;
    InfoFull = ssFull.str();
    if (ship.PathnamePropeller1.length())
        mPropeller1 = make_unique<Model>(ship.PathnamePropeller1);
    if (ship.PathnamePropeller2.length())
        mPropeller2 = make_unique<Model>(ship.PathnamePropeller2);
    if (ship.PathnameRudder.length())
        mRudder = make_unique<Model>(ship.PathnameRudder);
    if (ship.PathnameRadar1.length())
        mRadar1 = make_unique<Model>(ship.PathnameRadar1);
    if (ship.nRadar > 1 && ship.PathnameRadar2.length())
        mRadar2 = make_unique<Model>(ship.PathnameRadar2);
    if (ship.bFlag)
    {
        float spacing = ship.DimXFlag / 15.0f;
        mFlag = make_unique<Flag>(15, 10, spacing, ship.PathnameFlag.c_str());
    }

    // Others
    BBoxShape = make_unique<BBox>(mBbox.min, mBbox.max);
    BBoxShape->bVisible = false;
    mForceVector = make_unique<Cube>();
    mForceApplication = make_unique<Sphere>(0.1f, 16);
    mAxis = make_unique<Cube>();
    mLight = make_unique<Sphere>(0.1f, 16);
}
void Ship::InitSounds(Camera& camera)
{
    // Sounds
    g_SoundMgr->setListenerPosition(camera.GetPosition());
    g_SoundMgr->setListenerOrientation(camera.GetAt(), camera.GetUp());
    
    // Power 1
    mSoundThrust1 = make_unique<Sound>(ship.ThrustSound);
    mSoundThrust1->setVolume(0.25f);
    mSoundThrust1->setPosition(ship.Position + ship.PosPropeller1);
    mSoundThrust1->setLooping(true);
    mSoundThrust1->adjustDistances();
 
    // Power 2
    mSoundThrust2 = make_unique<Sound>(ship.ThrustSound);
    mSoundThrust2->setVolume(0.25f);
    mSoundThrust2->setPosition(ship.Position + ship.PosPropeller2);
    mSoundThrust2->setLooping(true);
    mSoundThrust2->adjustDistances();
    
    if (bSound)
    {
        mSoundThrust1->play();
        mSoundThrust2->play();
    }

    // Bow thruster
    if (ship.HasBowThruster)
    {
        mSoundBowThruster = make_unique<Sound>(ship.BowThrusterSound);
        mSoundBowThruster->setVolume(0.25f);
        mSoundBowThruster->setPosition(ship.Position + ship.PosBowThruster);
        mSoundBowThruster->setLooping(true);
        mSoundBowThruster->adjustDistances();
    }

    // Stern thruster
    if (ship.HasSternThruster)
    {
        mSoundSternThruster = make_unique<Sound>(ship.SternThrusterSound);
        mSoundSternThruster->setVolume(0.25f);
        mSoundSternThruster->setPosition(ship.Position + ship.PosSternThruster);
        mSoundSternThruster->setLooping(true);
        mSoundSternThruster->adjustDistances();
    }
}
void Ship::InitSmoke()
{
    // Initialize array of "dead" particles
    vector<ParticleGPU> particles(mSmokeMaxParticles);

    for (int i = 0; i < mSmokeMaxParticles; ++i)
        particles[i].life = 0.0f; // dead at the start

    // Generate and allocate SSBO
    glGenBuffers(1, &mSSBO_SMOKE);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSSBO_SMOKE);
    glBufferData(GL_SHADER_STORAGE_BUFFER, mSmokeMaxParticles * sizeof(ParticleGPU), particles.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mSSBO_SMOKE);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // VAO setup for drawing points
    glGenVertexArrays(1, &mVaoSmoke);
    glBindVertexArray(mVaoSmoke);
    // No attributes needed, data will be read into vertex shader via SSBO
    glBindVertexArray(0);

#ifdef DEBUG_SMOKE
    glGenBuffers(1, &SSBO_LIVE_COUNTER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO_LIVE_COUNTER);
    int zero = 0;
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int), &zero, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
#endif

    mShaderSmokeCompute = make_unique<Shader>("", "", "", "Resources/Ship/smoke.comp");
    mShaderSmokeRender = make_unique<Shader>("Resources/Ship/smoke.vert", "Resources/Ship/smoke.frag");
}
void FilterClosePoints(std::vector<sSprayPt>& pts)
{
    if (pts.size() < 2)
        return;

    float totalDist = glm::length(pts.front().p - pts.back().p);
    float threshold = totalDist / pts.size();

    // nouvelle liste filtrée
    std::vector<sSprayPt> filtered;
    filtered.reserve(pts.size());

    filtered.push_back(pts[0]); // toujours garder le premier point
    vec3 lastPos = pts[0].p;

    for (size_t i = 1; i < pts.size(); ++i)
    {
        float dist = glm::length(pts[i].p - lastPos);
        if (dist >= threshold)
        {
            filtered.push_back(pts[i]);
            lastPos = pts[i].p;
        }
        // sinon on ignore le point trop proche
    }

    pts = std::move(filtered);
}
void Ship::InitSpray(vector<vec3>& contour)
{
    if (contour.size() == 0)
        return;

    float maxForward = contour[0].x;
    int frontIndex = 0;
    for (int i = 1; i < (int)contour.size(); i++)
    {
        if (contour[i].x > maxForward)
        {
            maxForward = contour[i].x;
            frontIndex = i;
        }
    }

    vec3 frontPoint = contour[frontIndex];

    mLeft.clear();
    mRight.clear();

    float dist = mLength * ship.SprayLength;

    for (size_t i = 0; i < contour.size(); ++i)
    {
        vec3 p = contour[i];
        float d = glm::length(p - frontPoint);

        if (d <= dist)
        {
            // Find the point before and the point after on the contour
            int idxPrev = (i == 0) ? (int)contour.size() - 1 : (int)i - 1;
            int idxNext = (i == contour.size() - 1) ? 0 : (int)i + 1;

            vec3 prev = contour[idxPrev];
            vec3 next = contour[idxNext];

            // Tangent vector (direction of the contour at point p)
            vec3 tangent = glm::normalize(next - prev);

            // For the x/z plane: take the outside
            vec3 toPrev = prev - p;
            vec3 toNext = next - p;
            // Lateral vector in the plane (here (toNext - toPrev))
            vec3 lateral = toNext - toPrev;
            // Normal: perpendicular to tangent, oriented outward
            vec3 n = glm::normalize(glm::cross(tangent, vec3(0, 1, 0)));

            // We want n.z to have the same sign as p.z 
            if ((p.z < 0.0f && n.z < 0.0f) || (p.z > 0.0f && n.z > 0.0f))
            {
                // n already on the right side
            }
            else
                n = -n;

            sSprayPt pt;
            pt.p = p;
            pt.n = n;

            if (p.z < 0.0f)
                mLeft.push_back(pt);
            else if (p.z > 0.0f)
                mRight.push_back(pt);
        }
    }

    // Comparator to sort in ascending order of distance on the X axis from frontPoint
    auto compareNearToFarX = [frontPoint](const sSprayPt& a, const sSprayPt& b) {
        float distA = frontPoint.x - a.p.x;  // the "distance" in x from frontPoint
        float distB = frontPoint.x - b.p.x;
        return distA < distB;
        };

    std::sort(mLeft.begin(), mLeft.end(), compareNearToFarX);
    std::sort(mRight.begin(), mRight.end(), compareNearToFarX);

    FilterClosePoints(mLeft);
    FilterClosePoints(mRight);

    if (mLeft.size() > 1 && mRight.size() > 1)
    {
        mRandomOffsetRange = 0.0f;
        for (size_t i = 0; i < mLeft.size() - 1; ++i)
            mRandomOffsetRange += glm::length(mLeft[i + 1].p - mLeft[i].p);

        for (size_t i = 0; i < mRight.size() - 1; ++i)
            mRandomOffsetRange += glm::length(mRight[i + 1].p - mRight[i].p);

        mRandomOffsetRange /= (mLeft.size() - 1 + mRight.size() - 1);
        mRandomOffsetRange *= 0.5f;
    }
}

GLuint Ship::GetTraceID() 
{ 
#ifdef TRACE
    if (mTexTrace[mTraceIdx])
        return mTexTrace[mTraceIdx]; 
#endif
    return 0;
}
void Ship::InitTrace()
{
    mTexTrace = new unsigned int[2];
    glGenTextures(2, mTexTrace);
    for (unsigned int i = 0; i < 2; i++)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mTexTrace[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, TEX_SIZE, TEX_SIZE, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    mShaderTrace = make_unique<Shader>("", "", "", "Resources/Ship/trace.comp");
}
void Ship::UpdateTrace()
{
    static int prevX = ship.Position.x;
    static int prevZ = ship.Position.z;

    int dx = std::roundf(ship.Position.x) - prevX;
    int dz = std::roundf(ship.Position.z) - prevZ;

    prevX = std::roundf(ship.Position.x);
    prevZ = std::roundf(ship.Position.z);

    glBindImageTexture(0, mTexTrace[mTraceIdx], 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
    glBindImageTexture(1, mTexTrace[1 - mTraceIdx], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

    mShaderTrace->use();
    mShaderTrace->setInt("dx", dx);
    mShaderTrace->setInt("dz", dz);

    glDispatchCompute(512 / 16, 512 / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    mTraceIdx = 1 - mTraceIdx;
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

    float halfW = TexContourShipW / 2.0f;
    float halfH = TexContourShipH / 2.0f;

    vector<vec2> contour2D;
    for (const auto& v : contour)
        contour2D.emplace_back( v.x + halfW, v.z + halfH );
    
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
    if (TexContourShip)
    {
        glDeleteTextures(1, &TexContourShip);
        TexContourShip = 0;
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
    if (mVaoContour1)
    {
        glDeleteVertexArrays(1, &mVaoContour1);
        mVaoContour1 = 0;
        glDeleteBuffers(1, &mVboContour1);
        mVboContour1 = 0;
    }

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
    if (mVaoContour2)
    {
        glDeleteVertexArrays(1, &mVaoContour2);
        mVaoContour2 = 0;
        glDeleteBuffers(1, &mVboContour2);
        mVboContour2 = 0;
    }

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
bool InterpolateTriangle(const vec3& p1, const vec3& p2, const vec3& p3, vec3& pos)
{
    // Calculation of barycentric coordinates
    float det = ((p2.z - p3.z) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.z - p3.z));
    float w1 = ((p2.z - p3.z) * (pos.x - p3.x) + (p3.x - p2.x) * (pos.z - p3.z)) / det;
    float w2 = ((p3.z - p1.z) * (pos.x - p3.x) + (p1.x - p3.x) * (pos.z - p3.z)) / det;
    float w3 = 1.0f - w1 - w2;

    // Check if the point is inside the triangle
    if (w1 >= 0 && w2 >= 0 && w3 >= 0 && w1 <= 1 && w2 <= 1 && w3 <= 1)
    {
        // Interpolation of the y value
        pos.y = w1 * p1.y + w2 * p2.y + w3 * p3.y;
        return true;
    }
    else
    {
        // The point is outside the triangle
        return false;
    }
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
    World = glm::rotate(World, Yaw, vec3(0.0f, 1.0f, 0.0f));
    World = glm::rotate(World, Roll, vec3(1.0f, 0.0f, 0.0f));
    World = glm::rotate(World, Pitch, vec3(0.0f, 0.0f, 1.0f));
}
void Ship::ResetVelocities()
{
    Pitch = 0.0f;
    Roll = 0.0f;
    
    PitchCouple = 0.0f;
    RollCouple = 0.0f;
    PitchAcceleration = 0.0f;
    RollAcceleration = 0.0f;
    PitchVelocity = 0.0f;
    RollVelocity = 0.0f;
    HeaveAcceleration = 0.0f;
    HeaveVelocity = 0.0f;
    SurgeAcceleration = 0.0f;
    SurgeVelocity = 0.0f;
    YawAcceleration = 0.0f;
    YawVelocity = 0.0f;
    WindAcceleration = 0.0f;
    WindVelocity = 0.0f;
    LinearVelocity = 0.0f;
    DriftVelocity = 0.0f;
    DriftAngleDeg = 0.0f;
    Velocity = 0.0f;
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
void Ship::SetYawFromHDG(float hdg)
{
    float deg_Yaw = fmod(450.0f - hdg, 360.0f);
    if (deg_Yaw < 0.0f) 
        deg_Yaw += 360.0f;
    Yaw = glm::radians(deg_Yaw);
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
    glBindBuffer(GL_ARRAY_BUFFER, mVboHull);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mvVertexColored.size() * sizeof(float), mvVertexColored.data());
}
sBBvec3 Ship::GetBoundingBox()
{
    return mBbox;
}

// Creation of all the Kelvin wake textures 
constexpr int IMAGE_HEIGHT = 1024;
constexpr int IMAGE_WIDTH = 512;
constexpr int IMAGE_WIDTH_2SIDES = 2 * IMAGE_WIDTH;
constexpr float MAX_Y_TILDE = 10.0f;
constexpr float PI = 3.14159265358979323846f;
float pressure_field(float theta, float FR)
{
    // Adaptation de pressure_field

    float K0_inv = (FR * std::cos(theta)) * (FR * std::cos(theta));
    float denom = 2.0f * M_PI * K0_inv;
    float exponent = -1.0f / (denom * denom);
    return std::exp(exponent);
}
float fonction_a_integrer(float theta, float x_tilde, float y_tilde, float froude_nbr)
{
    // Fonction sous intégrale (fonction_a_integrer)

    float sin_num = 2.0f * M_PI * (std::cos(theta) * x_tilde - std::sin(theta) * y_tilde);
    float sin_den = std::pow(std::cos(theta), 2);
    float numerator = std::sin(sin_num / sin_den);
    float denominator = std::pow(std::cos(theta), 4);

    return pressure_field(theta, froude_nbr) * numerator / denominator;
}
float integrate(std::function<float(float)> f, float a, float b, int n = 1000)
{
    // Intégration numérique simple par méthode des trapèzes

    float h = (b - a) / n;
    float sum = 0.5f * (f(a) + f(b));
    for (int i = 1; i < n; ++i)
    {
        float x = a + i * h;
        sum += f(x);
    }
    return sum * h;
}
float surface_displacement(float x_tilde, float y_tilde, float froude_nbr)
{
    // Rotation 90 degrés
    std::swap(x_tilde, y_tilde);

    auto integrand = [&](float theta) {
        return fonction_a_integrer(theta, x_tilde, y_tilde, froude_nbr);
        };

    float val = integrate(integrand, -M_PI_2, M_PI_2, 1000);
    return -val;
}
void wake_simulation(float froude_nbr, vector<float>& buffer)
{
    // Calcul de la simulation entière dans un buffer 2D

    // Vous pouvez éventuellement gérer les masques, offsets, symétries ici
    buffer.resize(IMAGE_HEIGHT * IMAGE_WIDTH);

    float subpixel_nbr = IMAGE_HEIGHT / MAX_Y_TILDE;
    int y_offset = 0;// static_cast<int>(IMAGE_HEIGHT * 0.05f); // ex: offset vertical (-44, -22)

    int x1 = 20, y1 = 1024 - 973;
    int x2 = 160, y2 = 1024 - 1007;
    y_offset = y1 + (y2 - y1) * (100.0f * froude_nbr - x1) / (x2 - x1);
    y_offset = -y_offset;
    //cout << int(100.0f * froude_nbr) << "  " << y_offset << endl;

    for (int y_img = 0; y_img < IMAGE_HEIGHT; ++y_img)
    {
        for (int x_img = 0; x_img < IMAGE_WIDTH; ++x_img)
        {
            float x_tilde = x_img / subpixel_nbr;
            float y_tilde = (y_img - y_offset) / subpixel_nbr;
            buffer[y_img * IMAGE_WIDTH + x_img] = surface_displacement(x_tilde, y_tilde, froude_nbr);
        }
    }
}
void normalizeBuffer(vector<float>& buffer) 
{
    if (buffer.empty()) return;

    // Trouver min, max et moyenne actuelle
    float min_val = FLT_MAX;
    float max_val = FLT_MIN;
    double sum = 0.0;

    for (float v : buffer) 
    {
        if (v < min_val) min_val = v;
        if (v > max_val) max_val = v;
        sum += v;
    }

    float range = max_val - min_val;
    if (range == 0) 
    {
        // Toute valeur identique, on met à 0.5
        for (auto& v : buffer) v = 0.5f;
        return;
    }

    double mean = sum / buffer.size();

    // Étape 1 : normaliser entre 0 et 1
    for (auto& v : buffer) 
        v = (v - min_val) / range;

    // Étape 2 : ajuster pour que la moyenne soit à 0.5
    // Calculer la moyenne après normalisation
    double new_sum = 0.0;
    for (float v : buffer) 
    {
        new_sum += v;
    }

    double new_mean = new_sum / buffer.size();

    // Décalage nécessaire pour que la moyenne soit 0.5
    float offset = 0.5f - static_cast<float>(new_mean);

    for (auto& v : buffer) 
    {
        v += offset;
        // Clamper entre 0 et 1
        if (v < 0.0f) v = 0.0f;
        else if (v > 1.0f) v = 1.0f;
    }
}
void Ship::CreateKelvinImages()
{
    GLuint kelvinTex3;
    glGenTextures(1, &kelvinTex3);
    glBindTexture(GL_TEXTURE_2D, kelvinTex3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, IMAGE_WIDTH_2SIDES, IMAGE_HEIGHT, 0, GL_RED, GL_FLOAT, nullptr);
    
    for (int fr = 0; fr < 161; fr++)
    {
        vector<float> buffer;
        float froude = 0.01f * fr;
        wake_simulation(froude, buffer);
        normalizeBuffer(buffer);

        // Préparation d'un buffer complet avec les deux côtés
        vector<float> buffer_two_sides(IMAGE_HEIGHT * IMAGE_WIDTH_2SIDES);
        for (int y = 0; y < IMAGE_HEIGHT; ++y)
        {
            for (int x = 0; x < IMAGE_WIDTH_2SIDES; ++x)
            {
                if (x < IMAGE_WIDTH)
                {
                    // Partie miroir à gauche : miroir horizontal de la bordure gauche de l'image originelle
                    int mirror_x = IMAGE_WIDTH - 1 - x;
                    buffer_two_sides[y * IMAGE_WIDTH_2SIDES + x] = buffer[y * IMAGE_WIDTH + mirror_x];
                }
                else
                {
                    // Partie originale à droite, placée à partir de IMAGE_WIDTH
                    int original_x = x - IMAGE_WIDTH;
                    buffer_two_sides[y * IMAGE_WIDTH_2SIDES + x] = buffer[y * IMAGE_WIDTH + original_x];
                }
            }
        }

        string name;
        if (fr > 99)
            name = "Outputs/Kelvin-1024_Fr-" + to_string(fr) + ".png";
        else if (fr < 10)
            name = "Outputs/Kelvin-1024_Fr-00" + to_string(fr) + ".png";
        else
            name = "Outputs/Kelvin-1024_Fr-0" + to_string(fr) + ".png";

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, IMAGE_WIDTH_2SIDES, IMAGE_HEIGHT, 0, GL_RED, GL_FLOAT, buffer_two_sides.data());
        SaveTexture2D(kelvinTex3, IMAGE_WIDTH_2SIDES, IMAGE_HEIGHT, 1, GL_RED, name);
    }
}

// Update
void Ship::Update(float time)
{
    UpdateSounds();
    
    if (g_bPause)
        return;

    if (!bVisible)
        return;

    static float prevTime = 0.0f;
    float dt = time - prevTime;
    mDt = dt;

    UpdateWorldMatrix();

    // Preparation
    TransformVertices();
    // Computation
    GetHeightOfAllVertices();
    GetTrisUnderWater();
    // Forces
    ComputeArchimede();
    ComputeGravity();
    ComputeHeave();
    ComputeThrust1(dt);
    ComputeThrust2(dt);
    ComputePropellersDrag();
    ComputePropellersTorque();
    ComputeResistanceViscous();
    ComputeResistanceWaves();
    ComputeResistanceResidual();
    ComputeBowThrust(dt);
    ComputeSternThrust(dt);
    ComputeRudder(dt);
    ComputeWind();
    ComputeCentrifugal();
    // Result
    ComputeForces(dt);
   
    UpdateSmoke(dt);
    UpdateSpray(dt);
    UpdateAutopilot(dt);

    if (bPressure)
        UpdateVaoPressureLines();

    UpdateWakeVao();
    if(bTexWakeByVAO)
        UpdateTextureWakeVao();
    else
        UpdateWakeBuffer();

#ifdef TRACE
    UpdateTrace();
#endif

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
void Ship::ComputeHeave()
{
    // Force which resist to the couple Archimede / Gravity
    ResistanceHeave.Magnitude = ship.HeaveCoef * mMass * HeaveVelocity * AreaXZ_RacCub * AreaWetted / AreaWettedMax;
    if (Archimede.Magnitude > Gravity.Magnitude)    ResistanceHeave.Vector = TransformVector(vec3(0.0f, -ResistanceHeave.Magnitude, 0.0f));
    else                                            ResistanceHeave.Vector = TransformVector(vec3(0.0f, ResistanceHeave.Magnitude, 0.0f));
    ResistanceHeave.Position = Archimede.Position;
}
void Ship::ComputeThrust1(float dt)
{
    if (ship.nPropeller == 1)
        return;

    // Force as a result of the engine and the propellers

    float limitRpm1 = 0.0f;
    if (PowerCurrentStep1 >= 0) limitRpm1 = ((float)PowerCurrentStep1 / (float)ship.PowerStepMax) * ship.PropRpmMax;
    else                        limitRpm1 = ((float)PowerCurrentStep1 / (float)ship.PowerStepMax) * ship.PropRpmMax;

    if (PropRpm1 < limitRpm1 - ship.PropRpmIncrement * dt)
        PropRpm1 += ship.PropRpmIncrement * dt;
    else if (PropRpm1 > limitRpm1 + ship.PropRpmIncrement * dt)
        PropRpm1 -= ship.PropRpmIncrement * dt;

    if (PropRpm1 >= 0) PowerApplied1 = mPowerW * PropRpm1 / ship.PropRpmMax;
    else                PowerApplied1 = mPowerW * PropRpm1 / ship.PropRpmMax;

    // Stop to 0 if close to zero and target is zero
    if (PropRpm1 > -ship.PropRpmIncrement * dt && PropRpm1 < ship.PropRpmIncrement * dt)
    {
        PropRpm1 = 0.0f;
        PowerApplied1 = 0.0f;
    }

    Thrust1.Magnitude = PowerApplied1;
    Thrust1.Vector = TransformVector(vec3(Thrust1.Magnitude, 0.0f, 0.0f));
    Thrust1.Position = TransformPosition(ship.PosPropeller1);
}
void Ship::ComputeThrust2(float dt)
{
    // Force as a result of the engine and the propellers

    float limitRpm2 = 0.0f;
    if (PowerCurrentStep2 >= 0) limitRpm2 = ((float)PowerCurrentStep2 / (float)ship.PowerStepMax) * ship.PropRpmMax;
    else                        limitRpm2 = ((float)PowerCurrentStep2 / (float)ship.PowerStepMax) * ship.PropRpmMax;

    if (PropRpm2 < limitRpm2 - ship.PropRpmIncrement * dt)
        PropRpm2 += ship.PropRpmIncrement * dt;
    else if (PropRpm2 > limitRpm2 + ship.PropRpmIncrement * dt)
        PropRpm2 -= ship.PropRpmIncrement * dt;

    if (PropRpm2 >= 0) PowerApplied2 = mPowerW * PropRpm2 / ship.PropRpmMax;
    else                PowerApplied2 = mPowerW * PropRpm2 / ship.PropRpmMax;

    // Stop to 0 if close to zero and target is zero
    if (PropRpm2 > -ship.PropRpmIncrement * dt && PropRpm2 < ship.PropRpmIncrement * dt)
    {
        PropRpm2 = 0.0f;
        PowerApplied2 = 0.0f;
    }

    Thrust2.Magnitude = PowerApplied2;
    Thrust2.Vector = TransformVector(vec3(Thrust2.Magnitude, 0.0f, 0.0f));
    Thrust2.Position = TransformPosition(ship.PosPropeller2);
}
void Ship::ComputePropellersDrag()
{
    static float SurgeVelocityPrevious = 0.0f;
    static float S = M_PI * 0.25f * ship.PropDiameter * ship.PropDiameter;
    float Cx = 1.0f; // disc drag coefficient for blocked or dragging propeller

    // Propeller 1 ======================
    
    float drag1 = 0.0f;

    // Apply brake if current speed is positive and previous was higher (deceleration)
    if ((SurgeVelocity > 0.0f && SurgeVelocityPrevious > SurgeVelocity + 0.1f) || (SurgeVelocity < 0.0f && SurgeVelocityPrevious < SurgeVelocity - 0.1f))
    {
        drag1 = 0.5f * mWATER_DENSITY * Cx * S * SurgeVelocity * fabs(SurgeVelocity);
        drag1 = -copysign(drag1, SurgeVelocity);
    }
    // If opposite impulse, apply an additional brake according to the power
    else if ((SurgeVelocity > 0.0f && PropRpm1 < 0.0f) || (SurgeVelocity < 0.0f && PropRpm1 > 0.0f))
    {
        float max_drag = 0.5f * mWATER_DENSITY * Cx * S * SurgeVelocity * fabs(SurgeVelocity);
        if (fabs(PowerApplied1) < fabs(max_drag))
            drag1 = -copysign(max_drag - fabs(PowerApplied1), SurgeVelocity);
    }

    PropDrag1.Magnitude = drag1;
    PropDrag1.Vector = TransformVector(vec3(drag1, 0.0f, 0.0f));
    PropDrag1.Position = TransformPosition(ship.PosPropeller1);

    // Propeller 2 ======================
    
    float drag2 = 0.0f;

    if ((SurgeVelocity > 0.0f && SurgeVelocityPrevious > SurgeVelocity + 0.1f) || (SurgeVelocity < 0.0f && SurgeVelocityPrevious < SurgeVelocity - 0.1f))
    {
        drag2 = 0.5f * mWATER_DENSITY * Cx * S * SurgeVelocity * fabs(SurgeVelocity);
        drag2 = -copysign(drag2, SurgeVelocity);
    }
    else if ((SurgeVelocity > 0.0f && PropRpm2 < 0.0f) || (SurgeVelocity < 0.0f && PropRpm2 > 0.0f))
    {
        float max_drag = 0.5f * mWATER_DENSITY * Cx * S * SurgeVelocity * fabs(SurgeVelocity);
        if (fabs(PowerApplied2) < fabs(max_drag))
            drag2 = -copysign(max_drag - fabs(PowerApplied2), SurgeVelocity);
    }

    PropDrag2.Magnitude = drag2;
    PropDrag2.Vector = TransformVector(vec3(drag2, 0.0f, 0.0f));
    PropDrag2.Position = TransformPosition(ship.PosPropeller2);

    SurgeVelocityPrevious = SurgeVelocity;
}
void Ship::ComputePropellersTorque()
{
    // Propeller 1 ======================

    PropTorque1.Magnitude = 0.0f;
    if (ship.nPropeller == 2)
    {
        // Vitesse angulaire en rad/s
        float omega = (2.0f * M_PI * fabs(PropRpm1)) / 60.0f;

        if (omega > 1e-3f) // Pour éviter division par zéro
        {
            // Calcul du couple, signe inverse de la RPM car réaction
            float torqueMagnitude = fabs(PowerApplied1) / omega;
            float torqueSign = (PropRpm1 >= 0) ? 1.0f : -1.0f;

            PropTorque1.Magnitude = 20.0f * torqueSign * torqueMagnitude * ship.PropTorque1;
        }

        // Le vecteur torque autour de l'axe X longitudinal (ex : vecteur vers tribord au sens inverse)
        PropTorque1.Vector = TransformVector(vec3(0.0f, 0.0f, PropTorque1.Magnitude));
        PropTorque1.Position = TransformPosition(ship.PosPropeller1);
    }

    // Propeller 2 ======================
    
    PropTorque2.Magnitude = 0.0f;
    float omega = (2.0f * M_PI * fabs(PropRpm2)) / 60.0f;

    if (omega > 1e-3f) // Pour éviter division par zéro
    {
        // Calcul du couple, signe inverse de la RPM car réaction
        float torqueMagnitude = fabs(PowerApplied2) / omega;
        float torqueSign = (PropRpm2 >= 0) ? 1.0f : -1.0f;

        PropTorque2.Magnitude = 20.0f * torqueSign * torqueMagnitude * ship.PropTorque2;
    }

    // Le vecteur torque autour de l'axe X longitudinal (ex : vecteur vers tribord au sens inverse)
    PropTorque2.Vector = TransformVector(vec3(0.0f, 0.0f, PropTorque2.Magnitude));
    PropTorque2.Position = TransformPosition(ship.PosPropeller2);
}
void Ship::ComputeResistanceViscous() 
{
    // Reynolds number (ITTC-1957)
    float Re = (fabs(SurgeVelocity) * LWL) / mKINEMATIC_VISCOSITY;

    // Friction coefficient (ITTC-1957)
    float Cf = 0.075f / pow(log10(Re) - 2.0f, 2);

    // Form factor
    float k_form = 0.2f;
    float Cv = Cf * (1.0f + k_form);

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
void Ship::ComputeResistanceWaves()
{
    // Froude number
    float Fn = fabs(SurgeVelocity) / sqrt(mGRAVITY * LWL);
    //cout << fixed << setprecision(2) << SurgeVelocity << "   " << Fn << endl;
    
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

    if (SurgeVelocity < 0.0f)
        resWav *= 5.0f;

    ResistanceWaves.Magnitude = -Sign(SurgeVelocity) * resWav;   // Against the speed
    ResistanceWaves.Vector = TransformVector(vec3(ResistanceWaves.Magnitude, 0.0f, 0.0f));
    ResistanceWaves.Position = Archimede.Position;
}
void Ship::ComputeResistanceResidual()
{
    if (SurgeVelocity < -0.1f || SurgeVelocity > 0.1f)
    {
        ResistanceResidual.Magnitude = 0.0f;
        ResistanceResidual.Vector = TransformVector(vec3(ResistanceResidual.Magnitude, 0.0f, 0.0f));
        ResistanceResidual.Position = Archimede.Position;
        return;
    }

    // Frottement résiduel (toujours là, même à vitesse très faible)
    
    float hullFrictionCoeff = 0.4f;
    switch (ship.Class)
    {
    case eClass::FastBoat:      hullFrictionCoeff = 0.70f; break;
    case eClass::Corvette:      hullFrictionCoeff = 0.55f; break;
    case eClass::Frigate:       hullFrictionCoeff = 0.45f; break;
    case eClass::Fishing:       hullFrictionCoeff = 0.45f; break;
    case eClass::Submarine:     hullFrictionCoeff = 0.35f; break;
    case eClass::Ferry:         hullFrictionCoeff = 0.40f; break;
    case eClass::Tugboat:       hullFrictionCoeff = 0.35f; break;
    case eClass::Cargo:         hullFrictionCoeff = 0.35f; break;
    case eClass::Supertanker:   hullFrictionCoeff = 0.25f; break;
    }
    float residualFriction = mWATER_DENSITY * hullFrictionCoeff * SurgeVelocity * AreaWetted;    // proportionnel à V directement
    
    ResistanceResidual.Magnitude = -Sign(SurgeVelocity) * residualFriction;   // Against the speed
    ResistanceResidual.Vector = TransformVector(vec3(ResistanceResidual.Magnitude, 0.0f, 0.0f));
    ResistanceResidual.Position = Archimede.Position;
}
void Ship::ComputeBowThrust(float dt)
{
    if (!ship.HasBowThruster)
        return;

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
void Ship::ComputeSternThrust(float dt)
{
    if (!ship.HasSternThruster)
        return;

    // Force as a result of the stern thruster

    float limitRpm = 0.0f;
    if (SternThrusterCurrentStep >= 0)
        limitRpm = ship.SternThrusterRpmMin + ((float)SternThrusterCurrentStep / (float)ship.SternThrusterStepMax) * ((float)ship.SternThrusterRpmMax - (float)ship.SternThrusterRpmMin);
    else
        limitRpm = -ship.SternThrusterRpmMin + ((float)SternThrusterCurrentStep / (float)ship.SternThrusterStepMax) * ((float)ship.SternThrusterRpmMax - (float)ship.SternThrusterRpmMin);

    if (SternThrusterRpm < limitRpm - ship.SternThrusterRpmIncrement * dt)      SternThrusterRpm += ship.SternThrusterRpmIncrement * dt;
    else if (SternThrusterRpm > limitRpm + ship.SternThrusterRpmIncrement * dt) SternThrusterRpm -= ship.SternThrusterRpmIncrement * dt;

    if (SternThrusterRpm >= 0)    SternThrusterApplied = ship.SternThrusterPowerW * (SternThrusterRpm - ship.SternThrusterRpmMin) / (ship.SternThrusterRpmMax - ship.SternThrusterRpmMin);
    else                        SternThrusterApplied = ship.SternThrusterPowerW * (SternThrusterRpm + ship.SternThrusterRpmMin) / (ship.SternThrusterRpmMax - ship.SternThrusterRpmMin);

    // Stop to 0 if close to zero and target is zero
    if (SternThrusterRpm > -ship.SternThrusterRpmIncrement * dt && SternThrusterRpm < ship.SternThrusterRpmIncrement * dt)
        SternThrusterRpm = 0.0f;

    SternThrust.Magnitude = SternThrusterApplied * ship.SternThrusterPerf;
    SternThrust.Vector = TransformVector(vec3(0.0f, 0.0f, SternThrust.Magnitude));
    SternThrust.Position = TransformPosition(ship.PosSternThruster);

    mSoundSternThruster->setPitch(1.0f + 0.5f * fabs(SternThrusterApplied) / ship.SternThrusterPowerW);
    if (g_Camera.GetPosition().y < 0.0f)
        mSoundSternThruster->setVolume(0.01f + 0.25f * fabs(SternThrusterApplied) / ship.SternThrusterPowerW);
    else
        mSoundSternThruster->setVolume(0.25f + 0.25f * fabs(SternThrusterApplied) / ship.SternThrusterPowerW);
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

    // Force
    float force = 0.5f * mWATER_DENSITY * pow(SurgeVelocity, 2) * mRudderArea * sin(angle) * ship.TurningPerf;

    // Lateral component (gyration of the ship)
    RudderLift.Magnitude = Sign(SurgeVelocity) * force;
    RudderLift.Vector = TransformVector(vec3(0.0f, 0.0f, RudderLift.Magnitude));
    RudderLift.Position = TransformPosition(ship.PosRudder);

    // Axial component (slow the ship, function of the angle of the rudder)
    float a = 0.1f; // min factor at 0°
    float b = 1.0f; // max factor at 35°
    float n = 3.0f; // controls the shape of the curve (bigger = smoother)
    float x = fabs(RudderAngleDeg) / ship.RudderStepMax;
    if (x > 1.0f) x = 1.0f;
    float dragFactor = a + (b - a) * pow(x, n);
    RudderDrag.Magnitude = -Sign(SurgeVelocity) * fabs(force) * dragFactor;
    RudderDrag.Vector = TransformVector(vec3(RudderDrag.Magnitude, 0.0f, 0.0f));
    RudderDrag.Position = TransformPosition(ship.PosRudder);
}
void Ship::ComputeWind()
{
    // 2 forces, one on the forward half of the ship and the other on the aft half of the ship, 
    
    // both exerting a thrust that causes the ship to drift and a turning force that brings the ship across to windward

    // Angle between Wind and Heading (on horizontal plan XZ) (-0 to -180 = port wind, 0 to 180 = starboard wind)
    vec3 heading = TransformPosition(mBow) - ship.Position;
    vec2 windXZ = g_Wind + vCOG;
    vec2 headingXZ(heading.x, heading.z);
    windXZ = glm::normalize(windXZ);
    headingXZ = glm::normalize(headingXZ);
    float angleWind = atan2(windXZ.y, windXZ.x);
    float angleHeading = atan2(headingXZ.y, headingXZ.x);
    float angle = angleWind - angleHeading;
    angle = atan2(sin(angle), cos(angle));
  
    // Between 0° and 90°: alpha is positive (from 0 to 1 then goes back down to 0), between 90° and 180°: alpha is negative (from 0 to -1 then goes back up to 0)
    float alpha = sin(2 * angle); 
   
    float wind = glm::length(g_Wind);
    WindApparent = glm::length(g_Wind + vCOG);

    // WIND ROTATION
    WindRotation.Magnitude = 0.5f * mAIR_DENSITY * mPLATE_DRAG_COEFF * AreaLat * pow(wind, 2) * alpha;
    WindRotation.Vector = TransformVector(vec3(0.0f, 0.0f, WindRotation.Magnitude));
    WindRotation.Position = TransformPosition(ship.AreaLatCenter);

    // Angle decomposition
    const float cosPhi = std::cos(angle);
    const float sinPhi = std::sin(angle);
    const float absCosPhi = std::abs(cosPhi);
    const float absSinPhi = std::abs(sinPhi);
    
    // Directional factors
    const float frontFactor = (cosPhi >= 0.0f) ? 1.0f : 0.0f;
    const float rearFactor = (cosPhi <= 0.0f) ? 1.0f : 0.0f;

    // Projected areas
    float frontAreaFrontProj = AreaFront * absCosPhi * frontFactor;
    float rearAreaFrontProj = AreaFront * absCosPhi * rearFactor;
    float latProj = AreaLat * absSinPhi;

    float frontProjectedArea = 0.5f * latProj + frontAreaFrontProj;
    float rearProjectedArea = 0.5f * latProj + rearAreaFrontProj;

    // Forces
    vec3 windNormalized = -glm::normalize(vec3(g_Wind.x, 0.0f, g_Wind.y));
    WindFront.Magnitude = 0.5f * mAIR_DENSITY * mPLATE_DRAG_COEFF * frontProjectedArea * wind * wind * 0.5f;
    WindFront.Vector = WindFront.Magnitude * windNormalized;
    WindFront.Position = TransformPosition(vec3((mBow.x + AreaLatCenter.x) * 0.5f, AreaLatCenter.y, 0.0f));

    WindRear.Magnitude = 0.5f * mAIR_DENSITY * mPLATE_DRAG_COEFF * rearProjectedArea * wind * wind * 0.5f;
    WindRear.Vector = WindRear.Magnitude * windNormalized;
    WindRear.Position = TransformPosition(vec3((mStern.x + AreaLatCenter.x) * 0.5f, AreaLatCenter.y, 0.0f));

    ResistanceAir.Magnitude = -(0.5f * mAIR_DENSITY * mPLATE_DRAG_COEFF * frontProjectedArea + 0.5f * mAIR_DENSITY * mPLATE_DRAG_COEFF * rearProjectedArea) * WindApparent * WindApparent * 0.5f * cos(angle);
    ResistanceAir.Magnitude *= 0.1f;
    vec3 windAppNormalized = -glm::normalize(vec3(g_Wind.x + vCOG.x, 0.0f, g_Wind.y + vCOG.y));
    ResistanceAir.Vector = ResistanceAir.Magnitude * windAppNormalized;
    ResistanceAir.Position = TransformPosition(AreaLatCenter);
}
void Ship::ComputeCentrifugal()
{
    // Centrifugal force during a turn that causes the ship to roll outward

    Centrifugal.Magnitude = mMass * fabs(YawVelocity) * ship.CentrifugalPerf;
    Centrifugal.Vector = TransformVector(vec3(0.0f, 0.0f, Centrifugal.Magnitude));
    Centrifugal.Position = TransformPosition(ship.PosGravity);
}
float Ship::ComputePivotPosition()
{
    // Return a distance from the center, positive if forward, negative if backward
    
    // Max speed conversion (en m/s)
    static float maxSpeed = KnotsToMS(ship.SpeedMaxKn);

    float P0 = 0.5f * mLength;                      // Pivot point at standstill (center)
    float alpha = 0.2f;                             // Climb parameter

    float pivotPos = P0;

    if (SurgeVelocity >= 0.0f)
    {
        // Positive speed case (forward)
        float PmaxFwd = ship.PivotFwd * mLength;
        pivotPos = P0 + (PmaxFwd - P0) * (1.0f - std::exp(-alpha * SurgeVelocity));
        if (pivotPos < PmaxFwd)
            pivotPos = PmaxFwd;
        if (pivotPos > P0)
            pivotPos = P0;
        pivotPos = 0.5f * mLength - pivotPos;
    }
    else
    {
        // Negative speed case (rear)
        float PmaxBwd = ship.PivotBwd * mLength;
        pivotPos = P0 + (PmaxBwd - P0) * (1.0f - std::exp(-alpha * -SurgeVelocity));
        if (pivotPos > PmaxBwd)
            pivotPos = PmaxBwd;
        if (pivotPos < P0)
            pivotPos = P0;
        pivotPos = 0.5f * mLength - pivotPos;
    }

    return pivotPos;
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

    // CoupleZ (tangage) (AG.x positive = the bow lowers)
    PitchAcceleration = 0.5f * PitchCouple * force / Izz;
    float asymmetryFactor = (PitchAcceleration > 0.0f) ? 1.0f : 0.3f;
    PitchAcceleration -= PitchVelocity * asymmetryFactor;
    PitchVelocity += PitchAcceleration * dt;
    Pitch += PitchVelocity * dt;

    // CoupleX (roulis) (AG.z positive = list to starboard)
    RollAcceleration = 0.25f * -RollCouple * force / Ixx;
    if (SurgeVelocity != 0.0f)
        RollAcceleration += Sign(YawVelocity) * Centrifugal.Magnitude / Ixx;
    RollAcceleration -= RollVelocity;
    RollVelocity += RollAcceleration * dt;
    Roll += RollVelocity * dt;

    // Normalization of Pitch and Roll Angles
    Pitch = fmod(Pitch, 2.0f * M_PI);
    Roll = fmod(Roll, 2.0f * M_PI);

    // Propulsive force
    float forceFWD = Thrust1.Magnitude + Thrust2.Magnitude
        + PropDrag1.Magnitude + PropDrag2.Magnitude
        + ResistanceViscous.Magnitude
        + ResistanceWaves.Magnitude
        + ResistanceResidual.Magnitude
        + ResistanceAir.Magnitude;
        + RudderDrag.Magnitude;

    // Mass of the ship is multiplied by 1.5 to simulate inertia (added displaced water)
	SurgeAcceleration = forceFWD / mMass;  
    
    // Drift + Yaw drag ==================
    
    // Drift factor (lateral drift)
    float driftFactor = pow(std::abs(std::sin(glm::radians(DriftAngleDeg))), 2.0f);

    // Pivot effect
    float pivot = ComputePivotPosition();
    float pivotFactorDrag = fabs(pivot) / (0.5f * mLength);

    // Characteristic speed related to critical Froude
    float Fr_crit = 0.25f; // 0.2 (large cargo ship), 0.3 (light ship), 0.35 (fast)
    switch (ship.Class)
    {
    case eClass::FastBoat:      Fr_crit = 0.38f; break;
    case eClass::Corvette:      Fr_crit = 0.32f; break;
    case eClass::Frigate:       Fr_crit = 0.30f; break;
    case eClass::Fishing:       Fr_crit = 0.30f; break;
    case eClass::Submarine:     Fr_crit = 0.25f; break;
    case eClass::Ferry:         Fr_crit = 0.25f; break;
    case eClass::Tugboat:       Fr_crit = 0.24f; break;
    case eClass::Cargo:         Fr_crit = 0.22f; break;
    case eClass::Supertanker:   Fr_crit = 0.20f; break;
    }
    float Vc = Fr_crit * sqrt(mGRAVITY * mLength);

    // Quadratic weight (increases with speed)
    float alpha = SurgeVelocity / (SurgeVelocity + Vc);

    // Base coefficients
    float baseLinCoef = 6000.0f * ship.TurningDragCoef;
    float baseQuadCoef = 2000.0f * ship.TurningDragCoef;

    // Interpolated coefficients : blend lin ↔ quad
    float effLinCoef = (1.0f - alpha) * baseLinCoef;
    float effQuadCoef = alpha * baseQuadCoef;
    float coeff = effLinCoef * SurgeVelocity + effQuadCoef * SurgeVelocity * SurgeVelocity;
  
    // Drift: combination of flax and dynamic quad
    float driftDrag = coeff * driftFactor * (mLength * pivotFactorDrag);
    if (driftDrag < 0.0001f) driftDrag = 0.0f;

    // Pure yaw (yaw drag)
    float yawDrag = coeff * 0.5f * fabs(YawVelocity) * (mLength * pivotFactorDrag);
    if (yawDrag < 0.0001f) yawDrag = 0.0f;

    // Final effect
    SurgeAcceleration -= (driftDrag + yawDrag) / mMass;

    // Slam and Surge ==================
    if (AreaWetted > 0.0f)
    {
        // Slam decreases the speed
        if (SurgeAcceleration > 0.0f && PitchVelocity > 0.0f)
            SurgeAcceleration -= 0.1f * PitchVelocity;

        // Surge: Positive pitch decreases the speed, negative pitch increases the speed
        SurgeAcceleration -= 0.1f * (AreaWetted / AreaWettedMax) * mGRAVITY * sin(Pitch);
    }

    // Yaw management ==================
    YawAcceleration = 0.0f;

    // -- Thrusters --
    if (ship.HasBowThruster)    
        YawAcceleration -= BowThrust.Magnitude * fabs(ship.PosBowThruster.x) / Iyy;
    if (ship.HasSternThruster)  
        YawAcceleration += SternThrust.Magnitude * fabs(ship.PosSternThruster.x) / Iyy;

    // -- Rudder --
    YawAcceleration += RudderLift.Magnitude * (fabs(pivot - ship.PosRudder.x)) / Iyy;
    
    // -- Propellers --
    if (ship.nPropeller == 2)
    {
        // 2 Propellers rotation
        float couplePropellers = -(Thrust1.Magnitude - Thrust2.Magnitude) * glm::length(ship.PosPropeller1 - ship.PosPropeller2);
        YawAcceleration += 5.0f * couplePropellers / Iyy;
        // Propeller torque
        YawAcceleration += PropTorque1.Magnitude / Iyy;
    }
    YawAcceleration += PropTorque2.Magnitude / Iyy;
    
    // -- Wind --
    YawAcceleration += WindRotation.Magnitude * (mLength * 0.5f) / Iyy;
    
    // -- Roll→yaw coupling --
    YawAcceleration -= 0.2f * Roll;

    // -- Hydrodynamic directional stability --
    float yawStability = 0.05f * SurgeVelocity * SurgeVelocity / mLength;
    YawAcceleration -= (YawVelocity + yawStability * YawVelocity);

    // -- Influence of the pivot --
    float pivotEffect = 1.0f - 0.5f * pivotFactorDrag;  // Less pivot (close to the midship) = more pivotEffect
    YawAcceleration *= pivotEffect;

    // -- Integration --
    YawVelocity += YawAcceleration * dt;

    // -- Physical limit of the rate of turn --
    float velocityAbs = fabs(SurgeVelocity);
    // Values for high speed (low rate of turn)
    const float speedHigh = ship.SpeedTestKn * 0.5144f;
    // Values for low speed (high rate of turn)
    const float speedLow = 0.5f * speedHigh;
    const float yawVelocityMax = 1.0f;
    // Smooth interpolation
    float t = (SurgeVelocity - speedLow) / (speedHigh - speedLow);
    t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t; // clamp between 0 et 1
    float s = t * t * (3.0f - 2.0f * t);
    float limitFactor = (1.0f - s) * yawVelocityMax + s * ship.HighSpeedCoeff;
    float rateOfTurn_rad_s = limitFactor * ship.RoTMax * (M_PI / 180.0f) / 60.0f;
    
    TurnDiameter_m = 2.0f * fabs(SurgeVelocity / 0.5144f) * 1852.0f / (fabs(YawVelocity) * (180.0f / M_PI) * 60.0f);
    TurnDiameter_L = TurnDiameter_m / ship.Length;

    if (fabs(YawVelocity) > rateOfTurn_rad_s)
        YawVelocity = rateOfTurn_rad_s * Sign(YawVelocity);

    Yaw += YawVelocity * dt;

    // New position
    mat4 rotationMatrix = glm::rotate(mat4(1.0f), Yaw, vec3(0.0f, 1.0f, 0.0f));
    vec3 xA(1.0f, 0.0f, 0.0f);
    vec3 forward = vec3(rotationMatrix * vec4(xA, 0.0f));
    SurgeVelocity += SurgeAcceleration * dt;
    ship.Position += forward * SurgeVelocity * dt;

    // Displacements (Bow & Stern thrusters, Rudder lift)
    float dLat = 0.0f;
    if (ship.HasBowThruster && BowThrust.Magnitude != 0.0f)
        dLat += ship.PosBowThruster.x * Sign(BowThrust.Magnitude) * 0.01f * dt;
    if (ship.HasSternThruster && SternThrust.Magnitude != 0.0f)
        dLat += -ship.PosSternThruster.x * Sign(SternThrust.Magnitude) * 0.01f * dt;
    if (RudderLift.Magnitude != 0.0f)
        dLat += pivot * YawVelocity * dt;

    vec3 zA(0.0f, 0.0f, 1.0f);
    vec3 perpend = vec3(rotationMatrix * vec4(zA, 0.0f));
    ship.Position += perpend * dLat;

    // Wind drift
    float forceDrift = 0.005f * (WindFront.Magnitude + WindRear.Magnitude);
    WindAcceleration = forceDrift / (0.5f * mLength * mLength) - 0.1 * WindVelocity;
    WindVelocity += WindAcceleration * dt;
    ship.Position += glm::normalize(-vec3(g_Wind.x, 0.0f, g_Wind.y)) * WindVelocity * dt;

    /////////////////////////

    // Velocities
    vec2 deltaPos = vec2(ship.Position.x, ship.Position.z) - prevPosition;
    LinearVelocity = dot(normalize(vec2(forward.x, forward.z)), deltaPos) / dt;
    DriftVelocity = dot(normalize(vec2(perpend.x, perpend.z)), deltaPos) / dt;
    vCOG = deltaPos / dt;
    Velocity = glm::length(vCOG);

    // HDG, COG, SOG
    HDG = fmod(450.0f - glm::degrees(Yaw), 360.0f);
    while (HDG < 0.0f)      HDG += 360.0f;
    while (HDG > 360.0f)    HDG -= 360.0f;

    vec2 dPos = vec2(ship.Position.x, ship.Position.z) - prevPosition;
    if (dt != 0.0f)
        SOG = MsToKnots(glm::length(dPos) / dt);

    if (glm::length(dPos) == 0.0f)  COG = HDG;
    else                            COG = glm::degrees(std::atan2(dPos.y, dPos.x)) + 90;
    while (COG < 0.0f)      COG += 360.0f;
    while (COG > 360.0f)    COG -= 360.0f;

    DriftAngleDeg = DifferenceDEG(HDG, COG);

	// SOG at the bow and the stern
    mat4 world = mat4(1.0f);
    world = glm::translate(world, ship.Position);
    world = glm::rotate(world, Yaw, vec3(0.0f, 1.0f, 0.0f));
    world = glm::rotate(world, Roll, vec3(1.0f, 0.0f, 0.0f));
    world = glm::rotate(world, Pitch, vec3(0.0f, 0.0f, 1.0f));

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

    /////////////////////////////////////////////
    
    auto fillResultData = [](const sForce& force) -> sResultData
        {
            sResultData result;
            result.variable = force.Name;
            result.value = static_cast<double>(force.Magnitude);
            result.decimal = force.Decimal;
            result.unit = force.Unit;
            return result;
        };
    
    if (g_bShowShipForcesWindows)
    {
        vForcesData.clear();
        vForcesData.push_back(fillResultData(Archimede));
        vForcesData.push_back(fillResultData(Gravity));
        vForcesData.push_back(fillResultData(ResistanceHeave));
        if (ship.nPropeller == 2) vForcesData.push_back(fillResultData(Thrust1));
        vForcesData.push_back(fillResultData(Thrust2));
        if (ship.nPropeller == 2) vForcesData.push_back(fillResultData(PropDrag1));
        vForcesData.push_back(fillResultData(PropDrag2));
        if (ship.nPropeller == 2) vForcesData.push_back(fillResultData(PropTorque1));
        vForcesData.push_back(fillResultData(PropTorque2));
        vForcesData.push_back(fillResultData(ResistanceViscous));
        vForcesData.push_back(fillResultData(ResistanceWaves));
        vForcesData.push_back(fillResultData(ResistanceResidual));
        if (ship.HasBowThruster) vForcesData.push_back(fillResultData(BowThrust));
        if (ship.HasSternThruster) vForcesData.push_back(fillResultData(SternThrust));
        vForcesData.push_back(fillResultData(RudderLift));
        vForcesData.push_back(fillResultData(RudderDrag));
        sResultData rd;
        rd.unit = "N";
        rd.decimal = 3;
        rd.variable = "DriftDrag";
        rd.value = static_cast<double>(driftDrag);
        vForcesData.push_back(rd);
        rd.variable = "YawDrag";
        rd.value = static_cast<double>(yawDrag);
        vForcesData.push_back(rd);
        vForcesData.push_back(fillResultData(WindRotation));
        vForcesData.push_back(fillResultData(WindFront));
        vForcesData.push_back(fillResultData(WindRear));
        vForcesData.push_back(fillResultData(ResistanceAir));
        vForcesData.push_back(fillResultData(Centrifugal));
    }
}
void Ship::UpdateAutopilot(float dt)
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
    ship.DynamicFactor        Factor to adjust the influence of speed
    ship.MinSpeed           Minimum speed to avoid division by zero
    ship.LowSpeedBoost      Low speed amplification factor
    ship.SeaSateFactor      Increases responsiveness in difficult conditions (Pitch and Roll)
    */

    // Calculate the heading error (the shortest difference between headings)
    float error = std::fmod(HDGInstruction - HDG + 540.0f, 360.0f) - 180.0f;

    // Reverse the error to correct in the right direction
    error = -error;

    auto adjustGain = [&](float baseGain, float shipSpeed) {
        float speedFactor = 1.0f / (1.0f + shipSpeed / ship.DynamicFactor);
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

    // Adjust the rudder angle according to the low speed
    float adjustedSpeed = std::max(SurgeVelocity, ship.MinSpeed);
    float speedFactor = (ship.DynamicFactor / adjustedSpeed) * ship.LowSpeedBoost;
    speedFactor = std::min(speedFactor, ship.LowSpeedBoost); // Limit the maximum factor
    rudderAngle *= speedFactor;

    // Adjust the rudder angle according to the high speed
    float highSpeedLimitFactor = 1.0f; 
    float maxReduction = 0.01f; // réduit l'angle au minimum à 5% à très haute vitesse

    if (SurgeVelocity <= ship.HighSpeedLimit) 
        highSpeedLimitFactor = 1.0f;
    else 
    {
        // Exponentielle décroissante lissée : 
        // Valeur à limitStart = 1, diminue vers maxReduction à haute vitesse
        float excess = SurgeVelocity - ship.HighSpeedLimit;
        highSpeedLimitFactor = std::max(maxReduction, std::exp(-0.3f * excess));
    }

    // application
    rudderAngle *= highSpeedLimitFactor;
    // Limit the rudder angle to the maximum allowed
    rudderAngle = std::clamp(rudderAngle, -(float)ship.RudderStepMax, (float)ship.RudderStepMax);
    RudderCurrentStep = rudderAngle;

    // Update last error for next calculation
    lastError = error;
}
void Ship::UpdateVaoPressureLines()
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
void Ship::UpdateSounds()
{   
    bool sound = bSound && g_SoundMgr->bSound && bVisible && !g_bPause;
    
    // Power 1
    mSoundThrust1->setPitch(2.0f + 0.25f * fabs(PowerApplied1) / mPowerW);
    if (g_Camera.GetPosition().y < 0.0f)
        mSoundThrust1->setVolume(0.02f + 0.25f * fabs(PowerApplied1) / mPowerW);
    else
        mSoundThrust1->setVolume(0.25f + 0.25f * fabs(PowerApplied1) / mPowerW);
    mSoundThrust1->setPosition(TransformPosition(ship.PosPropeller1));

    // Power 2
    mSoundThrust2->setPitch(2.0f + 0.25f * fabs(PowerApplied2) / mPowerW);
    if (g_Camera.GetPosition().y < 0.0f)
        mSoundThrust2->setVolume(0.02f + 0.25f * fabs(PowerApplied2) / mPowerW);
    else
        mSoundThrust2->setVolume(0.25f + 0.25f * fabs(PowerApplied2) / mPowerW);
    mSoundThrust2->setPosition(TransformPosition(ship.PosPropeller2));

    // Bow thruster
    if (ship.HasBowThruster)
    {
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
    }

    // Stern thruster
    if (ship.HasSternThruster)
    {
        mSoundSternThruster->setPosition(TransformPosition(ship.PosSternThruster));
        if (!mbSoundSternThrusterPlaying)
        {
            if (SternThrusterRpm != ship.SternThrusterRpmMin)
            {
                alGetError(); // clear error state
                mSoundSternThruster->play();
                ALenum error;
                if ((error = alGetError()) != AL_NO_ERROR)
                    cout << "alSourcef 0 AL_PITCH : " << error << endl;
                mbSoundSternThrusterPlaying = true;
            }
        }
        else
        {
            if (SternThrusterRpm == ship.SternThrusterRpmMin)
            {
                mSoundSternThruster->pause();
                mbSoundSternThrusterPlaying = false;
            }
        }
    }

    static bool bPause = false;
    if (!sound)
    {
        mSoundThrust1->pause();
        mSoundThrust2->pause();
        if (ship.HasBowThruster)
            mSoundBowThruster->pause();
        if (ship.HasSternThruster)
            mSoundSternThruster->pause();
        bPause = true;
    }
    if (sound && bPause)
    {
        mSoundThrust1->play();
        mSoundThrust2->play();
        if (ship.HasBowThruster && BowThrusterRpm != ship.BowThrusterRpmMin)
            mSoundBowThruster->play();
        if (ship.HasSternThruster && SternThrusterRpm != ship.SternThrusterRpmMin)
            mSoundSternThruster->play();
        bPause = false;
    }
}
void Ship::UpdateSmoke(float dt)
{
    if (!bVisible)
        return;
    
    if (!bSmoke)
        return;

    if (ship.nChimney == 0)
        return;

#ifdef DEBUG_SMOKE
    // Reset the counter
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO_LIVE_COUNTER);
    int zero = 0;
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &zero);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    // Bind buffer to binding point 1 before dispatch
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, SSBO_LIVE_COUNTER);
#endif

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mSSBO_SMOKE);

    vec3 windDirection = 0.25f * vec3(-g_Wind.x, 0.1f * g_WindSpeedKN, -g_Wind.y);

    mShaderSmokeCompute->use();
    mShaderSmokeCompute->setFloat("dt", dt);
    mShaderSmokeCompute->setInt("particlesPerFrame", 3);
    vec3 p = TransformPosition(ship.PosChimney1);
    mShaderSmokeCompute->setVec3("emitPositions[0]", p);
    if (ship.nChimney == 2)
    {
        p = TransformPosition(ship.PosChimney2);
        mShaderSmokeCompute->setVec3("emitPositions[1]", p);
    }
    mShaderSmokeCompute->setInt("numEmitters", ship.nChimney);
    mShaderSmokeCompute->setVec3("windDirection", windDirection);
    mShaderSmokeCompute->setInt("frameCount", mFrameSmokeCount);
    mShaderSmokeCompute->setFloat("shortLife", 5.0f);
    mShaderSmokeCompute->setFloat("longLife", 10.0f);
    // Dispatch compute shader (dividing mSmokeMaxParticles by local_size_x)
    glDispatchCompute((mSmokeMaxParticles + 255) / 256, 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // guarantee write before read

#ifdef DEBUG_SMOKE
    // Reading after dispatch
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO_LIVE_COUNTER);
    int* counterValue = (int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), GL_MAP_READ_BIT);
    if (counterValue) {
        int liveParticlesCount = *counterValue;
        cout << "Number of living particles: " << liveParticlesCount << std::endl;
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
#endif

    mFrameSmokeCount++;
}
void Ship::UpdateSpray(float dt)
{
    if (!bVisible)
        return;
    
    if (!bSpray)
        return;

    // Emits particules of spray from points distributed on the contour
    
    size_t leftCount = mLeft.size();
    size_t rightCount = mRight.size();

    if (leftCount < 2 || rightCount < 2)
        return;

    float intensity1, intensity2;

    // Lambda function to emit multiple interpolated particles between two points
    auto EmitInterpolatedSpray = [&](const sSprayPt& pt1, const sSprayPt& pt2, float intensity1, float intensity2)
        {
            for (int j = 0; j <= ship.SprayMultiplier; ++j)
            {
                float t = float(j) / float(ship.SprayMultiplier);
                // Local position interpolation
                vec3 interpPos = pt1.p * (1.0f - t) + pt2.p * t;

                vec3 emitPosWorld = TransformPosition(interpPos);
                GetHeightFast(emitPosWorld);
                
                // Added random offset to start position only
                vec3 randomOffset(
                    ((float)rand() / RAND_MAX - 0.5f) * 2.0f * mRandomOffsetRange,
                    ((float)rand() / RAND_MAX - 0.5f) * 2.0f * mRandomOffsetRange,
                    ((float)rand() / RAND_MAX - 0.5f) * 2.0f * mRandomOffsetRange
                );
                emitPosWorld += randomOffset;

                // Vertical interpolation factor
                float intensity = intensity1 * (1.0f - t) + intensity2 * t;

                // Normal interpolation and velocity calculation
                vec3 interpNormal = pt1.n * (1.0f - t) + pt2.n * t;

                vec3 velocity = TransformVector(interpNormal) * intensity;
                velocity.y += intensity + 2.0f * ship.SprayVerticalPerf * PitchVelocity;
                velocity.x += 0.01f * vCOG.x;
                velocity.z += 0.01f * vCOG.y;
                velocity *= Velocity * 0.5f;
                if (intensity > 0.0f)
                    mSpray->Emit(emitPosWorld, velocity);
            }
        };

    // Emission on the left with interpolation between points
    for (size_t i = 0; i < leftCount - 1; ++i)
    {
        intensity1 = 1.0f - float(i) / float(leftCount - 1);
        intensity2 = 1.0f - float(i + 1) / float(leftCount - 1);
        
        if (ship.SprayType == 1)
        {
            // Using a sine in [0, pi/2] to vary from 1.0 to 0
            intensity1 = sinf(1.57079632679f * intensity1);
            intensity2 = sinf(1.57079632679f * intensity2);
        }

        EmitInterpolatedSpray(mLeft[i], mLeft[i + 1], intensity1, intensity2);
    }
    mSpray->Update(dt);

    // Emission on the right with interpolation between points
    for (size_t i = 0; i < rightCount - 1; ++i)
    {
        intensity1 = 1.0f - float(i) / float(rightCount - 1);
        intensity2 = 1.0f - float(i + 1) / float(rightCount - 1);
        
        if (ship.SprayType == 1)
        {
            // Using a sine in [0, pi/2] to vary from 1.0 to 0
            intensity1 = sinf(1.57079632679f * intensity1);
            intensity2 = sinf(1.57079632679f * intensity2);
        }
        
        EmitInterpolatedSpray(mRight[i], mRight[i + 1], intensity1, intensity2);
    }
    mSpray->Update(dt);

    //cout << mSpray->GetNbActiveParticles() << endl;
}
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
        sfp.pos.y = 1.0f;
        sfp.time = glfwGetTime();
        vWakePoints.push_back(sfp);
        // Cleaning to not exceed a limit
        if (vWakePoints.size() > 500) vWakePoints.erase(vWakePoints.begin());
        compteurSillage = 0;
    }
    
    // Temporarily adds the current position
    sFoamPts sfp;
    sfp.pos = TransformPosition(mWakePivot);
    sfp.pos.y = 1.0f;
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
    
    // 3 trails (left and right with foam and center without foam)
    for (size_t i = 0; i + 1 < n; ++i)
    {
        float v0 = uv_v, v1 = uv_v + dv; 
        
        float alpha0 = calcAlpha(vWakePoints[i].time, now);
        float alpha1 = calcAlpha(vWakePoints[i + 1].time, now);

        vec3 center0 = vWakePoints[i].pos;       // centre actuel
        vec3 center1 = vWakePoints[i + 1].pos;   // centre suivant

        // Triangle gauche-centre
        vWakeVertices.push_back({ sideLeft[i],  {0, v0}, alpha0 });
        vWakeVertices.push_back({ center0,      {0.5, v0}, 0.0f }); // centre sans mousse, alpha 0
        vWakeVertices.push_back({ sideLeft[i + 1],  {0, v1}, alpha1 });

        // Triangle centre-gauche (suite)
        vWakeVertices.push_back({ sideLeft[i + 1],  {0, v1}, alpha1 });
        vWakeVertices.push_back({ center0,      {0.5, v0}, 0.0f });
        vWakeVertices.push_back({ center1,      {0.5, v1}, 0.0f });

        // Triangle centre-droite
        vWakeVertices.push_back({ center0,      {0.5, v0}, 0.0f });
        vWakeVertices.push_back({ sideRight[i], {1, v0}, alpha0 });
        vWakeVertices.push_back({ center1,      {0.5, v1}, 0.0f });

        // Triangle droite-centre (suite)
        vWakeVertices.push_back({ center1,      {0.5, v1}, 0.0f });
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

    mShaderWakeVaoToTex->use();     // Ship/wake_vao.vert, Ship/wake_vao.frag
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

    mShaderGaussH->use();       // Ship/wake_gauss.vert, Ship/wake_gauss_h.frag
    mShaderGaussH->setSampler2D("tex", TexBlit, 0);
    mShaderGaussH->setFloat("texelWidth", 1.0f / (float)TexWakeVaoSize);
    mScreenQuadWakeBuffer->Render();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Gauss pass (horizontal)
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_GAUSS2);
    glViewport(0, 0, TexWakeVaoSize, TexWakeVaoSize);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    mShaderGaussV->use();       // Ship/wake_gauss.vert, Ship/wake_gauss_v.frag
    mShaderGaussV->setSampler2D("tex", mTexGauss1, 0);
    mShaderGaussV->setFloat("texelHeight", 1.0f / (float)TexWakeVaoSize);
    mScreenQuadWakeBuffer->Render();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    mScreenQuadWakeBuffer->Render();  // -> TexWakeVao
}

// NMEA

unsigned char calculate_checksum(const std::string& sentence) 
{
    // Calcul du checksum NMEA XOR sur la chaîne entre $ et *
    unsigned char checksum = 0;
    for (size_t i = 1; i < sentence.size(); ++i) 
    {
        if (sentence[i] == '*') break;
        checksum ^= sentence[i];
    }
    return checksum;
}
string format_latitude(double latitude) 
{
    // Formatage de la latitude/direction N/S
    char dir = (latitude >= 0) ? 'N' : 'S';
    latitude = std::abs(latitude);
    int deg = static_cast<int>(latitude);
    double min = (latitude - deg) * 60;
    ostringstream ss;
    ss << std::setfill('0') << std::setw(2) << deg << std::fixed << std::setprecision(4) << std::setw(7) << min << "," << dir;
    return ss.str();
}
string format_longitude(double longitude) 
{
    // Formatage de la longitude/direction E/W
    char dir = (longitude >= 0) ? 'E' : 'W';
    longitude = std::abs(longitude);
    int deg = static_cast<int>(longitude);
    double min = (longitude - deg) * 60;
    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(3) << deg << std::fixed << std::setprecision(4) << std::setw(7) << min << "," << dir;
    return ss.str();
}
string Ship::NMEA_RMC()
{
    // Exemple de données à envoyer
    string time_utc = "123519"; // hhmmss
    char status = 'A'; // A = valide, V = invalide
    vec2 p = OpenGLToLonLat(ship.Position.x, ship.Position.z);
    double longitude = p.x; 
    double latitude = p.y;  
    double speed_knots = MsToKnots(SurgeVelocity);
    double course = HDG;
    string date = "230394"; // ddmmyy

    // Construction de la phrase sans le checksum ni $
    ostringstream sentence;
    sentence << "GPRMC,"
        << time_utc << ","
        << status << ","
        << format_latitude(latitude) << ","
        << format_longitude(longitude) << ","
        << std::fixed << std::setprecision(1) << speed_knots << ","
        << course << "," << date << ",,,A";

    // Construire la phrase complète avec $ et checksum *
    string full_sentence = "$" + sentence.str();
    unsigned char checksum = calculate_checksum(full_sentence);
    std::ostringstream final_sentence;
    final_sentence << full_sentence << "*"
        << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
        << (int)checksum << "\r\n";

    cout << final_sentence.str();
    return final_sentence.str();
}

// Render
void Ship::RenderForceRefBody(Camera& camera, vec3 P, vec3 V, float scale, vec3 color, bool bRenderOrigin)
{
    // P & V are in the starting position

    // Transformations in World
    float longueur = scale * glm::length(V);
    mat4 model(1.0f);
    model = glm::translate(model, vec3(longueur, 0, 0));
    model = glm::scale(model, vec3(longueur, 0.1f, 0.1f));
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
    model = glm::scale(model, vec3(longueur, 0.1f, 0.1f));
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

    mShaderWakeVao->use();    // Misc/unicolor.vert, Misc/unicolor.frag
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
    if (Rendering == eRendering::TRIANGLES)
        return;
    
    mShaderNavLight->use();     // Misc/ship_light.vert, Misc/ship_light.frag
    mat4 model = glm::translate(mat4(1.0), TransformPosition(ship.LightPositions[i]));
    float scale = mix(1.0f, 20.0f, distance / 2000.0f);
    model = glm::scale(model, vec3(scale));
    mShaderNavLight->setMat4("model", model);
    mShaderNavLight->setMat4("view", camera.GetView());
    mShaderNavLight->setMat4("projection", camera.GetProjection());
    mShaderNavLight->setVec3("lightColor", ship.LightColors[i]);
    mShaderNavLight->setVec3("viewPos", camera.GetPosition());
    mLight->Bind();
}
void Ship::RenderPropellers(Camera& camera, Sky* sky)
{
    if (ship.nPropeller == 0)
        return;

    // Light from sun
    mShaderShip->use();     // Ship/ship.vert, Ship/ship.frag
    mShaderShip->setVec3("light.position", sky->SunPosition);
    mShaderShip->setVec3("light.ambient", sky->SunAmbient);
    mShaderShip->setVec3("light.diffuse", sky->SunDiffuse);
    mShaderShip->setVec3("light.specular", sky->SunSpecular);
    mShaderShip->setVec3("viewPos", camera.GetPosition());
    mShaderShip->setFloat("exposure", sky->Exposure);
    mShaderShip->setMat4("view", camera.GetView());
    mShaderShip->setMat4("projection", camera.GetProjection());
    // Matricies
    if (ship.nPropeller == 2 && mPropeller1.get())
    {
        float omega1 = PropRpm1 * (2.0f * M_PI) / 60.0f; // radians par seconde
        static float rotation1 = 0.0f;
        rotation1 -= omega1 * mDt; // incrémente la rotation à chaque frame
        if (rotation1 > 360.0f)
            rotation1 -= 360.0f;
        mat4 matPropeller1 = mat4(1.0f);
        matPropeller1 = glm::translate(matPropeller1, ship.PosPropeller1);
        matPropeller1 = glm::rotate(matPropeller1, rotation1, vec3(1.0f, 0.0f, 0.0f));
        mShaderShip->setMat4("model", World * matPropeller1);
        mPropeller1->Render(*mShaderShip);
    }
    if (ship.nPropeller > 0 && mPropeller2.get())
    {
        float omega2 = PropRpm2 * (2.0f * M_PI) / 60.0f; // radians par seconde
        static float rotation2 = 0.0f;
        rotation2 += omega2 * mDt; // incrémente la rotation à chaque frame
        if (rotation2 > 360.0f)
            rotation2 -= 360.0f;
        mat4 matPropeller2 = mat4(1.0f);
        matPropeller2 = glm::translate(matPropeller2, ship.PosPropeller2);
        matPropeller2 = glm::rotate(matPropeller2, rotation2, vec3(1.0f, 0.0f, 0.0f));
        mShaderShip->setMat4("model", World * matPropeller2);
        mPropeller2->Render(*mShaderShip);
    }
}
void Ship::RenderRudders(Camera& camera, Sky* sky)
{
    if (!mRudder.get())
        return;

    // Light from sun
    mShaderShip->use();     // Ship/ship.vert, Ship/ship.frag
    mShaderShip->setVec3("light.position", sky->SunPosition);
    mShaderShip->setVec3("light.ambient", sky->SunAmbient);
    mShaderShip->setVec3("light.diffuse", sky->SunDiffuse);
    mShaderShip->setVec3("light.specular", sky->SunSpecular);
    mShaderShip->setVec3("viewPos", camera.GetPosition());
    mShaderShip->setFloat("exposure", sky->Exposure);
    // Matricies
    float rotation = -RudderAngleDeg * M_PI / 180.0f;
    mat4 matRudder1 = mat4(1.0f);
    matRudder1 = glm::translate(matRudder1, ship.PosRudder1);
    matRudder1 = glm::rotate(matRudder1, rotation, vec3(0.0f, 1.0f, 0.0f));
    mShaderShip->setMat4("model", World * matRudder1);

    mShaderShip->setMat4("view", camera.GetView());
    mShaderShip->setMat4("projection", camera.GetProjection());
    mRudder->Render(*mShaderShip);

    if (ship.nRudder > 1)
    {
        mat4 matRudder2 = mat4(1.0f);
        matRudder2 = glm::translate(matRudder2, ship.PosRudder2);
        matRudder2 = glm::rotate(matRudder2, rotation, vec3(0.0f, 1.0f, 0.0f));

        mShaderShip->setMat4("model", World * matRudder2);
        mRudder->Render(*mShaderShip);
    }
}
void Ship::RenderRadars(Camera& camera, Sky* sky)
{
    if (!bVisible)
        return;

    if (!bRadar)
        return;

    if (ship.nRadar == 0)
        return;

    if (!mRadar1.get())
        return;

    if (Rendering == eRendering::TRIANGLES)
        return;

    // Light from sun
    mShaderShip->use();     // Ship/ship.vert, Ship/ship.frag
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
    matRadar1 = glm::translate(matRadar1, ship.PosRadar1);
    matRadar1 = glm::rotate(matRadar1, glm::radians(rot1), vec3(0.0f, 1.0f, 0.0f));
    mShaderShip->setMat4("model", World * matRadar1);
    mShaderShip->setMat4("view", camera.GetView());
    mShaderShip->setMat4("projection", camera.GetProjection());
    mRadar1->Render(*mShaderShip);

    if (ship.nRadar > 1)
    {
        static float rot2 = 0.0f;
        rot2 -= ship.RotationRadar2 * 6.0f * mDt;
        rot2 = fmod(rot2, 360.0f);
        mat4 matRadar2 = mat4(1.0f);
        matRadar2 = glm::translate(matRadar2, ship.PosRadar2);
        matRadar2 = glm::rotate(matRadar2, glm::radians(rot2), vec3(0.0f, 1.0f, 0.0f));
        mShaderShip->setMat4("model", World * matRadar2);
        mRadar2->Render(*mShaderShip);
    }
}
void Ship::RenderFlag(Camera& camera, Sky* sky)
{
    if (Rendering == eRendering::TRIANGLES)
        return;
    
    if (!bFlag)
        return;

    if (!ship.bFlag)
        return;

    if (mFlag.get() == 0)
        return;

    vec2 windApparent = (- vCOG) + (- g_Wind);
    mFlag->Update(mDt, windApparent);

    mat4 model = glm::translate(mat4(1.0f), ship.PosFlag);
    model = glm::rotate(model, -Yaw, vec3(0.0f, 1.0f, 0.0f));
    mFlag->Render(World * model, camera.GetView(), camera.GetProjection(), sky->Exposure);
}
void Ship::RenderSmoke(Camera& camera, Sky* sky)
{
    if (Rendering == eRendering::TRIANGLES)
        return;

    if (!bSmoke)
        return;
    
    if (ship.nChimney == 0)
        return;

    float density = InterpolateAValue(0.0f, mPowerW, 0.01f, 0.15f, fabs(PowerApplied1));

    glDepthMask(GL_FALSE);

    glEnable(GL_PROGRAM_POINT_SIZE);
    mShaderSmokeRender->use();

    mShaderSmokeRender->setMat4("view", camera.GetView());
    mShaderSmokeRender->setMat4("projection", camera.GetProjection());
    mShaderSmokeRender->setFloat("density", density);
    mShaderSmokeRender->setFloat("lifeSpan", 5.0f);
    mShaderSmokeRender->setFloat("exposure", sky->Exposure);

    glBindVertexArray(mVaoSmoke);
    glDrawArraysInstanced(GL_POINTS, 0, 1, mSmokeMaxParticles);
    glDepthMask(GL_TRUE);

    glBindVertexArray(0);
}
void Ship::RenderSpray(Camera& camera, Sky* sky)
{
    if (Rendering == eRendering::TRIANGLES)
        return;
    
    if (!bVisible)
        return;

    if (!bSpray)
        return;

    glDepthMask(GL_FALSE);

    // ... = 0, [2 = 0, 6 = 1], ... = 1
    const float min = 2.0f;     // density starts to increase
    const float range = 4.0f;   // density stops to increase
    float density = std::clamp((Velocity - min) / range, 0.0f, 1.0f);
    if (mSpray)
        mSpray->Render(camera, density, sky->Exposure);
    
    glDepthMask(GL_TRUE);
}
void Ship::RenderReflexion(Camera& camera, Sky* sky)
{
    if (!bVisible)
        return;

    if (bWireframe)
        return;

    if (Rendering == eRendering::TRIANGLES)
        return;

    UpdateWorldMatrix();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_CULL_FACE); // Necessary because the windows of the bridge are transparent

    if (bModel)
    {
        switch (Rendering)
        {
        case eRendering::BASIC_LIGHT:
        {
            // Light from camera
            mShaderCamera->use();   // Misc/camera.vert, Misc/camera.frag
            mShaderCamera->setVec3("light.position", camera.GetPosition() - ship.Position);
            mShaderCamera->setVec3("light.diffuse", vec3(1.0f));
            mShaderCamera->setMat4("model", World);
            mShaderCamera->setMat4("view", camera.GetViewReflexion());
            mShaderCamera->setMat4("projection", camera.GetProjection());
            mModelFull->Render(*mShaderCamera);
        }
        break;
        case eRendering::SUN:
        {
            // Light from sun
            mShaderShip->use();     // Ship/ship.vert, Ship/ship.frag
            mShaderShip->setVec3("light.position", sky->SunPosition);
            mShaderShip->setVec3("light.ambient", sky->SunAmbient);
            mShaderShip->setVec3("light.diffuse", sky->SunDiffuse);
            mShaderShip->setVec3("light.specular", sky->SunSpecular);
            mShaderShip->setVec3("viewPos", camera.GetPosition());
            mShaderShip->setFloat("exposure", sky->Exposure);
            if (camera.GetMode() == eCameraMode::ORBITAL || camera.GetMode() == eCameraMode::FPS)
                mShaderShip->setFloat("envmapFactor", ship.EnvMapFactor);
            else
                mShaderShip->setFloat("envmapFactor", 0.0f);
            mShaderShip->setInt("envmap", 1);
            // Matricies
            mShaderShip->setMat4("model", World);
            mShaderShip->setMat4("view", camera.GetViewReflexion());
            mShaderShip->setMat4("projection", camera.GetProjection());

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, mTexEnvironment->id);

            mModelFull->Render(*mShaderShip);
        }
        break;
        }
    }
}
void Ship::RenderShadow(Camera& camera, Sky* sky)
{
    if (!bVisible)
        return;

    if (bWireframe)
        return;

    if (Rendering == eRendering::TRIANGLES)
        return;
    
    glEnable(GL_DEPTH_TEST);

    vec3 lightDir = glm::normalize(sky->SunPosition);
    vec3 lightPos = ship.Position + lightDir * 200.0f;
    mat4 lightView = glm::lookAt(lightPos, ship.Position, vec3(0.0f, 1.0f, 0.0f));
    

    vec3 BBmin = TransformPosition(mBbox.min);
    vec3 BBmax = TransformPosition(mBbox.max);
    vec3 bboxCorners[8];
    bboxCorners[0] = vec3(BBmin.x, BBmin.y, BBmin.z);
    bboxCorners[1] = vec3(BBmax.x, BBmin.y, BBmin.z);
    bboxCorners[2] = vec3(BBmin.x, BBmax.y, BBmin.z);
    bboxCorners[3] = vec3(BBmax.x, BBmax.y, BBmin.z);
    bboxCorners[4] = vec3(BBmin.x, BBmin.y, BBmax.z);
    bboxCorners[5] = vec3(BBmax.x, BBmin.y, BBmax.z);
    bboxCorners[6] = vec3(BBmin.x, BBmax.y, BBmax.z);
    bboxCorners[7] = vec3(BBmax.x, BBmax.y, BBmax.z);

    vec3 lightSpaceMin(FLT_MAX);
    vec3 lightSpaceMax(-FLT_MAX);
    for (int i = 0; i < 8; ++i) 
    {
        vec4 lightPos = lightView * vec4(bboxCorners[i], 1.0f);
        vec3 ls = vec3(lightPos);
        lightSpaceMin = glm::min(lightSpaceMin, ls);
        lightSpaceMax = glm::max(lightSpaceMax, ls);
    }
    vec3 lightSpaceSize = lightSpaceMax - lightSpaceMin;
    float dimOptimized = glm::max(lightSpaceSize.x, lightSpaceSize.y) * 0.5f;
    
    const float far_plane = 300.0f;
    mat4 lightProjection = glm::ortho(-dimOptimized, dimOptimized, -dimOptimized, dimOptimized, -lightSpaceMax.z, far_plane);
    LightViewProjection = lightProjection * lightView;

    glBindFramebuffer(GL_FRAMEBUFFER, FBO_SHADOW);
    glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
    glClear(GL_DEPTH_BUFFER_BIT);

    mShaderShadow->use();
    mShaderShadow->setMat4("lightSpaceMatrix", LightViewProjection);
    mShaderShadow->setMat4("model", World);
    mModelFull->Render(*mShaderShadow);

    //SaveTexture2D(TexShadowMap, SHADOW_SIZE, SHADOW_SIZE, 1, GL_DEPTH_COMPONENT, "Outputs/depth_map.png");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    glDisable(GL_DEPTH_TEST);
}

void Ship::Render(Camera& camera, Sky* sky)
{
    if (!bVisible)
        return;
    
    UpdateWorldMatrix();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_CULL_FACE); // Necessary because the windows of the bridge are transparent

    if (bWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

#pragma region Ship
    if (bModel)
    {
        switch (Rendering)
        {
        case eRendering::TRIANGLES:
        {
            mShaderHullColored->use();     // Misc/hull_colored.vert, Misc/hull_colored.frag
            mShaderHullColored->setMat4("model", World);
            mShaderHullColored->setMat4("view", camera.GetView());
            mShaderHullColored->setMat4("projection", camera.GetProjection());

            glBindVertexArray(mVaoHull);
            glDrawElements(GL_TRIANGLES, mIndicesFull, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        break;
        case eRendering::BASIC_LIGHT:
        {
            // Light from camera
            mShaderCamera->use();   // Misc/camera.vert, Misc/camera.frag
            mShaderCamera->setVec3("light.position", camera.GetPosition() - ship.Position);
            mShaderCamera->setVec3("light.diffuse", vec3(1.0f));
            mShaderCamera->setMat4("model", World);
            mShaderCamera->setMat4("view", camera.GetView());
            mShaderCamera->setMat4("projection", camera.GetProjection());
            mModelFull->Render(*mShaderCamera);
        }
        break;
        case eRendering::SUN:
        {
            // Light from sun
            mShaderShipShadow->use();     // Ship/ship_shadow.vert, Ship/ship_shadow.frag
            mShaderShipShadow->setVec3("light.position", sky->SunPosition);
            mShaderShipShadow->setVec3("light.ambient", sky->SunAmbient);
            mShaderShipShadow->setVec3("light.diffuse", sky->SunDiffuse);
            mShaderShipShadow->setVec3("light.specular", sky->SunSpecular);
            mShaderShipShadow->setVec3("viewPos", camera.GetPosition());
            mShaderShipShadow->setFloat("exposure", sky->Exposure);
            mShaderShipShadow->setInt("texture_diffuse1", 0);
            if (camera.GetMode() == eCameraMode::ORBITAL || camera.GetMode() == eCameraMode::FPS)
                mShaderShipShadow->setFloat("envmapFactor", ship.EnvMapFactor);
            else
                mShaderShipShadow->setFloat("envmapFactor", 0.0f);
            mShaderShipShadow->setInt("envmap", 1);
            // Matricies
            mShaderShipShadow->setMat4("model", World);
            mShaderShipShadow->setMat4("view", camera.GetView());
            mShaderShipShadow->setMat4("projection", camera.GetProjection());
            mShaderShipShadow->setMat4("matLightViewProj", LightViewProjection);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, mTexEnvironment->id);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, TexShadowMap);
            mShaderShipShadow->setInt("shadowMap", 2);
            mShaderShipShadow->setBool("bShipShadow", g_bShipShadow);

            mModelFull->RenderWithTransparency(*mShaderShipShadow, camera, World);
        }
        break;
        }

        RenderPropellers(camera, sky);
        RenderRudders(camera, sky);
        RenderRadars(camera, sky);
        RenderFlag(camera, sky);
    }
#pragma endregion

    if (bWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#pragma region Navigation lights
    {
        if (camera.GetMode() != eCameraMode::BRIDGE)
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
    }
#pragma endregion

#pragma region Wireframe
    if (bOutline)
    {
        mShaderWireframe->use();    // Misc/unicolor.vert, Misc/unicolor.frag, Misc/unicolor.geom
        mShaderWireframe->setVec3("lineColor", vec3(1.0f, 1.0f, 1.0f));
        mShaderWireframe->setMat4("model", World);
        mShaderWireframe->setMat4("view", camera.GetView());
        mShaderWireframe->setMat4("projection", camera.GetProjection());

        switch (Rendering)
        {
        case eRendering::TRIANGLES:
            glBindVertexArray(mVaoHull);
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
        mShaderPressure->use();     // Misc/unicolor.vert, Misc/unicolor.frag
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
		mShaderUnicolor->use();     // Misc/unicolor.vert", Misc/unicolor.frag
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
        if (ship.nPropeller == 2)
        {
            RenderForceRefWorld(camera, Thrust1, 50.0f * f, Red, true);
            RenderForceRefWorld(camera, PropTorque1, 50.0f * f, Red, true);
        }
        RenderForceRefWorld(camera, Thrust2, 50.0f * f, Red, true);
        RenderForceRefWorld(camera, PropTorque2, 50.0f * f, Red, true);
        RenderForceRefWorld(camera, ResistanceViscous, 50.0f * f, Pink, true);
        RenderForceRefWorld(camera, ResistanceWaves, 50.0f * f, Cyan, true);
        RenderForceRefWorld(camera, BowThrust, 50.0f * f, Magenta, true);
        RenderForceRefWorld(camera, SternThrust, 50.0f * f, Magenta, true);
        RenderForceRefWorld(camera, RudderLift, 50.0f * f, Green, true);
        RenderForceRefWorld(camera, RudderDrag, 50.0f * f, Green, true);
        RenderForceRefWorld(camera, WindFront, 500.0f * f, Violet, true);
        RenderForceRefWorld(camera, WindRear, 500.0f * f, Violet, true);
        RenderForceRefWorld(camera, ResistanceAir, 500.0f * f, Marron, true);

        RenderForceRefWorld(camera, COGSOG, 50.0f * f, Yellow, true);
    }
#pragma endregion

#pragma region Axis
    if (bAxis)
    {
        mat4 modelX = glm::scale(World, vec3(5.0f + mLength / 2.0f, 0.05f, 0.05f));
        mAxis->RenderDistorted(camera, modelX, vec3(1.0f, 0.0f, 0.0f));

        mat4 modelY = glm::scale(World, vec3(0.05f, 10.0f + mHeight / 2.0f, 0.05f));
        mAxis->RenderDistorted(camera, modelY, vec3(0.0f, 1.0f, 0.0f));

        mat4 modelZ = glm::scale(World, vec3(0.05f, 0.05f, 5.0f + mWidth / 2.0f));
        mAxis->RenderDistorted(camera, modelZ, vec3(0.0f, 0.0f, 1.0f));
    }
#pragma endregion

}
