
#include "AuraController.h"
#include "FanController.h"
#include "FanCurveController.h"
#include "LanguageController.h"
#include "SystemStatsMonitor.h"
#include "ThemeController.h"
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

int main(int argc, char *argv[]) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

  // Set Qt Quick Controls style to "Basic" to enable custom styling
  // This MUST be set BEFORE creating QGuiApplication
  QQuickStyle::setStyle("Basic");

  QGuiApplication app(argc, argv);
  app.setWindowIcon(QIcon("resources/FanControl.png"));
  app.setOrganizationName("AsusTuf");
  app.setOrganizationDomain("asus.com");
  app.setApplicationName("AsusTufFanControl");

  // Initialize Controllers (Static instances to persist and link)
  static LanguageController langController;
  static ThemeController themeController;
  static FanController fanController;
  static FanCurveController fanCurveController;

  // Link Controllers (Crucial Fix: Connect Brain to Hands)
  fanCurveController.setFanController(&fanController);

  // Register Types exposed to QML
  qmlRegisterType<AuraController>("AsusTufFanControl", 1, 0, "AuraController");
  qmlRegisterType<SystemStatsMonitor>("AsusTufFanControl", 1, 0,
                                      "SystemStatsMonitor");

  // Register Singletons
  qmlRegisterSingletonInstance<LanguageController>(
      "AsusTufFanControl", 1, 0, "LanguageController", &langController);
  qmlRegisterSingletonInstance<ThemeController>(
      "AsusTufFanControl", 1, 0, "ThemeController", &themeController);
  qmlRegisterSingletonInstance<FanController>("AsusTufFanControl", 1, 0,
                                              "FanController", &fanController);
  qmlRegisterSingletonInstance<FanCurveController>(
      "AsusTufFanControl", 1, 0, "FanCurveController", &fanCurveController);

  QQmlApplicationEngine engine;
  // Qt 6 Recommended: Load from Module URI
  // This allows "import AsusTufFanControl" to resolve correctly inside the main
  // entry
  const QUrl url("qrc:/AsusTufFanControl/ui/Main.qml");

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
          QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);

  engine.load(url);

  return app.exec();
}
