#ifndef SEARCHMODEL_H
#define SEARCHMODEL_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class SearchModel : public QObject
{
    Q_OBJECT
public:
    explicit SearchModel(QObject *parent = nullptr);

    // Метод принимает массив приложений и поисковый запрос от QML
    Q_INVOKABLE QVariantList filterApps(const QVariantList &allApps, const QString &query);
};

#endif // SEARCHMODEL_H
