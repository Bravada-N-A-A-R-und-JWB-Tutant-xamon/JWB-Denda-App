#include "categoriesmodel.h"

CategoriesModel::CategoriesModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_ready(false)
      {
	 m_list.append({ "video",     "Video",      0, QUrl("qrc:/Assets/Icons/video.png") });
	 m_list.append({ "audio",       "Audio",    0, QUrl("qrc:/Assets/Icons/audio.png") });
	 m_list.append({ "personalization", "Personalization",  0, QUrl("qrc:/Assets/Icons/theme.png") });
	 m_list.append({ "games",     "Games",      0, QUrl("qrc:/Assets/Icons/games.png") });
	 m_list.append({ "education",     "Education",  0, QUrl("qrc:/Assets/Icons/education.png") });
	 m_list.append({ "dev_tools",      "Development Tools",  0, QUrl("qrc:/Assets/Icons/developer.png") });
	 m_list.append({ "accessibility",  "Accessibility" , 0, QUrl("qrc:/Assets/Icons/accessibility.png") });
	 m_list.append({ "communication", "Communication & Messengers",  0, QUrl("qrc:/Assets/Icons/chat.png") });
	 m_list.append({ "addons",     "Addons",     0, QUrl("qrc:/Assets/Icons/addons.png") });
	 m_list.append({ "system",       "System",   0, QUrl("qrc:/Assets/Icons/system.png") });

	 m_ready = true;
	 emit updated();
      }

      QHash<int, QByteArray> CategoriesModel::roleNames() const
      {
	 QHash<int, QByteArray> roles;
	 roles.insert(RoleId, "id");
	 roles.insert(RoleCount, "modelCount");
	 roles.insert(RoleIconUrl, "iconUrl");
	 return roles;
      }

      int CategoriesModel::rowCount(const QModelIndex& parent) const
      {
	 Q_UNUSED(parent)
	 return m_list.count();
      }

      QVariant CategoriesModel::data(const QModelIndex& index, int role) const
      {
	 if (index.row() < 0 || index.row() >= m_list.count())
	     return QVariant();

	 const CategoryItem& cat = m_list.at(index.row());

	 switch (role) {
	     case RoleId:
		return cat.id;
	     case RoleCount:
		return cat.count;
	     case RoleIconUrl:
		return cat.iconUrl.toString();
	     default:
		return QVariant();
	 }
      }

      void CategoriesModel::update()
      {
	 m_ready = true;
	 emit updated();
      }
