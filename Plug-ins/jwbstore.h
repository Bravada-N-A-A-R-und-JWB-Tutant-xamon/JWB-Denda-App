#ifndef JWBSTORE_H
#define JWBSTORE_H

#include <QObject>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMap>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QVariantList>
#include <QVariantMap>

#if defined(Q_OS_LINUX)
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusConnection>
#endif

// ====================================================================
// 1. КЛАСС: УСТАНОВЩИК ПАКЕТОВ (JWBStoreInstaller)
// ====================================================================
class JWBStoreInstaller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)

public:
    explicit JWBStoreInstaller(QObject *parent = nullptr);

    Q_INVOKABLE void installApplication(const QString &downloadUrl, const QString &appName);
    double downloadProgress() const { return m_downloadProgress; }
    Q_INVOKABLE QString getLocalFilePath() const { return m_localFilePath; }

    // Бескомпромиссная проверка обновлений
    Q_INVOKABLE QString getInstalledVersion(const QString &packageName);

signals:
    void statusChanged(QString status); // "downloading", "installing", "installed", "error"
    void downloadProgressChanged();
    void errorMessage(QString message);

private slots:
    void onDownloadFinished(QNetworkReply *reply);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    QNetworkAccessManager *m_networkManager;
    double m_downloadProgress;
    QString m_targetAppName;
    QString m_localFilePath;
};

// ====================================================================
// 2. КЛАСС: СИСТЕМА ОЦЕНОК (Ratings)
// ====================================================================
class Ratings : public QObject
{
    Q_OBJECT
    Q_ENUMS(Rating)
    Q_PROPERTY(unsigned int thumbsUpCount READ thumbsUpCount NOTIFY updated)
    Q_PROPERTY(unsigned int thumbsDownCount READ thumbsDownCount NOTIFY updated)
    Q_PROPERTY(unsigned int neutralCount READ neutralCount NOTIFY updated)
    Q_PROPERTY(unsigned int happyCount READ happyCount NOTIFY updated)
    Q_PROPERTY(unsigned int buggyCount READ buggyCount NOTIFY updated)
    Q_PROPERTY(unsigned int totalCount READ totalCount NOTIFY updated)

public:
    explicit Ratings(const QMap<QString, QVariant>& map, QObject* parent = Q_NULLPTR);
    explicit Ratings(QObject* parent = Q_NULLPTR);
    explicit Ratings(const Ratings& ratings);

    enum Rating
    {
        RatingNone = -1,
        RatingThumbsUp = 0,
        RatingThumbsDown = 1,
        RatingNeutral = 2,
        RatingHappy = 3,
        RatingBuggy = 4,
    };

    static QString ratingToString(enum Rating rating);
    static QString ratingsToString(enum Rating rating) { return ratingToString(rating); }
    static Rating ratingFromString(const QString& rating);

    unsigned int thumbsDownCount() const;
    unsigned int thumbsUpCount() const;
    unsigned int neutralCount() const;
    unsigned int happyCount() const;
    unsigned int buggyCount() const;
    unsigned int totalCount() const;

Q_SIGNALS:
    void updated();

private:
    static QMap<QString, Rating>& stringToRatingMap();

    unsigned int m_thumbsUpCount = 0;
    unsigned int m_thumbsDownCount = 0;
    unsigned int m_neutralCount = 0;
    unsigned int m_happyCount = 0;
    unsigned int m_buggyCount = 0;
};

typedef Ratings::Rating Rating;

// ====================================================================
// 3. КЛАСС: ЭЛЕМЕНТ ОТЗЫВА (ReviewItem)
// ====================================================================
class ReviewItem : public QObject
{
    Q_OBJECT
public:
    explicit ReviewItem(const QJsonObject& json, QObject* parent = Q_NULLPTR);
    explicit ReviewItem(const ReviewItem& review);

    class Comment
    {
    public:
        QString m_body;
        qlonglong m_date;

        Comment() : m_body(""), m_date(0) {}
        Comment(const QString& body, unsigned int date) : m_body(body), m_date(date) {}
        Comment(const Comment& comment) : m_body(comment.m_body), m_date(comment.m_date) {}

        QString body() const { return m_body; }
        qlonglong date() const { return m_date; }
    };

    QString id() const;
    QString version() const;
    Rating rating() const;
    QString body() const;
    Comment comment() const;
    bool redacted() const;
    QString author() const;
    qlonglong date() const;

    QString m_reviewId;
    QString m_author;
    QString m_body;
    Rating m_rating;
    QString m_reviewedVersion;
    Comment m_comment;
    bool m_isRedacted;
    qlonglong m_date;
};

// ====================================================================
// 4. КЛАСС: УМНЫЙ КЭШ ИКОНОК (JWBIconCache)
// ====================================================================
class JWBIconCache : public QObject
{
    Q_OBJECT
public:
    explicit JWBIconCache(QObject *parent = nullptr);

    Q_INVOKABLE QString getIconPath(const QString &appName, const QString &networkUrl);

signals:
    void iconReady(QString appName, QString localPath);
    void errorOccurred(QString appName, QString message);

private slots:
    void onDownloadFinished(QNetworkReply *reply, const QString &appName, const QString &targetPath);

private:
    QNetworkAccessManager *m_networkManager;
    QString m_cacheDir;
};

// ====================================================================
// 5. КЛАСС: УПРАВЛЕНИЕ ОТЗЫВАМИ И БАЗОЙ (JWBReviewBackend)
// ====================================================================
class JWBReviewBackend : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int countThumbsUp READ countThumbsUp NOTIFY statsChanged)
    Q_PROPERTY(int countThumbsDown READ countThumbsDown NOTIFY statsChanged)
    Q_PROPERTY(int countNeutral READ countNeutral NOTIFY statsChanged)
    Q_PROPERTY(int countHappy READ countHappy NOTIFY statsChanged)
    Q_PROPERTY(int countBuggy READ countBuggy NOTIFY statsChanged)
    Q_PROPERTY(QVariantList reviewsList READ reviewsList NOTIFY reviewsLoaded)

public:
    explicit JWBReviewBackend(QObject *parent = nullptr);

    Q_INVOKABLE void fetchReviews(const QString &orgName, const QString &repoName);
    Q_INVOKABLE void addLocalReview(const QString &repoName, const QString &author, const QString &ratingStr, const QString &text);
    Q_INVOKABLE QVariantMap detectCurrentSystem();

    Q_INVOKABLE QVariantMap loadProfile();
    Q_INVOKABLE bool saveProfile(const QString &nickname, const QString &osName, const QString &osType, const QString &device);

    int countThumbsUp() const { return m_countThumbsUp; }
    int countThumbsDown() const { return m_countThumbsDown; }
    int countNeutral() const { return m_countNeutral; }
    int countHappy() const { return m_countHappy; }
    int countBuggy() const { return m_countBuggy; }
    QVariantList reviewsList() const { return m_reviewsList; }

signals:
    void statsChanged();
    void reviewsLoaded();
    void errorOccurred(QString message);

private slots:
    void onNetworkReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;
    QString m_targetRepo;

    int m_countThumbsUp = 0;
    int m_countThumbsDown = 0;
    int m_countNeutral = 0;
    int m_countHappy = 0;
    int m_countBuggy = 0;

    QVariantList m_reviewsList;
    void resetStats();
};

Q_DECLARE_METATYPE(ReviewItem::Comment)
Q_DECLARE_METATYPE(Ratings)

#endif // JWBSTORE_H
