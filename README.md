# RL Game

RL (Real Game) is a game written in C++ using Vulkan for graphics rendering. The game features a polygon-based world that players can explore and interact with. The game includes a variety of biomes, creatures, and resources to discover, and allows players to explore the space and build spacecrafts to explore the space and other planets. The coordinate system is based on a 3D grid, where each 1000 Units represents a unit in the grid (every 1000 = 1 Meter, represented using a 64-bit coordinate)

## Features
- Polygon-based world (or similar)
- Space exploration
- Spacecraft building
- Multiplayer support
- Cross-platform support (Windows, Linux, macOS)
- Lightweight and easy to run on a wide range of devices
- Written in C++ using Vulkan for graphics rendering (high performance)
- Open source and free to use

## Highly optimized for older devices

When tested the cheap ray-tracing, PBR, Cook-Torrance, Reflections, etc.
Run very fast on my computer IGPU: Intel HD Graphics 530
So it should run fast on most devices with a dedicated GPU or even old or new integrated graphics, this Videogame is designed to be lightweight and optimized for performance, ensuring a smooth gaming experience even on older hardware, doesn't require a powerful GPU or CPU to run

- Example of Unit render graphics on **Intel HD Graphics 530**

![Test Unit render graphics on Intel HD Graphics 530](Public/Examples/UnitRenderExample.png)