import QtQuick 2.12
import QtQuick.Controls 2.12

Button {
    id: control

    // --- НАШИ КАСТОМНЫЕ ПРОПЕРТИ ---
    property color accentColor: "#21be2b" // Дефолтный фон кнопки (твой фирменный зелёный)
    property color textColor: "#ffffff"   // Дефолтный цвет текста

    // Пути к иконкам (пустые по умолчанию)
    property url leftIconSource: ""
    property url rightIconSource: ""

    // Инверсия для правой иконки (если true, перевернет по горизонтали)
    property bool invertRightIcon: false

    implicitWidth: Math.max(implicitBackgroundWidth + leftPadding + rightPadding,
                            contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: 40 // Фиксированная высота под MD3 капсулу

    // Отступы внутри кнопки под стиль MD3 (чуть больше по бокам)
    leftPadding: 24
    rightPadding: 24
    topPadding: 0
    bottomPadding: 0

    // --- ЛОГИКА И СОДЕРЖИМОЕ (КОНТЕНТ) ---
    contentItem: Row {
        spacing: 8 // Зазор между иконками и текстом по гайдлайнам MD3
        // Центрируем весь Row внутри кнопки
        anchors.centerIn: parent

        // Левая иконка
        Image {
            id: leftIcon
            source: control.leftIconSource
            // Показывать только если передан путь
            visible: control.leftIconSource != ""
            width: visible ? 18 : 0
            height: 18
            sourceSize: Qt.size(18, 18)
            anchors.verticalCenter: parent.verticalCenter
            smooth: true
        }

        // Сам текст кнопки
        Text {
            text: control.text
            font: control.font
            color: control.textColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        // Правая иконка
        Image {
            id: rightIcon
            source: control.rightIconSource
            visible: control.rightIconSource != ""
            width: visible ? 18 : 0
            height: 18
            sourceSize: Qt.size(18, 18)
            anchors.verticalCenter: parent.verticalCenter
            smooth: true

            // Магия инверсии: если флаг true, зеркалим иконку по горизонтали
            transform: Scale {
                origin.x: 9 // половина ширины (18 / 2)
                origin.y: 9
                xScale: control.invertRightIcon ? -1 : 1
            }
        }
    }

    // --- ЗАДНИЙ ФОН (КАПСУЛА MD3) ---
    background: Rectangle {
        // Капсула: радиус равен половине высоты кнопки
        radius: control.height / 2

        // Меняем прозрачность или цвет при нажатии/наведении (эффект MD3)
        color: control.down ? Qt.darker(control.accentColor, 1.2) : control.accentColor

        // Легкая обводка, если кнопка отключена
        border.color: control.enabled ? "transparent" : "#353637"

        // Плавное изменение цвета при нажатии
        Behavior on color {
            ColorAnimation { duration: 100 }
        }
    }
}
