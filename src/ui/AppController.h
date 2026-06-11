#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <QObject>
#include <QString>
#include <QVector>

class AppController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int activeViewIndex READ activeViewIndex WRITE setActiveViewIndex NOTIFY activeViewIndexChanged)
    Q_PROPERTY(int viewCount READ viewCount NOTIFY viewCountChanged)

    Q_PROPERTY(QString modelPath READ modelPath NOTIFY modelPathChanged)
    Q_PROPERTY(QString texturePath READ texturePath NOTIFY texturePathChanged)

    Q_PROPERTY(float zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(float rotationX READ rotationX WRITE setRotationX NOTIFY rotationXChanged)
    Q_PROPERTY(float rotationY READ rotationY WRITE setRotationY NOTIFY rotationYChanged)

public:
    explicit AppController(QObject* parent = nullptr);

    int activeViewIndex() const;
    void setActiveViewIndex(int index);

    int viewCount() const;

    QString modelPath() const;
    QString texturePath() const;

    float zoom() const;
    void setZoom(float value);

    float rotationX() const;
    void setRotationX(float value);

    float rotationY() const;
    void setRotationY(float value);

    Q_INVOKABLE bool addView();
    Q_INVOKABLE void loadModel(const QString& path);
    Q_INVOKABLE void loadTexture(const QString& path);
    Q_INVOKABLE void resetCamera();

signals:
    void activeViewIndexChanged();
    void viewCountChanged();

    void modelPathChanged();
    void texturePathChanged();
    void zoomChanged();
    void rotationXChanged();
    void rotationYChanged();

private:
    struct ViewState {
        QString modelPath;
        QString texturePath;
        float zoom = 1.0f;
        float rotationX = 0.0f;
        float rotationY = 0.0f;
    };

    ViewState& activeView();
    const ViewState& activeView() const;

private:
    QVector<ViewState> m_views;
    int m_activeViewIndex = 0;
    static constexpr int MaxViews = 15;
};

#endif