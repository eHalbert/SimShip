/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#pragma once

#define NOMINMAX
#include <map>
#include <array>
#include <functional>
#include <iostream>

// glfw
#include <glfw/glfw3.h>
// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>

using namespace std; 
using namespace glm;
using InterpFunc = std::function<float(float)>;

enum eCameraMode{ ORBITAL, FPS, IN_FIXED, IN_FREE, OUT_FREE, CAMERA_MODE_COUNT };

static float LinInterp(float t) { return t; }
static float SmoothStepInterp(float t) { return t * t * (3.0f - 2.0f * t); }
static float EaseInInterp(float t) { return t * t; }
static float EaseOutInterp(float t) { return 1.0f - (1.0f - t) * (1.0f - t); }
static float EaseInOutInterp(float t) { return t < 0.5f ? 2.f * t * t : 1.f - pow(-2.f * t + 2.f, 2) / 2.f; }

enum eInterpolation { Linear, SmoothStep, EaseIn, EaseOut, EaseInOut, COUNT };

class Camera 
{
public:
	// Init
    void    SetProjection(float fovy, int width, int height, float znear, float zfar);
	void    LookAt(vec3 posCamera, vec3 posTarget, vec3 up = { 0.f, 1.f, 0.f });
    void    SetZoom(float fovy);
    void    SetSpeeds(float movementSpeed, float rotationSpeed) { mMoveSpeed = movementSpeed; mRotateSpeed = rotationSpeed; }
    void    SetViewportSize(int width, int height);
    void    SetPosition(vec3 posCamera);
    void    SetInterpolation(eInterpolation type) { mInterpolation = type; }

    // Inputs
    void    KeyboardUpdate(int key, int scancode, int action, int mods);
    void    MousePosUpdate(double xpos, double ypos);
    void    MouseButtonUpdate(int button, int action, int mods);
    void    Animate(float deltaT, vec3& orbitalTarget, vec3& view1Pos, vec3& view1Target);
    InterpFunc mInterpFunc = [](float t) { return t; }; // linéaire par défaut

    // Get
    vec3    GetPosition() const { return mPosition; }
	mat4    GetProjection()  const { return mMatProjection; }
	mat4    GetView() const { return mMatView; }
	mat4    GetViewProjection() const { return mMatViewProjection; }
    mat4    GetViewReflexion() const { return mMatViewReflexion; }
    vec3    GetDirection() { return mDirection; }
    float   GetZoom() const { return mFovyDeg; }
    float   GetInterpolationValue(float t) const;
    eInterpolation GetInterpolation() const { return mInterpolation; }
    bool    IsUnchanged() const { return mIsUnchanged; }

    eCameraMode GetMode() { return mCurrentMode; }
    bool    IsInViewFrustum(const vec3& position);
    float   GetHorizonViewportY() const;
    float   GetNorthAngleDEG();
    float   GetAttitudeDEG();
    float   GetRollDEG();

    float   GetOrbitRadius() { return mOrbitRadius; }
    int     GetViewportWidth() { return mWindowW; }
    int     GetViewportHeight() { return mWindowH; }

    vec3    GetAt() const { return mDirection; }
    vec3    GetUp() const { return mUp; }

    // Orbital mode
    void    SetOrbitalMode();
    void    SetTarget(const vec3& target);
    void    AdjustOrbitRadius(float delta);

    void    SetMode(eCameraMode mode);
    void    ReturnToPreviousMode();

private:
    void    updateView();
	void    updateViewProjection();
    void    RotateCamera(float yaw, float pitch, float roll);
    void    MoveCamera(const vec3& delta);

    bool    mIsUnchanged;

    mat4    mMatView;               // transform from world space to screen UV [0, 1]
	mat4    mMatProjection;         // projection matrix (view space to clip space)
	mat4    mMatViewProjection;     // transform from world space to clip space [-1, 1] (WorldToView * Projection)
    mat4    mMatViewReflexion;      // transform from world space to screen UV [0, 1]

    float   mMoveSpeed = 0.001f;       // movement speed in units/second
    float   mRotateSpeed = 0.0025f;    // mouse sensitivity in radians/pixel

	vec2    mMousePos;
	vec2    mMousePosPrev;

	vec3    mPosition;              // in world space
	vec3    mDirection;             // normalized
	vec3    mUp;                    // normalized
	vec3    mRight;                 // normalized

    vec3    mPositionTarget;
    vec3    mDirectionTarget;
    vec3    mUpTarget;
    vec3    mRightTarget;
    bool    bFirstUpdate = true;
    eInterpolation mInterpolation = eInterpolation::EaseInOut;

    int     mWindowW = 1600;
    int     mWindowH = 1000;
    float   mFovyDeg;
    float   mZnear;
    float   mZfar;
   
    eCameraMode mCurrentMode;
    eCameraMode mPreviousMode;

    // Orbital mode
    vec3    mTargetPos;
    float   mOrbitRadius;
    float   mOrbitYaw;
    float   mOrbitPitch;

    typedef enum
    {
        MoveUp,
        MoveDown,
        MoveLeft,
        MoveRight,
        MoveForward,
        MoveBackward,
        Stop,

        YawRight,
        YawLeft,
        PitchUp,
        PitchDown,
        RollLeft,
        RollRight,

        SpeedUp,
        SlowDown,

        PreviousCamera,
        NextCamera,

        KeyboardControlCount,
    } KeyboardControls;

    typedef enum
    {
        Left,
        Middle,
        Right,

        MouseButtonCount,
        MouseButtonFirst = Left,
    } MouseButtons;

    const map<int, int> mKeyboardMap = {
        { GLFW_KEY_E,           KeyboardControls::MoveUp },
        { GLFW_KEY_Q,           KeyboardControls::MoveDown },
        { GLFW_KEY_A,           KeyboardControls::MoveLeft },
        { GLFW_KEY_D,           KeyboardControls::MoveRight },
        { GLFW_KEY_W,           KeyboardControls::MoveForward },
        { GLFW_KEY_S,           KeyboardControls::MoveBackward },
        { GLFW_KEY_LEFT,        KeyboardControls::YawLeft },
        { GLFW_KEY_RIGHT,       KeyboardControls::YawRight },
        { GLFW_KEY_UP,          KeyboardControls::PitchUp },
        { GLFW_KEY_DOWN,        KeyboardControls::PitchDown },
        { GLFW_KEY_Z,           KeyboardControls::RollLeft },
        { GLFW_KEY_X,           KeyboardControls::RollRight },
        { GLFW_KEY_C,           KeyboardControls::PreviousCamera },
        { GLFW_KEY_V,           KeyboardControls::NextCamera },
        { GLFW_KEY_LEFT_SHIFT,  KeyboardControls::SpeedUp },
        { GLFW_KEY_LEFT_CONTROL,KeyboardControls::SlowDown },
    };

    const std::map<int, int> mMouseButtonMap = {
        { GLFW_MOUSE_BUTTON_LEFT, MouseButtons::Left },
        { GLFW_MOUSE_BUTTON_MIDDLE, MouseButtons::Middle },
        { GLFW_MOUSE_BUTTON_RIGHT, MouseButtons::Right },
    };

    std::array<bool, KeyboardControls::KeyboardControlCount> mKeyboardState = { false };
    std::array<bool, MouseButtons::MouseButtonCount> mMouseButtonState = { false };
};
