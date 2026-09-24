#include "SceneBuilder.h"
#include "ScenePresets.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
SceneData SceneBuilder::build(const SceneSpec& input, const SelectedModelSnapshot& hero,
                              const QString& id, const SceneAssetManager& assets) {
    const auto s = SceneSpec::fromJson(input.toJson());
    const auto size = hero.maximum - hero.minimum;
    for (int i=0;i<3;++i) if (!std::isfinite(size[i]) || size[i] < 0 || !std::isfinite(hero.minimum[i]) || !std::isfinite(hero.maximum[i]))
        throw std::runtime_error("Selected model has invalid bounds.");
    const float largest = std::max({size.x(),size.y(),size.z()});
    if (largest < 1e-8f) throw std::runtime_error("Selected model has empty bounds.");
    SceneData out; out.id = id; out.spec = s;
    const auto box = assets.resolve("unit_box");
    out.assets.insert(box->id,box);
    auto add = [&](const QString& name, QVector3D center, QVector3D dimensions, const QString& material) {
        SceneObject o; o.id = id + "/" + name; o.asset = box->id;
        o.transform.translate(center); o.transform.scale(dimensions);
        o.material = ScenePresets::material(material,s.style); out.objects.append(o);
    };
    add("floor", {0,-.06f,0}, {s.width,.12f,s.depth}, "neutral_floor");
    add("back_wall", {0,s.height/2,-s.depth/2}, {s.width,s.height,.12f}, "white_wall");
    add("left_wall", {-s.width/2,s.height/2,0}, {.12f,s.height,s.depth}, "white_wall");
    const bool pedestal = s.surface == "pedestal";
    const float top = pedestal ? 1.05f : .8f;
    const float w = pedestal ? .85f : std::min(2.1f,s.width*.45f);
    const float d = pedestal ? .85f : std::min(1.3f,s.depth*.4f);
    if (pedestal) add("pedestal", {0,top/2,0}, {w,top,d}, s.material);
    else {
        add("table_top", {0,top-.05f,0}, {w,.1f,d}, s.material);
        for (int x : {-1,1}) for (int z : {-1,1})
            add(QString("leg_%1_%2").arg(x).arg(z), {x*(w/2-.12f),(top-.1f)/2,z*(d/2-.12f)}, {.12f,top-.1f,.12f},s.material);
    }
    if (s.style == "cozy") {
        add("rug", {0,.012f,0}, {s.width*.66f,.02f,s.depth*.65f}, "wood");
        add("bench", {s.width*.3f,.22f,-s.depth*.2f}, {.65f,.44f,1.3f}, "ceramic");
    }
    // Uniform fit using all dimensions. Translate the actual source minimum to the surface.
    const float scale = std::min({.9f/largest, (w*.75f)/std::max(size.x(),1e-8f), (d*.75f)/std::max(size.z(),1e-8f)});
    const auto center = (hero.minimum + hero.maximum)*.5f;
    out.heroTransform.translate(-center.x()*scale, top-hero.minimum.y()*scale,-center.z()*scale);
    out.heroTransform.scale(scale);
    out.heroMinimum = out.heroTransform.map(hero.minimum); out.heroMaximum = out.heroTransform.map(hero.maximum);
    out.minimum = {-s.width/2-.06f,-.12f,-s.depth/2-.06f}; out.maximum = {s.width/2,s.height,s.depth/2};
    out.light = ScenePresets::lighting(s.lighting); ScenePresets::camera(out);
    return out;
}
