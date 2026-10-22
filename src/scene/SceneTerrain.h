#pragma once

#include "HeightField.h"
#include <memory>
#include "TerrainMesh.h"
#include "gl/Shader.h"
#include "gl/TextureCache.h"
#include "asset/Mesh.h"
#include "SceneLayout.h"

namespace gl {

struct TerrainData {
    TerrainData(const layout::HeightMapSpawn &spawn, gl::PbrMaterial material);
    HeightField heightField;
    Mesh mesh;
};

class SceneTerrain {
  public:
    explicit SceneTerrain(TextureCache &cache);
    static Mesh buildMesh(HeightField &field, PbrMaterial material, const layout::HeightMapSpawn &spawn);
    void Draw(Shader &shader) const;
    std::vector<const HeightField*> getHeightField();

  private:
    std::vector<std::unique_ptr<TerrainData>> terrains_;
};
} // namespace gl
