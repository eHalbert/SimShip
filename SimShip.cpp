/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#include "SimShip.h"

extern GLuint   TexWakeBuffer;
extern int      TexWakeBufferSize;
extern GLuint   TexWakeVao;
extern int      TexWakeVaoSize;
extern bool     bTexWakeByVAO;


// Callbacks
GLFWmonitor* get_current_monitor(GLFWwindow* window)
{
    int nmonitors, i;
    int wx, wy, ww, wh;
    int mx, my, mw, mh;
    int overlap, bestoverlap;
    GLFWmonitor* bestmonitor;
    GLFWmonitor** monitors;
    const GLFWvidmode* mode;

    bestoverlap = 0;
    bestmonitor = NULL;

    glfwGetWindowPos(window, &wx, &wy);
    glfwGetWindowSize(window, &ww, &wh);
    monitors = glfwGetMonitors(&nmonitors);

    for (i = 0; i < nmonitors; i++) 
    {
        mode = glfwGetVideoMode(monitors[i]);
        glfwGetMonitorPos(monitors[i], &mx, &my);
        mw = mode->width;
        mh = mode->height;

        overlap = std::max(0, std::min(wx + ww, mx + mw) - std::max(wx, mx)) * std::max(0, std::min(wy + wh, my + mh) - std::max(wy, my));

        if (bestoverlap < overlap) 
        {
            bestoverlap = overlap;
            bestmonitor = monitors[i];
        }
    }

    return bestmonitor;
}
void SwitchToFullScreen()
{
    static uint32_t oldWindowX;
    static uint32_t oldWindowY;
    static uint32_t oldWindowW;
    static uint32_t oldWindowH;

    if (!g_IsFullScreen)    // -> Switch to fullscreen
    {
        oldWindowX = g_WindowX;
        oldWindowY = g_WindowY;
        oldWindowW = g_WindowW;
        oldWindowH = g_WindowH;

        GLFWmonitor* targetMonitor = get_current_monitor(g_hWindow);
        const GLFWvidmode* mode = glfwGetVideoMode(targetMonitor);
        glfwSetWindowMonitor(g_hWindow, targetMonitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    else
    {
        int left, top, right, bottom;
        glfwGetWindowFrameSize(g_hWindow, &left, &top, &right, &bottom);

        glfwSetWindowMonitor(g_hWindow, 0, oldWindowX - left, oldWindowY - top, oldWindowW, oldWindowH, 0);// Restore old state
        glfwSetWindowPos(g_hWindow, oldWindowX, oldWindowY);
        glfwSetWindowSize(g_hWindow, oldWindowW, oldWindowH);
    }

    g_IsFullScreen = !g_IsFullScreen;
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // Callback function for window resizing

    g_WindowW = width;
    g_WindowH = height;
    g_WindowW_2 = width / 2;
    g_WindowH_2 = height / 2;

    glViewport(0, 0, g_WindowW, g_WindowH);
    g_Camera.SetViewportSize(g_WindowW, g_WindowH);

    // Resize the reflection textures
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_REFLECTION);
    
    glBindTexture(GL_TEXTURE_2D, TexReflectionColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_WindowW, g_WindowH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TexReflectionColor, 0);

    glBindTexture(GL_TEXTURE_2D, TexReflectionDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, g_WindowW, g_WindowH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, TexReflectionDepth, 0);

    // Checking framebuffer status after update
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) 
        cout << "Error creating FBO for the reflection" << endl;

    // Resize scene render textures
    glBindFramebuffer(GL_FRAMEBUFFER, msFBO_SCENE);
    
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msTexSceneColor);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, g_WindowW, g_WindowH, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msTexSceneColor, 0);

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msTexSceneDepth);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_DEPTH_COMPONENT32F, g_WindowW, g_WindowH, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, msTexSceneDepth, 0);

    // Checking framebuffer status after update
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "Error creating FBO for the scene" << endl;
    
    // Resize post processing textures
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_SCENE);
    
    glBindTexture(GL_TEXTURE_2D, TexSceneColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_WindowW, g_WindowH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TexSceneColor, 0);

    glBindTexture(GL_TEXTURE_2D, TexSceneDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, g_WindowW, g_WindowH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, TexSceneDepth, 0);

    // Checking framebuffer status after update
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "Error creating FBO for the post processing" << endl;

    // FBO normal for post processing ================================================

    glBindFramebuffer(GL_FRAMEBUFFER, FBO_POST);

    glBindTexture(GL_TEXTURE_2D, TexPostColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_WindowW, g_WindowH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TexPostColor, 0);

    glBindTexture(GL_TEXTURE_2D, TexPostDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, g_WindowW, g_WindowH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, TexPostDepth, 0);

    // FBO Verification
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "Error creating FBO for the post processing" << endl;

    // FBO normal for rain & binoculars ================================================

    glBindFramebuffer(GL_FRAMEBUFFER, FBO_RAIN);

    glBindTexture(GL_TEXTURE_2D, TexRainColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_WindowW, g_WindowH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TexRainColor, 0);

    glBindTexture(GL_TEXTURE_2D, TexRainDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, g_WindowW, g_WindowH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, TexRainDepth, 0);

    // Revert to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    g_Clouds.release();
    g_Clouds = make_unique<Clouds>(g_WindowW, g_WindowH, g_TWS_Kn);
    int sunHour = g_Sky->SunHour;
    int sunMinute = g_Sky->SunMinute;
    g_Sky.release();
    g_Sky = make_unique<Sky>(g_InitialPosition, g_WindowW, g_WindowH);
    g_Sky->SetTime(sunHour, sunMinute);
    g_Ocean.release();
    g_Ocean = make_unique<Ocean>(g_Wind, g_Sky.get());
    g_Ship->SetOcean(g_Ocean.get());
}
void cursor_pos_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    g_Camera.MousePosUpdate(xposIn, yposIn);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    vec2 mouse(mouseX, mouseY);
    if (IsInRect(g_CtrlPanel, mouse))
    {
        if (IsInRect(g_CtrlThrottle1, mouse))
        {
            if (yoffset > 0)
            {
                g_Ship->PowerCurrentStep1++;
                if (g_Ship->PowerCurrentStep1 > g_Ship->ship.PowerStepMax)
                    g_Ship->PowerCurrentStep1 = g_Ship->ship.PowerStepMax;
            }
            else
            {
                g_Ship->PowerCurrentStep1--;
                if (g_Ship->PowerCurrentStep1 < -g_Ship->ship.PowerStepMax)
                    g_Ship->PowerCurrentStep1 = -g_Ship->ship.PowerStepMax;
            }
            return;
        }
        if (IsInRect(g_CtrlThrottle2, mouse))
        {
            if (yoffset > 0)
            {
                g_Ship->PowerCurrentStep2++;
                if (g_Ship->PowerCurrentStep2 > g_Ship->ship.PowerStepMax)
                    g_Ship->PowerCurrentStep2 = g_Ship->ship.PowerStepMax;
            }
            else
            {
                g_Ship->PowerCurrentStep2--;
                if (g_Ship->PowerCurrentStep2 < -g_Ship->ship.PowerStepMax)
                    g_Ship->PowerCurrentStep2 = -g_Ship->ship.PowerStepMax;
            }
            return;
        }
        else if (IsInRect(g_CtrlRudder, mouse))
        {
            if (yoffset < 0)
            {
                if (g_Ship->bAutopilot) g_Ship->bAutopilot = false;
                g_Ship->RudderCurrentStep++;
                if (g_Ship->RudderCurrentStep > g_Ship->ship.RudderStepMax)
                    g_Ship->RudderCurrentStep = g_Ship->ship.RudderStepMax;
            }
            else
            {
                if (g_Ship->bAutopilot) g_Ship->bAutopilot = false;
                g_Ship->RudderCurrentStep--;
                if (g_Ship->RudderCurrentStep < -g_Ship->ship.RudderStepMax)
                    g_Ship->RudderCurrentStep = -g_Ship->ship.RudderStepMax;
            }
            return;
        }
        else if (IsInRect(g_CtrlBowThruster, mouse))
        {
            if (yoffset > 0)
            {
                if (g_Ship->bAutopilot) g_Ship->bAutopilot = false;
                g_Ship->BowThrusterCurrentStep++;
                if (g_Ship->BowThrusterCurrentStep > g_Ship->ship.BowThrusterStepMax)
                    g_Ship->BowThrusterCurrentStep = g_Ship->ship.BowThrusterStepMax;
            }
            else
            {
                if (g_Ship->bAutopilot) g_Ship->bAutopilot = false;
                g_Ship->BowThrusterCurrentStep--;
                if (g_Ship->BowThrusterCurrentStep < -g_Ship->ship.BowThrusterStepMax)
                    g_Ship->BowThrusterCurrentStep = -g_Ship->ship.BowThrusterStepMax;
            }
            return;
        }
        else if (IsInRect(g_CtrlSternThruster, mouse))
        {
            if (yoffset > 0)
            {
                if (g_Ship->bAutopilot) g_Ship->bAutopilot = false;
                g_Ship->SternThrusterCurrentStep++;
                if (g_Ship->SternThrusterCurrentStep > g_Ship->ship.SternThrusterStepMax)
                    g_Ship->SternThrusterCurrentStep = g_Ship->ship.SternThrusterStepMax;
            }
            else
            {
                if (g_Ship->bAutopilot) g_Ship->bAutopilot = false;
                g_Ship->SternThrusterCurrentStep--;
                if (g_Ship->SternThrusterCurrentStep < -g_Ship->ship.SternThrusterStepMax)
                    g_Ship->SternThrusterCurrentStep = -g_Ship->ship.SternThrusterStepMax;
            }
            return;
        }
        else if (IsInRect(g_CtrlTimeHour, mouse))
        {
            sHM hm = g_Sky->GetTime();
            if (yoffset < 0)
            {
                hm.hour--;
                if (hm.hour < 0.0f)
                    hm.hour = 0.0f;
            }
            else
            {
                hm.hour++;
                if (hm.hour > 23.0f)
                    hm.hour = 23.0f;
            }
            g_Sky->SetTime(hm.hour, hm.minute);
            g_Ship->bLights = g_Sky->SunPosition.y < 0.0f ? true : false;
			g_Traffics->bLights = g_Ship->bLights;
            return;
        }
        else if (IsInRect(g_CtrlTimeMinute, mouse))
        {
            sHM hm = g_Sky->GetTime();
            if (yoffset < 0)
            {
                hm.minute--;
                if (hm.minute < 0)
                {
                    if (hm.hour > 0)
                    {
                        hm.minute = 59;
                        hm.hour--;
                    }
                    else
                        hm.minute = 0;
                }
            }
            else
            {
                hm.minute++;
                if (hm.minute > 59)
                {
                    if (hm.hour < 23)
                    {
                        hm.minute = 0;
                        hm.hour++;
                    }
                    else
                        hm.minute = 59;
                }
            }
            g_Sky->SetTime(hm.hour, hm.minute);
            g_Ship->bLights = g_Sky->SunPosition.y < 0.0f ? true : false;
            g_Traffics->bLights = g_Ship->bLights;
            return;
        }
        else if (IsInRect(g_CtrlWind, mouse))
        {
            if (yoffset < 0)
                g_TWS_Kn--;
            else
                g_TWS_Kn++;
            g_TWS_Kn = glm::clamp(g_TWS_Kn, 1.0f, 30.0f);
            g_Wind = wind_from_speeddir(g_TWS_Deg, g_TWS_Kn);
            g_Ocean->GetWind(g_Wind);
            g_Ocean->InitFrequencies();
            g_Clouds->SetCloudSpeed(g_TWS_Kn);
            return;
        }
        else if (IsInRect(g_CtrlAutopilotM1, mouse))
        {
            if (yoffset < 0)
            {
                g_Ship->HDGInstruction--;
                if (g_Ship->HDGInstruction < 0)
                    g_Ship->HDGInstruction += 360;
            }
            if (yoffset > 0)
            {
                g_Ship->HDGInstruction++;
                if (g_Ship->HDGInstruction > 360)
                    g_Ship->HDGInstruction -= 360;
            }
            return;
        }
        if (IsInRect(g_CtrlAutopilotP1, mouse))
        {
            if (yoffset < 0)
            {
                g_Ship->HDGInstruction--;
                if (g_Ship->HDGInstruction < 0)
                    g_Ship->HDGInstruction += 360;
            }
            if (yoffset > 0)
            {
                g_Ship->HDGInstruction++;
                if (g_Ship->HDGInstruction > 360)
                    g_Ship->HDGInstruction -= 360;
            }
            return;
        }
        if (IsInRect(g_CtrlAutopilotM10, mouse))
        {
            if (yoffset < 0)
            {
                g_Ship->HDGInstruction -= 10;
                if (g_Ship->HDGInstruction < 0)
                    g_Ship->HDGInstruction += 360;
            }
            if (yoffset > 0)
            {
                g_Ship->HDGInstruction += 10;
                if (g_Ship->HDGInstruction > 360)
                    g_Ship->HDGInstruction -= 360;
            }
            return;
        }
        if (IsInRect(g_CtrlAutopilotP10, mouse))
        {
            if (yoffset < 0)
            {
                g_Ship->HDGInstruction -= 10;
                if (g_Ship->HDGInstruction < 0)
                    g_Ship->HDGInstruction += 360;
            }
            if (yoffset > 0)
            {
                g_Ship->HDGInstruction += 10;
                if (g_Ship->HDGInstruction > 360)
                    g_Ship->HDGInstruction -= 360;
            }
            return;
        }
    }
    else if (!ImGui::GetIO().WantCaptureMouse)
    {
        if (g_Camera.GetMode() == eCameraMode::ORBITAL)
            g_Camera.AdjustOrbitRadius(-yoffset * 0.1f * g_Camera.GetOrbitRadius());
    }
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    vec2 mouse = vec2(int(xpos), int(ypos));
    if (button == 0 && action == GLFW_PRESS)
    {
        if (IsInRect(g_CtrlAutopilotCMD, mouse))
        {
            g_Ship->bAutopilot = !g_Ship->bAutopilot;
            return;
        }
        if (IsInRect(g_CtrlAutopilotM1, mouse))
        {
            g_Ship->HDGInstruction--;
            if (g_Ship->HDGInstruction < 0)
                g_Ship->HDGInstruction += 360;
            return;
        }
        if (IsInRect(g_CtrlAutopilotP1, mouse))
        {
            g_Ship->HDGInstruction++;
            if (g_Ship->HDGInstruction > 360)
                g_Ship->HDGInstruction -= 360;
            return;
        }
        if (IsInRect(g_CtrlAutopilotM10, mouse))
        {
            g_Ship->HDGInstruction -= 10;
            if (g_Ship->HDGInstruction < 0)
                g_Ship->HDGInstruction += 360;
            return;
        }
        if (IsInRect(g_CtrlAutopilotP10, mouse))
        {
            g_Ship->HDGInstruction += 10;
            if (g_Ship->HDGInstruction > 360)
                g_Ship->HDGInstruction -= 360;
            return;
        }
        if (IsInRect(g_CtrlThrottle1, mouse))
        {
            float valeur = 1 - (mouse.y - g_CtrlThrottleHigh1) / (g_CtrlThrottleLow1 - g_CtrlThrottleHigh1);
            g_Ship->PowerCurrentStep1 = (valeur - 0.5f) * (2.0f * g_Ship->ship.PowerStepMax);
        }
        if (IsInRect(g_CtrlThrottle2, mouse))
        {
            float valeur = 1 - (mouse.y - g_CtrlThrottleHigh2) / (g_CtrlThrottleLow2 - g_CtrlThrottleHigh2);
            g_Ship->PowerCurrentStep2 = (valeur - 0.5f) * (2.0f * g_Ship->ship.PowerStepMax);
        }
        if (IsInRect(g_CtrlBowThruster, mouse))
        {
            float valeur = (mouse.x - g_CtrlBowThrusterLeft) / (g_CtrlBowThrusterRight - g_CtrlBowThrusterLeft);
            g_Ship->BowThrusterCurrentStep = (valeur - 0.5f) * (2.0f * g_Ship->ship.BowThrusterStepMax);
        }
        if (IsInRect(g_CtrlSternThruster, mouse))
        {
            float valeur = (mouse.x - g_CtrlSternThrusterLeft) / (g_CtrlSternThrusterRight - g_CtrlSternThrusterLeft);
            g_Ship->SternThrusterCurrentStep = (valeur - 0.5f) * (2.0f * g_Ship->ship.SternThrusterStepMax);
        }
        if (IsInRect(g_CtrlRudder, mouse))
        {
            float valeur = 1 - (mouse.x - g_CtrlRudderLeft) / (g_CtrlRudderRight - g_CtrlRudderLeft);
            g_Ship->RudderCurrentStep = (valeur - 0.5f) * (2.0f * g_Ship->ship.RudderStepMax);
        }
        if (IsInRect(g_CtrlNow, mouse))
        {
            g_Sky->SetNow();
        }
        if (IsInRect(g_CtrlSound, mouse))
        {
            g_SoundMgr->bSound = !g_SoundMgr->bSound;
        }
        if (IsInRect(g_CtrlTimer, mouse))
        {
            g_ChronoStep++;
            if (g_ChronoStep > 2) g_ChronoStep = 0;
            switch (g_ChronoStep)
            {
            case 0: g_bChrono = true; g_Chrono.restart();  break;
            case 1: g_Chrono.stop(); break;
            case 2: g_bChrono = false; break;
            }
        }
        if (IsInRect(g_CtrlMto1, mouse))
            SetMeteo(1);
        if (IsInRect(g_CtrlMto2, mouse))
            SetMeteo(2);
        if (IsInRect(g_CtrlMto3, mouse))
            SetMeteo(3);
    }
   
    if (button == 1 && action == GLFW_PRESS)
        g_bBinoculars = !g_bBinoculars;

    if (!ImGui::GetIO().WantCaptureMouse)
        g_Camera.MouseButtonUpdate(button, action, mods);
}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    g_Camera.KeyboardUpdate(key, scancode, action, mods);

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        // A key has just been pressed
        switch (key) 
        {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;
        // Pause
        case GLFW_KEY_SPACE:
            g_bPause = !g_bPause;
            if (g_bPause)   g_TimerShipMotion.stop();
            else            g_TimerShipMotion.start();
            break;
        case GLFW_KEY_1:
            SetShip(0);
            break;
        case GLFW_KEY_2:
            SetShip(1);
            break;
        case GLFW_KEY_3:
            SetShip(2);
            break;
        case GLFW_KEY_4:
            SetShip(3);
            break;
        case GLFW_KEY_5:
            SetShip(4);
            break;
        case GLFW_KEY_6:
            SetShip(5);
            break;
        case GLFW_KEY_7:
            SetShip(6);
            break;
        case GLFW_KEY_8:
            SetShip(7);
            break;
        case GLFW_KEY_9:
            SetShip(8);
            break;
        case GLFW_KEY_0:
            SetShip(9);
            break;
        // Bridge views
        case GLFW_KEY_S:
            if (g_Camera.GetMode() != eCameraMode::FPS)
            {
                g_eBridgeView = eBridgeView::WHEEL;
                g_Camera.KeyboardUpdate(GLFW_KEY_B, scancode, action, mods);
            }
            break;
        case GLFW_KEY_A:
            if (g_Camera.GetMode() != eCameraMode::FPS)
            {
                g_eBridgeView = eBridgeView::LEFT;
                g_Camera.KeyboardUpdate(GLFW_KEY_B, scancode, action, mods);
            }
            break;
        case GLFW_KEY_D:
            if (g_Camera.GetMode() != eCameraMode::FPS)
            {
                g_eBridgeView = eBridgeView::RIGHT;
                g_Camera.KeyboardUpdate(GLFW_KEY_B, scancode, action, mods);
            }
            break;
        case GLFW_KEY_W:
            if (g_Camera.GetMode() != eCameraMode::FPS)
            {
                g_eBridgeView = eBridgeView::BOW;
                g_Camera.KeyboardUpdate(GLFW_KEY_B, scancode, action, mods);
            }
            break;
        case GLFW_KEY_X:
            if (g_Camera.GetMode() != eCameraMode::FPS)
            {
                g_eBridgeView = eBridgeView::STERN;
                g_Camera.KeyboardUpdate(GLFW_KEY_B, scancode, action, mods);
            }
            break;
        // Horn
        case GLFW_KEY_H:
            if (g_SoundMgr->bSound && g_Ship->bSound)
                g_SoundHorn->play();
            break;
        case GLFW_KEY_I:
        {
            // Cycle the interpolation function to the next type
            int current = static_cast<int>(g_Camera.GetInterpolation());
            int next = (current + 1) % static_cast<int>(eInterpolation::COUNT);
            g_Camera.SetInterpolation(static_cast<eInterpolation>(next));
        }
        break;
        case GLFW_KEY_KP_SUBTRACT:
            g_Ship->SurgeVelocity -= 0.5144f;   // 1 kn
            break;
        case GLFW_KEY_KP_ADD:
            g_Ship->SurgeVelocity += 0.5144f;   // 1 kn
            break;
        case GLFW_KEY_KP_DIVIDE:
            g_Ship->PowerCurrentStep1 = g_Ship->ship.PowerStepMax * 7 / 10;
            g_Ship->PowerCurrentStep2 = g_Ship->ship.PowerStepMax * 7 / 10;
            break;
        case GLFW_KEY_KP_MULTIPLY:
            g_Ship->SurgeVelocity = knot_to_ms(g_Ship->ship.SpeedTestKn);
            break;
        case GLFW_KEY_L:
            g_Ship->bLights = !g_Ship->bLights;
            g_Traffics->bLights = g_Ship->bLights;
            break;
        case GLFW_KEY_N:
            g_bNightVision = !g_bNightVision;
            break;
        // Textures
        case GLFW_KEY_T:
            g_bTextureDisplay = !g_bTextureDisplay;
            break;
        case GLFW_KEY_Y:
            g_bTextureWakeDisplay = !g_bTextureWakeDisplay;
            break;
        // Rudder
        case GLFW_KEY_LEFT:
            if (g_Ship->bAutopilot) g_Ship->bAutopilot = false;
            g_Ship->RudderCurrentStep++;
            if (g_Ship->RudderCurrentStep > g_Ship->ship.RudderStepMax)
                g_Ship->RudderCurrentStep = g_Ship->ship.RudderStepMax;
            break;
        case GLFW_KEY_DOWN:
            if (g_Ship->bAutopilot) g_Ship->bAutopilot = false;
            g_Ship->RudderCurrentStep = 0;
            break;
        case GLFW_KEY_RIGHT:
            if (g_Ship->bAutopilot) g_Ship->bAutopilot = false;
            g_Ship->RudderCurrentStep--;
            if (g_Ship->RudderCurrentStep < -g_Ship->ship.RudderStepMax)
                g_Ship->RudderCurrentStep = -g_Ship->ship.RudderStepMax;
            break;
        // Power LEFT
        case GLFW_KEY_KP_7:
            if (g_Ship->ship.nPropeller == 2)
            {
                g_Ship->PowerCurrentStep1++;
                if (g_Ship->PowerCurrentStep1 > g_Ship->ship.PowerStepMax)
                    g_Ship->PowerCurrentStep1 = g_Ship->ship.PowerStepMax;
            }
            break;
        case GLFW_KEY_KP_4:
            if (g_Ship->ship.nPropeller == 2)
            {
                g_Ship->PowerCurrentStep1 = 0;
            }
            break;
        case GLFW_KEY_KP_1:
            if (g_Ship->ship.nPropeller == 2)
            {
                g_Ship->PowerCurrentStep1--;
                if (g_Ship->PowerCurrentStep1 < -g_Ship->ship.PowerStepMax)
                    g_Ship->PowerCurrentStep1 = -g_Ship->ship.PowerStepMax;
            }
            break;
        // Power RIGHT
        case GLFW_KEY_KP_9:
            if (g_Ship->ship.nPropeller == 2)
            {
                g_Ship->PowerCurrentStep2++;
                if (g_Ship->PowerCurrentStep2 > g_Ship->ship.PowerStepMax)
                    g_Ship->PowerCurrentStep2 = g_Ship->ship.PowerStepMax;
            }
            break;
        case GLFW_KEY_KP_6:
            if (g_Ship->ship.nPropeller == 2)
            {
                g_Ship->PowerCurrentStep2 = 0;
            }
            break;
        case GLFW_KEY_KP_3:
            if (g_Ship->ship.nPropeller == 2)
            {
                g_Ship->PowerCurrentStep2--;
                if (g_Ship->PowerCurrentStep2 < -g_Ship->ship.PowerStepMax)
                    g_Ship->PowerCurrentStep2 = -g_Ship->ship.PowerStepMax;
            }
            break;
        // Power ALL
        case GLFW_KEY_KP_8:
            if (g_Ship->ship.nPropeller == 2)
            {
                int average = (g_Ship->PowerCurrentStep1 + g_Ship->PowerCurrentStep2) / 2;
                g_Ship->PowerCurrentStep1 = average + 1;
                if (g_Ship->PowerCurrentStep1 > g_Ship->ship.PowerStepMax)
                    g_Ship->PowerCurrentStep1 = g_Ship->ship.PowerStepMax;
                g_Ship->PowerCurrentStep2 = average + 1;
                if (g_Ship->PowerCurrentStep2 > g_Ship->ship.PowerStepMax)
                    g_Ship->PowerCurrentStep2 = g_Ship->ship.PowerStepMax;
            }
            else
            {
                g_Ship->PowerCurrentStep2++;
                if (g_Ship->PowerCurrentStep2 > g_Ship->ship.PowerStepMax)
                    g_Ship->PowerCurrentStep2 = g_Ship->ship.PowerStepMax;
            }
            break;
        case GLFW_KEY_KP_5:
            g_Ship->PowerCurrentStep1 = 0;
            g_Ship->PowerCurrentStep2 = 0;
            break;
        case GLFW_KEY_KP_2:
            if (g_Ship->ship.nPropeller == 2)
            {
                int average = (g_Ship->PowerCurrentStep1 + g_Ship->PowerCurrentStep2) / 2;
                g_Ship->PowerCurrentStep1 = average - 1;
                if (g_Ship->PowerCurrentStep1 < -g_Ship->ship.PowerStepMax)
                    g_Ship->PowerCurrentStep1 = -g_Ship->ship.PowerStepMax;
                g_Ship->PowerCurrentStep2 = average - 1;
                if (g_Ship->PowerCurrentStep2 < -g_Ship->ship.PowerStepMax)
                    g_Ship->PowerCurrentStep2 = -g_Ship->ship.PowerStepMax;
            }
            else
            {
                g_Ship->PowerCurrentStep2--;
                if (g_Ship->PowerCurrentStep2 < -g_Ship->ship.PowerStepMax)
                    g_Ship->PowerCurrentStep2 = -g_Ship->ship.PowerStepMax;
            }
            break;
        // Bow Thruster
        case GLFW_KEY_INSERT:
            if (g_Ship->ship.HasBowThruster)
            {
                g_Ship->BowThrusterCurrentStep--;
                if (g_Ship->BowThrusterCurrentStep < -g_Ship->ship.BowThrusterStepMax)
                    g_Ship->BowThrusterCurrentStep = -g_Ship->ship.BowThrusterStepMax;
            }
            break;
		case GLFW_KEY_HOME:
            if (g_Ship->ship.HasBowThruster)
            {
                g_Ship->BowThrusterCurrentStep = 0;
            }
			break;
        case GLFW_KEY_PAGE_UP:
            if (g_Ship->ship.HasBowThruster)
            {
                g_Ship->BowThrusterCurrentStep++;
                if (g_Ship->BowThrusterCurrentStep > g_Ship->ship.BowThrusterStepMax)
                    g_Ship->BowThrusterCurrentStep = g_Ship->ship.BowThrusterStepMax;
            }
            break;
        // Stern Thruster
        case GLFW_KEY_DELETE:
            if (g_Ship->ship.HasSternThruster)
            {
                g_Ship->SternThrusterCurrentStep--;
                if (g_Ship->SternThrusterCurrentStep < -g_Ship->ship.SternThrusterStepMax)
                    g_Ship->SternThrusterCurrentStep = -g_Ship->ship.SternThrusterStepMax;
            }
            break;
        case GLFW_KEY_END:
            if (g_Ship->ship.HasSternThruster)
            {
                g_Ship->SternThrusterCurrentStep = 0;
            }
            break;
        case GLFW_KEY_PAGE_DOWN:
            if (g_Ship->ship.HasSternThruster)
            {
                g_Ship->SternThrusterCurrentStep++;
                if (g_Ship->SternThrusterCurrentStep > g_Ship->ship.SternThrusterStepMax)
                    g_Ship->SternThrusterCurrentStep = g_Ship->ship.SternThrusterStepMax;
            }
            break;

        // Windows
        case GLFW_KEY_F1:
            g_bShowShortcuts = !g_bShowShortcuts;
            break;
        case GLFW_KEY_F2:
            g_bShowSceneWindow = !g_bShowSceneWindow;
            break;
        case GLFW_KEY_F3:
            g_bShowShipWindow = !g_bShowShipWindow;
            break;
        case GLFW_KEY_F4:
            g_bShowStatusBar = !g_bShowStatusBar;
            break;
        break;
        case GLFW_KEY_F5:
            g_bShowAutopilotWindow = !g_bShowAutopilotWindow;
            break;
        case GLFW_KEY_F6:
            g_bShowShipForcesWindows = !g_bShowShipForcesWindows;
            break;
        case GLFW_KEY_F8:
            g_CaptureName = SaveClientArea(g_hWnd);
            break;
        case GLFW_KEY_F11:
            SwitchToFullScreen();
            break;
        }
    }
    else if (action == GLFW_RELEASE) 
    {
        // A key has just been released
    }
    else if (action == GLFW_REPEAT) 
    {
        // A key is held down
    }
}
void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    // Ignore non-critical notifications
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;

    cout << "OpenGL Debug - ";

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         cout << "SEVERITY_HIGH"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       cout << "SEVERITY_MEDIUM"; break;
    case GL_DEBUG_SEVERITY_LOW:          cout << "SEVERITY_LOW"; break;
    default:                             cout << "SEVERITY_UNKNOWN"; break;
    }

    cout << endl;
    cout << "Message : " << message << endl;
}
void debugCallbackToFile(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    // Ignore non-critical notifications
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;

    // Opens the file in append mode
    static ofstream file("Outputs/opengl_log.txt", ios::app);
    if (!file.is_open()) return;

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         file << "SEVERITY_HIGH"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       file << "SEVERITY_MEDIUM"; break;
    case GL_DEBUG_SEVERITY_LOW:          file << "SEVERITY_LOW"; break;
    default:                             file << "SEVERITY_UNKNOWN"; break;
    }

    file << endl;
    file << "Message : " << message << endl;
}

#ifdef _DEBUG
//#define CONSOLE
#define DEMO // (Only Houat & HMS Clyde)
#endif

int main()
{
    // The real entry point is mainCRTStartup (see Linker / Advanced / Entry Point)

#ifdef CONSOLE
    InitConsole();
#endif

    // Initializing GLFW
    if (!glfwInit())
        return -1;  // GLFW initialization failed

    // GLFW Configuration
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    // Creating the window
    g_hWindow = glfwCreateWindow(g_WindowW, g_WindowH, "SimShip", NULL, NULL);
    if (g_hWindow == NULL)
    {
        glfwTerminate();
        return -1;  // Failed to create GLFW window
    }
    // Window in the middle of the screen
    int left, top, right, bottom;
    glfwGetWindowFrameSize(g_hWindow, &left, &top, &right, &bottom);
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    g_WindowX = (mode->width - g_WindowW) / 2;
    g_WindowY = (mode->height - g_WindowH) / 2;
    glfwSetWindowPos(g_hWindow, g_WindowX, g_WindowY + top);

    glfwMakeContextCurrent(g_hWindow);

    // Initializing GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return -1;  // GLAD initialization failed

    // Initializing extensions
    if (!gladLoadGL())
        return -1;  // Unable to load OpenGL extensions

    glEnable(GL_MULTISAMPLE);

    g_hWnd = glfwGetWin32Window(g_hWindow);

    // Viewport Configuration
    glViewport(0, 0, g_WindowW, g_WindowH);

    SetVsync((int)g_bVsync);

    // Defining the resize callback
    glfwSetFramebufferSizeCallback(g_hWindow, framebuffer_size_callback);
    glfwSetCursorPosCallback(g_hWindow, cursor_pos_callback);
    glfwSetScrollCallback(g_hWindow, scroll_callback);
    glfwSetMouseButtonCallback(g_hWindow, mouse_button_callback);
    glfwSetKeyCallback(g_hWindow, key_callback);
    //glfwSetInputMode(g_hWindow, GLFW_STICKY_KEYS, GLFW_TRUE);

#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debugCallback, nullptr);
    //glDebugMessageCallback(debugCallbackToFile, nullptr);
#endif

    //GetOpenGLInfo();

    InitNanoVg();
    RenderSplashScreen("Interface ...");

    InitImGUI();
    InitFPSCounter();
    InitScene();
    //DisplayWaveParametersFromModels(knot_to_ms(g_TWS_Kn), 50000.0);

    // Main rendering loop
    while (!glfwWindowShouldClose(g_hWindow))
    {
        float time = g_TimeSpeed * g_TimerShipMotion.getTime();
        
        // Camera zoom
        static float fov = g_Camera.GetZoom();
        if (g_bBinoculars && g_Camera.GetPosition().y > 0.0f)
            g_Camera.SetZoom(45.0f / 7.0f);
        else
            g_Camera.SetZoom(fov);

        // Camera update
        if (g_vShips.size() && g_NoShip >= 0 && g_NoShip < g_vShips.size())
        {
            vec3 orbitalTarget = g_Ship->ship.Position + vec3(0.0f, 10.0f, 0.0f);
            vec3 viewPos = vec3(0.0f);
            switch (g_eBridgeView)
            {
            case eBridgeView::WHEEL:    viewPos = g_Ship->TransformPosition(g_vShips[g_NoShip].ViewWheel); break;
            case eBridgeView::LEFT:     viewPos = g_Ship->TransformPosition(g_vShips[g_NoShip].ViewLeft); break;
            case eBridgeView::RIGHT:    viewPos = g_Ship->TransformPosition(g_vShips[g_NoShip].ViewRight); break;
            case eBridgeView::BOW:      viewPos = g_Ship->TransformPosition(g_vShips[g_NoShip].ViewBow); break;
            case eBridgeView::STERN:    viewPos = g_Ship->TransformPosition(g_vShips[g_NoShip].ViewStern); break;
            }
            // Views -> Look forward
            vec3 viewTarget = g_Ship->TransformPosition(vec3(1000.0f, viewPos.y, 0.0f)) - g_Ship->ship.Position;
            // View -> Look back
            if (g_eBridgeView == eBridgeView::STERN)
                viewTarget = g_Ship->TransformPosition(vec3(-1000.0f, viewPos.y, 0.0f)) - g_Ship->ship.Position;
            g_Camera.Animate(time, orbitalTarget, viewPos, viewTarget);
        }
        else
        {
            vec3 pos = vec3(0.0f);
            vec3 p = vec3(0.0f);
            vec3 t = vec3(0.0f);
            g_Camera.Animate(time, pos, p, t);
        }

        // Updates
        if (g_Ocean)    g_Ocean->Update(time);
        if (g_Ship)     g_Ship->Update(time);
        if (g_Traffics) g_Traffics->Update(time);
        CheckCrossingPort();

        g_Ocean->GetRecordFromBuoy(vec2(0.0f, 0.0f), time);
        UpdateFPS();
        UpdateSounds();

        // Render all
        Render();

        SendNMEA(time);

        // Buffer swapping and event handling
        glfwSwapBuffers(g_hWindow);
        glfwPollEvents();
    }

    // Cleaning
	SAFE_DELETE(g_SoundMgr);
    nvgDeleteGL3(g_Nvg);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    glDeleteFramebuffers(1, &FBO_REFLECTION);
    glDeleteTextures(1, &TexReflectionColor);
    glDeleteTextures(1, &TexReflectionDepth);

    glDeleteFramebuffers(1, &msFBO_SCENE);
    glDeleteTextures(1, &msTexSceneColor);
    glDeleteTextures(1, &msTexSceneDepth);

    glDeleteFramebuffers(1, &FBO_SCENE);
    glDeleteTextures(1, &TexSceneColor);
    glDeleteTextures(1, &TexSceneDepth);

    //system("ffmpeg -y -i ./Outputs/fold%04d.png fold.mp4");
    return 0;
}

// Init
void InitScene()
{
    LoadPositions();

    // Camera
    g_Camera.SetProjection(45.0f, g_WindowW, g_WindowH, 0.5f, 30000.f);
    g_Camera.LookAt(vec3(-20.0f, 10.0f, 100.0f), vec3(0.0f, 0.0f, 0.0f));
    g_Camera.SetSpeeds(0.01f, 0.0025f);
    g_Camera.SetMode(eCameraMode::ORBITAL);
    //mat4 proj = g_Camera.GetProjection();
    //PrintGlmMatrix(proj, "Camera Projection Matrix");

    // Wind
    g_TWS_Deg = 270.0f;
    g_TWS_Kn = 15.0f;
    g_Wind = wind_from_speeddir(g_TWS_Deg, g_TWS_Kn);
    
    // Sky & Clouds
    RenderSplashScreen("Sky ...");
    g_Sky = make_unique<Sky>(g_InitialPosition, g_WindowW, g_WindowH);
    RenderSplashScreen("Clouds ...");
    g_Clouds = make_unique<Clouds>(g_WindowW, g_WindowH, g_TWS_Kn);

    // Shaders
    g_ShaderSun = make_unique<Shader>("Resources/Misc/sun.vert", "Resources/Misc/sun.frag");              // Light from sun
    g_ShaderCamera = make_unique<Shader>("Resources/Misc/camera.vert", "Resources/Misc/camera.frag");     // Light from camera
    g_ShaderBackground = make_unique<Shader>("Resources/Clouds/background.vert", "Resources/Clouds/background.frag");     // To draw the clouds in a separate process
    g_ShaderPostProcessing = make_unique<Shader>("Resources/Misc/post_processing.vert", "Resources/Misc/post_processing.frag");   // Mist + fog + underwater
    g_ShaderFXAA = make_unique<Shader>("Resources/Misc/fxaa.vert", "Resources/Misc/fxaa.frag");
    g_ShaderRain = make_unique<Shader>("Resources/Sky/rain.vert", "Resources/Sky/rain.frag");
	g_ShaderRainVolume = make_unique<Shader>("Resources/Sky/rain_volume.vert", "Resources/Sky/rain_volume.frag");

    // Models
    LoadModels();

    // Oceans
    RenderSplashScreen("Ocean ...");
    g_Ocean = make_unique<Ocean>(g_Wind, g_Sky.get());
    g_Ocean->bVisible = true;

    // Quad for texture display
    g_QuadTexture = make_unique<QuadTexture>();
    
    // Timer
    g_TimerShipMotion.start();

    //Terrains
    RenderSplashScreen("Terrain ...");
    LoadTerrains();
    LoadPortContour();

    // Markup
    RenderSplashScreen("Markup ...");
    g_Markup = make_unique<Markup>(L"Resources/Terrains/Islands/Markup-BHH.xml");
    g_Lighthouses = make_unique<LighthouseMgr>(L"Resources/Terrains/Islands/Lighthouses-BHH.xml");

    // Sounds
    RenderSplashScreen("Sounds ...");
    LoadSounds();
    
    // Ships
    RenderSplashScreen("Ship ...");
    LoadShips();
    SetShip(6);
    g_Ship->bLights = g_Sky->SunPosition.y < 0.0f ? true : false;

    // Traffic
    RenderSplashScreen("Traffic ...");
    g_Traffics = make_unique<Traffics>("Resources/Traffic/traffic.xml");
    g_Traffics->bLights = g_Ship->bLights;

    glEnable(GL_PROGRAM_POINT_SIZE);
    InitFBO();

    g_UdpSender = make_unique<UdpSender>("127.0.0.1", 54000);
}
void InitFBO()
{
    // FBO for the reflection of the ship on the ocean ===============================

    glGenFramebuffers(1, &FBO_REFLECTION);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_REFLECTION);

    // Creating the reflection texture
    glGenTextures(1, &TexReflectionColor);
    glBindTexture(GL_TEXTURE_2D, TexReflectionColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_WindowW, g_WindowH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TexReflectionColor, 0);

    glGenTextures(1, &TexReflectionDepth);
    glBindTexture(GL_TEXTURE_2D, TexReflectionDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, g_WindowW, g_WindowH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, TexReflectionDepth, 0);

    // FBO Verification
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "Error creating FBO for the reflection" << endl;

    // FBO Multisample for the entire scene ===========================================
    
    glGenFramebuffers(1, &msFBO_SCENE);
    glBindFramebuffer(GL_FRAMEBUFFER, msFBO_SCENE);

    // Creating the multisample color texture
    glGenTextures(1, &msTexSceneColor);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msTexSceneColor);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, g_WindowW, g_WindowH, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msTexSceneColor, 0);

    // Creating the multisample depth texture
    glGenTextures(1, &msTexSceneDepth);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msTexSceneDepth);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_DEPTH_COMPONENT32F, g_WindowW, g_WindowH, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, msTexSceneDepth, 0);

    GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);
    
    // FBO Verification
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "Error creating FBO for the scene" << endl;

    // FBO normal (blit from the multisample FBO ================================================
    
    glGenFramebuffers(1, &FBO_SCENE);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_SCENE);

    // Create a non-multisample texture for color
    glGenTextures(1, &TexSceneColor);
    glBindTexture(GL_TEXTURE_2D, TexSceneColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_WindowW, g_WindowH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TexSceneColor, 0);

    // Create a non-multisample texture for depth
    glGenTextures(1, &TexSceneDepth);
    glBindTexture(GL_TEXTURE_2D, TexSceneDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, g_WindowW, g_WindowH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, TexSceneDepth, 0);
    
    glDrawBuffers(1, drawBuffers);

    // FBO Verification
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "Error creating FBO for the post processing" << endl;
    
    // FBO normal for post processing ================================================

    glGenFramebuffers(1, &FBO_POST);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_POST);

    // Create a non-multisample texture for color
    glGenTextures(1, &TexPostColor);
    glBindTexture(GL_TEXTURE_2D, TexPostColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_WindowW, g_WindowH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TexPostColor, 0);

    // Create a non-multisample texture for depth
    glGenTextures(1, &TexPostDepth);
    glBindTexture(GL_TEXTURE_2D, TexPostDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, g_WindowW, g_WindowH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, TexPostDepth, 0);

    glDrawBuffers(1, drawBuffers);

    // FBO Verification
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "Error creating FBO for the post processing" << endl;
    
    // FBO normal for rain & binoculars ================================================

    glGenFramebuffers(1, &FBO_RAIN);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_RAIN);

    // Create a non-multisample texture for color
    glGenTextures(1, &TexRainColor);
    glBindTexture(GL_TEXTURE_2D, TexRainColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_WindowW, g_WindowH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TexRainColor, 0);

    // Create a non-multisample texture for depth
    glGenTextures(1, &TexRainDepth);
    glBindTexture(GL_TEXTURE_2D, TexRainDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, g_WindowW, g_WindowH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, TexRainDepth, 0);

    glDrawBuffers(1, drawBuffers);

    // FBO Verification
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "Error creating FBO for the post processing" << endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// FBO ocean for rain volume ================================================

    // Création (même taille que msFBO_SCENE)
    glGenFramebuffers(1, &FBO_OCEAN_TEMP);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_OCEAN_TEMP);

    // Color
    glGenTextures(1, &TexOceanTempColor);
    glBindTexture(GL_TEXTURE_2D, TexOceanTempColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, g_WindowW, g_WindowH, 0, GL_RGBA, GL_FLOAT, NULL);

    // Depth
    glGenTextures(1, &TexOceanTempDepth);
    glBindTexture(GL_TEXTURE_2D, TexOceanTempDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, g_WindowW, g_WindowH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TexOceanTempColor, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, TexOceanTempDepth, 0);

    ////////////////////

    g_ScreenQuadPost = make_unique<ScreenQuad>();
    g_ScreenQuadCloud = make_unique<ScreenQuad>();
	g_ScreenQuadRainVolume = make_unique<ScreenQuad>();
}
void InitImGUI()
{
    // ImGui Configuration
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(g_hWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}
void InitNanoVg()
{
    g_Nvg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);

    HDC screen = GetDC(NULL);
    int dpiX = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(NULL, screen);
    g_DevicePixelRatio = dpiX / 96.0f;        // 96 DPI is the default for many systems

    int fontArial = nvgCreateFont(g_Nvg, "arial", "c:/Windows/Fonts/Arial.ttf");
    if (fontArial == -1)
        printf("Unable to load Arial font.\n");   // Handle font loading error

    int fontCaveat = nvgCreateFont(g_Nvg, "caveat", "Resources/Interface/Caveat.ttf");
    if (fontCaveat == -1)
        printf("Unable to load Caveat font.\n");

    g_NvgImgClock = LoadNvgIcon("Resources/Interface/clock_white.png");
    if (g_NvgImgClock == -1)
        printf("Unable to load clock png.\n");

    g_NvgImgSoundOff = LoadNvgIcon("Resources/Interface/volume_off_white.png");
    if (g_NvgImgSoundOff == -1)
        printf("Unable to load volume_off png.\n");

    g_NvgImgSoundUp = LoadNvgIcon("Resources/Interface/volume_up_white.png");
    if (g_NvgImgSoundUp == -1)
        printf("Unable to load volume_up png.\n");

    g_NvgImgTimer = LoadNvgIcon("Resources/Interface/timer_white.png");
    if (g_NvgImgTimer == -1)
        printf("Unable to load timer png.\n");

    g_NvgImgMto1 = LoadNvgIcon("Resources/Interface/clear.png");
    if (g_NvgImgMto1 == -1)
        printf("Unable to load clear png.\n");

    g_NvgImgMto2 = LoadNvgIcon("Resources/Interface/cloud.png");
    if (g_NvgImgMto2 == -1)
        printf("Unable to load cloud png.\n");

    g_NvgImgMto3 = LoadNvgIcon("Resources/Interface/foggy.png");
    if (g_NvgImgMto3 == -1)
        printf("Unable to load foggy png.\n");
}
void InitFPSCounter()
{
    QueryPerformanceFrequency(&g_Frequency);
    QueryPerformanceCounter(&g_LastTime);
}
int  LoadNvgIcon(string name)
{
    // Load the image with stbi
    int wi, hi, n, image;
    unsigned char* img;
    stbi_set_unpremultiply_on_load(1);
    stbi_convert_iphone_png_to_rgb(1);
    img = stbi_load(name.c_str(), &wi, &hi, &n, 4);
    if (img == NULL)
    {
        // printf("Failed to load %s - %s\n", filename, stbi_failure_reason());
        return -1;
    }
    image = nvgCreateImageRGBA(g_Nvg, wi, hi, 0, img);
    stbi_image_free(img);
    return image;
}

void LoadPositions()
{
    if (g_vPositions.size() != 0)
        return;

    const char* filename = "Resources/Terrains/Positions.xml";
    pugi::xml_document doc;
    if (doc.load_file(filename))
    {
        auto root = doc.child(L"Positions");
        g_NoPosition = root.child(L"indexPosition").text().as_int();

        g_vPositions.clear();
        for (auto node : root.children(L"Position"))
        {
            sPositions p;
            p.name = wstring_to_utf8(node.attribute(L"name").as_string());
            p.pos.x = node.attribute(L"lon").as_float();
            p.pos.y = node.attribute(L"lat").as_float();
            p.heading = node.attribute(L"heading").as_float();
            g_vPositions.push_back(p);
        }

        if (g_NoPosition > g_vPositions.size())
            g_NoPosition = g_vPositions.size() - 1;
    }
    else
    {
        sPositions p;
        p.name = "Treac'h er Goured";
        p.pos.x = -2.94097114;
        p.pos.y = 47.3816223;
        p.heading = 90;
        g_vPositions.push_back(p);
        g_NoPosition = 0;
    }
}
void SetPosition()
{
    g_Ship->ship.Position = lonlat_to_opengl(g_vPositions[g_NoPosition].pos.x, g_vPositions[g_NoPosition].pos.y);
    g_Ship->SetYawFromHDG(g_vPositions[g_NoPosition].heading);
    g_Ship->ResetVelocities();
    g_Ship->PowerCurrentStep1 = 0;
    g_Ship->PowerCurrentStep2 = 0;
    g_Ship->PropRpm1 = 0.0f;
    g_Ship->PropRpm2 = 0.0f;
    g_Ship->RudderCurrentStep = 0;
    g_Ship->RudderAngleDeg = 0.0f;

    g_Ship->bVisible = true;
    g_Camera.SetFirstUpdate(true);
    if (g_Camera.GetMode() == eCameraMode::BRIDGE)
        g_Camera.KeyboardUpdate(GLFW_KEY_B, 0, 1, 0);
    g_bReset = true;
}
void SetHeading(float heading)
{
    g_Ship->SetYawFromHDG(heading);
    g_Ship->ResetVelocities();
    g_bReset = true;
}
void LoadModels()
{
    // Balls
    g_StaticBall = make_unique<Sphere>(0.5f, 32); // Radius of 0.5 m (diameter of 1 m)
    g_StaticBall->bVisible = false;
    
    // Grid XZ
    g_Grid = make_unique<Grid>(10, 1);
    g_Grid->bVisible = true;

    // Axis
    g_Axe = make_unique<Cube>();
    g_Axe->bVisible = false;

    // Arrow for the wind
    g_ArrowWind = make_unique<Model>("Resources/Interface/direction_wind.glb");
    g_ArrowWind->bVisible = false;
}
void ReadObjHeader(sTerrain& terrain)
{
	ifstream file(terrain.file);
    string line;

    if (file.is_open()) 
    {
        while (std::getline(file, line)) 
        {
            istringstream iss(line);
            string token;
            iss >> token;

            if (token == "#") 
            {
                iss >> token;
                if (token == "Centre") 
                    iss >> terrain.center.x >> terrain.center.y;
                else if (token == "Corner") 
                {
                    iss >> token;
                    if (token == "NW")
                        iss >> terrain.xMin >> terrain.zMax;
                    else if (token == "SE") 
                        iss >> terrain.xMax >> terrain.zMin;
                }
            }
        }
        file.close();
        terrain.xMin = lon_to_opengl(terrain.xMin);
        terrain.xMax = lon_to_opengl(terrain.xMax);
        terrain.zMin = lat_to_opengl(terrain.zMin);
        terrain.zMax = lat_to_opengl(terrain.zMax);
    }
}
void LoadTerrains()
{
    if (g_vTerrains.size() == 0)
    {
        vector<string> files = ListFiles("Resources/Terrains/Islands/", ".obj");
        int n = 0;
        for (auto& file : files)
		{
            if (file.find("c1_") != string::npos)
            {
                g_idxHouat = n;
            }
            else
            {
#ifdef DEMO
                continue;
#endif
            }
            sTerrain terrain;
			terrain.file = file;
			ReadObjHeader(terrain);
			terrain.pos = lonlat_to_opengl(terrain.center.x, terrain.center.y);
			terrain.scale = vec3(1.0f);
			terrain.model = make_unique<Model>(terrain.file);
			g_vTerrains.push_back(move(terrain));
            n++;
		}
    }
    g_Port = make_unique<Model>("Resources/Terrains/Islands/port.gltf");
}
void LoadPortContour()
{
    string filename = "Resources/Terrains/Islands/Contour-port.txt";
    g_vPortLines.clear();

    ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    vector<vec3> vertices;
    string line;

    while (std::getline(file, line))
    {
        stringstream ss(line);
        string prefix;
        ss >> prefix;

        if (prefix == "v")
        {
            // Reading the coordinates of a vertex
            float x, y, z;
            ss >> x >> y >> z;
            vertices.emplace_back(x, y, z);
        }
        else if (prefix == "l") 
        {
            // Reading a line by vertex indices (1-based in the file)
            int idx1, idx2;
            ss >> idx1 >> idx2;
            // Validity check of indices (1-based in file -> 0-based in C++)
            if (idx1 > 0 && idx1 <= (int)vertices.size() && idx2 > 0 && idx2 <= (int)vertices.size()) 
            {
                sLine lineObj;
                lineObj.p1 = vec2(vertices[idx1 - 1].x, vertices[idx1 - 1].z);
                lineObj.p2 = vec2(vertices[idx2 - 1].x, vertices[idx2 - 1].z);
                g_vPortLines.push_back(lineObj);
            }
            else
            {
                std::cerr << "Invalid vertex index in a row: " << idx1 << ", " << idx2 << std::endl;
                return;
            }
        }
    }

    mat4 model = glm::translate(mat4(1.0f), g_vTerrains[g_idxHouat].pos);
    for (auto& line : g_vPortLines)
    {
        vec4 p_homo1(line.p1.x, 0.0f, line.p1.y, 1.0f);
        vec4 p_transformed1 = model * p_homo1;
        line.p1 = vec2(p_transformed1.x, p_transformed1.z);
        vec4 p_homo2(line.p2.x, 0.0f, line.p2.y, 1.0f);
        vec4 p_transformed2 = model * p_homo2;
        line.p2 = vec2(p_transformed2.x, p_transformed2.z);
    }
}
void LoadShips()
{
    vector<string> vFiles = ListFiles("Resources/Ships/", ".ini");

    g_vShips.clear();
    for (const auto& file : vFiles)
    {
        Ini ini;
        ini.Load(const_cast<wchar_t*>(utf8_to_wstring(file).c_str()));

        sShip ship;
        wstring ws = L"";
        
        // Files
        ship.ShortName = wstring_to_utf8(ini.GetString(L"Files",  L"ShortName", const_cast<wchar_t*>(ws.c_str())));
#ifdef DEMO
        if (ship.ShortName != "HMS Clyde")
            continue;
#endif
        ship.PathnameHull = wstring_to_utf8(ini.GetString(L"Files", L"PathnameHull", const_cast<wchar_t*>(ws.c_str())));
        ship.PathnameFull = wstring_to_utf8(ini.GetString(L"Files", L"PathnameFull", const_cast<wchar_t*>(ws.c_str())));
        ship.PathnamePropeller1 = wstring_to_utf8(ini.GetString(L"Files", L"PathnamePropeller1", const_cast<wchar_t*>(ws.c_str())));
        ship.PathnamePropeller2 = wstring_to_utf8(ini.GetString(L"Files", L"PathnamePropeller2", const_cast<wchar_t*>(ws.c_str())));
        ship.PathnameRudder = wstring_to_utf8(ini.GetString(L"Files", L"PathnameRudder", const_cast<wchar_t*>(ws.c_str())));
        ship.PathnameRadar1 = wstring_to_utf8(ini.GetString(L"Files", L"PathnameRadar1", const_cast<wchar_t*>(ws.c_str())));
        ship.PathnameRadar2 = wstring_to_utf8(ini.GetString(L"Files", L"PathnameRadar2", const_cast<wchar_t*>(ws.c_str())));
        ship.PathnameFlag = wstring_to_utf8(ini.GetString(L"Files", L"PathnameFlag", const_cast<wchar_t*>(ws.c_str())));
        ship.ThrustSound = wstring_to_utf8(ini.GetString(L"Files", L"ThrustSound", const_cast<wchar_t*>(ws.c_str())));
        ship.BowThrusterSound = wstring_to_utf8(ini.GetString(L"Files", L"BowThrusterSound", const_cast<wchar_t*>(ws.c_str())));
        ship.SternThrusterSound = wstring_to_utf8(ini.GetString(L"Files", L"SternThrusterSound", const_cast<wchar_t*>(ws.c_str())));
        
        // Positions
        ship.Position = ini.GetVec3(L"Positions", L"Position", ship.Position);
        ship.Rotation = ini.GetVec3(L"Positions", L"Rotation", ship.Rotation);
        ship.ViewWheel = ini.GetVec3(L"Positions", L"ViewWheel", ship.ViewWheel);
        ship.ViewLeft = ini.GetVec3(L"Positions", L"ViewLeft", ship.ViewLeft);
        ship.ViewRight = ini.GetVec3(L"Positions", L"ViewRight", ship.ViewRight);
        ship.ViewBow = ini.GetVec3(L"Positions", L"ViewBow", ship.ViewBow);
        ship.ViewStern = ini.GetVec3(L"Positions", L"ViewStern", ship.ViewStern);

        // Dimensions
        ship.Class = static_cast<eClass>(ini.GetInt(L"Dimensions", L"Class", static_cast<int>(ship.Class)));
        ship.Length = ini.GetFloat(L"Dimensions", L"Length", ship.Length);
        ship.SpeedMaxKn = ini.GetFloat(L"Dimensions", L"SpeedMaxKn", ship.SpeedMaxKn);
        ship.SpeedTestKn = ini.GetFloat(L"Dimensions", L"SpeedTestKn", ship.SpeedTestKn);
        ship.Mass_t = ini.GetFloat(L"Dimensions", L"Mass_t", ship.Mass_t);
        ship.PosGravity = ini.GetVec3(L"Dimensions", L"PosGravity", ship.PosGravity);
        ship.HeaveCoef = ini.GetFloat(L"Dimensions", L"HeaveCoef", ship.HeaveCoef);
        ship.FormFactor = ini.GetFloat(L"Dimensions", L"FormFactor", ship.FormFactor);
        ship.EnvMapFactor = ini.GetFloat(L"Dimensions", L"EnvMapFactor", ship.EnvMapFactor);
        ship.mAreaFront = ini.GetFloat(L"Dimensions", L"AreaFront", ship.mAreaFront);
        ship.mAreaFrontCenter = ini.GetVec3(L"Dimensions", L"AreaFrontCenter", ship.mAreaFrontCenter);
        ship.mAreaLat = ini.GetFloat(L"Dimensions", L"AreaLat", ship.mAreaLat);
        ship.mAreaLatCenter = ini.GetVec3(L"Dimensions", L"AreaLatCenter", ship.mAreaLatCenter);

        // Spray
        ship.SprayVerticalPerf = ini.GetFloat(L"Spray", L"SprayVerticalPerf", ship.SprayVerticalPerf);
        ship.SprayMultiplier = ini.GetInt(L"Spray", L"SprayMultiplier", ship.SprayMultiplier);
        ship.SprayLength = ini.GetFloat(L"Spray", L"SprayLength", ship.SprayLength);
        ship.SprayType = ini.GetInt(L"Spray", L"SprayType", ship.SprayType);

        // Rudder
        ship.PosRudder = ini.GetVec3(L"Rudder", L"PosRudder", ship.PosRudder);
        ship.RudderIncrement = ini.GetInt(L"Rudder", L"RudderIncrement", ship.RudderIncrement);
        ship.RudderStepMax = ini.GetInt(L"Rudder", L"RudderStepMax", ship.RudderStepMax);
        ship.RudderRotSpeed = ini.GetFloat(L"Rudder", L"RudderRotSpeed", ship.RudderRotSpeed);
        ship.nRudder = ini.GetInt(L"Rudder", L"nRudder", ship.nRudder);
        ship.PosRudder1 = ini.GetVec3(L"Rudder", L"PosRudder1", ship.PosRudder1);
        ship.PosRudder2 = ini.GetVec3(L"Rudder", L"PosRudder2", ship.PosRudder2);

        // Turning
        ship.TurningPerf = ini.GetFloat(L"Turning", L"TurningPerf", ship.TurningPerf);
        ship.TurningDragCoef = ini.GetFloat(L"Turning", L"TurningDragCoef", ship.TurningDragCoef);
        ship.RoTMax = ini.GetFloat(L"Turning", L"RoTMax", ship.RoTMax);
        ship.HighSpeedCoeff = ini.GetFloat(L"Turning", L"HighSpeedCoeff", ship.HighSpeedCoeff);
        ship.PivotFwd = ini.GetFloat(L"Turning", L"PivotFwd", ship.PivotFwd);
        ship.PivotBwd = ini.GetFloat(L"Turning", L"PivotBwd", ship.PivotBwd);
        ship.CentrifugalPerf = ini.GetFloat(L"Turning", L"CentrifugalPerf", ship.CentrifugalPerf);

        // Power
        ship.PosPower = ini.GetVec3(L"Power", L"PosPower", ship.PosPower);
        ship.PowerkW = ini.GetFloat(L"Power", L"PowerkW", ship.PowerkW);
        ship.PowerStepMax = ini.GetInt(L"Power", L"PowerStepMax", ship.PowerStepMax);

        // Propellers
        ship.PropRpmMax = ini.GetFloat(L"Propellers", L"PropRpmMax", ship.PropRpmMax);
        ship.PropRpmIncrement = ini.GetFloat(L"Propellers", L"PropRpmIncrement", ship.PropRpmIncrement);
        ship.nPropeller = ini.GetInt(L"Propellers", L"nPropeller", ship.nPropeller);
        ship.PosPropeller1 = ini.GetVec3(L"Propellers", L"PosPropeller1", ship.PosPropeller1);
        ship.mPropTorque1 = ini.GetFloat(L"Propellers", L"PropTorque1", ship.mPropTorque1);
        ship.PosPropeller2 = ini.GetVec3(L"Propellers", L"PosPropeller2", ship.PosPropeller2);
        ship.mPropTorque2 = ini.GetFloat(L"Propellers", L"PropTorque2", ship.mPropTorque2);
        ship.PropDiameter = ini.GetFloat(L"Propellers", L"PropDiameter", ship.PropDiameter);
        ship.WakeWidth = ini.GetFloat(L"Propellers", L"WakeWidth", ship.WakeWidth);

        // Chimneys
        ship.nChimney = ini.GetInt(L"Chimneys", L"nChimney", ship.nChimney);
        ship.PosChimney1 = ini.GetVec3(L"Chimneys", L"PosChimney1", ship.PosChimney1);
        ship.PosChimney2 = ini.GetVec3(L"Chimneys", L"PosChimney2", ship.PosChimney2);

        // Bow Thruster
        ship.HasBowThruster = ini.GetBoolean(L"BowThruster", L"HasBowThruster", ship.HasBowThruster);
        ship.PosBowThruster = ini.GetVec3(L"BowThruster", L"PosBowThruster", ship.PosBowThruster);
        ship.BowThrusterPerf = ini.GetFloat(L"BowThruster", L"BowThrusterPerf", ship.BowThrusterPerf);
        ship.BowThrusterPowerW = ini.GetFloat(L"BowThruster", L"BowThrusterPowerW", ship.BowThrusterPowerW);
        ship.BowThrusterStepMax = ini.GetInt(L"BowThruster", L"BowThrusterStepMax", ship.BowThrusterStepMax);
        ship.BowThrusterRpmMin = ini.GetFloat(L"BowThruster", L"BowThrusterRpmMin", ship.BowThrusterRpmMin);
        ship.BowThrusterRpmMax = ini.GetFloat(L"BowThruster", L"BowThrusterRpmMax", ship.BowThrusterRpmMax);
        ship.BowThrusterRpmIncrement = ini.GetFloat(L"BowThruster", L"BowThrusterRpmIncrement", ship.BowThrusterRpmIncrement);

        // Stern Thruster
        ship.HasSternThruster = ini.GetBoolean(L"SternThruster", L"HasSternThruster", ship.HasSternThruster);
        ship.PosSternThruster = ini.GetVec3(L"SternThruster", L"PosSternThruster", ship.PosSternThruster);
        ship.SternThrusterPerf = ini.GetFloat(L"SternThruster", L"SternThrusterPerf", ship.SternThrusterPerf);
        ship.SternThrusterPowerW = ini.GetFloat(L"SternThruster", L"SternThrusterPowerW", ship.SternThrusterPowerW);
        ship.SternThrusterStepMax = ini.GetInt(L"SternThruster", L"SternThrusterStepMax", ship.SternThrusterStepMax);
        ship.SternThrusterRpmMin = ini.GetFloat(L"SternThruster", L"SternThrusterRpmMin", ship.SternThrusterRpmMin);
        ship.SternThrusterRpmMax = ini.GetFloat(L"SternThruster", L"SternThrusterRpmMax", ship.SternThrusterRpmMax);
        ship.SternThrusterRpmIncrement = ini.GetFloat(L"SternThruster", L"SternThrusterRpmIncrement", ship.SternThrusterRpmIncrement);

        // Lights
        ship.LightPositions = ini.GetVec3Array(L"Lights", L"LightPositions", ship.LightPositions);
        ship.LightColors = ini.GetVec3Array(L"Lights", L"LightColors", ship.LightColors);

        // Radar
        ship.nRadar = ini.GetInt(L"Radar", L"nRadar", ship.nRadar);
        ship.PosRadar1 = ini.GetVec3(L"Radar", L"PosRadar1", ship.PosRadar1);
        ship.RotationRadar1 = ini.GetFloat(L"Radar", L"RotationRadar1", ship.RotationRadar1);
        ship.PosRadar2 = ini.GetVec3(L"Radar", L"PosRadar2", ship.PosRadar2);
        ship.RotationRadar2 = ini.GetFloat(L"Radar", L"RotationRadar2", ship.RotationRadar2);

        // Autopilot
        ship.BaseP = ini.GetFloat(L"Autopilot", L"BaseP", ship.BaseP);
        ship.BaseI = ini.GetFloat(L"Autopilot", L"BaseI", ship.BaseI);
        ship.BaseD = ini.GetFloat(L"Autopilot", L"BaseD", ship.BaseD);
        ship.MaxIntegral = ini.GetFloat(L"Autopilot", L"MaxIntegral", ship.MaxIntegral);

        // Waves
        ship.CenterFore = ini.GetFloat(L"Waves", L"CenterFore", ship.CenterFore);
        ship.BaseFroude = ini.GetInt(L"Waves", L"BaseFroude", ship.BaseFroude);

        // Flag
        ship.bFlag = ini.GetBoolean(L"Flag", L"bFlag", ship.bFlag);
        ship.PosFlag = ini.GetVec3(L"Flag", L"PosFlag", ship.PosFlag);
        ship.DimXFlag = ini.GetFloat(L"Flag", L"DimXFlag", ship.DimXFlag);

        g_vShips.push_back(ship);
    }
}
void SetShip(int n)
{
    bool bTakeOldParameters = false;
    vec3 position = vec3(0.0f);
    float surgeVelocity = 0.0f;
    float yawVelocity = 0.0f;
    float yaw = 0.0f;
    float powerCurrentStep1 = 0.0f;
    float powerCurrentStep2 = 0.0f;
    float propRpm1 = 0.0f;
    float propRpm2 = 0.0f;
    float rudderCurrentStep = 0.0f;
    float rudderAngleDeg = 0.0f;

    if (g_vShips.size() == 1)
        n = 0;

    if (g_vShips.size() > 0 && n >= 0 && n < g_vShips.size())
    {
        if (g_Ship.get() != 0)
        {
            bTakeOldParameters = true;
            position = g_Ship->ship.Position;
            surgeVelocity = g_Ship->SurgeVelocity / g_Ship->ship.SpeedMaxKn;
            yawVelocity = g_Ship->YawVelocity;
            yaw = g_Ship->Yaw;
            powerCurrentStep1 = (float)g_Ship->PowerCurrentStep1 / (float)g_Ship->ship.PowerStepMax;
            powerCurrentStep2 = (float)g_Ship->PowerCurrentStep2 / (float)g_Ship->ship.PowerStepMax;
            propRpm1 = g_Ship->PropRpm1 / g_Ship->ship.PropRpmMax;
            propRpm2 = g_Ship->PropRpm2 / g_Ship->ship.PropRpmMax;
            rudderCurrentStep = (float)g_Ship->RudderCurrentStep / (float)g_Ship->ship.RudderStepMax;
            rudderAngleDeg = g_Ship->RudderAngleDeg / (float)g_Ship->ship.RudderStepMax;
        }
        
        g_Ship.reset();
        
        g_NoShip = n;
        g_Ship = make_unique<Ship>();
        g_Ship->SetOcean(g_Ocean.get());
        g_Ship->Init(g_vShips[g_NoShip], g_Camera);
        g_LowMass = g_vShips[g_NoShip].Mass_t / 2;
        g_HighMass = g_vShips[g_NoShip].Mass_t * 2;

        SetPosition();

        if (bTakeOldParameters)
        {
            g_Ship->ship.Position = position;
            g_Ship->SurgeVelocity = surgeVelocity * g_Ship->ship.SpeedMaxKn;
            g_Ship->YawVelocity = yawVelocity;
            g_Ship->Yaw = yaw;
            g_Ship->PowerCurrentStep1 = powerCurrentStep1 * g_Ship->ship.PowerStepMax;
            g_Ship->PowerCurrentStep2 = powerCurrentStep2 * g_Ship->ship.PowerStepMax;
            g_Ship->PropRpm1 = propRpm1 * g_Ship->ship.PropRpmMax;
            g_Ship->PropRpm2 = propRpm2 * g_Ship->ship.PropRpmMax;
            g_Ship->RudderCurrentStep = rudderCurrentStep * g_Ship->ship.RudderStepMax;
            g_Ship->RudderAngleDeg = rudderAngleDeg * g_Ship->ship.RudderStepMax;
        }

        g_Ship->HDGInstruction = fmod(450.0f - glm::degrees(g_Ship->Yaw), 360.0f);
        g_bReset = true;
    }
}
void SetMeteo(int n)
{
    switch (n)
    {
    case 1:
        g_Clouds->Coverage = 0.0f;
        g_Sky->FogDensity = 0.0f;
        g_Sky->MistDensity = 0.00015f;
        g_TWS_Kn = 5.0f;
        g_Wind = wind_from_speeddir(g_TWS_Deg, g_TWS_Kn);
        g_Ocean->GetWind(g_Wind);
        g_Ocean->InitFrequencies();
        g_Clouds->SetCloudSpeed(g_TWS_Kn);
        g_Ocean->iOceanColor = 8;
        g_Ocean->OceanColor = color_255_to_1(g_Ocean->vOceanColors[g_Ocean->iOceanColor]);
        break;
    
    case 2:
        g_Clouds->Coverage = 0.3f;
        g_Sky->FogDensity = 0.0f;
        g_Sky->MistDensity = 0.00015f;
        g_TWS_Kn = 15.0f;
        g_Wind = wind_from_speeddir(g_TWS_Deg, g_TWS_Kn);
        g_Ocean->GetWind(g_Wind);
        g_Ocean->InitFrequencies();
        g_Clouds->SetCloudSpeed(g_TWS_Kn);
        g_Ocean->iOceanColor = 6;
        g_Ocean->OceanColor = color_255_to_1(g_Ocean->vOceanColors[g_Ocean->iOceanColor]);
        break;
    
    case 3:
        g_Clouds->Coverage = 0.0f;
        g_Sky->FogDensity = 0.005f;
        g_Sky->MistDensity = 0.0f;
        g_TWS_Kn = 20.0f;
        g_Wind = wind_from_speeddir(g_TWS_Deg, g_TWS_Kn);
        g_Ocean->GetWind(g_Wind);
        g_Ocean->InitFrequencies();
        g_Clouds->SetCloudSpeed(g_TWS_Kn);
        g_Ocean->iOceanColor = 1;
        g_Ocean->OceanColor = color_255_to_1(g_Ocean->vOceanColors[g_Ocean->iOceanColor]);
        break;
    }
}
void LoadSounds()
{
    g_SoundMgr = SoundManager::getInstance();
    
    string s = "Resources/Sounds/Seagull_";
    for (int i = 0; i < 8; i++)
    {
        string name = s + to_string(i) + ".wav";
        g_SoundSeagull[i] = make_unique<Sound>(name.c_str());
        g_SoundSeagull[i]->setVolume(1.0f);
        g_SoundSeagull[i]->setPosition(vec3(0.0, 10.0f, 0.0f));
        g_SoundSeagull[i]->adjustDistances();
    }
    g_bSoundSeagull = false;

    g_SoundHorn = make_unique<Sound>("Resources/Sounds/Horn1.wav");
    g_SoundHorn->setVolume(1.0f);
    g_SoundHorn->setPosition(vec3(0.0, 0.0f, 0.0f));
    g_SoundHorn->adjustDistances();

    g_SoundRain = make_unique<Sound>("Resources/Sounds/Rain.wav");
    g_SoundRain->setVolume(1.0f);
    g_SoundRain->setLooping(true);
    g_SoundRain->setPosition(vec3(0.0, 0.0f, 0.0f));
    g_SoundRain->adjustDistances();
}
void UpdateFPS()
{
    g_FrameCount++;

    QueryPerformanceCounter(&g_CurrentTime);
    float deltaTime = (g_CurrentTime.QuadPart - g_LastTime.QuadPart) / (float)g_Frequency.QuadPart;

    if (deltaTime >= 1.0f)
    {
        g_Fps = 0.5 + (float)g_FrameCount / deltaTime;
        g_FrameCount = 0;
        g_LastTime = g_CurrentTime;
    }
}
void UpdateSounds()
{
    if (!g_SoundMgr->bSound)
        return;

    g_SoundMgr->setListenerPosition(g_Camera.GetPosition());
    g_SoundMgr->setListenerOrientation(g_Camera.GetAt(), g_Camera.GetUp());
    g_SoundHorn->setPosition(g_Ship->ship.Position);
    g_SoundRain->setPosition(g_Camera.GetPosition());

    if (g_bSoundSeagull)
    {
        std::random_device rd;
        std::mt19937 gen(rd());

        std::uniform_int_distribution<> distribPlay(0, 1000);
        std::uniform_int_distribution<> distribSound(0, 6);
        std::uniform_int_distribution<> distribPosX(-1000, 1000);
        std::uniform_int_distribution<> distribPosY(-1000, 1000);
        std::uniform_int_distribution<> distribPosZ(-1000, 1000);
        std::uniform_real_distribution<> distribVol(0.05f, 1.0f);
        if (distribPlay(gen) < 2)
        {
            int r = distribSound(gen);
            vec3 pos = g_Camera.GetPosition();
            pos.x += distribPosX(gen);
            pos.y += distribPosX(gen);
            pos.z += distribPosZ(gen);
            g_SoundSeagull[r]->setPosition(pos);
            g_SoundSeagull[r]->setVolume(distribVol(gen));
            g_SoundSeagull[r]->play();
        }
    }
}
bool CheckCrossingPort()
{
    float y_plane = 2.0f;   // Height of the port quay
    vec3 bbCorners[8];
    float t;
    vec3 point;
    vector<vec2> vCorners(4);

    sBBvec3 bb = g_Ship->GetBoundingBox();

    bbCorners[0] = g_Ship->TransformPosition(vec3(bb.min.x, bb.min.y, bb.min.z));
    bbCorners[2] = g_Ship->TransformPosition(vec3(bb.min.x, bb.max.y, bb.min.z));
    t = (y_plane - bbCorners[0].y) / (bbCorners[2].y - bbCorners[0].y);
    point = bbCorners[0] + t * (bbCorners[2] - bbCorners[0]);
    vCorners[0] = vec2(point.x, point.z);

    bbCorners[1] = g_Ship->TransformPosition(vec3(bb.max.x, bb.min.y, bb.min.z));
    bbCorners[3] = g_Ship->TransformPosition(vec3(bb.max.x, bb.max.y, bb.min.z));
    t = (y_plane - bbCorners[1].y) / (bbCorners[3].y - bbCorners[1].y);
    point = bbCorners[1] + t * (bbCorners[3] - bbCorners[1]);
    vCorners[1] = vec2(point.x, point.z);

    bbCorners[4] = g_Ship->TransformPosition(vec3(bb.min.x, bb.min.y, bb.max.z));
    bbCorners[6] = g_Ship->TransformPosition(vec3(bb.min.x, bb.max.y, bb.max.z));
    t = (y_plane - bbCorners[4].y) / (bbCorners[6].y - bbCorners[4].y);
    point = bbCorners[4] + t * (bbCorners[6] - bbCorners[4]);
    vCorners[2] = vec2(point.x, point.z);

    bbCorners[5] = g_Ship->TransformPosition(vec3(bb.max.x, bb.min.y, bb.max.z));
    bbCorners[7] = g_Ship->TransformPosition(vec3(bb.max.x, bb.max.y, bb.max.z));
    t = (y_plane - bbCorners[5].y) / (bbCorners[7].y - bbCorners[5].y);
    point = bbCorners[5] + t * (bbCorners[7] - bbCorners[5]);
    vCorners[3] = vec2(point.x, point.z);

    vector<sLine> bbEdges = {
        { vCorners[0], vCorners[1] },
        { vCorners[1], vCorners[2] },
        { vCorners[2], vCorners[3] },
        { vCorners[3], vCorners[0] }
    };

    for (const auto& line : g_vPortLines)
    {
        for (const auto& edge : bbEdges) 
        {
            vec3 intersectionPoint;
            if (IntersectionOfSegments(line.p1, line.p2, edge.p1, edge.p2))
            {
                g_CaptureName = L"C R A S H";
                return true; // cut detected
            }
        }
    }
    return false; // no cut detected on the 4 lateral sides
}
void SendNMEA(float time)
{
    if (g_Ship.get() == 0)
        return;

    static double lastTime = 0.0;
    static double lastSentAIVDM1 = 0.0;
    static double lastSentAIVDM5 = 0.0;
    static double initialDelayAIVDM5 = -1.0;

    // Initialiser le générateur aléatoire une seule fois
    static bool randInitialized = false;
    if (!randInitialized)
    {
        srand(static_cast<unsigned int>(std::time(0)));
        randInitialized = true;
    }

    // Déterminer le délai initial aléatoire pour la première émission AIVDM_5
    if (initialDelayAIVDM5 < 0.0)
        initialDelayAIVDM5 = (rand() % 60000) / 1000.0; // 0 à 60 secondes

    // Envoyer toutes les 50 ms les phrases RMC, VHW, VWR
    if (time - lastTime < 0.05)
        return;

    string sentence = g_Ship->NMEA_RMC();
    if (!sentence.empty())
        g_UdpSender->SendString(sentence);

    sentence = g_Ship->NMEA_VHW();
    if (!sentence.empty())
        g_UdpSender->SendString(sentence);

    sentence = g_Ship->NMEA_VWR();
    if (!sentence.empty())
        g_UdpSender->SendString(sentence);

    // Envoyer NMEA_AIVDM_1 toutes les 2 secondes
    if (time - lastSentAIVDM1 >= 2.0 || lastSentAIVDM1 == 0.0)
    {
        sentence = g_Traffics->NMEA_AIVDM_1();
        if (!sentence.empty())
            g_UdpSender->SendString(sentence);
        lastSentAIVDM1 = time;
    }

    // Envoyer NMEA_AIVDM_5 avec délai initial aléatoire, puis toutes les 60 s, et choisir un index aléatoire à chaque émission
    if ((lastSentAIVDM5 == 0.0 && time >= initialDelayAIVDM5) || (lastSentAIVDM5 > 0.0 && time - lastSentAIVDM5 >= 60.0))
    {
        int index = rand() % g_Traffics->vTraffics.size();
        sentence = g_Traffics->NMEA_AIVDM_5(index);
        if (!sentence.empty())
            g_UdpSender->SendString(sentence);
        lastSentAIVDM5 = time;
    }

    lastTime = time;
}
// Render 3D
void RenderAxis()
{
    if (g_Axe)
    {
        if (g_Axe->bVisible)
        {
            g_Axe->RenderDistorted(g_Camera, vec3(5.0f, 0.0f, 0.0f), vec3(5.0f, 0.025f, 0.025f), vec3(1.0f, 0.0f, 0.0f)); // Red
            g_Axe->RenderDistorted(g_Camera, vec3(0.0f, 5.0f, 0.0f), vec3(0.025f, 5.0f, 0.025f), vec3(0.0f, 1.0f, 0.0f)); // Green
            g_Axe->RenderDistorted(g_Camera, vec3(0.0f, 0.0f, 5.0f), vec3(0.025f, 0.025f, 5.0f), vec3(0.0f, 0.0f, 1.0f)); // Blue
        }
    }
}
void RenderBalls()
{
    if (!g_StaticBall->bVisible)
        return;
    
    float scale = 10.0f * g_Ship->ship.Length / 200.0f;

    // Limits of the turning circle
    vec3 red0 = vec3(50.0f, 0.0f, 0.0f);
    g_StaticBall->Render(g_Camera, red0, scale, vec3(1.0f, 0.4f, 0.5f));
    vec3 red1 = vec3(50.0f + 4.5f * g_Ship->ship.Length, 0.0f, 0.0f);
    g_StaticBall->Render(g_Camera, red1, scale, vec3(1.0f, 0.4f, 0.5f));
    vec3 red2 = vec3(50.0f + 4.5f * g_Ship->ship.Length, 0.0f, 5.0f * g_Ship->ship.Length);
    g_StaticBall->Render(g_Camera, red2, scale, vec3(1.0f, 0.4f, 0.5f));
    vec3 red3 = vec3(50.0f, 0.0f, 5.0f * g_Ship->ship.Length);
    g_StaticBall->Render(g_Camera, red3, scale, vec3(1.0f, 0.4f, 0.5f));

    // Optimum turning circle (3 L x 3 L)

    // 90° (3.5 L advance)
    vec3 green0 = vec3(50.0f + 3.5f * g_Ship->ship.Length, 0.0f, 2.5f * g_Ship->ship.Length);
    g_StaticBall->Render(g_Camera, green0, scale, vec3(0.4f, 1.0f, 0.5f));
    // 180° (4 L width)
    vec3 green1 = vec3(50.0f + 2.0f * g_Ship->ship.Length, 0.0f, 4.0f * g_Ship->ship.Length);
    g_StaticBall->Render(g_Camera, green1, scale, vec3(0.4f, 1.0f, 0.5f));
    // 270° (0.5 L bottom point)
    vec3 green2 = vec3(50.0f + 0.5f * g_Ship->ship.Length, 0.0f, 2.5f * g_Ship->ship.Length);
    g_StaticBall->Render(g_Camera, green2, scale, vec3(0.4f, 1.0f, 0.5f));
    // 360° (1 L width)
    vec3 green3 = vec3(50.0f + 2.0f * g_Ship->ship.Length, 0.0f, 1.0f * g_Ship->ship.Length);
    g_StaticBall->Render(g_Camera, green3, scale, vec3(0.4f, 1.0f, 0.5f));

    // Stopping ability
    vec3 red4 = vec3(50.0f + 15.0f * g_Ship->ship.Length, 0.0f, 0.0f);
    g_StaticBall->Render(g_Camera, red4, scale, vec3(1.0f, 0.4f, 0.5f));
}
void RenderCentralGridColored()
{
    if (!g_bGridVisible)
        return;

    int mesh_size = g_Ocean->PATCH_SIZE;
    vec3 translation = vec3(-g_Ocean->PATCH_SIZE / 2.0f, 0.0f, -g_Ocean->PATCH_SIZE / 2.0f);

    float cell = 10.0f;

    vec3 pos(-cell / 2.0f, 0.0f, -cell / 2.0f);
    pos += translation;
    vec3 color[3] = { {0.75f, 0.0f, 0.0f}, {0.0f, 0.75f, 0.0f}, {0.0f, 0.0f, 0.75f} };
    int c = 0;
    for (unsigned int z = 0; z < mesh_size; z += cell)
    {
        pos.x = translation.x - cell / 2.0f;
        pos.z += cell;
        for (unsigned int x = 0; x < mesh_size; x += cell)
        {
            pos.x += cell;
            g_Grid->Render(g_Camera, pos, 1.0f, color[c++]);
            if (c >= 3) c = 0;
        }
    }
}
void RenderGridsOfPatchs()
{
    if (!g_bGridVisible)
        return;

    float gridSize = g_Ocean->PATCH_SIZE;
    int numLayers = 10; // Number of grids on each side

    // Alternating colors: white, gray, white, gray, ...
    vec3 color[2] = { vec3(1.0f), vec3(0.7f, 0.7f, 0.7f) };

    // Origin of the center of the central grid
    vec3 center = vec3(0, 0, 0);

    for (int i = -numLayers; i <= numLayers; ++i)
    {
        for (int j = -numLayers; j <= numLayers; ++j)
        {
            vec3 pos = center + vec3(i * gridSize, 0, j * gridSize);

            // Choice of color according to distance from the central grid
            int distance = std::max(abs(i), abs(j));
            int colorIdx = distance % 2;
            vec3 currentColor = color[colorIdx];

            g_Grid->Render(g_Camera, pos, gridSize, currentColor);
        }
    }
}
void RenderOcean()
{
    if (!g_Ocean)
        return;

    if (!g_Ocean->bVisible)
        return;

    if (!g_bWireframe && g_bOceanWireframe) 
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    float waveLength = glm::two_pi<float>() * g_Ship->SOG * g_Ship->SOG / 9.81f;  // length between 2 crests
    float kelvinScale = 101.0f / waveLength;                // the texture is 1024 and there are 9 wavelengths in 910 pixels
    g_Ocean->Render(g_TimeSpeed * g_TimerShipMotion.getTime(), 
        g_Camera, 
        g_Ship->ship.Position, 
        g_Ship->Yaw, 
        g_Ship->bKelvinWakes && (g_Ship->SOG > 0.0f),
        g_Ship->LWL, 
        kelvinScale, 
        g_Ship->SOG, 
        g_Ship->ship.CenterFore, 
        g_Ship->ship.BaseFroude);

    if (!g_bWireframe && g_bOceanWireframe) 
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
void RenderShip()
{
    if (!g_Ship)
        return;

    if (!g_Ship->bVisible)
        return;

    if (g_Ship)
        g_Ship->Render(g_Camera, g_Sky.get());
}
void RenderTerrain()
{
    if (!g_bShowTerrain || g_vTerrains.size() == 0)
        return;

    // Terrain
    g_ShaderSun->use(); // Misc/sun.vert, Misc/sun.frag
    g_ShaderSun->setVec3("light.position", g_Sky->SunPosition);
    g_ShaderSun->setVec3("light.ambient", g_Sky->SunAmbient);
    g_ShaderSun->setVec3("light.diffuse", g_Sky->SunDiffuse);
    g_ShaderSun->setVec3("light.specular", g_Sky->SunSpecular);
    g_ShaderSun->setVec3("viewPos", g_Camera.GetPosition());
    g_ShaderSun->setFloat("exposure", g_Sky->Exposure);
    g_ShaderSun->setBool("bAbsorbance", g_Sky->bAbsorbance);
    g_ShaderSun->setVec3("absorbanceColor", g_Sky->AbsorbanceColor);
    g_ShaderSun->setFloat("absorbanceCoeff", g_Sky->AbsorbanceCoeff);
    g_ShaderSun->setFloat("specularIntensity", 0.0f);

    g_ShaderSun->setMat4("view", g_Camera.GetView());
    g_ShaderSun->setMat4("projection", g_Camera.GetProjection());

    for (size_t t = 0; t < g_vTerrains.size(); t++)
    {
        mat4 model = glm::translate(mat4(1.0f), g_vTerrains[t].pos);
        g_ShaderSun->setMat4("model", model);
        g_vTerrains[t].model->Render(*g_ShaderSun);
    }

    if (g_vTerrains.size() > g_idxHouat)
    {
        mat4 model = glm::translate(mat4(1.0f), g_vTerrains[g_idxHouat].pos);    // The 2 objects are linked to Houat island
        g_ShaderSun->setMat4("model", model);
        if (g_Port) g_Port->Render(*g_ShaderSun);
    }
}
void RenderTraffic()
{
    if (g_Traffics.get())
        g_Traffics->Render(g_Camera, g_Sky.get());
}
void RenderMarks()
{
    if (g_Markup.get())
        g_Markup->Render(g_Camera, g_Ocean.get(), g_Sky.get());
}
void RenderArrowWind()
{
    if (!g_ArrowWind)
        return;
    
    if (!g_ArrowWind->bVisible)
        return;

    vec3 position = vec3(g_Ship->ship.Position.x, 1.0f, g_Ship->ship.Position.z) + 2.0f * g_Ship->GetLength() * glm::normalize(vec3(g_Wind.x, 0.0f, g_Wind.y));
    float dir = g_TWS_Deg + 180.0f;
    dir = fmod(450.0f - dir, 360.0f);
    dir = glm::radians(dir);

    mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, vec3(1.0f));
    model = glm::rotate(model, dir, vec3(0, 1, 0));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Light from camera
    g_ShaderCamera->use();
    g_ShaderCamera->setVec3("light.position", g_Camera.GetPosition() - position);
    g_ShaderCamera->setVec3("light.diffuse", vec3(1.0f));
    g_ShaderCamera->setMat4("model", model);
    g_ShaderCamera->setMat4("view", g_Camera.GetView());
    g_ShaderCamera->setMat4("projection", g_Camera.GetProjection());
    g_ArrowWind->Render(*g_ShaderCamera);
}
void RenderRainVolumeFullScreen()
{
    g_ShaderRainVolume->use();
    g_ShaderRainVolume->setMat4("matViewProj", g_Camera.GetViewProjection());
    g_ShaderRainVolume->setMat4("matInvViewProj", glm::inverse(g_Camera.GetViewProjection()));

    // ✅ CORRIGÉ : FBO TEMPORAIRE (océan SEUL)
    g_ShaderRainVolume->setSampler2D("sceneColor", TexOceanTempColor, 0);
    g_ShaderRainVolume->setSampler2D("sceneDepth", TexOceanTempDepth, 1);

    // Paramètres pluie INCHANGÉS
    g_ShaderRainVolume->setFloat("uTime", g_TimerShipMotion.getTime());
    g_ShaderRainVolume->setVec3("cameraPos", g_Camera.GetPosition());
    vec3 wind3D(g_Wind.x, 0.0f, g_Wind.y);
    g_ShaderRainVolume->setVec3("windDir", wind3D);
    g_ShaderRainVolume->setVec2("screenSize", vec2(g_WindowW, g_WindowH));
    g_ShaderRainVolume->setFloat("rainDensity", 1.0f);
    g_ShaderRainVolume->setFloat("rainSpeed", 1.0f);

    // RENDRE dans msFBO_SCENE (accumulation)
    g_ScreenQuadRainVolume->Render();
}
void RenderRain(bool bInside)
{
    // EXTÉRIEUR : Pluie partout (pas de stencil)
    if (!bInside)
    {
        RenderRainVolumeFullScreen();
        return;
    }

    // INTÉRIEUR : Pluie SEULEMENT par vitres (STENCIL)
    glEnable(GL_STENCIL_TEST);

    // Pass 1 : MARQUER vitres (stencil=1)
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilMask(0xFF);
    glDepthMask(GL_FALSE);  // Vitres transparentes

    g_Ship->RenderStencilForWindows(g_Camera);

    glDepthMask(GL_TRUE);

    // Pass 2 : PLUIE UNIQUEMENT où stencil=1
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0x00);
    glDepthFunc(GL_ALWAYS);  // Overlay pluie

    RenderRainVolumeFullScreen();

    // RESET STENCIL
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glDepthFunc(GL_LESS);
    glDisable(GL_STENCIL_TEST);
}

// Interface 2D
void RenderImGui()
{
    // Start of ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    float gray = 56.0f / 255.0f;

    static string resultAnalyse;
    static vector<vector<sResultData>> vResults;

    // Parameters
    if (g_bShowSceneWindow)
    {
        // Fixed position at the bottom right of the window
        ImVec2 window_size(300, 930);   // W, H
        ImVec2 window_pos(5, 5);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);

        // Change the background color
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(gray, gray, gray, 0.75f));

        if (ImGui::Begin("Scene [F2]", &g_bShowSceneWindow))
        {
            if (ImGui::Checkbox("VSYNC", &g_bVsync))
                SetVsync((int)g_bVsync);
            ImGui::SameLine();
            ImGui::Text("-> FPS = %d", g_Fps);
            ImGui::SameLine();
            double elapsed = ImGui::GetTimeGMT();  // seconds elapsed since the start of the program or ImGui
            int hours = static_cast<int>(elapsed) / 3600;
            int minutes = (static_cast<int>(elapsed) % 3600) / 60;
            int seconds = static_cast<int>(elapsed) % 60;
            char buffer[12];
            snprintf(buffer, sizeof(buffer), "  %02d:%02d:%02d", hours, minutes, seconds);            
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 128, 255));
            ImGui::SetWindowFontScale(1.5f);      
            ImGui::Text("%s", buffer);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor();
            // ------------
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            // Camera type
            switch (g_Camera.GetMode())
            {
            case eCameraMode::ORBITAL:  ImGui::Text("[ ORBITAL ]"); break;
            case eCameraMode::BRIDGE:   ImGui::Text("[ BRIDGE ]"); break;
            case eCameraMode::FPS:      ImGui::Text("[ FPS ]");     break;
            }
            ImGui::SameLine();
            // Interpolation function
            switch (g_Camera.GetInterpolation())
            {
            case eInterpolation::Linear:    ImGui::Text("( Linear )");      break;
            case eInterpolation::SmoothStep:ImGui::Text("( SmoothStep )");  break;
            case eInterpolation::EaseIn:    ImGui::Text("( EaseIn )");      break;
            case eInterpolation::EaseOut:   ImGui::Text("( EaseOut )");     break;
            case eInterpolation::EaseInOut: ImGui::Text("( EaseInOut )");   break;
            }
            ImGui::PopStyleColor(1);                                                            
            // ------------
            ImGui::Checkbox("Night vision", &g_bNightVision);
            ImGui::SameLine();
            ImGui::Checkbox("Low vision", &g_bLowIntensity);
            ImGui::SameLine();
            ImGui::Checkbox("Chrono", &g_bChrono);
            // ------------
            if (ImGui::Button(" PAUSE "))
            {
                g_bPause = !g_bPause;
                if (g_bPause)
                    g_TimerShipMotion.stop();
                else
                    g_TimerShipMotion.start();
            }
            ImGui::SameLine();
            if (ImGui::Button(" FULLSCREEN "))
                SwitchToFullScreen();

            /////////////////////////////////
            if (ImGui::CollapsingHeader("SCENE", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (g_Ocean) ImGui::Checkbox("Ocean", &g_Ocean->bVisible);
                ImGui::SameLine();
                if (g_Clouds) ImGui::Checkbox("Sky", &g_bSkyVisible);
                ImGui::SameLine();
                if (ImGui::Checkbox("Ship", &g_Ship->bVisible))
                {
                    static float prevYaw = g_Ship->Yaw;
                    static vec3 prevPos = g_Ship->ship.Position;
                    g_bShipWake = g_Ship->bVisible;
                    g_bShipShadow = g_Ship->bVisible;
                    if (!g_Ship->bVisible)
                    {
                        prevYaw = g_Ship->Yaw;
                        prevPos = g_Ship->ship.Position;
                    }
                    else
                    {
                        g_Ship->Yaw = prevYaw;
                        g_Ship->ship.Position = prevPos;
                        g_Ship->ResetVelocities();
                        g_bReset = true;
                    }
                }
                ImGui::SameLine();
                ImGui::Checkbox("Sound##1", &g_SoundMgr->bSound);
                // ------------
                if (g_vTerrains.size()) ImGui::Checkbox("Terrain", &g_bShowTerrain);
                ImGui::SameLine();
                ImGui::Checkbox("Markup", &g_Markup->bVisible);
                ImGui::SameLine();
                ImGui::Checkbox("Grid", &g_bGridVisible);
                if (g_Axe) {
                    ImGui::SameLine();
                    ImGui::Checkbox("Axis", &g_Axe->bVisible);
                }
                // ------------
                if (g_StaticBall)
                    ImGui::Checkbox("Manoeuvrability", &g_StaticBall->bVisible);
                // ------------
                ImGui::Checkbox("Wind Arrow", &g_ArrowWind->bVisible);
                ImGui::SameLine();
                ImGui::Checkbox("Seagull", &g_bSoundSeagull);
                ImGui::SameLine();
                ImGui::Checkbox("Traffic", &g_Traffics->bShowRoute);
                // ------------
                ImGui::Checkbox("Wireframe##1", &g_bWireframe);
                ImGui::SameLine();
                static bool isPhysics = false;
                if (ImGui::Button(isPhysics ? "FULL###ModeButton" : "PHYSICS###ModeButton"))
                {
                    isPhysics = !isPhysics;
                    if (isPhysics)
                    {
                        g_Ship->bMotion = true;
                        g_Ship->Rendering = eRendering::TRIANGLES;
                        g_Ship->bWireframe = true;
                        g_Ship->bOutline = true;
                        g_Ship->bSmoke = false;
                        g_Ship->bRadar = false;
                        g_Ship->bForces = true;
                        g_Ocean->bVisible = false;
                        g_bGridVisible = true;
                    }
                    else
                    {
                        g_Ship->bMotion = true;
                        g_Ship->Rendering = eRendering::SUN;
                        g_Ship->bWireframe = false;
                        g_Ship->bOutline = false;
                        g_Ship->bSmoke = true;
                        g_Ship->bRadar = true;
                        g_Ship->bForces = false;
                        g_Ocean->bVisible = true;
                        g_bGridVisible = false;
                    }
                }
            }

            /////////////////////////////////
            if (ImGui::CollapsingHeader("WIND"))
            {
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));         // Red
                ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));   // Dark red when active
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));            // Dark background
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));     // Lighter background on hover
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));      // Even brighter background when active
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));               // White text
                if (ImGui::SliderFloat("Direction", &g_TWS_Deg, 0.0f, 360.0f, "%.f°"))
                {
                    g_Wind = wind_from_speeddir(g_TWS_Deg, g_TWS_Kn);
                    g_Ocean->GetWind(g_Wind);
                    g_Ocean->InitFrequencies();
                }
                if (ImGui::SliderFloat("Speed##1", &g_TWS_Kn, 1.0f, 30.0f, "%0.0f kn"))
                {
                    g_Wind = wind_from_speeddir(g_TWS_Deg, g_TWS_Kn);
                    g_Ocean->GetWind(g_Wind);
                    g_Ocean->InitFrequencies();
                    g_Clouds->SetCloudSpeed(g_TWS_Kn);
                }
                ImGui::PopStyleColor(6);                                                            // For the 6 PushStyleColor above
            }

            /////////////////////////////////
            if (ImGui::CollapsingHeader("OCEAN"))
            {
                if (g_Ocean)
                {
                    ImGui::SliderFloat("Lambda", &g_Ocean->Lambda, 0.0f, -2.0f, "%0.2f");
                    if (ImGui::SliderFloat("Foam", &g_Ocean->PersistenceSec, 0.0f, 5.0f, "%0.1f s")) g_Ocean->EvaluatePersistence(g_Ocean->PersistenceSec);
                    ImGui::SliderFloat("Transparency", &g_Ocean->Transparency, 0.0f, 1.0f, "%0.2f");
                    // Colors
                    if (ImGui::SliderInt("# Color", &g_Ocean->iOceanColor, 0, g_Ocean->vOceanColors.size() - 1)) 
                        g_Ocean->OceanColor = color_255_to_1(g_Ocean->vOceanColors[g_Ocean->iOceanColor]);
                    ImGui::ColorEdit3("Ocean", (float*)&g_Ocean->OceanColor[0], 0);
                    // Heights
                    static float waves1_3 = 0.0f;
                    static float waveMax = 0.0f;
                    static float average_period = 0.0f;
                    static int nWaves = 0;
                    g_Ocean->GetWaveByWaveAnalysis(waves1_3, waveMax, nWaves, average_period);
                    ImGui::Text("Height 1/3  : %.1f m (%.1f s) (%d waves)", waves1_3, average_period, nWaves);

                    double fetch = 50000.0; // 50 km en mètres
                    auto waveParams = JONSWAPModel::GetWaveParameters(knot_to_ms(g_TWS_Kn), fetch);
                    ImGui::Text("JONSWAP 1/3 : %.1f m (%.1f s)", waveParams.significantWaveHeight, waveParams.peakPeriod);
                    ImGui::Checkbox("Analyse spectrale", &g_bShowOceanAnalysisWindow);
                    static double lastUpdateTime = 0.0;
                    if (g_bShowOceanAnalysisWindow)
                    {
                        double currentTime = glfwGetTime(); // temps actuel en secondes depuis l'initialisation GLFW
                        if (currentTime - lastUpdateTime >= 1.0)
                        {
                            lastUpdateTime = currentTime;
                            vResults.clear();
                            vResults.push_back(g_Ocean->SpectralAnalysis());
                            vResults.push_back(g_Ocean->DirectionalAnalysis());
                        }
                    }
                    else
                        lastUpdateTime = 0.0;
                    ImGui::SameLine();
                    ImGui::Checkbox("Ocean cut", &g_bShowOceanCut);
                    // ------------
                    ImGui::Checkbox("Wireframe", &g_bOceanWireframe);
                    ImGui::SameLine();
                    ImGui::Checkbox("Patches", &g_Ocean->bShowPatch);
                }
            }
            
            /////////////////////////////////
            if (ImGui::CollapsingHeader("SUN"))
            {
                ImGui::ColorEdit3("Ambient", (float*)&g_Sky->SunAmbient[0], 0);
                ImGui::ColorEdit3("Diffuse", (float*)&g_Sky->SunDiffuse[0], 0);
                if (g_Sky) ImGui::SliderFloat("Exposure", &g_Sky->Exposure, 0.0f, 3.0f, "%0.2f");

                ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));         // Red
                ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));   // Dark red when active
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));            // Dark background
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));     // Lighter background on hover
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));      // Even brighter background when active
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));               // White text

                static sHM hm = g_Sky->GetNow();
                sHM storeHM = hm;
                float hour = hm.hour + hm.minute / 60.0f;
                if (ImGui::SliderFloat("Time", &hour, 0.0f, 24.0f, "", ImGuiSliderFlags_None))
                {
                    hm.hour = int(hour);
                    hm.minute = fract(hour) * 60.0f + 0.5f;
                    if (hm.hour != storeHM.hour || hm.minute != storeHM.minute)
                        g_Sky->SetTime(hm.hour, hm.minute);
                }
                char buf[16];
                snprintf(buf, 16, "%02d:%02d", hm.hour, hm.minute);
                ImGui::SameLine();
                ImGui::Text("%s", buf);

                ImGui::PopStyleColor(6);                                                        // For the 6 PushStyleColor above

                if (ImGui::Button(" NOW "))
                {
                    g_Sky->SetNow();
                    sHM hm1 = g_Sky->GetNow();
                    hour = hm1.hour + hm1.minute / 60.0f;
                }
            }
            /////////////////////////////////
            if (ImGui::CollapsingHeader("ATMOSPHERE"))
            {
                ImGui::Separator();

                if (ImGui::SliderFloat("Mist", &g_Sky->MistDensity, 0.0f, 0.001f, "%.5f"))
                {
                    if (g_Sky->MistDensity > 0.0f)
                        g_Sky->FogDensity = 0.0f;   // Disables Fog if Mist > 0
                }
                if (ImGui::SliderFloat("Fog", &g_Sky->FogDensity, 0.0f, 0.01f, "%.5f"))
                {
                    if (g_Sky->FogDensity > 0.0f)
                        g_Sky->MistDensity = 0.0f;  // Disables Mist if Fog > 0
                }
            }

            /////////////////////////////////
            if (ImGui::CollapsingHeader("CLOUDS"))
            {
                ImGui::SliderFloat("Coverage", &g_Clouds->Coverage, 0.0f, 1.0f, "%.2f");
                ImGui::SliderFloat("Speed##3", &g_Clouds->CloudSpeed, 0.0f, 2000.0f, "%.0f");
                ImGui::SliderFloat("Crispiness", &g_Clouds->Crispiness, 0.0f, 120.0f, "%.0f");
                ImGui::SliderFloat("Curliness", &g_Clouds->Curliness, 0.0f, 3.0f, "%.2f");
                ImGui::SliderFloat("Illumination", &g_Clouds->Illumination, 1.0f, 10.0f, "%.1f");
                ImGui::SliderFloat("Absorption", &g_Clouds->Absorption, 0.0f, 1.5f, "%.2f");
                ImGui::SliderFloat("Top", &g_Clouds->SphereOuterRadius, 1000.0f, 40000.0f, "%.0f");
                ImGui::SliderFloat("Bottom", &g_Clouds->SphereInnerRadius, 1000.0f, 15000.0f, "%.0f");
                if (ImGui::SliderFloat("Frequency", &g_Clouds->PerlinFrequency, 0.0f, 4.0f, "%.2f"))
                    g_Clouds->GenerateWeatherMap();
                ImGui::ColorEdit3("Cloud top", (float*)&g_Clouds->CloudColorTop[0], 0);
                ImGui::ColorEdit3("Cloud bottom", (float*)&g_Clouds->CloudColorBottom[0], 0);
                ImGui::Checkbox("Godrays", &g_Clouds->bEnableGodRays);
            }
           
            /////////////////////////////////
            if (ImGui::CollapsingHeader("RAIN"))
            {
                if (ImGui::Checkbox("Droplets", &g_Sky->bRainDropsTrails))
                    if (g_Sky->bRainDropsTrails)
                        g_Sky->bRainBlurDrips= false;
                ImGui::SameLine();
                if (ImGui::Checkbox("Blur", &g_Sky->bRainBlurDrips))
                    if (g_Sky->bRainBlurDrips)
                        g_Sky->bRainDropsTrails = false;
                g_Sky->bRain = g_Sky->bRainDropsTrails || g_Sky->bRainBlurDrips;
                if (g_Sky->bRain)
                {
                    if (g_SoundMgr->bSound && g_Ship->bSound && !g_bSoundRain)
                    {
                        g_SoundRain->play();
                        g_bSoundRain = true;
                    }
                }
                else
                {
                    g_SoundRain->stop();
                    g_bSoundRain = false;
                }
            }
        }
        ImGui::End();

        // Restoration of the previous style
        ImGui::PopStyleColor();
    }

    if (g_bShowShipWindow && g_Ship)
    {
        ImGui::SetNextWindowSize(ImVec2(350, 200), ImGuiCond_FirstUseEver);
        ImVec2 window_size(280, 600);   // W, H
        ImVec2 window_pos(g_WindowW - window_size.x - 5, 5);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);
        float gray = 56.0f / 255.0f;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(gray, gray, gray, 0.75f));
        if (ImGui::Begin("Ship [F3]", &g_bShowShipWindow))
        {
            string temp;
            if (g_vShips.size() && g_NoShip >= 0 && g_NoShip < g_vShips.size())
                temp = g_vShips[g_NoShip].ShortName;
            const char* combo_preview_value = temp.c_str();
            if (ImGui::BeginCombo(" ", combo_preview_value, 0))
            {
                for (int n = 0; n < g_vShips.size(); n++)
                {
                    const bool is_selected = (g_NoShip == n);
                    if (ImGui::Selectable(g_vShips[n].ShortName.c_str(), is_selected))
                        SetShip(n);

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            // ------------
            ImGui::Checkbox("Model", &g_Ship->bModel);
            ImGui::SameLine();
            ImGui::Checkbox("Sound##2", &g_Ship->bSound);
            ImGui::SameLine();
            ImGui::Checkbox("Lights", &g_Ship->bLights);
            // ------------
            ImGui::Checkbox("Spray", &g_Ship->bSpray);
            ImGui::SameLine();
            ImGui::Checkbox("Wakes", &g_Ship->bKelvinWakes);
            ImGui::SameLine();
            ImGui::Checkbox("Wake", &g_bShipWake);
            // ------------
            ImGui::Checkbox("Smoke", &g_Ship->bSmoke);
            ImGui::SameLine();
            ImGui::Checkbox("Shadow", &g_bShipShadow);
            ImGui::SameLine();
            ImGui::Checkbox("Flag", &g_Ship->bFlag);
            // ------------
            int choix = (int)g_Ship->Rendering;
            int store = choix;
            ImGui::RadioButton("Triangles", &choix, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Basic", &choix, 1);
            ImGui::SameLine();
            ImGui::RadioButton("Sun", &choix, 2);
            if (store != choix)
            {
                g_Ship->Rendering = (eRendering)choix;
                if (choix == 0)
                    g_Ship->bOutline = true;
                if (choix == 2)
                    g_Ship->bOutline = false;
            }
            // ------------
            ImGui::Checkbox("Wireframe##2", &g_Ship->bWireframe);
            ImGui::SameLine();
            ImGui::Checkbox("Outline", &g_Ship->bOutline);
            // ------------
            ImGui::Checkbox("Axis", &g_Ship->bAxis);
            ImGui::SameLine();
            ImGui::Checkbox("Forces", &g_Ship->bForces);
            ImGui::SameLine();
            if (ImGui::Checkbox("Pressure", &g_Ship->bPressure))
            {
                if (g_Ship->bPressure)
                {
                    g_Ship->Rendering = eRendering::TRIANGLES;
                    g_Ship->bOutline = true;
                    g_Ship->bSmoke = false;
                    g_Ship->bLights = false;
                    g_Traffics->bLights = g_Ship->bLights;
                }
            }
            // ------------
            ImGui::Checkbox("Wake D", &g_Ship->bWakeVao);
            ImGui::SameLine();
            ImGui::Checkbox("Contours D", &g_Ship->bContour);
            ImGui::SameLine();
            ImGui::Checkbox("BBox", &g_Ship->BBoxShape->bVisible);

            /////////////////////////////////
            ImGui::SeparatorText("AUTOPILOT");
            if (ImGui::Button("SETTINGS"))
                g_bShowAutopilotWindow = !g_bShowAutopilotWindow;

            /////////////////////////////////
            ImGui::SeparatorText("PHYSICS");
            
            const char* preview_name = g_vPositions[g_NoPosition].name.c_str();
            bool changed = false;
            if (ImGui::BeginCombo("##combo", preview_name)) 
            {
                for (int n = 0; n < (int)g_vPositions.size(); n++) 
                {
                    const bool is_selected = (g_NoPosition == n);
                    // Affiche texte avec le nom + la position
                    std::string item_text = g_vPositions[n].name;
                    if (ImGui::Selectable(item_text.c_str(), is_selected))
                    {
                        g_NoPosition = n;
                        changed = true;
                    }

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            if (changed)
                SetPosition();

            if (ImGui::Checkbox("Motion", &g_Ship->bMotion))
                g_Ship->ResetVelocities();

            //ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));         // Red
            //ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));   // Dark red when active
            //ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));            // Dark background
            //ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));     // Lighter background on hover
            //ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));      // Even brighter background when active
            //ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));               // White text
            //float rot = glm::degrees(g_Ship->Yaw);
            //if (ImGui::SliderFloat("Rotation", &rot, -180.0f, 180.0f, "%.f°"))
            //{
            //    g_Ship->Yaw = glm::radians(rot);
            //    g_Ship->ResetVelocities();
            //}
            //ImGui::PopStyleColor(6);                                                            // For the 6 PushStyleColor above
            if (ImGui::Button("S"))
                SetHeading(180.0f);
            ImGui::SameLine();
            if (ImGui::Button("SW"))
                SetHeading(225.0f);
            ImGui::SameLine();
            if (ImGui::Button("W"))
                SetHeading(270.0f);
            ImGui::SameLine();
            if (ImGui::Button("NW"))
                SetHeading(315.0f);
            ImGui::SameLine();
            if (ImGui::Button("N"))
                SetHeading(0.0f);
            ImGui::SameLine();
            if (ImGui::Button("NE"))
                SetHeading(45.0f);
            ImGui::SameLine();
            if (ImGui::Button("E"))
                SetHeading(90.0f);
            ImGui::SameLine();
            if (ImGui::Button("SE"))
                SetHeading(135.0f);
     
            ImGui::SliderFloat3("Gravity", (float*)&g_Ship->ship.PosGravity[0], -10.0f, 5.0f, "%.1f");
            int massT = g_Ship->ship.Mass_t;
            if (ImGui::SliderInt("Mass", &massT, g_LowMass, g_HighMass, "%d t"))
            {
                g_Ship->ship.Mass_t = massT;
                g_Ship->SetMass();
            }

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.0f, 1.0f));
            ImGui::Text(g_Ship->InfoHull.c_str());
            char txt[50];
            sprintf(txt, "Ocean Search Complexity : %d", g_Ship->WaterSearch);
            ImGui::Text(txt);
            ImGui::PopStyleColor();
          
            ImGui::SeparatorText("MODEL");
          
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
            ImGui::Text(g_Ship->InfoFull.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::End();
        ImGui::PopStyleColor();
    }

    if (g_bShowOceanAnalysisWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(350, 250), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Spectrum Analysis", &g_bShowOceanAnalysisWindow))
        {
            ImGui::BeginTable("Results", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
            // Table Headers
            ImGui::TableSetupColumn("VARIABLE", ImGuiTableColumnFlags_WidthStretch, 0.7f);
            ImGui::TableSetupColumn("VALUE", ImGuiTableColumnFlags_WidthStretch, 0.3f);
            ImGui::TableSetupColumn("UNIT", ImGuiTableColumnFlags_WidthStretch, 0.4f);
            ImGui::TableHeadersRow();

            // Table data
            for (const auto& results : vResults)
            {
                for (const auto& result : results)
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", result.variable.c_str());

                    ImGui::TableSetColumnIndex(1);
                    string format = "%." + to_string(result.decimal) + "f";
                    ImGui::Text(format.c_str(), result.value);

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%s", result.unit.c_str());
                }
            }
            ImGui::EndTable();
        }

        ImGui::End();
    }
    
    if (g_bShowShipForcesWindows)
    {
        ImGui::SetNextWindowSize(ImVec2(350, 250), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Ship Forces", &g_bShowShipForcesWindows))
        {
            ImGui::BeginTable("Forces", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
            // Table Headers
            ImGui::TableSetupColumn("VARIABLE", ImGuiTableColumnFlags_WidthStretch, 0.7f);
            ImGui::TableSetupColumn("VALUE", ImGuiTableColumnFlags_WidthStretch, 0.3f);
            ImGui::TableSetupColumn("UNIT", ImGuiTableColumnFlags_WidthStretch, 0.4f);
            ImGui::TableHeadersRow();

            // Table data
            for (const auto& result : g_Ship->vForcesData)
            {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", result.variable.c_str());

                ImGui::TableSetColumnIndex(1);
                string format = "%." + to_string(result.decimal) + "f";
                if (abs(result.value) > 10000.0)
                {
                    format += " k";
                    ImGui::Text(format.c_str(), result.value / 1000.0);
                }
                else
                    ImGui::Text(format.c_str(), result.value);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", result.unit.c_str());
            }
            ImGui::EndTable();
        }

        ImGui::End();
    }

    if (g_bShowAutopilotWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(350, 300), ImGuiCond_FirstUseEver);
        ImVec2 window_pos((g_WindowW - 350) / 2, g_WindowH - 300);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(gray, gray, gray, 0.75f));
        if (ImGui::Begin("Autopilot [F5]", &g_bShowAutopilotWindow))
        {
            ImGui::SliderFloat("P", &g_Ship->ship.BaseP, 0.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("I", &g_Ship->ship.BaseI, 0.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("D", &g_Ship->ship.BaseD, 0.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("MaxIntegral", &g_Ship->ship.MaxIntegral, 0.0f, 20.0f, "%.1f");
        }
        ImGui::End();
        ImGui::PopStyleColor();
    }
}
void RenderStatusbar()
{
    if (!g_bShowStatusBar)
        return;

    char text[256];
    vec3 position = g_Camera.GetPosition();
    float distCameraShip = glm::length(g_Ship->ship.Position - position);
    vec2 lonlat = opengl_to_lonlat(position.x, position.z);
    sprintf_s(text, "CAMERA x = %.2f y = %.2f z = %.2f Heading = %03d° Roll = %d° Pitch = %d       Distance to ship = %d m      Lon = %.6f Lat =  %.6f", 
        position.x, position.y, position.z, 
        (int)g_Camera.GetNorthAngleDEG(), (int)g_Camera.GetAttitudeDEG(), (int)g_Camera.GetRollDEG(), 
        int(distCameraShip), lonlat.x, lonlat.y);

    // Rectangle
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, 0, g_WindowH - 20, g_WindowW, 20);
    nvgClosePath(g_Nvg);
    nvgFillColor(g_Nvg, nvgRGBA(255, 255, 255, 64));
    nvgFill(g_Nvg);

    // Text
    nvgFontSize(g_Nvg, 12.0f);
    nvgFontFace(g_Nvg, "arial");
    nvgTextAlign(g_Nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));

    nvgText(g_Nvg, 5, g_WindowH - 9, text, NULL);
}
void RenderOceanCut(float x, float y, float w, float h, int xN)
{
    if (!g_bShowOceanCut)
        return;

    // Get the cut
    vector<vec2> cut = g_Ocean->GetCut(xN);
    if (cut.empty())
        return;

    for (size_t i = 0; i < cut.size(); i++)
        cut[i].x += i;

    // Upper left corner of the centered rectangle
    float rectX = x - w * 0.5f;
    float rectY = y - h * 0.5f;

    // Min and max limits of the points to normalize in the rectangle
    auto [minY_it, maxY_it] = std::minmax_element(cut.begin(), cut.end(), [](const vec2& a, const vec2& b) { return a.y < b.y; });
    float minY = minY_it->y;
    float maxY = maxY_it->y;

    float rangeX = cut.back().x - cut.front().x;
    float rangeY = (maxY - minY) * 2.0;
    if (rangeY < 1e-6f) rangeY = 1.0f;    // Avoid division by zero

    // Draw the background + outline rectangle
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, rectX, rectY, w, h);
    nvgFillColor(g_Nvg, nvgRGBA(30, 144, 255, 64));  // transparent light blue background
    nvgFill(g_Nvg);
    nvgStrokeColor(g_Nvg, nvgRGBA(30, 144, 255, 200)); // transparent blue outline
    nvgStrokeWidth(g_Nvg, 2.0f);
    nvgStroke(g_Nvg);

    // Draw the curve of the cut in the rectangle
    nvgBeginPath(g_Nvg);

    for (size_t i = 0; i < cut.size(); i++)
    {
        // x normalized increasing from left to right
        float px = rectX + ((cut[i].x) / rangeX) * w;

        // y normalized from bottom to top (screen originally at the top)
        float py = rectY + h * 0.5 - ((cut[i].y - minY) / rangeY) * h;

        if (i == 0)
            nvgMoveTo(g_Nvg, px, py);
        else
            nvgLineTo(g_Nvg, px, py);
    }

    nvgStrokeColor(g_Nvg, nvgRGBA(255, 69, 0, 255)); // dark orange color
    nvgStrokeWidth(g_Nvg, 2.0f);
    nvgStroke(g_Nvg);
}
void RenderTitle(float x, float y, float size, int align)
{
    nvgBeginPath(g_Nvg);
    nvgFillColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgFontSize(g_Nvg, size);
    nvgFontFace(g_Nvg, "caveat");
    nvgTextAlign(g_Nvg, align);

    float bounds[4];
    const char* text = "SimShip";
    nvgTextBounds(g_Nvg, x, y, text, NULL, bounds); //float textWidth = bounds[2] - bounds[0];//float textHeight = bounds[3] - bounds[1];

    nvgText(g_Nvg, x, y, text, nullptr);
    nvgStroke(g_Nvg);
}
void RenderInfo()
{
    if (g_CaptureName.length() == 0)
        return;
    static double textStartTime = 0.0;
    static bool textVisible = false;
    if (!textVisible) {
        textStartTime = glfwGetTime();
        textVisible = true;
    }
    double now = glfwGetTime();
    if (textVisible && (now - textStartTime) < 1.0)
    {
        string s(g_CaptureName.begin(), g_CaptureName.end());
        const char* text = s.c_str();
        float x = g_WindowW / 2.0f;
        float y = g_WindowH / 2.0f;
        float bounds[4];

        // Measures the bounds of the text
        nvgFontSize(g_Nvg, 36);
        nvgFontFace(g_Nvg, "arial");
        nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgTextBounds(g_Nvg, x, y, text, nullptr, bounds);

        // Calculate the dimensions of the rectangle
        float pad = 24.0f;
        float rx = bounds[0] - pad;
        float ry = bounds[1] - pad;
        float rw = (bounds[2] - bounds[0]) + 2 * pad;
        float rh = (bounds[3] - bounds[1]) + 2 * pad;

        // Draw the charcoal background under the text
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, rx, ry, rw, rh);
        nvgFillColor(g_Nvg, nvgRGBA(38, 38, 38, 200));
        nvgFill(g_Nvg);

        // Draw the text
        nvgBeginPath(g_Nvg);
        nvgFillColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
        nvgText(g_Nvg, x, y, text, nullptr);

        nvgStroke(g_Nvg);
    }
    else if (textVisible && (now - textStartTime) >= 1.0)
    {
        textVisible = false;
        g_CaptureName.clear();
    }
}
void RenderTextures()
{
    // Textures of displacement & gradients
    if (g_bTextureDisplay)
    {
        int side = g_Ocean->FFT_SIZE + 5;
        g_QuadTexture->Render(g_Ocean->GetDisplacementID(), 5 + 0 * side, g_WindowH - side, g_Ocean->FFT_SIZE, g_Ocean->FFT_SIZE, QuadTexture::RGBA);
        g_QuadTexture->Render(g_Ocean->GetGradientsID(), 5 + 1 * side, g_WindowH - side, g_Ocean->FFT_SIZE, g_Ocean->FFT_SIZE, QuadTexture::RG);
        g_QuadTexture->Render(g_Ocean->GetFoamBufferID(), 5 + 2 * side, g_WindowH - side, g_Ocean->FFT_SIZE, g_Ocean->FFT_SIZE, QuadTexture::R);

        if (g_Ship->GetTraceID())
            g_QuadTexture->Render(g_Ship->GetTraceID(), 5, 5, 512, 512, QuadTexture::R);
    }

    if (g_bTextureWakeDisplay)
        if (bTexWakeByVAO)
            g_QuadTexture->Render(TexWakeVao, 5, 5, TexWakeVaoSize, TexWakeVaoSize, QuadTexture::R);
        else
            g_QuadTexture->Render(TexWakeBuffer, 5, 5, TexWakeBufferSize, TexWakeBufferSize, QuadTexture::R);
}

// Dashboard 2D of the ship
void DrawIcon(int image, float x, float y)
{
    int wi, hi;
    nvgImageSize(g_Nvg, image, &wi, &hi);

    nvgBeginPath(g_Nvg);
    nvgRoundedRect(g_Nvg, x, y, wi, hi, 5);
    nvgFillColor(g_Nvg, g_ColorSimShip3);
    nvgFill(g_Nvg);

    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x, y, wi, hi);
    NVGpaint imgPaint = nvgImagePattern(g_Nvg, x, y, wi, hi, 0, image, 1.0f);
    nvgFillPaint(g_Nvg, imgPaint);
    nvgFill(g_Nvg);
}
void DrawCircle(float x, float y, float diameter, NVGcolor fill, const char* label, float fontsize, NVGcolor text)
{
    float r = diameter * 0.5f;
    float cx = x + r;
    float cy = y + r;

    nvgBeginPath(g_Nvg);
    nvgCircle(g_Nvg, cx, cy, r);
    nvgFillColor(g_Nvg, fill);
    nvgFill(g_Nvg);
    
    nvgFillColor(g_Nvg, text);
    nvgFontSize(g_Nvg, fontsize);
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(g_Nvg, cx, cy, label, nullptr);
};
void DrawRoundRect(float x, float y, float w, float h, NVGcolor fill, const char* label, float fontsize, NVGcolor text)
{
    nvgBeginPath(g_Nvg);
    nvgRoundedRect(g_Nvg, x, y, w, h, 5.0f);
    nvgFillColor(g_Nvg, fill);
    nvgFill(g_Nvg);
    
    nvgFillColor(g_Nvg, text);
    nvgFontSize(g_Nvg, fontsize);
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(g_Nvg, x + w / 2, y + h / 2 + 0.5f, label, nullptr);
}
void DrawRoundFrame(float x, float y, float w, float h, NVGcolor fill)
{
    nvgBeginPath(g_Nvg);
    nvgRoundedRect(g_Nvg, x, y, w, h, 10);
    nvgFillColor(g_Nvg, fill);
    nvgFill(g_Nvg);
    nvgStrokeColor(g_Nvg, g_ColorWhite);
    nvgStrokeWidth(g_Nvg, 2.0f);
    nvgStroke(g_Nvg);
}

void RenderEnv(float x, float y)
{
    float w = 80;
    float h = 136;
    float buttonWidth = 72;
    float buttonHeight = 28;
    float gap = 5;

    // Rounded rectangle
    DrawRoundFrame(x, y, w, h, g_ColorSimShip4);

    // Camera mode button
    float currentY = y + 5;
    NVGcolor color;
    switch (g_Camera.GetMode())
    {
    case eCameraMode::ORBITAL:  color = g_ColorAmbre;  break;
    case eCameraMode::BRIDGE:   color = g_ColorRed;  break;
    case eCameraMode::FPS:      color = g_ColorGreen; break;
    }
    string mode;
    switch (g_Camera.GetMode())
    {
    case eCameraMode::ORBITAL:  mode = "ORBITAL"; break;
    case eCameraMode::BRIDGE:
        switch (g_eBridgeView)
        {
        case eBridgeView::WHEEL:    mode = "BRIDGE"; break;
        case eBridgeView::LEFT:     mode = "LEFT"; break;
        case eBridgeView::RIGHT:    mode = "RIGHT"; break;
        case eBridgeView::BOW:      mode = "BOW"; break;
        case eBridgeView::STERN:    mode = "STERN"; break;
        }
        break;
    case eCameraMode::FPS:      mode = "FPS"; break;
    }
    nvgFontFace(g_Nvg, "arial");
    DrawRoundRect(x + (w - buttonWidth) / 2, currentY, buttonWidth, buttonHeight, color, mode.c_str(), 16.0f, g_ColorWhite);

    // Lookat angle
    buttonWidth = 60;
    buttonHeight = 18;
    char lookat[16];
    snprintf(lookat, sizeof(lookat), "%03d°", (int)g_Camera.GetNorthAngleDEG());
    DrawRoundRect(x + (w - buttonWidth) / 2, y + 38, buttonWidth, buttonHeight, g_ColorSimShip1, lookat, 16.0f, g_ColorCyan);
    
    // Fps
    char fps[16];
    snprintf(fps, sizeof(fps), "%d i/s", g_Fps);
    DrawRoundRect(x + (w - buttonWidth) / 2, y + 61, buttonWidth, buttonHeight, g_ColorSimShip1, fps, 16.0f, g_ColorCyan);

    // Icon Sound
    if (g_SoundMgr->bSound) DrawIcon(g_NvgImgSoundUp, x + 15, 88);
    else                    DrawIcon(g_NvgImgSoundOff, x + 15, 88);
    g_CtrlSound = vec4(x + 15, 88, 20.0f, 20.0f);
    
    // Icon Timer
    DrawIcon(g_NvgImgTimer, x + 45, 88);
    g_CtrlTimer = vec4(x + 45, 88, 20.0f, 20.0f);

    // Icons Meteo
    DrawIcon(g_NvgImgMto1, x + 5, 112);
    g_CtrlMto1 = vec4(x + 5, 112, 20.0f, 20.0f);

    DrawIcon(g_NvgImgMto2, x + 30, 112);
    g_CtrlMto2 = vec4(x + 30, 112, 20.0f, 20.0f);

    DrawIcon(g_NvgImgMto3, x + 55, 112);
    g_CtrlMto3 = vec4(x + 55, 112, 20.0f, 20.0f);

    // Night overlay
    if (g_Sky->SunPosition.y < 0.0f)
    {
        nvgBeginPath(g_Nvg);
        nvgRoundedRect(g_Nvg, x, y, w, h, 10);
        nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 128));
        nvgFill(g_Nvg);
    }
}
void RenderControlFrame(float x, float y, float w, float h)
{
    // Draw the rectangle
    nvgBeginPath(g_Nvg);
    nvgRoundedRect(g_Nvg, x, y, w, h, 10);
    nvgFillColor(g_Nvg, g_ColorSimShip4);
    nvgFill(g_Nvg);
    nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgStrokeWidth(g_Nvg, 2.0f);
    nvgStroke(g_Nvg);

    // TWS text
    nvgFontFace(g_Nvg, "arial");
    nvgFillColor(g_Nvg, nvgRGBA(255, 0, 0, 255));
    nvgTextAlign(g_Nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgFontSize(g_Nvg, 8.0f);
    nvgText(g_Nvg, x + 5, y + 5, "TWS (kn)", nullptr);
    char windText[32];
    snprintf(windText, sizeof(windText), "%.0f", g_TWS_Kn);
    nvgFontSize(g_Nvg, 20.0f);
    nvgText(g_Nvg, x + 5, y + 15, windText, nullptr);

    g_CtrlWind = vec4(x + 3, y + 15, 20, 20);

    // AWS text
    nvgFillColor(g_Nvg, nvgRGBA(0, 0, 255, 255));
    nvgTextAlign(g_Nvg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
    nvgFontSize(g_Nvg, 8.0f);
    nvgText(g_Nvg, x + 5, y + h - 25, "AWS (kn)", nullptr);
    snprintf(windText, sizeof(windText), "%.1f", ms_to_knot(g_Ship->AWS));
    nvgFontSize(g_Nvg, 20.0f);
    nvgText(g_Nvg, x + 5, y + h - 3, windText, nullptr);
}
void RenderCompass(float x, float y, float radius)
{
    // Settings
    float thicknessCircle = 2.0f;
    float lengthGraduation = 5.0f;
    float sizeTriangle = 15.0f;
    float sizeTexDisplay = 8.0f;
    float sizeNumDisplay = 22.0f;
    float spacing = 18.0f; // Vertical spacing between displays

    // Circle ==========================

    nvgBeginPath(g_Nvg);
    nvgCircle(g_Nvg, x, y, radius);
    nvgFillColor(g_Nvg, g_ColorSimShip1);
    nvgFill(g_Nvg);
    //nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    //nvgStrokeWidth(g_Nvg, thicknessCircle);
    //nvgStroke(g_Nvg);

    // Graduations and cardinal points
    for (int i = 0; i < 360; i += 10)
    {
        float angle = i * NVG_PI / 180.0f;
        float cx = x + cosf(angle) * radius;
        float cy = y + sinf(angle) * radius;

        nvgBeginPath(g_Nvg);
        nvgMoveTo(g_Nvg, cx, cy);
        nvgLineTo(g_Nvg, x + cosf(angle) * (radius - lengthGraduation), y + sinf(angle) * (radius - lengthGraduation));
        nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
        nvgStrokeWidth(g_Nvg, 1.0f);
        nvgStroke(g_Nvg);
    }

    // HDG ==========================

    float angleTriangle = (g_Ship->HDG - 90.0f) * NVG_PI / 180.0f;
    float baseTriangle = 10.0f;     // Width of the base of the triangle
    float heightTriangle = 17.0f;  // Height of the triangle

    nvgBeginPath(g_Nvg);
    // Point of the triangle on the outer circle
    nvgMoveTo(g_Nvg, x + cosf(angleTriangle) * radius, y + sinf(angleTriangle) * radius);
    // Left corner of the base
    nvgLineTo(g_Nvg,
        x + cosf(angleTriangle) * (radius - heightTriangle) - sinf(angleTriangle) * (baseTriangle / 2),
        y + sinf(angleTriangle) * (radius - heightTriangle) + cosf(angleTriangle) * (baseTriangle / 2));
    // Right corner of the base
    nvgLineTo(g_Nvg,
        x + cosf(angleTriangle) * (radius - heightTriangle) + sinf(angleTriangle) * (baseTriangle / 2),
        y + sinf(angleTriangle) * (radius - heightTriangle) - cosf(angleTriangle) * (baseTriangle / 2));
    nvgClosePath(g_Nvg);
    nvgFillColor(g_Nvg, g_ColorCyan);
    nvgFill(g_Nvg);

    // HDG display 
    nvgFontFace(g_Nvg, "arial");
    nvgFillColor(g_Nvg, g_ColorCyan);
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    char strHDG[4];
    strcpy(strHDG, "HDG");
    nvgFontSize(g_Nvg, sizeTexDisplay);
    nvgText(g_Nvg, x, y - 2.0f * spacing - 3.0f, strHDG, NULL);
    char capStrHDG[10];
    snprintf(capStrHDG, sizeof(capStrHDG), "%.1f°", g_Ship->HDG);
    nvgFontSize(g_Nvg, sizeNumDisplay);
    nvgText(g_Nvg, x, y - spacing - 5.0f, capStrHDG, NULL);

    // COG ==========================

    angleTriangle = (g_Ship->COG - 90.0f) * NVG_PI / 180.0f;

    nvgBeginPath(g_Nvg);
    // Point of the triangle on the outer circle
    nvgMoveTo(g_Nvg, x + cosf(angleTriangle) * radius, y + sinf(angleTriangle) * radius);
    // Left corner of the base
    nvgLineTo(g_Nvg,
        x + cosf(angleTriangle) * (radius - heightTriangle) - sinf(angleTriangle) * (baseTriangle / 2),
        y + sinf(angleTriangle) * (radius - heightTriangle) + cosf(angleTriangle) * (baseTriangle / 2));
    // Right corner of the base
    nvgLineTo(g_Nvg,
        x + cosf(angleTriangle) * (radius - heightTriangle) + sinf(angleTriangle) * (baseTriangle / 2),
        y + sinf(angleTriangle) * (radius - heightTriangle) - cosf(angleTriangle) * (baseTriangle / 2));
    nvgClosePath(g_Nvg);
    nvgFillColor(g_Nvg, g_ColorYellow);
    nvgFill(g_Nvg);

    // COG display
    nvgFontFace(g_Nvg, "arial");
    nvgFillColor(g_Nvg, g_ColorYellow);
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    char strCOG[4];
    strcpy(strCOG, "COG");
    nvgFontSize(g_Nvg, sizeTexDisplay);
    nvgText(g_Nvg, x, y - 5.0f, strCOG, NULL);
    char capStrCOG[10];
    snprintf(capStrCOG, sizeof(capStrCOG), "%.1f°", g_Ship->COG);
    nvgFontSize(g_Nvg, sizeNumDisplay);
    nvgText(g_Nvg, x, y + spacing - 7.0f, capStrCOG, NULL);

    // Drift angle display
    char capStrDrift[16];
    snprintf(capStrDrift, sizeof(capStrDrift), "Drift: %.1f°", g_Ship->DriftAngleDeg);
    nvgFontSize(g_Nvg, sizeTexDisplay * 1.6f);
    nvgFontFace(g_Nvg, "arial");
    nvgFillColor(g_Nvg, g_ColorAmbre);
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(g_Nvg, x, y + 2.0f * spacing - 5.0f, capStrDrift, NULL);

    // Wind marker ==========================
    
    float angleWind = (g_TWS_Deg - 90.0f) * NVG_PI / 180.0f;
    float triangleHauteur = 10.8f;
    float triangleBase = 9.35f;
    // Point slightly tucked into the circle
    float pointeR = radius - 3.0f; // 3px à l'intérieur du bord
    float outerX = x + cosf(angleWind) * pointeR;
    float outerY = y + sinf(angleWind) * pointeR;
    // Base positioned outside the point (always outside the circle)
    float baseCenterX = x + cosf(angleWind) * (pointeR + triangleHauteur);
    float baseCenterY = y + sinf(angleWind) * (pointeR + triangleHauteur);
    // Perpendicular direction for the base
    float perpAngleL = angleWind + NVG_PI / 2.0f;
    float perpAngleR = angleWind - NVG_PI / 2.0f;
    // The two corners of the base
    float leftBaseX = baseCenterX + cosf(perpAngleL) * (triangleBase / 2.0f);
    float leftBaseY = baseCenterY + sinf(perpAngleL) * (triangleBase / 2.0f);
    float rightBaseX = baseCenterX + cosf(perpAngleR) * (triangleBase / 2.0f);
    float rightBaseY = baseCenterY + sinf(perpAngleR) * (triangleBase / 2.0f);
    nvgBeginPath(g_Nvg);
    // Point of the triangle (entered into the circle)
    nvgMoveTo(g_Nvg, outerX, outerY);
    // First base corner (left)
    nvgLineTo(g_Nvg, leftBaseX, leftBaseY);
    // Second base corner (right)
    nvgLineTo(g_Nvg, rightBaseX, rightBaseY);
    nvgClosePath(g_Nvg);
    nvgFillColor(g_Nvg, g_ColorRed);
    nvgFill(g_Nvg);

    // HDG instruction marker ========================

    float angleInstruc = (g_Ship->HDGInstruction - 90.0f) * NVG_PI / 180.0f;

    // Center marker position, just outside the circle
    float markDist = radius + 4.0f; // distance from the center of the circle to the center of the square
    float centerX = x + cosf(angleInstruc) * markDist;
    float centerY = y + sinf(angleInstruc) * markDist;

    // Size of the rectangle (width in perpendicular direction, height in vector direction)
    float rectWidth = 6.0f;     // width of the small square/rectangle
    float rectHeight = 6.0f;    // height (radial direction)

    // Direction and perpendicular vectors to orient the rectangle
    float dirX = cosf(angleInstruc);
    float dirY = sinf(angleInstruc);
    float perpX = cosf(angleInstruc + NVG_PI / 2);
    float perpY = sinf(angleInstruc + NVG_PI / 2);

    // Calculation of the 4 corners of the rectangle (starting from the base side of the circle, going clockwise)
    float halfWidth = rectWidth / 2;

    // Corner coordinates
 
    // 1 = near corner of circle, left
    float x1 = centerX - perpX * halfWidth - dirX * (rectHeight / 2);
    float y1 = centerY - perpY * halfWidth - dirY * (rectHeight / 2);
    // 2 = corner near the circle, right
    float x2 = centerX + perpX * halfWidth - dirX * (rectHeight / 2);
    float y2 = centerY + perpY * halfWidth - dirY * (rectHeight / 2);
    // 3 = outside corner, right
    float x3 = centerX + perpX * halfWidth + dirX * (rectHeight / 2);
    float y3 = centerY + perpY * halfWidth + dirY * (rectHeight / 2);
    // 4 = outside corner, left
    float x4 = centerX - perpX * halfWidth + dirX * (rectHeight / 2);
    float y4 = centerY - perpY * halfWidth + dirY * (rectHeight / 2);

    // Drawing of the colored solid rectangle (in the center)
    nvgBeginPath(g_Nvg);
    nvgMoveTo(g_Nvg, x1, y1);
    nvgLineTo(g_Nvg, x2, y2);
    nvgLineTo(g_Nvg, x3, y3);
    nvgLineTo(g_Nvg, x4, y4);
    nvgClosePath(g_Nvg);
    nvgFillColor(g_Nvg, g_Ship->bAutopilot ? g_ColorGreen : g_ColorRed);
    nvgFill(g_Nvg);

    // Drawing of the two white side borders
    nvgStrokeWidth(g_Nvg, 1.0f);
    nvgStrokeColor(g_Nvg, g_ColorWhite);

    // Left side border (line between x1-y1 and x4-y4)
    nvgBeginPath(g_Nvg);
    nvgMoveTo(g_Nvg, x1, y1);
    nvgLineTo(g_Nvg, x4, y4);
    nvgStroke(g_Nvg);

    // Right side border (line between x2-y2 and x3-y3)
    nvgBeginPath(g_Nvg);
    nvgMoveTo(g_Nvg, x2, y2);
    nvgLineTo(g_Nvg, x3, y3);
    nvgStroke(g_Nvg);

    // Outer side border (line between x3-y3 and x4-y4)
    nvgBeginPath(g_Nvg);
    nvgMoveTo(g_Nvg, x3, y3);
    nvgLineTo(g_Nvg, x4, y4);
    nvgStroke(g_Nvg);

    // Rate of turn ==================================

    if (fabsf(g_Ship->YawVelocity * (180.0f / M_PI) * 60.0f) > 1.0f)
    { 
        float rate = -g_Ship->YawVelocity * 60.0f;
        float rayonArc = radius - 10.0f;
        float angleStart = (g_Ship->HDG - 90.0f) * NVG_PI / 180.0f;
        float angleEnd = angleStart + rate;

        nvgBeginPath(g_Nvg);
        nvgArc(g_Nvg, x, y, rayonArc, angleStart, angleEnd, rate < 0.0f ? NVG_CCW : NVG_CW);
        nvgStrokeColor(g_Nvg, nvgRGBA(0, 200, 200, 180));
        nvgStrokeWidth(g_Nvg, 4.0f);
        nvgStroke(g_Nvg);
    }
}
void RenderThrottle(float x, float y, float w, float h, float e)
{
    float n = std::min(10.0f, 2.0f * g_Ship->ship.PowerStepMax);
    float dh = h / n;
    float value1 = 0.5f + g_Ship->PowerCurrentStep1 / (2.0f * g_Ship->ship.PowerStepMax);
    float value2 = 0.5f + g_Ship->PowerCurrentStep2 / (2.0f * g_Ship->ship.PowerStepMax);

    float xx = x;

    if (g_Ship->ship.nPropeller == 2)
    {
        g_CtrlThrottle1 = vec4(x, y, w, h);
        g_CtrlThrottleHigh1 = y;
        g_CtrlThrottleLow1 = y + h;

        g_CtrlThrottle2 = vec4(x + w + e, y, w, h);
        g_CtrlThrottleHigh2 = y;
        g_CtrlThrottleLow2 = y + h;

        // S L I D E R  1 ================

        // Slider background
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, xx, y, w, h);
        nvgFillColor(g_Nvg, nvgRGBA(36, 36, 36, 255));
        nvgFill(g_Nvg);
        nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
        nvgStrokeWidth(g_Nvg, 2.0f);
        nvgStroke(g_Nvg);

        // Green part (upper)
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, xx, y, w, h * 0.5f);
        nvgFillColor(g_Nvg, nvgRGBA(0, 200, 0, 255));
        nvgFill(g_Nvg);

        if (g_Ship->PropRpm1 > 0)
        {
            float r = g_Ship->PropRpm1 / g_Ship->ship.PropRpmMax;
            float yy = y + (1.0f - r) * h * 0.5f;
            nvgBeginPath(g_Nvg);
            nvgRect(g_Nvg, xx, y + (1.0f - r) * h * 0.5f, w, r * h * 0.5f);
            nvgFillColor(g_Nvg, nvgRGBA(0, 255, 0, 255));
            nvgFill(g_Nvg);

            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xx, yy);
            nvgLineTo(g_Nvg, xx + w, yy);
            nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
            nvgStrokeWidth(g_Nvg, 1.0f);
            nvgStroke(g_Nvg);
        }

        // Red part (lower)
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, xx, y + h * 0.5f, w, h * 0.5f);
        nvgFillColor(g_Nvg, nvgRGBA(200, 0, 0, 255));
        nvgFill(g_Nvg);

        if (g_Ship->PropRpm1 < 0)
        {
            float r = g_Ship->PropRpm1 / -g_Ship->ship.PropRpmMax;
            float yy = y + h * 0.5f + r * h * 0.5f;
            nvgBeginPath(g_Nvg);
            nvgRect(g_Nvg, xx, y + h * 0.5f, w, r * h * 0.5f);
            nvgFillColor(g_Nvg, nvgRGBA(255, 0, 0, 255));
            nvgFill(g_Nvg);

            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xx, yy);
            nvgLineTo(g_Nvg, xx + w, yy);
            nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
            nvgStrokeWidth(g_Nvg, 1.0f);
            nvgStroke(g_Nvg);
        }

        // Separations
        nvgStrokeColor(g_Nvg, nvgRGBA(0, 0, 0, 100));
        nvgStrokeWidth(g_Nvg, 1);
        for (int i = 1; i < n; i++)
        {
            float yPos = y + i * dh;
            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xx, yPos);
            nvgLineTo(g_Nvg, xx + w, yPos);
            nvgStroke(g_Nvg);
        }

        // Cursor
        float curseurY = y + h * (1 - value1);

        // Lateral axes
        nvgBeginPath(g_Nvg);
        nvgFillColor(g_Nvg, nvgRGBA(128, 128, 128, 255));
        nvgRect(g_Nvg, xx - 2, curseurY, 4, h / 2 - h * (1 - value1));
        nvgRect(g_Nvg, xx + w - 2, curseurY, 4, h / 2 - h * (1 - value1));
        nvgFill(g_Nvg);

        // Central controller
        nvgBeginPath(g_Nvg);
        int hh = 8;
        nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
        nvgRect(g_Nvg, xx - 2, curseurY - hh / 2, w + 4, hh);
        nvgFill(g_Nvg);

        // S L I D E R  2 ================

        xx = x + w + e;

        // Slider background
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, xx, y, w, h);
        nvgFillColor(g_Nvg, nvgRGBA(36, 36, 36, 255));
        nvgFill(g_Nvg);
        nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
        nvgStrokeWidth(g_Nvg, 2.0f);
        nvgStroke(g_Nvg);

        // Green part (upper)
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, xx, y, w, h * 0.5f);
        nvgFillColor(g_Nvg, nvgRGBA(0, 200, 0, 255));
        nvgFill(g_Nvg);

        if (g_Ship->PropRpm2 > 0)
        {
            float r = g_Ship->PropRpm2 / g_Ship->ship.PropRpmMax;
            float yy = y + (1.0f - r) * h * 0.5f;
            nvgBeginPath(g_Nvg);
            nvgRect(g_Nvg, xx, y + (1.0f - r) * h * 0.5f, w, r * h * 0.5f);
            nvgFillColor(g_Nvg, nvgRGBA(0, 255, 0, 255));
            nvgFill(g_Nvg);

            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xx, yy);
            nvgLineTo(g_Nvg, xx + w, yy);
            nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
            nvgStrokeWidth(g_Nvg, 1.0f);
            nvgStroke(g_Nvg);
        }

        // Red part (lower)
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, xx, y + h * 0.5f, w, h * 0.5f);
        nvgFillColor(g_Nvg, nvgRGBA(200, 0, 0, 255));
        nvgFill(g_Nvg);

        if (g_Ship->PropRpm2 < 0)
        {
            float r = g_Ship->PropRpm2 / -g_Ship->ship.PropRpmMax;
            float yy = y + h * 0.5f + r * h * 0.5f;
            nvgBeginPath(g_Nvg);
            nvgRect(g_Nvg, xx, y + h * 0.5f, w, r * h * 0.5f);
            nvgFillColor(g_Nvg, nvgRGBA(255, 0, 0, 255));
            nvgFill(g_Nvg);

            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xx, yy);
            nvgLineTo(g_Nvg, xx + w, yy);
            nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
            nvgStrokeWidth(g_Nvg, 1.0f);
            nvgStroke(g_Nvg);
        }

        // Separations
        nvgStrokeColor(g_Nvg, nvgRGBA(0, 0, 0, 100));
        nvgStrokeWidth(g_Nvg, 1);
        for (int i = 1; i < n; i++)
        {
            float yPos = y + i * dh;
            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xx, yPos);
            nvgLineTo(g_Nvg, xx + w, yPos);
            nvgStroke(g_Nvg);
        }

        // Cursor
        curseurY = y + h * (1 - value2);

        // Lateral axes
        nvgBeginPath(g_Nvg);
        nvgFillColor(g_Nvg, nvgRGBA(128, 128, 128, 255));
        nvgRect(g_Nvg, xx - 2, curseurY, 4, h / 2 - h * (1 - value2));
        nvgRect(g_Nvg, xx + w - 2, curseurY, 4, h / 2 - h * (1 - value2));
        nvgFill(g_Nvg);

        // Central controller
        nvgBeginPath(g_Nvg);
        hh = 8;
        nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
        nvgRect(g_Nvg, xx - 2, curseurY - hh / 2, w + 4, hh);
        nvgFill(g_Nvg);
    }
    else
    {
        g_CtrlThrottle1 = vec4(0.0f);

        g_CtrlThrottle2 = vec4(x, y, w, h);
        g_CtrlThrottleHigh2 = y;
        g_CtrlThrottleLow2 = y + h;
        
        // Slider background
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, xx, y, w, h);
        nvgFillColor(g_Nvg, nvgRGBA(36, 36, 36, 255));
        nvgFill(g_Nvg);
        nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
        nvgStrokeWidth(g_Nvg, 2.0f);
        nvgStroke(g_Nvg);

        // Green part (upper)
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, xx, y, w, h * 0.5f);
        nvgFillColor(g_Nvg, nvgRGBA(0, 200, 0, 255));
        nvgFill(g_Nvg);

        if (g_Ship->PropRpm2 > 0)
        {
            float r = g_Ship->PropRpm2 / g_Ship->ship.PropRpmMax;
            float yy = y + (1.0f - r) * h * 0.5f;
            nvgBeginPath(g_Nvg);
            nvgRect(g_Nvg, xx, y + (1.0f - r) * h * 0.5f, w, r * h * 0.5f);
            nvgFillColor(g_Nvg, nvgRGBA(0, 255, 0, 255));
            nvgFill(g_Nvg);

            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xx, yy);
            nvgLineTo(g_Nvg, xx + w, yy);
            nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
            nvgStrokeWidth(g_Nvg, 1.0f);
            nvgStroke(g_Nvg);
        }

        // Red part (lower)
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, xx, y + h * 0.5f, w, h * 0.5f);
        nvgFillColor(g_Nvg, nvgRGBA(200, 0, 0, 255));
        nvgFill(g_Nvg);

        if (g_Ship->PropRpm2 < 0)
        {
            float r = g_Ship->PropRpm2 / -g_Ship->ship.PropRpmMax;
            float yy = y + h * 0.5f + r * h * 0.5f;
            nvgBeginPath(g_Nvg);
            nvgRect(g_Nvg, xx, y + h * 0.5f, w, r * h * 0.5f);
            nvgFillColor(g_Nvg, nvgRGBA(255, 0, 0, 255));
            nvgFill(g_Nvg);

            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xx, yy);
            nvgLineTo(g_Nvg, xx + w, yy);
            nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
            nvgStrokeWidth(g_Nvg, 1.0f);
            nvgStroke(g_Nvg);
        }

        // Separations
        nvgStrokeColor(g_Nvg, nvgRGBA(0, 0, 0, 100));
        nvgStrokeWidth(g_Nvg, 1);
        for (int i = 1; i < n; i++)
        {
            float yPos = y + i * dh;
            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xx, yPos);
            nvgLineTo(g_Nvg, xx + w, yPos);
            nvgStroke(g_Nvg);
        }

        // Cursor
        float curseurY = y + h * (1 - value2);

        // Lateral axes
        nvgBeginPath(g_Nvg);
        nvgFillColor(g_Nvg, nvgRGBA(128, 128, 128, 255));
        nvgRect(g_Nvg, xx - 2, curseurY, 4, h / 2 - h * (1 - value2));
        nvgRect(g_Nvg, xx + w - 2, curseurY, 4, h / 2 - h * (1 - value2));
        nvgFill(g_Nvg);

        // Central controller
        nvgBeginPath(g_Nvg);
        int hh = 8;
        nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
        nvgRect(g_Nvg, xx - 2, curseurY - hh / 2, w + 4, hh);
        nvgFill(g_Nvg);
    }

    // T E X T =======================
    
    // Speed ​​Update
	bool bUpdate = false;
    static double prevTime = 0.0;
    if (g_TimerShipMotion.getTime() - prevTime > 0.5)
    {
        bUpdate = true;
        prevTime = g_TimerShipMotion.getTime();
    }
    else
        bUpdate = false;

    // Speed
    static char text[50];
    if (bUpdate)
        sprintf_s(text, "%.2f kt", fabs(ms_to_knot(g_Ship->SOG)));
    nvgFontSize(g_Nvg, 16.0f);
    nvgFontFace(g_Nvg, "arial");
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
    nvgText(g_Nvg, xx + w + 35, y + h * 0.5f, text, NULL);

    // Directional triangle
    float triangleX = xx + w + 35;
    // Upward triangle (forward)
    float triangleY = y + h * 0.5f - 20;
    nvgBeginPath(g_Nvg);
    nvgMoveTo(g_Nvg, triangleX - 10, triangleY + 8);
    nvgLineTo(g_Nvg, triangleX + 10, triangleY + 8);
    nvgLineTo(g_Nvg, triangleX, triangleY - 8);
    if (g_Ship->LinearVelocity > 0) nvgFillColor(g_Nvg, nvgRGBA(250, 250, 250, 255));
    else                            nvgFillColor(g_Nvg, nvgRGBA(200, 200, 200, 255));
    nvgClosePath(g_Nvg);
    nvgFill(g_Nvg);

    // Downward triangle (backward)
    triangleY = y + h * 0.5f + 20;
    nvgBeginPath(g_Nvg);
    nvgMoveTo(g_Nvg, triangleX - 10, triangleY - 8);
    nvgLineTo(g_Nvg, triangleX + 10, triangleY - 8);
    nvgLineTo(g_Nvg, triangleX, triangleY + 8);
    if (g_Ship->LinearVelocity < 0) nvgFillColor(g_Nvg, nvgRGBA(250, 250, 250, 255));
    else                            nvgFillColor(g_Nvg, nvgRGBA(200, 200, 200, 255));
    nvgClosePath(g_Nvg);
    nvgFill(g_Nvg);

    // Lateral bow speed
    if (fabs(g_Ship->SOGbow) >= 0.1f)
    {
        static char text[50];
        if (bUpdate)
            sprintf_s(text, "%.1f kt", fabs(ms_to_knot(g_Ship->SOGbow)));
        nvgFontSize(g_Nvg, 12.0f);
        nvgFontFace(g_Nvg, "arial");
        nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        if (g_Ship->SOGbow < 0)
        {
            nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
            nvgText(g_Nvg, x - 35, y + 10, text, NULL);
            // Triangle
            float xxx = x - 5;
            float yy = y + 10;
            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xxx - 8, yy);
            nvgLineTo(g_Nvg, xxx, yy - 5);
            nvgLineTo(g_Nvg, xxx, yy + 5);
            nvgFillColor(g_Nvg, nvgRGBA(200, 0, 0, 255));
            nvgFill(g_Nvg);
            nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
            nvgStrokeWidth(g_Nvg, 1.0f);
            nvgStroke(g_Nvg);
        }
        else
        {
            nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
            nvgText(g_Nvg, xx + w + 35, y + 10, text, NULL);
            // Triangle
            float xxx = xx + w + 5;
            float yy = y + 10;
            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xxx + 8, yy);
            nvgLineTo(g_Nvg, xxx, yy - 5);
            nvgLineTo(g_Nvg, xxx, yy + 5);
            nvgFillColor(g_Nvg, nvgRGBA(0, 200, 0, 255));
            nvgFill(g_Nvg);
            nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
            nvgStrokeWidth(g_Nvg, 1.0f);
            nvgStroke(g_Nvg);
        }
    }

    // Stern lateral speed
    if (fabs(g_Ship->SOGstern) >= 0.1f)
    {
        static char text[50];
        if (bUpdate)
            sprintf_s(text, "%.1f kt", fabs(ms_to_knot(g_Ship->SOGstern)));
        nvgFontSize(g_Nvg, 12.0f);
        nvgFontFace(g_Nvg, "arial");
        nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        if (g_Ship->SOGstern < 0)
        {
            nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
            nvgText(g_Nvg, x - 35, y + h - 10, text, NULL);
            // Triangle
            float xxx = x - 5;
            float yy = y + h - 10;
            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xxx - 8, yy);
            nvgLineTo(g_Nvg, xxx, yy - 5);
            nvgLineTo(g_Nvg, xxx, yy + 5);
            nvgFillColor(g_Nvg, nvgRGBA(200, 0, 0, 255));
            nvgFill(g_Nvg);
            nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
            nvgStrokeWidth(g_Nvg, 1.0f);
            nvgStroke(g_Nvg);
        }
        else
        {
            nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
            nvgText(g_Nvg, xx + w + 35, y + h - 10, text, NULL);
            // Triangle
            float xxx = xx + w + 5;
            float yy = y + h - 10;
            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xxx + 8, yy);
            nvgLineTo(g_Nvg, xxx, yy - 5);
            nvgLineTo(g_Nvg, xxx, yy + 5);
            nvgFillColor(g_Nvg, nvgRGBA(0, 200, 0, 255));
            nvgFill(g_Nvg);
            nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
            nvgStrokeWidth(g_Nvg, 1.0f);
            nvgStroke(g_Nvg);
        }
    }
}
void RenderBowThruster(float x, float y, float w, float h)
{
    if (!g_Ship-> ship.HasBowThruster)
        return;

    float largeur = w;
    float hauteur = h;
    float n = 2.0f * g_Ship->ship.BowThrusterStepMax;
    float separationLargeur = largeur / n;
    float valeur = 0.5f + g_Ship->BowThrusterCurrentStep / (2.0f * g_Ship->ship.BowThrusterStepMax);

    g_CtrlBowThruster = vec4(x, y, w, h);
    g_CtrlBowThrusterLeft = x;
    g_CtrlBowThrusterRight = x + w;

    // Slider background
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x, y, largeur, hauteur);
    nvgFillColor(g_Nvg, nvgRGBA(36, 36, 36, 255));
    nvgFill(g_Nvg);
    nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgStrokeWidth(g_Nvg, 2.0f);
    nvgStroke(g_Nvg);

    // Green part (right)
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x + largeur * 0.5f, y, largeur * 0.5f, hauteur);
    nvgFillColor(g_Nvg, nvgRGBA(0, 200, 0, 255));
    nvgFill(g_Nvg);

    if (g_Ship->BowThrusterRpm > 0)
    {
        float r = g_Ship->BowThrusterRpm / g_Ship->ship.BowThrusterRpmMax;
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, x + largeur * 0.5f, y, r * largeur * 0.5f, hauteur);
        nvgFillColor(g_Nvg, nvgRGBA(0, 255, 0, 255));
        nvgFill(g_Nvg);

        nvgBeginPath(g_Nvg);
        float xx = x + largeur * 0.5f + r * largeur * 0.5f;
        nvgMoveTo(g_Nvg, xx, y);
        nvgLineTo(g_Nvg, xx , y + hauteur);
        nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
        nvgStrokeWidth(g_Nvg, 1.0f);
        nvgStroke(g_Nvg);
    }

    // Red part (left)
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x, y, largeur * 0.5f, hauteur);
    nvgFillColor(g_Nvg, nvgRGBA(200, 0, 0, 255));
    nvgFill(g_Nvg);

    if (g_Ship->BowThrusterRpm < 0)
    {
        float r = g_Ship->BowThrusterRpm / -g_Ship->ship.BowThrusterRpmMax;
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, x + (1.0 - r) * largeur * 0.5f, y, r * largeur * 0.5f, hauteur);
        nvgFillColor(g_Nvg, nvgRGBA(255, 0, 0, 255));
        nvgFill(g_Nvg);

        nvgBeginPath(g_Nvg);
        float xx = x + (1.0 - r) * largeur * 0.5f;
        nvgMoveTo(g_Nvg, xx, y);
        nvgLineTo(g_Nvg, xx, y + hauteur);
        nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
        nvgStrokeWidth(g_Nvg, 1.0f);
        nvgStroke(g_Nvg);
    }

    // Separations
    nvgStrokeColor(g_Nvg, nvgRGBA(0, 0, 0, 100));
    nvgStrokeWidth(g_Nvg, 1);
    for (int i = 1; i < n; i++)
    {
        float xPos = x + i * separationLargeur;
        nvgBeginPath(g_Nvg);
        nvgMoveTo(g_Nvg, xPos, y);
        nvgLineTo(g_Nvg, xPos, y + hauteur);
        nvgStroke(g_Nvg);
    }

    // Cursor
    float curseurX = x + largeur * valeur;
 
    nvgBeginPath(g_Nvg);
    nvgFillColor(g_Nvg, nvgRGBA(128, 128, 128, 255));
    nvgRect(g_Nvg, curseurX, y - 2, largeur / 2 - largeur * valeur, 4);
    nvgRect(g_Nvg, curseurX, y + hauteur - 2, largeur / 2 - largeur * valeur, 4);
    nvgFill(g_Nvg);
    
    nvgBeginPath(g_Nvg);
    int ww = 6;
    nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
    nvgRect(g_Nvg, curseurX - ww / 2, y - 2, ww, hauteur + 4);
    nvgFill(g_Nvg);

}
void RenderSternThruster(float x, float y, float w, float h)
{
    if (!g_Ship->ship.HasSternThruster)
        return;

    float largeur = w;
    float hauteur = h;
    float n = 2.0f * g_Ship->ship.SternThrusterStepMax;
    float separationLargeur = largeur / n;
    float valeur = 0.5f + g_Ship->SternThrusterCurrentStep / (2.0f * g_Ship->ship.SternThrusterStepMax);

    g_CtrlSternThruster = vec4(x, y, w, h);
    g_CtrlSternThrusterLeft = x;
    g_CtrlSternThrusterRight = x + w;

    // Slider background
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x, y, largeur, hauteur);
    nvgFillColor(g_Nvg, nvgRGBA(36, 36, 36, 255));
    nvgFill(g_Nvg);
    nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgStrokeWidth(g_Nvg, 2.0f);
    nvgStroke(g_Nvg);

    // Green part (right)
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x + largeur * 0.5f, y, largeur * 0.5f, hauteur);
    nvgFillColor(g_Nvg, nvgRGBA(0, 200, 0, 255));
    nvgFill(g_Nvg);

    if (g_Ship->SternThrusterRpm > 0)
    {
        float r = g_Ship->SternThrusterRpm / g_Ship->ship.SternThrusterRpmMax;
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, x + largeur * 0.5f, y, r * largeur * 0.5f, hauteur);
        nvgFillColor(g_Nvg, nvgRGBA(0, 255, 0, 255));
        nvgFill(g_Nvg);

        nvgBeginPath(g_Nvg);
        float xx = x + largeur * 0.5f + r * largeur * 0.5f;
        nvgMoveTo(g_Nvg, xx, y);
        nvgLineTo(g_Nvg, xx, y + hauteur);
        nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
        nvgStrokeWidth(g_Nvg, 1.0f);
        nvgStroke(g_Nvg);
    }

    // Red part (left)
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x, y, largeur * 0.5f, hauteur);
    nvgFillColor(g_Nvg, nvgRGBA(200, 0, 0, 255));
    nvgFill(g_Nvg);

    if (g_Ship->SternThrusterRpm < 0)
    {
        float r = g_Ship->SternThrusterRpm / -g_Ship->ship.SternThrusterRpmMax;
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, x + (1.0 - r) * largeur * 0.5f, y, r * largeur * 0.5f, hauteur);
        nvgFillColor(g_Nvg, nvgRGBA(255, 0, 0, 255));
        nvgFill(g_Nvg);

        nvgBeginPath(g_Nvg);
        float xx = x + (1.0 - r) * largeur * 0.5f;
        nvgMoveTo(g_Nvg, xx, y);
        nvgLineTo(g_Nvg, xx, y + hauteur);
        nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
        nvgStrokeWidth(g_Nvg, 1.0f);
        nvgStroke(g_Nvg);
    }

    // Separations
    nvgStrokeColor(g_Nvg, nvgRGBA(0, 0, 0, 100));
    nvgStrokeWidth(g_Nvg, 1);
    for (int i = 1; i < n; i++)
    {
        float xPos = x + i * separationLargeur;
        nvgBeginPath(g_Nvg);
        nvgMoveTo(g_Nvg, xPos, y);
        nvgLineTo(g_Nvg, xPos, y + hauteur);
        nvgStroke(g_Nvg);
    }

    // Cursor
    float curseurX = x + largeur * valeur;

    nvgBeginPath(g_Nvg);
    nvgFillColor(g_Nvg, nvgRGBA(128, 128, 128, 255));
    nvgRect(g_Nvg, curseurX, y - 2, largeur / 2 - largeur * valeur, 4);
    nvgRect(g_Nvg, curseurX, y + hauteur - 2, largeur / 2 - largeur * valeur, 4);
    nvgFill(g_Nvg);

    nvgBeginPath(g_Nvg);
    int ww = 6;
    nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
    nvgRect(g_Nvg, curseurX - ww / 2, y - 2, ww, hauteur + 4);
    nvgFill(g_Nvg);

}
void RenderTime(float x, float y, float w, float h)
{
    sHM hm = g_Sky->GetTime();
    char buffer[12];
    int hr = hm.hour + hm.timezoneOffsetHours;
    if (hr > 23)
        hr -= 24;
    snprintf(buffer, sizeof(buffer), "%02d:%02d", hr, hm.minute);
    nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
    nvgFontSize(g_Nvg, 22.0f);
    nvgFontFace(g_Nvg, "arial");
    nvgTextAlign(g_Nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgText(g_Nvg, x, y - 2, buffer, nullptr);
    g_CtrlTimeHour = vec4(x, y - 2, 23, 20);
    g_CtrlTimeMinute = vec4(x + 23, y - 2, 23, 20);

    DrawIcon(g_NvgImgClock, x + w - 25, y - 3);
    g_CtrlNow = vec4(x + w - 25, y - 3, 20.0f, 20.0f);
}
void RenderPitchRollRot(float x, float y, float w, float h)
{
    // Black frame
    nvgBeginPath(g_Nvg);
    nvgRoundedRect(g_Nvg, x, y, w, h, 5);
    nvgFillColor(g_Nvg, g_ColorSimShip1);
    nvgFill(g_Nvg);

    float sizeTitle = 8.0f;
    float sizeValue = 12.0f;

    // List of values
    nvgFontFace(g_Nvg, "arial");
    nvgTextAlign(g_Nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    char text[50];
    float x1 = x + 40;

    // Pitch
    float y1 = y + 12;
    strcpy_s(text, "PITCH");
    nvgFontSize(g_Nvg, sizeTitle);
    nvgFillColor(g_Nvg, g_ColorWhite);
    nvgText(g_Nvg, x + 5, y1, text, NULL);

    nvgFontSize(g_Nvg, sizeValue);
    sprintf_s(text, "%.1f°", glm::degrees(g_Ship->Pitch));
    nvgFillColor(g_Nvg, g_ColorCyan);
    nvgText(g_Nvg, x1, y1, text, NULL);

    // Roll
    y1 += sizeValue + 6;
    strcpy_s(text, "ROLL");
    nvgFontSize(g_Nvg, sizeTitle);
    nvgFillColor(g_Nvg, g_ColorWhite);
    nvgText(g_Nvg, x + 5, y1, text, NULL);

    nvgFontSize(g_Nvg, sizeValue);
    sprintf_s(text, "%.1f°", glm::degrees(g_Ship->Roll));
    nvgFillColor(g_Nvg, g_ColorCyan);
    nvgText(g_Nvg, x1, y1, text, NULL);

    // Rate of turn
    y1 += sizeValue + 6;
    strcpy_s(text, "ROT");
    nvgFontSize(g_Nvg, sizeTitle);
    nvgFillColor(g_Nvg, g_ColorWhite);
    nvgText(g_Nvg, x + 5, y1, text, NULL);

    nvgFontSize(g_Nvg, sizeValue);
    sprintf_s(text, "%.0f°/mn", -g_Ship->YawVelocity * (180.0f / M_PI) * 60.0f);
    nvgFillColor(g_Nvg, g_ColorCyan);
    nvgText(g_Nvg, x1, y1, text, NULL);

    // Diameter
    y1 += sizeValue + 6;
    strcpy_s(text, "DIAM");
    nvgFontSize(g_Nvg, sizeTitle);
    nvgFillColor(g_Nvg, g_ColorWhite);
    nvgText(g_Nvg, x + 5, y1, text, NULL);

    nvgFontSize(g_Nvg, sizeValue);
    nvgFillColor(g_Nvg, g_ColorCyan);
    if (g_Ship->TurnDiameter_L < 20.0f)
        sprintf_s(text, "%.0f m", g_Ship->TurnDiameter_m);
    else
        sprintf_s(text, "> %.0f m", 20.0f * g_Ship->ship.Length);
    nvgText(g_Nvg, x1, y1, text, NULL);

    // Radius
    y1 += sizeValue + 6;
    strcpy_s(text, "RADIUS");
    nvgFontSize(g_Nvg, sizeTitle);
    nvgFillColor(g_Nvg, g_ColorWhite);
    nvgText(g_Nvg, x + 5, y1, text, NULL);

    nvgFontSize(g_Nvg, sizeValue);
    nvgFillColor(g_Nvg, g_ColorCyan);
    if (g_Ship->TurnDiameter_L < 20.0f)
        sprintf_s(text, "%.1f L", g_Ship->TurnDiameter_L);
    else
        sprintf_s(text, "> 20 L");
    nvgText(g_Nvg, x1, y1, text, NULL);

}
void RenderRudder(float x, float y, float w, float h)
{
    // Draw the left part (red)
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x, y, w / 2, h);
    nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgStrokeWidth(g_Nvg, 2.0f);
    nvgStroke(g_Nvg);
    nvgFillColor(g_Nvg, nvgRGBA(200, 0, 0, 255));
    nvgFill(g_Nvg);

    // Draw the right part (green)
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x + w / 2, y, w / 2, h);
    nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgStrokeWidth(g_Nvg, 1.0f);
    nvgStroke(g_Nvg);
    nvgFillColor(g_Nvg, nvgRGBA(0, 200, 0, 255));
    nvgFill(g_Nvg);

    g_CtrlRudder = vec4(x, y, w, h);
    g_CtrlRudderLeft = x;
    g_CtrlRudderRight = x + w;

    // Draw the actual position of the rudder
    float barrePos0 = x + w / 2 + ((float)-g_Ship->RudderAngleDeg / (float)g_Ship->ship.RudderStepMax) * (w / 2);
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, barrePos0, y, x + w / 2 - barrePos0, h);
    if (g_Ship->RudderAngleDeg < 0.0f)
        nvgFillColor(g_Nvg, nvgRGBA(0, 255, 0, 255));
    else
        nvgFillColor(g_Nvg, nvgRGBA(255, 0, 0, 255));
    nvgFill(g_Nvg);

    // Draw small horizontal ticks on each side of the vertical middle line when RudderAngleDeg == 0
    if (g_Ship->RudderCurrentStep == 0.0f)
    {
        float tickWidth = w * 0.05f; 
        float tickHeight = 2.0f;
        float centerX = x + w / 2;
        float tickY = y + h / 2;

        // Left line
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, centerX - tickWidth - 3.0f, tickY - tickHeight / 2, tickWidth, tickHeight);
        nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
        nvgFill(g_Nvg);

        // Right line
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, centerX + 3.0f, tickY - tickHeight / 2, tickWidth, tickHeight);
        nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
        nvgFill(g_Nvg);
    }

    // Draw the graduations
    int nbGraduations = static_cast<int>(ceil(g_Ship->ship.RudderStepMax / 10.0f));

    nvgStrokeColor(g_Nvg, nvgRGBA(150, 150, 150, 255));
    nvgStrokeWidth(g_Nvg, 1.0f);

    for (int i = 1; i < nbGraduations; i++) 
    {
        float graduationValue = i * 10.0f;
        float ratio = std::min(graduationValue / g_Ship->ship.RudderStepMax, 1.0f);

        // Graduations on the right
        float posX = x + w / 2 + ratio * (w / 2);
        nvgBeginPath(g_Nvg);
        nvgMoveTo(g_Nvg, posX, y);
        nvgLineTo(g_Nvg, posX, y + h);
        nvgStroke(g_Nvg);

        // Graduations on the left
        posX = x + w / 2 - ratio * (w / 2);
        nvgBeginPath(g_Nvg);
        nvgMoveTo(g_Nvg, posX, y);
        nvgLineTo(g_Nvg, posX, y + h);
        nvgStroke(g_Nvg);
    }
    
    // Calculate the position of the indicator bar
    float barrePos1 = x + w / 2 + ((float)-g_Ship->RudderCurrentStep / (float)g_Ship->ship.RudderStepMax) * (w / 2);
    float barreWidth = 2.0f;

    // Draw the indicator bar
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, barrePos1 - barreWidth / 2, y, barreWidth, h);
    nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
    nvgFill(g_Nvg);
}
void RenderControlFrameNight(float x, float y, float w, float h)
{
    // Night
    if (g_Sky->SunPosition.y < 0.0f)
    {
        nvgBeginPath(g_Nvg);
        nvgRoundedRect(g_Nvg, x, y, w, h, 10);
        nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 128));
        nvgFill(g_Nvg);
    }
}
void RenderAutopilot(float x, float y)
{
    float w = 80.0f;
    float h = 136.0f;
    bool isAuto = g_Ship->bAutopilot;

    // Main frame
    DrawRoundFrame(x, y, w, h, g_ColorSimShip4);

    // Auto/Stby button
    float buttonWidth = 72;
    float buttonHeight = 28;
    NVGcolor color = isAuto ? g_ColorGreen : g_ColorRed;
    DrawRoundRect(x + (w - buttonWidth) / 2, y + 5, buttonWidth, buttonHeight, color, isAuto ? "AUTO" : "STBY", 18.0f, g_ColorWhite);
    g_CtrlAutopilotCMD = vec4(x + (w - buttonWidth) / 2, y + 5, buttonWidth, buttonHeight);

    // Course instruction
    buttonWidth = 60;
    buttonHeight = 18;
    char course[10];
    snprintf(course, sizeof(course), "%03d°", g_Ship->HDGInstruction);
    DrawRoundRect(x + (w - buttonWidth) / 2, y + 38, buttonWidth, buttonHeight, g_ColorSimShip1, course, 16.0f, g_ColorCyan);

    // Control buttons
    float buttonSize = 30;
    float buttonX1 = x + 7;
    float buttonX2 = x + 43;
    float buttonY1 = y + 65;
    float buttonY2 = y + 100;

    DrawCircle(buttonX1, buttonY1, buttonSize, g_ColorSimShip3, "<", 16.0f, g_ColorWhite);
    DrawCircle(buttonX2, buttonY1, buttonSize, g_ColorSimShip3, ">", 16.0f, g_ColorWhite);
    DrawCircle(buttonX1, buttonY2, buttonSize, g_ColorSimShip3, "<<", 16.0f, g_ColorWhite);
    DrawCircle(buttonX2, buttonY2, buttonSize, g_ColorSimShip3, ">>", 16.0f, g_ColorWhite);

    g_CtrlAutopilotM1 = vec4(buttonX1, buttonY1, buttonSize, buttonSize);
    g_CtrlAutopilotP1 = vec4(buttonX2, buttonY1, buttonSize, buttonSize);
    g_CtrlAutopilotM10 = vec4(buttonX1, buttonY2, buttonSize, buttonSize);
    g_CtrlAutopilotP10 = vec4(buttonX2, buttonY2, buttonSize, buttonSize);
    
    // Night =====================

	if (g_Sky->SunPosition.y < 0.0f)
	{
        nvgBeginPath(g_Nvg);
        nvgRoundedRect(g_Nvg, x, y, w, h, 10);
        nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 128));
        nvgFill(g_Nvg);
    }
}
void RenderChronometer(float x, float y)
{
    if (!g_bChrono)
        return;

    double secondsTotal = g_Chrono.getTime();
    int totalSec = static_cast<int>(secondsTotal);
    int hours = totalSec / 3600;
    int minutes = (totalSec % 3600) / 60;
    int seconds = totalSec % 60;
    int deciseconds = static_cast<int>((secondsTotal - totalSec) * 10);

    ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << hours << ":"
        << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << seconds << "."
        << std::setw(0) << std::setfill('0') << deciseconds;

    nvgBeginPath(g_Nvg);
    nvgFontSize(g_Nvg, 24.0f);
    nvgFontFace(g_Nvg, "arial");
    nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
    nvgText(g_Nvg, x, y, oss.str().c_str(), nullptr);
}
void RenderDashboard()
{
    // Ship controls interface
    if (g_Ship && g_Ship->bVisible && !g_bBinoculars)
    {
        int widthAutopilot = 80;
        int widthSeparation = 5;
        float xChrono;

        if (g_Ship->ship.HasBowThruster)
        {
            int widthControlFrame = 455;
            if (g_Ship->ship.nPropeller == 2)
                widthControlFrame += 30 + 10;
            int startX = g_WindowW_2 - (widthControlFrame + 2 * widthSeparation + 2 * widthAutopilot) / 2;
            g_CtrlPanel = vec4(startX, 2, startX + widthControlFrame + 2 * widthSeparation + 2 * widthAutopilot, 136);
            RenderEnv(startX, 2);
            int xFrame = startX + widthAutopilot + widthSeparation;
            RenderControlFrame(xFrame, 2, widthControlFrame, 136);
            int x = xFrame + 90;
            RenderCompass(x, 70, 58);
            x += 80;
            RenderBowThruster(x, 35, 50, 20);
            RenderSternThruster(x, 65, 50, 20);
            x += 70;
            RenderThrottle(x, 10, 30, 100, 10);
            xChrono = x;
            x -= 45;
            if (g_Ship->ship.nPropeller == 2)
                x += (30 + 10) / 2;
            RenderRudder(x, 115, 120, 15);
            x += 150;
            if (g_Ship->ship.nPropeller == 2)
                x += (30 + 10) / 2;
            RenderTime(x, 13, 100, 30);
            RenderPitchRollRot(x, 35, 100, 95);
            RenderControlFrameNight(xFrame, 2, widthControlFrame, 136);
            RenderAutopilot(xFrame + widthControlFrame + widthSeparation, 2);
        }
        else
        {
            int widthControlFrame = 455 - 35;
            if (g_Ship->ship.nPropeller == 2)
                widthControlFrame += 30 + 10;
            int startX = g_WindowW_2 - (widthControlFrame + 2 * widthSeparation + 2 * widthAutopilot) / 2;
            g_CtrlPanel = vec4(startX, 2, startX + widthControlFrame + 2 * widthSeparation + 2 * widthAutopilot, 136);
            RenderEnv(startX, 2);
            int xFrame = startX + widthAutopilot + widthSeparation;
            RenderControlFrame(xFrame, 2, widthControlFrame, 136);
            int x = xFrame + 90;
            RenderCompass(x, 70, 58);
            x += 110;
            RenderThrottle(x, 10, 30, 100, 10);
            xChrono = x;
            x -= 45;
            if (g_Ship->ship.nPropeller == 2)
                x += (30 + 10) / 2;
            RenderRudder(x, 115, 120, 15);
            x += 150;
            if (g_Ship->ship.nPropeller == 2)
                x += (30 + 10) / 2;
            RenderTime(x, 13, 100, 30);
            RenderPitchRollRot(x, 35, 100, 95);
            RenderControlFrameNight(xFrame, 2, widthControlFrame, 136);
            RenderAutopilot(xFrame + widthControlFrame + widthSeparation, 2);
        }
        RenderChronometer(xChrono + 35.0f, 150.0f);
    }
}
void RenderShortcuts()
{
    if (!g_bShowShortcuts)
        return;

    // Split : from "W" to "F11" → left col
    size_t splitIndex = 23;

    float padding = 20.0f;
    float lineHeight = 22.0f;
    float boxWidth = 560.0f;
    float boxHeight = lineHeight * std::max(22, (int)(vShortcuts.size() - 22)) + padding * 2;

    // Center the box
    float x = (g_WindowW - boxWidth) / 2.0f;
    float y = (g_WindowH - boxHeight) / 2.0f;

    // Semi-transparent background
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x, y, boxWidth, boxHeight);
    nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 160));
    nvgFill(g_Nvg);

    // Font
    nvgFontSize(g_Nvg, 18.0f);
    nvgFontFace(g_Nvg, "arial");
    nvgFillColor(g_Nvg, nvgRGBA(255, 255, 255, 255));

    // Width of a column
    float columnWidth = (boxWidth - 2 * padding) * 0.5f;

    // Column positions
    float colLeftXKey = x + padding;
    float colLeftXDesc = x + padding + columnWidth - 20;
    float colRightXKey = x + padding + columnWidth + 20;
    float colRightXDesc = x + boxWidth - padding;

    // Central dividing line
    float separatorX = x + padding + columnWidth;
    nvgBeginPath(g_Nvg);
    nvgMoveTo(g_Nvg, separatorX, y + padding);
    nvgLineTo(g_Nvg, separatorX, y + boxHeight - padding);
    nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 180));
    nvgStrokeWidth(g_Nvg, 2.0f);
    nvgStroke(g_Nvg);

    // Left column
    float textYLeft = y + padding + lineHeight * 0.8f;
    for (size_t i = 0; i <= splitIndex && i < vShortcuts.size(); ++i)
    {
        nvgTextAlign(g_Nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(g_Nvg, colLeftXKey, textYLeft, vShortcuts[i].first.c_str(), NULL);

        nvgTextAlign(g_Nvg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        nvgText(g_Nvg, colLeftXDesc, textYLeft, vShortcuts[i].second.c_str(), NULL);

        textYLeft += lineHeight;
    }

    // Right column
    float textYRight = y + padding + lineHeight * 0.8f;
    for (size_t i = splitIndex + 1; i < vShortcuts.size(); ++i)
    {
        nvgTextAlign(g_Nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(g_Nvg, colRightXKey, textYRight, vShortcuts[i].first.c_str(), NULL);

        nvgTextAlign(g_Nvg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        nvgText(g_Nvg, colRightXDesc, textYRight, vShortcuts[i].second.c_str(), NULL);

        textYRight += lineHeight;
    }
}
void RenderCompassOfBinoculars()
{
    float aspect = float(g_WindowW) / float(g_WindowH);
    float fovY_rad = glm::radians(45.0f);
    float fovX_rad = 2.0f * atanf(aspect * tanf(fovY_rad * 0.5f));
    float degVisibleHorizontal = glm::degrees(fovX_rad); // degrees visible horizontally

    float capCenter = g_Camera.GetNorthAngleDEG();  // central heading in degrees (0..359)
    float centerX = float(g_WindowW) * 0.5f;        // horizontal center
    float rulerY = float(g_WindowH) * 0.5f;         // height graduated line

    float visibleRatio = 0.7f;
    float halfVisiblePixels = (float(g_WindowW) * visibleRatio) * 0.5f;
    float leftBound = centerX - halfVisiblePixels;
    float rightBound = centerX + halfVisiblePixels;

    nvgFontFace(g_Nvg, "arial");

    // Horizontal main line limited according to visibleRatio
    nvgBeginPath(g_Nvg);
    nvgMoveTo(g_Nvg, leftBound, rulerY);
    nvgLineTo(g_Nvg, rightBound, rulerY);
    nvgStrokeColor(g_Nvg, nvgRGB(255, 255, 255));
    nvgStrokeWidth(g_Nvg, 2.0f);
    nvgStroke(g_Nvg);

    // Graduations
    int minDegree = int(capCenter - degVisibleHorizontal * 0.5f);
    int maxDegree = int(capCenter + degVisibleHorizontal * 0.5f);

    for (int deg = minDegree; deg <= maxDegree; ++deg) {
        int displayedDeg = deg;
        if (displayedDeg < 0) displayedDeg += 360;
        if (displayedDeg >= 360) displayedDeg -= 360;

        float i = float(deg) - capCenter;
        float x = centerX + (i * float(g_WindowW) / degVisibleHorizontal);

        if (x < leftBound || x > rightBound)
            continue;

        bool isTenDegree = (displayedDeg % 10 == 0);
        float gradHeight = isTenDegree ? 22.f : 12.f;

        nvgBeginPath(g_Nvg);
        nvgMoveTo(g_Nvg, x, rulerY - gradHeight * 0.5f);
        nvgLineTo(g_Nvg, x, rulerY + gradHeight * 0.5f);
        nvgStrokeColor(g_Nvg, nvgRGB(200, 200, 220));
        nvgStrokeWidth(g_Nvg, 1.0f);
        nvgStroke(g_Nvg);

        if (isTenDegree) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%03d", displayedDeg);
            nvgFontSize(g_Nvg, 22.0f);
            nvgFillColor(g_Nvg, nvgRGB(220, 220, 255));
            nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
            nvgText(g_Nvg, x, rulerY + gradHeight * 0.5f + 4.f, buf, NULL);
        }
    }

    // Heading above the ruler, in the center
    char capBuf[8];
    snprintf(capBuf, sizeof(capBuf), "%03.1f°", capCenter);
    nvgFontSize(g_Nvg, 28.0f);
    nvgFillColor(g_Nvg, nvgRGB(255, 255, 255));
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
    nvgText(g_Nvg, centerX, rulerY - 24.f, capBuf, NULL);

    // Extended vertical line reinforcement (above the text and below the horizontal line)
    float capFontHeight = 28.f;  // font size used for the heading
    float capPaddingTop = 4.f;   // space between text and vertical line
    float capLineTop = rulerY - capFontHeight + capPaddingTop; // above the text heading
    float capLineBottom = rulerY + 200.f;  // a little below the horizontal line

    nvgBeginPath(g_Nvg);
    nvgMoveTo(g_Nvg, centerX, capLineTop);
    nvgLineTo(g_Nvg, centerX, capLineBottom);
    nvgStrokeColor(g_Nvg, nvgRGB(255, 255, 255));
    nvgStrokeWidth(g_Nvg, 2.0f);
    nvgStroke(g_Nvg);
    nvgBeginPath(g_Nvg);
    capLineTop = rulerY - 200.f;
    capLineBottom = rulerY - 24.0f - 28.0f - capPaddingTop;
    nvgMoveTo(g_Nvg, centerX, capLineTop);
    nvgLineTo(g_Nvg, centerX, capLineBottom);
    nvgStroke(g_Nvg);
}
void RenderSplashScreen(string subtitle)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_WindowW, g_WindowH);
    glClearColor(1.0/255.0, 53.0/255.0, 75.0/255.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    nvgBeginFrame(g_Nvg, g_WindowW, g_WindowH, g_DevicePixelRatio);
    {
        RenderTitle(g_WindowW_2, g_WindowH_2, 72.0f, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        if (!subtitle.empty())
        {
            nvgFontSize(g_Nvg, 20.0f);
            nvgFontFace(g_Nvg, "arial");
            nvgFillColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
            nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
            nvgText(g_Nvg, g_WindowW_2, g_WindowH_2 + 100.0f, subtitle.c_str(), nullptr);
		}
    }
    nvgEndFrame(g_Nvg);

    glfwSwapBuffers(g_hWindow);
}
void Render()
{
    static bool bInit = false;
    static float firstTime = g_TimerShipMotion.getTime();
    if (g_bReset)
    {
        // New ship: we reset the physical tempo
        bInit = false;
        firstTime = g_TimerShipMotion.getTime();
        g_Ship->bMotion = false;            // disable physics during latency
        g_bReset = false;
    }

    // Physics cutoff for 1 second after any change
    if (!bInit && (g_TimerShipMotion.getTime() > firstTime + 1.0f)) 
    {
        g_Ship->bMotion = true;
        bInit = true;
    }

    RenderImGui();

    bool bAboveWater = g_Camera.GetPosition().y > 0.0f;

    // Update the shadow map of the ship (used by the ship and the ocean)
    if (g_bShipShadow)
        g_Ship->RenderShadow(g_Camera, g_Sky.get());

    // Rendering the scene inverted for the reflection texture (used for the ocean)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, FBO_REFLECTION);

        glViewport(0, 0, g_WindowW, g_WindowH);
        glEnable(GL_CLIP_DISTANCE0);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (g_Ship)
            g_Ship->RenderReflexion(g_Camera, g_Sky.get());

        // Revert to default
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_CLIP_DISTANCE0);
    }
  
    // Update the sky (background + clouds)
    if (g_bSkyVisible)
    {
        // Update the sky texture (atmosphere)
        g_Sky->Render(g_Camera);
        
        // Update the clouds texture (with the atmosphere incorporated)
        glDisable(GL_DEPTH_TEST);
        g_Clouds->Render(g_Camera, g_Sky.get(), g_Wind);
        glEnable(GL_DEPTH_TEST);
    }

    // Main rendering to the msFBO_SCENE
    {
        if (!g_bWireframe)  glBindFramebuffer(GL_FRAMEBUFFER, msFBO_SCENE);
        else                glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, g_WindowW, g_WindowH);
        glPolygonMode(GL_FRONT_AND_BACK, g_bWireframe ? GL_LINE : GL_FILL);

        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the sky
        if (!g_bWireframe && g_bSkyVisible)
        {
            g_ShaderBackground->use();
            g_ShaderBackground->setSampler2D("uTexture", g_Clouds->GetTexture(), 0);
            g_ScreenQuadCloud->Render();
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Render all the other objects
        RenderTerrain();
        RenderTraffic();
        RenderMarks();
        RenderAxis();
        RenderBalls();
        RenderCentralGridColored();
        RenderGridsOfPatchs();
        RenderArrowWind();

        if (bAboveWater)
        {
            // To see the ocean through the windows of the bridge
            RenderOcean();
            RenderShip();   // The windows of the ship are the last rendered
        }
        else
        {
            // Normal view or view through the ocean (the part of the ship that is under the waterline)
            RenderShip();
            RenderOcean();
        }
        
        g_Lighthouses->RenderBeamLights(g_Camera, g_Ship->bLights);

        // Particles and billboards
        if (bAboveWater)
        {
            g_Lighthouses->RenderLights(g_Camera, g_Ship->bLights);
            g_Markup->RenderLights(g_Camera, g_Ship->bLights);
			g_Traffics->RenderLights(g_Camera, g_Ship->bLights);
			g_Ship->RenderNavLights(g_Camera);
            g_Ship->RenderSmoke(g_Camera, g_Sky.get());
            g_Ship->RenderSpray(g_Camera, g_Sky.get());
            g_Ship->RenderWakeVao(g_Camera);
        }

        // Polyline
        g_Ship->RenderContour(g_Camera);

        // Revert to default
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Post processing (mist + fog + underwater)
    if (!g_bWireframe)
    {
        // Copies the entire window from the source framebuffer (msFBO_SCENE) to the destination framebuffer (FBO_SCENE), transferring the color and depth buffers, pixel by pixel (GL_NEAREST)
        
        // Résoudre couleur principale
        glBindFramebuffer(GL_READ_FRAMEBUFFER, msFBO_SCENE);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO_SCENE);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glBlitFramebuffer(0, 0, g_WindowW, g_WindowH, 0, 0, g_WindowW, g_WindowH, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // Résoudre profondeur (idem, si besoin)
        glBindFramebuffer(GL_READ_FRAMEBUFFER, msFBO_SCENE);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO_SCENE);
        glBlitFramebuffer(0, 0, g_WindowW, g_WindowH, 0, 0, g_WindowW, g_WindowH, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        // Do the 1st post processing
        if ((g_Sky->bRain || g_bBinoculars) && g_Camera.GetPosition().y > 0.0f)
            glBindFramebuffer(GL_FRAMEBUFFER, FBO_POST);
        else
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, g_WindowW, g_WindowH);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Mist, Fog, Low intensity, Night vision

        g_ShaderPostProcessing->use();  // Misc/post_processing.vert Misc/post_processing.frag 
        g_ShaderPostProcessing->setSampler2D("texColor", TexSceneColor, 0);     // Get the previous color texture (from the scene)
        g_ShaderPostProcessing->setSampler2D("texDepth", TexSceneDepth, 1);
        g_ShaderPostProcessing->setFloat("exposure", g_Sky->Exposure);
        g_ShaderPostProcessing->setFloat("near", 0.1f);
        g_ShaderPostProcessing->setFloat("far", 30000.f);
        g_ShaderPostProcessing->setFloat("horizonHeight", g_Camera.GetHorizonViewportY());
        g_ShaderPostProcessing->setVec3("eyePos", g_Camera.GetPosition());      // For underwater effect
        g_ShaderPostProcessing->setVec3("oceanColor", g_Ocean->OceanColor);
        g_ShaderPostProcessing->setVec3("fogColor", g_Sky->FogColor);
        g_ShaderPostProcessing->setFloat("mistDensity", g_Sky->MistDensity);
        g_ShaderPostProcessing->setFloat("fogDensity", g_Sky->FogDensity);
        g_ShaderPostProcessing->setFloat("uTime", g_TimerShipMotion.getTime()); // For underwater particles
        g_ShaderPostProcessing->setVec2("screenSize", vec2(g_WindowW, g_WindowH));
        g_ShaderPostProcessing->setBool("bLowIntensity", g_bLowIntensity && bAboveWater);
        g_ShaderPostProcessing->setBool("bNightVision", g_bNightVision && bAboveWater);
        g_ScreenQuadPost->Render();

        // Rain, Binoculars

        if ((g_Sky->bRain || g_bBinoculars) && bAboveWater)
        {
            // 2 passes: 1 for the rain And or the binoculars, 2 for the fxaa
            
            // Rendering to a FBO for the 1st pass
            glBindFramebuffer(GL_FRAMEBUFFER, FBO_RAIN);
            glViewport(0, 0, g_WindowW, g_WindowH);
            glClearColor(0.0, 0.0, 0.0, 0.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            g_ShaderRain->use();    // Sky/rain.vert Sky/rain.frag
            g_ShaderRain->setSampler2D("texColor", TexPostColor, 0);    // Get the previous color texture (post processing)
            g_ShaderRain->setFloat("uTime", g_TimerShipMotion.getTime());           
            g_ShaderRain->setVec2("screenSize", vec2(g_WindowW, g_WindowH));
            g_ShaderRain->setBool("bBinoculars", g_bBinoculars);
            g_ShaderRain->setBool("bRainDropsTrails", g_Sky->bRainDropsTrails);
            g_ShaderRain->setBool("bRainBlurDrips", g_Sky->bRainBlurDrips);
            g_ScreenQuadPost->Render();
            
            // Rendering to the screen for the 2nd pass (FXAA)
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, g_WindowW, g_WindowH);
            glClearColor(0.0, 0.0, 0.0, 0.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            g_ShaderFXAA->use();    // Misc/fxaa.vert Misc/fxaa.frag
            g_ShaderFXAA->setSampler2D("texInput", TexRainColor, 0);    // Get the previous color texture (rain)
            g_ShaderFXAA->setVec2("invScreenSize", vec2(1.0f / g_WindowW, 1.0f / g_WindowH));
            g_ScreenQuadPost->Render();
        }
    }

    // Revert to default
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Nanovg
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    nvgBeginFrame(g_Nvg, g_WindowW, g_WindowH, g_DevicePixelRatio);
    {
        RenderStatusbar();
        RenderOceanCut(g_WindowW_2, 200, 800, 50, 0);
        RenderTitle(g_WindowW - 70, g_WindowH - 25, 36.0f, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        RenderInfo();
        RenderDashboard();
        RenderShortcuts();
        if (g_bBinoculars)
            RenderCompassOfBinoculars();
    }
    nvgEndFrame(g_Nvg);

    // ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Debugging for the textures (Displacement, Gradient, Foam buffer, Wake)
    RenderTextures();
}

