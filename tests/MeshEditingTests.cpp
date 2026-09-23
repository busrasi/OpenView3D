#include "ai/ChatController.h"
#include "ai/MeshOperation.h"
#include "mesh/MeshEditingService.h"
#include "ui/AppController.h"
#include "core/loader.h"
#include <manifold/manifold.h>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QElapsedTimer>
#include <QThread>
#include <QDir>
#include <iostream>
#include <set>
#include <map>

void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
QByteArray read(const QString& path) {
    QFile f(path); check(f.open(QIODevice::ReadOnly), "Read failed"); return f.readAll();
}
void write(const QString& path, const QByteArray& bytes) {
    QFile f(path); check(f.open(QIODevice::WriteOnly), "Write failed"); check(f.write(bytes) == bytes.size(), "Write incomplete");
}
QByteArray cube() {
    // Quads, negative indices, UV seams, material references, off-origin/non-unit geometry.
    return "mtllib cube.mtl\no cube\n"
           "v 2 3 4\nv 4 3 4\nv 4 5 4\nv 2 5 4\nv 2 3 6\nv 4 3 6\nv 4 5 6\nv 2 5 6\n"
           "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nusemtl test\n"
           "f 4/1 3/2 2/3 1/4\nf 5/1 6/2 7/3 8/4\nf 1/1 2/2 6/3 5/4\n"
           "f 2/1 3/2 7/3 6/4\nf 3/1 4/2 8/3 7/4\nf -5/1 -8/2 -4/3 -1/4\n";
}
manifold::Manifold readSolid(const QString& path, int* euler = nullptr) {
    // Independent output reader, welding by exact coordinates for topology verification.
    manifold::MeshGL64 mesh;
    std::vector<uint64_t> mapping;
    std::map<std::array<double,3>, uint64_t> vertices;
    std::set<std::pair<uint64_t,uint64_t>> edges;
    QString text = QString::fromUtf8(read(path));
    QTextStream input(&text);
    while (!input.atEnd()) {
        const auto parts = input.readLine().split(' ', Qt::SkipEmptyParts);
        if (parts.isEmpty()) continue;
        if (parts[0] == "v") {
            std::array<double,3> p{parts[1].toDouble(), parts[2].toDouble(), parts[3].toDouble()};
            auto entry = vertices.emplace(p, vertices.size());
            if (entry.second) mesh.vertProperties.insert(mesh.vertProperties.end(), p.begin(), p.end());
            mapping.push_back(entry.first->second);
        } else if (parts[0] == "f") {
            check(parts.size() == 4, "Output must be triangulated");
            uint64_t indices[3];
            for (int i = 0; i < 3; ++i) {
                indices[i] = mapping.at(parts[i+1].section('/',0,0).toULongLong()-1);
                mesh.triVerts.push_back(indices[i]);
            }
            for (int i = 0; i < 3; ++i) edges.emplace(std::min(indices[i],indices[(i+1)%3]), std::max(indices[i],indices[(i+1)%3]));
        }
    }
    if (euler) *euler = int(vertices.size()) - int(edges.size()) + int(mesh.NumTri());
    return manifold::Manifold(mesh);
}
void waitForChat(ChatController& chat) {
    QElapsedTimer timer; timer.start();
    while (chat.busy() && timer.elapsed() < 15000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    check(!chat.busy(), "Chat timed out");
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    qunsetenv("OPENAI_API_KEY"); // Tests are offline regardless of the launching environment.
    try {
        if (argc == 3 && QString::fromLocal8Bit(argv[1]) == "--edit") {
            const auto result = MeshEditingService::apply(QString::fromLocal8Bit(argv[2]), {});
            if (!result.error.isEmpty()) throw std::runtime_error(result.error.toStdString());
            std::cout << "Edited: " << result.outputPath.toStdString() << " volume "
                      << result.originalVolume << " -> " << result.editedVolume << '\n';
            return 0;
        }
        QTemporaryDir temp;
        check(temp.isValid(), "Temp directory unavailable");
        const auto source = temp.filePath("cube.obj");
        write(source, cube());
        for (const auto& key : {"axis", "through", "radius", "operation"}) {
            auto json = MeshOperation{}.toJson();
            json.insert(key, "invalid");
            bool rejected = false;
            try { MeshOperation::fromJson(json); } catch (...) { rejected = true; }
            check(rejected, "Invalid command accepted");
        }
        auto injected = MeshOperation{}.toJson(); injected.insert("path", "arbitrary.obj");
        bool rejected = false;
        try { MeshOperation::fromJson(injected); } catch (...) { rejected = true; }
        check(rejected, "Provider path injection accepted");
        const auto result = MeshEditingService::apply(source, {});
        if (!result.error.isEmpty()) throw std::runtime_error(result.error.toStdString());
        check(read(source) == cube(), "Original changed");
        check(result.outputPath.endsWith("cube_ai_edited.obj"), "Wrong output name");
        int euler = -1;
        auto solid = readSolid(result.outputPath, &euler);
        check(solid.Status() == manifold::Manifold::Error::NoError, "Output not manifold");
        check(euler == 0, "Output is not genus one (real through-hole)");
        check(solid.Volume() > 7.74 && solid.Volume() < 7.76, "Wrong removed volume");
        const auto probe = manifold::Manifold::Cylinder(4, 0.19, 0.19, 32, true).Translate({3,4,5});
        check((solid ^ probe).IsEmpty(), "Central passage still contains triangles/solid");
        Loader loader;
        check(loader.loadOBJ(result.outputPath.toUtf8().constData()), "Viewer OBJ loader rejected edited output");
        auto bytes = read(result.outputPath);
        check(bytes.contains("mtllib cube.mtl") && bytes.contains("usemtl test") && bytes.contains("vt 1"), "Material or UV loss");
        const auto second = MeshEditingService::apply(source, {});
        check(second.error.isEmpty() && second.outputPath.endsWith("_ai_edited_1.obj"), "Filename collision not handled");
        check(!MeshEditingService::apply(result.outputPath, {}).error.isEmpty(), "Already empty cutter should report no-op");
        write(temp.filePath("open.obj"), "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");
        check(!MeshEditingService::apply(temp.filePath("open.obj"), {}).error.isEmpty(), "Open mesh accepted");
        write(temp.filePath("bad.obj"), "v 0 0 0\nf 1 2 99\n");
        check(!MeshEditingService::apply(temp.filePath("bad.obj"), {}).error.isEmpty(), "Bad indices accepted");
        AppController controller;
        ChatController chat(&controller);
        check(chat.providerName().contains("Mock"), "Mock fallback unavailable");
        chat.send("make a hole through the center");
        check(chat.error().contains("No model selected"), "Missing selection not reported");
        controller.loadModel(source);
        check(chat.selectedModelName() == "cube.obj" && controller.selectedModel()->modelPath == source, "Wrong selection");
        for (const auto& command : {"make a hole through the center", "make a hole in the middle", "create a hole through this model", "Make a hole through the center of the selected model."}) {
            controller.loadModel(source);
            chat.send(command); waitForChat(chat);
            check(chat.error().isEmpty() && controller.modelPath() != source, "Mock-to-reload flow failed");
        }
        controller.loadModel(source);
        chat.send("do not make a hole through the center"); waitForChat(chat);
        check(controller.modelPath() == source, "Negated request edited geometry");
        chat.send("make a hole through the center");
        controller.addView();
        waitForChat(chat);
        check(chat.error().contains("Selection changed") && controller.modelPath().isEmpty(), "Stale selection was edited");
        // Keep a reviewable input/output pair when an artifact directory is supplied.
        if (argc > 1) {
            QDir dir(QString::fromLocal8Bit(argv[1])); dir.mkpath(".");
            write(dir.filePath("cube.obj"), cube());
            const auto artifact = MeshEditingService::apply(dir.filePath("cube.obj"), {});
            check(artifact.error.isEmpty(), "Artifact edit failed");
            std::cout << "Artifact: " << artifact.outputPath.toStdString() << '\n';
        }
        std::cout << "PASS: validation, geometry genus=1, clear center, volume=" << solid.Volume()
                  << ", OBJ reload, UV/materials, no overwrite, malformed/open/no-op meshes, mock aliases, selection safety\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
