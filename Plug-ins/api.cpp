#include "api.h"
#include "jwbstore.h"
#include "appsreviewsapisystem.h"
#include <QProcess>
#include <QSysInfo>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ================= ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ (ВЕРХНИЙ УРОВЕНЬ) =================

// Служебный curl-движок для Windows окружения
static QByteArray executeCurlOnWindows(const QString &url, const QString &token)
{
    QProcess process;
    QStringList arguments;
    arguments << "-s" << "-L";
    arguments << "-A" << "Mozilla/5.0 (Windows NT 10.0; Win64; x64) JWB-Denda/1.0";
    if (!token.isEmpty()) {
        arguments << "-H" << QString("Authorization: Bearer %1").arg(token);
    }
    arguments << url;

    process.start("curl", arguments);
    if (process.waitForFinished(15000)) {
        return process.readAllStandardOutput();
    }
    return QByteArray();
}

// Определение архитектуры текущего устройства (ЖЕЛЕЗОБЕТОННЫЙ ФИКС ARMHF)
static QString getCurrentDeviceArchitecture()
{
#if defined(Q_OS_WIN)
    return "amd64"; // Для тестов на Винде подкидываем amd64
#else
    // 1. Проверяем размер указателя. Если он 4 байта (32 бита) — это стопудово 32-битный ARM (armhf)
    if (sizeof(void*) == 4) {
        qDebug() << ">>> [JWB-Api] Detected 32-bit runtime pointer. Forcing armhf!";
        return "armhf";
    }

    // 2. Если указатель 8 байт (64 бита), проверяем строку от Qt
    QString arch = QSysInfo::currentCpuArchitecture().toLower();
    qDebug() << ">>> [JWB-Api] QSysInfo reports architecture:" << arch;

    if (arch == "x86_64" || arch == "amd64") return "amd64";
    if (arch == "arm64" || arch == "aarch64") return "arm64";
    if (arch == "arm" || arch == "armhf" || arch.contains("armv7") || arch.contains("v7l")) return "armhf";

    return arch;
#endif
}

// ================= КОНСТРУКТОР / ДЕСТРУКТОР / СЛУЖЕБНЫЕ МЕТОДЫ =================

GitHubApi::GitHubApi(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<QVariantList>("QVariantList");

    m_networkManager = new QNetworkAccessManager(this);

    m_retryTimer = new QTimer(this);
    m_retryTimer->setSingleShot(true);
    connect(m_retryTimer, &QTimer::timeout, this, &GitHubApi::fetchOrganizationRepos);
}

GitHubApi::~GitHubApi() {}

void GitHubApi::updateLoadingState(int delta)
{
    int old = m_activeRequestsCount;
    m_activeRequestsCount = qMax(0, m_activeRequestsCount + delta);
    if ((old == 0 && m_activeRequestsCount > 0) || (old > 0 && m_activeRequestsCount == 0))
        emit isRequestActiveChanged(isRequestActive());
}

void GitHubApi::injectAuthHeader(QNetworkRequest &request)
{
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) JWB-Denda/1.0");
    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_token).toUtf8());
    }
}

QString GitHubApi::getCacheFilePath()
{
    QString cachePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(cachePath);
    return cachePath + "/global_store_cache.json";
}

void GitHubApi::clearCache()
{
    QFile file(getCacheFilePath());
    if (file.exists()) {
        file.remove();
    }
    qDebug() << ">>> JWB-Denda cache completely cleared!";
}

void GitHubApi::loadCachedRepos()
{
    QFile file(getCacheFilePath());
    if (file.open(QIODevice::ReadOnly)) {
        QJsonArray arr = QJsonDocument::fromJson(file.readAll()).array();
        file.close();

        if (!arr.isEmpty()) {
	   QVariantList repos;
	   for (const QJsonValue &v : arr) {
	       repos.append(v.toObject().toVariantMap());
	   }
	   qDebug() << ">>> Loaded successfully from local cache. Count:" << repos.size();
	   emit reposListReceived(repos);
        }
    }
}

void GitHubApi::saveReposToCache(const QVariantList &repos)
{
    QFile file(getCacheFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        QJsonArray arr;
        for (const QVariant &v : repos) {
	   arr.append(QJsonObject::fromVariantMap(v.toMap()));
        }
        file.write(QJsonDocument(arr).toJson());
        file.close();
        qDebug() << ">>> Global store cache saved successfully! Total items:" << repos.size();
    } else {
        qDebug() << ">>> ALERT: Failed to open cache file for writing!";
    }
}

// ================= ОСНОВНЫЕ МЕТОДЫ СИНХРОНИЗАЦИИ =================

void GitHubApi::fetchOrganizationRepos()
{
    m_retryTimer->stop();
    loadCachedRepos();

    updateLoadingState(1);
    m_progressText = "JWB-Denda synchronization...";
    emit progressTextChanged();

    QString urlStr = QString("https://%1.github.io/JWB-Denda-Database/store_data.json").arg(m_orgName.toLower());

#if defined(Q_OS_WIN)
    QByteArray responseData = executeCurlOnWindows(urlStr, m_token);
    updateLoadingState(-1);
    m_progressText = "";
    emit progressTextChanged();

    if (responseData.isEmpty()) {
        qDebug() << ">>> Windows Curl synchronization failed!";
        emit errorOccurred("Offline mode view (Cache)");
        m_retryTimer->start(7000);
        return;
    }

    QJsonArray storeArray = QJsonDocument::fromJson(responseData).array();
    if (storeArray.isEmpty()) {
        emit errorOccurred("Database JSON parse error");
        return;
    }

    QVariantList finalList;
    for (int i = 0; i < storeArray.size(); i++) {
        QJsonObject appObj = storeArray[i].toObject();
        QString repoName = appObj["name"].toString();
        QJsonObject splashObj = appObj["splash"].toObject();
        QString splashColor = splashObj.contains("color") ? splashObj["color"].toString() : "#141414";

        QVariantMap repoMap;
        repoMap["repoName"] = repoName;
        repoMap["name"] = repoName;
        repoMap["appName"] = appObj["display_name"].toString();
        repoMap["appVersion"] = appObj["version"].toString();
        repoMap["appAuthor"] = appObj["author"].toString();
        repoMap["author"] = appObj["author"].toString();
        repoMap["appDescription"] = appObj["description"].toString();

        QString iconUrl = appObj["icon"].toString();
        repoMap["appIcon"] = iconUrl;

        QString cat = appObj["type"].toString().toLower().trimmed();
        if (appObj.contains("category")) cat = appObj["category"].toString().toLower().trimmed();
        if (cat.isEmpty()) {
	   cat = (repoName.toLower().contains("sokoban") || repoName.toLower().contains("blockerino")) ? "games" : "apps";
        }

        repoMap["category"] = cat;
        repoMap["appCategory"] = cat;
        repoMap["appSplashColor"] = splashColor;
        repoMap["appSize"] = "0.0 MB";
        repoMap["appArchitectures"] = "none";
        repoMap["appUrl"] = "";

        finalList.append(repoMap);

        if (iconUrl.startsWith("http")) {
	   QTimer::singleShot(10, this, [this, iconUrl, repoName]() {
	       QByteArray iconBytes = executeCurlOnWindows(iconUrl, m_token);
	       if (!iconBytes.isEmpty()) {
		  QString format = "png";
		  if (iconUrl.endsWith(".svg", Qt::CaseInsensitive)) format = "svg+xml";
		  else if (iconUrl.endsWith(".jpg", Qt::CaseInsensitive) || iconUrl.endsWith(".jpeg", Qt::CaseInsensitive)) format = "jpeg";

		  QString base64Data = QString("data:image/%1;base64,%2").arg(format, QString(iconBytes.toBase64()));
		  emit releaseInfoReceived(repoName, "", "", "", "", "", base64Data, "", "", "");
	       }
	   });
        }
    }

    saveReposToCache(finalList);
    emit reposListReceived(finalList);

#else
    QNetworkRequest request((QUrl(urlStr)));
    injectAuthHeader(request);
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &GitHubApi::onReposFetched);
#endif
}

void GitHubApi::onReposFetched()
{
#if !defined(Q_OS_WIN)
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    updateLoadingState(-1);

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("Offline mode view (Cache)");
        m_progressText = "";
        emit progressTextChanged();
        m_retryTimer->start(7000);
        return;
    }

    QJsonArray storeArray = QJsonDocument::fromJson(reply->readAll()).array();
    if (storeArray.isEmpty()) return;

    QVariantList finalList;
    for (int i = 0; i < storeArray.size(); i++) {
        QJsonObject appObj = storeArray[i].toObject();
        QString repoName = appObj["name"].toString();
        QJsonObject splashObj = appObj["splash"].toObject();
        QString splashColor = splashObj.contains("color") ? splashObj["color"].toString() : "#141414";

        QVariantMap repoMap;
        repoMap["repoName"] = repoName;
        repoMap["name"] = repoName;
        repoMap["appName"] = appObj["display_name"].toString();
        repoMap["appVersion"] = appObj["version"].toString();
        repoMap["appAuthor"] = appObj["author"].toString();
        repoMap["author"] = appObj["author"].toString();
        repoMap["appDescription"] = appObj["description"].toString();

        QString iconUrl = appObj["icon"].toString();
        repoMap["appIcon"] = iconUrl;

        QString cat = appObj["type"].toString().toLower().trimmed();
        if (appObj.contains("category")) cat = appObj["category"].toString().toLower().trimmed();
        if (cat.isEmpty()) {
	   cat = (repoName.toLower().contains("sokoban") || repoName.toLower().contains("blockerino")) ? "games" : "apps";
        }

        repoMap["category"] = cat;
        repoMap["appCategory"] = cat;
        repoMap["appSplashColor"] = splashColor;
        repoMap["appSize"] = "0.0 MB";
        repoMap["appArchitectures"] = "none";
        repoMap["appUrl"] = "";

        finalList.append(repoMap);

        if (iconUrl.startsWith("http")) {
	   QNetworkRequest iconReq((QUrl(iconUrl)));
	   injectAuthHeader(iconReq);
	   QNetworkReply *iconReply = m_networkManager->get(iconReq);
	   connect(iconReply, &QNetworkReply::finished, this, [this, iconReply, repoName, iconUrl]() {
	       if (iconReply && iconReply->error() == QNetworkReply::NoError) {
		  QString format = "png";
		  if (iconUrl.endsWith(".svg", Qt::CaseInsensitive)) format = "svg+xml";
		  else if (iconUrl.endsWith(".jpg", Qt::CaseInsensitive) || iconUrl.endsWith(".jpeg", Qt::CaseInsensitive)) format = "jpeg";

		  QString base64Data = QString("data:image/%1;base64,%2").arg(format, QString(iconReply->readAll().toBase64()));
		  emit releaseInfoReceived(repoName, "", "", "", "", "", base64Data, "", "", "");
	       }
	       if (iconReply) iconReply->deleteLater();
	   });
        }
    }

    m_progressText = "";
    emit progressTextChanged();
    saveReposToCache(finalList);
    emit reposListReceived(finalList);
#endif
}

// ================= ОБНОВЛЕННЫЙ МЕТОД: fetchReleaseInfo (УМНАЯ ФИЛЬТРАЦИЯ С BREAK + ОФФЛАЙН ФОЛБЭК) =================
void GitHubApi::fetchReleaseInfo(const QString &repoName)
{
    updateLoadingState(1);
    QString repoUrl = QString("https://api.github.com/repos/%1/%2").arg(m_orgName, repoName);
    QString deviceArch = getCurrentDeviceArchitecture();

#if defined(Q_OS_WIN)
    QTimer::singleShot(10, this, [this, repoUrl, repoName, deviceArch]() {
        QByteArray repoRes = executeCurlOnWindows(repoUrl, m_token);
        if (repoRes.isEmpty()) {
	   updateLoadingState(-1);
	   return;
        }
        QJsonObject rObj = QJsonDocument::fromJson(repoRes).object();

        QString relUrl = QString("https://api.github.com/repos/%1/%2/releases/latest").arg(m_orgName, repoName);
        QByteArray relRes = executeCurlOnWindows(relUrl, m_token);
        updateLoadingState(-1);

        QString finalDescription = rObj["description"].toString();
        QString versionStr = "1.0.0";
        QString dlUrl = "", sizeStr = "0.0 MB";
        QStringList archs;

        if (!relRes.isEmpty()) {
	   QJsonObject relObj = QJsonDocument::fromJson(relRes).object();
	   if (relObj.contains("tag_name")) versionStr = relObj["tag_name"].toString();

	   QJsonArray assets = relObj["assets"].toArray();
	   double targetSize = 0;
	   QString fallbackUrl = "";

	   for (const QJsonValue &a : assets) {
	       QString fName = a.toObject()["name"].toString().toLower();
	       if (fName.endsWith(".click")) {
		   QString currentAssetUrl = a.toObject()["browser_download_url"].toString();
		   double sz = a.toObject()["size"].toDouble() / (1024.0 * 1024.0);

		   if (fName.contains("armhf") && !archs.contains("armhf")) archs << "armhf";
		   if (fName.contains("arm64") && !archs.contains("arm64")) archs << "arm64";
		   if ((fName.contains("amd64") || fName.contains("x86_64")) && !archs.contains("x86_64")) archs << "x86_64";
		   if (fName.contains("_all") && !archs.contains("all")) archs << "all";

		   // БЕЗОПАСНЫЙ ПОДБОР: точное совпадение останавливает цикл
		   if (fName.contains(deviceArch)) {
		       dlUrl = currentAssetUrl;
		       targetSize = sz;
		       break;
		   } else if (fName.contains("_all") && dlUrl.isEmpty()) {
		       dlUrl = currentAssetUrl;
		       targetSize = sz;
		   } else if (fallbackUrl.isEmpty()) {
		       fallbackUrl = currentAssetUrl;
		       if (targetSize == 0) targetSize = sz;
		   }
	       }
	   }
	   if (dlUrl.isEmpty()) dlUrl = fallbackUrl;
	   if (targetSize > 0) sizeStr = QString::number(targetSize, 'f', 1) + " MB";
        }

        emit releaseInfoReceived(repoName, versionStr, m_orgName, sizeStr, "apps", finalDescription, "", dlUrl, "#141414", archs.isEmpty() ? "none" : archs.join(","));
    });
#else
    QNetworkRequest request((QUrl(repoUrl)));
    injectAuthHeader(request);
    request.setRawHeader("Accept", "application/vnd.github.v3+json");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, repoName, deviceArch]() {
        if (!reply) return;
        reply->deleteLater();
        updateLoadingState(-1);

        if (reply->error() != QNetworkReply::NoError) {
	   // ФОЛБЭК ДЛЯ ОФФЛАЙНА (ПЕРВЫЙ ЭТАП ЗАПРОСА): Если упал базовый инфо-запрос репозитория
	   qDebug() << ">>> [JWB-Api] Repo info fetch failed (offline). Firing fallback for:" << repoName;
	   QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
	   QString expectedFile = cacheDir + "/" + repoName.toLower().remove("-") + ".click";

	   QString dlUrl = "";
	   QString sizeStr = "0.0 MB";
	   QStringList archs;

	   if (QFile::exists(expectedFile)) {
	       dlUrl = "offline://" + repoName.toLower().remove("-") + ".click";
	       sizeStr = "Cached";
	       archs << deviceArch;
	   }
	   emit releaseInfoReceived(repoName, "Local", m_orgName, sizeStr, "apps", "Offline description mirror", "", dlUrl, "#141414", archs.isEmpty() ? "none" : archs.join(","));
	   return;
        }

        QJsonObject rObj = QJsonDocument::fromJson(reply->readAll()).object();

        QString relUrl = QString("https://api.github.com/repos/%1/%2/releases/latest").arg(m_orgName, repoName);
        QNetworkRequest relReq((QUrl(relUrl)));
        injectAuthHeader(relReq);
        relReq.setRawHeader("Accept", "application/vnd.github.v3+json");

        updateLoadingState(1);
        QNetworkReply *relReply = m_networkManager->get(relReq);
        connect(relReply, &QNetworkReply::finished, this, [this, relReply, rObj, repoName, deviceArch]() {
	   if (!relReply) return;
	   relReply->deleteLater();
	   updateLoadingState(-1);

	   QString finalDescription = rObj["description"].toString();
	   QString versionStr = "1.0.0";
	   QString dlUrl = "", sizeStr = "0.0 MB";
	   QStringList archs;

	   if (relReply->error() == QNetworkReply::NoError) {
	       QJsonObject relObj = QJsonDocument::fromJson(relReply->readAll()).object();
	       if (relObj.contains("tag_name")) versionStr = relObj["tag_name"].toString();

	       QJsonArray assets = relObj["assets"].toArray();
	       double targetSize = 0;
	       QString fallbackUrl = "";

	       for (const QJsonValue &a : assets) {
		  QString fName = a.toObject()["name"].toString().toLower();
		  if (fName.endsWith(".click")) {
		      QString currentAssetUrl = a.toObject()["browser_download_url"].toString();
		      double sz = a.toObject()["size"].toDouble() / (1024.0 * 1024.0);

		      if (fName.contains("armhf") && !archs.contains("armhf")) archs << "armhf";
		      if (fName.contains("arm64") && !archs.contains("arm64")) archs << "arm64";
		      if ((fName.contains("amd64") || fName.contains("x86_64")) && !archs.contains("x86_64")) archs << "x86_64";
		      if (fName.contains("_all") && !archs.contains("all")) archs << "all";

		      // БЕЗОПАСНЫЙ ПОДБОР: точное совпадение останавливает цикл
		      if (fName.contains(deviceArch)) {
			  dlUrl = currentAssetUrl;
			  targetSize = sz;
			  break;
		      } else if (fName.contains("_all") && dlUrl.isEmpty()) {
			  dlUrl = currentAssetUrl;
			  targetSize = sz;
		      } else if (fallbackUrl.isEmpty()) {
			  fallbackUrl = currentAssetUrl;
			  if (targetSize == 0) targetSize = sz;
		      }
		  }
	       }
	       if (dlUrl.isEmpty()) dlUrl = fallbackUrl;
	       if (targetSize > 0) sizeStr = QString::number(targetSize, 'f', 1) + " MB";
	   } else {
	       // ФОЛБЭК ДЛЯ ОФФЛАЙНА (ВТОРОЙ ЭТАП ЗАПРОСА): Сети нет, проверяем кэш на диске
	       qDebug() << ">>> [JWB-Api] Release fetch failed (offline). Checking storage cache for:" << repoName;
	       QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
	       QString expectedFile = cacheDir + "/" + repoName.toLower().remove("-") + ".click";

	       if (QFile::exists(expectedFile)) {
		   versionStr = "Local";
		   dlUrl = "offline://" + repoName.toLower().remove("-") + ".click";
		   sizeStr = "Cached";
		   archs << deviceArch;
	       }
	   }

	   emit releaseInfoReceived(repoName, versionStr, m_orgName, sizeStr, "apps", finalDescription, "", dlUrl, "#141414", archs.isEmpty() ? "none" : archs.join(","));
        });
    });
#endif
}

// ================= ОБНОВЛЕННЫЙ МЕТОД: checkClickRelease (УМНАЯ ФИЛЬТРАЦИЯ С БЕЗОПАСНЫМ RETRY + ОФФЛАЙН ФОЛБЭК) =================
void GitHubApi::checkClickRelease(const QString &repoName)
{
    if (repoName.isEmpty()) {
        emit clickReleaseChecked(false, "", "0.0 MB", "none");
        return;
    }

    updateLoadingState(1);
    QString relUrl = QString("https://api.github.com/repos/%1/%2/releases/latest").arg(m_orgName, repoName);
    QString deviceArch = getCurrentDeviceArchitecture();

#if defined(Q_OS_WIN)
    QTimer::singleShot(10, this, [this, relUrl, repoName, deviceArch]() {
        updateLoadingState(-1);
        QByteArray res = executeCurlOnWindows(relUrl, m_token);

        bool available = false;
        QString dlUrl = "", sizeStr = "0.0 MB";
        QStringList archList;

        if (!res.isEmpty()) {
	   QJsonObject relObj = QJsonDocument::fromJson(res).object();
	   QJsonArray assets = relObj["assets"].toArray();
	   double targetSize = 0;
	   QString fallbackUrl = "";

	   for (const QJsonValue &a : assets) {
	       QString fName = a.toObject()["name"].toString().toLower();
	       if (fName.endsWith(".click")) {
		  available = true;
		  QString currentAssetUrl = a.toObject()["browser_download_url"].toString();
		  double sz = a.toObject()["size"].toDouble() / (1024.0 * 1024.0);

		  if (fName.contains("armhf") && !archList.contains("armhf")) archList << "armhf";
		  if (fName.contains("arm64") && !archList.contains("arm64")) archList << "arm64";
		  if ((fName.contains("amd64") || fName.contains("x86_64")) && !archList.contains("x86_64")) archList << "x86_64";
		  if (fName.contains("_all") && !archList.contains("all")) archList << "all";

		  // ЖЕСТКИЙ ФИКС СОВПАДЕНИЯ АРХИТЕКТУРЫ
		  if (fName.contains(deviceArch)) {
		      dlUrl = currentAssetUrl;
		      targetSize = sz;
		      break; // Родная архитектура найдена, стопаем, чтобы не перебило универсальным пакетом!
		  } else if (fName.contains("_all") && dlUrl.isEmpty()) {
		      dlUrl = currentAssetUrl;
		      targetSize = sz;
		  } else if (fallbackUrl.isEmpty()) {
		      fallbackUrl = currentAssetUrl;
		      if (targetSize == 0) targetSize = sz;
		  }
	       }
	   }
	   if (dlUrl.isEmpty()) dlUrl = fallbackUrl;
	   if (targetSize > 0) sizeStr = QString::number(targetSize, 'f', 1) + " MB";
        }
        emit clickReleaseChecked(available, dlUrl, sizeStr, archList.isEmpty() ? "none" : archList.join(","));
    });
#else
    QNetworkRequest request((QUrl(relUrl)));
    injectAuthHeader(request);
    request.setRawHeader("Accept", "application/vnd.github.v3+json");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, repoName, deviceArch]() {
        if (!reply) return;
        reply->deleteLater();
        updateLoadingState(-1);

        bool available = false;
        QString dlUrl = "";
        QString sizeStr = "0.0 MB";
        QStringList archList;

        if (reply->error() == QNetworkReply::NoError) {
	   QJsonObject relObj = QJsonDocument::fromJson(reply->readAll()).object();
	   QJsonArray assets = relObj["assets"].toArray();
	   double targetSize = 0;
	   QString fallbackUrl = "";

	   for (const QJsonValue &a : assets) {
	       QString fName = a.toObject()["name"].toString().toLower();
	       if (fName.endsWith(".click")) {
		  available = true;
		  QString currentAssetUrl = a.toObject()["browser_download_url"].toString();
		  double sz = a.toObject()["size"].toDouble() / (1024.0 * 1024.0);

		  if (fName.contains("armhf") && !archList.contains("armhf")) archList << "armhf";
		  if (fName.contains("arm64") && !archList.contains("arm64")) archList << "arm64";
		  if ((fName.contains("amd64") || fName.contains("x86_64")) && !archList.contains("x86_64")) archList << "x86_64";
		  if (fName.contains("_all") && !archList.contains("all")) archList << "all";

		  // ЖЕСТКИЙ ФИКС СОВПАДЕНИЯ АРХИТЕКТУРЫ
		  if (fName.contains(deviceArch)) {
		      dlUrl = currentAssetUrl;
		      targetSize = sz;
		      break; // Прерываем цикл, сохраняя точный armhf линк
		  } else if (fName.contains("_all") && dlUrl.isEmpty()) {
		      dlUrl = currentAssetUrl;
		      targetSize = sz;
		  } else if (fallbackUrl.isEmpty()) {
		      fallbackUrl = currentAssetUrl;
		      if (targetSize == 0) targetSize = sz;
		  }
	       }
	   }
	   if (dlUrl.isEmpty()) dlUrl = fallbackUrl;
	   if (targetSize > 0) sizeStr = QString::number(targetSize, 'f', 1) + " MB";
        } else {
	   // ФОЛБЭК ДЛЯ ОФФЛАЙНА: Ошибка сети, проверяем физическое наличие файла .click в кэше
	   qDebug() << ">>> [JWB-Api] Check release failed (offline). Simulating availability via disk cache for:" << repoName;
	   QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
	   QString expectedFile = cacheDir + "/" + repoName.toLower().remove("-") + ".click";

	   if (QFile::exists(expectedFile)) {
	       available = true;
	       dlUrl = "offline://" + repoName.toLower().remove("-") + ".click";
	       sizeStr = "Cached";
	       archList << deviceArch;
	   }
        }
        emit clickReleaseChecked(available, dlUrl, sizeStr, archList.isEmpty() ? "none" : archList.join(","));
    });
#endif
}

// ================= СЛОТЫ-ЗАГЛУШКИ ДЛЯ КОРРЕКТНОЙ ЛИНКОВКИ НА WINDOWS =================

void GitHubApi::onIconFetched(QNetworkReply *reply, const QString &repoName)
{
    Q_UNUSED(reply);
    Q_UNUSED(repoName);
}

void GitHubApi::onReleaseInfoFetched(QNetworkReply *reply, const QString &repoName)
{
    Q_UNUSED(reply);
    Q_UNUSED(repoName);
}

void GitHubApi::onClickChecked(QNetworkReply *reply, const QString &repoName)
{
    Q_UNUSED(reply);
    Q_UNUSED(repoName);
}
