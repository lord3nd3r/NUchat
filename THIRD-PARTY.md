# Third-party notices

NUchat itself is licensed under the GNU General Public License version 3.0
only. The full text is in [LICENSE](LICENSE). Source files do not carry
per-file SPDX headers. The macOS bundle copyright string is `GPL-3.0`.

This file records the other software NUchat links, the tools used only to
build installers, and the bits of the tree whose authorship is not spelled
out elsewhere. It is not a copy of those upstream licenses. Redistributors
should read the license that applies to each component.

## Linked at build time

| Component | How NUchat uses it | License | Notes |
|-----------|--------------------|---------|--------|
| Qt 6 (Core, Gui, Widgets, Network, Concurrent, Quick, QuickControls2) | Required. Linked into `nuchat`. | GNU LGPL version 3 (the open-source Qt distribution). Some Qt add-on modules are GPL-only; this client uses the LGPL modules listed above. | A GPL-3.0 program may link LGPL Qt. |
| Python 3 | Optional embedded interpreter (`PythonScriptEngine`). | Python Software Foundation License. | `find_package(Python3)` selects whichever Python 3 development files CMake finds. The embedded interpreter is tied to that ABI. The Windows CI job disables Python. |
| Lua 5.4 | Optional embedded interpreter (`LuaScriptEngine`). | MIT License. | Built when CMake's FindLua or pkg-config finds Lua. |
| Hunspell | Optional spell checking. | Tri-license: MPL 1.1, GNU GPL 2.0, and GNU LGPL 2.1. | Dictionaries are system files with their own licenses. NUchat only opens `/usr/share/hunspell/<lang>.dic` and the matching `.aff` file. |

Qt, Python, Lua, and Hunspell are not vendored in this repository.

### Qt in Windows and macOS binaries

The Windows installer and the macOS disk image produced by CI bundle Qt
shared libraries next to the application. The LGPL requires that people who
ship those binaries also ship Qt's license and either Qt's corresponding
source or a written offer for it, and that users be able to relink against
a modified Qt. This source tree does not contain Qt's source and does not
itself make that offer. Anyone who redistributes a NUchat binary that
contains Qt has to satisfy those terms.

The Linux and FreeBSD CI artifacts are a single `nuchat` executable. They
link the Qt installed on the build machine and do not bundle it.

## Build and CI tools

These are not linked into `nuchat`.

| Tool | Role | License note |
|------|------|----------------|
| CMake | Build system | BSD 3-Clause. |
| NSIS | Windows installer generator in CI | NSIS license (zlib-like, with additional terms for some compression libraries). |
| aqtinstall | Downloads Qt in the Windows and macOS workflows | MIT. |
| GitHub Actions (`actions/checkout`, `actions/cache`, `actions/setup-python`, `actions/upload-artifact`, `jurplel/install-qt-action`, `vmactions/freebsd-vm`) | CI only | Each action has its own license. Workflows pin them by commit SHA. |

## Themes

The 25 themes live as color tables in `qml/ThemeManager.qml`. They were
written for this project. Several are named after well-known palettes
(Monokai, Dracula, Nord, Gruvbox, Catppuccin, and others). Those names
identify the palette. This repository does not include those projects'
source files, theme packages, or license texts, and it does not claim to
ship an official port.

## Icons

`resources/icons/nuchat.svg` and the PNG sizes next to it have no separate
copyright or license header. They are part of this GPL-3.0 tree. Nothing in
the repository identifies a third-party icon set.

## What this repository does not include

- A software bill of materials.
- A single pinned Qt version. CI uses the distro Qt on Linux and FreeBSD,
  Qt 6.5.3 on Windows, and Qt 6.8.3 on macOS. See the workflows under
  `.github/workflows/` and the build notes in [README.md](README.md).
