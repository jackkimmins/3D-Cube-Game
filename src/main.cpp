#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <emscripten.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <tuple>
#include <chrono>
#include "PerlinNoise.hpp"
#include "MatrixSupports.hpp"
#include "WorldChunksBlocks.hpp"

// Screen Dimensions
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

// Keyboard State Array
bool keys[SDL_NUM_SCANCODES];

// Mouse Movements
int mouse_dx = 0, mouse_dy = 0;

// Mouse Buttons
bool leftMouseButtonDown = false;
bool rightMouseButtonDown = false;

// Constants
const float PI = 3.1415926535f;

// Camera Struct
struct Camera {
    Vec3 pos;
    Vec3 lookDir;
    float yaw;
    float pitch;
    float verticalVelocity;
    bool isOnGround;

    // View Bobbing Settings
    float bobbingTimer = 0.0f;
    float bobbingAmplitude = 0.05f;
    float bobbingFrequency = 10.0f;
    float bobbingOffsetY = 0.0f;
};

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* textureAtlas = nullptr;

Mesh meshCube;
Camera camera;
World world;

// SortedTriangle struct to store triangles with depth - this is used for painter's algorithm but it is not working correctly
struct SortedTriangle {
    Triangle tri;
    float depth;
    BlockType type;
    Vec3 faceNormal;
};

bool wireframeMode = false;
Vec3 selectedBlockPosition;
bool hasSelectedBlock = false;

// Initialise Cube Mesh
void InitCubeMesh() {
    // Vertices with corrected positions and default texture coordinates
    Vertex verts[] = {
        Vertex({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}), // 0 - Bottom-left corner (Front)
        Vertex({0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}), // 1 - Top-left corner (Front)
        Vertex({1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}), // 2 - Top-right corner (Front)
        Vertex({1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}), // 3 - Bottom-right corner (Front)
        Vertex({0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}), // 4 - Bottom-left corner (Back)
        Vertex({0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}), // 5 - Top-left corner (Back)
        Vertex({1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}), // 6 - Top-right corner (Back)
        Vertex({1.0f, 0.0f, 1.0f}, {1.0f, 1.0f})  // 7 - Bottom-right corner (Back)
    };

    // Front face (z=0)
    Face frontFace;
    frontFace.tris[0].v[0] = Vertex(verts[0].pos, {0.0f, 1.0f}); // Bottom-left
    frontFace.tris[0].v[1] = Vertex(verts[1].pos, {0.0f, 0.0f}); // Top-left
    frontFace.tris[0].v[2] = Vertex(verts[2].pos, {1.0f, 0.0f}); // Top-right

    frontFace.tris[1].v[0] = Vertex(verts[0].pos, {0.0f, 1.0f}); // Bottom-left
    frontFace.tris[1].v[1] = Vertex(verts[2].pos, {1.0f, 0.0f}); // Top-right
    frontFace.tris[1].v[2] = Vertex(verts[3].pos, {1.0f, 1.0f}); // Bottom-right
    frontFace.normal = {0.0f, 0.0f, -1.0f};

    // Right face (x=1)
    Face rightFace;
    rightFace.tris[0].v[0] = Vertex(verts[3].pos, {0.0f, 1.0f}); // Bottom-left
    rightFace.tris[0].v[1] = Vertex(verts[2].pos, {0.0f, 0.0f}); // Top-left
    rightFace.tris[0].v[2] = Vertex(verts[6].pos, {1.0f, 0.0f}); // Top-right

    rightFace.tris[1].v[0] = Vertex(verts[3].pos, {0.0f, 1.0f}); // Bottom-left
    rightFace.tris[1].v[1] = Vertex(verts[6].pos, {1.0f, 0.0f}); // Top-right
    rightFace.tris[1].v[2] = Vertex(verts[7].pos, {1.0f, 1.0f}); // Bottom-right
    rightFace.normal = {1.0f, 0.0f, 0.0f};

    // Back face (z=1)
    Face backFace;
    backFace.tris[0].v[0] = Vertex(verts[7].pos, {0.0f, 1.0f}); // Bottom-left
    backFace.tris[0].v[1] = Vertex(verts[6].pos, {0.0f, 0.0f}); // Top-left
    backFace.tris[0].v[2] = Vertex(verts[5].pos, {1.0f, 0.0f}); // Top-right

    backFace.tris[1].v[0] = Vertex(verts[7].pos, {0.0f, 1.0f}); // Bottom-left
    backFace.tris[1].v[1] = Vertex(verts[5].pos, {1.0f, 0.0f}); // Top-right
    backFace.tris[1].v[2] = Vertex(verts[4].pos, {1.0f, 1.0f}); // Bottom-right
    backFace.normal = {0.0f, 0.0f, 1.0f};

    // Left face (x=0)
    Face leftFace;
    leftFace.tris[0].v[0] = Vertex(verts[4].pos, {0.0f, 1.0f}); // Bottom-left
    leftFace.tris[0].v[1] = Vertex(verts[5].pos, {0.0f, 0.0f}); // Top-left
    leftFace.tris[0].v[2] = Vertex(verts[1].pos, {1.0f, 0.0f}); // Top-right

    leftFace.tris[1].v[0] = Vertex(verts[4].pos, {0.0f, 1.0f}); // Bottom-left
    leftFace.tris[1].v[1] = Vertex(verts[1].pos, {1.0f, 0.0f}); // Top-right
    leftFace.tris[1].v[2] = Vertex(verts[0].pos, {1.0f, 1.0f}); // Bottom-right
    leftFace.normal = {-1.0f, 0.0f, 0.0f};

    // Top face (y=1)
    Face topFace;
    topFace.tris[0].v[0] = Vertex(verts[1].pos, {0.0f, 0.0f}); // Top-left
    topFace.tris[0].v[1] = Vertex(verts[5].pos, {0.0f, 1.0f}); // Bottom-left
    topFace.tris[0].v[2] = Vertex(verts[6].pos, {1.0f, 1.0f}); // Bottom-right

    topFace.tris[1].v[0] = Vertex(verts[1].pos, {0.0f, 0.0f}); // Top-left
    topFace.tris[1].v[1] = Vertex(verts[6].pos, {1.0f, 1.0f}); // Bottom-right
    topFace.tris[1].v[2] = Vertex(verts[2].pos, {1.0f, 0.0f}); // Top-right
    topFace.normal = {0.0f, 1.0f, 0.0f};

    // Bottom face (y=0)
    Face bottomFace;
    bottomFace.tris[0].v[0] = Vertex(verts[4].pos, {0.0f, 1.0f}); // Bottom-left
    bottomFace.tris[0].v[1] = Vertex(verts[0].pos, {0.0f, 0.0f}); // Top-left
    bottomFace.tris[0].v[2] = Vertex(verts[3].pos, {1.0f, 0.0f}); // Top-right

    bottomFace.tris[1].v[0] = Vertex(verts[4].pos, {0.0f, 1.0f}); // Bottom-left
    bottomFace.tris[1].v[1] = Vertex(verts[3].pos, {1.0f, 0.0f}); // Top-right
    bottomFace.tris[1].v[2] = Vertex(verts[7].pos, {1.0f, 1.0f}); // Bottom-right
    bottomFace.normal = {0.0f, -1.0f, 0.0f};

    // Add all faces to the cube mesh
    meshCube.faces.push_back(frontFace);
    meshCube.faces.push_back(rightFace);
    meshCube.faces.push_back(backFace);
    meshCube.faces.push_back(leftFace);
    meshCube.faces.push_back(topFace);
    meshCube.faces.push_back(bottomFace);
}

// Handle User Input Events
void HandleInput() {
    SDL_Event event;
    mouse_dx = mouse_dy = 0;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            emscripten_cancel_main_loop();
        } else if (event.type == SDL_KEYDOWN) {
            keys[event.key.keysym.scancode] = true;
            // User can switch between wireframe and solid mode by pressing 'X'
            if (event.key.keysym.scancode == SDL_SCANCODE_X) wireframeMode = !wireframeMode;
        } else if (event.type == SDL_KEYUP) {
            keys[event.key.keysym.scancode] = false;
        } else if (event.type == SDL_MOUSEMOTION) {
            mouse_dx = event.motion.xrel;
            mouse_dy = event.motion.yrel;
        } else if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                leftMouseButtonDown = true;
            } else if (event.button.button == SDL_BUTTON_RIGHT) {
                rightMouseButtonDown = true;
            }
        }
    }
}

// Function to check collision with blocks
bool CheckCollision(const Vec3& pos, bool checkX, bool checkY, bool checkZ) {
    const float playerWidth = 0.3f;
    const float playerHeight = 1.8f;

    int minX = floor(pos.x - playerWidth);
    int maxX = floor(pos.x + playerWidth);
    int minY = floor(pos.y - playerHeight);
    int maxY = floor(pos.y);
    int minZ = floor(pos.z - playerWidth);
    int maxZ = floor(pos.z + playerWidth);

    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            for (int z = minZ; z <= maxZ; z++) {
                if (world.IsBlockAtPosition(x, y, z)) {
                    // Check which axes are involved in the collision
                    bool collision = false;
                    if (checkX && x >= floor(pos.x - playerWidth) && x <= floor(pos.x + playerWidth)) collision = true;
                    if (checkY && y >= floor(pos.y - playerHeight) && y <= floor(pos.y)) collision = true;
                    if (checkZ && z >= floor(pos.z - playerWidth) && z <= floor(pos.z + playerWidth)) collision = true;

                    if (collision) return true;
                }
            }
        }
    }
    return false;
}

// Texture Atlas Settings
const int TEX_SIZE = 16;
const int ATLAS_COLUMNS = 5; // Number of textures horizontally
const int ATLAS_WIDTH = ATLAS_COLUMNS * TEX_SIZE; // Total width of the atlas in pixels
const int ATLAS_HEIGHT = 16; // Total height of the atlas in pixels

// Texture coordinates based on BlockType
std::unordered_map<BlockType, Vec2> blockTextureOffsets = {
    { BlockType::Stone,     Vec2(0.0f, 0.0f) },     // Column 0
    { BlockType::Dirt,      Vec2(1.0f, 0.0f) },     // Column 1
    { BlockType::OakWood,   Vec2(2.0f, 0.0f) },     // Column 2
    { BlockType::GrassSide, Vec2(4.0f, 0.0f) }      // Column 4
};


// Update texture coordinates based on BlockType
Vec2 GetTextureOffset(BlockType type) {
    if (blockTextureOffsets.find(type) != blockTextureOffsets.end()) { return blockTextureOffsets[type]; }
    // Default to Stone texture if not found - need to swap it with a missing texture
    return Vec2(0.0f, 0.0f);
}

// Helper function to project 3D points to 2D screen space
bool ProjectToScreen(const Vec3& point, const Mat4& matView, const Mat4& matProj, Vec2& screenPoint) {
    Vec3 transformed = MultiplyMatrixVector(point, matView);
    Vec3 projected = MultiplyMatrixVector(transformed, matProj);

    if (projected.z == 0.0f) return false;

    // Normalise
    projected.x /= projected.z;
    projected.y /= projected.z;

    // Convert to screen coordinates
    screenPoint.u = (projected.x + 1.0f) * 0.5f * SCREEN_WIDTH;
    screenPoint.v = (1.0f - (projected.y + 1.0f) * 0.5f) * SCREEN_HEIGHT;

    return true;
}

// Update camera and scene
void Update(float deltaTime) {
    camera.yaw += mouse_dx * 0.1f;
    camera.pitch -= mouse_dy * 0.1f;

    if (camera.pitch > 89.0f) camera.pitch = 89.0f;
    if (camera.pitch < -89.0f) camera.pitch = -89.0f;

    camera.lookDir = {
        cosf(camera.pitch * PI / 180.0f) * sinf(camera.yaw * PI / 180.0f),
        sinf(camera.pitch * PI / 180.0f),
        cosf(camera.pitch * PI / 180.0f) * cosf(camera.yaw * PI / 180.0f)
    };

    // Calculate forward and right vectors based on yaw only
    float yawRad = camera.yaw * PI / 180.0f;
    Vec3 forward = {
        sinf(yawRad),
        0.0f,
        cosf(yawRad)
    };

    Vec3 right = {
        forward.z,
        0.0f,
        -forward.x
    };

    float speed = 5.0f;

    // Save old position for collision handling
    Vec3 oldPosition = camera.pos;

    // Initialise movement direction
    Vec3 moveDir = {0, 0, 0};

    // Accumulate movement based on input
    if (keys[SDL_SCANCODE_W]) moveDir = moveDir + forward * deltaTime * speed;
    if (keys[SDL_SCANCODE_S]) moveDir = moveDir - forward * deltaTime * speed;
    if (keys[SDL_SCANCODE_A]) moveDir = moveDir - right * deltaTime * speed;
    if (keys[SDL_SCANCODE_D]) moveDir = moveDir + right * deltaTime * speed;

    // Move in X and check collision
    camera.pos.x += moveDir.x;
    if (CheckCollision(camera.pos, true, false, false)) camera.pos.x = oldPosition.x;

    // Move in Z and check collision
    camera.pos.z += moveDir.z;
    if (CheckCollision(camera.pos, false, false, true)) camera.pos.z = oldPosition.z;

    // Handle jumping and gravity in the Y-axis
    if (keys[SDL_SCANCODE_SPACE] && camera.isOnGround) {
        camera.verticalVelocity = 6.0f; // Jump velocity
        camera.isOnGround = false;
    }

    // Apply gravity to vertical velocity
    const float gravity = 13.8f;
    camera.verticalVelocity -= gravity * deltaTime;
    camera.pos.y += camera.verticalVelocity * deltaTime;

    // Check Y-axis collision
    if (CheckCollision(camera.pos, false, true, false)) {
        // If moving downwards, snap to the block and set on-ground
        if (camera.verticalVelocity < 0) {
            camera.isOnGround = true;
            camera.pos.y = oldPosition.y; // Revert to prevent sinking
        } else {
            // If moving upwards, stop the vertical movement
            camera.isOnGround = false;
        }
        camera.verticalVelocity = 0; // Stop vertical movement
    } else {
        camera.isOnGround = false;
    }

    // View Bobbing Logic
    bool isMoving = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_D];
    if (isMoving && camera.isOnGround) {
        // Increment the bobbing timer
        camera.bobbingTimer += deltaTime * camera.bobbingFrequency;

        // Calculate the bobbing offset using a sine wave
        camera.bobbingOffsetY = sinf(camera.bobbingTimer) * camera.bobbingAmplitude;
    }
    else {
        // Reset bobbing when not moving or in the air
        camera.bobbingOffsetY = 0.0f;
        camera.bobbingTimer = 0.0f;
    }

    // Handle block placement and deletion
    if (leftMouseButtonDown || rightMouseButtonDown) {
        Vec3 hitBlockPosition, hitNormal;
        float maxDistance = 8.0f;
        if (CastRay(camera.pos, camera.lookDir, maxDistance, hitBlockPosition, hitNormal)) {
            int hx = int(hitBlockPosition.x);
            int hy = int(hitBlockPosition.y);
            int hz = int(hitBlockPosition.z);

            if (leftMouseButtonDown) {
                world.RemoveBlockAtPosition(hx, hy, hz);
            } else if (rightMouseButtonDown) {
                // Calculate the new block position based on the hit position and normal
                Vec3 newBlockPos = hitBlockPosition + hitNormal;
                BlockType newType = BlockType::OakWood; // Currently the player can only place OakWood blocks
                world.AddBlockAtPosition(int(newBlockPos.x), int(newBlockPos.y), int(newBlockPos.z), newType);
            }
        }
    }

    leftMouseButtonDown = false;
    rightMouseButtonDown = false;
    {
        Vec3 hitBlockPos, hitNorm;
        float selectionDistance = 8.0f;
        if (CastRay(camera.pos, camera.lookDir, selectionDistance, hitBlockPos, hitNorm)) {
            selectedBlockPosition = hitBlockPos;
            hasSelectedBlock = true;
        }
        else {
            hasSelectedBlock = false;
        }
    }
}

void ClearScreen() {
    // Blue Sky Background
    SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255);
    SDL_RenderClear(renderer);
}

// Draw a triangle using SDL_RenderGeometry with adjusted texture coordinates
void DrawTriangle(const Triangle& tri, BlockType type, const Vec3& faceNormal) {
    SDL_Vertex vertices[3];

    // Determine texture offset based on BlockType and face normal
    Vec2 texOffset;

    // Grass is slightly different as it has a top and side texture - this should really be handled in a more general way - future me, please fix this 😇
    if (type == BlockType::Grass)
    {
        if (faceNormal.y > 0.5f) texOffset = Vec2(3.0f, 0.0f);
        else if (faceNormal.y < -0.5f) texOffset = blockTextureOffsets[BlockType::Dirt];
        else texOffset = blockTextureOffsets[BlockType::GrassSide];
    }
    else
    {
        // Use the standard texture for the block type
        texOffset = GetTextureOffset(type);
    }

    float scaleU = 1.0f / static_cast<float>(ATLAS_COLUMNS);
    float scaleV = 1.0f / static_cast<float>(ATLAS_HEIGHT / TEX_SIZE);

    for (int i = 0; i < 3; ++i) {
        vertices[i].position.x = tri.v[i].pos.x;
        vertices[i].position.y = tri.v[i].pos.y;
        vertices[i].color.r = 255;
        vertices[i].color.g = 255;
        vertices[i].color.b = 255;
        vertices[i].color.a = 255;
        vertices[i].tex_coord.x = texOffset.u * scaleU + tri.v[i].tex.u * scaleU;
        vertices[i].tex_coord.y = texOffset.v * scaleV + tri.v[i].tex.v * scaleV;
    }

    SDL_RenderGeometry(renderer, textureAtlas, vertices, 3, NULL, 0);
}

// Function to draw wireframe triangles
void DrawWireframe(const Triangle& tri) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    // Draw lines between the vertices
    SDL_RenderDrawLine(renderer, (int)tri.v[0].pos.x, (int)tri.v[0].pos.y,
                               (int)tri.v[1].pos.x, (int)tri.v[1].pos.y);
    SDL_RenderDrawLine(renderer, (int)tri.v[1].pos.x, (int)tri.v[1].pos.y,
                               (int)tri.v[2].pos.x, (int)tri.v[2].pos.y);
    SDL_RenderDrawLine(renderer, (int)tri.v[2].pos.x, (int)tri.v[2].pos.y,
                               (int)tri.v[0].pos.x, (int)tri.v[0].pos.y);
}

// Draw the world
Vertex IntersectPlane(const Vec3 &plane_p, const Vec3 &plane_n, const Vertex &lineStart, const Vertex &lineEnd, float &t) {
    Vec3 plane_n_norm = plane_n.normalize();
    float plane_d = -plane_n_norm.dot(plane_p);
    float ad = lineStart.pos.dot(plane_n_norm);
    float bd = lineEnd.pos.dot(plane_n_norm);
    float denominator = bd - ad;
    if (fabs(denominator) < 1e-6f) {
        t = 0.0f;
        return lineStart;
    }
    t = (-plane_d - ad) / denominator;
    Vec3 lineStartToEnd = lineEnd.pos - lineStart.pos;
    Vec3 intersectionPoint = lineStart.pos + lineStartToEnd * t;

    Vec2 texStartToEnd = lineEnd.tex - lineStart.tex;
    Vec2 intersectionTex = lineStart.tex + texStartToEnd * t;

    return Vertex(intersectionPoint, intersectionTex);
}

// Clipping triangle against near plane (returns the number of output triangles)
int TriangleClipAgainstPlane(const Vec3 &plane_p, const Vec3 &plane_n, const Triangle &inTri, Triangle &outTri1, Triangle &outTri2) {
    Vec3 plane_n_norm = plane_n.normalize();

    auto dist = [&](const Vertex &v) {
        Vec3 n = v.pos - plane_p;
        return plane_n_norm.dot(n);
    };

    const Vertex *inside_points[3];  int nInsidePointCount = 0;
    const Vertex *outside_points[3]; int nOutsidePointCount = 0;

    float d0 = dist(inTri.v[0]);
    float d1 = dist(inTri.v[1]);
    float d2 = dist(inTri.v[2]);

    if (d0 >= 0) inside_points[nInsidePointCount++] = &inTri.v[0]; else outside_points[nOutsidePointCount++] = &inTri.v[0];
    if (d1 >= 0) inside_points[nInsidePointCount++] = &inTri.v[1]; else outside_points[nOutsidePointCount++] = &inTri.v[1];
    if (d2 >= 0) inside_points[nInsidePointCount++] = &inTri.v[2]; else outside_points[nOutsidePointCount++] = &inTri.v[2];

    if (nInsidePointCount == 0)  return 0;

    float t;
    if (nInsidePointCount == 3) {
        outTri1 = inTri;
        return 1;
    }

    if (nInsidePointCount == 1 && nOutsidePointCount == 2) {
        outTri1.v[0] = *inside_points[0];
        outTri1.v[1] = IntersectPlane(plane_p, plane_n_norm, *inside_points[0], *outside_points[0], t);
        outTri1.v[2] = IntersectPlane(plane_p, plane_n_norm, *inside_points[0], *outside_points[1], t);
        return 1;
    }

    if (nInsidePointCount == 2 && nOutsidePointCount == 1) {
        outTri1.v[0] = *inside_points[0];
        outTri1.v[1] = *inside_points[1];
        outTri1.v[2] = IntersectPlane(plane_p, plane_n_norm, *inside_points[0], *outside_points[0], t);

        outTri2.v[0] = *inside_points[1];
        outTri2.v[1] = outTri1.v[2];
        outTri2.v[2] = IntersectPlane(plane_p, plane_n_norm, *inside_points[1], *outside_points[0], t);
        return 2;
    }

    return 0;
}

// Raycasting function using 3D DDA algorithm
bool CastRay(Vec3 origin, Vec3 direction, float maxDistance, Vec3& hitBlockPosition, Vec3& hitNormal) {
    direction = direction.normalize();

    float dx = direction.x;
    float dy = direction.y;
    float dz = direction.z;

    int ix = int(floor(origin.x));
    int iy = int(floor(origin.y));
    int iz = int(floor(origin.z));

    int stepX = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
    int stepY = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);
    int stepZ = (dz > 0) ? 1 : ((dz < 0) ? -1 : 0);

    float tMaxX, tMaxY, tMaxZ;
    float tDeltaX = (dx != 0) ? fabs(1.0f / dx) : 1e30;
    float tDeltaY = (dy != 0) ? fabs(1.0f / dy) : 1e30;
    float tDeltaZ = (dz != 0) ? fabs(1.0f / dz) : 1e30;

    if (dx != 0) {
        float voxelBoundaryX = (dx > 0) ? (ix + 1) : ix;
        tMaxX = (voxelBoundaryX - origin.x) / dx;
    } else {
        tMaxX = 1e30;
    }

    if (dy != 0) {
        float voxelBoundaryY = (dy > 0) ? (iy + 1) : iy;
        tMaxY = (voxelBoundaryY - origin.y) / dy;
    } else {
        tMaxY = 1e30;
    }

    if (dz != 0) {
        float voxelBoundaryZ = (dz > 0) ? (iz + 1) : iz;
        tMaxZ = (voxelBoundaryZ - origin.z) / dz;
    } else {
        tMaxZ = 1e30;
    }

    float t = 0;
    int maxSteps = 1000;

    for (int i = 0; i < maxSteps; i++) {
        if (world.IsBlockAtPosition(ix, iy, iz)) {
            hitBlockPosition = Vec3(float(ix), float(iy), float(iz));

            if (tMaxX < tMaxY && tMaxX < tMaxZ)  hitNormal = Vec3(-stepX, 0, 0);
            else if (tMaxY < tMaxZ)  hitNormal = Vec3(0, -stepY, 0);
            else  hitNormal = Vec3(0, 0, -stepZ);
            return true;
        }

        if (tMaxX < tMaxY && tMaxX < tMaxZ) {
            ix += stepX;
            t = tMaxX;
            tMaxX += tDeltaX;
        } else if (tMaxY < tMaxZ) {
            iy += stepY;
            t = tMaxY;
            tMaxY += tDeltaY;
        } else {
            iz += stepZ;
            t = tMaxZ;
            tMaxZ += tDeltaZ;
        }

        if (t > maxDistance) break;
    }

    return false;
}

// Draw the crosshair at the center of the screen
void DrawCrosshair() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    int crosshairSize = 10;

    // Horizontal line
    SDL_RenderDrawLine(renderer, SCREEN_WIDTH / 2 - crosshairSize, SCREEN_HEIGHT / 2,
                               SCREEN_WIDTH / 2 + crosshairSize, SCREEN_HEIGHT / 2);
    // Vertical line
    SDL_RenderDrawLine(renderer, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - crosshairSize,
                               SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + crosshairSize);
}

// Draw the outline around the selected block by outlining its edges
void DrawBlockOutline(const Vec3& blockPos, const Mat4& matView, const Mat4& matProj) {
    // Define the 8 corners of the block
    std::vector<Vec3> corners = {
        {blockPos.x, blockPos.y, blockPos.z},                       // Corner 0
        {blockPos.x + 1.0f, blockPos.y, blockPos.z},                // Corner 1
        {blockPos.x + 1.0f, blockPos.y + 1.0f, blockPos.z},         // Corner 2
        {blockPos.x, blockPos.y + 1.0f, blockPos.z},                // Corner 3
        {blockPos.x, blockPos.y, blockPos.z + 1.0f},                // Corner 4
        {blockPos.x + 1.0f, blockPos.y, blockPos.z + 1.0f},         // Corner 5
        {blockPos.x + 1.0f, blockPos.y + 1.0f, blockPos.z + 1.0f},  // Corner 6
        {blockPos.x, blockPos.y + 1.0f, blockPos.z + 1.0f}          // Corner 7
    };

    // Project all corners to screen space
    std::vector<Vec2> projectedCorners;
    projectedCorners.reserve(8);
    for (const auto& corner : corners) {
        Vec2 screenPoint;
        if (ProjectToScreen(corner, matView, matProj, screenPoint)) {
            projectedCorners.push_back(screenPoint);
        } else {
            // If projection fails (e.g., behind camera), push an invalid point
            projectedCorners.emplace_back(Vec2(-1.0f, -1.0f));
        }
    }

    // 12 edges of the cube by specifying pairs of corner indices
    std::vector<std::pair<int, int>> edges = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Bottom edges
        {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Top edges
        {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Side edges
    };

    // Set the outline color (1px black)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    // Draw each edge
    for (const auto& edge : edges) {
        int startIdx = edge.first;
        int endIdx = edge.second;

        // Retrieve the projected points
        Vec2 start = projectedCorners[startIdx];
        Vec2 end = projectedCorners[endIdx];

        // Only draw lines where both points are valid
        if (start.u >= 0 && start.v >= 0 && end.u >= 0 && end.v >= 0)  SDL_RenderDrawLine(renderer, (int)start.u, (int)start.v, (int)end.u, (int)end.v);
    }
}

// Main Rendering Function
void Render() {
    float fNear = 0.1f;
    float fFar = 1000.0f;
    float fFov = 80.0f;
    float fAspectRatio = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;
    float fFovRad = 1.0f / tanf(fFov * 0.5f / 180.0f * PI);

    Mat4 matProj = {0};
    matProj.m[0][0] = fAspectRatio * fFovRad;
    matProj.m[1][1] = fFovRad;
    matProj.m[2][2] = fFar / (fFar - fNear);
    matProj.m[3][2] = (-fFar * fNear) / (fFar - fNear);
    matProj.m[2][3] = 1.0f;
    matProj.m[3][3] = 0.0f;

    // Adjusted Camera Position with Bobbing Offset for Rendering
    Vec3 renderPos = camera.pos;
    renderPos.y += camera.bobbingOffsetY;

    // Camera matrix
    Vec3 up = {0, 1, 0};
    Vec3 target = renderPos + camera.lookDir;
    Mat4 matCamera = MatrixPointAt(renderPos, target, up);

    // View matrix (inverse of camera matrix)
    Mat4 matView = MatrixQuickInverse(matCamera);

    // Clipping plane setup
    Vec3 nearPlanePos = {0, 0, fNear};
    Vec3 nearPlaneNormal = {0, 0, 1};

    // Collect all visible triangles globally
    std::vector<SortedTriangle> visibleTriangles;

    for (auto& chunk : world.chunks) {
        for (auto& block : chunk.blocks) {
            if (block.type == BlockType::Air) continue;

            Mat4 matTrans = MatrixMakeTranslation(block.position.x, block.position.y, block.position.z);
            Mat4 matWorld = matTrans;

            for (auto& face : meshCube.faces) {
                Vec3 neighborPos = block.position + face.normal;
                int nx = static_cast<int>(floor(neighborPos.x));
                int ny = static_cast<int>(floor(neighborPos.y));
                int nz = static_cast<int>(floor(neighborPos.z));

                // Check for a neighboring block, and skip the face if it exists
                if (world.IsBlockAtPosition(nx, ny, nz)) continue;

                for (int i = 0; i < 2; i++) {
                    Triangle tri = face.tris[i];
                    Triangle triTransformed;

                    // Transform vertices
                    for (int j = 0; j < 3; ++j) {
                        triTransformed.v[j].pos = MultiplyMatrixVector(tri.v[j].pos, matWorld);
                        triTransformed.v[j].tex = tri.v[j].tex;
                    }

                    // Calculate depth (average distance to camera along lookDir)
                    Vec3 center = (triTransformed.v[0].pos + triTransformed.v[1].pos + triTransformed.v[2].pos) * (1.0f / 3.0f);
                    float depth = (center - camera.pos).dot(camera.lookDir.normalize());

                    // Store the triangle with its depth, block type, and face normal
                    SortedTriangle sortedTri;
                    sortedTri.tri = triTransformed;
                    sortedTri.depth = depth;
                    sortedTri.type = block.type;
                    sortedTri.faceNormal = face.normal;
                    visibleTriangles.push_back(sortedTri);
                }
            }
        }
    }

    // Sort triangles by depth (Painter's Algorithm: far to near)
    std::sort(visibleTriangles.begin(), visibleTriangles.end(), [](const SortedTriangle& a, const SortedTriangle& b) {
        return a.depth > b.depth; // Sort from farthest to nearest
    });

    // Clear the screen
    ClearScreen();

    // Conditional Rendering: Textured or Wireframe
    if (!wireframeMode) {
        // Textured Rendering Mode
        for (const auto& sortedTri : visibleTriangles) {
            Triangle triProjected, triTransformed, triViewed;
            Triangle clipped[2];

            triTransformed = sortedTri.tri;

            Vec3 normal, line1, line2;

            line1 = triTransformed.v[1].pos - triTransformed.v[0].pos;
            line2 = triTransformed.v[2].pos - triTransformed.v[0].pos;

            normal = line1.cross(line2).normalize();

            Vec3 cameraRay = triTransformed.v[0].pos - camera.pos;

            if (normal.dot(cameraRay) >= 0.0f) continue;

            // Transform to view space
            Mat4 matViewCopy = matView;
            for (int j = 0; j < 3; ++j) {
                triViewed.v[j].pos = MultiplyMatrixVector(triTransformed.v[j].pos, matViewCopy);
                triViewed.v[j].tex = triTransformed.v[j].tex;
            }

            int nClippedTriangles = TriangleClipAgainstPlane(nearPlanePos, nearPlaneNormal, triViewed, clipped[0], clipped[1]);

            for (int n = 0; n < nClippedTriangles; n++) {
                // Project the triangle
                Triangle triProjectedTemp;
                for (int j = 0; j < 3; ++j) {
                    triProjectedTemp.v[j].pos = MultiplyMatrixVector(clipped[n].v[j].pos, matProj);
                    triProjectedTemp.v[j].tex = clipped[n].v[j].tex;
                }

                // Scale into view
                for (int j = 0; j < 3; ++j) {
                    triProjectedTemp.v[j].pos.x = (triProjectedTemp.v[j].pos.x + 1.0f) * 0.5f * SCREEN_WIDTH;
                    triProjectedTemp.v[j].pos.y = (1.0f - (triProjectedTemp.v[j].pos.y + 1.0f) * 0.5f) * SCREEN_HEIGHT;
                }

                DrawTriangle(triProjectedTemp, sortedTri.type, sortedTri.faceNormal);
            }
        }
    }
    else {
        // Wireframe Rendering Mode
        for (const auto& sortedTri : visibleTriangles) {
            Triangle triProjected, triTransformed, triViewed;
            Triangle clipped[2];

            triTransformed = sortedTri.tri;

            Vec3 normal, line1, line2;

            line1 = triTransformed.v[1].pos - triTransformed.v[0].pos;
            line2 = triTransformed.v[2].pos - triTransformed.v[0].pos;

            normal = line1.cross(line2).normalize();

            Vec3 cameraRay = triTransformed.v[0].pos - camera.pos;

            if (normal.dot(cameraRay) >= 0.0f) continue;

            // Transform to view space
            Mat4 matViewCopy = matView;
            for (int j = 0; j < 3; ++j) {
                triViewed.v[j].pos = MultiplyMatrixVector(triTransformed.v[j].pos, matViewCopy);
                triViewed.v[j].tex = triTransformed.v[j].tex;
            }

            int nClippedTriangles = TriangleClipAgainstPlane(nearPlanePos, nearPlaneNormal, triViewed, clipped[0], clipped[1]);

            for (int n = 0; n < nClippedTriangles; n++) {
                // Project the triangle
                Triangle triProjectedTemp;
                for (int j = 0; j < 3; ++j) {
                    triProjectedTemp.v[j].pos = MultiplyMatrixVector(clipped[n].v[j].pos, matProj);
                    triProjectedTemp.v[j].tex = clipped[n].v[j].tex;
                }

                // Scale into view
                for (int j = 0; j < 3; ++j) {
                    triProjectedTemp.v[j].pos.x = (triProjectedTemp.v[j].pos.x + 1.0f) * 0.5f * SCREEN_WIDTH;
                    triProjectedTemp.v[j].pos.y = (1.0f - (triProjectedTemp.v[j].pos.y + 1.0f) * 0.5f) * SCREEN_HEIGHT;
                }

                // Draw the wireframe triangle
                DrawWireframe(triProjectedTemp);
            }
        }
    }

    // Draws the outline around the selected block - this is currently disabled as there is an alignment issue
    if (hasSelectedBlock) {
        // DrawBlockOutline(selectedBlockPosition, matView, matProj);
    }

    DrawCrosshair();

    SDL_RenderPresent(renderer);
}

// Main loop Function
void MainLoop() {
    static Uint32 lastTime = SDL_GetTicks();
    Uint32 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;

    HandleInput();
    Update(deltaTime);
    Render();
}

int main(int argc, char* argv[])
{
    SDL_Init(SDL_INIT_VIDEO);

    // Initialise SDL_image
    IMG_Init(IMG_INIT_PNG);

    window = SDL_CreateWindow("3D Cube Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Load the texture atlas
    textureAtlas = IMG_LoadTexture(renderer, "assets/texture_atlas.png");
    if (!textureAtlas) {
        printf("Failed to load texture atlas: %s\n", IMG_GetError());
        return 1;
    }

    // Set texture properties
    SDL_SetTextureBlendMode(textureAtlas, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(textureAtlas, SDL_ScaleModeNearest);

    InitCubeMesh();

    // Initialise world and set variables
    world.Initialise();
    // world.GenerateFlatWorld();
    world.GeneratePerlinWorld();

    // Calculate center position
    float centerX = (world.worldSize * world.chunkSize) / 2.0f;
    float centerZ = (world.worldSize * world.chunkSize) / 2.0f;
    camera.pos = {centerX, 20.0f, centerZ};
    camera.yaw = 0.0f;
    camera.pitch = 0.0f;
    camera.verticalVelocity = 0.0f;
    camera.isOnGround = false;

    SDL_SetRelativeMouseMode(SDL_TRUE);

    emscripten_set_main_loop(MainLoop, 0, 1);

    // Clean up
    SDL_DestroyTexture(textureAtlas);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}