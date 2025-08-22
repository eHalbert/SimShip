/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#include "Sky.h"

extern "C"
{
#include "nova/nova.h"
}
#ifdef _DEBUG
#pragma comment(lib, "nova/Debug/nova.lib")
#else
#pragma comment(lib, "nova/Release/nova.lib")
#endif

extern vec2 g_InitialPosition;

// Extracted table, format (temperature, r, g, b) from 1000K to 10000K, 500K/step
static const struct { float k, r, g, b; } blackbodyLUT[] = {
    {1000, 1.000f, 0.036f, 0.000f},
    {1500, 1.000f, 0.252f, 0.033f},
    {2000, 1.000f, 0.396f, 0.127f},
    {2500, 1.000f, 0.522f, 0.210f},
    {3000, 1.000f, 0.641f, 0.326f},
    {3500, 1.000f, 0.739f, 0.442f},
    {4000, 1.000f, 0.812f, 0.549f},
    {4500, 1.000f, 0.873f, 0.647f},
    {5000, 1.000f, 0.921f, 0.724f},
    {5500, 1.000f, 0.959f, 0.817f},
    {6000, 1.000f, 0.990f, 0.906f},
    {6500, 1.000f, 1.000f, 1.000f},
    {7000, 0.910f, 0.968f, 1.000f},
    {8000, 0.780f, 0.913f, 1.000f},
    {9000, 0.669f, 0.862f, 1.000f},
    {10000,0.568f, 0.816f, 1.000f}
};

Sky::Sky(vec2 pos, int width, int height)
{
    LongitudeObserver = pos.x;
    LatitudeObserver = pos.y;
    mWidth = width;
    mHeight = height;

    SetNow();

    // Textures

    float* data = new float[16 * 64 * 3];
    FILE* f = fopen("Resources/Sky/irradiance.raw", "rb");
    fread(data, 1, 16 * 64 * 3 * sizeof(float), f);
    fclose(f);
    glGenTextures(1, &mTexIrradiance);
    glBindTexture(GL_TEXTURE_2D, mTexIrradiance);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 64, 16, 0, GL_RGB, GL_FLOAT, data);
    delete[] data;

    int res = 64;
    int nr = res / 2;
    int nv = res * 2;
    int nb = res / 2;
    int na = 8;
    f = fopen("Resources/Sky/inscatter.raw", "rb");
    data = new float[nr * nv * nb * na * 4];
    fread(data, 1, nr * nv * nb * na * 4 * sizeof(float), f);
    fclose(f);
    glGenTextures(1, &mTexInscatter);
    glBindTexture(GL_TEXTURE_3D, mTexInscatter);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, na * nb, nv, nr, 0, GL_RGBA, GL_FLOAT, data);
    delete[] data;

    data = new float[256 * 64 * 3];
    f = fopen("Resources/Sky/transmittance.raw", "rb");
    fread(data, 1, 256 * 64 * 3 * sizeof(float), f);
    fclose(f);
    glGenTextures(1, &mTexTransmittance);
    glBindTexture(GL_TEXTURE_2D, mTexTransmittance);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 256, 64, 0, GL_RGB, GL_FLOAT, data);
    delete[] data;

    float maxAnisotropy = 1.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
    maxAnisotropy = 2.0f;

    unsigned char* img = new unsigned char[512 * 512 + 38];
    f = fopen("Resources/Sky/noise.pgm", "rb");
    fread(img, 1, 512 * 512 + 38, f);
    fclose(f);
    glGenTextures(1, &mTexNoise);
    glBindTexture(GL_TEXTURE_2D, mTexNoise);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512, 512, 0, GL_RED, GL_UNSIGNED_BYTE, img + 38);
    glGenerateMipmap(GL_TEXTURE_2D);
    delete[] img;

    // Texture sky & Framebuffer
    glGenTextures(1, &mTextureSky);
    glBindTexture(GL_TEXTURE_2D, mTextureSky);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glGenerateMipmap(GL_TEXTURE_2D);

    glGenFramebuffers(1, &FRAMEBUFFER_SKY);
    glBindFramebuffer(GL_FRAMEBUFFER, FRAMEBUFFER_SKY);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTextureSky, 0);

    // Back to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);

    // Shader

    mShader = new Shader();
    mShader->Load("Resources/Sky/sky.vert", "Resources/Sky/sky.frag");
    mShader->use();
    mShader->setInt("skyIrradianceSampler", 0);
    mShader->setInt("inscatterSampler", 1);
    mShader->setInt("transmittanceSampler", 2);
    mShader->setInt("noiseSampler", 3);
    mShader->setInt("skySampler", 4);

    mQuad = new Quad();
}
Sky::~Sky()
{
    delete mShader;
    delete mQuad;

    glDeleteTextures(1, &mTexIrradiance);
    glDeleteTextures(1, &mTexInscatter);
    glDeleteTextures(1, &mTexTransmittance);
    glDeleteTextures(1, &mTexNoise);

    glDeleteFramebuffers(1, &FRAMEBUFFER_SKY);
    glDeleteTextures(1, &mTextureSky);
}

void Sky::SetObserver(vec2 pos)
{
    LongitudeObserver = pos.x;
    LatitudeObserver = pos.y;
}
void Sky::SetPositionFromDistance()
{
    float radPhi = glm::radians(SunAzimut + 270.0f);
    float radTheta = glm::radians(SunElevation);

    SunDirection = vec3(sin(radTheta) * cos(radPhi), cos(radTheta), sin(radTheta) * sin(radPhi));
    SunPosition = SunDirection * SunDistance;

    radPhi = glm::radians(360.0f - SunAzimut);
    radTheta = glm::radians(SunElevation);
    SunDirectionEB = vec3(sin(radTheta) * cos(radPhi), sin(radTheta) * sin(radPhi), cos(radTheta));
};
void Sky::ExpositionFromAngle()
{
    const float minExposure = 0.02f;
    const float maxExposure = 1.0f;
    const float maxNightAngle = 108.0f;      // beginning of the "dark night"
    const float plateauStart = 65.0f;        // start of decay (adjusts according to desired effect)

    // Clamp de l'angle
    float clampedAngle = SunElevation;
    if (clampedAngle < 0.0f) clampedAngle = 0.0f;
    if (clampedAngle > maxNightAngle) clampedAngle = maxNightAngle;

    // Plateau at 1.0 on the sunny edge
    if (clampedAngle <= plateauStart)
    {
        Exposure = maxExposure;
        return;
    }

    // Smoothstep transition (Hermite/Cubic)
    // x = 0 when we start to decrease; x = 1 when we are in the night
    float t = (clampedAngle - plateauStart) / (maxNightAngle - plateauStart);
    // Smoothstep : 3t² - 2t³
    float smooth = 3.0f * t * t - 2.0f * t * t * t;
    Exposure = (1.0f - smooth) * maxExposure + smooth * minExposure;
}
vec3 blackbody_to_rgb(float tempK)
{
    // Clamp aux bornes de la table :
    if (tempK <= blackbodyLUT[0].k) return vec3(blackbodyLUT[0].r, blackbodyLUT[0].g, blackbodyLUT[0].b);
    if (tempK >= blackbodyLUT[15].k) return vec3(blackbodyLUT[15].r, blackbodyLUT[15].g, blackbodyLUT[15].b);

    // Interpolation linéaire
    for (int i = 0; i < 15; ++i)
    {
        if (tempK >= blackbodyLUT[i].k && tempK < blackbodyLUT[i + 1].k)
        {
            float t = (tempK - blackbodyLUT[i].k) / (blackbodyLUT[i + 1].k - blackbodyLUT[i].k);
            vec3 rgb0(blackbodyLUT[i].r, blackbodyLUT[i].g, blackbodyLUT[i].b);
            vec3 rgb1(blackbodyLUT[i + 1].r, blackbodyLUT[i + 1].g, blackbodyLUT[i + 1].b);
            return glm::mix(rgb0, rgb1, t);
        }
    }
    return vec3(1.0, 1.0, 1.0); // fallback
}
void Sky::ColorOfTheSun()
{
    // Empirical model: visible sun temperature as a function of elevation
    // 2000K (red horizon) to 6500K (white zenith)
    float t_min = 2000.0f, t_max = 6500.0f;

    float t = glm::clamp(glm::radians(SunElevation) / 1.5708f, 0.0f, 1.0f);
    t = 1.0f - t;  // t=1 (zenith), t=0 (horizon)
    float tempK = glm::mix(t_min, t_max, t);

    vec3 temp = blackbody_to_rgb(tempK);

    SunEmissive = temp;
    SunDiffuse = temp;
    SunSpecular = temp;
}

void Sky::SetNow()
{
    // Get the current time in UTC
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    // Convert to tm structure in UTC
    std::tm tm_utc;
    gmtime_s(&tm_utc, &now_c);

    // Create the ln_date structure with UTC values
    struct ln_date date;
    date.years = tm_utc.tm_year + 1900;
    date.months = tm_utc.tm_mon + 1;
    date.days = tm_utc.tm_mday;
    date.hours = tm_utc.tm_hour;
    date.minutes = tm_utc.tm_min;
    date.seconds = 0.0;

    SunHour = date.hours;
    SunMinute = date.minutes;
    tmTimeStored = tm_utc;

    // Calculate the Julian day
    double JD = ln_get_julian_day(&date);

    struct ln_lnlat_posn observer;
    observer.lng = LongitudeObserver;
    observer.lat = LatitudeObserver;

    struct ln_equ_posn sun;
    ln_get_solar_equ_coords(JD, &sun);

    struct ln_hrz_posn position;
    ln_get_hrz_from_equ(&sun, &observer, JD, &position);

    SunElevation = 90.0f - position.alt;
    SunAzimut = position.az + 180.0f;

    while (SunElevation < 0.0f)      SunElevation += 360.0f;
    while (SunElevation > 360.0f)    SunElevation -= 360.0f;
    while (SunAzimut < 0.0f)        SunAzimut += 360.0f;
    while (SunAzimut > 360.0f)      SunAzimut -= 360.0f;
    ExpositionFromAngle();
    ColorOfTheSun();
    SetPositionFromDistance();
}
sHM Sky::GetNow()
{
    // Get the current time in UTC
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    // Convert to tm structure in UTC
    std::tm tm_utc;
    gmtime_s(&tm_utc, &now_c);

    sHM hm{};
    hm.hour = SunHour = tm_utc.tm_hour;
    hm.minute = SunMinute = tm_utc.tm_min;

    return hm;
}
void Sky::SetTime(int hour, int minute)
{
    // Get the current time in UTC
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    // Convert to tm structure in UTC
    std::tm tm_utc;
    gmtime_s(&tm_utc, &now_c);

    // Create the ln_date structure with UTC values
    struct ln_date date;
    date.years = tm_utc.tm_year + 1900;
    date.months = tm_utc.tm_mon + 1;
    date.days = tm_utc.tm_mday;
    date.hours = hour;
    date.minutes = minute;
    date.seconds = 0.0;

    SunHour = hour;
    SunMinute = minute;
    tmTimeStored = tm_utc;

    // Calculate the Julian day
    double JD = ln_get_julian_day(&date);

    struct ln_lnlat_posn observer;
    observer.lng = LongitudeObserver;
    observer.lat = LatitudeObserver;

    struct ln_equ_posn sun;
    ln_get_solar_equ_coords(JD, &sun);

    struct ln_hrz_posn position;
    ln_get_hrz_from_equ(&sun, &observer, JD, &position);

    SunElevation = 90.0f - position.alt;
    SunAzimut = position.az + 180.0f;

    while (SunElevation < 0.0f)      SunElevation += 360.0f;
    while (SunElevation > 360.0f)    SunElevation -= 360.0f;
    while (SunAzimut < 0.0f)        SunAzimut += 360.0f;
    while (SunAzimut > 360.0f)      SunAzimut -= 360.0f;
    ExpositionFromAngle();
    ColorOfTheSun();
    SetPositionFromDistance();
}
sHM Sky::GetTime()
{
    // Get the current time in UTC
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    // Convert to tm structure in UTC
    std::tm tm_utc;
    gmtime_s(&tm_utc, &now_c);

    int hour = tm_utc.tm_hour - tmTimeStored.tm_hour;
    int minute = tm_utc.tm_min - tmTimeStored.tm_min;
    if (SunHour + hour > 23)
        hour = 0;
    if (SunMinute + minute > 59)
    {
        if (SunHour + hour < 23)
        {
            SunMinute = 0;
            SunHour++;
        }
        else
        {
            SunMinute = 59;
            SunHour = 23;
        }
    }
    sHM hm{};
    hm.hour = SunHour + hour;
    hm.minute = SunMinute + minute;

    return hm;
}

void Sky::Render(Camera& camera)
{
    // Matricies
    vec3 cameraPos = camera.GetPosition();
    mat4eb<float> proj = mat4eb<float>::perspectiveProjection(camera.GetZoom(), (float)camera.GetViewportWidth() / (float)camera.GetViewportHeight(), 0.1f, 300000.0f);
    mat4eb<float> view = mat4eb<float>(
        0.0, -1.0, 0.0, -cameraPos.x,
        0.0, 0.0, 1.0, -cameraPos.y,
        -1.0, 0.0, 0.0, -cameraPos.z,
        0.0, 0.0, 0.0, 1.0
    );
    view = mat4eb<float>::rotatey(camera.GetNorthAngleDEG()) * view;
    view = mat4eb<float>::rotatex(-camera.GetAttitudeDEG()) * view;

    // Framebuffer -> output = mTextureSky
    glBindFramebuffer(GL_FRAMEBUFFER, FRAMEBUFFER_SKY);

    glDisable(GL_CULL_FACE); // Disables face rendering choice

    mShader->use();     // Sky/sky.vert Sky/sky.frag
    glUniformMatrix4fv(glGetUniformLocation(mShader->ID, "screenToCamera"), 1, true, proj.inverse().coefficients());
    glUniformMatrix4fv(glGetUniformLocation(mShader->ID, "cameraToWorld"), 1, true, view.inverse().coefficients());

    mShader->setVec3("worldCamera", 0.0f, 0.0f, 3.5f);
    mShader->setVec3("worldSunDir", SunDirectionEB);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexIrradiance);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, mTexInscatter);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mTexTransmittance);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTexNoise);

    mQuad->Render();
    //SaveTexture2D(mTextureSky, mWidth, mHeight, 4, GL_RGBA, "Outputs/Sky.png");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}