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
    static uint32_t oldWindowW;
    static uint32_t oldWindowH;

    if (!g_IsFullScreen)    // -> Switch to fullscreen
    {
        oldWindowW = g_WindowW;
        oldWindowH = g_WindowH;
        
        GLFWmonitor* targetMonitor = get_current_monitor(g_hWindow);
        const GLFWvidmode* mode = glfwGetVideoMode(targetMonitor);
        glfwSetWindowMonitor(g_hWindow, targetMonitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    else
    {
        // Restore old state
        glfwSetWindowMonitor(g_hWindow, NULL, g_WindowX, g_WindowY, oldWindowW, oldWindowH, 0);
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
    g_Clouds = make_unique<VolumetricClouds>(g_WindowW, g_WindowH);
    g_Sky.release();
    g_Sky = make_unique<Sky>(g_InitialPosition, g_WindowW, g_WindowH);
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
    if (IsInRect(g_CtrlThrottle, mouse))
    {
        if (yoffset > 0)
        {
            g_Ship->PowerCurrentStep++;
            if (g_Ship->PowerCurrentStep > g_Ship->ship.PowerStepMax)
                g_Ship->PowerCurrentStep = g_Ship->ship.PowerStepMax;
        }
        else
        {
            g_Ship->PowerCurrentStep--;
            if (g_Ship->PowerCurrentStep < -g_Ship->ship.PowerStepMax)
                g_Ship->PowerCurrentStep = -g_Ship->ship.PowerStepMax;
        }
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
    }
    else if (IsInRect(g_CtrlWind, mouse))
    {
        if (yoffset < 0)
            g_WindSpeedKN--;
        else
            g_WindSpeedKN++;
        g_WindSpeedKN = glm::clamp(g_WindSpeedKN, 1.0f, 30.0f);
        g_Wind = WindDirSpeed_Vec(g_WindDirectionDEG, g_WindSpeedKN);
        g_Ocean->GetWind(g_Wind);
        g_Ocean->InitFrequencies();
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
        if (IsInRect(g_CtrlAutopilotDynAdjust, mouse))
        {
            g_Ship->bDynamicAdjustment = !g_Ship->bDynamicAdjustment;
            return;
        }
        if (IsInRect(g_CtrlThrottle, mouse))
        {
            float valeur = 1 - (mouse.y - g_CtrlThrottleHigh) / (g_CtrlThrottleLow - g_CtrlThrottleHigh);
            g_Ship->PowerCurrentStep = (valeur - 0.5f) * (2.0f * g_Ship->ship.PowerStepMax);
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
            if(g_bPause)g_Timer.stop();
            else        g_Timer.start();
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
            g_Ship->SurgeVelocity--;
            break;
        case GLFW_KEY_KP_ADD:
            g_Ship->SurgeVelocity++;
            break;
        case GLFW_KEY_KP_MULTIPLY:
            g_Ship->SurgeVelocity = KnotsToMS(g_Ship->ship.SpeedMaxKn);
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
        // Power
        case GLFW_KEY_KP_8:
            g_Ship->PowerCurrentStep++;
            if (g_Ship->PowerCurrentStep > g_Ship->ship.PowerStepMax)
                g_Ship->PowerCurrentStep = g_Ship->ship.PowerStepMax;
            break;
        case GLFW_KEY_KP_5:
            g_Ship->PowerCurrentStep = 0;
            break;
        case GLFW_KEY_KP_2:
            g_Ship->PowerCurrentStep--;
            if (g_Ship->PowerCurrentStep <  -g_Ship->ship.PowerStepMax)
                g_Ship->PowerCurrentStep = -g_Ship->ship.PowerStepMax;
            break;
        // Bow Thruster
        case GLFW_KEY_DELETE:
            if (g_Ship->ship.HasBowThruster)
            {
                g_Ship->BowThrusterCurrentStep--;
                if (g_Ship->BowThrusterCurrentStep < -g_Ship->ship.BowThrusterStepMax)
                    g_Ship->BowThrusterCurrentStep = -g_Ship->ship.BowThrusterStepMax;
            }
            break;
		case GLFW_KEY_END:
            if (g_Ship->ship.HasBowThruster)
            {
                g_Ship->BowThrusterCurrentStep = 0;
            }
			break;
        case GLFW_KEY_PAGE_DOWN:
            if (g_Ship->ship.HasBowThruster)
            {
                g_Ship->BowThrusterCurrentStep++;
                if (g_Ship->BowThrusterCurrentStep > g_Ship->ship.BowThrusterStepMax)
                    g_Ship->BowThrusterCurrentStep = g_Ship->ship.BowThrusterStepMax;
            }
            break;
        // Windows
        case GLFW_KEY_F1:
            g_bShowSceneWindow = !g_bShowSceneWindow;
            break;
        case GLFW_KEY_F2:
            g_bShowShipWindow = !g_bShowShipWindow;
            break;
        case GLFW_KEY_F3:
            g_bShowStatusBar = !g_bShowStatusBar;
            break;
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
    // Ignore les notifications non critiques
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;

    // Ouvre le fichier en mode ajout (append)
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
#define CONSOLE
#endif
#define RECORD

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
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    g_WindowX = (mode->width - g_WindowW) / 2;
    g_WindowY = (mode->height - g_WindowH) / 2;
    glfwSetWindowPos(g_hWindow, g_WindowX, g_WindowY);

    glfwMakeContextCurrent(g_hWindow);

    // Initializing GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return -1;  // GLAD initialization failed

    // Initializing extensions
    if (!gladLoadGL())
        return -1;  // Unable to load OpenGL extensions

#if 0
    GLint profile;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
    if (profile & GL_CONTEXT_CORE_PROFILE_BIT)                  cout << "Profil Core" << endl;
    else if (profile & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)    cout << "Profil Compatibility" << endl;
    else                                                        cout << "Profil Not Specified" << endl;
#endif

    glEnable(GL_MULTISAMPLE);

    g_hWnd = glfwGetWin32Window(g_hWindow);
    g_DirExecutable = GetExecutablePath();

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

    //GetLimitsGPU();

    InitImGUI();
    InitNanoVg();
    InitFPSCounter();
    InitScene();
    //DisplayWaveParametersFromModels(KnotsToMS(g_WindSpeedKN), 50000.0);

    // Main rendering loop
    while (!glfwWindowShouldClose(g_hWindow))
    {
        // Camera zoom
        static float fov = g_Camera.GetZoom();
        if (g_bBinoculars && g_Camera.GetPosition().y > 0.0f)
            g_Camera.SetZoom(15.0f);
        else
            g_Camera.SetZoom(fov);

        // Camera update
        if (g_vShips.size() && g_NoShip >= 0 && g_NoShip < g_vShips.size())
        {
            vec3 p = g_Ship->TransformPosition(g_vShips[g_NoShip].View1);
            if (g_Camera.GetMode() == eCameraMode::OUT_FREE)
                p = g_Ship->TransformPosition(g_vShips[g_NoShip].View2);
            vec3 t = g_Ship->TransformPosition(vec3(50.0f, p.y, 0.0f));
            vec3 orbitalTarget = g_Ship->ship.Position + vec3(0.0f, 1.5f, 0.0f);
            g_Camera.Animate(g_TimeSpeed * g_Timer.getTime(), orbitalTarget, p, t);
        }
        else
        {
            vec3 pos = vec3(0.0f);
            vec3 p = vec3(0.0f);
            vec3 t = vec3(0.0f);
            g_Camera.Animate(g_TimeSpeed * g_Timer.getTime(), pos, p, t);

        }

        // Updates
        if (g_Ocean)    g_Ocean->Update(g_TimeSpeed * g_Timer.getTime());
        if (g_Ship)     g_Ship->Update(g_TimeSpeed * g_Timer.getTime());

#ifdef RECORD
        static int counter = 0;
        //if(counter % 100 == 0)
            g_Ocean->GetRecordFromBuoy(vec2(0.0f, 0.0f), g_TimeSpeed * g_Timer.getTime());
        counter++;
#endif
        UpdateFPS();
        UpdateSounds();

        // Render all
        Render();

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
    // All positions
    LoadPositions();
    
    // Camera
    g_Camera.SetProjection(45.0f, g_WindowW, g_WindowH, 0.1f, 30000.f);
    g_Camera.LookAt(vec3(-12.0f, 21.f, 100.f), vec3(0.f, 0.f, 0.f));
    g_Camera.SetSpeeds(0.01f, 0.0025f);
    g_Camera.SetTarget(vec3(0.0f, 0.0f, 0.0f));
    g_Camera.SetMode(eCameraMode::ORBITAL);

    // Wind
    g_WindDirectionDEG = 270.0f;
    g_WindSpeedKN = 15.0f;
    g_Wind = WindDirSpeed_Vec(g_WindDirectionDEG, g_WindSpeedKN);
    
    // Sky & Clouds
    g_Sky = make_unique<Sky>(g_InitialPosition, g_WindowW, g_WindowH);
    g_Clouds = make_unique<VolumetricClouds>(g_WindowW, g_WindowH);

    // Shaders
    g_ShaderSun = make_unique<Shader>("Resources/Shaders/sun.vert", "Resources/Shaders/sun.frag");              // Light from sun
    g_ShaderCamera = make_unique<Shader>("Resources/Shaders/camera.vert", "Resources/Shaders/camera.frag");     // Light from camera
    g_ShaderBackground = make_unique<Shader>("Resources/Clouds/background.vert", "Resources/Clouds/background.frag");     // To draw the clouds in a separate process
    g_ShaderPostProcessing = make_unique<Shader>("Resources/Shaders/post_processing.vert", "Resources/Shaders/post_processing.frag");   // Mist + fog + underwater
    g_ShaderFXAA = make_unique<Shader>("Resources/Shaders/fxaa.vert", "Resources/Shaders/fxaa.frag");
    g_ShaderRain = make_unique<Shader>("Resources/Sky/rain.vert", "Resources/Sky/rain.frag");
    
    // Models
    LoadModels();

    // Oceans
    g_Ocean = make_unique<Ocean>(g_Wind, g_Sky.get());
    g_Ocean->bVisible = true;

    // Quad for texture display
    g_QuadTexture = make_unique<QuadTexture>();
    
    // Timer
    g_Timer.start();

    // Terrains
    LoadTerrains();

    // Markup
    g_Markup = make_unique<Markup>(L"Resources/Terrains/Houat/Markup-houat.xml");

    // Sounds
    LoadSounds();
    
    // Ship
    LoadShips();

    glEnable(GL_PROGRAM_POINT_SIZE);
    InitFBO();
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

    ////////////////////

    g_ScreenQuadPost = make_unique<ScreenQuad>();
    g_ScreenQuadCloud = make_unique<ScreenQuad>();
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

    int font = nvgCreateFont(g_Nvg, "arial", "c:/Windows/Fonts/Arial.ttf");
    if (font == -1)
        printf("Unable to load arial font.\n");   // Handle font loading error

    int fontId = nvgCreateFont(g_Nvg, "caveat", "Resources/Interface/Caveat.ttf");
    if (fontId == -1)  
        printf("Failed to load Caveat font.\n");
}
void InitFPSCounter()
{
    QueryPerformanceFrequency(&g_Frequency);
    QueryPerformanceCounter(&g_LastTime);
}
void LoadPositions()
{
    if (g_vPositions.size() == 0)
    {
        sPositions sList;
        sList.name = "Treac'h er Goured";
        sList.pos = vec2(-2.94097114, 47.38162231);
        sList.heading = -90.0f;// 305.0f;
        g_vPositions.push_back(sList);

        sList.name = "Port de Houat";
        sList.pos = vec2(-2.95672011, 47.39293289);
        sList.heading = 119.0f;
        g_vPositions.push_back(sList);

        sList.name = "Nord du port de Houat";
        sList.pos = vec2(-2.95201015, 47.39797974);
        sList.heading = 191.0f;
        g_vPositions.push_back(sList);

        sList.name = "Sud Béniguet";
        sList.pos = vec2(-3.01105905, 47.39477158);
        sList.heading = 34.0f;
        g_vPositions.push_back(sList);

        sList.name = "Tréach er Salus";
        sList.pos = vec2(-2.96275711, 47.38032913);
        sList.heading = 288.0f;
        g_vPositions.push_back(sList);

        sList.name = "La Teignouse";
        sList.pos = vec2(-3.03526974, 47.45735550);
        sList.heading = 150.0f;
        g_vPositions.push_back(sList);
    }
}
void LoadModels()
{
    // Floating ball
    g_FloatingBall = make_unique<Sphere>(0.5f, 32); // Radius of 0.5 m (diameter of 1 m)
    g_FloatingBall->bVisible = false;
    
    // Grid XZ
    g_Grid = make_unique<Grid>(10, 1);
    g_Grid->bVisible = true;
    g_bGridVisible = false;

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
    }
}
void LoadTerrains()
{
    if (g_vTerrains.size() == 0)
    {
        vector<string> files = ListFiles("Resources/Terrains/Houat/", ".obj");
        for (auto& file : files)
		{
			sTerrain terrain;
			terrain.file = file;
			ReadObjHeader(terrain);
			vec2 p = LonLatToOpenGL(terrain.center.x, terrain.center.y);
			terrain.pos = vec3(p.x, 0.0f, p.y);
			terrain.scale = vec3(1.0f);
			terrain.model = make_unique<Model>(terrain.file);
			g_vTerrains.push_back(move(terrain));
		}
    }
    g_Pier = make_unique<Model>("Resources/Terrains/Houat/pier.gltf");
}
void LoadShips()
{
    if(g_vShips.size() == 0)
    {
        sShip ship;
#pragma region Support Vessel
        // Names
        ship.ShortName = "Support vessel";
        ship.PathnameHull = "Resources/Models/support_vessel/support_vessel_hull.obj";
        ship.PathnameFull = "Resources/Models/support_vessel/support_vessel.glb";
        ship.PathnamePropeller = "Resources/Models/support_vessel/support_vessel_propeller.glb";
        ship.PathnameRudder = "Resources/Models/support_vessel/support_vessel_rudder.glb";
        ship.PathnameRadar1 = "Resources/Models/support_vessel/support_vessel_radar1.glb";
        ship.PathnameRadar2 = "Resources/Models/support_vessel/support_vessel_radar2.glb";
        // Dimensions
        ship.Position = vec3(0.0f, 0.0f, 0.0f);
        ship.Rotation = vec3(0.0f);
        ship.View1 = vec3(12.74f, 9.7f, 0.0f);
        ship.View2 = vec3(-20.55f, 6.2f, -4.82f);
        ship.Length = 51.30f;
        ship.SpeedMaxKn = 15.8f;
        ship.Mass_t = 1100.0f;
        ship.PosGravity = vec3(0.4f, -2.5f, 0.0f);
        ship.HeavePerf = 1.0f;
        ship.EnvMapFactor = 0.1f;
        // Spray
        ship.SprayVerticalPerf = 10.0f;
        ship.SprayMultiplier = 5;
        ship.SprayLength = 0.15f;
        ship.SprayType = 1;
        // Rudders
        ship.PosRudder = vec3(-10.5f, -1.5f, 0.0f);
        ship.RudderIncrement = 1.0f;
        ship.RudderStepMax = 35;
        ship.RudderRotSpeed = 10.0f;
        ship.RudderLiftPerf = 5.0f;
        ship.RudderDragPerf = 10.0f;
        ship.TurningDragPerf = 1.0f;
        ship.CentrifugalPerf = 3.0f;
        ship.RudderPivotFwd = vec3(20.0f, 0.0f, 0.0f);
        ship.RudderPivotBwd = vec3(-20.0f, 0.0f, 0.0f);
        ship.nRudder = 2;
        ship.Rudder1 = vec3(-20.361f, -2.24f, -2.53f);
        ship.Rudder2 = vec3(-20.361f, -2.24f, 2.53f);
        // Power
        ship.PosPower = vec3(-9.5f, -1.6f, 0.0f);
        ship.PowerPerf = 0.15f;
        ship.PowerkW = 1000.0f;
        ship.PowerStepMax = 10;
        ship.PowerRpmMin = 0.0f;
        ship.PowerRpmMax = 300.0f;
        ship.PowerRpmIncrement = 30.f;
        ship.nChimney = 2;
        ship.Chimney1 = vec3(-4.46f, 11.237f, -5.63f);
        ship.Chimney2 = vec3(-4.46f, 11.237f, 5.63f);
        ship.nPropeller = 2;
        ship.Propeller1 = vec3(-17.724f, -3.40f, -2.5f);
        ship.Propeller2 = vec3(-17.724f, -3.40, 2.50f);
        ship.WakeWidth = 1.1f;
        ship.ThrustSound = "Resources/Sounds/Engine3.wav";
        // Bow thruster
        ship.HasBowThruster = true;
        ship.PosBowThruster = vec3(8.5f, -1.6f, 0.0f);
        ship.BowThrusterPerf = 0.6f;
        ship.BowThrusterPowerW = 100000.0f;
        ship.BowThrusterStepMax = 5;
        ship.BowThrusterRpmMin = 0.0f;
        ship.BowThrusterRpmMax = 500.0f;
        ship.BowThrusterRpmIncrement = 100.f;
        ship.BowThrusterSound = "Resources/Sounds/Engine0.wav";
        // Lights
        ship.LightPositions.clear();
        ship.LightColors.clear();
        ship.LightPositions.push_back(vec3(13.2f, 12.1f, -5.15f));  // Red
        ship.LightColors.push_back(vec3(1.0f, 0.0f, 0.0f));
        ship.LightPositions.push_back(vec3(13.2f, 12.1f, 5.15f));   // Green
        ship.LightColors.push_back(vec3(0.0f, 1.0f, 0.0f));
        ship.LightPositions.push_back(vec3(-22.4f, 5.0f, 0.0f));    // White stern
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        ship.LightPositions.push_back(vec3(5.25f, 19.5f, 0.0f));    // White high
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        // Radar
        ship.nRadar = 2;
        ship.Radar1 = vec3(6.4235f, 14.402f, 0.0f);
        ship.RotationRadar1 = 30.0f;
        ship.Radar2 = vec3(6.7807f, 12.726f, 0.0f);
        ship.RotationRadar2 = 30.0f;
        // Autopilot
        ship.BaseP = 2.0f;
        ship.BaseI = 0.3f;
        ship.BaseD = 20.0f;
        ship.MaxIntegral = 10.0f;	
        ship.SpeedFactor = 5.0f;	
        ship.MinSpeed = 1.0f;		
        ship.LowSpeedBoost = 2.0f;	
        ship.SeaSateFactor = 1.0f;	
        // Waves
        ship.CenterFore = 60.0f;
        g_vShips.push_back(ship);
#pragma endregion

#pragma region Grand Banks 43
        // Names
        ship.ShortName = "Grand Banks 43";
        ship.PathnameHull = "Resources/Models/grand_banks/grand_banks_hull.obj";
        ship.PathnameFull = "Resources/Models/grand_banks/grand_banks.gltf";
        ship.PathnamePropeller = "Resources/Models/grand_banks/grand_banks_propeller.gltf";
        ship.PathnameRudder = "Resources/Models/grand_banks/grand_banks_rudder.gltf";
        // Dimensions
        ship.Position = vec3(0.0f, 0.0f, 0.0f);
        ship.Rotation = vec3(0.0f);
        ship.View1 = vec3(0.16f, 5.0f, 0.98f);
        ship.View2 = vec3(-0.1f, 3.2f, 1.0f);
        ship.Length = 13.14f;
        ship.SpeedMaxKn = 20.8f;
        ship.Mass_t = 15.0f;
        ship.PosGravity = vec3(-0.7f, -0.5f, 0.0f);
        ship.HeavePerf = 3.0f;
        ship.EnvMapFactor = 0.2f;
        // Spray
        ship.SprayVerticalPerf = 3.0f;
        ship.SprayMultiplier = 5;
        ship.SprayLength = 0.15f;
        ship.SprayType = 0;
        // Rudders
        ship.PosRudder = vec3(-6.0f, -0.7f, 0.0f);
        ship.RudderIncrement = 1.0f;
        ship.RudderStepMax = 45;
        ship.RudderRotSpeed = 15.0f;
        ship.RudderLiftPerf = 0.3f;
        ship.RudderDragPerf = 0.05f;
        ship.TurningDragPerf = 10.0f;
        ship.CentrifugalPerf = 1.0f;
        ship.RudderPivotFwd = vec3(5.0f, 0.0f, 0.0f);
        ship.RudderPivotBwd = vec3(-5.0f, 0.0f, 0.0f);
        ship.nRudder = 2;
        ship.Rudder1 = vec3(-6.2f, 0.3f, -0.5f);
        ship.Rudder2 = vec3(-6.2f, 0.3f, 0.5f);
        // Power
        ship.PosPower = vec3(-6.5f, -0.7f, 0.0f);
        ship.PowerPerf = 0.1f;
        ship.PowerkW = 67.1f;
        ship.PowerStepMax = 10;
        ship.PowerRpmMin = 0.0f;
        ship.PowerRpmMax = 2500.0f;
        ship.PowerRpmIncrement = 400.f;
        ship.nChimney = 2;
        ship.Chimney1 = vec3(-6.7f, 0.1f, -1.7f);
        ship.Chimney2 = vec3(-6.7f, 0.1f, 1.7);
        ship.nPropeller = 2;
        ship.Propeller1 = vec3(-5.7f, -0.45f, -0.5f);
        ship.Propeller2 = vec3(-5.7f, -0.45f, 0.5f);
        ship.WakeWidth = 1.1f;
        ship.ThrustSound = "Resources/Sounds/Engine2.wav";
        // Bow thruster
        ship.HasBowThruster = false;
        // Lights
        ship.LightPositions.clear();
        ship.LightColors.clear();
        ship.LightPositions.push_back(vec3(0.95f, 3.4f, -1.7f));    // Red
        ship.LightColors.push_back(vec3(1.0f, 0.0f, 0.0f));
        ship.LightPositions.push_back(vec3(0.95f, 3.4f, 1.7f));     // Green
        ship.LightColors.push_back(vec3(0.0f, 1.0f, 0.0f));
        ship.LightPositions.push_back(vec3(-6.1f, 1.5f, 0.0f));     // White stern
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        ship.LightPositions.push_back(vec3(0.0f, 6.0f, 0.0f));      // White high
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        // Radar
        ship.nRadar = 0;
        // Autopilot
        ship.BaseP = 0.5f;
        ship.BaseI = 0.1f;
        ship.BaseD = 1.0f;
        ship.MaxIntegral = 5.0f;	
        ship.SpeedFactor = 5.0f;	
        ship.MinSpeed = 1.0f;		
        ship.LowSpeedBoost = 2.0f;	
        ship.SeaSateFactor = 1.0f;	
        // Waves
        ship.CenterFore = 2.0f;
        g_vShips.push_back(ship);
#pragma endregion

#pragma region Doga
        // Names
        ship.ShortName = "Trawler Doga";
        ship.PathnameHull = "Resources/Models/doga/doga_hull_sharp6.obj";
        ship.PathnameFull = "Resources/Models/doga/doga.gltf";
        ship.PathnamePropeller = "Resources/Models/doga/doga_propeller.glb";
        ship.PathnameRudder = "Resources/Models/doga/doga_rudder.glb";
        ship.PathnameRadar1 = "Resources/Models/doga/doga_radar.glb";
        // Dimensions
        ship.Position = vec3(0.0f);
        ship.Rotation = vec3(0.0f);
        ship.View1 = vec3(6.58f, 3.3f, 0.0f);
        ship.View2 = vec3(-9.0f, 2.0f, 0.0f);
        ship.Length = 19.37f;
        ship.SpeedMaxKn = 15.0f;
        ship.Mass_t = 48.0f;
        ship.PosGravity = vec3(0.0f, -0.5f, 0.0f);
        ship.HeavePerf = 2.0f;
        ship.EnvMapFactor = 0.5f;
        // Spray
        ship.SprayVerticalPerf = 2.0f;
        ship.SprayMultiplier = 5;
        ship.SprayLength = 0.15f;
        ship.SprayType = 1;
        // Rudders
        ship.PosRudder = vec3(-9.0f, -0.6f, 0.0f);
        ship.RudderIncrement = 1.0f;
        ship.RudderStepMax = 45;
        ship.RudderRotSpeed = 10.0f;
        ship.RudderLiftPerf = 0.7f;
        ship.RudderDragPerf = 4.0f;
        ship.TurningDragPerf = 2.0f;
        ship.CentrifugalPerf = 2.0f;
        ship.RudderPivotFwd = vec3(8.0f, 0.0f, 0.0f);
        ship.RudderPivotBwd = vec3(-8.0f, 0.0f, 0.0f);
        ship.nRudder = 2;
        ship.Rudder1 = vec3(-9.0f, -0.2f, -0.75f);
        ship.Rudder2 = vec3(-9.0f, -0.2f, 0.75f);
        // Power
        ship.PosPower = vec3(-8.2f, -0.75f, 0.0f);
        ship.PowerPerf = 0.051f; 
        ship.PowerkW = 150.0f;
        ship.PowerStepMax = 10;
        ship.PowerRpmMin = 0.0f;
        ship.PowerRpmMax = 400.0f;
        ship.PowerRpmIncrement = 50.0f;
        ship.nChimney = 2;
        ship.Chimney1 = vec3(2.37f, 3.7f, -1.73f); 
        ship.Chimney2 = vec3(2.37f, 3.7f, 1.73f);
        ship.nPropeller = 2;
        ship.Propeller1 = vec3(-8.5f, -0.8f, -0.75f);
        ship.Propeller2 = vec3(-8.5f, -0.8f, 0.75f);
        ship.WakeWidth = 1.1f;
        ship.ThrustSound = "Resources/Sounds/Engine7.wav";
        // Bow thruster
        ship.HasBowThruster = false;
        // Lights
        ship.LightPositions.clear();
        ship.LightColors.clear();
        ship.LightPositions.push_back(vec3(7.06f, 3.69f, -1.41f));  // Red
        ship.LightColors.push_back(vec3(1.0f, 0.0f, 0.0f));
        ship.LightPositions.push_back(vec3(7.06f, 3.69f, 1.41f));   // Green
        ship.LightColors.push_back(vec3(0.0f, 1.0f, 0.0f));
        ship.LightPositions.push_back(vec3(-9.54f, 1.53f, 0.0f));   // White stern
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        ship.LightPositions.push_back(vec3(4.99f, 8.05f, 0.0f));    // White high
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        // Radar
        ship.nRadar = 1;
        ship.Radar1 = vec3(5.4655f, 4.9726f, 0.0f);
        ship.RotationRadar1 = 20.0f;
        // Autopilot
        ship.BaseP = 0.5f;
        ship.BaseI = 0.1f;
        ship.BaseD = 1.0f;
        ship.MaxIntegral = 2.0f;	
        ship.SpeedFactor = 3.0f;	
        ship.MinSpeed = 1.0f;		
        ship.LowSpeedBoost = 2.0f;	
        ship.SeaSateFactor = 1.0f;	
        // Waves
        ship.CenterFore = 35.0f;
        g_vShips.push_back(ship);
#pragma endregion

#pragma region Boga
        // Names
        ship.ShortName = "Trawler Boga";
        ship.PathnameHull = "Resources/Models/boga/boga_hull.obj";
        ship.PathnameFull = "Resources/Models/boga/boga.gltf";
        ship.PathnamePropeller = "Resources/Models/boga/boga_propeller.glb";
        ship.PathnameRudder = "Resources/Models/boga/boga_rudder.glb";
        ship.PathnameRadar1 = "Resources/Models/boga/boga_radar.glb";
        // Dimensions
        ship.Position = vec3(0.0f);
        ship.Rotation = vec3(0.0f);
        ship.View1 = vec3(6.8f, 4.2f, 0.0f);
        ship.View2 = vec3(-8.5f, 3.2f, 1.7f);
        ship.Length = 21.2f;
        ship.SpeedMaxKn = 15.0f;
        ship.Mass_t = 48.0f;
        ship.PosGravity = vec3(0.0f, -0.5f, 0.0f);
        ship.HeavePerf = 2.0f;
        ship.EnvMapFactor = 0.5f;
        // Spray
        ship.SprayVerticalPerf = 3.0f;
        ship.SprayMultiplier = 5;
        ship.SprayLength = 0.15f;
        ship.SprayType = 1;
        // Rudders
        ship.PosRudder = vec3(-9.0f, -0.6f, 0.0f);
        ship.RudderIncrement = 1.0f;
        ship.RudderStepMax = 45;
        ship.RudderRotSpeed = 10.0f;
        ship.RudderLiftPerf = 0.7f;
        ship.RudderDragPerf = 4.0f;
        ship.TurningDragPerf = 2.0f;
        ship.CentrifugalPerf = 2.0f;
        ship.RudderPivotFwd = vec3(8.0f, 0.0f, 0.0f);
        ship.RudderPivotBwd = vec3(-8.0f, 0.0f, 0.0f);
        ship.nRudder = 1;
        ship.Rudder1 = vec3(-8.5f, -0.1f, 0.0f);
        // Power
        ship.PosPower = vec3(-8.166f, -0.61f, 0.0f);
        ship.PowerPerf = 0.07f;  
        ship.PowerkW = 150.0f;
        ship.PowerStepMax = 10;
        ship.PowerRpmMin = 0.0f;
        ship.PowerRpmMax = 400.0f;
        ship.PowerRpmIncrement = 50.0f;
        ship.nChimney = 2;
        ship.Chimney1 = vec3(4.05f, 5.7f, -0.11f);
        ship.Chimney2 = vec3(4.05f, 5.7f, 0.11f);
        ship.nPropeller = 1;
        ship.Propeller1 = vec3(-8.166f, -0.61f, 0.0f);
        ship.WakeWidth = 1.1f;
        ship.ThrustSound = "Resources/Sounds/Engine7.wav";
        // Bow thruster
        ship.HasBowThruster = false;
        // Lights
        ship.BowThrusterRpmIncrement = 100.f;
        ship.LightPositions.clear();
        ship.LightColors.clear();
        ship.LightPositions.push_back(vec3(6.9f, 4.8f, -1.475f));  // Red
        ship.LightColors.push_back(vec3(1.0f, 0.0f, 0.0f));
        ship.LightPositions.push_back(vec3(6.9f, 4.8f, 1.475f));   // Green
        ship.LightColors.push_back(vec3(0.0f, 1.0f, 0.0f));
        ship.LightPositions.push_back(vec3(-10.393f, 2.1f, 0.0f));  // White stern
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        ship.LightPositions.push_back(vec3(2.164f, 11.45f, 0.0f));  // White high
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        // Radar
        ship.nRadar = 1;
        ship.Radar1 = vec3(5.5682f, 5.4684f, 0.0f);
        ship.RotationRadar1 = 20.0f;
        // Autopilot
        ship.BaseP = 0.5f;
        ship.BaseI = 0.1f;
        ship.BaseD = 1.0f;
        ship.MaxIntegral = 2.0f;	
        ship.SpeedFactor = 3.0f;	
        ship.MinSpeed = 1.0f;		
        ship.LowSpeedBoost = 2.0f;	
        ship.SeaSateFactor = 1.0f;	
        // Waves
        ship.CenterFore = 40.0f;
        g_vShips.push_back(ship);
#pragma endregion

#pragma region Jang Bogo
        // Names
        ship.ShortName = "Submarine Jang Bogo";
        ship.PathnameHull = "Resources/Models/jang_bogo/jang_bogo_hull.obj";
        ship.PathnameFull = "Resources/Models/jang_bogo/jang_bogo.gltf";
        ship.PathnamePropeller = "Resources/Models/jang_bogo/jang_bogo_propeller.gltf";
        ship.PathnameRudder = "Resources/Models/jang_bogo/jang_bogo_rudder.gltf";
        // Dimensions
        ship.Position = vec3(0.0f, 0.0f, 0.0f);
        ship.Rotation = vec3(0.0f);
        ship.View1 = vec3(5.99f, 5.8613f, 0.0f);
        ship.View2 = vec3(5.99f, 6.6f, 0.0f);
        ship.Length = 53.98f;
        ship.SpeedMaxKn = 14.4f;
        ship.Mass_t = 1300.0f;
        ship.PosGravity = vec3(0.4f, -2.5f, 0.0f);
        ship.HeavePerf = 1.0f;
        ship.EnvMapFactor = 0.1f;
        // Spray
        ship.SprayVerticalPerf = 10.0f;
        ship.SprayMultiplier = 3;
        ship.SprayLength = 0.1f;
        ship.SprayType = 0;
        // Rudders
        ship.PosRudder = vec3(-24.022f, -2.4748f, 0.0f);
        ship.RudderIncrement = 1.0f;
        ship.RudderStepMax = 35;
        ship.RudderRotSpeed = 10.0f;
        ship.RudderLiftPerf = 5.0f;
        ship.RudderDragPerf = 1.0f;
        ship.TurningDragPerf = 1.0f;
        ship.CentrifugalPerf = 0.0f;
        ship.RudderPivotFwd = vec3(10.0f, 0.0f, 0.0f);
        ship.RudderPivotBwd = vec3(-10.0f, 0.0f, 0.0f);
        ship.nRudder = 1;
        ship.Rudder1 = vec3(-24.022f, -2.4748f, 0.0f);
        // Power
        ship.PosPower = vec3(-27.0f, -2.7118f, 0.0f);
        ship.PowerPerf = 0.03f;
        ship.PowerkW = 3700.0f;
        ship.PowerStepMax = 10;
        ship.PowerRpmMin = 0.0f;
        ship.PowerRpmMax = 250.0f;
        ship.PowerRpmIncrement = 30.f;
        ship.nChimney = 1;
        ship.Chimney1 = vec3(0.7117f, 10.127f, 0.0f);
        ship.nPropeller = 1;
        ship.Propeller1 = vec3(-26.346f, -2.7118f, 0.0f);
        ship.WakeWidth = 0.3f;
        ship.ThrustSound = "Resources/Sounds/Engine3.wav";
        // Bow thruster
        ship.HasBowThruster = false;
        // Lights
        ship.LightPositions.clear();
        ship.LightColors.clear();
        ship.LightPositions.push_back(vec3(0.0f));          // None
        ship.LightColors.push_back(vec3(0.0f, 0.0f, 0.0f));
        ship.LightPositions.push_back(vec3(0.0f));          // None
        ship.LightColors.push_back(vec3(0.0f, 0.0f, 0.0f));
        ship.LightPositions.push_back(vec3(0.0f));          // None
        ship.LightColors.push_back(vec3(0.0f, 0.0f, 0.0f));
        ship.LightPositions.push_back(vec3(3.2874f, 11.737f, 0.0f));    // Yellow high
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 0.0f));
        // Radar
        ship.nRadar = 0;
        // Autopilot
        ship.BaseP = 2.0f;
        ship.BaseI = 0.3f;
        ship.BaseD = 5.0f;
        ship.MaxIntegral = 3.0f;	
        ship.SpeedFactor = 3.0f;	
        ship.MinSpeed = 1.0f;		
        ship.LowSpeedBoost = 2.0f;	
        ship.SeaSateFactor = 1.0f;	
        // Waves
        ship.CenterFore = 0.0f;
        g_vShips.push_back(ship);
#pragma endregion

#pragma region Gun Ship
        // Names
        ship.ShortName = "Gun Ship";
        ship.PathnameHull = "Resources/Models/gun_ship/gun_ship_hull.obj";
        ship.PathnameFull = "Resources/Models/gun_ship/gun_ship.glb";
        ship.PathnamePropeller = "Resources/Models/gun_ship/gun_ship_propeller.glb";
        ship.PathnameRudder = "Resources/Models/gun_ship/gun_ship_rudder.glb";
        ship.PathnameRadar1 = "Resources/Models/gun_ship/gun_ship_radar1.glb";
        ship.PathnameRadar2 = "Resources/Models/gun_ship/gun_ship_radar2.glb";
        // Dimensions
        ship.Position = vec3(0.0f);
        ship.Rotation = vec3(0.0f);
        ship.View1 = vec3(4.9f, 8.0f, -1.0f);
        ship.View2 = vec3(-28.0f, 4.3f, 3.2f);
        ship.Length = 59.83f;
        ship.SpeedMaxKn = 19.2f;
        ship.Mass_t = 820.0f;
        ship.PosGravity = vec3(-2.4f, -1.9f, 0.0f);
        ship.HeavePerf = 1.0f;
        ship.EnvMapFactor = 0.1f;
        // Spray
        ship.SprayVerticalPerf = 5.0f;
        ship.SprayMultiplier = 5;
        ship.SprayLength = 0.15f;
        ship.SprayType = 0;
        // Rudders
        ship.PosRudder = vec3(-28.0f, -2.4f, 0.0f);
        ship.RudderIncrement = 1.0f;
        ship.RudderStepMax = 35;
        ship.RudderRotSpeed = 10.0f;
        ship.RudderLiftPerf = 1.5f;
        ship.RudderDragPerf = 6.0f;
        ship.TurningDragPerf = 2.0f;
        ship.CentrifugalPerf = 5.0f;
        ship.RudderPivotFwd = vec3(20.0f, 0.0f, 0.0f);
        ship.RudderPivotBwd = vec3(-20.0f, 0.0f, 0.0f);
        ship.nRudder = 2;
        ship.Rudder1 = vec3(-27.2f, -1.65f, -1.43f);
        ship.Rudder2 = vec3(-27.2f, -1.65f, 1.43f);
        // Power
        ship.PosPower = vec3(-25.0f, -2.4f, 0.0f);
        ship.PowerPerf = 0.1f;
        ship.PowerkW = 2000.0f;
        ship.PowerStepMax = 10;
        ship.PowerRpmMin = 0.0f;
        ship.PowerRpmMax = 500.0f;
        ship.PowerRpmIncrement = 50.f;
        ship.nChimney = 2;
        ship.Chimney1 = vec3(-11.994f, 5.3264f, -1.0294f);
        ship.Chimney2 = vec3(-11.994f, 5.3264f, 1.0294f);
        ship.nPropeller = 3;
        ship.Propeller1 = vec3(-25.3f, -2.45f, -2.3f);
        ship.Propeller2 = vec3(-25.3f, -2.45f, 2.3f);
        ship.Propeller3 = vec3(-25.3f, -2.75f, 0.0f);
        ship.WakeWidth = 1.2f;
        ship.ThrustSound = "Resources/Sounds/Engine6.wav";
        // Bow thruster
        ship.HasBowThruster = false;
        ship.PosBowThruster = vec3(23.0f, -2.0f, 0.0f);
        ship.BowThrusterPerf = 0.4f;
        ship.BowThrusterPowerW = 70000.0f;
        ship.BowThrusterStepMax = 5;
        ship.BowThrusterRpmMin = 0.0f;
        ship.BowThrusterRpmMax = 500.0f;
        ship.BowThrusterRpmMax = 500.0f;
        ship.BowThrusterRpmIncrement = 100.f;
        // Lights
        ship.LightPositions.clear();
        ship.LightColors.clear();
        ship.LightPositions.push_back(vec3(5.1f, 8.8f, -3.1f));     // Red
        ship.LightColors.push_back(vec3(1.0f, 0.0f, 0.0f));
        ship.LightPositions.push_back(vec3(5.1f, 8.8f, 3.1f));      // Green
        ship.LightColors.push_back(vec3(0.0f, 1.0f, 0.0f));
        ship.LightPositions.push_back(vec3(-29.15f, 2.3f, 0.0f));   // White stern
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        ship.LightPositions.push_back(vec3(-2.2f, 16.0f, 0.0f));    // White high
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        ship.LightPositions.push_back(vec3(30.4f, 5.5f, 0.0f));     // White low
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        // Radar
        ship.nRadar = 2;
        ship.Radar1 = vec3(-2.8894f, 16.185f, 0.005442f);
        ship.RotationRadar1 = 10.0f;
        ship.Radar2 = vec3(-2.9006f, 14.244f, -1.4877f);
        ship.RotationRadar2 = 20.0f;
        // Autopilot
        ship.BaseP = 1.0f;
        ship.BaseI = 0.3f;
        ship.BaseD = 5.0f;
        ship.MaxIntegral = 5.0f;	
        ship.SpeedFactor = 2.0f;	
        ship.MinSpeed = 1.0f;		
        ship.LowSpeedBoost = 2.0f;	
        ship.SeaSateFactor = 1.0f;	
        // Waves
        ship.CenterFore = 60.0f;
        g_vShips.push_back(ship);
#pragma endregion

#pragma region Cargo
        // Names
        ship.ShortName = "Cargo bulk carrier";
        ship.PathnameHull = "Resources/Models/cargo/cargo_sharp6.obj";
        ship.PathnameFull = "Resources/Models/cargo/cargo.gltf";
        ship.PathnamePropeller = "Resources/Models/cargo/cargo_propeller.glb";
        ship.PathnameRudder = "Resources/Models/cargo/cargo_rudder.glb";
        ship.PathnameRadar1 = "Resources/Models/cargo/cargo_radar.glb";
        // Dimensions
        ship.Position = vec3(0.0f);
        ship.Rotation = vec3(0.0f);
        ship.View1 = vec3(-30.5f, 9.5f, 0.0f);
        ship.View2 = vec3(-30.5f, 9.5f, -5.8f);
        ship.Length = 90.99f;
        ship.SpeedMaxKn = 15.5f;
        ship.Mass_t = 4000.0f;
        ship.PosGravity = vec3(-0.5f, -1.9f, 0.0f);
        ship.HeavePerf = 1.0f;
        ship.EnvMapFactor = 0.25f;
        // Spray
        ship.SprayVerticalPerf = 20.0f;
        ship.SprayMultiplier = 5;
        ship.SprayLength = 0.1f;
        ship.SprayType = 1;
        // Rudders
        ship.PosRudder = vec3(-44.0f, -2.4f, 0.0f);
        ship.RudderIncrement = 1.0f;
        ship.RudderStepMax = 35;
        ship.RudderRotSpeed = 10.0f;
        ship.RudderLiftPerf = 3.5f;
        ship.RudderDragPerf = 10.0f;
        ship.TurningDragPerf = 0.5f;
        ship.CentrifugalPerf = 15.0f;
        ship.RudderPivotFwd = vec3(30.0f, 0.0f, 0.0f);
        ship.RudderPivotBwd = vec3(-25.0f, 0.0f, 0.0f);
        ship.nRudder = 1;
        ship.Rudder1 = vec3(-43.5f, -0.5f,0.0f);
        // Power
        ship.PosPower = vec3(-42.5f, -2.8f, 0.0f);
        ship.PowerPerf = 0.12f;
        ship.PowerkW = 1600.0f;	
        ship.PowerStepMax = 10;
        ship.PowerRpmMin = 0.0f;
        ship.PowerRpmMax = 200.0f;
        ship.PowerRpmIncrement = 14.0f;
        ship.nChimney = 1;
        ship.Chimney1 = vec3(-40.8f, 12.0f, 0.0f); 
        ship.nPropeller = 1;
        ship.Propeller1 = vec3(-41.7f, -2.8f, 0.0f); 
        ship.WakeWidth = 0.7f;
        ship.ThrustSound = "Resources/Sounds/Engine10.wav";
        // Bow thruster
        ship.HasBowThruster = false;
        ship.PosBowThruster = vec3(32.0f, -4.0f, 0.0f);
        ship.BowThrusterPerf = 0.4f;
        ship.BowThrusterPowerW = 240000.0f;
        ship.BowThrusterStepMax = 5;
        ship.BowThrusterRpmMin = 0.0f;
        ship.BowThrusterRpmMax = 500.0f;
        ship.BowThrusterRpmMax = 500.0f;
        ship.BowThrusterRpmIncrement = 100.f;
        // Lights
        ship.LightPositions.clear();
        ship.LightColors.clear();
        ship.LightPositions.push_back(vec3(-30.0f, 8.6f, -7.0f));   // Red
        ship.LightColors.push_back(vec3(1.0f, 0.0f, 0.0f));
        ship.LightPositions.push_back(vec3(-30.0f, 8.6f, 7.0f));    // Green
        ship.LightColors.push_back(vec3(0.0f, 1.0f, 0.0f));
        ship.LightPositions.push_back(vec3(-45.5f, 5.5f, 0.0f));    // White stern
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        ship.LightPositions.push_back(vec3(-33.2f, 19.0f, 0.0f));   // White high
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        ship.LightPositions.push_back(vec3(35.7f, 11.9f, 0.0f));    // White low
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        // Radar
        ship.nRadar = 1;
        ship.Radar1 = vec3(-31.853f, 16.046f, 0.020625f);
        ship.RotationRadar1 = 20.0f;
        // Autopilot
        ship.BaseP = 2.0f;
        ship.BaseI = 0.3f;
        ship.BaseD = 20.0f;
        ship.MaxIntegral = 10.0f;	
        ship.SpeedFactor = 5.0f;	
        ship.MinSpeed = 1.0f;		
        ship.LowSpeedBoost = 2.0f;	
        ship.SeaSateFactor = 1.0f;	
        // Waves
        ship.CenterFore = 120.0f;
        g_vShips.push_back(ship);
#pragma endregion

#pragma region Frigate
        // Names
        ship.ShortName = "Frigate Type 054A";
        ship.PathnameHull = "Resources/Models/frigate/frigate_hull.obj";
        ship.PathnameFull = "Resources/Models/frigate/frigate.gltf";
        ship.PathnamePropeller = "Resources/Models/frigate/frigate_propeller.gltf";
        ship.PathnameRudder = "Resources/Models/frigate/frigate_rudder.gltf";
        ship.PathnameRadar1 = "Resources/Models/frigate/frigate_radar1.gltf";
        ship.PathnameRadar2 = "Resources/Models/frigate/frigate_radar2.gltf";
        // Dimensions
        ship.Position = vec3(0.0f);
        ship.Rotation = vec3(0.0f);
        ship.View1 = vec3(24.0f, 11.068f, 0.0f);
        ship.View2 = vec3(-59.561f, 6.3f, 0.0f);
        ship.Length = 134.59f;
        ship.SpeedMaxKn = 27.1f;
        ship.Mass_t = 3963.0f;
        ship.PosGravity = vec3(-8.0f, -2.3f, -0.18f);
        ship.HeavePerf = 1.0f;
        ship.EnvMapFactor = 0.25f;
        // Spray
        ship.SprayVerticalPerf = 20.0f;
        ship.SprayMultiplier = 2;
        ship.SprayLength = 0.1f;
        ship.SprayType = 0;
        // Rudders
        ship.PosRudder = vec3(-62.401f, -2.5f, 0.0f);
        ship.RudderIncrement = 1.0f;
        ship.RudderStepMax = 35;
        ship.RudderRotSpeed = 10.0f;
        ship.RudderLiftPerf = 2.0f;
        ship.RudderDragPerf = 5.0f;
        ship.TurningDragPerf = 0.5f;
        ship.CentrifugalPerf = 20.0f;
        ship.RudderPivotFwd = vec3(49.0f, 0.0f, 0.0f);
        ship.RudderPivotBwd = vec3(-44.0f, 0.0f, 0.0f);
        ship.nRudder = 2;
        ship.Rudder1 = vec3(-62.401f, -1.4961f, 2.7038f);
        ship.Rudder2 = vec3(-62.401f, -1.4961f, -2.7038f);
        // Power
        ship.PosPower = vec3(-59.0f, -3.56f, 0.0f);
        ship.PowerPerf = 0.065f;
        ship.PowerkW = 20000.0f;
        ship.PowerStepMax = 10;
        ship.PowerRpmMin = 0.0f;
        ship.PowerRpmMax = 200.0f;
        ship.PowerRpmIncrement = 20.0f;
        ship.nChimney = 2;
        ship.Chimney1 = vec3(-18.4f, 13.0f, 1.0f);     
        ship.Chimney2 = vec3(-18.4f, 13.0f, -1.0f);      
        ship.nPropeller = 2;
        ship.Propeller1 = vec3(-57.567f, -3.5635f, 2.8342f);
        ship.Propeller2 = vec3(-57.567f, -3.5635f, -2.8342f);
        ship.WakeWidth = 1.0f;
        ship.ThrustSound = "Resources/Sounds/Engine10.wav";
        // Bow thruster
        ship.HasBowThruster = false;
        // Lights
        ship.LightPositions.clear();
        ship.LightColors.clear();
        ship.LightPositions.push_back(vec3(21.994f, 9.629f, -7.1082f)); // Red
        ship.LightColors.push_back(vec3(1.0f, 0.0f, 0.0f));
        ship.LightPositions.push_back(vec3(21.994f, 9.629f, 7.1082f));  // Green
        ship.LightColors.push_back(vec3(0.0f, 1.0f, 0.0f));
        ship.LightPositions.push_back(vec3(-67.422f, 7.6993f, 0.0f));   // White stern
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        ship.LightPositions.push_back(vec3(1.8116f, 29.107f, 0.0f));    // White high
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        ship.LightPositions.push_back(vec3(66.73f, 11.436f, 0.0f));     // White low
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        // Radar
        ship.nRadar = 2;
        ship.Radar1 = vec3(6.281f, 24.284f, 0.0f);
        ship.Radar2 = vec3(9.2113f, 22.67f, 0.0f);
        ship.RotationRadar1 = 40.0f;
        ship.RotationRadar2 = 20.0f;
        // Autopilot
        ship.BaseP = 2.0f;
        ship.BaseI = 0.3f;
        ship.BaseD = 10.0f;
        ship.MaxIntegral = 5.0f;	
        ship.SpeedFactor = 50.0f;	
        ship.MinSpeed = 1.0f;		
        ship.LowSpeedBoost = 2.0f;	
        ship.SeaSateFactor = 1.0f;	
        // Waves
        ship.CenterFore = 60.0f;
        g_vShips.push_back(ship);
#pragma endregion

#pragma region Spyrosk
        // Names
        ship.ShortName = "Tanker Spyrosk";
        ship.PathnameHull = "Resources/Models/spyrosk/spyrosk_sharp7.obj";
        ship.PathnameFull = "Resources/Models/spyrosk/spyrosk.gltf";
        ship.PathnamePropeller = "Resources/Models/spyrosk/spyrosk_propeller.glb";
        ship.PathnameRudder = "Resources/Models/spyrosk/spyrosk_rudder.gltf";
        ship.PathnameRadar1 = "Resources/Models/spyrosk/spyrosk_radar.gltf";
        // Dimensions
        ship.Position = vec3(0.0f);
        ship.Rotation = vec3(0.0f);
        ship.View1 = vec3(-93.0f, 24.5f, 2.0f);
        ship.View2 = vec3(-91.3f, 24.5f, 22.5f);
        ship.Length = 273.09f;
        ship.SpeedMaxKn = 22.8f;
        ship.Mass_t = 130000.0f;
        ship.PosGravity = vec3(6.0f, -5.0f, 0.0f);
        ship.HeavePerf = 1.0f;
        ship.EnvMapFactor = 0.1f;
        // Spray
        ship.SprayVerticalPerf = 20.0f;
        ship.SprayMultiplier = 10;
        ship.SprayLength = 0.1f;
        ship.SprayType = 1;
        // Rudders
        ship.PosRudder = vec3(-133.0f, -4.15f, 0.0f);
        ship.RudderIncrement = 1.0f;
        ship.RudderStepMax = 35;
        ship.RudderRotSpeed = 2.8f;
        ship.RudderLiftPerf = 10.0f;
        ship.RudderDragPerf = 2.0f;
        ship.TurningDragPerf = 1.0f;
        ship.CentrifugalPerf = 30.0f;
        ship.RudderPivotFwd = vec3(100.0f, 0.0f, 0.0f);
        ship.RudderPivotBwd = vec3(-100.0f, 0.0f, 0.0f);
        ship.nRudder = 1;
        ship.Rudder1 = vec3(-131.6f, -0.025f, 0.0f);
        // Power
        ship.PosPower = vec3(-127.0f, -7.0f, 0.0f);
        ship.PowerPerf = 0.2f;
        ship.PowerkW = 18235.0f;
        ship.PowerStepMax = 10;
        ship.PowerRpmMin = 0.0f;
        ship.PowerRpmMax = 90.0f;
        ship.PowerRpmIncrement = 1.0f;
        ship.nChimney = 2;
        ship.Chimney1 = vec3(-118.0f, 31.1f, -1.4f); 
        ship.Chimney2 = vec3(-118.0f, 31.1f, 1.4f);
        ship.nPropeller = 1;
        ship.Propeller1 = vec3(-127.0f, -7.0f, 0.0f);
        ship.WakeWidth = 0.3f;
        ship.ThrustSound = "Resources/Sounds/Engine13.wav";
        // Bow thruster
        ship.HasBowThruster = false;
        ship.PosBowThruster = vec3(23.0f, -2.0f, 0.0f);
        ship.BowThrusterPerf = 0.4f;
        ship.BowThrusterPowerW = 0.0f;
        ship.BowThrusterStepMax = 5;
        ship.BowThrusterRpmMin = 0.0f;
        ship.BowThrusterRpmMax = 500.0f;
        ship.BowThrusterRpmMax = 500.0f;
        ship.BowThrusterRpmIncrement = 100.f;
        // Lights
        ship.LightPositions.clear();
        ship.LightColors.clear();
        ship.LightPositions.push_back(vec3(-94.0f, 22.89f, -24.56f));   // Red
        ship.LightColors.push_back(vec3(1.0f, 0.0f, 0.0f));
        ship.LightPositions.push_back(vec3(-94.0f, 22.89f, 24.56f));    // Green
        ship.LightColors.push_back(vec3(0.0f, 1.0f, 0.0f));
        ship.LightPositions.push_back(vec3(-136.0f, 11.0f, 0.0f));      // White stern
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        ship.LightPositions.push_back(vec3(-98.0f, 37.0f, -0.5f));      // White high
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        ship.LightPositions.push_back(vec3(124.5f, 26.0f, 0.0f));       // White low
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        // Radar
        ship.nRadar = 1;
        ship.Radar1 = vec3(-97.269f, 33.512f, -0.5268f);
        ship.RotationRadar1 = 10.0f;
        // Autopilot
        ship.BaseP = 2.0f;
        ship.BaseI = 0.3f;
        ship.BaseD = 20.0f;
        ship.MaxIntegral = 10.0f;	
        ship.SpeedFactor = 5.0f;	
        ship.MinSpeed = 1.0f;		
        ship.LowSpeedBoost = 2.0f;
        ship.SeaSateFactor = 1.0f;
        // Waves
        ship.CenterFore = 170.0f;
        g_vShips.push_back(ship);
#pragma endregion

#pragma region Gas Carrier
        // Names
        ship.ShortName = "Gas Carrier";
        ship.PathnameHull = "Resources/Models/gas_carrier/gas_carrier_sharp7.obj";
        ship.PathnameFull = "Resources/Models/gas_carrier/gas_carrier.gltf";
        ship.PathnamePropeller = "Resources/Models/gas_carrier/gas_carrier_propeller.gltf";
        ship.PathnameRudder = "Resources/Models/gas_carrier/gas_carrier_rudder.gltf";
        ship.PathnameRadar1 = "Resources/Models/gas_carrier/gas_carrier_radar.gltf";
        ship.PathnameRadar2 = "Resources/Models/gas_carrier/gas_carrier_radar.gltf";
        // Dimensions
        ship.Position = vec3(0.0f);
        ship.Rotation = vec3(0.0f);
        ship.View1 = vec3(-69.83f, 45.85f, -2.3f);
        ship.View2 = vec3(-70.5f, 44.7f, -25.857f);
        ship.Length = 273.09f;
        ship.SpeedMaxKn = 22.6f;
        ship.Mass_t = 160000.0f;
        ship.PosGravity = vec3(10.0f, -6.0f, 0.0f);
        ship.HeavePerf = 1.0f;
        ship.EnvMapFactor = 0.05f;
        // Spray
        ship.SprayVerticalPerf = 10.0f;
        ship.SprayMultiplier = 20;
        ship.SprayLength = 0.1f;
        ship.SprayType = 1;
        // Rudders
        ship.PosRudder = vec3(-135.32f, -2.24f, 0.0f);
        ship.RudderIncrement = 1.0f;
        ship.RudderStepMax = 35;
        ship.RudderRotSpeed = 2.8f;
        ship.RudderLiftPerf = 15.0f;
        ship.RudderDragPerf = 10.0f;
        ship.TurningDragPerf = 2.0f;
        ship.CentrifugalPerf = 40.0f;
        ship.RudderPivotFwd = vec3(100.0f, 0.0f, 0.0f);
        ship.RudderPivotBwd = vec3(-100.0f, 0.0f, 0.0f);
        ship.nRudder = 1;
        ship.Rudder1 = vec3(-135.36, -2.25f, 0.0f);
        // Power
        ship.PosPower = vec3(-131.0f, -12.4f, 0.0f);
        ship.PowerPerf = 0.15f;
        ship.PowerkW = 27160.0f;
        ship.PowerStepMax = 10;
        ship.PowerRpmMin = 0.0f;
        ship.PowerRpmMax = 80.0f;
        ship.PowerRpmIncrement = 1.0f;
        ship.nChimney = 1;
        ship.Chimney1 = vec3(-111.17f, 50.906f, 0.0f);
        ship.Chimney2 = vec3(-111.17f, 50.906f, 0.0f);
        ship.nPropeller = 1;
        ship.Propeller1 = vec3(-131.33f, -12.395f, 0.0f);
        ship.WakeWidth = 0.3f;
        ship.ThrustSound = "Resources/Sounds/Engine14.wav";
        // Bow thruster
        ship.HasBowThruster = false;
        // Lights
        ship.LightPositions.clear();
        ship.LightColors.clear();
        ship.LightPositions.push_back(vec3(-70.918f, 44.0f, -27.636f));     // Red
        ship.LightColors.push_back(vec3(1.0f, 0.0f, 0.0f));
        ship.LightPositions.push_back(vec3(-70.918f, 44.0f, 27.636f));      // Green
        ship.LightColors.push_back(vec3(0.0f, 1.0f, 0.0f));
        ship.LightPositions.push_back(vec3(-141.43f, 10.093f, 0.0f));       // White stern
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        ship.LightPositions.push_back(vec3(-78.207f, 62.104f, 0.0f));       // White high
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        ship.LightPositions.push_back(vec3(95.06f, 40.5f, 0.0f));           // White low
        ship.LightColors.push_back(vec3(1.0f, 1.0f, 1.0f));
        // Radar
        ship.nRadar = 2;
        ship.Radar1 = vec3(-78.54f, 59.363f, 5.5994f);
        ship.RotationRadar1 = 20.0f;
        ship.Radar2 = vec3(-78.54f, 59.363f, -5.5994f);
        ship.RotationRadar2 = 30.0f;
        // Autopilot
        ship.BaseP = 2.0f;
        ship.BaseI = 0.3f;
        ship.BaseD = 20.0f;
        ship.MaxIntegral = 10.0f;	
        ship.SpeedFactor = 5.0f;	
        ship.MinSpeed = 1.0f;		
        ship.LowSpeedBoost = 2.0f;	
        ship.SeaSateFactor = 1.0f;	
        // Waves
        ship.CenterFore = 180.0f;
        g_vShips.push_back(ship);
#pragma endregion

    }
   
    // Ship
    g_Ship.reset();

    g_Ship = make_unique<Ship>();
    g_Ship->SetOcean(g_Ocean.get());
    g_Ship->Init(g_vShips[g_NoShip], g_Camera);
    g_LowMass = g_vShips[g_NoShip].Mass_t / 2;
    g_HighMass = g_vShips[g_NoShip].Mass_t * 2;
    vec2 p = LonLatToOpenGL(g_vPositions[g_NoPosition].pos.x, g_vPositions[g_NoPosition].pos.y);
    g_Ship->ship.Position = vec3(p.x, 0.0f, p.y);
    g_Ship->SetYawFromHDG(g_vPositions[g_NoPosition].heading);
    g_Ship->ResetVelocities();
    g_Ship->bVisible = true;
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

    if (!g_bSoundSeagull)
        return;

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
    if (!g_FloatingBall->bVisible)
        return;
   
    float dim = (float)g_Ocean->PATCH_SIZE / 2.0f;

    // Draw 4 floating balls at the corner of the patch
    vec3 pos;
    if (g_Ocean->GetVertice(vec2(-dim, dim - 0.01f), pos))  // red
        g_FloatingBall->Render(g_Camera, pos, 1.0f, vec3(1.0f, 0.4f, 0.5f));
    if (g_Ocean->GetVertice(vec2(-dim, -dim), pos))         // green
        g_FloatingBall->Render(g_Camera, pos, 1.0f, vec3(0.4f, 1.0f, 0.5f));
    
    if (g_Ocean->GetVertice(vec2(dim, -dim), pos))
        g_FloatingBall->Render(g_Camera, pos, 1.0f, vec3(1.0f, 0.4f, 0.5f));
    if (g_Ocean->GetVertice(vec2(dim - 0.01f, dim - 0.01f), pos))
        g_FloatingBall->Render(g_Camera, pos, 1.0f, vec3(1.0f, 0.4f, 0.5f));
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
void RenderExternalGridsAround()
{
    if (!g_bGridVisible)
        return;

    float gridSize = g_Ocean->PATCH_SIZE;
    int numLayers = 10; // Number of grids on each side
    float halfSize = gridSize * numLayers / 2.0f;

    // Alternating colors: white, gray, white, gray, ...
    vec3 color[2] = { vec3(1.0f), vec3(0.7f, 0.7f, 0.7f) };

    // Origin of the center of the central grid
    vec3 center = vec3(0, 0, 0);

    for (int i = -numLayers; i <= numLayers; ++i)
    {
        for (int j = -numLayers; j <= numLayers; ++j)
        {
            // Grid position (i,j)
            if (i == 0 && j == 0)
                continue;

            vec3 pos = center + vec3(i * gridSize, 0, j * gridSize);

            // Choice of color according to distance from the central grid
            int distance = std::max(abs(i), abs(j));
            int colorIdx = distance % 2;
            vec3 currentColor = color[colorIdx];

            g_Grid->Render(g_Camera, pos, gridSize / 10.0f, currentColor);
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
    
    float waveLength = glm::two_pi<float>() * g_Ship->Velocity * g_Ship->Velocity / 9.81f;  // length between 2 crests
    float kelvinScale = 101.0f / waveLength;                // the texture is 1024 and there are 9 wavelengths in 910 pixels
    g_Ocean->Render(g_TimeSpeed * g_Timer.getTime(), g_Camera, g_Ship->ship.Position, g_Ship->Yaw, g_Ship->bWaves, g_Ship->LWL, kelvinScale, g_Ship->Velocity, g_Ship->ship.CenterFore);

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
void RenderTerrains(int t)
{
    // Terrain
    g_ShaderSun->use();
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
        model = glm::scale(model, g_vTerrains[t].scale);
        g_ShaderSun->setMat4("model", model);
        g_vTerrains[t].model->Render(*g_ShaderSun);
    }
    
    if (g_vTerrains[0].model->bVisible && g_Pier)
        g_Pier->Render(*g_ShaderSun);
}
void RenderArrowWind()
{
    if (!g_ArrowWind)
        return;
    
    if (!g_ArrowWind->bVisible)
        return;

    vec3 position = vec3(g_Ship->ship.Position.x, 1.0f, g_Ship->ship.Position.z) + 2.0f * g_Ship->GetLength() * glm::normalize(vec3(g_Wind.x, 0.0f, g_Wind.y));
    float dir = g_WindDirectionDEG + 180.0f;
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

        //ImGui::Begin("Parameters [F1]", nullptr, ImGuiWindowFlags_NoMove);
        if (ImGui::Begin("Scene [F1]", &g_bShowSceneWindow))
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
            case eCameraMode::ORBITAL:      ImGui::Text("[ ORBITAL ]"); break;
            case eCameraMode::FPS:          ImGui::Text("[ FPS ]");     break;
            case eCameraMode::IN_FIXED:     ImGui::Text("[ IN FIX ]");  break;
            case eCameraMode::IN_FREE:      ImGui::Text("[ IN FREE ]"); break;
            case eCameraMode::OUT_FREE:     ImGui::Text("[ OUT FREE ]"); break;
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
            // ------------
            if (ImGui::Button(" PAUSE "))
            {
                g_bPause = !g_bPause;
                if (g_bPause)
                    g_Timer.stop();
                else
                    g_Timer.start();
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
                    }
                }
                ImGui::SameLine();
                ImGui::Checkbox("Sound##1", &g_SoundMgr->bSound);
                // ------------
                if (g_vTerrains.size()) ImGui::Checkbox("Terrain", &g_vTerrains[0].model->bVisible);
                ImGui::SameLine();
                ImGui::Checkbox("Markup", &g_Markup->bVisible);
                ImGui::SameLine();
                ImGui::Checkbox("Grid", &g_bGridVisible);
                if (g_Axe) {
                    ImGui::SameLine();
                    ImGui::Checkbox("Axis", &g_Axe->bVisible);
                }
                // ------------
                if (g_FloatingBall) {
                    ImGui::Checkbox("Balls", &g_FloatingBall->bVisible);
                }
                ImGui::SameLine();
                ImGui::Checkbox("Wind Arrow", &g_ArrowWind->bVisible);
                ImGui::SameLine();
                ImGui::Checkbox("Seagull", &g_bSoundSeagull);
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
            if (ImGui::CollapsingHeader("WIND", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));         // Red
                ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));   // Dark red when active
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));            // Dark background
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));     // Lighter background on hover
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));      // Even brighter background when active
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));               // White text
                if (ImGui::SliderFloat("Direction", &g_WindDirectionDEG, 0.0f, 360.0f, "%.f°"))
                {
                    g_Wind = WindDirSpeed_Vec(g_WindDirectionDEG, g_WindSpeedKN);
                    g_Ocean->GetWind(g_Wind);
                    g_Ocean->InitFrequencies();
                }
                if (ImGui::SliderFloat("Speed##1", &g_WindSpeedKN, 1.0f, 30.0f, "%0.0f kn"))
                {
                    g_Wind = WindDirSpeed_Vec(g_WindDirectionDEG, g_WindSpeedKN);
                    g_Ocean->GetWind(g_Wind);
                    g_Ocean->InitFrequencies();
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
                    if (ImGui::SliderInt("# Color", &g_Ocean->iOceanColor, 0, g_Ocean->vOceanColors.size() - 1)) g_Ocean->OceanColor = ConvertToFloat(g_Ocean->vOceanColors[g_Ocean->iOceanColor]);
                    ImGui::ColorEdit3("Ocean", (float*)&g_Ocean->OceanColor[0], 0);
                    // Heights
                    static float waves1_3 = 0.0f;
                    static float waveMax = 0.0f;
                    static float average_period = 0.0f;
                    static int nWaves = 0;
                    g_Ocean->GetWaveByWaveAnalysis(waves1_3, waveMax, nWaves, average_period);
                    ImGui::Text("Height 1/3  : %.1f m (%.1f s) (%d waves)", waves1_3, average_period, nWaves);

                    double fetch = 50000.0; // 50 km en mètres
                    auto waveParams = JONSWAPModel::GetWaveParameters(KnotsToMS(g_WindSpeedKN), fetch);
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
            if (ImGui::CollapsingHeader("SUN", ImGuiTreeNodeFlags_DefaultOpen))
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
        if (ImGui::Begin("Ship [F2]", &g_bShowShipWindow))
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
                    {
                        g_NoShip = n;
                        LoadShips();
                    }

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
            ImGui::Checkbox("Waves", &g_Ship->bWaves);
            ImGui::SameLine();
            ImGui::Checkbox("Wake", &g_bShipWake);
            ImGui::SameLine();
            ImGui::Checkbox("Smoke", &g_Ship->bSmoke);
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
                {
                    g_Ship->bOutline = true;
                    g_Ship->bSmoke = false;
                    g_Ship->bLights = false;
                }
                if (choix == 2)
                {
                    g_Ship->bOutline = false;
                    g_Ship->bSmoke = true;
                }
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
            ImGui::Checkbox("Dynamic", &g_Ship->bDynamicAdjustment);
            ImGui::SameLine();
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
            {
                vec2 p = LonLatToOpenGL(g_vPositions[g_NoPosition].pos.x, g_vPositions[g_NoPosition].pos.y);
                g_Ship->ship.Position = vec3(p.x, 0.0f, p.y);
                g_Ship->SetYawFromHDG(g_vPositions[g_NoPosition].heading);
                g_Ship->ResetVelocities();
            }

            if (ImGui::Checkbox("Motion", &g_Ship->bMotion))
                g_Ship->ResetVelocities();

            ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));         // Red
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));   // Dark red when active
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));            // Dark background
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));     // Lighter background on hover
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));      // Even brighter background when active
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));               // White text
            float rot = glm::degrees(g_Ship->Yaw);
            if (ImGui::SliderFloat("Rotation", &rot, -180.0f, 180.0f, "%.f°"))
            {
                g_Ship->Yaw = glm::radians(rot);
                g_Ship->ResetVelocities();
            }
            ImGui::PopStyleColor(6);                                                            // For the 6 PushStyleColor above
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
    
    if (g_bShowAutopilotWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(350, 300), ImGuiCond_FirstUseEver);
        ImVec2 window_pos((g_WindowW - 350) / 2, g_WindowH - 300);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(gray, gray, gray, 0.75f));
        if (ImGui::Begin("Autopilot", &g_bShowAutopilotWindow))
        {
            ImGui::SliderFloat("P", &g_Ship->ship.BaseP, 0.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("I", &g_Ship->ship.BaseI, 0.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("D", &g_Ship->ship.BaseD, 0.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("MaxIntegral", &g_Ship->ship.MaxIntegral, 0.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("SpeedFactor", &g_Ship->ship.SpeedFactor, 0.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("MinSpeed", &g_Ship->ship.MinSpeed, 0.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("LowSpeedBoost", &g_Ship->ship.LowSpeedBoost, 0.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("SeaSateFactor", &g_Ship->ship.SeaSateFactor, 0.0f, 20.0f, "%.1f");
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
    sprintf_s(text, "CAMERA x = %.2f y = %.2f z = %.2f Heading = %03d° Roll = %d° Pitch = %d Distance to ship = %d m", 
        position.x, position.y, position.z, 
        (int)g_Camera.GetNorthAngleDEG(), (int)g_Camera.GetAttitudeDEG(), (int)g_Camera.GetRollDEG(), 
        int(distCameraShip));

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

    // Obtenir la coupe
    vector<vec2> cut = g_Ocean->GetCut(xN);
    if (cut.empty())
        return;

    for (size_t i = 0; i < cut.size(); i++)
        cut[i].x += i;

    // Calcul du coin supérieur gauche du rectangle centré
    float rectX = x - w * 0.5f;
    float rectY = y - h * 0.5f;

    // Déterminer les bornes min et max des points pour normaliser dans le rectangle
    auto [minY_it, maxY_it] = std::minmax_element(cut.begin(), cut.end(), [](const vec2& a, const vec2& b) { return a.y < b.y; });
    float minY = minY_it->y;
    float maxY = maxY_it->y;

    // Éviter division par zéro
    float rangeX = cut.back().x - cut.front().x;
    float rangeY = (maxY - minY) * 2.0;
    if (rangeY < 1e-6f) rangeY = 1.0f;

    // Tracer le rectangle fond + contour
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, rectX, rectY, w, h);
    nvgFillColor(g_Nvg, nvgRGBA(30, 144, 255, 64));  // fond bleu clair transparent
    nvgFill(g_Nvg);
    nvgStrokeColor(g_Nvg, nvgRGBA(30, 144, 255, 200)); // contour bleu transparent
    nvgStrokeWidth(g_Nvg, 2.0f);
    nvgStroke(g_Nvg);

    // Tracer la courbe de la coupe dans le rectangle
    nvgBeginPath(g_Nvg);

    for (size_t i = 0; i < cut.size(); i++)
    {
        // x normalisé croissant de gauche à droite
        float px = rectX + ((cut[i].x) / rangeX) * w;

        // y normalisé de bas en haut (écran a l’origine en haut)
        float py = rectY + h * 0.5 - ((cut[i].y - minY) / rangeY) * h;

        if (i == 0)
            nvgMoveTo(g_Nvg, px, py);
        else
            nvgLineTo(g_Nvg, px, py);
    }

    nvgStrokeColor(g_Nvg, nvgRGBA(255, 69, 0, 255)); // couleur orange foncé
    nvgStrokeWidth(g_Nvg, 2.0f);
    nvgStroke(g_Nvg);
}
void RenderTitle(float x, float y, float size, int align)
{
    nvgBeginPath(g_Nvg);
    nvgFillColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgFontSize(g_Nvg, size);
    nvgFontFace(g_Nvg, "caveat");
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

    float bounds[4];
    const char* text = "SimShip";
    nvgTextBounds(g_Nvg, x, y, text, NULL, bounds);
    //float textWidth = bounds[2] - bounds[0];
    //float textHeight = bounds[3] - bounds[1];

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
        nvgBeginPath(g_Nvg);
        nvgFillColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
        nvgFontSize(g_Nvg, 36);
        nvgFontFace(g_Nvg, "caveat");
        nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

        string s(g_CaptureName.begin(), g_CaptureName.end());
        const char* text = s.c_str();

        float x = g_WindowW / 2.0f;
        float y = g_WindowH / 4.0f;

        float bounds[4];
        nvgTextBounds(g_Nvg, x, y, text, NULL, bounds);

        nvgText(g_Nvg, x, y, text, nullptr);
        nvgStroke(g_Nvg);
    }
    // Afer 1 seconde, stop visibilty
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
void RenderDebugRectangle(float x, float y, float w, float h)
{
    // Rectangle à dessiner
    float dashLength = 6.0f;
    float gapLength = 3.0f;
    float strokeWidth = 1.0f;

    // Couleur du trait : noir
    NVGcolor color = nvgRGB(0, 0, 0);
    nvgStrokeColor(g_Nvg, color);
    nvgStrokeWidth(g_Nvg, strokeWidth);

    // TOP
    for (float dx = 0; dx < w; dx += dashLength + gapLength) {
        float dashW = std::min(dashLength, w - dx);
        nvgBeginPath(g_Nvg);
        nvgMoveTo(g_Nvg, x + dx, y);
        nvgLineTo(g_Nvg, x + dx + dashW, y);
        nvgStroke(g_Nvg);
    }
    // BOTTOM
    for (float dx = 0; dx < w; dx += dashLength + gapLength) {
        float dashW = std::min(dashLength, w - dx);
        nvgBeginPath(g_Nvg);
        nvgMoveTo(g_Nvg, x + dx, y + h);
        nvgLineTo(g_Nvg, x + dx + dashW, y + h);
        nvgStroke(g_Nvg);
    }
    // LEFT
    for (float dy = 0; dy < h; dy += dashLength + gapLength) {
        float dashH = std::min(dashLength, h - dy);
        nvgBeginPath(g_Nvg);
        nvgMoveTo(g_Nvg, x, y + dy);
        nvgLineTo(g_Nvg, x, y + dy + dashH);
        nvgStroke(g_Nvg);
    }
    // RIGHT
    for (float dy = 0; dy < h; dy += dashLength + gapLength) {
        float dashH = std::min(dashLength, h - dy);
        nvgBeginPath(g_Nvg);
        nvgMoveTo(g_Nvg, x + w, y + dy);
        nvgLineTo(g_Nvg, x + w, y + dy + dashH);
        nvgStroke(g_Nvg);
    }
}
void RenderEnv(float x, float y)
{
    float w = 80.0f;
    float h = 136.0f;
    float buttonWidth = w * 0.9f;
    float buttonHeight = h * 0.2f;
    float gap = 5.0f;

    // Rounded rectangle
    nvgBeginPath(g_Nvg);
    nvgRoundedRect(g_Nvg, x, y, w, h, 10);
    nvgFillColor(g_Nvg, nvgRGBA(192, 192, 192, 255));
    nvgFill(g_Nvg);
    nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgStrokeWidth(g_Nvg, 2.0f);
    nvgStroke(g_Nvg);

    // Camera mode button
    float currentY = y + 5;
    nvgBeginPath(g_Nvg);
    nvgRoundedRect(g_Nvg, x + (w - buttonWidth) / 2, currentY, buttonWidth, buttonHeight, 5);
    NVGcolor color = nvgRGBA(1, 121, 158, 255);
    switch (g_Camera.GetMode())
    {
    case eCameraMode::ORBITAL:  color = nvgRGBA(1, 121, 158, 255);    break;
    case eCameraMode::FPS:      color = nvgRGBA(38, 172, 225, 255);    break;
    case eCameraMode::IN_FIXED: color = nvgRGBA(252, 131, 40, 255);    break;
    case eCameraMode::IN_FREE:  color = nvgRGBA(254, 206, 1, 255);    break;
    case eCameraMode::OUT_FREE: color = nvgRGBA(139, 81, 157, 255);    break;
    }
    nvgFillColor(g_Nvg, color);
    nvgFill(g_Nvg);
    nvgFillColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgFontSize(g_Nvg, 18.0f);
    nvgFontFace(g_Nvg, "arial");
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    std::string mode;
    switch (g_Camera.GetMode())
    {
    case eCameraMode::ORBITAL:  mode = "orbital";    break;
    case eCameraMode::FPS:      mode = "fps";        break;
    case eCameraMode::IN_FIXED: mode = "in fix";     break;
    case eCameraMode::IN_FREE:  mode = "in free";    break;
    case eCameraMode::OUT_FREE: mode = "out free";   break;
    }
    nvgText(g_Nvg, x + w / 2, currentY + buttonHeight / 2, mode.c_str(), nullptr);

    // Fps
    float fpsFontSize = 10.0f;
    nvgFillColor(g_Nvg, nvgRGBA(80, 80, 80, 255));
    nvgFontSize(g_Nvg, fpsFontSize);
    nvgFontFace(g_Nvg, "arial");
    nvgTextAlign(g_Nvg, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
    char fpsTxt[16];
    snprintf(fpsTxt, sizeof(fpsTxt), "%d ips", g_Fps);
    float fpsPad = 6.0f;
    nvgText(g_Nvg, x + w - fpsPad, currentY + buttonHeight + 2.0f, fpsTxt, nullptr);

    currentY += buttonHeight + gap + fpsFontSize + 1.0f;
    
    // Time
    sHM hm = g_Sky->GetTime();
    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", hm.hour, hm.minute);
    nvgFillColor(g_Nvg, nvgRGBA(32, 32, 32, 255));
    nvgFontSize(g_Nvg, 22.0f);
    nvgFontFace(g_Nvg, "arial");
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(g_Nvg, x + w / 2, currentY + buttonHeight / 2, buffer, nullptr);

    g_CtrlTimeHour = vec4(x + 10, currentY, 30, 22);
    g_CtrlTimeMinute = vec4(x + 40, currentY, 30, 22);

    float nowButtonHeight = buttonHeight * 0.75f;
    currentY += buttonHeight + gap - 3;

    // Now button
    nvgBeginPath(g_Nvg);
    nvgRoundedRect(g_Nvg, x + (w - buttonWidth * 0.7f) / 2, currentY, buttonWidth * 0.7f, nowButtonHeight, 5);
    nvgFillColor(g_Nvg, nvgRGBA(150, 150, 150, 255));
    nvgFill(g_Nvg);
    nvgFillColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgFontSize(g_Nvg, 12.0f);
    nvgText(g_Nvg, x + w / 2, currentY + nowButtonHeight / 2, "NOW", nullptr);
    g_CtrlNow = vec4(x + (w - buttonWidth * 0.7f) / 2, currentY, buttonWidth * 0.7f, nowButtonHeight);
        
    currentY += nowButtonHeight + gap + 3;

    // Wind text
    char windText[32];
    snprintf(windText, sizeof(windText), "%d kn", int(g_WindSpeedKN));
    nvgFillColor(g_Nvg, nvgRGBA(255, 0, 0, 255));
    nvgFontSize(g_Nvg, 22.0f);
    nvgFontFace(g_Nvg, "arial");
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(g_Nvg, x + w / 2, currentY + 12, windText, nullptr);

    g_CtrlWind = vec4(x + 20, currentY, w - 20, 40);

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
    nvgFillColor(g_Nvg, nvgRGBA(192, 192, 192, 255));
    nvgFill(g_Nvg);
    nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgStrokeWidth(g_Nvg, 2.0f);
    nvgStroke(g_Nvg);
}
void RenderCompass(float x, float y, float rayon)
{
    // Settings
    float epaisseurCercle = 2.0f;
    float longueurGraduation = 5.0f;
    float taillePointCardinal = 10.0f;
    float tailleTriangle = 15.0f;

    // Circle ==========================

    nvgBeginPath(g_Nvg);
    nvgCircle(g_Nvg, x, y, rayon);
    nvgFillColor(g_Nvg, nvgRGBA(53, 73, 100, 255));
    nvgFill(g_Nvg);
    nvgStrokeColor(g_Nvg, nvgRGBA(53, 73, 100, 255));
    nvgStrokeWidth(g_Nvg, epaisseurCercle);
    nvgStroke(g_Nvg);

    // Graduations and cardinal points
    for (int i = 0; i < 360; i += 10)
    {
        float angle = i * NVG_PI / 180.0f;
        float cx = x + cosf(angle) * rayon;
        float cy = y + sinf(angle) * rayon;

        nvgBeginPath(g_Nvg);
        nvgMoveTo(g_Nvg, cx, cy);
        nvgLineTo(g_Nvg, x + cosf(angle) * (rayon - longueurGraduation), y + sinf(angle) * (rayon - longueurGraduation));
        nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
        nvgStrokeWidth(g_Nvg, 1.0f);
        nvgStroke(g_Nvg);
    }

    char pointCardinal[4];
    nvgFontSize(g_Nvg, taillePointCardinal);
    nvgFontFace(g_Nvg, "arial");
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    strcpy(pointCardinal, "COG");
    nvgFillColor(g_Nvg, nvgRGBA(200, 200, 0, 255));
    float angle = 90 * NVG_PI / 180.0f;
    float cx = x + cosf(angle) * rayon;
    float cy = y + sinf(angle) * rayon;
    nvgText(g_Nvg, x + cosf(angle) * (rayon - longueurGraduation - 25), y + sinf(angle) * (rayon - longueurGraduation - 25), pointCardinal, NULL);

    strcpy(pointCardinal, "HDG"); 
    nvgFillColor(g_Nvg, nvgRGBA(0, 200, 200, 255));
    angle = 270 * NVG_PI / 180.0f;
    cx = x + cosf(angle) * rayon;
    cy = y + sinf(angle) * rayon;
    nvgText(g_Nvg, x + cosf(angle) * (rayon - longueurGraduation - 25), y + sinf(angle) * (rayon - longueurGraduation - 25), pointCardinal, NULL);

    // HDG ==========================

    float angleTriangle = (g_Ship->HDG - 90.0f) * NVG_PI / 180.0f;
    float baseTriangle = 10.0f;     // Width of the base of the triangle
    float hauteurTriangle = 17.0f;  // Height of the triangle

    nvgBeginPath(g_Nvg);
    // Point of the triangle on the outer circle
    nvgMoveTo(g_Nvg, x + cosf(angleTriangle) * rayon, y + sinf(angleTriangle) * rayon);
    // Left corner of the base
    nvgLineTo(g_Nvg,
        x + cosf(angleTriangle) * (rayon - hauteurTriangle) - sinf(angleTriangle) * (baseTriangle / 2),
        y + sinf(angleTriangle) * (rayon - hauteurTriangle) + cosf(angleTriangle) * (baseTriangle / 2));
    // Right corner of the base
    nvgLineTo(g_Nvg,
        x + cosf(angleTriangle) * (rayon - hauteurTriangle) + sinf(angleTriangle) * (baseTriangle / 2),
        y + sinf(angleTriangle) * (rayon - hauteurTriangle) - cosf(angleTriangle) * (baseTriangle / 2));
    nvgClosePath(g_Nvg);
    nvgFillColor(g_Nvg, nvgRGBA(0, 200, 200, 255));
    nvgFill(g_Nvg);

    // HDG display 
    char capStr1[10];
    snprintf(capStr1, sizeof(capStr1), "%.1f°", g_Ship->HDG);
    nvgFontSize(g_Nvg, rayon / 3);
    nvgFontFace(g_Nvg, "arial");
    nvgFillColor(g_Nvg, nvgRGBA(0, 200, 200, 255));
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(g_Nvg, x, y - 12, capStr1, NULL);

    // COG ==========================

    angleTriangle = (g_Ship->COG - 90.0f) * NVG_PI / 180.0f;

    nvgBeginPath(g_Nvg);
    // Point of the triangle on the outer circle
    nvgMoveTo(g_Nvg, x + cosf(angleTriangle) * rayon, y + sinf(angleTriangle) * rayon);
    // Left corner of the base
    nvgLineTo(g_Nvg,
        x + cosf(angleTriangle) * (rayon - hauteurTriangle) - sinf(angleTriangle) * (baseTriangle / 2),
        y + sinf(angleTriangle) * (rayon - hauteurTriangle) + cosf(angleTriangle) * (baseTriangle / 2));
    // Right corner of the base
    nvgLineTo(g_Nvg,
        x + cosf(angleTriangle) * (rayon - hauteurTriangle) + sinf(angleTriangle) * (baseTriangle / 2),
        y + sinf(angleTriangle) * (rayon - hauteurTriangle) - cosf(angleTriangle) * (baseTriangle / 2));
    nvgClosePath(g_Nvg);
    nvgFillColor(g_Nvg, nvgRGBA(200, 200, 0, 255));
    nvgFill(g_Nvg);

    // COG display
    char capStr2[10];
    snprintf(capStr2, sizeof(capStr2), "%.1f°", g_Ship->COG);
    nvgFontSize(g_Nvg, rayon / 3);
    nvgFontFace(g_Nvg, "arial");
    nvgFillColor(g_Nvg, nvgRGBA(200, 200, 0, 255));
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(g_Nvg, x, y + 15, capStr2, NULL);

    // Wind marker ==========================
    
    float angleWind = (g_WindDirectionDEG - 90.0f) * NVG_PI / 180.0f;
    float triangleHauteur = 10.8f; // Hauteur réduite et rentrée
    float triangleBase = 9.35f;    // Base ajustée pour équilatéral
    // Pointe légèrement rentrée dans le cercle
    float pointeR = rayon - 3.0f; // 3px à l'intérieur du bord
    float outerX = x + cosf(angleWind) * pointeR;
    float outerY = y + sinf(angleWind) * pointeR;
    // Base positionnée à l’extérieur de la pointe (toujours à l'extérieur du cercle)
    float baseCenterX = x + cosf(angleWind) * (pointeR + triangleHauteur);
    float baseCenterY = y + sinf(angleWind) * (pointeR + triangleHauteur);
    // Direction perpendiculaire pour la base
    float perpAngleL = angleWind + NVG_PI / 2.0f;
    float perpAngleR = angleWind - NVG_PI / 2.0f;
    // Les deux coins de la base
    float leftBaseX = baseCenterX + cosf(perpAngleL) * (triangleBase / 2.0f);
    float leftBaseY = baseCenterY + sinf(perpAngleL) * (triangleBase / 2.0f);
    float rightBaseX = baseCenterX + cosf(perpAngleR) * (triangleBase / 2.0f);
    float rightBaseY = baseCenterY + sinf(perpAngleR) * (triangleBase / 2.0f);
    nvgBeginPath(g_Nvg);
    // Pointe du triangle (rentrée dans le cercle)
    nvgMoveTo(g_Nvg, outerX, outerY);
    // Premier coin de base (gauche)
    nvgLineTo(g_Nvg, leftBaseX, leftBaseY);
    // Second coin de base (droit)
    nvgLineTo(g_Nvg, rightBaseX, rightBaseY);
    nvgClosePath(g_Nvg);
    nvgFillColor(g_Nvg, nvgRGBA(255, 0, 0, 255));
    nvgFill(g_Nvg);

    // HDG instruction marker ========================

    if (g_Ship->bAutopilot)
    {
        float angleInstruc = (g_Ship->HDGInstruction - 90.0f) * NVG_PI / 180.0f;

        // Center marker position, just outside the circle
        float markDist = rayon + 4.0f; // distance from the center of the circle to the center of the square
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
        nvgFillColor(g_Nvg, nvgRGBA(0, 200, 200, 255));
        nvgFill(g_Nvg);

        // Drawing of the two white side borders
        nvgStrokeWidth(g_Nvg, 1.0f);
        nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255)); // white

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
    }

    // Rate of turn ==================================

    if (fabsf(g_Ship->YawRate * (180.0f / M_PI) * 60.0f) > 1.0f)
    { 
        float rate = g_Ship->YawRate * 60.0f * Sign(-g_Ship->VariationYawSigned);
        float rayonArc = rayon - 11.0f;
        float angleStart = (g_Ship->HDG - 90.0f) * NVG_PI / 180.0f;
        float angleEnd = angleStart + rate;

        nvgBeginPath(g_Nvg);
        nvgArc(g_Nvg, x, y, rayonArc, angleStart, angleEnd, rate < 0.0f ? NVG_CCW : NVG_CW);
        nvgStrokeColor(g_Nvg, nvgRGBA(0, 200, 200, 180));
        nvgStrokeWidth(g_Nvg, 4.0f);
        nvgStroke(g_Nvg);
    }
}
void RenderThrottle(float x, float y, float w, float h)
{
    float largeur = w;
    float hauteur = h;
    float n = std::min(10.0f, 2.0f * g_Ship->ship.PowerStepMax);
    float separationHauteur = hauteur / n;
    float valeur = 0.5f + g_Ship->PowerCurrentStep / (2.0f * g_Ship->ship.PowerStepMax);

    g_CtrlThrottle = vec4(x, y, largeur, hauteur);
    g_CtrlThrottleHigh = y;
    g_CtrlThrottleLow = y + h;

    // Slider background
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x, y, largeur, hauteur);
    nvgFillColor(g_Nvg, nvgRGBA(36, 36, 36, 255));
    nvgFill(g_Nvg);
    nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgStrokeWidth(g_Nvg, 2.0f);
    nvgStroke(g_Nvg);

    // Green part (upper)
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x, y, largeur, hauteur * 0.5f);
    nvgFillColor(g_Nvg, nvgRGBA(0, 200, 0, 255));
    nvgFill(g_Nvg);

    if (g_Ship->PowerRpm > 0)
    {
        float r = g_Ship->PowerRpm / g_Ship->ship.PowerRpmMax;
        float yy = y + (1.0f - r) * hauteur * 0.5f;
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, x, y + (1.0f - r) * hauteur * 0.5f, largeur, r * hauteur * 0.5f);
        nvgFillColor(g_Nvg, nvgRGBA(0, 255, 0, 255));
        nvgFill(g_Nvg);
       
        nvgBeginPath(g_Nvg);
        nvgMoveTo(g_Nvg, x, yy);
        nvgLineTo(g_Nvg, x + largeur, yy);
        nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
        nvgStrokeWidth(g_Nvg, 1.0f);
        nvgStroke(g_Nvg);
    }

    // Red part (lower)
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x, y + hauteur * 0.5f, largeur, hauteur * 0.5f);
    nvgFillColor(g_Nvg, nvgRGBA(200, 0, 0, 255));
    nvgFill(g_Nvg);

    if (g_Ship->PowerRpm < 0)
    {
        float r = g_Ship->PowerRpm / -g_Ship->ship.PowerRpmMax;
        float yy = y + hauteur * 0.5f + r * hauteur * 0.5f;
        nvgBeginPath(g_Nvg);
        nvgRect(g_Nvg, x, y + hauteur * 0.5f, largeur, r * hauteur * 0.5f);
        nvgFillColor(g_Nvg, nvgRGBA(255, 0, 0, 255));
        nvgFill(g_Nvg);
        
        nvgBeginPath(g_Nvg);
        nvgMoveTo(g_Nvg, x, yy);
        nvgLineTo(g_Nvg, x + largeur, yy);
        nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
        nvgStrokeWidth(g_Nvg, 1.0f);
        nvgStroke(g_Nvg);
    }

    // Separations
    nvgStrokeColor(g_Nvg, nvgRGBA(0, 0, 0, 100));
    nvgStrokeWidth(g_Nvg, 1);
    for (int i = 1; i < n; i++) 
    {
        float yPos = y + i * separationHauteur;
        nvgBeginPath(g_Nvg);
        nvgMoveTo(g_Nvg, x, yPos);
        nvgLineTo(g_Nvg, x + largeur, yPos);
        nvgStroke(g_Nvg);
    }

    // Cursor
    float curseurY = y + hauteur * (1 - valeur);
   
    // Lateral axes
    nvgBeginPath(g_Nvg);
    nvgFillColor(g_Nvg, nvgRGBA(128, 128, 128, 255));
    nvgRect(g_Nvg, x - 2, curseurY, 4, hauteur / 2 - hauteur * (1 - valeur));
    nvgRect(g_Nvg, x + largeur - 2, curseurY, 4, hauteur / 2 - hauteur * (1 - valeur));
    nvgFill(g_Nvg);
    
    // Central controller
    nvgBeginPath(g_Nvg);
    int hh = 8;
    nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
    nvgRect(g_Nvg, x - 2, curseurY - hh / 2, largeur + 4, hh);
    nvgFill(g_Nvg);

    // Speed ​​Update
	bool bUpdate = false;
    static double prevTime = 0.0;
    if (g_Timer.getTime() - prevTime > 0.5)
    {
        bUpdate = true;
        prevTime = g_Timer.getTime();
    }
    else
        bUpdate = false;

    // Speed
    static char text[50];
    if (bUpdate)
        sprintf_s(text, "%.2f kt", fabs(g_Ship->SOG));
    nvgFontSize(g_Nvg, 16.0f);
    nvgFontFace(g_Nvg, "arial");
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
    nvgText(g_Nvg, x + largeur + 35, y + hauteur * 0.5f, text, NULL);

    // Directional triangle
    float triangleX = x + largeur + 35;
    // Upward triangle (forward)
    float triangleY = y + hauteur * 0.5f - 20;
    nvgBeginPath(g_Nvg);
    nvgMoveTo(g_Nvg, triangleX - 10, triangleY + 8);
    nvgLineTo(g_Nvg, triangleX + 10, triangleY + 8);
    nvgLineTo(g_Nvg, triangleX, triangleY - 8);
    if (g_Ship->LinearVelocity > 0) nvgFillColor(g_Nvg, nvgRGBA(250, 250, 250, 255));
    else                            nvgFillColor(g_Nvg, nvgRGBA(200, 200, 200, 255));
    nvgClosePath(g_Nvg);
    nvgFill(g_Nvg);

    // Downward triangle (backward)
    triangleY = y + hauteur * 0.5f + 20;
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
            sprintf_s(text, "%.1f kt", fabs(g_Ship->SOGbow));
        nvgFontSize(g_Nvg, 12.0f);
        nvgFontFace(g_Nvg, "arial");
        nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        if (g_Ship->SOGbow < 0)
        {
            nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
            nvgText(g_Nvg, x - 35, y + 10, text, NULL);
            // Triangle
            float xx = x - 5;
            float yy = y + 10;
            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xx - 8, yy);
            nvgLineTo(g_Nvg, xx, yy - 5);
            nvgLineTo(g_Nvg, xx, yy + 5);
            nvgFillColor(g_Nvg, nvgRGBA(200, 0, 0, 255));
            nvgFill(g_Nvg);
            nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
            nvgStrokeWidth(g_Nvg, 1.0f);
            nvgStroke(g_Nvg);
        }
        else
        {
            nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
            nvgText(g_Nvg, x + largeur + 35, y + 10, text, NULL);
            // Triangle
            float xx = x + largeur + 5;
            float yy = y + 10;
            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xx + 8, yy);
            nvgLineTo(g_Nvg, xx, yy - 5);
            nvgLineTo(g_Nvg, xx, yy + 5);
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
            sprintf_s(text, "%.1f kt", fabs(g_Ship->SOGstern));
        nvgFontSize(g_Nvg, 12.0f);
        nvgFontFace(g_Nvg, "arial");
        nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        if (g_Ship->SOGstern < 0)
        {
            nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
            nvgText(g_Nvg, x - 35, y + hauteur - 10, text, NULL);
            // Triangle
            float xx = x - 5;
            float yy = y + hauteur - 10;
            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xx - 8, yy);
            nvgLineTo(g_Nvg, xx, yy - 5);
            nvgLineTo(g_Nvg, xx, yy + 5);
            nvgFillColor(g_Nvg, nvgRGBA(200, 0, 0, 255));
            nvgFill(g_Nvg);
            nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
            nvgStrokeWidth(g_Nvg, 1.0f);
            nvgStroke(g_Nvg);
        }
        else
        {
            nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 255));
            nvgText(g_Nvg, x + largeur + 35, y + hauteur - 10, text, NULL);
            // Triangle
            float xx = x + largeur + 5;
            float yy = y + hauteur - 10;
            nvgBeginPath(g_Nvg);
            nvgMoveTo(g_Nvg, xx + 8, yy);
            nvgLineTo(g_Nvg, xx, yy - 5);
            nvgLineTo(g_Nvg, xx, yy + 5);
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
void RenderPitchRoll(float x, float y, float w, float h)
{
    // Draw the lower half of the square (blue)
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x, y + h / 2, w, h / 2);
    nvgFillColor(g_Nvg, nvgRGBA(53, 73, 100, 255));
    nvgFill(g_Nvg);

    // Draw the top half of the square (very light blue)
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x, y, w, h / 2);
    nvgFillColor(g_Nvg, nvgRGBA(73, 93, 120, 255));
    nvgFill(g_Nvg);

    char text[50];
    sprintf_s(text, "Pitch  %.1f°", glm::degrees(g_Ship->Pitch));
    nvgFontSize(g_Nvg, 12.0f);
    nvgFontFace(g_Nvg, "arial");
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgText(g_Nvg, x + w / 2, y + h / 6, text, NULL);

    sprintf_s(text, "Roll  %.1f°", glm::degrees(g_Ship->Roll));
    nvgFontSize(g_Nvg, 12.0f);
    nvgFontFace(g_Nvg, "arial");
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgText(g_Nvg, x + w / 2, y + 5 * h / 6, text, NULL);

    // Calculate the position of the circle based on the pitch
    float centreY = y + h / 2 - 2.0f * g_Ship->Pitch * h / 2;
    float rayon = w / 10;
    // Calculate the top and bottom limits so that the circle remains within the rectangle
    float minCentreY = y + rayon;
    float maxCentreY = y + h - rayon;
    // Clamp to keep center Y inside the frame
    if (centreY < minCentreY)
        centreY = minCentreY;
    else if (centreY > maxCentreY)
        centreY = maxCentreY;
    
    // Draw the small, non-filled circle (very light gray)
    nvgBeginPath(g_Nvg);
    nvgCircle(g_Nvg, x + w / 2, centreY, rayon);
    nvgStrokeColor(g_Nvg, nvgRGBA(220, 220, 220, 255));
    nvgStrokeWidth(g_Nvg, 1.0f);
    nvgStroke(g_Nvg);

    // Draw the slanted lines
    float angleRad = 2.0f * g_Ship->Roll;
    float traitLongueur = w / 2;

    nvgSave(g_Nvg);
    nvgTranslate(g_Nvg, x + w / 2, centreY);
    nvgRotate(g_Nvg, angleRad);

    nvgBeginPath(g_Nvg);
    nvgMoveTo(g_Nvg, -rayon, 0);
    nvgLineTo(g_Nvg, -traitLongueur, 0);
    nvgMoveTo(g_Nvg, rayon, 0);
    nvgLineTo(g_Nvg, traitLongueur, 0);
    nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgStrokeWidth(g_Nvg, 1.0f);
    nvgStroke(g_Nvg);

    nvgRestore(g_Nvg);
}
void RenderRoT(float x, float y, float w, float h)
{
    // Draw the square (light blue)
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x, y, w, h);
    nvgFillColor(g_Nvg, nvgRGBA(93, 113, 140, 255));
    nvgFill(g_Nvg);
    
    char text[50];
    sprintf_s(text, "RoT %.0f°/mn", g_Ship->YawRate * (180.0f / M_PI) * 60.0f);
    nvgFontSize(g_Nvg, 12.0f);
    nvgFontFace(g_Nvg, "arial");
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgText(g_Nvg, x + w / 2, y + h / 2 + 2, text, NULL);
}
void RenderRudder(float x, float y, float w, float h)
{
    // Draw the left part (red)
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x, y, w / 2, h);
    nvgFillColor(g_Nvg, nvgRGBA(200, 0, 0, 255));
    nvgFill(g_Nvg);

    // Draw the right part (green)
    nvgBeginPath(g_Nvg);
    nvgRect(g_Nvg, x + w / 2, y, w / 2, h);
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

    // Main frame ============================

    nvgBeginPath(g_Nvg);
    nvgRoundedRect(g_Nvg, x, y, w, h, 10);
    nvgFillColor(g_Nvg, nvgRGBA(192, 192, 192, 255));
    nvgFill(g_Nvg);
    nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgStrokeWidth(g_Nvg, 2.0f);
    nvgStroke(g_Nvg);

    // Auto/Stby button
    float buttonWidth = w * 0.9f;
    float buttonHeight = h * 0.2f;
    nvgBeginPath(g_Nvg);
    nvgRoundedRect(g_Nvg, x + (w - buttonWidth) / 2, y + 5, buttonWidth, buttonHeight, 5);
    nvgFillColor(g_Nvg, isAuto ? nvgRGBA(0, 200, 0, 255) : nvgRGBA(200, 0, 0, 255));
    nvgFill(g_Nvg);

    nvgFillColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
    nvgFontSize(g_Nvg, 18.0f);
    nvgFontFace(g_Nvg, "arial");
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(g_Nvg, x + w / 2, y + 7 + buttonHeight / 2, isAuto ? "AUTO" : "STBY", nullptr);

    g_CtrlAutopilotCMD = vec4(x + (w - buttonWidth) / 2, y + 5, buttonWidth, buttonHeight);

    // Course instruction ========================

    buttonWidth = w * 0.7f;
    buttonHeight = h * 0.12f;
    nvgBeginPath(g_Nvg);
    nvgRoundedRect(g_Nvg, x + (w - buttonWidth) / 2, y + 35, buttonWidth, buttonHeight, 5);
    nvgFillColor(g_Nvg, nvgRGBA(100, 100, 100, 255));
    nvgFill(g_Nvg);
    nvgFillColor(g_Nvg, nvgRGBA(0, 200, 200, 255));
    nvgStrokeWidth(g_Nvg, 1.0f);
    nvgFontSize(g_Nvg, 14.0f);
    nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    char capStr1[10];
    snprintf(capStr1, sizeof(capStr1), "%03d°", g_Ship->HDGInstruction);
    nvgText(g_Nvg, x + w / 2, y + 36 + buttonHeight / 2, capStr1, nullptr);

    // Control buttons ==============================

    float buttonSize = h * 0.25f;
    float buttonY1 = y + h * 0.4f;
    float buttonY2 = y + h * 0.7f;
    float buttonX1 = x + w * 0.25f - buttonSize / 2;
    float buttonX2 = x + w * 0.75f - buttonSize / 2;

    // Function to Draw a round button
    auto drawRoundButton = [&](float bx, float by, const char* label) {
        nvgBeginPath(g_Nvg);
        nvgCircle(g_Nvg, bx + buttonSize / 2, by + buttonSize / 2, buttonSize / 2);
        nvgFillColor(g_Nvg, nvgRGBA(150, 150, 150, 255));
        nvgFill(g_Nvg);
        nvgStrokeColor(g_Nvg, nvgRGBA(200, 200, 200, 255));
        nvgStrokeWidth(g_Nvg, 1.0f);
        nvgStroke(g_Nvg);

        nvgFillColor(g_Nvg, nvgRGBA(255, 255, 255, 255));
        nvgFontSize(g_Nvg, 16.0f);
        nvgTextAlign(g_Nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(g_Nvg, bx + buttonSize / 2, by + buttonSize / 2, label, nullptr);
        };

    drawRoundButton(buttonX1, buttonY1, "<");
    drawRoundButton(buttonX2, buttonY1, ">");
    drawRoundButton(buttonX1, buttonY2, "<<");
    drawRoundButton(buttonX2, buttonY2, ">>");

    g_CtrlAutopilotM1 = vec4(buttonX1, buttonY1, buttonSize, buttonSize);
    g_CtrlAutopilotP1 = vec4(buttonX2, buttonY1, buttonSize, buttonSize);
    g_CtrlAutopilotM10 = vec4(buttonX1, buttonY2, buttonSize, buttonSize);
    g_CtrlAutopilotP10 = vec4(buttonX2, buttonY2, buttonSize, buttonSize);

    // Dynamic adjustement =========================

    // Center of the symbol between the buttons
    float centerX = x + w / 2;
    float centerY = (buttonY1 + buttonY2 + buttonSize) / 2;
    float diamondSize = w / 4;
    float halfSize = diamondSize / 2.0f;

    g_CtrlAutopilotDynAdjust = vec4(centerX - diamondSize, centerY - diamondSize, 2 * diamondSize, 2 * diamondSize);

    // Coordinates of the 4 vertices of the rhombus (top, right, bottom, left)
    float x0 = centerX;
    float y0 = centerY - halfSize;     // top

    float x1 = centerX + halfSize;
    float y1 = centerY;                // right

    float x2 = centerX;
    float y2 = centerY + halfSize;     // bottom

    float x3 = centerX - halfSize;
    float y3 = centerY;                // left

    // Light gray filled diamond
    nvgBeginPath(g_Nvg);
    nvgMoveTo(g_Nvg, x0, y0);
    nvgLineTo(g_Nvg, x1, y1);
    nvgLineTo(g_Nvg, x2, y2);
    nvgLineTo(g_Nvg, x3, y3);
    nvgClosePath(g_Nvg);
    nvgFillColor(g_Nvg, nvgRGBA(150, 150, 150, 255));
    nvgFill(g_Nvg);
    nvgStrokeColor(g_Nvg, nvgRGBA(200, 200, 200, 255));
    nvgStrokeWidth(g_Nvg, 1.0f);
    nvgStroke(g_Nvg);

    if (g_Ship->bDynamicAdjustment)
    { 
        // Small horizontal white line in the center of the diamond
        float lineLength = diamondSize * 0.6f; // stroke length
        float lineHalf = lineLength / 2.0f;

        nvgBeginPath(g_Nvg);
        nvgMoveTo(g_Nvg, centerX - lineHalf, centerY);
        nvgLineTo(g_Nvg, centerX + lineHalf, centerY);
        nvgStrokeColor(g_Nvg, nvgRGBA(255, 255, 255, 255)); // white
        nvgStrokeWidth(g_Nvg, 2.0f); // line thickness
        nvgStroke(g_Nvg);
    }
    else
    {
        // Small white circle in the center of the diamond
        float radius = diamondSize * 0.12f; // radius of the circle
        nvgBeginPath(g_Nvg);
        nvgCircle(g_Nvg, centerX, centerY, radius);
        nvgFillColor(g_Nvg, nvgRGBA(255, 255, 255, 255)); // white
        nvgFill(g_Nvg);
    }

    // Night =====================

	if (g_Sky->SunPosition.y < 0.0f)
	{
        nvgBeginPath(g_Nvg);
        nvgRoundedRect(g_Nvg, x, y, w, h, 10);
        nvgFillColor(g_Nvg, nvgRGBA(0, 0, 0, 128));
        nvgFill(g_Nvg);
    }
}
void RenderDashboard()
{
    // Ship controls interface
    if (g_Ship && g_Ship->bVisible && !g_bBinoculars)
    {
        int widthAutopilot = 80;
        int widthSeparation = 5;

        if(g_Ship->ship.HasBowThruster)
        {
            int widthControlFrame = 415;
            int startX = g_WindowW_2 - (widthControlFrame + 2 * widthSeparation + 2 * widthAutopilot) / 2;
            RenderEnv(startX, 2);
            int xFrame = startX + widthAutopilot + widthSeparation;
            RenderControlFrame(xFrame, 2, widthControlFrame, 136);
            int x = xFrame + 70;
            RenderCompass(x, 70, 58);
            x += 80;
            RenderBowThruster(x, 45, 50, 30);
            x += 70;
            RenderThrottle(x, 10, 30, 100);
            x -= 45;
            RenderRudder(x, 120, 120, 10);
            x += 150;
            RenderPitchRoll(x, 20, 80, 80);
            RenderRoT(x, 105, 80, 25);
            RenderControlFrameNight(xFrame, 2, widthControlFrame, 136);
            RenderAutopilot(xFrame + widthControlFrame + widthSeparation, 2);
        }
        else
        {
            int widthControlFrame = 415 - 35;
            int startX = g_WindowW_2 - (widthControlFrame + 2 * widthSeparation + 2 * widthAutopilot) / 2;
            RenderEnv(startX, 2);
            int xFrame = startX + widthAutopilot + widthSeparation;
            RenderControlFrame(xFrame, 2, widthControlFrame, 136);
            int x = xFrame + 70;
            RenderCompass(x, 70, 58);
            x += 110;
            RenderThrottle(x, 10, 30, 100);
            x -= 45;
            RenderRudder(x, 120, 120, 10);
            x += 150;
            RenderPitchRoll(x, 20, 80, 80);
            RenderRoT(x, 105, 80, 25);
            RenderControlFrameNight(xFrame, 2, widthControlFrame, 136);
            RenderAutopilot(xFrame + widthControlFrame + widthSeparation, 2);
        }
    }
}

void Render()
{
    // Physics actuator
    static bool bInit = false;
    static float firstTime = g_Timer.getTime();
    static int PrevNoShip;      // to detect a change of vessel
    static int PrevNoPosition;  // to detect a change of position
    static bool PrevVisiblity;
    // Detects if the current ship has changed (different pointer)
    if (g_NoShip != PrevNoShip || g_NoPosition != PrevNoPosition || g_Ship->bVisible != PrevVisiblity)
    {
        // New ship: we reset the physical tempo
        bInit = false;
        firstTime = g_Timer.getTime();
        g_Ship->bMotion = false;            // disable physics during latency
        PrevNoShip = g_NoShip;              // memorizes the current ship
        PrevNoPosition = g_NoPosition;      // memorizes the current position
        PrevVisiblity = g_Ship->bVisible;   // memorizes the current visibility
    }

    // Physics cutoff for 1 second after any change
    if (!bInit && (g_Timer.getTime() > firstTime + 1.0f)) 
    {
        g_Ship->bMotion = true;
        bInit = true;
    }

    RenderImGui();

    bool bAboveWater = g_Camera.GetPosition().y > 0.0f;

    // Rendering the scene inverted for the reflection texture
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
        RenderTerrains(0);
        g_Markup->Render(g_Camera, g_Ocean.get(), g_Sky.get());
        RenderAxis();
        RenderBalls();
        RenderCentralGridColored();
        RenderExternalGridsAround();
        RenderArrowWind();

        if (g_Camera.GetMode() == eCameraMode::IN_FIXED || g_Camera.GetMode() == eCameraMode::IN_FREE)
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

        // Particles
        if (bAboveWater)
        {
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

        g_ShaderPostProcessing->use();  // Shaders/post_processing.vert Shaders/post_processing.frag
        g_ShaderPostProcessing->setSampler2D("texColor", TexSceneColor, 0);     // Get the previous color texture (from the scene)
        g_ShaderPostProcessing->setSampler2D("texDepth", TexSceneDepth, 1);
        g_ShaderPostProcessing->setFloat("exposure", g_Sky->Exposure);
        g_ShaderPostProcessing->setFloat("near", 0.1f);
        g_ShaderPostProcessing->setFloat("far", 30000.f);
        g_ShaderPostProcessing->setFloat("horizonHeight", g_Camera.GetHorizonViewportY());
        g_ShaderPostProcessing->setVec3("eyePos", g_Camera.GetPosition());          // For underwater effect
        g_ShaderPostProcessing->setVec3("oceanColor", g_Ocean->OceanColor);
        g_ShaderPostProcessing->setVec3("fogColor", g_Sky->FogColor);
        g_ShaderPostProcessing->setFloat("mistDensity", g_Sky->MistDensity);
        g_ShaderPostProcessing->setFloat("fogDensity", g_Sky->FogDensity);
        g_ShaderPostProcessing->setFloat("uTime", g_Timer.getTime());               // For underwater particles
        g_ShaderPostProcessing->setVec2("screenSize", vec2(g_WindowW, g_WindowH));
        g_ShaderPostProcessing->setBool("bLowIntensity", g_bLowIntensity && bAboveWater);
        g_ShaderPostProcessing->setBool("bNightVision", g_bNightVision && bAboveWater);
        g_ScreenQuadPost->Render();

        if ((g_Sky->bRain || g_bBinoculars) && bAboveWater)
        {
            // 2 passes: 1 for the rain And or the binoculars, 2 for the fxaa
            
            // Rendering to a FBO for the 1st pass
            glBindFramebuffer(GL_FRAMEBUFFER, FBO_RAIN);
            glViewport(0, 0, g_WindowW, g_WindowH);
            glClearColor(0.0, 0.0, 0.0, 0.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            g_ShaderRain->use();    // Shaders/rain.vert Shaders/rain.frag
            g_ShaderRain->setSampler2D("texColor", TexPostColor, 0);    // Get the previous color texture (post processing)
            g_ShaderRain->setFloat("uTime", g_Timer.getTime());           
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

            g_ShaderFXAA->use();    // Shaders/fxaa.vert Shaders/fxaa.frag
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
        //RenderTitle(g_WindowW_2, g_WindowH - 200, 72, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        RenderTitle(g_WindowW - 70, g_WindowH - 25, 36.0f, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        RenderInfo();
        RenderDashboard();
    }
    nvgEndFrame(g_Nvg);

    // ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Debugging for the textures (Displacement, Gradient, Foam buffer, Wake)
    RenderTextures();
}

