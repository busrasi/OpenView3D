#pragma once
#include "SceneAssetManager.h"
class SceneBuilder {
public:
    static SceneData build(const SceneSpec& spec, const SelectedModelSnapshot& hero,
                           const QString& jobId, const SceneAssetManager& assets);
};
