# OpenTAS64 Installation

## Automatic setup

Run the Python installer from the repository root:

```bash
python3 install.py
```

On Windows:

```powershell
py install.py
```

It detects the operating system and asks for confirmation before making changes. After confirmation it installs supported build dependencies, initializes Git submodules, builds OpenTAS64 and its Mupen64Plus input plugin, and runs the tests.

## Supported systems

| System | Tools | Installed requirements |
|---|---|---|
| Ubuntu / Debian | APT | build-essential, CMake, Git, SDL2 development files |
| Fedora | DNF | GCC C++, Make, CMake, Git, SDL2 development files |
| Arch Linux | Pacman | base-devel, CMake, Git, SDL2 |
| openSUSE | Zypper | GCC C++, Make, CMake, Git, SDL2 development files |
| macOS | Homebrew + Xcode Command Line Tools | CMake, Git, SDL2 |
| Windows 10/11 | winget + Visual Studio C++ Build Tools + vcpkg | MSVC, CMake, Git, SDL2 |

The installer may request administrator permission to install system packages. It does not download ROMs or game data. The installer has been statically reviewed and exercised by the repository CI build, but not manually tested on every listed operating system.

## Platform notes

- macOS requires Homebrew and Xcode Command Line Tools. If the command-line tools are missing, the installer explains how to install them and exits.
- Windows requires winget. The Visual Studio Build Tools installer may require a restart or a new terminal session before the compiler is available.
- Windows SDL2 is installed with vcpkg for the x64 toolchain.
- Unsupported Linux package managers are not guessed; the script stops and lists the manual prerequisites.
- Re-running the script is supported.
- This builds the software from source. It does not package a signed native Windows EXE or macOS DMG installer.

## Troubleshooting

- If Windows reports that Git or CMake is not available immediately after installation, close the terminal, open a new one, and run the installer again. Some Windows package changes do not update the current process environment.
- If the Visual Studio Build Tools installer requests a restart, complete that restart before rerunning the setup.
- On Linux, the installer uses `sudo` for system packages; make sure the account can install packages.
- If a dependency installation or build is interrupted, rerun the installer. Package managers and the CMake build directory are designed to continue from an already partially completed setup.
- If CMake cannot find SDL2, install the SDL2 development package for the current distribution and rerun the installer.

## Manual build

Install a C++17 compiler, CMake 3.16+, Git, and SDL2 development files, then run:

```bash
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```
