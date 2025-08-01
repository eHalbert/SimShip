#include "Utility.h"

// Support
void InitConsole()
{
    // Si l'attachement échoue, créez une nouvelle console
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);

    SetConsoleTitle(L"SimShip Console");
    SetWindowPos(GetConsoleWindow(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE);
}
void ConsoleClose()
{
    FreeConsole();
}
void ConsoleClear()
{
    HANDLE	hConsoleOut;    // Handle to the console 
    hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo; // Console information 
    GetConsoleScreenBufferInfo(hConsoleOut, &csbiInfo);

    DWORD dummy;
    COORD Home = { 0, 0 };
    FillConsoleOutputCharacter(hConsoleOut, ' ', csbiInfo.dwSize.X * csbiInfo.dwSize.Y, Home, &dummy);
}

void PrintGlmMatrix(mat4& mat, string name)
{
    cout << "glm::mat : " << name << endl;
    for (int i = 0; i < 4; ++i)
    {
        cout << "[ ";
        for (int j = 0; j < 4; ++j)
            cout << setw(10) << setprecision(4) << fixed << mat[j][i] << " ";
        cout << "]" << endl;
    }
}
void PrintGlmVec3(vec3 vec, string name)
{
    cout << "glm::vec3 : " << name << endl;
    cout << "[ ";
    for (int i = 0; i < 3; ++i)
    {
        cout << setw(10) << setprecision(4) << fixed << vec[i] << " ";
    }
    cout << "]" << endl;
}
void PrintGlmVec3(vec3 vec)
{
    cout << "[ ";
    for (int i = 0; i < 3; ++i)
    {
        cout << setw(10) << setprecision(4) << fixed << vec[i] << " ";
    }
    cout << "]" << endl;
}

void SetVsync(int interval)
{
    glfwSwapInterval(interval);
}
void GetLimitsGPU()
{
    // Vérifiez les limites de votre GPU
    GLint workGroupSize[3], workGroupInvocations;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &workGroupSize[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &workGroupSize[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &workGroupSize[2]);
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &workGroupInvocations);

    cout << "Taille maximale des groupes de travail : " << workGroupSize[0] << " x " << workGroupSize[1] << " x " << workGroupSize[2] << endl;
    cout << "Nombre maximal d'invocations par groupe : " << workGroupInvocations << endl;

    /*
    // Paramètres optimisés pour RTX 4070
    const GLuint groupSizeX = 16; // 16x16 = 256 threads par groupe
    const GLuint groupSizeY = 16;
    const GLuint groupSizeZ = 1;

    // Calculez le nombre de groupes nécessaires pour couvrir votre problème
    // Supposons que nous travaillons sur une grille 2D de 8192x8192
    const GLuint numGroupsX = (8192 + groupSizeX - 1) / groupSizeX;
    const GLuint numGroupsY = (8192 + groupSizeY - 1) / groupSizeY;

    // Dispatch
    glDispatchCompute(numGroupsX, numGroupsY, 1);
    */
}
string opengl_info_display()
{
    std::string s;
    s += "[VENDOR]      : " + string((char*)(glGetString(GL_VENDOR))) + "\n";
    s += "[RENDERER]    : " + string((char*)(glGetString(GL_RENDERER))) + "\n";
    s += "[VERSION]     : " + string((char*)(glGetString(GL_VERSION))) + "\n";
    s += "[GLSL VERSION]: " + string((char*)(glGetString(GL_SHADING_LANGUAGE_VERSION))) + "\n";

    return s;
}
void gl_check_error()
{
#if defined(_DEBUG) || defined(DEBUG)
    GLint error = glGetError();
    if (error)
    {
        const char* errorStr = 0;
        switch (error)
        {
        case GL_INVALID_ENUM: errorStr = "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE: errorStr = "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: errorStr = "GL_INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY: errorStr = "GL_OUT_OF_MEMORY"; break;
        default: errorStr = "unknown error"; break;
        }
        printf("GL error : %s\n", errorStr);
        error = 0;
    }
#endif
}
static string opengl_error_to_string(GLenum error)
{
    switch (error)
    {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
#ifndef __EMSCRIPTEN__
    case GL_STACK_UNDERFLOW:
        return "GL_STACK_UNDERFLOW";
    case GL_STACK_OVERFLOW:
        return "GL_STACK_OVERFLOW";
#endif
    default:
        return "UNKNOWN";
    }
}
void check_opengl_error(string const& file, string const& function, int line)
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        string msg = "OpenGL ERROR detected\n"
            "\tFile " + file + "\n"
            "\tFunction " + function + "\n"
            "\tLine " + to_string(line) + "\n"
            "\tOpenGL Error: " + opengl_error_to_string(error);

        cout << msg << endl;
    }
}

wstring GetExecutablePath()
{
    wchar_t buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);

    wstring ws = filesystem::path(wstring(buffer)).parent_path().wstring();
    ws += L"\\";

    return ws;
}
string ReplaceBackSlash(string& chaine)
{
    string resultat = chaine;
    replace(resultat.begin(), resultat.end(), '\\', '/');
    return resultat;
}
string RemoveExtensionAndPath(const string pathname)
{
    int period = pathname.find_last_of(".");
    int dash = pathname.find_last_of("/");
    return pathname.substr(dash + 1, period - dash - 1);
}

uint32_t Log2OfPow2(uint32_t x)
{
    uint32_t ret = 0;

    while (x >>= 1)
        ++ret;

    return ret;
}

float MsToKnots(float speedMS)
{
    return speedMS * 3600.0f / 1852.0f;
}
float KnotsToMS(float speedKnots)
{
    return speedKnots * 1852.0f / 3600.0f;
}
float WindVec_Dir(vec2 windVector)
{
    // Extraire x et z du vecteur de vent
    float x = -windVector.x; // Inverser x pour échanger Est et Ouest
    float z = windVector.y;

    // Calculer l'angle en radians
    float angleRad = atan(x, z);

    // Convertir en degrés et ajuster pour le référentiel souhaité
    float angleDeg = mod(degrees(angleRad) + 180.0, 360.0);

    return angleDeg;
}
vec2 WindDirSpeed_Vec(float directionDEG, float speedKN)
{
    // Convertir la direction en radians
    float directionRad = radians(directionDEG);

    // Calculer les composantes x et y du vecteur
    float x = KnotsToMS(speedKN) * sin(directionRad);
    float y = KnotsToMS(-speedKN) * cos(directionRad);

    // Retourner le vecteur vent
    return vec2(x, y);
}

quat RotationBetweenVectors(vec3 A, vec3 B)
{
    A = glm::normalize(A);
    B = glm::normalize(B);

    float cosTheta = glm::dot(A, B);
    vec3 rotationAxis;

    if (cosTheta < -1 + 0.001f)
    {
        // Les vecteurs pointent dans des directions opposées
        rotationAxis = glm::cross(vec3(0.0f, 0.0f, 1.0f), A);
        if (glm::length2(rotationAxis) < 0.01f)
            rotationAxis = glm::cross(vec3(1.0f, 0.0f, 0.0f), A);
        rotationAxis = glm::normalize(rotationAxis);
        return glm::angleAxis(glm::radians(180.0f), rotationAxis);
    }

    rotationAxis = glm::cross(A, B);

    float s = sqrt((1 + cosTheta) * 2);
    float invs = 1 / s;

    return glm::quat(
        s * 0.5f,
        rotationAxis.x * invs,
        rotationAxis.y * invs,
        rotationAxis.z * invs
    );
}
float Sign(float value)
{
    if (value > 0.0f)
        return 1.0f;
    else if (value < 0.0f)
        return -1.0f;
    else
        return 0.0f;
}
double InterpolateAValue(const double start_1, const double end_1, const double start_2, const double end_2, double value_between_start_1_and_end_1) 
{
    // Normaliser la valeur entre start_1 et end_1
    double normalized = (value_between_start_1_and_end_1 - start_1) / (end_1 - start_1);

    // Interpoler vers l'intervalle [start_2, end_2]
    return start_2 + normalized * (end_2 - start_2);
}
bool IsInRect(vec4& rect, vec2& point)
{
    return (point.x >= rect.x && point.x <= rect.x + rect.z && point.y >= rect.y && point.y <= rect.y + rect.w);
}

const float EARTH_RADIUS = 6371000.0; // Mean radius of the Earth in meters
const vec2 REFERENCE_POINT(-2.94097114, 47.38162231); // Houat

vec2 LonLatToOpenGL(float lon, float lat)
{
    float dLon = glm::radians(lon - REFERENCE_POINT.x);
    float dLat = glm::radians(lat - REFERENCE_POINT.y);

    float x = EARTH_RADIUS * dLon * cos(glm::radians(REFERENCE_POINT.y));
    float z = -EARTH_RADIUS * dLat;

    return vec2(x, z);
}
vec2 OpenGLToLonLat(float x, float z)
{
    float lon = REFERENCE_POINT.x + glm::degrees(x / (EARTH_RADIUS * cos(glm::radians(REFERENCE_POINT.y))));
    float lat = REFERENCE_POINT.y - glm::degrees(z / EARTH_RADIUS);

    return vec2(lon, lat);
}
bool InterpolateTriangle(const vec3& p1, const vec3& p2, const vec3& p3, vec3& pos)
{
    // Calcul des coordonnées barycentriques
    float det = ((p2.z - p3.z) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.z - p3.z));
    float w1 = ((p2.z - p3.z) * (pos.x - p3.x) + (p3.x - p2.x) * (pos.z - p3.z)) / det;
    float w2 = ((p3.z - p1.z) * (pos.x - p3.x) + (p1.x - p3.x) * (pos.z - p3.z)) / det;
    float w3 = 1.0f - w1 - w2;

    // Vérification si le point est à l'intérieur du triangle
    if (w1 >= 0 && w2 >= 0 && w3 >= 0 && w1 <= 1 && w2 <= 1 && w3 <= 1)
    {
        // Interpolation de la valeur y
        pos.y = w1 * p1.y + w2 * p2.y + w3 * p3.y;
        return true;
    }
    else
    {
        // Le point est en dehors du triangle
        return false;
    }
}
vector<string> ListFiles(const string& folder, const string& ext)
{
    vector<string> files;
    for (const auto& entry : filesystem::directory_iterator(folder))
    {
        if (entry.path().extension() == ext)
            files.push_back(entry.path().string());
    }
    return files;
}
vec3 ConvertToFloat(vec3 v)
{
    return vec3((float)v.x / 255.0f, (float)v.y / 255.0f, (float)v.z / 255.0f);
}
void RGBtoHSL(const vec3& rgb, float& h, float& s, float& l) 
{
    float r = rgb.r / 255.0f;
    float g = rgb.g / 255.0f;
    float b = rgb.b / 255.0f;

    float max = std::max(std::max(r, g), b);
    float min = std::min(std::min(r, g), b);
    l = (max + min) / 2.0f;

    if (max == min) {
        h = s = 0.0f;
        return;
    }

    float d = max - min;
    s = (l > 0.5f) ? d / (2.0f - max - min) : d / (max + min);

    if (max == r)
        h = (g - b) / d + (g < b ? 6.0f : 0.0f);
    else if (max == g)
        h = (b - r) / d + 2.0f;
    else
        h = (r - g) / d + 4.0f;

    h /= 6.0f;
}

// Save client area to image file (png)
wstring GetNextAvailableCaptureName(const wstring& folderPath)
{
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    wstring searchPath = folderPath + L"\\SimWaves - Capture *.png";
    vector<int> numbers;

    // Lister les fichiers correspondant au motif
    hFind = FindFirstFileW(searchPath.c_str(), &ffd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                wstring filename(ffd.cFileName);
                // Extraire le numéro
                size_t pos = filename.rfind(L"Capture ");
                if (pos != wstring::npos)
                {
                    pos += 8; // "Capture "
                    wstring numStr = filename.substr(pos, filename.size() - pos - 4); // -4 pour .png
                    try
                    {
                        int num = std::stoi(numStr);
                        numbers.push_back(num);
                    }
                    catch (...) {}
                }
            }
        } while (FindNextFileW(hFind, &ffd) != 0);
        FindClose(hFind);
    }

    // Trouver le plus grand numéro existant
    int nextNum = 1;
    if (!numbers.empty())
    {
        std::sort(numbers.begin(), numbers.end());
        nextNum = numbers.back() + 1;
    }

    // Générer le nom de fichier
    wstringstream ss;
    ss << L"SimWaves - Capture " << std::setw(2) << std::setfill(L'0') << nextNum << L".png";
    return ss.str();
}
void SaveHBITMAP(HBITMAP bitmap, HDC hDC, wchar_t* filename)
{
    BITMAP				bmp;
    PBITMAPINFO			pbmi;
    WORD				cClrBits;
    HANDLE				hf;			// file handle 
    BITMAPFILEHEADER	hdr;		// bitmap file-header 
    PBITMAPINFOHEADER	pbih;		// bitmap info-header 
    LPBYTE				lpBits;		// memory pointer 
    DWORD				dwTotal;	// total count of bytes 
    DWORD				cb;			// incremental count of bytes 
    BYTE* hp;		// byte pointer 
    DWORD				dwTmp;

    // Create the bitmapinfo header information
    if (!GetObject(bitmap, sizeof(BITMAP), (LPSTR)&bmp))
    {
        perror("Could not retrieve bitmap info");
        return;
    }

    // Convert the color format to a count of bits. 
    cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
    if (cClrBits == 1)
        cClrBits = 1;
    else if (cClrBits <= 4)
        cClrBits = 4;
    else if (cClrBits <= 8)
        cClrBits = 8;
    else if (cClrBits <= 16)
        cClrBits = 16;
    else if (cClrBits <= 24)
        cClrBits = 24;
    else cClrBits = 32;

    // Allocate memory for the BITMAPINFO structure.
    if (cClrBits != 24)
        pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << cClrBits));
    else
        pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER));

    // Initialize the fields in the BITMAPINFO structure. 
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = bmp.bmWidth;
    pbmi->bmiHeader.biHeight = bmp.bmHeight;
    pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
    pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
    if (cClrBits < 24)
        pbmi->bmiHeader.biClrUsed = (1 << cClrBits);

    // If the bitmap is not compressed, set the BI_RGB flag. 
    pbmi->bmiHeader.biCompression = BI_RGB;

    // Compute the number of bytes in the array of color indices and store the result in biSizeImage. 
    pbmi->bmiHeader.biSizeImage = (pbmi->bmiHeader.biWidth + 7) / 8 * pbmi->bmiHeader.biHeight * cClrBits;
    // Set biClrImportant to 0, indicating that all of the device colors are important. 
    pbmi->bmiHeader.biClrImportant = 0;

    // Now open file and save the data
    pbih = (PBITMAPINFOHEADER)pbmi;
    lpBits = (LPBYTE)GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);

    if (!lpBits)
    {
        perror("SaveHBITMAP::Could not allocate memory");
        return;
    }

    // Retrieve the color table (RGBQUAD array) and the bits 
    if (!GetDIBits(hDC, HBITMAP(bitmap), 0, (WORD)pbih->biHeight, lpBits, pbmi, DIB_RGB_COLORS))
    {
        perror("SaveHBITMAP::GetDIB error");
        return;
    }

    // Create the .BMP file. 
    hf = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, (DWORD)0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
    if (hf == INVALID_HANDLE_VALUE)
    {
        perror("Could not create file for writing");
        return;
    }
    hdr.bfType = 0x4d42; // 0x42 = "B" 0x4d = "M" 
    // Compute the size of the entire file
    hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof(RGBQUAD) + pbih->biSizeImage);
    hdr.bfReserved1 = 0;
    hdr.bfReserved2 = 0;

    // Compute the offset to the array of color indices
    hdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof(RGBQUAD);

    // Copy the BITMAPFILEHEADER into the .BMP file 
    if (!WriteFile(hf, (LPVOID)&hdr, sizeof(BITMAPFILEHEADER), (LPDWORD)&dwTmp, NULL))
    {
        perror("Could not write in to file");
        return;
    }

    // Copy the BITMAPINFOHEADER and RGBQUAD array into the file
    if (!WriteFile(hf, (LPVOID)pbih, sizeof(BITMAPINFOHEADER) + pbih->biClrUsed * sizeof(RGBQUAD), (LPDWORD)&dwTmp, (NULL)))
    {
        perror("Could not write in to file");
        return;
    }

    // Copy the array of color indices into the .BMP file
    dwTotal = cb = pbih->biSizeImage;
    hp = lpBits;
    if (!WriteFile(hf, (LPSTR)hp, (int)cb, (LPDWORD)&dwTmp, NULL))
    {
        perror("Could not write in to file");
        return;
    }

    // Close the .BMP file
    if (!CloseHandle(hf))
    {
        perror("Could not close file");
        return;
    }

    // Free memory
    GlobalFree((HGLOBAL)lpBits);
}
void SaveClientArea(HWND hwnd)
{
    // Get a compatible DC into the client area
    HDC hDC = GetDC(hwnd);
    HDC hTargetDC = CreateCompatibleDC(hDC);

    RECT rect = { 0 };
    GetClientRect(hwnd, &rect);

    HBITMAP hBitmap = CreateCompatibleBitmap(hDC, rect.right - rect.left, rect.bottom - rect.top);
    SelectObject(hTargetDC, hBitmap);
    PrintWindow(hwnd, hTargetDC, PW_CLIENTONLY);

    wstring name = GetNextAvailableCaptureName(L"Outputs");
    name = L"Outputs/" + name;
    SaveHBITMAP(hBitmap, hTargetDC, const_cast<wchar_t*>(name.c_str()));

    DeleteObject(hBitmap);
    ReleaseDC(hwnd, hDC);
    DeleteDC(hTargetDC);
}
