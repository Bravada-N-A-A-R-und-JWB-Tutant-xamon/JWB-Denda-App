#include "searchmodel.h"

SearchModel::SearchModel(QObject *parent) : QObject(parent) {}

QVariantList SearchModel::filterApps(const QVariantList &allApps, const QString &query)
{
    QVariantList filteredList;

    // Если в поиске пусто или одни пробелы — отдаем пустой список
    if (query.trimmed().isEmpty()) {
        return filteredList;
    }

    // Приводим запрос к нижнему регистру для независимого поиска
    QString cleanQuery = query.trimmed().toLower();

    for (const QVariant &appVar : allApps) {
        QVariantMap appMap = appVar.toMap();

        QString appName = appMap["appName"].toString().toLower();
        QString appAuthor = appMap["appAuthor"].toString().toLower();
        QString repoName = appMap["repoName"].toString().toLower();

        // Если поисковый запрос есть в названии, авторе или репозитории — забираем в результаты
        if (appName.contains(cleanQuery) || appAuthor.contains(cleanQuery) || repoName.contains(cleanQuery)) {
            filteredList.append(appMap);
        }
    }

    return filteredList;
}
