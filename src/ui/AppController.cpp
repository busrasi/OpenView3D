#include "core/loader.h"
#include "AppController.h"
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>

#include <QDebug>
#include <QUrl>
#include <QtMath>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QSaveFile>
#include <QNetworkRequest>
#include <QNetworkReply>

AppController::AppController(QObject* parent)
    : QObject(parent)
{
    m_views.append(ViewState{});
    m_networkManager = new QNetworkAccessManager(this);
}

int AppController::activeViewIndex() const
{
    return m_activeViewIndex;
}

void AppController::setActiveViewIndex(int index)
{
    if (index < 0 || index >= m_views.size())
        return;

    if (m_activeViewIndex == index)
        return;

    m_activeViewIndex = index;

    emit activeViewIndexChanged();
    ++m_selectionRevision;
    emit meshChanged();
    emit sceneChanged();
    emit modelPathChanged();
    emit texturePathChanged();
    emit zoomChanged();
    emit rotationXChanged();
    emit rotationYChanged();
}

int AppController::viewCount() const
{
    return m_views.size();
}

AppController::ViewState& AppController::activeView()
{
    return m_views[m_activeViewIndex];
}

const AppController::ViewState& AppController::activeView() const
{
    return m_views[m_activeViewIndex];
}

QString AppController::modelPath() const
{
    return activeView().modelPath;
}

QString AppController::texturePath() const
{
    return activeView().texturePath;
}

float AppController::zoom() const
{
    return activeView().zoom;
}

void AppController::setZoom(float value)
{
    if (qFuzzyCompare(activeView().zoom, value))
        return;

    activeView().zoom = value;
    emit zoomChanged();
}

float AppController::rotationX() const
{
    return activeView().rotationX;
}

void AppController::setRotationX(float value)
{
    if (qFuzzyCompare(activeView().rotationX, value))
        return;

    activeView().rotationX = value;
    emit rotationXChanged();
}

float AppController::rotationY() const
{
    return activeView().rotationY;
}

void AppController::setRotationY(float value)
{
    if (qFuzzyCompare(activeView().rotationY, value))
        return;

    activeView().rotationY = value;
    emit rotationYChanged();
}

bool AppController::addView()
{
    if (m_views.size() >= MaxViews) {
        qDebug() << "Maximum view count reached:" << MaxViews;
        return false;
    }

    m_views.append(ViewState{});
    m_activeViewIndex = m_views.size() - 1;

    emit viewCountChanged();
    emit activeViewIndexChanged();
    ++m_selectionRevision;
    emit meshChanged();
    emit sceneChanged();
    emit modelPathChanged();
    emit texturePathChanged();
    emit zoomChanged();
    emit rotationXChanged();
    emit rotationYChanged();

    return true;
}

void AppController::loadModel(const QString& path)
{
    if (path.isEmpty())
        return;

    QString localPath = path;

    if (localPath.startsWith("file:"))
        localPath = QUrl(localPath).toLocalFile();

    if (localPath.startsWith("qrc:")) localPath = ":" + QUrl(localPath).path();

    activeView().modelPath = localPath;
    activeView().mesh.reset();
    activeView().scene.reset();
    activeView().loadError.clear();
    const quint64 token = ++m_loadToken;
    activeView().loadToken = token;
    struct LoadResult { MeshPtr mesh; QString error, path; };
    auto* watcher = new QFutureWatcher<LoadResult>(this);
    connect(watcher, &QFutureWatcher<LoadResult>::finished, this, [this,watcher,token] {
        const auto result = watcher->result(); watcher->deleteLater();
        for (int i=0;i<m_views.size();++i) if (m_views[i].loadToken == token) {
            m_views[i].mesh = result.mesh; m_views[i].loadError = result.error;
            const bool relocated = !result.path.isEmpty() && m_views[i].modelPath != result.path;
            if (relocated) m_views[i].modelPath = result.path;
            if (i == m_activeViewIndex) {
                if (relocated) { ++m_selectionRevision; emit modelPathChanged(); }
                emit meshChanged();
            }
            break;
        }
    });
    watcher->setFuture(QtConcurrent::run([localPath] {
        LoadResult result;
        try {
            QString sourcePath = localPath;
            if (localPath.startsWith(":")) {
                // Bundled models become editable local files without installation-specific paths.
                const auto id = QString::fromLatin1(QCryptographicHash::hash(localPath.toUtf8(),QCryptographicHash::Sha256).toHex().left(16));
                QDir directory(QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).filePath("bundled/" + id));
                if (!directory.mkpath(".")) throw std::runtime_error("Cannot create bundled-model storage.");
                sourcePath = directory.filePath(QFileInfo(localPath).fileName());
                const QDir resourceDirectory(QFileInfo(localPath).path());
                for (const auto& name : resourceDirectory.entryList(QDir::Files)) {
                    const auto destination = directory.filePath(name);
                    if (QFileInfo::exists(destination)) continue;
                    QFile resource(resourceDirectory.filePath(name)); QSaveFile output(destination);
                    if (!resource.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly))
                        throw std::runtime_error("Cannot extract bundled model.");
                    while (!resource.atEnd()) {
                        const auto block = resource.read(1024*1024);
                        if (block.isEmpty() || output.write(block) != block.size()) throw std::runtime_error("Cannot copy bundled model.");
                    }
                    if (!output.commit()) throw std::runtime_error("Cannot save bundled model.");
                }
            }
            auto mesh = std::make_shared<Loader>();
            if (!mesh->loadOBJ(sourcePath.toUtf8().constData())) result.error = "Could not load selected OBJ. Check the file and its geometry.";
            else { result.mesh = mesh; result.path = sourcePath; }
        } catch (const std::exception& e) { result.error = QString::fromUtf8(e.what()); }
        catch (...) { result.error = "OBJ loading failed (invalid or excessively large mesh)."; }
        return result;
    }));
    ++m_selectionRevision;
    emit meshChanged();
    emit sceneChanged();
    emit modelPathChanged();

    qDebug() << "View" << m_activeViewIndex << "model selected:" << localPath;
}

void AppController::loadTexture(const QString& path)
{
    if (path.isEmpty())
        return;

    QString localPath = path;

    if (localPath.startsWith("file:"))
        localPath = QUrl(localPath).toLocalFile();

    if (localPath.startsWith("qrc:")) localPath = ":" + QUrl(localPath).path();

    activeView().texturePath = localPath;
    emit texturePathChanged();

    qDebug() << "View" << m_activeViewIndex << "texture selected:" << localPath;
}

void AppController::resetCamera()
{
    activeView().zoom = 1.0f;
    activeView().rotationX = 0.0f;
    activeView().rotationY = 0.0f;

    emit zoomChanged();
    emit rotationXChanged();
    emit rotationYChanged();

    qDebug() << "View" << m_activeViewIndex << "camera reset";
}

void AppController::closeView(int index)
{
    if (m_views.size() <= 1)
        return;

    if (index < 0 || index >= m_views.size())
        return;

    m_views.erase(m_views.begin() + index);

    if (m_activeViewIndex >= m_views.size())
        m_activeViewIndex = m_views.size() - 1;

    emit viewCountChanged();
    emit activeViewIndexChanged();
    ++m_selectionRevision;
    emit meshChanged();
    emit sceneChanged();
    emit modelPathChanged();
    emit texturePathChanged();
    emit zoomChanged();
    emit rotationXChanged();
    emit rotationYChanged();

    qDebug() << "Closed view:" << index
             << "active view:" << m_activeViewIndex
             << "view count:" << m_views.size();
}

void AppController::generateModel(const QString& imagePath)
{
    if (imagePath.isEmpty())
        return;

    emit generationStarted();

    QString localPath = imagePath;

    if (localPath.startsWith("file:"))
        localPath = QUrl(localPath).toLocalFile();

    QFile imageFile(localPath);

    if (!imageFile.open(QIODevice::ReadOnly)) {
        QString error = "Image file could not be opened: " + localPath;
        emit generationFailed(error);
        qDebug() << error;
        return;
    }

    QByteArray imageBytes = imageFile.readAll();
    imageFile.close();

    QString imageBase64 = QString::fromLatin1(imageBytes.toBase64());

    QJsonObject payload;
    payload["image"] = imageBase64;
    payload["type"] = "obj";

    QByteArray jsonData = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    QNetworkRequest request(QUrl("http://127.0.0.1:8080/generate"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    qDebug() << "Generating OBJ from image:" << localPath;

    QNetworkReply* reply = m_networkManager->post(request, jsonData);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray responseData = reply->readAll();

        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QString error = reply->errorString();
            emit generationFailed(error);
            qDebug() << "Generation failed:" << error;
            return;
        }

        if (statusCode != 200) {
            QString error = QString::fromUtf8(responseData);
            emit generationFailed(error);
            qDebug() << "Generation HTTP error:" << statusCode << error;
            return;
        }

        QDir outputDir(QDir::currentPath() + "/outputs");

        if (!outputDir.exists())
            outputDir.mkpath(".");

        QString fileName =
            "generated_" +
            QString::number(QDateTime::currentMSecsSinceEpoch()) +
            ".obj";

        QString objPath = outputDir.filePath(fileName);

        QFile objFile(objPath);

        if (!objFile.open(QIODevice::WriteOnly)) {
            QString error = "Could not write generated OBJ: " + objPath;
            emit generationFailed(error);
            qDebug() << error;
            return;
        }

        objFile.write(responseData);
        objFile.close();

        QString absoluteObjPath = QFileInfo(objPath).absoluteFilePath();

        qDebug() << "OBJ generated:" << absoluteObjPath;

        loadModel(absoluteObjPath);
        emit modelGenerated(absoluteObjPath);
    });
}
SelectedModelSnapshot AppController::selectedSnapshot() const {
    SelectedModelSnapshot s;
    s.path = activeView().modelPath; s.name = QFileInfo(s.path).fileName(); s.revision = m_selectionRevision;
    if (activeView().mesh) {
        const auto& a = activeView().mesh->minimum;
        const auto& b = activeView().mesh->maximum;
        s.minimum = {a.x,a.y,a.z}; s.maximum = {b.x,b.y,b.z};
    }
    if (activeView().scene) s.transform = activeView().scene->heroTransform;
    return s;
}
void AppController::applyScene(ScenePtr scene) {
    activeView().scene = std::move(scene);
    resetCamera();
    emit sceneChanged();
}
void AppController::clearGeneratedScene() {
    ++m_selectionRevision; // Discard in-flight staging results as well.
    activeView().scene.reset(); resetCamera(); emit sceneChanged();
}
