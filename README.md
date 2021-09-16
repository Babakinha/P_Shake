# P_Shake
Simple Shake effect using Perlin noise and OpenFX

# Instalation

## Getting the repo from github 
Just make sure you have `git` installed;
```
git clone https://github.com/Babakinha/P_Shake.git
cd P_Shake
git submodule update -i -r
```
Done! now just compile it to your operating system;
### ([Windows](#compiling-for-windows), [Unix](#compiling-for-linuxunix-based-os-x-freebsd-ubuntu-etc))

## Compiling for Linux/Unix Based (OS X, FreeBSD, Ubuntu etc.)
On Unix-like systems, the plugins can be compiled by typing in a terminal:
* `make [options]` to compile as a single combined plugin (see below for valid options).
* `make [options] CXXFLAGS_ADD=-fopenmp LDFLAGS_ADD=-fopenmp` to compile with OpenMP support (available for CImg-based plugins and DenoiseSharpen).
The most common options are `CONFIG=release` to compile a release version, `CONFIG=debug` to compile a debug version. Or `CONFIG=relwithdebinfo` to compile an optimized version with debugging symbols.

Another common option is `BITS=32` for compiling a 32-bits version, `BITS=64` for a 64-bits version, and `BITS=Universal` for a universal binary (OS X only).

The compiled plugins are placed in subdirectories named after the configuration,
for example Linux-64-realease for a 64-bits Linux compilation.
In each of these directories, a `*.bundle` directory is created, which has to be moved to the proper place (`/usr/OFX/Plugins` on Linux, or `/Library/OFX/Plugins` on OS X)

So you can just the command `make [options] && make install` to compile and move the plugin folder for your OS's OFX/Plugins folder

*And for you copy pasters just use this:*
```
sudo make CONFIG=release BITS=64 && sudo make install CONFIG=release BITS=64
```

## Compiling for Windows
bruh

# Sources
* [OpenFX(Natrons fork)](https://github.com/NatronGitHub/openfx)
* [The Base of the OpenFX plugin](https://github.com/NatronGitHub/openfx-misc/blob/master/Position/Position.cpp)
* [Really cool OpenFX Guide i found](https://github.com/MrKepzie/Natron/wiki/OpenFX-plugin-programming-guide-(Invert-plugin-walkthrough))
&nbsp;
* [The Perlin Noise implementation](https://github.com/Reputeless/PerlinNoise/)
* [The Random Unity Camera Shake algorithm that i used](https://forum.unity.com/threads/using-mathf-perlinnoise-for-camera-shake.208456/)
