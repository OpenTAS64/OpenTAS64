#!/usr/bin/env python3
"""Cross-platform dependency installer and builder for OpenTAS64."""
from __future__ import annotations
import os, platform, shutil, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / "build"
VCPKG = ROOT / "tools" / "vcpkg"

class SetupError(RuntimeError):
    pass

def run(args, cwd=ROOT):
    print("\n> " + subprocess.list2cmdline([str(x) for x in args]))
    try:
        subprocess.run([str(x) for x in args], cwd=cwd, check=True)
    except FileNotFoundError as exc:
        raise SetupError(f"Command not found: {args[0]}") from exc
    except subprocess.CalledProcessError as exc:
        raise SetupError(f"Command failed ({exc.returncode}): {args[0]}") from exc

def linux_info():
    data = {}
    try:
        for line in Path("/etc/os-release").read_text(encoding="utf-8").splitlines():
            if "=" in line:
                k, v = line.split("=", 1)
                data[k] = v.strip().strip('"')
    except OSError:
        pass
    return data.get("ID", "unknown").lower(), data.get("PRETTY_NAME", "Linux")

def detect():
    system = platform.system()
    if system == "Linux":
        _, name = linux_info()
        return "linux", name
    if system == "Windows":
        return "windows", f"Windows {platform.release()} ({platform.machine()})"
    if system == "Darwin":
        return "macos", f"macOS {platform.mac_ver()[0]} ({platform.machine()})"
    return "unsupported", f"{system} ({platform.machine()})"

def confirm(name):
    print("\nOpenTAS64 Setup\n================")
    print(f"Detected operating system: {name}")
    print("\nIs this your operating system?")
    print("  [Y] Yes, install dependencies and build OpenTAS64")
    print("  [N] No, cancel setup")
    while True:
        answer = input("\nChoose Y or N: ").strip().lower()
        if answer in ("y", "yes"): return True
        if answer in ("n", "no"): return False
        print("Please answer Y or N.")

def install_linux():
    distro, _ = linux_info()
    if shutil.which("apt-get"):
        run(["sudo","apt-get","update"])
        run(["sudo","apt-get","install","-y","build-essential","cmake","git","libsdl2-dev","pkg-config"])
    elif shutil.which("dnf"):
        run(["sudo","dnf","install","-y","gcc-c++","make","cmake","git","SDL2-devel","pkgconf-pkg-config"])
    elif shutil.which("pacman"):
        run(["sudo","pacman","-Sy","--needed","--noconfirm","base-devel","cmake","git","sdl2","pkgconf"])
    elif shutil.which("zypper"):
        run(["sudo","zypper","--non-interactive","install","gcc-c++","make","cmake","git","libSDL2-devel","pkg-config"])
    else:
        raise SetupError(f"Unsupported Linux package manager ({distro}). Install a C++17 compiler, CMake 3.16+, Git, and SDL2 development files manually.")

def install_macos():
    if not shutil.which("brew"):
        raise SetupError("Homebrew is required. Install it from https://brew.sh and rerun this script.")
    if subprocess.run(["xcode-select","-p"],capture_output=True).returncode != 0:
        raise SetupError("Xcode Command Line Tools are required. Run 'xcode-select --install', finish installation, then rerun.")
    run(["brew","update"])
    run(["brew","install","cmake","sdl2","git"])

def refresh_windows_path():
    """Make newly installed tools visible to this running Python process."""
    candidates = [
        Path(os.environ.get("ProgramFiles", r"C:\\Program Files")) / "Git" / "cmd",
        Path(os.environ.get("ProgramFiles", r"C:\\Program Files")) / "CMake" / "bin",
        Path(os.environ.get("LOCALAPPDATA", "")) / "Programs" / "Git" / "cmd",
        Path(os.environ.get("LOCALAPPDATA", "")) / "Programs" / "CMake" / "bin",
    ]
    current = os.environ.get("PATH", "").split(os.pathsep)
    additions = [str(p) for p in candidates if p.is_dir() and str(p) not in current]
    if additions:
        os.environ["PATH"] = os.pathsep.join(additions + current)

def install_windows():
    if not shutil.which("winget"):
        raise SetupError("winget is required. Install/update 'App Installer' from Microsoft Store, then rerun.")
    if not (shutil.which("cl") or os.environ.get("VisualStudioVersion")):
        run(["winget","install","--id","Microsoft.VisualStudio.2022.BuildTools","--exact","--accept-package-agreements","--accept-source-agreements","--override","--wait --quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"])
    if not shutil.which("cmake"):
        run(["winget","install","--id","Kitware.CMake","--exact","--accept-package-agreements","--accept-source-agreements"])
    if not shutil.which("git"):
        run(["winget","install","--id","Git.Git","--exact","--accept-package-agreements","--accept-source-agreements"])
    refresh_windows_path()
    if not shutil.which("git") or not shutil.which("cmake"):
        raise SetupError("Git or CMake was just installed. Restart your terminal and run py install.py again so Windows can refresh PATH.")
    if not VCPKG.exists():
        VCPKG.parent.mkdir(parents=True, exist_ok=True)
        run(["git","clone","https://github.com/microsoft/vcpkg.git",str(VCPKG)])
    run(["cmd","/c",str(VCPKG/"bootstrap-vcpkg.bat"),"-disableMetrics"])
    run([str(VCPKG/"vcpkg.exe"),"install","sdl2:x64-windows"])
    return VCPKG/"scripts"/"buildsystems"/"vcpkg.cmake"

def main():
    kind, name = detect()
    if kind == "unsupported":
        print(f"Automatic setup does not support {name}.")
        return 1
    try:
        accepted = confirm(name)
    except EOFError:
        print("\nSetup cancelled before installation. No changes were made.")
        return 0
    except KeyboardInterrupt:
        print("\nSetup cancelled before installation. No changes were made.")
        return 130
    if not accepted:
        print("Setup cancelled. No changes were made.")
        return 0
    try:
        toolchain = None
        if kind == "linux": install_linux()
        elif kind == "macos": install_macos()
        else: toolchain = install_windows()
        if not shutil.which("git"):
            raise SetupError("Git was not found after dependency setup. Restart your terminal and rerun.")
        run(["git","submodule","update","--init","--recursive"])
        if not shutil.which("cmake"):
            raise SetupError("CMake was not found. Restart your terminal after installing it.")
        cfg = ["cmake","-S",".","-B","build","-DCMAKE_BUILD_TYPE=Release","-DBUILD_TESTING=ON"]
        if toolchain: cfg.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain}")
        run(cfg)
        run(["cmake","--build","build","--config","Release","--parallel","2"])
        run(["ctest","--test-dir","build","-C","Release","--output-on-failure"])
    except SetupError as exc:
        print(f"\nSETUP FAILED: {exc}", file=sys.stderr)
        print("Resolve the requirement above and run the installer again.", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("\nSetup interrupted. Run the installer again to continue.", file=sys.stderr)
        return 130
    print("\nOpenTAS64 built successfully.")
    print(f"Build directory: {BUILD}")
    print("Launch: build\\Release\\OpenTAS64.exe" if kind == "windows" else "./build/OpenTAS64")
    print("The Mupen64Plus input plugin was built alongside the application.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
