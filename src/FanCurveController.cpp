#include "FanCurveController.h"

FanCurveController::FanCurveController(QObject *parent)
    : QObject(parent)
    , m_autoCurveEnabled(false)
{
}

void FanCurveController::setAutoCurveEnabled(bool enabled)
{
    if (m_autoCurveEnabled != enabled) {
        m_autoCurveEnabled = enabled;
        emit autoCurveEnabledChanged();
        // TODO: Enable/Disable software fan curve logic
    }
}
