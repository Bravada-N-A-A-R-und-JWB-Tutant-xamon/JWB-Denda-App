#include "appsreviewsapisystem.h"

AppsReviewsAPISystem::AppsReviewsAPISystem(QObject *parent)
    : QObject(parent), m_isSending(false)
{
    m_networkManager = new QNetworkAccessManager(this);
}

void AppsReviewsAPISystem::sendReview(const QString &orgName, const QString &repoName, const QString &author, const QString &ratingStr, const QString &text, const QString &osType, const QString &device)
{
    if (orgName.isEmpty() || repoName.isEmpty()) {
        emit errorOccurred("Error: Organization or Repository name is empty.");
        return;
    }

    m_isSending = true;
    emit isSendingChanged();

    // Собираем пакет данных отзыва
    QJsonObject reviewJson;
    reviewJson["author"] = author.isEmpty() ? "Anonymous Linux User" : author;
    reviewJson["rating"] = ratingStr; // "THUMBS_UP", "BUGGY", "HAPPY"
    reviewJson["body"] = text;
    reviewJson["version"] = "1.0.0"; // Сюда можно прокидывать динамическую версию
    reviewJson["date"] = QDateTime::currentMSecsSinceEpoch() / 1000;
    reviewJson["os_type"] = osType;
    reviewJson["device"] = device;

    QJsonObject rootObj;
    rootObj["repo"] = repoName;
    rootObj["org"] = orgName;
    rootObj["review"] = reviewJson;

    QJsonDocument doc(rootObj);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);

    // Урл твоего продакшн-сервера бэкенда для приёма отзывов
    QUrl serverUrl("https://github.com/Bravada-N-A-A-R-und-JWB-Tutant-xamon/JWB-Denda-Database/issues");
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Отрубаем TLS-проверки, чтобы кривые сертификаты или прокси не вешали отправку
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);

    QNetworkReply *reply = m_networkManager->post(request, postData);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_isSending = false;
        emit isSendingChanged();

        if (reply->error() != QNetworkReply::NoError) {
	   qCritical() << ">>> [JWB-Reviews] API Post failed:" << reply->errorString();
	   emit errorOccurred("Server connection error: " + reply->errorString());
        } else {
	   qDebug() << ">>> [JWB-Reviews] Review successfully pushed to server database!";
	   emit reviewSentSuccessfully();
        }
    });
}
