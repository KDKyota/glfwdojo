#pragma once

#include "HeightField.h"
#include <memory>
#include "TerrainMesh.h"
#include "gl/Shader.h"
#include "gl/TextureCache.h"
#include "asset/Mesh.h"
#include "SceneLayout.h"

namespace gl
{

  struct TerrainData
  {
    TerrainData(const layout::HeightMapSpawn &spawn, gl::PbrMaterial material);
    HeightField heightField;
    Mesh mesh;
  };

  class SceneTerrain
  {
  public:
    explicit SceneTerrain(TextureCache &cache);
    static Mesh buildMesh(HeightField &field, PbrMaterial material, const layout::HeightMapSpawn &spawn);
    void Draw(Shader &shader) const;
    std::vector<const HeightField *> getHeightFields() const;

    /**
     * @brief シャドウマップに描く地形をライト影響範囲に重なるものに絞る
     * @param shader シェーダー
     * @param lightPos 光源の位置
     * @param attenuationRadius 光が及ぶ範囲
     */
    void DrawIntersectWithSphere(Shader &shader, glm::vec3 lightPos, float attenuationRadius) const;

  private:
    std::vector<std::unique_ptr<TerrainData>> terrains_;
  };
} // namespace gl
