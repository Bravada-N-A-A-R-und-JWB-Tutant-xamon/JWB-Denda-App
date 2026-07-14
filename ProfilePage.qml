import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14

Page {
    id: profilePage

    // === ТЕМНЫЙ ДИЗАЙН TELEGRAM / JWB ===
    property color bgFolder: "#0A0A0A"
    property color cardColor: "#141414"
    property color borderColor: "#262626"
    property color accentColor: "#00FFCC"
    property color textColor: "#FFFFFF"
    property color subTextColor: "#aaaaaa"

    // Переключалка режимов: false — просмотр (Telegram), true — редактирование (рега)
    property bool isEditMode: false
    property string currentOsType: "unknown"

    background: Rectangle { color: profilePage.bgFolder }

    // === ШАПКА ===
    header: ToolBar {
	height: 50
	background: Rectangle {
	    color: profilePage.cardColor
	    border.color: profilePage.borderColor
	    border.width: 1
	}

	RowLayout {
	    anchors.fill: parent
	    anchors.leftMargin: 15; anchors.rightMargin: 15

	    ToolButton {
		text: "⬅"
		contentItem: Text {
		    text: parent.text
		    font.pointSize: 12
		    color: "#fff"
		    horizontalAlignment: Text.AlignHCenter
		    verticalAlignment: Text.AlignVCenter
		}
		onClicked: {
		    if (profilePage.isEditMode) {
			profilePage.isEditMode = false // Если редактировали, отменяем и возвращаемся в профиль
		    } else {
			page_manager.pop() // Иначе выходим на главную
		    }
		}
		background: Item {}
	    }

	    Label {
		text: profilePage.isEditMode ? qsTr("Edit Profile") : qsTr("Info")
		color: profilePage.textColor
		font.pixelSize: 16
		font.bold: true
		Layout.fillWidth: true
	    }

	    // Кнопка переключения в режим редактирования в стиле TG
	    ToolButton {
		visible: !profilePage.isEditMode
		text: "✏️"
		contentItem: Text {
		    text: parent.text
		    font.pointSize: 12
		    horizontalAlignment: Text.AlignHCenter
		    verticalAlignment: Text.AlignVCenter
		}
		onClicked: profilePage.isEditMode = true
		background: Item {}
	    }
	}
    }

    // === ОСНОВНОЙ КОНТЕНТ ===
    ScrollView {
	anchors.fill: parent
	contentWidth: parent.width
	clip: true

	// ----------------------------------------------------
	// 1️⃣ РЕЖИМ ПРОСМОТРА: ТЕЛЕГРАМ-ДИЗАЙН
	// ----------------------------------------------------
	ColumnLayout {
	    id: viewLayout
	    visible: !profilePage.isEditMode
	    width: parent.width
	    spacing: 0

	    // Блок Аватарки и Имени сверху (как в TG при открытии профиля)
	    Rectangle {
		Layout.fillWidth: true
		height: 180
		color: profilePage.cardColor
		border.color: profilePage.borderColor
		border.width: 1

		ColumnLayout {
		    anchors.centerIn: parent
		    spacing: 10

		    // Круглая аватарка с первой буквой ника
		    Rectangle {
			width: 70; height: 70
			radius: 35
			color: profilePage.accentColor
			Layout.alignment: Qt.AlignHCenter

			Label {
			    id: avatarLetter
			    text: nicknameField.text !== "" ? nicknameField.text.charAt(0).toUpperCase() : "J"
			    color: "#000"
			    font.pixelSize: 28
			    font.bold: true
			    anchors.centerIn: parent
			}
		    }

		    // Большое имя пользователя
		    Label {
			id: viewNickname
			text: nicknameField.text !== "" ? nicknameField.text : "Anonymous User"
			color: profilePage.textColor
			font.family: "Aller"
			font.pixelSize: 20
			font.bold: true
			Layout.alignment: Qt.AlignHCenter
		    }

		    Label {
			text: qsTr("Online")
			color: profilePage.accentColor
			font.pixelSize: 12
			Layout.alignment: Qt.AlignHCenter
		    }
		}
	    }

	    // Разделитель «Инфо»
	    Rectangle {
		Layout.fillWidth: true
		height: 30
		color: profilePage.bgFolder
		Label {
		    text: qsTr("Account Info")
		    color: profilePage.accentColor
		    font.pixelSize: 11
		    font.bold: true
		    anchors.left: parent.left
		    anchors.leftMargin: 20
		    anchors.verticalCenter: parent.verticalCenter
		}
	    }

	    // Список полей в стиле Telegram-элементов
	    Rectangle {
		Layout.fillWidth: true
		height: itemsColumn.implicitHeight + 20
		color: profilePage.cardColor
		border.color: profilePage.borderColor
		border.width: 1

		ColumnLayout {
		    id: itemsColumn
		    anchors.fill: parent
		    anchors.margins: 15
		    spacing: 15

		    // СТРОКА: НИКНЕЙМ / ЮЗЕРНЕЙМ
		    ColumnLayout {
			spacing: 2
			Layout.fillWidth: true
			Label { id: lblNick; text: nicknameField.text !== "" ? nicknameField.text : "@not_set"; color: profilePage.textColor; font.pixelSize: 14 }
			Label { text: qsTr("Username"); color: profilePage.subTextColor; font.pixelSize: 11 }
		    }

		    Rectangle { Layout.fillWidth: true; height: 1; color: profilePage.borderColor }

		    // СТРОКА: ОПЕРАЦИОННАЯ СИСТЕМА + ИКОНКА
		    RowLayout {
			Layout.fillWidth: true
			spacing: 15

			Image {
			    id: viewOsIcon
			    width: 24; height: 24
			    Layout.preferredWidth: 24; Layout.preferredHeight: 24
			    source: "/Assets/Operation Systems Logotypes/unknown.svg"
			    fillMode: Image.PreserveAspectFit
			}

			ColumnLayout {
			    spacing: 2
			    Layout.fillWidth: true
			    Label { id: viewOsName; text: osNameText.text; color: profilePage.textColor; font.pixelSize: 14 }
			    Label { text: qsTr("Operating System"); color: profilePage.subTextColor; font.pixelSize: 11 }
			}
		    }

		    Rectangle { Layout.fillWidth: true; height: 1; color: profilePage.borderColor }

		    // СТРОКА: ХАРДВЕРНЫЙ ДЕВАЙС
		    ColumnLayout {
			spacing: 2
			Layout.fillWidth: true
			Label { id: viewDevice; text: deviceField.text !== "" ? deviceField.text : "Not defined"; color: profilePage.textColor; font.pixelSize: 14 }
			Label { text: qsTr("Hardware Setup / Device"); color: profilePage.subTextColor; font.pixelSize: 11 }
		    }
		}
	    }
	}

	// ----------------------------------------------------
	// 2: РЕЖИМ РЕДАКТИРОВАНИЯ: СТРАНИЦА РЕГИСТРАЦИИ (НЕ УДАЛЕНА!)
	// ----------------------------------------------------
	ColumnLayout {
	    id: editLayout
	    visible: profilePage.isEditMode
	    width: parent.width - 40
	    anchors.horizontalCenter: parent.horizontalCenter
	    spacing: 20
	    Layout.topMargin: 20

	    Label {
		text: qsTr("MODIFY ECCOSYSTEM PROFILE")
		font.family: "Aller"
		font.pixelSize: 18
		font.bold: true
		color: profilePage.accentColor
		Layout.alignment: Qt.AlignHCenter
	    }

	    // INPUT: NICKNAME
	    ColumnLayout {
		Layout.fillWidth: true; spacing: 6
		Label { text: qsTr("Your Username / Nickname:"); font.family: "Aller"; color: profilePage.subTextColor; font.pixelSize: 13 }
		TextField {
		    id: nicknameField
		    Layout.fillWidth: true
		    placeholderText: qsTr("Enter nickname...")
		    color: profilePage.textColor
		    font.family: "Aller"
		    background: Rectangle {
			color: profilePage.cardColor
			border.color: nicknameField.activeFocus ? profilePage.accentColor : profilePage.borderColor
			border.width: 1; radius: 4
		    }
		}
	    }

	    // BLOCK: OS DETECTED
	    ColumnLayout {
		Layout.fillWidth: true; spacing: 6
		Label { text: qsTr("Detected OS & Desktop Environment:"); font.family: "Aller"; color: profilePage.subTextColor; font.pixelSize: 13 }
		Rectangle {
		    Layout.fillWidth: true; height: 65
		    color: profilePage.cardColor; border.color: profilePage.borderColor; border.width: 1; radius: 4
		    RowLayout {
			anchors.fill: parent; anchors.margins: 12; spacing: 15
			Image {
			    id: osIcon
			    width: 32; height: 32
			    Layout.preferredWidth: 32; Layout.preferredHeight: 32
			    source: "Assets/Operation Systems Logotypes/unknown.svg"
			    fillMode: Image.PreserveAspectFit
			}
			ColumnLayout {
			    spacing: 2; Layout.fillWidth: true
			    Label { id: osNameText; text: qsTr("Not Detected Yet"); color: profilePage.textColor; font.family: "Aller"; font.bold: true; font.pixelSize: 15 }
			    Label { id: osTypeText; text: qsTr("Run auto-detect to find your system specs"); color: "#555555"; font.family: "Aller"; font.pixelSize: 11 }
			}
		    }
		}
	    }

	    // INPUT: HARDWARE
	    ColumnLayout {
		Layout.fillWidth: true; spacing: 6
		Label { text: qsTr("Device Model / Hardware Setup:"); font.family: "Aller"; color: profilePage.subTextColor; font.pixelSize: 13 }
		TextField {
		    id: deviceField
		    Layout.fillWidth: true
		    placeholderText: qsTr("e.g., Redmi Note 9, Pinephone, Custom PC...")
		    color: profilePage.textColor
		    font.family: "Aller"
		    background: Rectangle {
			color: profilePage.cardColor
			border.color: deviceField.activeFocus ? profilePage.accentColor : profilePage.borderColor
			border.width: 1; radius: 4
		    }
		}
	    }

	    // BUTTON: DETECT
	    Button {
		id: detectButton
		Layout.fillWidth: true; height: 45

		contentItem: Text {
		    text: qsTr("FORCE AUTO-DETECT SYSTEM 📱")
		    font.family: "Aller"; font.bold: true
		    color: detectButton.hovered ? "#000000" : profilePage.accentColor
		    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
		}
		background: Rectangle {
		    color: detectButton.down ? "#00AA88" : (detectButton.hovered ? profilePage.accentColor : "transparent")
		    border.color: profilePage.accentColor; border.width: 1; radius: 4
		}
		onClicked: {
		    var sysInfo = jwbReviewBackend.detectCurrentSystem();
		    osNameText.text = sysInfo.name;
		    osTypeText.text = qsTr("Env Code: ") + sysInfo.type;
		    profilePage.currentOsType = sysInfo.type;
		    osIcon.source = "Assets/Operation Systems Logotypes/" + sysInfo.type + ".svg";

		    if (deviceField.text === "") {
			deviceField.text = sysInfo.name;
		    }
		}
	    }

	    // BUTTON: SAVE
	    Button {
		id: saveButton
		Layout.fillWidth: true; height: 45; Layout.topMargin: 10

		contentItem: Text {
		    text: qsTr("UPDATE DATABASE PROFILE 👑")
		    font.family: "Aller"; font.bold: true; color: "#FFFFFF"
		    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
		}
		background: Rectangle {
		    color: saveButton.down ? "#115511" : (saveButton.hovered ? "#22aa22" : "#1e7e1e")
		    radius: 4
		}
		onClicked: {
		    var user = nicknameField.text !== "" ? nicknameField.text : "Anonymous User";
		    var success = jwbReviewBackend.saveProfile(user, osNameText.text, profilePage.currentOsType, deviceField.text);

		    if (success) {
			// Синхронизируем иконку для режима просмотра
			viewOsIcon.source = "qrc:/Assets/Operation Systems Logotypes/" + profilePage.currentOsType + ".svg";
			saveSuccessToast.show(user);
			profilePage.isEditMode = false // Закрываем регу, возвращаемся в красивый профиль!
		    }
		}
	    }
	}
    }

    // === НЕОНОВЫЙ ТОУСТ-БАННЕР ===
    Popup {
	id: saveSuccessToast
	x: (parent.width - width) / 2; y: parent.height - height - 40
	width: parent.width - 80; height: 55
	modal: false; focus: false
	closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

	enter: Transition {
	    NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 250 }
	    NumberAnimation { property: "scale"; from: 0.92; to: 1.0; duration: 250; easing.type: Easing.OutCubic }
	}
	exit: Transition { NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 200 } }

	background: Rectangle {
	    color: "#141414"; border.color: profilePage.accentColor; border.width: 1; radius: 6; clip: true
	    // Rectangle { width: parent.width; height: 3; color: profilePage.accentColor; anchors.bottom: parent.bottom; radius: 2}
	}
	contentItem: RowLayout {
	    anchors.fill: parent; anchors.leftMargin: 15; spacing: 12
	    Text { text: "👑"; font.pixelSize: 18; Layout.alignment: Qt.AlignVCenter }
	    ColumnLayout {
		spacing: 2; Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
		Label { id: toastMessage; text: ""; color: profilePage.textColor; font.family: "Aller"; font.bold: true; font.pixelSize: 12 }
		Label { text: qsTr("Profile state synchronized and verified locally."); color: "#666666"; font.family: "Aller"; font.pixelSize: 10 }
	    }
	}
	Timer { id: toastTimer; interval: 3500; onTriggered: { saveSuccessToast.close() } }
	function show(username) {
	    toastMessage.text = qsTr("Identity Confirmed: ") + username;
	    toastTimer.restart();
	    saveSuccessToast.open();
	}
    }

    // === ЗАГРУЗКА ПРИ СТАРТЕ ===
    Component.onCompleted: {
	var savedData = jwbReviewBackend.loadProfile();

	nicknameField.text = savedData.nickname;
	osNameText.text = savedData.osName;
	osTypeText.text = qsTr("Environment Code: ") + savedData.osType;
	profilePage.currentOsType = savedData.osType;
	deviceField.text = savedData.device;

	// Ставим иконки в оба режима
	osIcon.source = "Assets/Operation Systems Logotypes/" + savedData.osType + ".svg";
	viewOsIcon.source = "Assets/Operation Systems Logotypes/" + savedData.osType + ".svg";
    }
}
