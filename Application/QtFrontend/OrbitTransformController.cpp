#include "OrbitTransformController.h"

#include <Qt3DCore/qtransform.h>

QT_BEGIN_NAMESPACE

OrbitTransformController::OrbitTransformController(QQuaternion &rotation, QObject *parent)
    : QObject(parent)
    , m_target(nullptr)
    , m_radius(1.0f)
    , m_angle(0.0f)
    , m_rotation(rotation)
{

}

void OrbitTransformController::setTarget(Qt3DCore::QTransform *target)
{
    if (m_target != target) {
        m_target = target;
        emit targetChanged();
    }
}

Qt3DCore::QTransform *OrbitTransformController::target() const
{
    return m_target;
}

void OrbitTransformController::setRadius(float radius)
{
    if (!qFuzzyCompare(radius, m_radius)) {
        m_radius = radius;
        updateMatrix();
        emit radiusChanged();
    }
}

float OrbitTransformController::radius() const
{
    return m_radius;
}

void OrbitTransformController::setAngle(float angle)
{
    if (!qFuzzyCompare(angle, m_angle)) {
        m_angle = angle;
        updateMatrix();
        emit angleChanged();
    }
}

float OrbitTransformController::angle() const
{
    return m_angle;
}

void OrbitTransformController::updateMatrix()
{
    m_count += 0.05;
    m_rotation = QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), 2 * qCos(m_count)) * m_rotation;
    m_target->setRotation(m_rotation);
}

QT_END_NAMESPACE
