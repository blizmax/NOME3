#ifndef ORBITTRANSFORMCONTROLLER_H
#define ORBITTRANSFORMCONTROLLER_H

#include <QObject>
#include <QMatrix4x4>
#include <QtMath>

QT_BEGIN_NAMESPACE

namespace Qt3DCore {
class QTransform;
}

class OrbitTransformController : public QObject
{
Q_OBJECT
    Q_PROPERTY(Qt3DCore::QTransform* target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(float radius READ radius WRITE setRadius NOTIFY radiusChanged)
    Q_PROPERTY(float angle READ angle WRITE setAngle NOTIFY angleChanged)

public:
    OrbitTransformController(QQuaternion &rotation, QObject *parent = 0);

    void setTarget(Qt3DCore::QTransform *target);
    Qt3DCore::QTransform *target() const;

    void setRadius(float radius);
    float radius() const;

    void setAngle(float angle);
    float angle() const;

signals:
    void targetChanged();
    void radiusChanged();
    void angleChanged();

protected:
    void updateMatrix();

private:
    Qt3DCore::QTransform *m_target;
    float m_count;
    float m_radius;
    float m_angle;
    QQuaternion &m_rotation;

};

QT_END_NAMESPACE

#endif // ORBITTRANSFORMCONTROLLER_H