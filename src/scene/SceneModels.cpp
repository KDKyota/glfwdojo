#include "scene/SceneModels.h"

#include "scene/SceneLayout.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace gl {

SceneModels::SceneModels(TextureCache &cache) {
    for (const layout::ModelSpawn &spawn : layout::modelSpawns) {
        try {
            models_.push_back(std::make_unique<Model>(spawn.path, cache));
        } catch (const std::exception &e) {
            std::cout << "Skipped model: " << spawn.path << " (" << e.what() << ")" << std::endl;
            continue;
        }
        glm::mat4 orientation = glm::rotate(glm::mat4(1.0f), glm::radians(spawn.rotationDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
        orientation = glm::rotate(orientation, glm::radians(spawn.rotationDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
        orientation = glm::rotate(orientation, glm::radians(spawn.rotationDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f));
        orientation = glm::scale(orientation, glm::vec3(spawn.scale));

        // 注意: T * R * S の順を崩すとモデルが原点まわりに公転する
        modelMatrices_.push_back(glm::translate(glm::mat4(1.0f), spawn.position) * orientation);

        if (spawn.followTarget) {
            playerModelIndex_ = static_cast<int>(models_.size()) - 1;
            playerBaseTransform_ = orientation;
            character_ = std::make_unique<Character>(spawn.position, models_.back()->Height() * spawn.scale);
        }
    }
}

void SceneModels::UpdateAnimation(float deltaTime) {
    for (size_t i = 0; i < models_.size(); ++i) {
        // 待機モーションが無いので停止中は再生位置を進めない
        const bool freeze = character_ && static_cast<int>(i) == playerModelIndex_ && !character_->IsMoving();
        models_[i]->UpdateAnimation(freeze ? 0.0f : deltaTime);
    }
}

void SceneModels::SyncPlayerMatrix() {
    if (!character_ || playerModelIndex_ < 0)
        return;

    modelMatrices_[playerModelIndex_] =
        glm::translate(glm::mat4(1.0f), character_->Position()) *
        glm::rotate(glm::mat4(1.0f), character_->Yaw(), glm::vec3(0.0f, 1.0f, 0.0f)) * playerBaseTransform_;
}

void SceneModels::Draw(Shader &shader) const {
    for (size_t i = 0; i < models_.size(); ++i)
        models_[i]->Draw(shader, modelMatrices_[i]);
}

} // namespace gl
