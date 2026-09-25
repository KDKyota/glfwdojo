#include "asset/MeshDistanceField.h"

#include "sdf/DistanceFieldBuilder.h"
#include <vector>

namespace gl {

BakedDistanceField BakeDistanceField(const Mesh &mesh, int resolution) {
    const glm::vec3 extent = mesh.BoundsMax() - mesh.BoundsMin();
    const glm::vec3 margin = extent * kBoundsMarginRatio;

    BakedDistanceField baked;
    baked.resolution = resolution;
    baked.boundsMin = mesh.BoundsMin() - margin;
    baked.boundsMax = mesh.BoundsMax() + margin;

    baked.values =
        BuildSignedDistanceField(mesh.Vertices(), mesh.Indices(), resolution, baked.boundsMin, baked.boundsMax);

    return baked;
}
MeshDistanceField UploadMeshDistanceField(const BakedDistanceField &baked) {
    MeshDistanceField result;
    result.boundsMin = baked.boundsMin;
    result.boundsMax = baked.boundsMax;

    result.texture.create();
    glBindTexture(GL_TEXTURE_3D, result.texture);
    // 負の値（内側）を保持する必要があるため浮動小数点フォーマット　1チャンネルで足りる
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R16F, baked.resolution, baked.resolution, baked.resolution, 0, GL_RED, GL_FLOAT,
                 baked.values.data());
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_3D, 0);

    return result;
}

} // namespace gl
