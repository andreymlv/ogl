#!/usr/bin/env python3

import json
import pathlib
import platform
import subprocess
import sys

SOURCE_DIR = pathlib.Path(__file__).resolve().parent.parent


def build_vcpkg():
    with open(SOURCE_DIR / "vcpkg.json", "r") as f:
        vcpkg_json = json.load(f)

    git_repo = "https://github.com/microsoft/vcpkg.git"
    git_rev = vcpkg_json["builtin-baseline"]

    build_dir = SOURCE_DIR / "build"
    build_dir.mkdir(parents=True, exist_ok=True)
    vcpkg_checkout = build_dir / "vcpkg"

    if not vcpkg_checkout.is_dir():
        subprocess.check_call(args=["git", "clone", git_repo], cwd=build_dir)
    else:
        current_rev = (
            subprocess.check_output(["git", "-C", str(vcpkg_checkout), "rev-parse", "HEAD"])
            .strip()
            .decode()
        )
        if current_rev == git_rev:
            return

    print(f"Building vcpkg@{git_rev}")
    subprocess.check_call(args=["git", "fetch", "origin"], cwd=vcpkg_checkout)
    subprocess.check_call(args=["git", "checkout", git_rev], cwd=vcpkg_checkout)

    bootstrap = "bootstrap-vcpkg.bat" if platform.system() == "Windows" else "bootstrap-vcpkg.sh"
    subprocess.check_call(args=[vcpkg_checkout / bootstrap, "-disableMetrics"], cwd=vcpkg_checkout)


if __name__ == "__main__":
    build_vcpkg()
