#include "AppController.h"

#include <QDebug>
#include <QUrl>
#include <QtMath>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
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

    activeView().modelPath = localPath;
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