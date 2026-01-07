#ifndef FANCVERCONTROLLER_H
#define FANCVERCONTROLLER_H

#include <QObject>

class FanCurveController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool autoCurveEnabled READ autoCurveEnabled WRITE setAutoCurveEnabled NOTIFY autoCurveEnabledChanged)

public:
    explicit FanCurveController(QObject *parent = nullptr);
    bool autoCurveEnabled() const { return m_autoCurveEnabled; }
    void setAutoCurveEnabled(bool enabled);

signals:
    void autoCurveEnabledChanged();

private:
    bool m_autoCurveEnabled;
};

#endif // FANCVERCONTROLLER_H
