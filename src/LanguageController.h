#ifndef LANGUAGECONTROLLER_H
#define LANGUAGECONTROLLER_H

#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTranslator>
#include <QVariantList>

class LanguageController : public QObject {
  Q_OBJECT

  Q_PROPERTY(QString currentLanguage READ currentLanguage WRITE setLanguage
                 NOTIFY languageChanged)
  Q_PROPERTY(QVariantList availableLanguages READ availableLanguages CONSTANT)

public:
  explicit LanguageController(QObject *parent = nullptr);
  ~LanguageController();

  // Property getters
  QString currentLanguage() const { return m_currentLanguage; }
  QVariantList availableLanguages() const { return m_availableLanguages; }

  // Setters
  Q_INVOKABLE void setLanguage(const QString &langCode);

  // Get display name for a language code
  Q_INVOKABLE QString getDisplayName(const QString &langCode) const;
  Q_INVOKABLE QString getNativeName(const QString &langCode) const;
  Q_INVOKABLE QString getFlag(const QString &langCode) const;

  // Load saved language or detect system language
  void loadSavedLanguage();

signals:
  void languageChanged();
  void retranslateRequired(); // Emitted after language change for UI refresh

private:
  void initLanguageList();
  bool loadTranslation(const QString &langCode);

  QString m_currentLanguage;
  QVariantList m_availableLanguages;
  QTranslator *m_translator;
  QSettings m_settings;

  // Language metadata: code -> {displayName, nativeName, flag}
  struct LanguageInfo {
    QString displayName;
    QString nativeName;
    QString flag;
  };
  QMap<QString, LanguageInfo> m_languageMap;
};

#endif // LANGUAGECONTROLLER_H
