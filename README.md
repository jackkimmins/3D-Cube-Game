# C++ WASM 3D-Cube-Game
3D-Cube-Game is a browser-based 3D block game developed using C++ and WebAssembly, compiled through Emscripten. This project was an exploration into SDL2 as the primary graphics library, which provided both unique challenges and learning opportunities into low-level graphics rendering.

## Features
Features of this project include:
 - Random World Generation (Perlin Noise for procedural terrain creation)
- Supports multiple block types sourced from a texture atlas.
- The player can break and place blocks within the game world.
- Navigate using WASD keys and pan the view with the mouse.
- Fully functional in the web browser, utilising WebAssembly for cross-platform compatibility.

## Limitations
- **MAJOR:** There are substantial issues with block texture alignment.
- Performance and scalability is limited due to non-GPU-based rendering.
- Slight inconsistencies in block placement.

## Update: New Version
While SDL2 provided an interesting framework for this project, it brought significant limitations, particularly in performance and texture management.

I have since developed a newer version of this project using OpenGL and WebAssembly for improved scalability and GPU utilisation with WebGL2. [Check out the new version here.](#)
