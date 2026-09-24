#include "ScenePresets.h"
#include <stdexcept>
SceneMaterial ScenePresets::material(const QString& name, const QString& style) {
    if (name == "wood") return {style == "scandinavian" ? QVector3D(.76f,.61f,.42f) : QVector3D(.46f,.25f,.10f), .15f, 40};
    if (name == "white_wall") return {style == "cozy" ? QVector3D(.85f,.73f,.59f) : QVector3D(.88f,.88f,.85f), .02f, 8};
    if (name == "neutral_floor") return {QVector3D(.51f,.49f,.45f), .05f, 16};
    if (name == "metal") return {QVector3D(.43f,.47f,.52f), .7f, 110};
    if (name == "glass") return {QVector3D(.55f,.76f,.8f), .9f, 180};
    if (name == "ceramic") return {QVector3D(.8f,.84f,.85f), .3f, 75};
    throw std::runtime_error("Unsupported scene material.");
}
SceneLight ScenePresets::lighting(const QString& name) {
    if (name == "warm_interior") return {{-.4f,.8f,.5f}, {1,.81f,.61f}, .45f,.65f};
    if (name == "studio_product") return {{-.5f,.6f,.8f}, {1,1,1}, .3f,.85f};
    if (name == "soft_daylight") return {{-.45f,.75f,.55f}, {.94f,.97f,1}, .45f,.65f};
    throw std::runtime_error("Unsupported lighting preset.");
}
void ScenePresets::camera(SceneData& s) {
    if (s.spec.camera == "wide_scene") {
        s.cameraTarget = (s.minimum + s.maximum) * .5f;
        s.cameraRadius = (s.maximum - s.minimum).length() * .5f;
    } else {
        s.cameraTarget = (s.heroMinimum + s.heroMaximum) * .5f;
        s.cameraRadius = (s.heroMaximum - s.heroMinimum).length() * (s.spec.camera == "close_product" ? .65f : 1.9f);
        if (s.spec.camera == "product_focus") s.cameraTarget.setY(s.cameraTarget.y() * .85f);
    }
}
