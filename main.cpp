#include <QGuiApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtDebug>
#include <QSslSocket>
#include <QDebug>
#include <QHostInfo>

#include "Plug-ins/jwbstore.h"
#include "Plug-ins/api.h"
#include "Plug-ins/searchmodel.h"
#include "Plug-ins/appsreviewsapisystem.h"
#include "Plug-ins/categoriesmodel.h" // <<< 1. ДОБАВИЛИ ИНКЛЮД НАШЕЙ НОВОЙ МОДЕЛИ!

int main(int argc, char *argv[])
{
    // High DPI для корректного отображения
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    //QCoreApplication::setApplicationVersion(QString(BUILD_VERSION));
    QCoreApplication::setApplicationName("jwb-denda.jwb-tutantxamon-n-jwb-bravada");

    QSslConfiguration sslCfg = QSslConfiguration::defaultConfiguration();
    sslCfg.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslCfg.setProtocol(QSsl::TlsV1_2OrLater);
    QSslConfiguration::setDefaultConfiguration(sslCfg);


    qDebug() << "==================================================";
    qDebug() << "Running into Qt Core... Initializating the app and system";
    qDebug() << "==================================================";
    qDebug() << "";
    qDebug() << "";
    qDebug() << "";
    qDebug() << "########################################";
    qDebug() << "JWB-Denda. The JasonWalt Bab@'s Store.";
    qDebug() << "########################################";
    qDebug() << "System triggered successfully";

    // Регистрация API
    qmlRegisterType<GitHubApi>("JWBStore.API.io", 1, 0, "GitHubApi");
    qmlRegisterType<JWBStoreInstaller>("JWBDenda.Installer.io", 1, 0, "JWBStoreInstaller");
    qmlRegisterType<SearchModel>("JWBStore.API.io", 1, 0, "SearchModel");
    qmlRegisterType<JWBIconCache>("JWBStore.API.io", 1, 0, "JWBIconCache");
    qmlRegisterType<JWBReviewBackend>("JWBStore.Backend", 1, 0, "JWBReviewBackend");
    qmlRegisterType<AppsReviewsAPISystem>("JWBStore.API.io", 1, 0, "ReviewItemAPI");

    // <<< 2. ЗАРЕГИСТРИРОВАЛИ ТИП ДЛЯ QML (в то же пространство имён, где сидит SearchModel)
    qmlRegisterType<CategoriesModel>("JWBStore.API.io", 1, 0, "CategoriesModel");

    QQmlApplicationEngine engine;

    // ОБРАТИ ВНИМАНИЕ НА СТРЕЛОЧКУ ТУТ!
    // reviewBackend создан через 'new', значит это указатель (JWBReviewBackend*)
    // Передаем адрес родителя через указатель &app (тут все верно, app на стеке)
    JWBReviewBackend *reviewBackend = new JWBReviewBackend(&app);
    engine.rootContext()->setContextProperty("jwbReviewBackend", reviewBackend);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
