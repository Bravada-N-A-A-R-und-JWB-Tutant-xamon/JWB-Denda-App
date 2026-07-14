import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12 as QLayouts
import JWBStore.API.io 1.0
import "Modules/JWB-Store.js" as FarADayPlugin

Page {
    id: categories_page

    property color bgFolder: "#0A0A0A"
    property color cardColor: "#141414"
    property color borderColor: "#262626"
    property color accentColor: "#00FFCC"
    property color textColor: "#FFFFFF"

    background: Rectangle {
	color: bgFolder
    }

    signal categoryTriggered(var name, var id)

    onCategoryTriggered: {
	var pageProps = {
	    "title": name,
	    "category": id
	}
	if (typeof page_manager !== "undefined") {
	    page_manager.push(filteredAppPageComponent, pageProps);
	}
    }

    header: ToolBar {
	height: 50
	background: Rectangle {
	    color: categories_page.cardColor
	    border.color: categories_page.borderColor
	    border.width: 1
	}

	QLayouts.RowLayout {
	    anchors.fill: parent
	    anchors.leftMargin: 15; anchors.rightMargin: 15

	    ToolButton {
		text: "<"
		contentItem: Text {
		    text: parent.text
		    font.pointSize: 11
		    verticalAlignment: Text.AlignVCenter
		    horizontalAlignment: Text.AlignHCenter
		    anchors.fill: parent
		    color: categories_page.accentColor
		}
		onClicked: page_manager.pop();
		background: Rectangle {
		    color: "#111111"
		    border.width: 2
		    border.color: borderColor
		    radius: height / 2
		}
		implicitHeight: 40
		implicitWidth: implicitHeight
	    }
	    Label {
		text: qsTr("Categories")
		color: categories_page.accentColor
		font.pixelSize: 20
		font.bold: true
		QLayouts.Layout.fillWidth: true
	    }
	}
    }

    ScrollView {
	anchors.fill: parent

	ListView {
	    id: categoriesList
	    anchors.fill: parent
	    model: CategoriesModel {}

	    property int __currentTMPIndex: 0
	    currentIndex: __currentTMPIndex

	    delegate: Rectangle {
		color: "transparent"
		border.width: 0.5
		border.color: categories_page.borderColor
		height: 50
		width: parent.width

		// Функция сопоставления ID и переводимого значения
		function getCategoryName(sysId) {
		    switch(sysId) {
			case "video":           return qsTr("Video");
			case "audio":           return qsTr("Audio");
			case "personalization": return qsTr("Personalization");
			case "games":           return qsTr("Games");
			case "education":       return qsTr("Education");
			case "dev_tools":       return qsTr("Development Tools");
			case "accessibility":   return qsTr("Accessibility");
			case "communication":   return qsTr("Communication & Messengers");
			case "addons":          return qsTr("Add-on's");
			case "system":          return qsTr("System");
			default:                return sysId;
		    }
		}

		QLayouts.RowLayout {
		    anchors.fill: parent
		    anchors.leftMargin: 10
		    spacing: 15

		    Image {
			source: model.iconUrl
			width: 30
			height: width
			fillMode: Image.PreserveAspectFit
		    }

		    Label {
			color: categories_page.accentColor
			// Передаем системный ID в функцию перевода
			text: "%1 (%2)".arg(getCategoryName(model.id)).arg(modelCount)
		    }
		}

		MouseArea {
		    anchors.fill: parent
		    onClicked: {
			categoriesList.__currentTMPIndex = model.index;
			// В сигнал отправляем уже переведенное имя для заголовка следующей страницы и чистый ID для фильтрации
			categoryTriggered(getCategoryName(model.id), model.id);
		    }
		}
	    }

	    Label {
		visible: count === 0
		anchors.fill: parent
		text: qsTr("Nothing to see here yet...")
		horizontalAlignment: Label.AlignHCenter
		verticalAlignment: Label.AlignVCenter
		color: categories_page.accentColor
	    }
	}
    }

    Component.onCompleted: {
	var theme = FarADayPlugin.getRandomTheme()
	categories_page.bgFolder = theme.bg
	categories_page.cardColor = theme.card
	categories_page.accentColor = theme.accent
    }
}
