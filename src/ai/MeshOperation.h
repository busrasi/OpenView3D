#pragma once
#include <QJsonObject>
#include <QString>
#include <cmath>
#include <stdexcept>

// Radius is a fraction of the smaller XY bounding-box dimension, in object space.
// Only this allowlisted operation can cross the provider/service boundary.
struct MeshOperation {
    double radius = 0.1;
    static MeshOperation fromJson(const QJsonObject& json) {
        if (json.size() != 6 || json.value("operation") != "boolean_subtract"
            || json.value("primitive") != "cylinder" || json.value("placement") != "center"
            || json.value("axis") != "z" || !json.value("through").isBool()
            || !json.value("through").toBool() || !json.value("radius").isDouble())
            throw std::runtime_error("Unsupported mesh command. Only a centered Z-axis through-hole is supported.");
        const double radius = json.value("radius").toDouble();
        if (!std::isfinite(radius) || radius < 0.01 || radius > 0.4)
            throw std::runtime_error("Hole radius must be between 0.01 and 0.4 of the smaller XY dimension.");
        return {radius};
    }
    QJsonObject toJson() const {
        return {{"operation", "boolean_subtract"}, {"primitive", "cylinder"},
                {"placement", "center"}, {"axis", "z"}, {"radius", radius}, {"through", true}};
    }
};
