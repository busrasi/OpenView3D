#include "AppController.h"

#include <QDebug>
#include <QUrl>
#include <QtMath>

AppController::AppController(QObject* parent)
    : QObject(parent)
{
    m_views.append(ViewState{});
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

    activeView().modelPath = path;
    emit modelPathChanged();

    qDebug() << "View" << m_activeViewIndex << "model selected:" << path;
}

void AppController::loadTexture(const QString& path)
{
    if (path.isEmpty())
        return;

    activeView().texturePath = path;
    emit texturePathChanged();

    qDebug() << "View" << m_activeViewIndex << "texture selected:" << path;
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