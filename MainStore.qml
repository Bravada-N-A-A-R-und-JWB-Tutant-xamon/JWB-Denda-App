import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import JWBStore.API.io 1.0
import QtQml.Models 2.14
import "Modules/JWB-Store.js" as JWBStorePlugin

Page {
    id: store_home

    // Цвета динамической темы (задаются при старте из JS)
    property color bgFolder: "#0A0A0A"
    property color cardColor: "#141414"
    property color borderColor: "#262626"
    property color accentColor: "#00FFCC"
    property color textColor: "#FFFFFF"
    property color bannerBgColor: "#141414" // Цвет для подложки баннера

    background: Rectangle { color: store_home.bgFolder }

    GitHubApi {
        id: storeApi

        onReposListReceived: (repos) => {
            loadingIndicator.visible = false
            errorText.visible = false
            appsModel.clear()

            // 1. Сначала заполняем основную модель данными
            for (var i = 0; i < repos.length; i++) {
                appsModel.append(repos[i])
            }

            // 2. Раскидываем элементы по группам для разделения списков софта и игр
            for (var j = 0; j < appsModel.count; j++) {
                var item = appsModel.get(j)
                var cat = item.appCategory ? item.appCategory.toLowerCase() : "apps"

                if (cat === "games" || cat === "game") {
                    visualModel.items.get(j).inGames = true
                }
            }

            if (appsModel.count > 0) {
                featuredLoader.setFeatured(appsModel.get(0))
            }
        }

        // === ИСПРАВЛЕНО: Ловим Base64-иконки, прилетающие от фоновых curl-потоков ===
        onReleaseInfoReceived: (appName, appVersion, appAuthor, appSize, appCategory, appDescription, appIcon, appUrl, appSplashColor, appArchitectures) => {
            if (appIcon && appIcon.indexOf("data:image") === 0) {
                // Ищем приложение в модели по repoName (оно же appName в данном сигнале)
                for (var i = 0; i < appsModel.count; i++) {
                    if (appsModel.get(i).repoName === appName) {
                        // Обновляем иконку на Base64 строку прямо в модели!
                        appsModel.setProperty(i, "appIcon", appIcon);
                        break;
                    }
                }
            }
        }

        onErrorOccurred: (message) => {
            loadingIndicator.visible = false
            errorText.text = message
            errorText.visible = true
        }
    }

    ListModel { id: appsModel }

    // Настройка групп фильтрации: разделяем софт и игры
    DelegateModel {
        id: visualModel
        model: appsModel

        groups: [
            DelegateModelGroup {
                id: gamesGroup
                name: "games"
            }
        ]

        filterOnGroup: "items" // Верхняя лента показывает всё подряд

        delegate: Item {
            width: 105
            height: 150

            ColumnLayout {
                anchors.fill: parent
                spacing: 5

                Rectangle {
                    Layout.preferredWidth: 95
                    Layout.preferredHeight: 95
                    color: "#161616"
                    radius: 14
                    border.color: store_home.borderColor
                    Layout.alignment: Qt.AlignHCenter

                    Image {
                        // === ИСПРАВЛЕНО: Если там сетевой URL, ставим заглушку, чтобы Qt не ломал TLS. ===
                        // === Как только curl пришлёт Base64, сработает setProperty и картинка обновится! ===
                        source: (model.appIcon && model.appIcon.indexOf("http") !== 0)
                                ? model.appIcon
                                : "qrc:/Assets/JWB-Denda Logo (none).svg"
                        anchors.fill: parent
                        anchors.margins: 12
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            openPage("ApplicationPage.qml", {
                                "appName": model.appName,
                                "repoName": model.repoName,
                                "appVersion": model.appVersion,
                                "appAuthor": model.appAuthor,
                                "appSize": model.appSize ? model.appSize : "0.0 MB",
                                "appDescription": model.appDescription ? model.appDescription : "",
                                "appIcon": model.appIcon,
                                "appUrl": model.appUrl ? model.appUrl : "",
                                "appSplashColor": model.appSplashColor ? model.appSplashColor : "#141414",
                                "appArchitectures": model.appArchitectures ? model.appArchitectures : "all",
                                "appCategory": model.appCategory ? model.appCategory : "apps",
                                "token": storeApi.token
                            })
                        }
                    }
                }

                Label {
                    text: model.appName ? model.appName : ""
                    color: store_home.textColor
                    font.bold: true
                    font.pixelSize: 13
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                Label {
                    text: model.appVersion ? "v" + model.appVersion : "v1.0.0"
                    color: "#666666"
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }

    header: ToolBar {
        height: 50
        background: Rectangle {
            color: store_home.cardColor
            border.color: store_home.borderColor
            border.width: 1
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 15; anchors.rightMargin: 15

            Label {
                text: "JWB-Denda"
                color: store_home.accentColor
                font.pixelSize: 20
                font.bold: true
                Layout.fillWidth: true
            }
            ToolButton {
                    text: "👤"
                    contentItem: Text {
                        text: parent.text
                        font.pointSize: 11
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        anchors.fill: parent
                        color: "#fff"
                    }

                    onClicked: openPage("ProfilePage.qml") // Открываем пустой холст, который мы сейчас оживим!

                    background: Rectangle {
                        color: "#111111"
                        border.width: 2
                        border.color: store_home.borderColor
                        radius: height / 2
                    }
                    implicitHeight: 40
                    implicitWidth: implicitHeight
                }

            ToolButton {
               /* text: "🔍"
                contentItem: Text {
                    text: parent.text
                    font.pointSize: 10
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    anchors.fill: parent
                    color: "#fff"
                }*/

                contentItem: Image {
                    id: icon
                    source: "Assets/icons/search.svg"
                    width: 50
                    height: 50
                    anchors.centerIn: parent
                }

                onClicked: openPage("SearchPage.qml", {
                    "appsModelRef": appsModel,
                    "storeApiRef": storeApi
                })

                background: Rectangle {
                    color: "#111111"
                    border.width: 2
                    border.color: store_home.borderColor
                    radius: height / 2
                }
                implicitHeight: 40
                implicitWidth: implicitHeight
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 0

            // БАННЕР
            Item {
                id: featuredLoader
                Layout.fillWidth: true
                height: 180

                property var currentApp: null
                function setFeatured(data) { currentApp = data }

                Rectangle {
                    anchors.fill: parent
                    color: store_home.bannerBgColor
                    border.color: store_home.borderColor
                    border.width: 1
                    Image {
                        id: wallpapers
                        fillMode: Image.PreserveAspectCrop
                        source: "Assets/JWB-Denda banner.svg"
                        anchors.fill: parent
                        opacity: 0.4
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 20

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Rectangle {
                            width: 75; height: 20
                            color: store_home.accentColor
                            radius: 3
                            Label { text: qsTr("FEATURED"); color: "#000"; font.bold: true; font.pixelSize: 10; anchors.centerIn: parent }
                        }

                        Label {
                            text: featuredLoader.currentApp ? featuredLoader.currentApp.appName + " Ubuntu Touch" : qsTr("JWB-Denda Market")
                            color: store_home.textColor
                            font.pixelSize: 22
                            font.bold: true
                        }
                        Label {
                            text: featuredLoader.currentApp ? featuredLoader.currentApp.appDescription : qsTr("The ultimate open source ecosystem companion.")
                            color: "#888888"
                            font.pixelSize: 13
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            clip: true
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }

            // КАТЕГОРИИ И УСТАНОВЛЕННЫЕ ПРИЛОЖЕНИЯ
            Rectangle {
                Layout.fillWidth: true
                height: 60
                color: store_home.bgFolder

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: store_home.cardColor
                        radius: 4
                        border.color: store_home.borderColor

                        Label { text: qsTr("Browse Categories"); color: store_home.textColor; font.bold: true; font.pixelSize: 13; anchors.centerIn: parent }
                        MouseArea { anchors.fill: parent; onClicked: openPage("CategoriesPage.qml") }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: store_home.cardColor
                        radius: 4
                        border.color: store_home.borderColor

                        Label { text: qsTr("Installed Apps"); color: store_home.textColor; font.bold: true; font.pixelSize: 13; anchors.centerIn: parent }
                        MouseArea { anchors.fill: parent; onClicked: openPage("InstalledAppsPage.qml") }
                    }
                }
            }

            // ИНДИКАТОРЫ ЗАГРУЗКИ
            BusyIndicator {
                id: loadingIndicator;
                Layout.alignment: Qt.AlignHCenter;
                Layout.topMargin: 20;
                visible: storeApi.isRequestActive
            }

            Label {
                id: progressLabel
                text: storeApi.progressText
                color: store_home.accentColor
                font.pixelSize: 13
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 6
                visible: storeApi.isRequestActive && storeApi.progressText !== ""
            }

            Label { id: errorText; color: "#FF3333"; Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 20; visible: false }

            // СЕКЦИЯ: ЛЕНТА НОВИНОК (ВСЕ ПРИЛОЖЕНИЯ)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 15
                Layout.leftMargin: 15
                spacing: 12

                Label {
                    text: qsTr("New and Updated Apps")
                    color: store_home.textColor
                    font.pixelSize: 16
                    font.bold: true
                }

                ListView {
                    id: horizontalAppsList
                    Layout.fillWidth: true
                    height: 160
                    model: visualModel
                    orientation: ListView.Horizontal
                    spacing: 15
                    clip: true
                }
            }

            // СЕКЦИЯ: ИГРЫ
            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 25
                Layout.leftMargin: 15
                Layout.bottomMargin: 20
                spacing: 12

                Label {
                    text: qsTr("Games")
                    color: store_home.textColor
                    font.pixelSize: 16
                    font.bold: true
                }

                ListView {
                    id: gamesAppsList
                    Layout.fillWidth: true
                    height: 160
                    spacing: 15
                    orientation: ListView.Horizontal
                    clip: true

                    model: DelegateModel {
                        model: appsModel
                        filterOnGroup: "games" // Фильтруем строго по созданной группе игр
                        delegate: visualModel.delegate
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        loadingIndicator.visible = true
        errorText.visible = false

        var theme = JWBStorePlugin.getRandomTheme()
        store_home.bgFolder = theme.bg
        store_home.cardColor = theme.card
        store_home.bannerBgColor = theme.banner
        store_home.accentColor = theme.accent

        storeApi.fetchOrganizationRepos()
    }
}
