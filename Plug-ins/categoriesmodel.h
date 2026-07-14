#ifndef CATEGORIESMODEL_H
#define CATEGORIESMODEL_H

#include <QAbstractListModel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QList>

struct CategoryItem
{
    QString id;
    QString name;
    int count;
    QUrl iconUrl;
};

class CategoriesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ ready NOTIFY updated)

public:
    enum Roles
    {
        RoleId = Qt::UserRole + 1, // В Qt роли пользователя должны начинаться отсюда
        RoleName,
        RoleCount,
        RoleIconUrl,
    };

    explicit CategoriesModel(QObject* parent = nullptr);
    ~CategoriesModel() override = default;

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    bool ready() const { return m_ready; }

    Q_INVOKABLE void update();

Q_SIGNALS:
    void updated();

private:
    QList<CategoryItem> m_list;
    bool m_ready;
    QNetworkAccessManager* m_networkManager;
};

#endif // CATEGORIESMODEL_H
