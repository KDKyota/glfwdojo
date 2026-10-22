#include "SceneTerrain.h"
#include <iostream>
#include <memory>

namespace gl
{

    TerrainData::TerrainData(const layout::HeightMapSpawn &spawn, gl::PbrMaterial material)
        : heightField(spawn.filepath, spawn.origin), mesh(SceneTerrain::buildMesh(heightField, material, spawn)) {}

    Mesh SceneTerrain::buildMesh(HeightField &field, PbrMaterial material, const layout::HeightMapSpawn &spawn)
    {
        TerrainMeshData meshData =
            BuildTerrainMesh(field, spawn.extent, spawn.cellCountX, spawn.cellCountZ, spawn.uvTileSize);
        Mesh mesh(std::move(meshData.vertices), std::move(meshData.indices), std::move(material), false);
        return mesh;
    }

    SceneTerrain::SceneTerrain(TextureCache &cache)
    {
        for (const auto &spawn : layout::heightMapSpawns)
        {
            PbrMaterial material{};
            material.baseColorMap = cache.get(spawn.baseColorTexturePath, false, ColorSpace::SRGB);
            material.roughness = 0.85f;
            terrains_.push_back(std::make_unique<TerrainData>(spawn, std::move(material)));
        }
    }

    void SceneTerrain::Draw(Shader &shader) const
    {
        for (const auto &terrain : terrains_)
        {
            shader.setBool("hasBones", false);
            shader.setMat4("model", glm::mat4(1.f));
            terrain->mesh.Draw(shader);
        }
    }

    std::vector<const HeightField *> SceneTerrain::getHeightField()
    {
        std::vector<const HeightField *> result;
        for (const auto &terrain : terrains_)
        {
            result.push_back(&terrain->heightField);
        }
        return result;
    }

} // namespace gl
