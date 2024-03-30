# gfxbox2, a C++ Computer Graphics Sandbox

[Original document location](https://jausoft.com/cgit/cs_class/gfxbox2.git/about/).

## Git Repository
This project's canonical repositories is hosted on [Gothel Software](https://jausoft.com/cgit/cs_class/gfxbox2.git/).

## Goals
This project provides a C++ sandbox for computer graphics for our computer science class.

Its examples demonstrate basic usage and physics simulations.

## Supported Platforms
C++20 and better where the [SDL2 library](https://www.libsdl.org/) is supported, 
optionally SDL2 with [emscripten](https://emscripten.org/) or [SFML library](https://www.sfml-dev.org/).

## Online WebAssembly Examples
* [freefall01](https://jausoft.com/projects/gfxbox2/freefall01.html)
* [pong01](https://jausoft.com/projects/gfxbox2/pong01.html)
* [piviz](https://jausoft.com/projects/gfxbox2/piviz.html)
* [spacewars](https://jausoft.com/projects/gfxbox2/spacewars.html)
* [sandbox01](https://jausoft.com/projects/gfxbox2/sandbox01.html)

## Building Binaries

### Build Dependencies
- CMake 3.13+ but >= 3.18 is recommended
- C++20 compiler
  - gcc >= 10
  - clang >= 15
- [SDL2 library](https://www.libsdl.org/) 
- [emscripten](https://emscripten.org/) **optional** for WebAssembly
- [SFML library](https://www.sfml-dev.org/) **optional**
- Example funcdraw
    - Parser generator
        - [bison >= 3.2](https://www.gnu.org/software/bison/manual/) 
    - Lexer generator
        - [flex](https://github.com/westes/flex)
- Optional for `lint` validation
  - clang-tidy >= 15
- Optional for `vscodium` integration
  - clangd >= 15
  - clang-tools >= 15
  - clang-format >= 15


Installing build dependencies on Debian (11 or better):
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
apt install git
apt install build-essential g++ gcc libc-dev libpthread-stubs0-dev 
apt install clang-15 clang-tidy-15 clangd-15 clang-tools-15 clang-format-15
apt install cmake cmake-extras extra-cmake-modules pkg-config
apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev
apt install libsfml-dev
apt install bison flex
apt install doxygen graphviz
apt install emscripten
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Perhaps change the clang version-suffix of above clang install line to the appropriate version.

After complete clang installation, you might want to setup the latest version as your default.
For Debian you can use this [clang alternatives setup script](https://jausoft.com/cgit/cs_class/gfxbox2.git/tree/scripts/setup_clang_alternatives.sh).

### Build Procedure
The following is covered with [a convenient build script](https://jausoft.com/cgit/cs_class/gfxbox2.git/tree/scripts/build.sh).

For a generic build use:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
CPU_COUNT=`getconf _NPROCESSORS_ONLN`
git clone --recurse-submodule git://jausoft.com/srv/scm/cs_class/gfxbox2.git
cd gfxbox2
mkdir build
cd build
cmake ..
make -j $CPU_COUNT install doc
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Our cmake configure has a number of options, *cmake-gui* or *ccmake* can show
you all the options. The interesting ones are detailed below:

Changing install path from /usr/local to /usr
~~~~~~~~~~~~~
-DCMAKE_INSTALL_PREFIX=/usr
~~~~~~~~~~~~~

Building debug build:
~~~~~~~~~~~~~
-DDEBUG=ON
~~~~~~~~~~~~~

Building with clang and clang-tidy `lint` validation
~~~~~~~~~~~~~
-DCMAKE_C_COMPILER=/usr/bin/clang 
-DCMAKE_CXX_COMPILER=/usr/bin/clang++ 
-DCMAKE_CXX_CLANG_TIDY=/usr/bin/clang-tidy;-p;$rootdir/$build_dir
~~~~~~~~~~~~~

Building examples with SFML
~~~~~~~~~~~~~
-DUSE_SFML=ON
~~~~~~~~~~~~~

To build documentation run: 
~~~~~~~~~~~~~
make doc
~~~~~~~~~~~~~

### IDE Integration

#### Eclipse 
IDE integration configuration files are provided for 
- [Eclipse](https://download.eclipse.org/eclipse/downloads/) with extensions
  - [CDT](https://github.com/eclipse-cdt/) or [CDT @ eclipse.org](https://projects.eclipse.org/projects/tools.cdt)
  - `CMake Support`, install `C/C++ CMake Build Support` with ID `org.eclipse.cdt.cmake.feature.group`

You can import the project to your workspace via `File . Import...` and `Existing Projects into Workspace` menu item.

For Eclipse one might need to adjust some setting in the `.project` and `.cproject` (CDT) 
via Eclipse settings UI, but it should just work out of the box.

#### VSCodium or VS Code

IDE integration configuration files are provided for 
- [VSCodium](https://vscodium.com/) or [VS Code](https://code.visualstudio.com/) with extensions
  - [vscode-clangd](https://github.com/clangd/vscode-clangd)
  - [twxs.cmake](https://github.com/twxs/vs.language.cmake)
  - [ms-vscode.cmake-tools](https://github.com/microsoft/vscode-cmake-tools)
  - [notskm.clang-tidy](https://github.com/notskm/vscode-clang-tidy)
  - [cschlosser.doxdocgen](https://github.com/cschlosser/doxdocgen)
  - [jerrygoyal.shortcut-menu-bar](https://github.com/GorvGoyl/Shortcut-Menu-Bar-VSCode-Extension)

For VSCodium one might copy the [example root-workspace file](https://jausoft.com/cgit/cs_class/gfxbox2.git/tree/.vscode/gfxbox2.code-workspace_example)
to the parent folder of this project (*note the filename change*) and adjust the `path` to your filesystem.
~~~~~~~~~~~~~
cp .vscode/gfxbox2.code-workspace_example ../gfxbox2.code-workspace
vi ../gfxbox2.code-workspace
~~~~~~~~~~~~~
Then you can open it via `File . Open Workspace from File...` menu item.
- All listed extensions are referenced in this workspace file to be installed via the IDE
- The [local settings.json](.vscode/settings.json) has `clang-tidy` enabled
  - If using `clang-tidy` is too slow, just remove it from the settings file.
  - `clangd` will still contain a good portion of `clang-tidy` checks

