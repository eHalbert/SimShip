/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#include "Camera.h"

// Init
void Camera::LookAt(vec3 cameraPos, vec3 cameraTarget, vec3 cameraUp)
{
    this->mPosition = cameraPos;
    this->mDirection = normalize(cameraTarget - cameraPos);
    this->mUp = normalize(cameraUp);
    this->mRight = normalize(cross(this->mUp, this->mDirection));

    updateView();
}
void Camera::SetProjection(float fovy, int width, int height, float znear, float zfar)
{
    mWindowW = width;
    mWindowH = height;
    mFovyDeg = fovy;
    mZnear = znear;
    mZfar = zfar;

    mMatProjection = glm::perspective(glm::radians(fovy), float(width) / float(height), znear, zfar);
    updateViewProjection();
}
void Camera::SetViewportSize(int width, int height)
{
    mWindowW = width;
    mWindowH = height;
    mMatProjection = glm::perspective(glm::radians(mFovyDeg), float(mWindowW) / float(mWindowH), mZnear, mZfar);
    updateViewProjection();
}
void Camera::SetPosition(vec3 posCamera)
{
    this->mPosition = posCamera;
    updateView();
}
void Camera::SetZoom(float fovy)
{
    mFovyDeg = fovy;
    mMatProjection = glm::perspective(glm::radians(fovy), float(mWindowW) / float(mWindowH), mZnear, mZfar);
    updateViewProjection();
}
// Update
void Camera::RotateCamera(float yaw, float pitch, float roll)
{
    mat4 rotationMatrix =
        glm::rotate(mat4(1.0f), yaw, vec3(0, 1.0f, 0)) *
        glm::rotate(mat4(1.0f), pitch, mRight) *
        glm::rotate(mat4(1.0f), roll, mDirection);

    mDirection = vec3(glm::normalize(rotationMatrix * vec4(mDirection, 0.f)));
    mUp = vec3(0, 1, 0); //vec3(glm::normalize(rotationMatrix * vec4(mUp, 0.f)));
    mRight = glm::normalize(cross(mUp, mDirection));
}
void Camera::MoveCamera(const vec3& delta)
{
    mPosition += delta;
}
void Camera::Animate(float deltaT, vec3& orbitalTarget, vec3& view1Pos, vec3& view1Target)
{
    mIsUnchanged = true;

    float baseT = 0.1f;
    float t = GetInterpolationValue(baseT);

    // Mouse delta management
    vec2 mouseMove = mMousePos - mMousePosPrev;
    mMousePosPrev = mMousePos;

    // Initialisation des targets la première frame
    if (bFirstUpdate)
    {
        mPositionTarget = mPosition;
        mDirectionTarget = mDirection;
        mUpTarget = mUp;
        mRightTarget = mRight;
        bFirstUpdate = false;
    }

    // Flag pour savoir si la caméra a changé de mode
    static bool bCameraModeChanged = false;

    //constexpr int CAMERA_MODE_COUNT = 5;

    // Gestion du changement de mode de caméra
    if (mKeyboardState[KeyboardControls::PreviousCamera] || mKeyboardState[KeyboardControls::NextCamera])
    {
        int dir = mKeyboardState[KeyboardControls::NextCamera] ? 1 : -1;
        mCurrentMode = static_cast<eCameraMode>((CAMERA_MODE_COUNT + static_cast<int>(mCurrentMode) + dir) % CAMERA_MODE_COUNT);
        mKeyboardState[KeyboardControls::PreviousCamera] = false;
        mKeyboardState[KeyboardControls::NextCamera] = false;

        bCameraModeChanged = true;  // le mode a changé
        mIsUnchanged = false;
    }

    // Si on vient de changer de mode on réinitialise les cibles à l'état actuel
    if (bCameraModeChanged)
    {
        mPositionTarget = mPosition;
        mDirectionTarget = mDirection;
        mUpTarget = vec3(0, 1, 0); //mUp;
        mRightTarget = mRight;

        bCameraModeChanged = false;
    }

    // ===== ORBITAL MODE ===== //
    if (mCurrentMode == eCameraMode::ORBITAL)
    {
        if (mMouseButtonState[MouseButtons::Left] && (mouseMove.x || mouseMove.y))
        {
            // There is a new mouse position with a left clic
            mOrbitYaw += mouseMove.x * mRotateSpeed;
            mOrbitPitch += mouseMove.y * mRotateSpeed;
            mOrbitPitch = glm::clamp(mOrbitPitch, -glm::pi<float>() / 2.f + 0.1f, glm::pi<float>() / 2.f - 0.1f);
            mIsUnchanged = false;
        }
        mTargetPos = orbitalTarget;

        float x = mTargetPos.x + mOrbitRadius * cos(mOrbitPitch) * cos(mOrbitYaw);
        float y = mTargetPos.y + mOrbitRadius * sin(mOrbitPitch);
        float z = mTargetPos.z + mOrbitRadius * cos(mOrbitPitch) * sin(mOrbitYaw);
        vec3 orbitalPosTarget(x, y, z);

        vec3 directionTarget = normalize(mTargetPos - orbitalPosTarget);
        vec3 rightTarget = normalize(cross(vec3(0, 1, 0), directionTarget));
        vec3 upTarget = vec3(0, 1, 0); //normalize(cross(directionTarget, rightTarget));

        mPosition = glm::mix(mPosition, orbitalPosTarget, t);
        mDirection = glm::normalize(glm::mix(mDirection, directionTarget, t));
        mUp = vec3(0, 1, 0); //glm::normalize(glm::mix(mUp, mUpTarget, t));
        mRight = glm::normalize(glm::mix(mRight, mRightTarget, t));
    }

    // ===== FPS MODE ===== //
    else if (mCurrentMode == eCameraMode::FPS)
    {
        bool cameraDirty = false;
        float yaw = 0, pitch = 0, roll = 0;
        vec3 moveVec{ 0.f };

        float moveStep = deltaT * mMoveSpeed;
        if (mKeyboardState[KeyboardControls::SpeedUp]) moveStep *= 10.f;
        if (mKeyboardState[KeyboardControls::SlowDown]) moveStep *= 0.1f;

        if (mMouseButtonState[MouseButtons::Left] && (mouseMove.x || mouseMove.y))
        {
            yaw = -mRotateSpeed * mouseMove.x;
            pitch = mRotateSpeed * mouseMove.y;
            cameraDirty = true;
        }

        if (mKeyboardState[KeyboardControls::RollLeft])
        {
            roll -= mRotateSpeed * 2.f;
            cameraDirty = true;
        }

        if (mKeyboardState[KeyboardControls::RollRight])
        {
            roll += mRotateSpeed * 2.f;
            cameraDirty = true;
        }

        if (mKeyboardState[KeyboardControls::MoveForward]) { moveVec += moveStep * mDirection; cameraDirty = true; }
        if (mKeyboardState[KeyboardControls::MoveBackward]) { moveVec += -moveStep * mDirection; cameraDirty = true; }
        if (mKeyboardState[KeyboardControls::MoveLeft]) { moveVec += moveStep * mRight;     cameraDirty = true; }
        if (mKeyboardState[KeyboardControls::MoveRight]) { moveVec -= moveStep * mRight;     cameraDirty = true; }
        if (mKeyboardState[KeyboardControls::MoveUp]) { moveVec += moveStep * mUp;        cameraDirty = true; }
        if (mKeyboardState[KeyboardControls::MoveDown]) { moveVec -= moveStep * mUp;        cameraDirty = true; }

        if (cameraDirty)
        {
            mPositionTarget += moveVec;

            mat4 rotationMatrix =
                glm::rotate(mat4(1.0f), yaw, vec3(0, 1.0f, 0)) *
                glm::rotate(mat4(1.0f), pitch, mRightTarget) *
                glm::rotate(mat4(1.0f), roll, mDirectionTarget);

            mDirectionTarget = glm::normalize(vec3(rotationMatrix * vec4(mDirectionTarget, 0.f)));
            mUpTarget = glm::normalize(vec3(rotationMatrix * vec4(mUpTarget, 0.f)));
            mRightTarget = glm::normalize(glm::cross(mUpTarget, mDirectionTarget));

            mIsUnchanged = false;
        }

        mPosition = glm::mix(mPosition, mPositionTarget, t);
        mDirection = glm::normalize(glm::mix(mDirection, mDirectionTarget, t));
        mUp = vec3(0, 1, 0); //glm::normalize(glm::mix(mUp, mUpTarget, t));
        mRight = glm::normalize(glm::mix(mRight, mRightTarget, t));
    }

    // ===== FIX MODE ===== //
    else if (mCurrentMode == eCameraMode::IN_FIXED)
    {
        LookAt(view1Pos, view1Target);

        // Position fixe pour ce mode, mais on pourrait adapter
        mPosition = view1Pos;
        mDirection = normalize(view1Target - view1Pos);
        mRight = normalize(cross(vec3(0, 1, 0), mDirection));
        mUp = vec3(0, 1, 0); //normalize(cross(mDirection, mRight));

        mPositionTarget = mPosition;
        mDirectionTarget = mDirection;
        mUpTarget = mUp;
        mRightTarget = mRight;
    }

    // ===== FREE MODE (VIEW1/VIEW2) ===== //
    else if (mCurrentMode == eCameraMode::IN_FREE || mCurrentMode == eCameraMode::OUT_FREE)
    {
        bool cameraDirty = false;
        float yaw = 0, pitch = 0;

        if (mMouseButtonState[MouseButtons::Left] && (mouseMove.x || mouseMove.y))
        {
            yaw = -mRotateSpeed * mouseMove.x;
            pitch = mRotateSpeed * mouseMove.y;
            cameraDirty = true;
        }

        if (cameraDirty)
        {
            mat4 rotationMatrix = glm::rotate(mat4(1.0f), yaw, vec3(0, 1.0f, 0)) * glm::rotate(mat4(1.0f), pitch, mRightTarget);

            mDirectionTarget = glm::normalize(vec3(rotationMatrix * vec4(mDirectionTarget, 0.f)));
            mUpTarget = glm::normalize(vec3(rotationMatrix * vec4(mUpTarget, 0.f)));
            mRightTarget = glm::normalize(glm::cross(mUpTarget, mDirectionTarget));

            mIsUnchanged = false;
        }

        // Interpolation vers la target (à garder)
        mPosition = view1Pos;
        mDirection = glm::normalize(glm::mix(mDirection, mDirectionTarget, t));
        mUp = vec3(0, 1, 0); //glm::normalize(glm::mix(mUp, mUpTarget, t));
        mRight = glm::normalize(glm::mix(mRight, mRightTarget, t));
    }

    updateView();
}

// Get
float Camera::GetInterpolationValue(float t) const
{
    switch (mInterpolation) {
    case eInterpolation::Linear:      return LinInterp(t);
    case eInterpolation::SmoothStep:  return SmoothStepInterp(t);
    case eInterpolation::EaseIn:      return EaseInInterp(t);
    case eInterpolation::EaseOut:     return EaseOutInterp(t);
    case eInterpolation::EaseInOut:   return EaseInOutInterp(t);
    default:                             return t;
    }
}
float Camera::GetNorthAngleDEG()
{
    // Vector representing North (negative Z axis)
    vec3 north(0.0f, 0.0f, -1.0f);

    // Projection of the direction onto the XZ plane
    vec3 directionXZ(mDirection.x, 0.0f, mDirection.z);

    // Normalization of the projected vector
    directionXZ = normalize(directionXZ);

    // Calculating the angle between the projected direction and North
    float North = glm::orientedAngle(north, directionXZ, vec3(0.0f, 1.0f, 0.0f));

    // Converting angle to degrees
    North = degrees(North);

    North = 360.0f - North;

    // Adjusting the angle to always be positive (0-360)
    while (North < 0)
        North += 360.0f;
    while (North > 360.0f)
        North -= 360.0f;

    return North;
}
float Camera::GetAttitudeDEG()
{
    // Projection of mDirection onto the horizontal plane (XZ)
    vec3 horizontalDir = glm::normalize(vec3(mDirection.x, 0.0f, mDirection.z));

    // Calculating the angle between mDirection and its horizontal projection
    float pitchRadians = glm::acos(glm::dot(mDirection, horizontalDir));

    // Determine if the angle is positive or negative
    if (mDirection.y < 0) 
        pitchRadians = -pitchRadians;

    // Conversion to degrees
    float pitchDegrees = glm::degrees(pitchRadians);

    return pitchDegrees;
}
float Camera::GetRollDEG()
{
    // Camera "right" vector
    vec3 cameraRight = glm::normalize(glm::cross(mDirection, vec3(0.0f, 1.0f, 0.0f)));

    // Camera "up" vector in the plane perpendicular to mDirection
    vec3 cameraUpProjected = glm::normalize(glm::cross(mDirection, cameraRight));

    // Calculate the angle between cameraUpProjected and the world's "up" vector (0, 1, 0)
    float rollRadians = glm::acos(glm::dot(cameraUpProjected, vec3(0.0f, 1.0f, 0.0f)));

    // Determine if the angle is positive or negative
    if (glm::dot(cameraRight, mUp) < 0)
        rollRadians = -rollRadians;

    // Conversion to degrees
    float rollDegrees = glm::degrees(rollRadians);

    return rollDegrees;
}
bool Camera::IsInViewFrustum(const vec3& position)
{
    vec4 clipSpace = mMatViewProjection * vec4(position, 1.0f);
    return std::abs(clipSpace.x) <= clipSpace.w && std::abs(clipSpace.y) <= clipSpace.w && clipSpace.z >= -clipSpace.w && clipSpace.z <= clipSpace.w;
}
float Camera::GetHorizonViewportY() const
{
    // Position far away on the line of sight
    glm::vec3 distantPoint = mPosition + mZfar * mDirection;

    // Vertical projection on the ground (y=0)
    glm::vec3 horizonPoint = glm::vec3(distantPoint.x, 0.0f, distantPoint.z);

    // Passage into clip space (viewprojection)
    glm::vec4 clipPos = mMatViewProjection * glm::vec4(horizonPoint, 1.0f);

    // Homogeneous division
    if (clipPos.w != 0.0f)
        clipPos /= clipPos.w;

    // Clamp between -1 and 1 (vertical NDC viewport)
    float viewportY = glm::clamp(clipPos.y, -1.0f, 1.0f);

    return viewportY;
}

// Inputs
void Camera::KeyboardUpdate(int key, int scancode, int action, int mods)
{
    if (mKeyboardMap.find(key) == mKeyboardMap.end())
        return;

    auto cameraKey = mKeyboardMap.at(key);
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
        mKeyboardState[cameraKey] = true;
    else 
        mKeyboardState[cameraKey] = false;
}
void Camera::MousePosUpdate(double xpos, double ypos)
{
    mMousePos = { float(xpos), float(ypos) };
}
void Camera::MouseButtonUpdate(int button, int action, int mods)
{
    if (mMouseButtonMap.find(button) == mMouseButtonMap.end())
        return;

    auto cameraButton = mMouseButtonMap.at(button);
    if (action == GLFW_PRESS)
        mMouseButtonState[cameraButton] = true;
    else 
        mMouseButtonState[cameraButton] = false;
}

// Orbital mode
void Camera::SetOrbitalMode()
{
    mPreviousMode = mCurrentMode;
    mCurrentMode = eCameraMode::ORBITAL;

    vec3 direction = mTargetPos - mPosition;
    mOrbitRadius = length(direction);
    mOrbitYaw = -atan2(direction.z, direction.x);
    mOrbitPitch = -asin(direction.y / mOrbitRadius);
}
void Camera::SetTarget(const vec3& target)
{
    mTargetPos = target;
}
void Camera::AdjustOrbitRadius(float delta)
{
    if (mCurrentMode == eCameraMode::ORBITAL)
        mOrbitRadius = glm::max(0.1f, mOrbitRadius + delta);
}

void Camera::SetMode(eCameraMode mode)
{
    this->mPreviousMode = mCurrentMode;
    this->mCurrentMode = mode;

    if (mCurrentMode == eCameraMode::ORBITAL)
    {
        vec3 direction = mTargetPos - mPosition;
        mOrbitRadius = length(direction);
        mOrbitYaw = -atan2(direction.z, direction.x);
        mOrbitPitch = -asin(direction.y / mOrbitRadius);
    }
}
void Camera::ReturnToPreviousMode()
{
    mCurrentMode = mPreviousMode;

    if (mCurrentMode == eCameraMode::ORBITAL)
    {
        vec3 direction = mTargetPos - mPosition;
        mOrbitRadius = length(direction);
        mOrbitYaw = -atan2(direction.z, direction.x);
        mOrbitPitch = -asin(direction.y / mOrbitRadius);
    }
}

void Camera::updateView()
{
    mMatView = glm::lookAt(mPosition, mPosition + mDirection, mUp);
    mMatViewReflexion = glm::scale(mMatView, vec3(1.0f, -1.0f, 1.0f));
    updateViewProjection();
}
void Camera::updateViewProjection()
{
    mMatViewProjection = mMatProjection * mMatView;
}
