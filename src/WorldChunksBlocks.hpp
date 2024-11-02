// WorldChunksBlocks.hpp
#ifndef WORLD_HPP
#define WORLD_HPP

// Gen seed based on the current time
unsigned int GenerateSeed() {
    using namespace std::chrono;
    return static_cast<unsigned int>(
        duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
        ).count()
    );
}

// Block Types
enum class BlockType {
    Air = 0,
    Stone,
    Dirt,
    OakWood,
    Grass,
    GrassSide
};

struct BlockKey {
    int x, y, z;
    bool operator==(const BlockKey& other) const { return x == other.x && y == other.y && z == other.z; }
};

// Hash function for BlockKey
struct BlockKeyHash {
    std::size_t operator()(const BlockKey& k) const {
        return ((std::hash<int>()(k.x) ^
                (std::hash<int>()(k.y) << 1)) >> 1) ^
                (std::hash<int>()(k.z) << 1);
    }
};

// Block struct with BlockType
struct Block {
    Vec3 position;
    BlockType type;
    Block(Vec3 pos = Vec3(), BlockType t = BlockType::Stone) : position(pos), type(t) {}
};

// Chunk Struct
struct Chunk {
    int sizeX, sizeY, sizeZ;
    Vec3 offset;
    std::vector<Block> blocks;

    // Create flat chunk of stone blocks - this is mainly used for testing
    void GenerateFlatTerrain(int sizeX, int sizeZ, Vec3 offset, int sizeY) {
        this->sizeX = sizeX;
        this->sizeZ = sizeZ;
        this->sizeY = sizeY;
        this->offset = offset;

        for (int x = 0; x < sizeX; x++) {
            for (int z = 0; z < sizeZ; z++) {
                Block block;
                block.position = {
                    static_cast<float>(x) + offset.x,
                    0.0f + offset.y,
                    static_cast<float>(z) + offset.z
                };
                block.type = BlockType::Stone;
                blocks.push_back(block);
            }
        }
    }
};

// World Struct
struct World {
    std::vector<Chunk> chunks;
    int worldSize;
    int chunkSize;
    int chunkHeight;
    PerlinNoise perlin;
    std::unordered_map<BlockKey, Block, BlockKeyHash> blockMap;
    World() : perlin(GenerateSeed()) {}

    void Initialise() {
        chunkSize = 12;
        chunkHeight = 32;
        worldSize = 3;
    }

    // Generate Flat World
    void GenerateFlatWorld() {
        for (int cx = 0; cx < worldSize; cx++) {
            for (int cz = 0; cz < worldSize; cz++) {
                Chunk chunk;
                Vec3 chunkOffset = {
                    static_cast<float>(cx * chunkSize),
                    0.0f,
                    static_cast<float>(cz * chunkSize)
                };
                chunk.GenerateFlatTerrain(chunkSize, chunkSize, chunkOffset, chunkHeight);
                chunks.push_back(chunk);

                // Populate blockMap
                for (const Block& block : chunk.blocks) {
                    BlockKey key = {static_cast<int>(block.position.x),
                                    static_cast<int>(block.position.y),
                                    static_cast<int>(block.position.z)};
                    blockMap[key] = block;
                }
            }
        }
    }

    // Generate Perlin World
    void GeneratePerlinWorld() {
        for (int cx = 0; cx < worldSize; cx++) {
            for (int cz = 0; cz < worldSize; cz++) {
                Chunk chunk;
                Vec3 chunkOffset = {
                    static_cast<float>(cx * chunkSize),
                    0.0f,
                    static_cast<float>(cz * chunkSize)
                };
                chunk.sizeX = chunkSize;
                chunk.sizeZ = chunkSize;
                chunk.sizeY = chunkHeight;
                chunk.offset = chunkOffset;

                // Generate terrain using Perlin noise
                for (int x = 0; x < chunkSize; x++) {
                    for (int z = 0; z < chunkSize; z++) {
                        // World coordinates
                        float worldX = chunkOffset.x + static_cast<float>(x);
                        float worldZ = chunkOffset.z + static_cast<float>(z);

                        // Parameters for Perlin noise
                        double frequency = 0.15;
                        double amplitude = 10.0;

                        // Calculate Perlin value
                        double noiseValue = perlin.noise(worldX * frequency, worldZ * frequency, 0.0);
                        int height = static_cast<int>(noiseValue * amplitude) + 1;

                        // Populate blocks up to calculated height
                        for (int y = 0; y < height && y < chunkHeight; y++) {
                            Block block;
                            block.position = {
                                static_cast<float>(x) + chunkOffset.x,
                                static_cast<float>(y) + chunkOffset.y,
                                static_cast<float>(z) + chunkOffset.z
                            };

                            // Assign Block Types to World
                            // - TOP LAYER is Grass
                            // - 3 LAYERS BELOW TOP are Dirt
                            // - REST are Stone
                            if (y == height - 1) block.type = BlockType::Grass;
                            else if (y >= height - 3)  block.type = BlockType::Dirt;
                            else  block.type = BlockType::Stone;

                            chunk.blocks.push_back(block);

                            // Add to blockMap
                            BlockKey key = {static_cast<int>(block.position.x),
                                           static_cast<int>(block.position.y),
                                           static_cast<int>(block.position.z)};
                            blockMap[key] = block;
                        }
                    }
                }
                chunks.push_back(chunk);
            }
        }
    }

    // Get chunk at a given world pos
    Chunk* GetChunkAt(int x, int y, int z) {
        for (Chunk& chunk : chunks) {
            if (x >= static_cast<int>(chunk.offset.x) && x < static_cast<int>(chunk.offset.x + chunk.sizeX) &&
                y >= static_cast<int>(chunk.offset.y) && y < static_cast<int>(chunk.offset.y + chunk.sizeY) &&
                z >= static_cast<int>(chunk.offset.z) && z < static_cast<int>(chunk.offset.z + chunk.sizeZ)) {
                return &chunk;
            }
        }
        return nullptr;
    }

    // Check if block exists at position using blockMap
    bool IsBlockAtPosition(int x, int y, int z) {
        BlockKey key = {x, y, z};
        return blockMap.find(key) != blockMap.end();
    }

    // Remove block at position using blockMap
    void RemoveBlockAtPosition(int x, int y, int z) {
        BlockKey key = {x, y, z};
        auto it = blockMap.find(key);
        if (it != blockMap.end()) {
            // Find the corresponding chunk
            Chunk* chunk = GetChunkAt(x, y, z);
            if (chunk) {
                // Remove the block from the chunk's blocks vector
                auto blockIt = std::remove_if(chunk->blocks.begin(), chunk->blocks.end(),
                    [&](const Block& block) {
                        return static_cast<int>(block.position.x) == x &&
                               static_cast<int>(block.position.y) == y &&
                               static_cast<int>(block.position.z) == z;
                    });
                if (blockIt != chunk->blocks.end()) {
                    chunk->blocks.erase(blockIt, chunk->blocks.end());
                }
            }

            // Remove from the blockMap
            blockMap.erase(it);
        }
    }

    // Add block at position using blockMap with BlockType
    void AddBlockAtPosition(int x, int y, int z, BlockType type) {
        BlockKey key = {x, y, z};
        if (blockMap.find(key) != blockMap.end()) return; // Block already exists

        Chunk* chunk = GetChunkAt(x, y, z);
        if (!chunk) {
            // Calculate new chunk offset based on block position
            int chunkX = (x / chunkSize) * chunkSize;
            int chunkY = (y / chunkHeight) * chunkHeight;
            int chunkZ = (z / chunkSize) * chunkSize;

            Vec3 chunkOffset = {
                static_cast<float>(chunkX),
                static_cast<float>(chunkY),
                static_cast<float>(chunkZ)
            };

            // Create and add the new chunk
            Chunk newChunk;
            newChunk.GenerateFlatTerrain(chunkSize, chunkSize, chunkOffset, chunkHeight);
            chunks.push_back(newChunk);
            chunk = &chunks.back();

            // Add new blocks from the new chunk to blockMap
            for (const Block& block : chunk->blocks) {
                BlockKey newKey = {static_cast<int>(block.position.x),
                                   static_cast<int>(block.position.y),
                                   static_cast<int>(block.position.z)};
                blockMap[newKey] = block;
            }
        }

        // Add the new block with the specified type
        Block block;
        block.position = Vec3(float(x), float(y), float(z));
        block.type = type;
        chunk->blocks.push_back(block);
        blockMap[key] = block;
    }
};

#endif