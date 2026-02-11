#include "LanguageController.h"
#include <QDebug>
#include <QGuiApplication>

LanguageController::LanguageController(QObject *parent)
    : QObject(parent), m_translator(new QTranslator(this)),
      m_settings("AsusTuf", "FanControl") {

  initLanguageList();
  loadSavedLanguage();
}

LanguageController::~LanguageController() {
  // Translator is parented to this, so auto-deleted
}

void LanguageController::initLanguageList() {
  // Initialize language metadata with display names, native names, and flags
  // Order: English first, then alphabetically by native name

  m_languageMap["en"] = {"English", "English", "🇺🇸"};
  m_languageMap["ar"] = {"Arabic", "العربية", "🇸🇦"};
  m_languageMap["bn"] = {"Bengali", "বাংলা", "🇧🇩"};
  m_languageMap["zh"] = {"Chinese", "中文", "🇨🇳"};
  m_languageMap["de"] = {"German", "Deutsch", "🇩🇪"};
  m_languageMap["es"] = {"Spanish", "Español", "🇪🇸"};
  m_languageMap["fa"] = {"Persian", "فارسی", "🇮🇷"};
  m_languageMap["fr"] = {"French", "Français", "🇫🇷"};
  m_languageMap["hi"] = {"Hindi", "हिन्दी", "🇮🇳"};
  m_languageMap["id"] = {"Indonesian", "Bahasa Indonesia", "🇮🇩"};
  m_languageMap["it"] = {"Italian", "Italiano", "🇮🇹"};
  m_languageMap["ja"] = {"Japanese", "日本語", "🇯🇵"};
  m_languageMap["ko"] = {"Korean", "한국어", "🇰🇷"};
  m_languageMap["mr"] = {"Marathi", "मराठी", "🇮🇳"};
  m_languageMap["pa"] = {"Punjabi", "ਪੰਜਾਬੀ", "🇮🇳"};
  m_languageMap["pl"] = {"Polish", "Polski", "🇵🇱"};
  m_languageMap["pt"] = {"Portuguese", "Português", "🇵🇹"};
  m_languageMap["ru"] = {"Russian", "Русский", "🇷🇺"};
  m_languageMap["sw"] = {"Swahili", "Kiswahili", "🇰🇪"};
  m_languageMap["ta"] = {"Tamil", "தமிழ்", "🇮🇳"};
  m_languageMap["tr"] = {"Turkish", "Türkçe", "🇹🇷"};
  m_languageMap["ur"] = {"Urdu", "اردو", "🇵🇰"};
  m_languageMap["vi"] = {"Vietnamese", "Tiếng Việt", "🇻🇳"};

  // Build the available languages list for QML
  // English first, then others sorted by display name
  m_availableLanguages.clear();

  // Add Tamil first (User Request)
  if (m_languageMap.contains("ta")) {
    QVariantMap taMap;
    taMap["code"] = "ta";
    taMap["displayName"] = m_languageMap["ta"].displayName;
    taMap["nativeName"] = m_languageMap["ta"].nativeName;
    taMap["flag"] = m_languageMap["ta"].flag;
    m_availableLanguages.append(taMap);
  }

  // Add English
  QVariantMap enMap;
  enMap["code"] = "en";
  enMap["displayName"] = m_languageMap["en"].displayName;
  enMap["nativeName"] = m_languageMap["en"].nativeName;
  enMap["flag"] = m_languageMap["en"].flag;
  m_availableLanguages.append(enMap);

  // Add others sorted alphabetically by display name
  QStringList sortedCodes = m_languageMap.keys();
  sortedCodes.removeOne("en");
  sortedCodes.removeOne("ta"); // Remove Tamil since it's already added
  std::sort(sortedCodes.begin(), sortedCodes.end(),
            [this](const QString &a, const QString &b) {
              return m_languageMap[a].displayName <
                     m_languageMap[b].displayName;
            });

  for (const QString &code : sortedCodes) {
    QVariantMap langMap;
    langMap["code"] = code;
    langMap["displayName"] = m_languageMap[code].displayName;
    langMap["nativeName"] = m_languageMap[code].nativeName;
    langMap["flag"] = m_languageMap[code].flag;
    m_availableLanguages.append(langMap);
  }
}

void LanguageController::loadSavedLanguage() {
  // Try to load saved language preference
  QString savedLang = m_settings.value("language", "").toString();

  if (savedLang.isEmpty()) {
    // Detect system language
    QString systemLang = QLocale::system().name().split('_').first();
    if (m_languageMap.contains(systemLang)) {
      savedLang = systemLang;
    } else {
      savedLang = "en"; // Default to English
    }
  }

  // Load the translation
  if (loadTranslation(savedLang)) {
    m_currentLanguage = savedLang;
  } else {
    m_currentLanguage = "en";
  }
}

bool LanguageController::loadTranslation(const QString &langCode) {
  if (langCode == "en") {
    // English is the source language, remove any translator
    QGuiApplication::instance()->removeTranslator(m_translator);
    return true;
  }

  // Try to load from resources
  QString basePath = ":/translations/AsusTufController_" + langCode;

  if (m_translator->load(basePath) || m_translator->load(basePath + ".qm")) {
    QGuiApplication::instance()->removeTranslator(m_translator);
    QGuiApplication::instance()->installTranslator(m_translator);
    return true;
  }

  return false;
}

void LanguageController::setLanguage(const QString &langCode) {
  if (langCode == m_currentLanguage) {
    return;
  }

  if (!m_languageMap.contains(langCode)) {
    return;
  }

  if (loadTranslation(langCode)) {
    m_currentLanguage = langCode;
    m_settings.setValue("language", langCode);
    m_settings.sync();

    emit languageChanged();
    emit retranslateRequired();
  }
}

QString LanguageController::getDisplayName(const QString &langCode) const {
  if (m_languageMap.contains(langCode)) {
    return m_languageMap[langCode].displayName;
  }
  return langCode;
}

QString LanguageController::getNativeName(const QString &langCode) const {
  if (m_languageMap.contains(langCode)) {
    return m_languageMap[langCode].nativeName;
  }
  return langCode;
}

QString LanguageController::getFlag(const QString &langCode) const {
  if (m_languageMap.contains(langCode)) {
    return m_languageMap[langCode].flag;
  }
  return "";
}
