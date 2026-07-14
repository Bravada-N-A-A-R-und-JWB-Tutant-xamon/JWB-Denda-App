#include "jwbstore.h"
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QProcess>
#include <QTextStream>
#include <QSysInfo>
#include <QDesktopServices>
#include <QTimer>

// ====================================================================
// РЕАЛИЗАЦИЯ: JWBStoreInstaller
// ====================================================================
JWBStoreInstaller::JWBStoreInstaller(QObject *parent)
    : QObject(parent), m_downloadProgress(0.0)
{
    m_networkManager = new QNetworkAccessManager(this);
}

void JWBStoreInstaller::installApplication(const QString &downloadUrl, const QString &appName)
{
    if (downloadUrl.isEmpty()) {
        emit errorMessage("Error: this release doesn't have a .click package");
        emit statusChanged("error");
        return;
    }

    m_targetAppName = appName;
    m_downloadProgress = 0.0;

    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(cacheDir);
    QUrl url(downloadUrl);
    QString targetFileName = url.fileName();

    if (targetFileName.isEmpty() || !targetFileName.endsWith(".click")) {
        targetFileName = appName.toLower().remove("-") + ".click";
    }

    m_localFilePath = cacheDir + "/" + targetFileName;

    // ====================================================================
    // БРОНЕБОЙНЫЙ PKCON ИНСТАЛЛЕР (ФИКС ДОСТУПА APPARMOR ЧЕРЕЗ PUBLIC DOWNLOADS)
    // ====================================================================
    auto runPkconInstaller = [this]() {
        qDebug() << ">>> [JWB-Store] Копируем пакет в публичную папку Downloads для обхода изоляции AppArmor...";

        QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        QDir().mkpath(downloadDir);

        QFileInfo fileInfo(m_localFilePath);
        QString publicFilePath = downloadDir + "/" + fileInfo.fileName();

        if (QFile::exists(publicFilePath)) {
	   QFile::remove(publicFilePath);
        }

        if (!QFile::copy(m_localFilePath, publicFilePath)) {
	   qCritical() << ">>> [JWB-Store] Критическая ошибка: Не удалось скопировать файл в Downloads!";
	   emit errorMessage("AppArmor Workaround Failed: Copy error.");
	   emit statusChanged("error");
	   return;
        }

        QFile::setPermissions(publicFilePath,
	   QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
	   QFileDevice::ReadUser  | QFileDevice::WriteUser  | QFileDevice::ExeUser  |
	   QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup |
	   QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther
        );
        qDebug() << ">>> [JWB-Store] Файл успешно скопирован в общий доступ:" << publicFilePath;

        qDebug() << ">>> [JWB-Store] Launching pkcon с контролем тайм-аута...";
        QProcess *proc = new QProcess(this);
        proc->setProcessEnvironment(QProcessEnvironment::systemEnvironment());

        QTimer *timeoutTimer = new QTimer(this);
        timeoutTimer->setSingleShot(true);

        QString program = "pkcon";
        QStringList arguments;
        arguments << "install-local" << "--non-interactive" << "--allow-untrusted" << publicFilePath;

        // ТРИГГЕР ТАЙМ-АУТА: Если AppArmor заблокировал процесс
        connect(timeoutTimer, &QTimer::timeout, this, [this, proc, timeoutTimer, publicFilePath]() {
	   qCritical() << ">>> [JWB-Store] ВНИМАНИЕ: Превышен лимит ожидания! PKCON заблокирован.";

	   if (proc->state() != QProcess::NotRunning) {
	       proc->kill();
	   }

	   QFile::remove(publicFilePath);

	   emit errorMessage("Installation timeout: PKCON blocked. Opening File Manager...");
	   emit statusChanged("error");

	   QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
	   QDesktopServices::openUrl(QUrl::fromLocalFile(cachePath));

	   timeoutTimer->deleteLater();
        });

        // Коннект нормального завершения процесса
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
	      this, [this, proc, timeoutTimer, publicFilePath](int exitCode, QProcess::ExitStatus exitStatus) {

	   if (timeoutTimer && timeoutTimer->isActive()) {
	       timeoutTimer->stop();
	       timeoutTimer->deleteLater();
	   }

	   QFile::remove(publicFilePath);

	   if (exitStatus == QProcess::NormalExit && exitCode == 0) {
	       qDebug() << ">>> [JWB-Store] Пакет успешно установлен через pkcon фолбэк!";
	       emit statusChanged("installed");
	       QFile::remove(m_localFilePath);
	   } else {
	       qCritical() << ">>> [JWB-Store] Pkcon failed. Exit code:" << exitCode;
	       emit errorMessage("Installer failed with code: " + QString::number(exitCode));
	       emit statusChanged("error");

	       QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
	       QDesktopServices::openUrl(QUrl::fromLocalFile(cachePath));
	   }
	   proc->deleteLater();
        });

        proc->start(program, arguments);
        timeoutTimer->start(25000);
    };

    // ====================================================================
    // ПРОВЕРКА СТАРОГО КЭША (ФИКС ОФФЛАЙН-УСТАНОВКИ)
    // ====================================================================
    QFile cacheCheckFile(m_localFilePath);
    if (cacheCheckFile.exists() && cacheCheckFile.size() > 0) {
        qDebug() << ">>> [JWB-Cache] Найдена старая копия пакета в кэше! Размер:" << cacheCheckFile.size() << "байт.";
        qDebug() << ">>> [JWB-Cache] Пропускаем скачивание. Переходим сразу к установке из кэша...";

        m_downloadProgress = 1.0;
        emit downloadProgressChanged();
        emit statusChanged("installing");

#if defined(Q_OS_WIN)
        qDebug() << ">>> [JWB-Store] Windows Dev Mode: Имитация установки пакета из кэша:" << m_localFilePath;
        emit statusChanged("installed");
        QFile::remove(m_localFilePath);
#else
        // Напрямую дёргаем изолированный фикс-инсталлятор, D-Bus com.lomiri.click в OpenStore вырезан/заблокирован
        runPkconInstaller();
#endif
        return;
    }

    // ====================================================================
    // СТАНДАРТНЫЙ СЕТЕВОЙ СТРИМ (ЕСЛИ КЭША НЕТ)
    // ====================================================================
    if (QFile::exists(m_localFilePath)) {
        QFile::remove(m_localFilePath);
    }

    emit downloadProgressChanged();
    emit statusChanged("downloading");

    qDebug() << ">>> [JWB-Store] Старый кэш отсутствует. Начинаем стрим-загрузку в файл:" << m_localFilePath;

    QFile *outputFile = new QFile(m_localFilePath, this);
    if (!outputFile->open(QIODevice::WriteOnly)) {
        qCritical() << ">>> [JWB-Store] Cannot open target file for writing!";
        emit errorMessage("Failed to open local cache file");
        emit statusChanged("error");
        outputFile->deleteLater();
        return;
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#else
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#endif

    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::readyRead, this, [outputFile, reply]() {
        if (outputFile && outputFile->isOpen()) {
	  QByteArray data = reply->readAll();
	  outputFile->write(data);
	  qDebug() << ">>> [JWB-Store] Chunk appended:" << data.size() << "bytes | Size:" << outputFile->size();
        }
    });

    connect(reply, &QNetworkReply::downloadProgress, this, &JWBStoreInstaller::onDownloadProgress);

    connect(reply, &QNetworkReply::finished, this, [this, reply, outputFile, runPkconInstaller]() {
        if (outputFile) {
	  if (outputFile->isOpen()) {
	      outputFile->flush();
	      outputFile->close();
	  }
	  outputFile->deleteLater();
        }

        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
	  qCritical() << ">>> [JWB-Store] Network failed:" << reply->errorString();
	  emit errorMessage("Package download error: " + reply->errorString());
	  emit statusChanged("error");
	  return;
        }

        QFile file(m_localFilePath);
        if (!file.exists() || file.size() == 0) {
	  qCritical() << ">>> [JWB-Store] File error: size is 0 after closing stream!";
	  emit errorMessage("Downloaded file is empty.");
	  emit statusChanged("error");
	  return;
        }

        qDebug() << ">>> [JWB-Store] Download finished successfully. Total size written to disk:" << file.size() << "bytes.";
        emit statusChanged("installing");

#if defined(Q_OS_WIN)
        qDebug() << ">>> [JWB-Store] Windows Dev Mode: Имитация установки пакета:" << m_localFilePath;
        emit statusChanged("installed");
        QFile::remove(m_localFilePath);
#else
        runPkconInstaller();
#endif
    });
}

void JWBStoreInstaller::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        m_downloadProgress = static_cast<double>(bytesReceived) / bytesTotal;
        qDebug() << ">>> [JWB-Store] Progress Dynamic Sync:" << bytesReceived << "/" << bytesTotal
	      << QString(" (%1%)").arg(int(m_downloadProgress * 100));
    } else {
        double megabytes = static_cast<double>(bytesReceived) / (1024.0 * 1024.0);
        m_downloadProgress = -megabytes;
        qDebug() << ">>> [JWB-Store] Receiving chunks:" << bytesReceived << "bytes |" << QString("%1 MB").arg(megabytes, 0, 'f', 2);
    }
    emit downloadProgressChanged();
}

void JWBStoreInstaller::onDownloadFinished(QNetworkReply *reply)
{
    reply->deleteLater();
}

QString JWBStoreInstaller::getInstalledVersion(const QString &packageName)
{
    if (packageName.isEmpty()) return "0.0.0";

#if defined(Q_OS_WIN)
    qDebug() << ">>> [JWB-Store Dev Mode Windows 10] Запрос версии для:" << packageName;
    if (packageName.contains("sokoban")) return "1.5.0";
    return "0.0.0";
#else
    QDBusConnection bus = QDBusConnection::systemBus();
    if (bus.isConnected()) {
        QDBusInterface clickIface("com.lomiri.click", "/com/lomiri/click", "com.lomiri.click", bus);
        if (clickIface.isValid()) {
	  QDBusMessage checkReply = clickIface.call("IsInstalled", packageName);
	  if (checkReply.type() != QDBusMessage::ErrorMessage && !checkReply.arguments().isEmpty()) {
	      bool isInstalled = checkReply.arguments().at(0).toBool();
	      if (!isInstalled) {
		qDebug() << ">>> [JWB-Store D-Bus] Пакет" << packageName << "не установлен в системе.";
		return "0.0.0";
	      }

	      QDBusMessage versionReply = clickIface.call("GetPackageVersion", packageName);
	      if (versionReply.type() != QDBusMessage::ErrorMessage && !versionReply.arguments().isEmpty()) {
		QString versionStr = versionReply.arguments().at(0).toString();
		if (!versionStr.isEmpty()) {
		    qDebug() << ">>> [JWB-Store D-Bus] Найдена версия через API:" << versionStr;
		    return versionStr;
		}
	      }
	  }
        }
    }

    QString manifestPath = QString("/click/%1/current/manifest.json").arg(packageName);
    QFile manifestFile(manifestPath);

    if (manifestFile.exists() && manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray manifestData = manifestFile.readAll();
        manifestFile.close();

        QJsonDocument doc = QJsonDocument::fromJson(manifestData);
        if (!doc.isNull() && doc.isObject()) {
	  QJsonObject rootObj = doc.object();
	  if (rootObj.contains("version")) {
	      QString localVersion = rootObj["version"].toString();
	      qDebug() << ">>> [JWB-Store Фолбэк ФС] Версия считана напрямую из манифеста:" << localVersion;
	      return localVersion;
	      }
        }
    }

    return "0.0.0";
#endif
}

// ====================================================================
// РЕАЛИЗАЦИЯ: Ratings
// ====================================================================
Ratings::Ratings(const QMap<QString, QVariant>& map, QObject* parent)
  : QObject(parent)
{
    m_thumbsUpCount = map[ratingToString(RatingThumbsUp)].toInt();
    m_thumbsDownCount = map[ratingToString(RatingThumbsDown)].toInt();
    m_neutralCount = map[ratingToString(RatingNeutral)].toInt();
    m_happyCount = map[ratingToString(RatingHappy)].toInt();
    m_buggyCount = map[ratingToString(RatingBuggy)].toInt();
}

Ratings::Ratings(QObject* parent) : QObject(parent) {}

Ratings::Ratings(const Ratings& ratings)
  : QObject(Q_NULLPTR)
  , m_thumbsUpCount(ratings.thumbsUpCount())
  , m_thumbsDownCount(ratings.thumbsDownCount())
  , m_neutralCount(ratings.neutralCount())
  , m_happyCount(ratings.happyCount())
  , m_buggyCount(ratings.buggyCount())
{
}

unsigned int Ratings::thumbsUpCount() const { return m_thumbsUpCount; }
unsigned int Ratings::thumbsDownCount() const { return m_thumbsDownCount; }
unsigned int Ratings::neutralCount() const { return m_neutralCount; }
unsigned int Ratings::happyCount() const { return m_happyCount; }
unsigned int Ratings::buggyCount() const { return m_buggyCount; }
unsigned int Ratings::totalCount() const
{
    return m_thumbsUpCount + m_thumbsDownCount + m_neutralCount + m_happyCount + m_buggyCount;
}

QString Ratings::ratingToString(enum Rating rating)
{
    return stringToRatingMap().key(rating);
}

Rating Ratings::ratingFromString(const QString& rating)
{
    return stringToRatingMap()[rating];
}

QMap<QString, Rating>& Ratings::stringToRatingMap()
{
    static QMap<QString, Rating> map{ { "THUMBS_UP", RatingThumbsUp },
			 { "THUMBS_DOWN", RatingThumbsDown },
			 { "NEUTRAL", RatingNeutral },
			 { "HAPPY", RatingHappy },
			 { "BUGGY", RatingBuggy } };
    return map;
}

// ====================================================================
// РЕАЛИЗАЦИЯ: ReviewItem
// ====================================================================
ReviewItem::ReviewItem(const QJsonObject& json, QObject* parent)
  : QObject(parent)
{
    m_reviewId = json["id"].toString();
    m_author = json["author"].toString();
    m_body = json["body"].toString();
    m_rating = Ratings::ratingFromString(json["rating"].toString());
    m_reviewedVersion = json["version"].toString();
    QJsonObject jsonComment = json["comment"].toObject();
    m_comment = Comment(jsonComment["body"].toString(), jsonComment["date"].toVariant().toLongLong());
    m_isRedacted = json["redacted"].toBool();
    m_date = json["date"].toVariant().toLongLong();
}

ReviewItem::ReviewItem(const ReviewItem& review)
  : QObject(Q_NULLPTR)
  , m_author(review.m_author)
  , m_body(review.m_body)
  , m_rating(review.m_rating)
  , m_reviewedVersion(review.m_reviewedVersion)
  , m_comment(review.m_comment)
  , m_isRedacted(review.m_isRedacted)
  , m_date(review.m_date)
{
}

QString ReviewItem::id() const { return m_reviewId; }
QString ReviewItem::version() const { return m_reviewedVersion; }
Rating ReviewItem::rating() const { return m_rating; }
QString ReviewItem::body() const { return m_body; }
ReviewItem::Comment ReviewItem::comment() const { return m_comment; }
bool ReviewItem::redacted() const { return m_isRedacted; }
QString ReviewItem::author() const { return m_author; }
qlonglong ReviewItem::date() const { return m_date; }

// ====================================================================
// РЕАЛИЗАЦИЯ: JWBIconCache (ФИКС ОФФЛАЙН ИКОНОК)
// ====================================================================
JWBIconCache::JWBIconCache(QObject *parent) : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
    m_cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/icons/";
    QDir().mkpath(m_cacheDir);
}

QString JWBIconCache::getIconPath(const QString &appName, const QString &networkUrl)
{
    QString safeName = appName.toLower().trimmed().replace(" ", "-") + ".png";
    QString localFilePath = m_cacheDir + safeName;

    // Корректное преобразование пути в URL без лишних слэшей (file://)
    if (QFile::exists(localFilePath)) {
        return QUrl::fromLocalFile(localFilePath).toString();
    }

    if (networkUrl.isEmpty()) {
        return "qrc:/Assets/JWB-Denda Logo (none).svg";
    }

    QNetworkRequest request((QUrl(networkUrl)));
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#else
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#endif

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, appName, localFilePath]() {
        this->onDownloadFinished(reply, appName, localFilePath);
    });

    return "qrc:/Assets/JWB-Denda Logo (none).svg";
}

void JWBIconCache::onDownloadFinished(QNetworkReply *reply, const QString &appName, const QString &targetPath)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(appName, "Network error due to downloading an icon: " + reply->errorString());
        return;
    }

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorOccurred(appName, "Failed to save an icon into cache.");
        return;
    }

    file.write(reply->readAll());
    file.close();

    qDebug() << "Icon cached succesfully for:" << appName << "-> Path:" << targetPath;
    // Здесь тоже отдаем корректный строковый URL
    emit iconReady(appName, QUrl::fromLocalFile(targetPath).toString());
}

// ====================================================================
// РЕАЛИЗАЦИЯ: JWBReviewBackend
// ====================================================================
JWBReviewBackend::JWBReviewBackend(QObject *parent)
    : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
    resetStats();
}

void JWBReviewBackend::resetStats()
{
    m_countThumbsUp = 0;
    m_countThumbsDown = 0;
    m_countNeutral = 0;
    m_countHappy = 0;
    m_countBuggy = 0;
    m_reviewsList.clear();
}

void JWBReviewBackend::fetchReviews(const QString &orgName, const QString &repoName)
{
    if (repoName.isEmpty()) return;

    m_targetRepo = repoName;
    resetStats();
    emit statsChanged();
    emit reviewsLoaded();

    QString urlStr = QString("https://%1.github.io/JWB-Denda-Database/reviews.json").arg(orgName.toLower().trimmed());

    QNetworkRequest request((QUrl(urlStr)));

    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#else
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#endif

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->onNetworkReplyFinished(reply);
    });
}

void JWBReviewBackend::onNetworkReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("Отзывы отсутствуют на сервере (Код: " + QString::number(reply->error()) + ")");
        return;
    }

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode != 200) {
        emit errorOccurred("База отзывов пуста (HTTP " + QString::number(statusCode) + ")");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) return;

    QJsonObject rootObj = doc.object();

    if (rootObj.contains(m_targetRepo) && rootObj[m_targetRepo].isArray()) {
        QJsonArray reviewsArray = rootObj[m_targetRepo].toArray();

        for (const QJsonValue &value : reviewsArray) {
	if (!value.isObject()) continue;

	ReviewItem *item = new ReviewItem(value.toObject(), this);

	if (item->rating() == Ratings::RatingThumbsUp) m_countThumbsUp++;
	else if (item->rating() == Ratings::RatingThumbsDown) m_countThumbsDown++;
	else if (item->rating() == Ratings::RatingNeutral) m_countNeutral++;
	else if (item->rating() == Ratings::RatingHappy) m_countHappy++;
	else if (item->rating() == Ratings::RatingBuggy) m_countBuggy++;

	QVariantMap map;
	map["author"] = item->author();
	map["body"] = item->body();
	map["version"] = item->version();
	map["ratingStr"] = Ratings::ratingToString(item->rating());

	if (item->rating() == Ratings::RatingThumbsUp) map["emoji"] = "💚";
	else if (item->rating() == Ratings::RatingThumbsDown) map["emoji"] = "👾";
	else if (item->rating() == Ratings::RatingNeutral) map["emoji"] = "😐";
	else if (item->rating() == Ratings::RatingHappy) map["emoji"] = "😁";
	else if (item->rating() == Ratings::RatingBuggy) map["emoji"] = "👎🏻";

	m_reviewsList.append(map);
	item->deleteLater();
        }
    }

    emit statsChanged();
    emit reviewsLoaded();
}

void JWBReviewBackend::addLocalReview(const QString &repoName, const QString &author, const QString &ratingStr, const QString &text)
{
    if (repoName != m_targetRepo || repoName.isEmpty()) return;

    Ratings::Rating r = Ratings::ratingFromString(ratingStr);

    if (r == Ratings::RatingThumbsUp) m_countThumbsUp++;
    else if (r == Ratings::RatingThumbsDown) m_countThumbsDown++;
    else if (r == Ratings::RatingNeutral) m_countNeutral++;
    else if (r == Ratings::RatingHappy) m_countHappy++;
    else if (r == Ratings::RatingBuggy) m_countBuggy++;

    QVariantMap map;
    map["author"] = author;
    map["body"] = text;
    map["version"] = "Local";
    map["ratingStr"] = ratingStr;

    if (r == Ratings::RatingThumbsUp) map["emoji"] = "💚";
    else if (r == Ratings::RatingThumbsDown) map["emoji"] = "👾";
    else if (r == Ratings::RatingNeutral) map["emoji"] = "😐";
    else if (r == Ratings::RatingHappy) map["emoji"] = "😁";
    else if (r == Ratings::RatingBuggy) map["emoji"] = "👎🏻";

    m_reviewsList.prepend(map);

    emit statsChanged();
    emit reviewsLoaded();
}

QVariantMap JWBReviewBackend::detectCurrentSystem()
{
    QVariantMap osData;
    QString osType = "unknown";
    QString osName = "Unknown OS";

#if defined(Q_OS_LINUX)
    QString osId = "unknown";
    QString variantId = "unknown";
    QFile file("/etc/os-release");

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
	QString line = in.readLine();
	if (line.startsWith("ID=")) {
	    osId = line.mid(3).remove('"').trimmed();
	} else if (line.startsWith("VARIANT_ID=")) {
	    variantId = line.mid(11).remove('"').trimmed();
	}
        }
        file.close();
    }

    bool hasUtFile = QFile::exists("/etc/ubuntu-touch");
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    bool isLomiri = hasUtFile ||
	      osId == "ubuntu-touch" ||
	      variantId == "touch" ||
	      env.contains("UBUNTU_TOUCH") ||
	      env.contains("LOMIRI_APP_LAUNCH_ENV");

    if (isLomiri) {
        if (osId == "manjaro") {
	osType = "manjaro_lomiri";
	osName = "Manjaro ARM (Lomiri Desktop)";
        } else if (osId == "kali") {
	osType = "kali_lomiri";
	osName = "Kali Linux (Lomiri HackEdition)";
        } else if (osId == "rhino") {
	osType = "rhino_lomiri";
	osName = "Rhino Linux (Unicorn-Lomiri Roll)";
        } else if (osId == "nixos") {
	osType = "nixos_lomiri";
	osName = "NixOS (Declarative Lomiri)";
        } else {
	osType = "ubuntu_touch";
	osName = "Ubuntu Touch (Lomiri)";
        }
    } else {
        if (osId == "kali") {
	osType = "kali_desktop";
	osName = "Kali Linux Desktop";
        } else if (osId == "nixos") {
	osType = "nixos_desktop";
	osName = "NixOS Desktop";
        } else if (osId == "gentoo") {
	osType = "gentoo_desktop";
	osName = "Gentoo Linux (Compiled Pure)";
        } else if (osId == "linux") {
	osType = "linux";
	osName = "GNU/Linux (" + osId + ")";
        }
    }

#elif defined(Q_OS_WIN)
    osType = "windows";
    if (QSysInfo::kernelVersion().startsWith("10")) {
        osName = "Windows 10";
    } else {
        osName = "Windows " + QSysInfo::productVersion();
    }
#elif defined(Q_OS_ANDROID)
    osType = "android";
    osName = "Android " + QSysInfo::productVersion();
#elif defined(Q_OS_IOS)
    osType = "ios";
    osName = "iOS " + QSysInfo::productVersion();
#endif

    osData["type"] = osType;
    osData["name"] = osName;

    return osData;
}

QVariantMap JWBReviewBackend::loadProfile()
{
    QVariantMap profile;
    profile["nickname"] = "Anonymous";
    profile["osName"] = "Not Detected Yet";
    profile["osType"] = "unknown";
    profile["device"] = "Basical QNX Device";

    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QFile file(configPath + "/user_profile.json");

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull() && doc.isObject()) {
	QJsonObject obj = doc.object();
	if (obj.contains("nickname")) profile["nickname"] = obj["nickname"].toString();
	if (obj.contains("osName"))   profile["osName"]   = obj["osName"].toString();
	if (obj.contains("osType"))   profile["osType"]   = obj["osType"].toString();
	if (obj.contains("device"))   profile["device"]   = obj["device"].toString();
        }
    }
    return profile;
}

bool JWBReviewBackend::saveProfile(const QString &nickname, const QString &osName, const QString &osType, const QString &device)
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configPath);
    if (!dir.exists()) {
        dir.mkpath(configPath);
    }

    QFile file(configPath + "/user_profile.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[JWB-Backend] Failed to open profile file for writing";
        return false;
    }

    QJsonObject obj;
    obj["nickname"] = nickname;
    obj["osName"] = osName;
    obj["osType"] = osType;
    obj["device"] = device;

    QJsonDocument doc(obj);
    file.write(doc.toJson());
    file.close();

    qDebug() << "[JWB-Store] Profile successfully written to local JSON database.";
    return true;
}
