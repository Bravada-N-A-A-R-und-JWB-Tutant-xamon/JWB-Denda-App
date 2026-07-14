#ifndef APPSREVIEWSAPISYSTEM_H
#define APPSREVIEWSAPISYSTEM_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QDebug>

class AppsReviewsAPISystem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isSending READ isSending NOTIFY isSendingChanged)

public:
    explicit AppsReviewsAPISystem(QObject *parent = nullptr);

    bool isSending() const { return m_isSending; }

    // Основной метод отправки отзыва на сервер (доступен из QML)
    Q_INVOKABLE void sendReview(const QString &orgName,
			     const QString &repoName,
			     const QString &author,
			     const QString &ratingStr,
			     const QString &text,
			     const QString &osType,
			     const QString &device);

signals:
    void isSendingChanged();
    void reviewSentSuccessfully();
    void errorOccurred(QString message);

private:
    QNetworkAccessManager *m_networkManager;
    bool m_isSending = false;
};

#endif // APPSREVIEWSAPISYSTEM_H
