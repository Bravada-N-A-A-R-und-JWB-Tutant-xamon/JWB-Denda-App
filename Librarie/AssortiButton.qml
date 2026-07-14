import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Shapes 1.12 // Для отрисовки геометрии

Button {
    id: control

    // --- НАШИ КАСТОМНЫЕ ПРОПЕРТИ ---
    property color accentColor: "#21be2b" // Дефолтный фон кнопки (твой зелёный)
    property color textColor: "#ffffff"   // Дефолтный цвет текста

    // Ширина бокового "носика"
    property real handleWidth: height * 0.4

    implicitWidth: Math.max(implicitBackgroundWidth + leftPadding + rightPadding,
                            contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: 40 // Фиксированная высота под MD3 капсулу

    leftPadding: handleWidth + 12
    rightPadding: handleWidth + 12

    // --- ЛОГИКА И СОДЕРЖИМОЕ (КОНТЕНТ) ---
    contentItem: Text {
        text: control.text
        font: control.font
        color: control.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    // --- ЗАДНИЙ ФОН (НАША КОНФЕТА "АССОРТИ") ---
        background: Shape {
            anchors.fill: parent
            antialiasing: true

            ShapePath {
                id: shapePath
                fillColor: control.down ? Qt.darker(control.accentColor, 1.2) : control.accentColor
                strokeColor: control.enabled ? "#353637" : "transparent"
                strokeWidth: 2

                // Стартуем с верхнего левого угла прямоугольной части
                startX: control.handleWidth
                startY: 0

                // Погнали по точкам через обычные PathLine
                PathLine { x: parent.width - control.handleWidth; y: 0 }                 // До верхнего правого угла
                PathLine { x: parent.width; y: parent.height / 2 }                       // Правый носик
                PathLine { x: parent.width - control.handleWidth; y: parent.height }     // До нижнего правого угла
                PathLine { x: control.handleWidth; y: parent.height }                    // До нижнего левого угла
                PathLine { x: 0; y: parent.height / 2 }                                  // Левый носик
                PathLine { x: control.handleWidth; y: 0 }                                // Замыкаем в старт
            }
        }
}
