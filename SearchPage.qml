import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Page {
    id: search_app_page

    // Ссылки на глобальные объекты из MainStore.qml
    property var appsModelRef: null
    property var storeApiRef: null // Ссылка на storeApi, чтобы не было ReferenceError

    readonly property color bgColor: "#0A0A0A"
    readonly property color cardColor: "#141414"
    readonly property color borderColor: "#262626"
    readonly property color accentColor: "#00FFCC"
    readonly property color textColor: "#FFFFFF"
    readonly property color textMuted: "#888888"

    background: Rectangle {
        color: search_app_page.bgColor
    }

    // Локальная модель для вывода результатов на экран
    ListModel {
        id: searchResultsModel
    }

    // Функция фильтрации
    function filterApplications(query) {
        searchResultsModel.clear()
        if (appsModelRef === null) return;

        var cleanQuery = query.trim().toLowerCase()

        for (var i = 0; i < appsModelRef.count; i++) {
            var item = appsModelRef.get(i)

            var appName = item.appName ? item.appName.toLowerCase() : ""
            var appAuthor = item.appAuthor ? item.appAuthor.toLowerCase() : ""
            var repoName = item.repoName ? item.repoName.toLowerCase() : ""

            if (cleanQuery === "" || appName.indexOf(cleanQuery) !== -1 || appAuthor.indexOf(cleanQuery) !== -1 || repoName.indexOf(cleanQuery) !== -1) {
                searchResultsModel.append({
                    "appName": item.appName ? item.appName : "",
                    "appAuthor": item.appAuthor ? item.appAuthor : "",
                    "repoName": item.repoName ? item.repoName : "",
                    "appVersion": item.appVersion ? item.appVersion : "1.0.0",
                    "appIcon": item.appIcon ? item.appIcon : "",
                    "appSize": item.appSize ? item.appSize : "0.0 MB",
                    "appDescription": item.appDescription ? item.appDescription : "",
                    "appUrl": item.appUrl ? item.appUrl : "",
                    "appSplashColor": item.appSplashColor ? item.appSplashColor : "#141414",
                    "appArchitectures": item.appArchitectures ? item.appArchitectures : "all",
                    "appCategory": item.appCategory ? item.appCategory : "apps"
                })
            }
        }
    }

    header: ToolBar {
        height: 50
        background: Rectangle {
            color: search_app_page.cardColor
            border.color: search_app_page.borderColor
            border.width: 1
        }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8

            ToolButton {
                text: "‹"
                Layout.fillHeight: true
                implicitWidth: 30
                implicitHeight: 30
                onClicked: page_manager.pop();
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 35
                    color: search_app_page.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { color: "transparent" }
            }

            TextField {
                id: search_field
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                placeholderText: qsTr("Search applications...")
                color: search_app_page.textColor
                leftPadding: 15

                background: Rectangle {
                    color: search_app_page.bgColor
                    border.width: 1
                    border.color: search_app_page.borderColor
                    radius: height / 3
                }

                onTextChanged: {
                    filterApplications(text)
                }
            }
        }
    }

    Item {
        anchors.fill: parent

        Text {
            text: qsTr("Sorry, but this app isn't found")
            color: search_app_page.textMuted
            font.pixelSize: 18
            anchors.centerIn: parent
            visible: search_field.text.trim().length > 0 && searchResultsModel.count === 0
        }

        ListView {
            id: searchListView
            anchors.fill: parent
            anchors.margins: 10
            model: searchResultsModel
            spacing: 10
            clip: true
            visible: searchResultsModel.count > 0

            delegate: Rectangle {
                width: searchListView.width
                height: 85
                color: search_app_page.cardColor
                border.color: search_app_page.borderColor
                border.width: 1
                radius: 10

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    spacing: 15

                    // 1. ЛЕВЫЙ КРАЙ: Иконка приложения
                    Rectangle {
                        width: 55; height: 55
                        color: "#161616"
                        radius: 10
                        border.color: search_app_page.borderColor
                        Layout.alignment: Qt.AlignVCenter

                        Image {
                            source: (model.appIcon && model.appIcon !== "") ? model.appIcon : "qrc:/Assets/JWB-Denda Logo (none).svg"
                            anchors.fill: parent
                            anchors.margins: 6
                            fillMode: Image.PreserveAspectFit
                        }
                    }

                    // 2. ЦЕНТР: Текстовый блок (Название, Автор, Описание)
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 2

                        RowLayout {
                            spacing: 8
                            Layout.fillWidth: true

                            Label {
                                text: model.appName ? model.appName : ""
                                color: search_app_page.textColor
                                font.bold: true
                                font.pixelSize: 15
                                elide: Text.ElideRight
                            }

                            Label {
                                text: model.appVersion ? "v" + model.appVersion : "v1.0.0"
                                color: search_app_page.textMuted
                                font.pixelSize: 11
                            }
                        }

                        Label {
                            text: model.appAuthor ? qsTr("by ") + model.appAuthor : ""
                            color: search_app_page.accentColor
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }

                        Label {
                            text: model.appDescription ? model.appDescription : qsTr("No description available")
                            color: search_app_page.textMuted
                            font.pixelSize: 12
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                    }

                    // 3. ПРАВЫЙ КРАЙ: Кнопка-стрелка ›
                    Label {
                        text: "›"
                        color: search_app_page.accentColor
                        font.pixelSize: 30
                        font.bold: true
                        Layout.alignment: Qt.AlignVCenter
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        // Используем переданную ссылку storeApiRef для безопасного взятия токена
                        var currentToken = (search_app_page.storeApiRef && search_app_page.storeApiRef.token) ? search_app_page.storeApiRef.token : ""

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
                            "token": currentToken
                        })
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        filterApplications("")
    }
}
