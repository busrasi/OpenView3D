#include "RequestRouter.h"
#include <QRegularExpression>
RequestRouter::Intent RequestRouter::route(const QString& prompt, bool hasScene) {
    const auto p = prompt.toLower().trimmed();
    if (QRegularExpression("\\b(don't|do not|never)\\b").match(p).hasMatch()) return Unsupported;
    if (QRegularExpression("^(what|which) (model|object)|^what.*selected").match(p).hasMatch()) return ModelQuery;
    if (QRegularExpression("^(clear|remove) (the )?generated (scene|environment)[.!]?$" ).match(p).hasMatch()) return ClearScene;
    if (QRegularExpression("\\b(hole|cut|subtract|mesh)\\b").match(p).hasMatch()) return MeshEdit;
    const bool action = QRegularExpression("^(please )?(show|put|place|create|make|stage|change|generate|regenerate)\\b").match(p).hasMatch();
    if (action && (QRegularExpression("\\b(room|table|pedestal|environment|living room|scene)\\b").match(p).hasMatch()
        || (hasScene && QRegularExpression("\\b(minimalist|modern|cozy|scandinavian|lighting|camera)\\b").match(p).hasMatch()))) return SceneGeneration;
    return Unsupported;
}
QString RequestRouter::name(Intent i) {
    switch(i) { case MeshEdit: return "MESH_EDIT"; case SceneGeneration: return "SCENE_GENERATION";
    case ModelQuery: return "MODEL_QUERY"; case ClearScene: return "CLEAR_SCENE"; default: return "UNSUPPORTED"; }
}
