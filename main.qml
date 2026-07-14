import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtMultimedia 5.12
import QtQml 2.12
import Qt.labs.settings 1.0
import JWBStore.API.io 1.0

Window {
    id: mainFocus
    visible: true
    width: 600
    height: 900
    title: qsTr("JWB-Store")

    property bool fullscreen: false
    property bool transparentWindow: false
    property bool play_the_sounds: false

    objectName: "MainFocus"
    color: transparentWindow ? "transparent" : "#0A0A0A"
    visibility: fullscreen ? Window.FullScreen : Window.Windowed

    /*Settings {
        category: "Interface"
        property alias fullscreen: mainFocus.fullscreen
        property alias transparentWindow: mainFocus.transparentWindow
        property alias play_the_sounds: mainFocus.play_the_sounds
    }*/

    Audio {
        id: clickSound
        source: "qrc:/sounds/click.wav"
        volume: 0.5
    }

    function playUiSound() {
        if (mainFocus.play_the_sounds && clickSound.status === Audio.Loaded) {
            clickSound.play()
        }
    }

    function openPage(pageUrl, properties) {
        playUiSound()
        if (properties === undefined) properties = {}
        page_manager.push(pageUrl, properties)
    }

    function goBack() {
        playUiSound()
        if (page_manager.depth > 1) {
            page_manager.pop()
            return true
        }
        return false
    }

    // Главный графический контейнер, который теперь корректно ловит нажатия кнопок
    Item {
        anchors.fill: parent
        focus: true

        StackView {
            id: page_manager
            anchors.fill: parent
            initialItem: "MainStore.qml"

            pushEnter: Transition {
                PropertyAnimation { property: "x"; from: page_manager.width; to: 0; duration: 200; easing.type: Easing.OutQuad }
            }
            pushExit: Transition {
                PropertyAnimation { property: "x"; to: -page_manager.width; duration: 200; easing.type: Easing.OutQuad }
            }
            popEnter: Transition {
                PropertyAnimation { property: "x"; from: -page_manager.width; to: 0; duration: 200; easing.type: Easing.OutQuad }
            }
            popExit: Transition {
                PropertyAnimation { property: "x"; to: page_manager.width; duration: 200; easing.type: Easing.OutQuad }
            }
        }

        Keys.onReleased: {
            if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape) {
                if (goBack()) {
                    event.accepted = true
                }
            }
        }
    }
}
