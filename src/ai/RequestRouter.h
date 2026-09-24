#pragma once
#include <QString>
class RequestRouter {
public:
    enum Intent { MeshEdit, SceneGeneration, ModelQuery, ClearScene, Unsupported };
    static Intent route(const QString& prompt, bool hasScene);
    static QString name(Intent intent);
};
