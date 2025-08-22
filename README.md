# SimShip
Simulation of a ship moving on a cinematic-quality ocean. 

(https://www.youtube.com/watch?v=ywmwANNGrns&ab_channel=EdouardHalbert)

(https://sourceforge.net/projects/simship/files/SimShip-1.0_Setup.exe/download)

**PHILOSOPHY**
- No commercial or freeware 3D engine.
- Simulation written in C++ with OpenGL, GLFW, Glad, GLM, Open Asset Import Library (assimp), libigl, ImGui, OpenAL, NanoVG, stb, Eigen, FFTW3, Pugixml, libnova, Clipper2.
- Real time simulation with high rate of frames per second (target of 150 fps in fullscreen 2560 x 1440).
- The rendering of the 3D scene is optimized to ensure consistency of visual quality between all elements: ocean, ship, sky, clouds, terrain, mist or fog effects.


**OCEAN**

- Spectral wind wave model, based on Phillips spectrum.
- Dynamic PBR anisotropic BRDF.
- Foam simulation.
- Underwater view.
- Instanced patches defined by LOD settings.
- Host readbacks for in-game physics.
- Spectral analysis.

**WIND**

- Strength (1 to 30 knots) and direction.

**SKY**

- Dynamic sky (physical atmosphere) with moving volumetric clouds, godrays and bloom.
- Sun positioned according to date and time (day and night).
- Mist and Fog.

**SHIP**

- Full motion over 6 degrees of freedom (surge, sway, heave, yaw, pitch, roll).
- Ship motion forces (archimede, gravity, viscous resistance, wave resistance, residual resistance, wind drift, wind rotation, thuster, bow thruster, rudder).
- Propellers and radars animated.
- Reflection of the ship on the water.
- Wake simulation (Kelvin waves, foam and bubbles).
- Smoke simulated with particles.
- Navigation lights.
- Autopilot with Proportional-Integral-Derivative controller.

**ENVIRONMENT**

- Camera full smooth motion (orbital around the ship, fps, fixed views and free views on board the ship, binoculars).
- 3D sounds (engines, seagulls, horn).
- Terrain (island).
- Markup (buoys).

# Compilation

- Only c++ files and shaders are provided with some resource files. Compilation needs the installation of several librairies.

# License

- Creative Commons CC BY-NC. This license enables reusers to distribute, remix, adapt, and build upon the material in any medium or format for noncommercial purposes only, and only so long as attribution is given to the creator.
