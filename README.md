# hdtSMP for Skyrim Special Edition

Fork of [original code](https://github.com/HydrogensaysHDT/hdt-skyrimse-mods) by hydrogensaysHDT

High Heels plugin has been removed, this repository only contains HDT-SMP.

## Changes 

+ build now works with unchanged skse & bullet source
+ support both "." and "," as decimal seperators in config files
+ properly remove tracked armors from physics world if the tracked skeleton isn't part of the active scene
+ write transforms during game pauses, fixes physics appearing to reset while game is paused
+ reset system on loading screens so there's no brief physics glitches when loading between areas
+ add a debug command to print some stats to console
+ removed dependency on hdtSSEFramework and removed it from the repository
+ rename plugin hdtSMP64 to differentiate from the old version that depends on Framework, and because we can support VR, not just SSE

## Build Instructions

Requires Visual Studio 2019 (Community Edition is fine)

1) Download the clean skse64 source from [skse's website](http://skse.silverlock.org/)
2) Extract or clone this repository into the skse64 folder of the source. You can safely delete skse64_loader, skse64_steam_loader, skse64_loader_common, and the skse64 solution.
3) Download and build [bullet physics](https://github.com/bulletphysics/bullet3) using cmake and Visual Studio. Update the hdtSMP64 project to point to your bullet source and built libraries. The Release configuration is intended to be built with Bullet's "Use MSVC AVX" cmake option enabled, and Release_noavx without.
4) Download and build [Microsoft's detours library](https://github.com/microsoft/Detours) using nmake. Update the hdtSMP64 project to point to your built library.
5) Build the solution :)

## Credits

hydrogensaysHDT - Creating this plugin

ousnius - fixes and updates

