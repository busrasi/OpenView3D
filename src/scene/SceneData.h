#pragma once
#include "SceneSpec.h"
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector>
#include <QMetaType>
#include <QHash>
#include <memory>
#include <vector>

// Interleaved position/normal CPU geometry. Stable IDs identify immutable assets.
struct SceneGeometry { QString id; std::vector<float> vertices; };
using SceneGeometryPtr = std::shared_ptr<const SceneGeometry>;
struct SceneMaterial { QVector3D color; float specular = .1f, shininess = 32; };
struct SceneLight { QVector3D direction, color; float ambient = .4f, diffuse = .7f; };
struct SceneObject { QString id, asset; QMatrix4x4 transform; SceneMaterial material; };
struct SelectedModelSnapshot {
    QString path, name;
    QVector3D minimum, maximum;
    QMatrix4x4 transform;
    quint64 revision = 0;
};
struct SceneData {
    QString id, savedPath;
    SceneSpec spec;
    QVector<SceneObject> objects;
    QHash<QString, SceneGeometryPtr> assets;
    QMatrix4x4 heroTransform;
    QVector3D minimum, maximum, heroMinimum, heroMaximum, cameraTarget;
    float cameraRadius = 1;
    SceneLight light;
};
using ScenePtr = std::shared_ptr<const SceneData>;
Q_DECLARE_METATYPE(ScenePtr)
