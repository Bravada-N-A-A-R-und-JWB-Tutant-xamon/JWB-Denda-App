import QtQuick 2.12
import QtQuick.Controls.Material 2.12 as MaterialYou32QQC2
import QtQml 2.12
import QtQuick.LocalStorage 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Page {
    id: parametres_page
    background: /*Image {
        id: wallpaper
        fillMode: Image.PreserveAspectCrop
        source: "Genei-Shadow.svg"
        //anchors.fill: parent
    }*/

    Rectangle {
        color: "black"
        anchors.fill: parent
    }

    header: ToolBar {
        height: 50
        background: Rectangle {
            color: "#111111"
            border.color: "#222222"
            border.width: 1
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10

            ToolButton {
                text: "<" // Кнопка поиска без Lomiri Components
                onClicked: {
                    console.log("parametres are closed")
                    page_manager.pop();
                }
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 20
                    color: "#00FFCC"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { color: "transparent" }
            }

            Label {
                text: qsTr("JWB-Denda Parametres")
                color: "#00FFCC"
                font.pixelSize: 20
                font.bold: true
                elide: Label.ElideRight
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }

        }
    }
    ScrollView {
	id: scrollview
	anchors.fill: parent
	contentWidth: parent.width
	clip: true

	ColumnLayout {
	    anchors.centerIn: parent
	    anchors.margins: 10
	    spacing: 10

	}
    }
}
