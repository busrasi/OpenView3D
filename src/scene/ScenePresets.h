#pragma once
#include "SceneData.h"
namespace ScenePresets {
SceneMaterial material(const QString& name, const QString& style);
SceneLight lighting(const QString& name);
void camera(SceneData& scene);
}
