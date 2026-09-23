#include "MeshEditingService.h"
#include <manifold/manifold.h>
#include <manifold/polygon.h>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QRegularExpression>
#include <QLocale>
#include <array>
#include <algorithm>
#include <map>
#include <limits>

namespace {
using namespace manifold;
struct ObjMesh {
    MeshGL64 mesh;
    QStringList libraries;
    std::vector<QString> materials;
};
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
int indexOf(const QString& token, size_t count) {
    bool ok = false;
    int index = token.toInt(&ok);
    require(ok && index != 0, "Invalid OBJ index.");
    index = index > 0 ? index - 1 : int(count) + index;
    require(index >= 0 && size_t(index) < count, "OBJ index is out of range.");
    return index;
}
double number(const QString& token) {
    bool ok = false;
    double value = token.toDouble(&ok);
    require(ok && std::isfinite(value), "OBJ contains an invalid coordinate.");
    return value;
}
ObjMesh readObj(QFile& file) {
    ObjMesh result;
    result.mesh.numProp = 5; // XYZ + UV, interpolated by the boolean library.
    std::vector<vec3> positions;
    std::vector<vec2> uvs;
    size_t normalCount = 0;
    std::map<std::pair<int, int>, uint64_t> corners;
    std::map<int, uint64_t> firstCorner;
    QString material;
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QString line = stream.readLine().section('#', 0, 0).trimmed();
        if (line.isEmpty()) continue;
        const auto parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        const auto tag = parts[0];
        if (tag == "v") {
            require(parts.size() == 4 || parts.size() == 5, "Unsupported OBJ vertex format (expected XYZ and optional W).");
            double w = parts.size() == 5 ? number(parts[4]) : 1.0;
            require(w != 0, "OBJ vertex has zero homogeneous weight.");
            positions.push_back({number(parts[1])/w, number(parts[2])/w, number(parts[3])/w});
        } else if (tag == "vt") {
            require(parts.size() >= 2 && parts.size() <= 4, "Invalid OBJ texture coordinate.");
            uvs.push_back({number(parts[1]), parts.size() > 2 ? number(parts[2]) : 0});
        } else if (tag == "vn") {
            require(parts.size() == 4, "Invalid OBJ normal.");
            for (int i = 1; i < 4; ++i) number(parts[i]);
            ++normalCount;
        } else if (tag == "mtllib") {
            result.libraries.append(line);
        } else if (tag == "usemtl") {
            material = line.mid(6).trimmed();
        } else if (tag == "f") {
            require(parts.size() >= 4, "OBJ face has fewer than three vertices.");
            std::vector<vec3> polygon;
            std::vector<uint64_t> ids;
            for (qsizetype i = 1; i < parts.size(); ++i) {
                auto fields = parts[i].split('/');
                require(fields.size() <= 3, "Invalid OBJ face token.");
                int v = indexOf(fields[0], positions.size());
                int uv = fields.size() > 1 && !fields[1].isEmpty() ? indexOf(fields[1], uvs.size()) : -1;
                if (fields.size() == 3 && !fields[2].isEmpty()) indexOf(fields[2], normalCount);
                const auto key = std::make_pair(v, uv);
                auto it = corners.find(key);
                if (it == corners.end()) {
                    uint64_t id = result.mesh.NumVert();
                    const auto p = positions[v];
                    const auto t = uv < 0 ? vec2{0, 0} : uvs[uv];
                    result.mesh.vertProperties.insert(result.mesh.vertProperties.end(), {p.x, p.y, p.z, t.x, t.y});
                    it = corners.emplace(key, id).first;
                    auto first = firstCorner.emplace(v, id);
                    if (!first.second) {
                        result.mesh.mergeFromVert.push_back(id);
                        result.mesh.mergeToVert.push_back(first.first->second);
                    }
                }
                ids.push_back(it->second);
                polygon.push_back(positions[v]);
            }
            std::vector<ivec3> triangles;
            if (ids.size() == 3) {
                triangles.push_back({0, 1, 2});
            } else {
                // Project a planar polygon onto its dominant plane; Manifold triangulates concave faces.
                vec3 normal{0, 0, 0};
                for (size_t i = 0; i < polygon.size(); ++i)
                    normal += la::cross(polygon[i], polygon[(i + 1) % polygon.size()]);
                require(la::length(normal) > 0, "Degenerate OBJ polygon.");
                int drop = std::abs(normal.x) > std::abs(normal.y) ? 0 : 1;
                if (std::abs(normal.z) > std::abs(normal[drop])) drop = 2;
                SimplePolygon projected;
                for (auto p : polygon) {
                    require(std::abs(la::dot(p - polygon[0], normal)) <= 1e-7 * la::length(normal) * std::max(1.0, la::length(p - polygon[0])),
                            "Non-planar OBJ polygon: triangulate the source mesh before editing.");
                    projected.push_back({p[(drop + 1) % 3], p[(drop + 2) % 3]});
                }
                bool reversed = normal[drop] < 0;
                if (reversed) std::reverse(projected.begin(), projected.end());
                triangles = Triangulate({projected});
                if (reversed) {
                    for (auto& t : triangles) {
                        for (int j = 0; j < 3; ++j) t[j] = int(ids.size()) - 1 - t[j];
                        std::swap(t[1], t[2]);
                    }
                }
            }
            for (auto t : triangles) {
                for (int j = 0; j < 3; ++j) result.mesh.triVerts.push_back(ids.at(t[j]));
                result.mesh.faceID.push_back(result.materials.size());
                result.materials.push_back(material);
            }
        } else if (tag != "o" && tag != "g" && tag != "s") {
            require(false, "Unsupported OBJ records: only polygon meshes can be edited.");
        }
    }
    require(!result.mesh.triVerts.empty(), "The selected OBJ contains no faces.");
    // Rejoin exact/near-exact position seams, common in exported OBJ solids.
    result.mesh.Merge();
    return result;
}
}

MeshEditResult MeshEditingService::apply(const QString& selectedObjPath, const MeshOperation& operation) {
    MeshEditResult result;
    try {
        MeshOperation::fromJson(operation.toJson());
        QFileInfo info(selectedObjPath);
        require(info.isFile() && info.suffix().compare("obj", Qt::CaseInsensitive) == 0,
                "Select a local OBJ file before editing. Embedded resources must first be saved to disk.");
        QFile source(info.absoluteFilePath());
        require(source.size() <= 256 * 1024 * 1024, "OBJ exceeds the 256 MB editing limit.");
        require(source.open(QIODevice::ReadOnly | QIODevice::Text), "Cannot read the selected OBJ.");
        auto obj = readObj(source);
        source.close();
        const uint32_t originalID = Manifold::ReserveIDs(1);
        obj.mesh.runOriginalID = {originalID};
        obj.mesh.runIndex = {0, obj.mesh.triVerts.size()};
        Manifold solid(obj.mesh);
        require(solid.Status() == Manifold::Error::NoError,
                "OBJ is not a closed, consistently oriented manifold mesh. Repair it before boolean editing.");
        const auto bounds = solid.BoundingBox();
        const auto size = bounds.Size();
        const auto center = bounds.Center();
        require(size.x > 0 && size.y > 0 && size.z > 0, "OBJ must enclose a three-dimensional solid.");
        result.originalVolume = solid.Volume();
        require(result.originalVolume > 0, "OBJ has no positive enclosed volume. Check face winding.");
        const double radius = operation.radius * std::min(size.x, size.y);
        const double margin = std::max({size.x, size.y, size.z}) * 0.1;
        const auto cutter = Manifold::Cylinder(size.z + 2 * margin, radius, radius, 64, true).Translate(center);
        auto edited = solid - cutter;
        require(edited.Status() == Manifold::Error::NoError && !edited.IsEmpty(), "Boolean subtraction failed or removed the entire model.");
        result.editedVolume = edited.Volume();
        require(result.originalVolume - result.editedVolume > result.originalVolume * 1e-10,
                "The center cutter removes no material. The model may already have a hole there.");
        const auto mesh = edited.GetMeshGL64();
        // Build the complete OBJ before reserving an output filename; no partially generated geometry.
        QString output;
        QTextStream out(&output);
        out.setLocale(QLocale::c());
        out.setRealNumberPrecision(17);
        out << "# OpenView3D: real centered cylinder boolean difference; object-space Z\n";
        for (const auto& library : obj.libraries) out << library << '\n';
        out << "o AI_Edited\n";
        for (uint64_t i = 0; i < mesh.NumVert(); ++i) {
            size_t p = i * mesh.numProp;
            out << "v " << mesh.vertProperties[p] << ' ' << mesh.vertProperties[p+1] << ' ' << mesh.vertProperties[p+2] << '\n';
        }
        for (uint64_t i = 0; i < mesh.NumVert(); ++i) {
            size_t p = i * mesh.numProp;
            out << "vt " << mesh.vertProperties[p+3] << ' ' << mesh.vertProperties[p+4] << '\n';
        }
        auto position = [&mesh](uint64_t i) {
            size_t p = i * mesh.numProp;
            return vec3{mesh.vertProperties[p], mesh.vertProperties[p+1], mesh.vertProperties[p+2]};
        };
        for (size_t i = 0; i < mesh.triVerts.size(); i += 3) {
            auto n = la::cross(position(mesh.triVerts[i+1]) - position(mesh.triVerts[i]),
                               position(mesh.triVerts[i+2]) - position(mesh.triVerts[i]));
            const double length = la::length(n);
            require(length > 0, "Boolean returned a degenerate triangle.");
            n /= length;
            out << "vn " << n.x << ' ' << n.y << ' ' << n.z << '\n';
        }
        QString lastMaterial;
        size_t run = 0;
        out << "s off\n";
        for (size_t i = 0; i < mesh.triVerts.size(); i += 3) {
            while (run + 1 < mesh.runOriginalID.size() && i >= mesh.runIndex[run+1]) ++run;
            QString material;
            if (mesh.runOriginalID[run] == originalID && mesh.faceID[i/3] < obj.materials.size())
                material = obj.materials[mesh.faceID[i/3]];
            if (material != lastMaterial) {
                out << "usemtl " << (material.isEmpty() ? "off" : material) << '\n';
                lastMaterial = material;
            }
            out << "f";
            for (size_t j = 0; j < 3; ++j) {
                auto index = mesh.triVerts[i+j] + 1;
                out << ' ' << index << '/' << index << '/' << i/3 + 1;
            }
            out << '\n';
        }
        out.flush();
        const QByteArray bytes = output.toUtf8();
        // NewOnly atomically reserves a sibling file, including against concurrent writers.
        for (int suffix = 0; suffix < 10000; ++suffix) {
            const auto path = info.dir().absoluteFilePath(info.completeBaseName() + "_ai_edited"
                + (suffix ? "_" + QString::number(suffix) : QString()) + ".obj");
            QFile destination(path);
            if (!destination.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
                if (QFileInfo::exists(path)) continue;
                throw std::runtime_error("Cannot create edited OBJ beside the source. Check directory permissions.");
            }
            if (destination.write(bytes) != bytes.size() || !destination.flush()) {
                destination.remove(); // Only our newly created, incomplete output.
                throw std::runtime_error("Failed to write edited OBJ.");
            }
            destination.close();
            result.outputPath = path;
            return result;
        }
        throw std::runtime_error("No available output filename.");
    } catch (const std::exception& error) {
        result.error = QString::fromUtf8(error.what());
    }
    return result;
}
