#ifndef API_H
#define API_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantList>
#include <QVariantMap>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QDebug>

class GitHubApi : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isRequestActive READ isRequestActive NOTIFY isRequestActiveChanged)
    Q_PROPERTY(QString progressText READ progressText NOTIFY progressTextChanged)
    Q_PROPERTY(QString orgName READ orgName WRITE setOrgName NOTIFY orgNameChanged)
    Q_PROPERTY(QString token READ token WRITE setToken NOTIFY tokenChanged)

public:
    explicit GitHubApi(QObject *parent = nullptr);
    ~GitHubApi();

    bool isRequestActive() const { return m_activeRequestsCount > 0; }
    QString progressText() const { return m_progressText; }

    QString orgName() const { return m_orgName; }
    void setOrgName(const QString &org) { if (m_orgName != org) { m_orgName = org; emit orgNameChanged(); } }

    QString token() const { return m_token; }
    void setToken(const QString &tok) { if (m_token != tok) { m_token = tok; emit tokenChanged(); } }

public slots:
    void fetchOrganizationRepos();
    void fetchReleaseInfo(const QString &repoName);
    void checkClickRelease(const QString &repoName);
    void clearCache();

signals:
    void isRequestActiveChanged(bool active);
    void progressTextChanged();
    void orgNameChanged();
    void tokenChanged();
    void reposListReceived(const QVariantList &repos);
    void releaseInfoReceived(QString name, QString ver, QString auth, QString sz, QString cat, QString desc, QString icon, QString dl, QString splash, QString archs);
    void clickReleaseChecked(bool available, QString dlUrl, QString sizeStr, QString archs);
    void errorOccurred(QString msg);

private slots:
    void onReposFetched();
    void onIconFetched(QNetworkReply *reply, const QString &repoName);
    void onReleaseInfoFetched(QNetworkReply *reply, const QString &repoName);
    void onClickChecked(QNetworkReply *reply, const QString &repoName);

private:
    void updateLoadingState(int delta);
    void injectAuthHeader(QNetworkRequest &request);
    void loadCachedRepos();
    void saveReposToCache(const QVariantList &repos);
    QString getCacheFilePath();

    QNetworkAccessManager *m_networkManager;
    int m_activeRequestsCount = 0;
    QString m_progressText;
    QString m_orgName = "Bravada-N-A-A-R-und-JWB-Tutant-xamon";
    QString m_token = "";
    QTimer *m_retryTimer = nullptr;
};

#endif // API_H
