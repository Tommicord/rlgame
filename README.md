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

# Example

Tested on a old PC with Intel HD Graphics 530, the game is able to render the world and units using Vulkan. The following screenshot shows the game running on this hardware without lag:

![Test Unit render graphics on Intel HD Graphics 530](Public/Examples/UnitRenderExample.png)

# What is already implemented
- Chunk system (only generating the noise values in the GPU) with procedural generation using Simplex noise
- Rendering of units using Vulkan
- Skybox, Time system with lighting and day/night cycle

# What is not implemented yet
- Multiplayer support
- Spacecraft building
- Space exploration
- More biomes, creatures, and resources to discover
- Physics system for interactions with the world
- Production-ready implementation (currently in development and testing phase)

# Why this project exists?
I made this project not only for learning, but also to create a game that is lightweight and can run on a wide range of devices, including older hardware. I wanted to create a game that is easy to run and doesn't require high-end graphics cards or processors. The goal is to make the game accessible to as many people as possible, while still providing an engaging and fun experience.