#include "loader.h"
#include <QFile>
#include <QFileInfo>
#include <cmath>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include <glm/glm.hpp>

Loader::Loader()
{}

Loader::~Loader()
{}

namespace
{
struct ObjFaceIndex
{
    int vertexIndex = 0;
    int uvIndex = 0;
    int normalIndex = 0;
};

static bool parseFaceToken(const std::string& token, ObjFaceIndex& out)
{
    // Supported OBJ face token formats:
    // v
    // v/vt
    // v//vn
    // v/vt/vn
    out = ObjFaceIndex{};

    if (token.empty()) {
        return false;
    }

    const size_t firstSlash = token.find('/');

    if (firstSlash == std::string::npos) {
        out.vertexIndex = std::atoi(token.c_str());
        return out.vertexIndex != 0;
    }

    const std::string vertexPart = token.substr(0, firstSlash);
    out.vertexIndex = std::atoi(vertexPart.c_str());

    const size_t secondSlash = token.find('/', firstSlash + 1);

    if (secondSlash == std::string::npos) {
        const std::string uvPart = token.substr(firstSlash + 1);
        if (!uvPart.empty()) {
            out.uvIndex = std::atoi(uvPart.c_str());
        }
        return out.vertexIndex != 0;
    }

    const std::string uvPart = token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
    const std::string normalPart = token.substr(secondSlash + 1);

    if (!uvPart.empty()) {
        out.uvIndex = std::atoi(uvPart.c_str());
    }

    if (!normalPart.empty()) {
        out.normalIndex = std::atoi(normalPart.c_str());
    }

    return out.vertexIndex != 0;
}

static int resolveObjIndex(int index, int count)
{
    // OBJ indices are 1-based. Negative indices are relative to the end.
    if (index > 0) {
        return index - 1;
    }

    if (index < 0) {
        return count + index;
    }

    return -1;
}
}

bool Loader::loadOBJ(const char* path)
{
    printf("Loading OBJ file %s...\n", path);

    vertices.clear();
    uvs.clear();
    normals.clear();

    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;

    QFile input(QString::fromUtf8(path));
    if (QFileInfo(input).size() > 256LL*1024*1024 || !input.open(QIODevice::ReadOnly)) return false;
    // QFile supports Unicode Windows paths and Qt resources.

    char line[2048];

    while (!input.atEnd() && input.readLine(line, sizeof(line)) > 0) {
        std::string currentLine(line);
        if (currentLine.size() == sizeof(line)-1 && currentLine.back() != '\n' && !input.atEnd()) return false;

        if (currentLine.empty() || currentLine[0] == '#') {
            continue;
        }

        std::istringstream stream(currentLine);
        std::string header;
        stream >> header;

        if (header == "v") {
            glm::vec3 vertex(0.0f);
            stream >> vertex.x >> vertex.y >> vertex.z;
            if (!stream || !std::isfinite(vertex.x) || !std::isfinite(vertex.y) || !std::isfinite(vertex.z)) return false;
            temp_vertices.push_back(vertex);
        } else if (header == "vt") {
            glm::vec2 uv(0.0f);
            stream >> uv.x >> uv.y;
            if (!stream || !std::isfinite(uv.x) || !std::isfinite(uv.y)) return false;

            // OBJ UV origin is usually bottom-left, while image data is often treated top-left.
            // Flip V once here so the renderer receives ready-to-use texture coordinates.
            uv.y = 1.0f - uv.y;

            temp_uvs.push_back(uv);
        } else if (header == "vn") {
            glm::vec3 normal(0.0f);
            stream >> normal.x >> normal.y >> normal.z;
            if (!stream || !std::isfinite(normal.x) || !std::isfinite(normal.y) || !std::isfinite(normal.z)) return false;
            temp_normals.push_back(normal);
        } else if (header == "f") {
            std::vector<ObjFaceIndex> faceIndices;
            std::string token;

            while (stream >> token) {
                ObjFaceIndex faceIndex;
                if (parseFaceToken(token, faceIndex)) {
                    faceIndices.push_back(faceIndex);
                }
            }

            if (faceIndices.size() < 3) {
                printf("Invalid face line skipped: %s", line);
                continue;
            }

            // Triangulate polygons using fan triangulation:
            // f 1 2 3 4 -> triangles (1,2,3), (1,3,4)
            for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
                const ObjFaceIndex triangle[3] = {
                    faceIndices[0],
                    faceIndices[i],
                    faceIndices[i + 1]
                };

                for (const ObjFaceIndex& index : triangle) {
                    const int vertexArrayIndex = resolveObjIndex(index.vertexIndex, static_cast<int>(temp_vertices.size()));
                    if (vertexArrayIndex < 0 || vertexArrayIndex >= static_cast<int>(temp_vertices.size())) {
                        printf("Invalid vertex index in OBJ face.\n");
                        input.close();
                        return false;
                    }

                    if (vertices.size() >= 12000000) return false;
                    vertices.push_back(temp_vertices[vertexArrayIndex]);

                    if (index.uvIndex != 0) {
                        const int uvArrayIndex = resolveObjIndex(index.uvIndex, static_cast<int>(temp_uvs.size()));
                        if (uvArrayIndex >= 0 && uvArrayIndex < static_cast<int>(temp_uvs.size())) {
                            uvs.push_back(temp_uvs[uvArrayIndex]);
                        } else {
                            uvs.push_back(glm::vec2(0.0f, 0.0f));
                        }
                    } else {
                        uvs.push_back(glm::vec2(0.0f, 0.0f));
                    }

                    if (index.normalIndex != 0) {
                        const int normalArrayIndex = resolveObjIndex(index.normalIndex, static_cast<int>(temp_normals.size()));
                        if (normalArrayIndex >= 0 && normalArrayIndex < static_cast<int>(temp_normals.size())) {
                            normals.push_back(temp_normals[normalArrayIndex]);
                        } else {
                            normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
                        }
                    } else {
                        normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
                    }
                }
                // Imported scans frequently omit normals. Give them real surface lighting.
                const size_t start = vertices.size()-3;
                auto normal = glm::cross(vertices[start+1]-vertices[start],vertices[start+2]-vertices[start]);
                const float length = glm::length(normal);
                normal = length > 1e-12f ? normal/length : glm::vec3(0,1,0);
                for (int corner=0;corner<3;++corner)
                    if (triangle[corner].normalIndex == 0) normals[start+corner] = normal;
            }
        }
    }

    input.close();

    printf("OBJ loaded. Vertices: %zu, UVs: %zu, Normals: %zu\n",
           vertices.size(),
           uvs.size(),
           normals.size());

    if (vertices.empty()) {
        printf("OBJ loaded but contains no renderable vertices.\n");
        return false;
    }

    minimum = maximum = vertices.front();
    for (const auto& v : vertices) { minimum = glm::min(minimum,v); maximum = glm::max(maximum,v); }
    return true;
}
