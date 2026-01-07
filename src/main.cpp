
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include "FanController.h"
#include "AuraController.h"
#include "SystemStatsMonitor.h"
#include "FanCurveController.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon("resources/FanControl.png"));
    app.setOrganizationName("AsusTuf");
    app.setOrganizationDomain("asus.com");
    app.setApplicationName("AsusTufFanControl");

    // Register Types exposed to QML
    qmlRegisterType<FanController>("AsusTufFanControl", 1, 0, "FanController");
    qmlRegisterType<AuraController>("AsusTufFanControl", 1, 0, "AuraController");
    qmlRegisterType<SystemStatsMonitor>("AsusTufFanControl", 1, 0, "SystemStatsMonitor");
    qmlRegisterType<FanCurveController>("AsusTufFanControl", 1, 0, "FanCurveController");

    QQmlApplicationEngine engine;
    // Qt 6 Recommended: Load from Module URI
    // This allows "import AsusTufFanControl" to resolve correctly inside the main entry
    const QUrl url("qrc:/AsusTufFanControl/ui/Main.qml");
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
