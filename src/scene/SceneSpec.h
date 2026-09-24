#pragma once
#include <QJsonObject>
#include <QString>

// All units are metres, Y is up. The provider never supplies paths or geometry.
struct SceneSpec {
    QString style = "modern", surface = "table", material = "wood";
    QString lighting = "soft_daylight", camera = "product_focus";
    float width = 6, depth = 5, height = 3;
    QJsonObject toJson() const;
    static SceneSpec fromJson(const QJsonObject& json);
    static QJsonObject schema();
};
