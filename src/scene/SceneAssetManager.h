#pragma once
#include "SceneData.h"
#include <vector>
// Providers return CPU descriptors only; GPU ownership stays in ViewportRenderer.
class GeneratedAssetProvider {
public:
    virtual ~GeneratedAssetProvider() = default;
    virtual bool supports(const QString& asset) const = 0;
    virtual SceneGeometryPtr resolve(const QString& asset) const = 0;
};
class ProceduralAssetProvider final : public GeneratedAssetProvider {
public:
    bool supports(const QString& asset) const override { return asset == "unit_box"; }
    SceneGeometryPtr resolve(const QString& asset) const override;
};
class SceneAssetManager {
public:
    SceneAssetManager();
    void addProvider(std::shared_ptr<GeneratedAssetProvider> provider);
    SceneGeometryPtr resolve(const QString& asset) const;
    static QString save(const SceneData& scene, const SelectedModelSnapshot& hero);
private:
    std::vector<std::shared_ptr<GeneratedAssetProvider>> m_providers;
};
