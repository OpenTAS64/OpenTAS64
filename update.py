#!/usr/bin/env python3
"""Update an existing OpenTAS64 source installation without reinstalling dependencies."""
from __future__ import annotations

import os
import platform
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / "build"
LAYOUT_FILE = ROOT / "imgui.ini"
APP_CANDIDATES = [
    BUILD / "OpenTAS64",
    BUILD / "Release" / "OpenTAS64.exe",
    BUILD / "Debug" / "OpenTAS64.exe",
]
PLUGIN_PATTERN = "mupen64plus-input-opentas64.*"


class UpdateError(RuntimeError):
    pass


def run(args: list[str], *, cwd: Path = ROOT, capture: bool = False) -> subprocess.CompletedProcess:
    print("\n> " + subprocess.list2cmdline([str(item) for item in args]))
    try:
        result = subprocess.run(
            [str(item) for item in args],
            cwd=cwd,
            check=True,
            text=True,
            capture_output=capture,
        )
    except FileNotFoundError as exc:
        raise UpdateError(f"Required command not found: {args[0]}") from exc
    except subprocess.CalledProcessError as exc:
        detail = (exc.stderr or exc.stdout or "").strip() if capture else ""
        suffix = f"\n{detail}" if detail else ""
        raise UpdateError(f"Command failed ({exc.returncode}): {args[0]}{suffix}") from exc
    return result


def detect_os() -> str:
    system = platform.system()
    if system == "Windows":
        return "Windows"
    if system == "Darwin":
        return "macOS"
    if system == "Linux":
        return "Linux"
    return system or "Unknown"


def confirm() -> bool:
    print("OpenTAS64 In-Place Updater")
    print("=========================")
    print(f"Detected system: {detect_os()}")
    print("This updates the existing source checkout and rebuilds OpenTAS64.")
    print("It does NOT reinstall system dependencies or reset your projects.")
    print("Your imgui.ini window layout will be backed up and restored.")
    while True:
        answer = input("\nContinue? [Y/N]: ").strip().lower()
        if answer in ("y", "yes"):
            return True
        if answer in ("n", "no"):
            return False
        print("Please enter Y or N.")


def verify_checkout() -> None:
    if not (ROOT / ".git").exists():
        raise UpdateError("Run update.py from inside the existing OpenTAS64 Git checkout.")
    result = run(["git", "remote", "get-url", "origin"], capture=True)
    remote = result.stdout.strip().lower().removesuffix(".git").rstrip("/")
    if not (remote.endswith("/opentas64/opentas64") or remote.endswith(":opentas64/opentas64")):
        raise UpdateError(f"This checkout does not appear to be OpenTAS64/OpenTAS64 (origin: {result.stdout.strip()}).")


def check_worktree() -> None:
    result = run(["git", "status", "--porcelain", "--untracked-files=no"], capture=True)
    unexpected = []
    for line in result.stdout.splitlines():
        path = line[3:].strip()
        if path == "imgui.ini":
            continue
        unexpected.append(line)
    if unexpected:
        details = "\n".join(unexpected)
        raise UpdateError(
            "There are modified tracked source files. The updater will not overwrite them.\n"
            f"{details}\nCommit or back up those changes, then run the updater again."
        )


def tracked_layout_backup(temp_dir: Path) -> tuple[Path | None, bool]:
    """Back up the ImGui layout, which is tracked in older OpenTAS64 revisions."""
    if not LAYOUT_FILE.exists():
        return None, False
    backup = temp_dir / "imgui.ini.backup"
    shutil.copy2(LAYOUT_FILE, backup)
    return backup, True


def restore_layout(backup: Path | None, existed: bool) -> None:
    if existed and backup and backup.exists():
        shutil.copy2(backup, LAYOUT_FILE)
        print("Restored your imgui.ini window layout.")
    elif not existed and LAYOUT_FILE.exists():
        # Preserve the user's previous choice to have no local layout file.
        LAYOUT_FILE.unlink()
        print("Preserved the absence of a local imgui.ini file.")


def binary_paths() -> list[Path]:
    paths = [path for path in APP_CANDIDATES if path.is_file()]
    if BUILD.exists():
        paths.extend(path for path in BUILD.rglob(PLUGIN_PATTERN) if path.is_file())
    # Keep paths unique while retaining order.
    return list(dict.fromkeys(paths))


def backup_binaries(temp_dir: Path) -> dict[Path, Path]:
    backups: dict[Path, Path] = {}
    for index, original in enumerate(binary_paths()):
        backup = temp_dir / f"binary-{index}-{original.name}"
        shutil.copy2(original, backup)
        backups[original] = backup
    return backups


def restore_binaries(backups: dict[Path, Path]) -> None:
    for original, backup in backups.items():
        if backup.exists():
            original.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(backup, original)
    if backups:
        print("Restored the previous executable/plugin files after the failed update.")


def build_and_test() -> None:
    if not shutil.which("cmake"):
        raise UpdateError("CMake is missing. Install it manually; the updater never installs system dependencies.")
    if not shutil.which("ctest"):
        raise UpdateError("CTest is missing. Install CMake properly, then retry.")

    config = [
        "cmake", "-S", ".", "-B", "build",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DBUILD_TESTING=ON",
    ]
    cache = BUILD / "CMakeCache.txt"
    vcpkg_toolchain = ROOT / "tools" / "vcpkg" / "scripts" / "buildsystems" / "vcpkg.cmake"
    if platform.system() == "Windows" and not cache.exists() and vcpkg_toolchain.exists():
        config.append(f"-DCMAKE_TOOLCHAIN_FILE={vcpkg_toolchain}")

    run(config)
    workers = max(2, min(os.cpu_count() or 2, 8))
    run(["cmake", "--build", "build", "--config", "Release", "--parallel", str(workers)])
    run(["ctest", "--test-dir", "build", "-C", "Release", "--output-on-failure"])


def main() -> int:
    try:
        verify_checkout()
        check_worktree()
        if not confirm():
            print("Update cancelled. No changes were made.")
            return 0
    except EOFError:
        print("\nUpdate cancelled. No changes were made.")
        return 0
    except KeyboardInterrupt:
        print("\nUpdate cancelled. No changes were made.")
        return 130
    except UpdateError as exc:
        print(f"\nUPDATE ABORTED: {exc}", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory(prefix="opentas64-update-") as temporary:
        temp_dir = Path(temporary)
        layout_backup, layout_existed = tracked_layout_backup(temp_dir)
        old_binaries = backup_binaries(temp_dir)
        update_succeeded = False

        try:
            # The layout file was tracked by earlier revisions. Temporarily restore
            # the repository copy so Git can fast-forward, then put the user's copy back.
            if layout_existed:
                run(["git", "checkout", "--", "imgui.ini"])

            run(["git", "pull", "--ff-only", "origin", "main"])
            run(["git", "submodule", "update", "--init", "--recursive"])
            build_and_test()
            update_succeeded = True
        except (UpdateError, KeyboardInterrupt) as exc:
            print(f"\nUPDATE FAILED: {exc}", file=sys.stderr)
            restore_binaries(old_binaries)
            if isinstance(exc, KeyboardInterrupt):
                print("The update was interrupted.", file=sys.stderr)
                return_code = 130
            else:
                print("Your previous executable was kept where possible.", file=sys.stderr)
                return_code = 1
        else:
            print("\nOpenTAS64 was updated and the tests passed.")
            return_code = 0
        finally:
            restore_layout(layout_backup, layout_existed)

    if update_succeeded:
        print("Your .otas projects and other untracked user files were left untouched.")
        print("Launch OpenTAS64 with: ./build/OpenTAS64 (Linux/macOS)")
        if platform.system() == "Windows":
            print("Launch OpenTAS64 with: .\\build\\Release\\OpenTAS64.exe")
    return return_code


if __name__ == "__main__":
    raise SystemExit(main())
