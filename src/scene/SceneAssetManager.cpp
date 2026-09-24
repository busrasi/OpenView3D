#include "SceneAssetManager.h"
#include <QDir>
#include <QStandardPaths>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <stdexcept>
#include <cmath>
SceneGeometryPtr ProceduralAssetProvider::resolve(const QString& asset) const {
    if (!supports(asset)) throw std::runtime_error("Unsupported procedural asset.");
    static const SceneGeometryPtr box = [] {
        auto geometry = std::make_shared<SceneGeometry>(); geometry->id = "procedural:unit_box:v1";
        const QVector3D normals[] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
        for (auto n : normals) {
            const QVector3D u = std::abs(n.y()) > .5f ? QVector3D(1,0,0) : QVector3D(0,1,0);
            const auto v = QVector3D::crossProduct(n,u);
            const QVector3D corners[] = {(n-u-v)*.5f,(n+u-v)*.5f,(n+u+v)*.5f,(n-u+v)*.5f};
            for (int i : {0,1,2,0,2,3}) {
                const auto p = corners[i];
                geometry->vertices.insert(geometry->vertices.end(),{p.x(),p.y(),p.z(),n.x(),n.y(),n.z()});
            }
        }
        return geometry;
    }();
    return box;
}
SceneAssetManager::SceneAssetManager() { addProvider(std::make_shared<ProceduralAssetProvider>()); }
void SceneAssetManager::addProvider(std::shared_ptr<GeneratedAssetProvider> p) { m_providers.insert(m_providers.begin(), std::move(p)); }
SceneGeometryPtr SceneAssetManager::resolve(const QString& asset) const {
    for (const auto& p : m_providers) if (p->supports(asset)) {
        const auto geometry = p->resolve(asset);
        if (!geometry || geometry->id.isEmpty() || geometry->vertices.empty()
            || geometry->vertices.size() % 18 != 0 || geometry->vertices.size() > 6*1000000)
            throw std::runtime_error("Asset provider returned invalid or excessively large geometry.");
        for (float value : geometry->vertices) if (!std::isfinite(value))
            throw std::runtime_error("Asset provider returned non-finite geometry.");
        return geometry;
    }
    throw std::runtime_error("No provider supports this scene asset.");
}
QString SceneAssetManager::save(const SceneData& s, const SelectedModelSnapshot& hero) {
    QDir root(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    for (const auto* folder : {"generated/scenes", "generated/models", "generated/textures"})
        if (!root.mkpath(folder)) throw std::runtime_error("Cannot create generated asset storage.");
    const auto vector = [](QVector3D v) { return QJsonArray{v.x(),v.y(),v.z()}; };
    QJsonArray transform; for (int i=0;i<16;++i) transform.append(s.heroTransform.constData()[i]);
    QJsonObject document{{"version", 1}, {"generator", "procedural-room-v1"}, {"id", s.id}, {"sceneSpec", s.spec.toJson()},
        {"hero", QJsonObject{{"path", hero.path}, {"name", hero.name}, {"minimum", vector(hero.minimum)},
            {"maximum", vector(hero.maximum)}, {"transformColumnMajor", transform}}}};
    const QString path = root.filePath("generated/scenes/" + s.id + ".json");
    QSaveFile file(path);
    const auto bytes = QJsonDocument(document).toJson();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        throw std::runtime_error("Cannot save generated SceneSpec.");
    return path;
}
