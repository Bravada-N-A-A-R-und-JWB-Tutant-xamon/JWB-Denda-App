import QtQuick 2.12
import QtQml 2.0
import QtQuick.Controls 2.12
import QtQuick.LocalStorage 2.12
import QtQuick.Layouts 1.12

Page {
    id: installed_apps_page
    background: Rectangle {
        color: "#151515"
    }
    header: ToolBar {
        height: 50
        background: Rectangle {
            color: "#111"
            border.color: "#222"
            border.width: 1
        }
        RowLayout {
            spacing: 1
            anchors.fill: parent
            anchors.rightMargin: 10
            anchors.leftMargin: 10
            anchors.topMargin: 2
            anchors.bottomMargin: 2

            ToolButton {
                text: "<"
                onClicked: page_manager.pop();
                contentItem: Text {
                    text: parent.text
                    color: "#00ffcc"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 20
                }
                background: Rectangle {
                    color: "transparent"
                }
            }
            Label {
                text: qsTr("Installed apps")
                transformOrigin: Item.Center
                color: "#00ffcc"
                font.pixelSize: 20
                font.bold: true
                elide: Label.ElideRight
                verticalAlignment: Qt.AlignVCenter
            }
        }
    }

    ListModel {
        id: installed_apps_model
    }

    Flickable {
        anchors.fill: parent
        anchors.topMargin: 15
    }

}
