# DragonBurn Internal (CS2)

Internal Direct3D 11 implementation of DragonBurn for Counter-Strike 2, featuring an ImGui-based user interface and Steam overlay hooking.

### 📌 References & Credits
This project is an internal port and redesign built with reference to the following open-source projects:
* **[ByteCorum / DragonBurn](https://github.com/ByteCorum/DragonBurn)** – Original base logic, game structures, and core features.
* **[Cl1cker0 / St-Mbappe](https://github.com/Cl1cker0/St-Mbappe)** – Reference for the internal architecture, DirectX 11 overlay hook, and custom UI design.

##Features

* **Direct3D 11 Hook:** Integrated via Steam's `GameOverlayRenderer64.dll`.
* **ImGui Modern UI:** Custom styled interface with tab navigation and themes.
* **Visuals & ESP:**
  * Box ESP (Corners, Full, Dashed)
  * Skeleton & Chams rendering
  * Health bar, Player names & Held weapon
  * C4 timer & Dropped C4 ESP
  * In-game Radar & Spectator list
* **Grenade Helper:** Trajectory calculation and interactive throw assists.
* **Aimbot & Triggerbot:** Configurable bone selection, FOV circle, and smoothness.

---

##Building from Source

### Prerequisites & Dependencies
* **Operating System:** Windows 10 / 11 (64-bit)
* **Build Tools:** Visual Studio 2022 (with *Desktop development with C++*)
* **SDKs & Libraries:**
  * Windows 10/11 SDK (Latest version)
  * DirectX 11 SDK / Runtimes (`d3d11.dll`, `dxgi.dll`)
* **Runtime Requirements:**
  * Visual C++ Redistributable (x64) 2015–2022 (e.g. `vcruntime140.dll`, `msvcp140.dll`)
  * Steam Client running (with In-Game Overlay enabled for `GameOverlayRenderer64.dll`)

## 🐛 Issues & Bug Reports

Please note that several features from the original project have been intentionally removed, while others have been fixed and reworked for this internal build.
If you encounter any bugs, crashes, broken features after a CS2 update, or simply want to complain about something, feel free to open an **[Issue](https://github.com/Muizz-129/DragonBurn-Internal/issues)**. 
To be completely honest, I'm currently too lazy to test and check every single detail by myself, so detailed bug reports, constructive complaints, and Pull Requests are always welcome!

##Upcoming Updates
Future updates will include an **auto-update offsets** system so the menu continues working seamlessly across new CS2 game patches without needing manual offset updates every time.
