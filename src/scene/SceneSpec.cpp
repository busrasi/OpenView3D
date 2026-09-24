#include "SceneSpec.h"
#include <QJsonArray>
#include <cmath>
#include <stdexcept>

namespace {
QJsonObject choice(std::initializer_list<const char*> values) {
    QJsonArray a; for (auto v : values) a.append(QString::fromLatin1(v));
    return {{"type", "string"}, {"enum", a}};
}
QJsonObject object(const QJsonObject& properties) {
    QJsonArray required; for (auto it = properties.begin(); it != properties.end(); ++it) required.append(it.key());
    return {{"type", "object"}, {"properties", properties}, {"required", required}, {"additionalProperties", false}};
}
void validate(const QJsonValue& value, const QJsonObject& schema, const QString& path) {
    const auto fail = [&] { throw std::runtime_error(("Invalid SceneSpec value at " + path).toStdString()); };
    const auto type = schema.value("type").toString();
    if (type == "object") {
        if (!value.isObject()) fail();
        const auto o = value.toObject(), properties = schema.value("properties").toObject();
        if (o.size() != properties.size()) fail();
        for (auto it = properties.begin(); it != properties.end(); ++it)
            validate(o.value(it.key()), it.value().toObject(), path + "." + it.key());
    } else if (type == "string") {
        if (!value.isString() || !schema.value("enum").toArray().contains(value)) fail();
    } else if (!value.isDouble() || !std::isfinite(value.toDouble())
               || value.toDouble() < schema.value("minimum").toDouble()
               || value.toDouble() > schema.value("maximum").toDouble()) fail();
}
}
QJsonObject SceneSpec::schema() {
    const QJsonObject span{{"type", "number"}, {"minimum", 3}, {"maximum", 20}};
    return object({{"type", choice({"interior"})}, {"style", choice({"modern", "minimalist", "cozy", "scandinavian"})},
        {"room", object({{"width", span}, {"depth", span}, {"height", QJsonObject{{"type", "number"}, {"minimum", 2.4}, {"maximum", 6}}}})},
        {"surface", object({{"type", choice({"table", "pedestal"})}, {"material", choice({"wood", "white_wall", "neutral_floor", "metal", "glass", "ceramic"})}, {"placement", choice({"room_center"})}})},
        {"lighting", object({{"preset", choice({"soft_daylight", "warm_interior", "studio_product"})}})},
        {"camera", object({{"preset", choice({"product_focus", "wide_scene", "close_product"})}})},
        {"heroModel", object({{"source", choice({"selected_model"})}, {"placement", choice({"surface_center"})}, {"scaleMode", choice({"fit"})}})}});
}
QJsonObject SceneSpec::toJson() const {
    return {{"type", "interior"}, {"style", style}, {"room", QJsonObject{{"width", width}, {"depth", depth}, {"height", height}}},
        {"surface", QJsonObject{{"type", surface}, {"material", material}, {"placement", "room_center"}}},
        {"lighting", QJsonObject{{"preset", lighting}}}, {"camera", QJsonObject{{"preset", camera}}},
        {"heroModel", QJsonObject{{"source", "selected_model"}, {"placement", "surface_center"}, {"scaleMode", "fit"}}}};
}
SceneSpec SceneSpec::fromJson(const QJsonObject& j) {
    validate(j, schema(), "scene");
    SceneSpec s; s.style = j["style"].toString();
    const auto r = j["room"].toObject(); s.width = r["width"].toDouble(); s.depth = r["depth"].toDouble(); s.height = r["height"].toDouble();
    s.surface = j["surface"].toObject()["type"].toString(); s.material = j["surface"].toObject()["material"].toString();
    s.lighting = j["lighting"].toObject()["preset"].toString(); s.camera = j["camera"].toObject()["preset"].toString();
    return s;
}
