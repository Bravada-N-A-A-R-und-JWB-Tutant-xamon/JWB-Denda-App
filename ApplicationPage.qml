import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import JWBStore.API.io 1.0
import JWBDenda.Installer.io 1.0
import JWBStore.Backend 1.0

Page {
    id: app_page
    title: app_page.appName + qsTr("at JWB-Denda")
    objectName: "Application Details Page"

    property string appName: ""
    property string repoName: ""
    property string appVersion: ""
    property string appAuthor: ""
    property string appSize: "0.0 MB"
    property string appDescription: ""
    property string appIcon: ""
    property string appUrl: ""
    property string appSplashColor: "#141414"
    property string appArchitectures: "none"
    property string appCategory: "Games"
    property string token: "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"

    property string currentUsername: ""
    property string currentDevice: ""
    property string currentOsType: ""

    property bool hasMyReview: false
    property int myReviewIndex: -1

    property int countSuper: 0
    property int countNorm: 0
    property int countNeutral: 0
    property int countTrash: 0
    property int countParasha: 0

    ListModel { id: screenshotsModel }
    ListModel { id: changelogModel }
    ListModel { id: reviewsModel }

    Component.onCompleted: {
	var profile = reviewSystem.loadProfile()
	app_page.currentUsername = profile.nickname !== "" ? profile.nickname : qsTr("Guest")
	app_page.currentDevice = profile.device !== "" ? profile.device : "Mobile Device"
	app_page.currentOsType = profile.osType !== "" ? profile.osType : "ubuntu_touch"
    }

    ReviewItemAPI {
	id: reviewsApiSystem
	onReviewSentSuccessfully: {
	    console.log(">>> [JWB-Network] Отзыв успешно улетел на сервер!")
	    reviewSystem.fetchReviews("Bravada-N-A-A-R-und-JWB-Tutant-xamon", app_page.repoName)
	}
	onErrorOccurred: (msg) => console.log(">>> [JWB-Network Error]: " + msg)
    }

    JWBReviewBackend {
	id: reviewSystem
	onReviewsLoaded: {
	    reviewsModel.clear()
	    app_page.hasMyReview = false
	    app_page.myReviewIndex = -1

	    for (var i = 0; i < reviewSystem.reviewsList.length; i++) {
		var item = reviewSystem.reviewsList[i]
		reviewsModel.append(item)
		if (item.author === app_page.currentUsername) {
		    app_page.hasMyReview = true
		    app_page.myReviewIndex = i
		}
	    }

	    if (app_page.hasMyReview && app_page.myReviewIndex !== -1) {
		var myReview = reviewsModel.get(app_page.myReviewIndex)
		reviewTextField.text = myReview.body
		emojiSelector.select(myReview.ratingStr, myReview.emoji)
	    } else {
		reviewTextField.text = ""
		emojiSelector.select("THUMBS_UP", "💚")
	    }
	    app_page.calculateStats()
	}
	onErrorOccurred: (msg) => console.log(">>> [JWB-Backend Error]: " + msg)
    }

    function calculateStats() {
	countSuper = 0; countNorm = 0; countNeutral = 0; countTrash = 0; countParasha = 0
	for (var i = 0; i < reviewsModel.count; i++) {
	    var rStr = reviewsModel.get(i).ratingStr
	    if (rStr === "THUMBS_UP") countSuper++
	    else if (rStr === "HAPPY") countNorm++
	    else if (rStr === "NEUTRAL") countNeutral++
	    else if (rStr === "BUGGY") countTrash++
	    else if (rStr === "THUMBS_DOWN") countParasha++
	}
    }

    background: Rectangle { color: app_page.appSplashColor }
    Image {
	id: wallpaper
	source: "Genei-Shadow.svg"
	anchors.fill: parent
	fillMode: Image.PreserveAspectCrop
    }

    GitHubApi {
	id: lazyChecker
	token: app_page.token
	onClickReleaseChecked: (available, downloadUrl, sizeStr, archs) => {
	    if (!available) {
		installButtonText.text = qsTr("Not available / not released yet")
		installButton.color = "#cc3333"
		installButton.enabled = false
		app_page.appSize = "0.0 MB"
		app_page.appArchitectures = "none"
	    } else {
		app_page.appUrl = downloadUrl
		app_page.appSize = sizeStr
		app_page.appArchitectures = archs

		var localVersion = appInstaller.getInstalledVersion(app_page.repoName)
		if (localVersion === "0.0.0") {
		    installButtonText.text = qsTr("INSTALL")
		    installButton.color = "#00FFCC"
		} else if (localVersion !== app_page.appVersion) {
		    installButtonText.text = qsTr("UPDATE AVAILABLE")
		    installButton.color = "#FFA500"
		} else {
		    installButtonText.text = qsTr("OPEN")
		    installButton.color = "#00cc66"
		}
		installButton.enabled = true
	    }
	}
    }

    onRepoNameChanged: {
	if (app_page.repoName !== "" && app_page.repoName !== undefined) {
	    installButtonText.text = qsTr("Checking Release...")
	    installButton.color = "#222222"
	    installButton.enabled = false

	    lazyChecker.checkClickRelease(app_page.repoName)
	    reviewSystem.fetchReviews("Bravada-N-A-A-R-und-JWB-Tutant-xamon", app_page.repoName)

	    screenshotsModel.clear()
	    screenshotsModel.append({ "src": "https://raw.githubusercontent.com/Bravada-N-A-A-R-und-JWB-Tutant-xamon/" + app_page.repoName + "/main/assets/screen1.png" })
	    screenshotsModel.append({ "src": "https://raw.githubusercontent.com/Bravada-N-A-A-R-und-JWB-Tutant-xamon/" + app_page.repoName + "/main/assets/screen2.png" })
	    screenshotsModel.append({ "src": "https://raw.githubusercontent.com/Bravada-N-A-A-R-und-JWB-Tutant-xamon/" + app_page.repoName + "/main/assets/screen3.png" })

	    changelogModel.clear()
	    changelogModel.append({ "version": "v" + app_page.appVersion, "date": "19.06.2026", "note": "• Фикс утечки памяти в J2ME рендере\n• Оптимизация биндингов QML\n• Изменение механики падения капель" })
	    changelogModel.append({ "version": "v1.0.0", "date": "10.05.2026", "note": "• Первый релиз организации Xarmbrassadora-Bravada\n• Базовая поддержка Lomiri" })
	}
    }

    JWBStoreInstaller {
	id: appInstaller
	property string currentStatus: "idle"
	onStatusChanged: (status) => {
	    appInstaller.currentStatus = status
	    if (status === "installing") {
		installButtonText.text = qsTr("Installing...")
		installButton.enabled = false
		installButton.color = "#222222"
	    } else if (status === "installed") {
		installButtonText.text = qsTr("OPEN")
		installButton.enabled = true
		installButton.color = "#00cc66"
	    } else if (status === "error") {
		installButtonText.text = qsTr("RETRY INSTALL")
		installButton.enabled = true
		installButton.color = "#cc3333"
		manualInstallPopup.open() // Саня, если ошибка авто-инсталла — сразу выкатываем инфо-попап!
	    }
	}
	onDownloadProgressChanged: {
	    if (appInstaller.currentStatus === "downloading") {
		if (appInstaller.downloadProgress >= 0) {
		    installButtonText.text = qsTr("Downloading... ") + Math.floor(appInstaller.downloadProgress * 100) + "%"
		} else {
		    var mbStr = (-appInstaller.downloadProgress).toFixed(2)
		    installButtonText.text = qsTr("Downloading... ") + mbStr + " MB"
		}
	    }
	}
    }

    header: ToolBar {
	height: 50
	background: Rectangle { color: "#141414"; opacity: 0.3 }
	RowLayout {
	    anchors.fill: parent
	    ToolButton {
		text: qsTr("‹")
		background: Rectangle { color: "transparent" }
		contentItem: Text { text: parent.text; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; color: "#fff"; font.pixelSize: 24 }
		onClicked: page_manager.pop();
	    }
	    Label { text: app_page.appName; color: "#FFFFFF"; font.bold: true; font.pixelSize: 16; Layout.fillWidth: true }
	}
    }


    // ==========================================
    // ГЛАВНЫЙ СТРУКТУРНЫЙ ЛЕЙАУТ (ФИКС КЛАВИАТУРЫ)
    // ==========================================
    ColumnLayout {
	anchors.fill: parent
	spacing: 0
	anchors.topMargin: 10

	// Верхняя часть: весь контент, который можно скроллить
	ScrollView {
	    id: mainScrollView
	    Layout.fillWidth: true
	    Layout.fillHeight: true
	    clip: true
	    contentWidth: parent.width

	    ColumnLayout {
		width: parent.width - 15
		spacing: 18
		anchors.margins: 15
		anchors.horizontalCenter: parent.horizontalCenter

		// Карта приложения (Иконка, Имя, Автор)
		Rectangle {
		    Layout.fillWidth: true; height: 120; color: "#19000000"; radius: 8
		    RowLayout {
			anchors.fill: parent; anchors.margins: 15; spacing: 15
			Image { source: app_page.appIcon ? app_page.appIcon : "qrc:/Assets/JWB-Denda Logo (none).svg"; Layout.preferredWidth: 70; Layout.preferredHeight: 70; fillMode: Image.PreserveAspectFit; cache: true }
			ColumnLayout {
			    spacing: 2; Layout.fillWidth: true
			    Label { text: app_page.appName; color: "#FFF"; font.bold: true; font.pixelSize: 20; elide: Text.ElideRight; Layout.fillWidth: true }
			    Label { text: app_page.appAuthor; color: "#EEE"; font.pixelSize: 13; elide: Text.ElideRight; Layout.fillWidth: true }
			    Label { text: app_page.appCategory + " • " + app_page.appSize + " • v" + app_page.appVersion; color: "#CCC"; font.pixelSize: 12 }
			}
		    }
		}

		// Скриншоты
		ColumnLayout {
		    Layout.fillWidth: true; spacing: 6; visible: screenshotsModel.count > 0
		    Label { text: qsTr("Screenshots"); color: "#FFF"; font.bold: true; font.pixelSize: 14 }
		    ListView {
			id: screenshotsListView; Layout.fillWidth: true; height: 200; orientation: ListView.Horizontal; spacing: 10; model: screenshotsModel; clip: true
			delegate: Rectangle {
			    width: 320; height: 200; color: "#1A1A1A"; radius: 6; clip: true
			    Image {
				anchors.fill: parent; source: model.src; fillMode: Image.PreserveAspectCrop; asynchronous: true
				Rectangle { anchors.fill: parent; color: "#222"; visible: parent.status !== Image.Ready; BusyIndicator { anchors.centerIn: parent; running: parent.visible } }
				MouseArea {
				    anchors.fill: parent
				    onClicked: ScreenshotViewer.open()
				}
			    }
			}
		    }
		}

		// Статистика отзывов (Эмодзи-бар)
		Rectangle {
		    Layout.fillWidth: true; height: 38; color: "#19424242"; radius: 4; border.color: "#19626262"
		    RowLayout {
			anchors.centerIn: parent; spacing: 20
			Label { text: "💚 " + app_page.countSuper; color: "#888"; font.pixelSize: 12 }
			Label { text: "😁 " + app_page.countNorm; color: "#888"; font.pixelSize: 12 }
			Label { text: "😐 " + app_page.countNeutral; color: "#888"; font.pixelSize: 12 }
			Label { text: "👎🏻 " + app_page.countTrash; color: "#888"; font.pixelSize: 12 }
			Label { text: "👾 " + app_page.countParasha; color: "#888"; font.pixelSize: 12 }
		    }
		}

		// Платформы
		ColumnLayout {
		    spacing: 6
		    Label { text: qsTr("Platform Support:"); color: "#FFF"; font.bold: true; font.pixelSize: 13 }
		    RowLayout {
			spacing: 8
			Rectangle { height: 28; width: 85; color: "#141414"; radius: 4; border.color: "#262626"; Label { text: "armhf"; color: (app_page.appArchitectures === "all" || app_page.appArchitectures.indexOf("armhf") !== -1) ? "#00FFCC" : "#444444"; anchors.centerIn: parent; font.bold: true; font.pixelSize: 11 } }
			Rectangle { height: 28; width: 85; color: "#141414"; radius: 4; border.color: "#262626"; Label { text: "arm64"; color: (app_page.appArchitectures === "all" || app_page.appArchitectures.indexOf("arm64") !== -1) ? "#00FFCC" : "#444444"; anchors.centerIn: parent; font.bold: true; font.pixelSize: 11 } }
			Rectangle { height: 28; width: 85; color: "#141414"; radius: 4; border.color: "#262626"; Label { text: "x86_64"; color: (app_page.appArchitectures === "all" || app_page.appArchitectures.indexOf("x86_64") !== -1) ? "#00FFCC" : "#444444"; anchors.centerIn: parent; font.bold: true; font.pixelSize: 11 } }
		    }
		}

		// Главная кнопка Установки
		Rectangle {
		    id: installButton; Layout.fillWidth: true; height: 50; color: "#222222"; radius: height / 3; enabled: false
		    Label { id: installButtonText; text: qsTr("Checking Release..."); color: (text === qsTr("INSTALL") || text === qsTr("OPEN") || text === qsTr("UPDATE AVAILABLE")) ? "#000000" : "#FFFFFF"; font.bold: true; font.pixelSize: 14; anchors.centerIn: parent }
		    Label {
			id: updateNoticeLabel; visible: appInstaller.currentStatus === "installed" || installButtonText.text === qsTr("OPEN"); horizontalAlignment: Text.AlignHCenter; anchors.top: parent.bottom; anchors.topMargin: 4
			text: "!" + qsTr("JWB-Denda ha actualitzat / JWB-Store is updated") + "!"; color: "#00FFCC"; font.pixelSize: 11; font.bold: true
		    }
		    MouseArea {
			anchors.fill: parent
			onClicked: {
			    if (installButtonText.text === qsTr("OPEN")) {
				console.log(">>> [JWB-UI] Launching click package...")
			    } else if (installButtonText.text === qsTr("INSTALL") || installButtonText.text === qsTr("RETRY INSTALL") || installButtonText.text === qsTr("UPDATE AVAILABLE")) {
				appInstaller.installApplication(app_page.appUrl, app_page.appName)
			    }
			}
		    }
		}

		// Маленький незаметный триггер для вызова Popup, если была ошибка установки
		Text {
		    text: qsTr("Manual installation options available")
		    color: "#FFA500"; font.pixelSize: 11; font.underline: true; Layout.alignment: Qt.AlignHCenter
		    visible: appInstaller.currentStatus === "error"
		    MouseArea { anchors.fill: parent; onClicked: manualInstallPopup.open() }
		}

		// Описание приложения
		ColumnLayout {
		    Layout.fillWidth: true; spacing: 6
		    Label { text: qsTr("Description"); color: "#FFF"; font.bold: true; font.pixelSize: 14 }
		    Label { text: app_page.appDescription !== "" ? app_page.appDescription : qsTr("No description provided."); color: "#AAA"; Layout.fillWidth: true; wrapMode: Text.Wrap; font.pixelSize: 13 }
		}

		// Чейнджлог
		ColumnLayout {
		    Layout.fillWidth: true; spacing: 6; visible: changelogModel.count > 0
		    Label { text: qsTr("Changelog / History"); color: "#FFF"; font.bold: true; font.pixelSize: 14 }
		    ColumnLayout {
			Layout.fillWidth: true; spacing: 8
			Repeater {
			    model: changelogModel
			    delegate: Rectangle {
				Layout.fillWidth: true; implicitHeight: changelogColumn.implicitHeight + 16
				Behavior on implicitHeight { NumberAnimation { duration: 150; easing.type: Easing.InOutQuad } }
				color: "#19000000"; radius: 6; border.color: "#19262626"; clip: true
				property bool isExpanded: index === 0

				ColumnLayout {
				    id: changelogColumn; anchors.fill: parent; anchors.margins: 10; spacing: 5
				    RowLayout {
					Layout.fillWidth: true
					Label { text: model.version; color: "#00FFCC"; font.bold: true; font.pixelSize: 13 }
					Label { text: model.date; color: "#666"; font.pixelSize: 11; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
					Label { text: isExpanded ? "▲" : "▼"; color: "#444"; font.pixelSize: 10 }
				    }
				    Label { text: model.note; color: "#BBB"; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.Wrap; visible: parent.parent.isExpanded }
				}
				MouseArea { anchors.fill: parent; onClicked: parent.isExpanded = !parent.isExpanded }
			    }
			}
		    }
		}

		// Декоративная линия перед разделом отзывов
		Rectangle { Layout.fillWidth: true; height: 1; color: "#262626"; Layout.topMargin: 10; Layout.bottomMargin: 5 }

		// Шапка отзывов и кнопка удаления
		RowLayout {
		    Layout.fillWidth: true
		    Label { text: qsTr("User Reviews (" + reviewsModel.count + ")"); color: "#FFF"; font.bold: true; font.pixelSize: 14; Layout.fillWidth: true }
		    Text {
			text: qsTr("Delete My Review"); color: deleteMouseArea.containsMouse ? "#FF3366" : "#666"
			Behavior on color { ColorAnimation { duration: 100; easing.type: Easing.InOutQuad } }
			font.pixelSize: 12; font.underline: true; visible: app_page.hasMyReview; verticalAlignment: Text.AlignVCenter
			MouseArea {
			    id: deleteMouseArea; anchors.fill: parent; hoverEnabled: true
			    onClicked: {
				console.log(">>> [JWB-UI] Requesting deletion for user:", app_page.currentUsername)
				reviewSystem.deleteReview(app_page.currentUsername)
			    }
			}
		    }
		}

		// ЛИСТОВКА ВСЕХ СУЩЕСТВУЮЩИХ ОТЗЫВОВ
		ListView {
		    id: reviewsListView; Layout.fillWidth: true; implicitHeight: contentHeight; model: reviewsModel; interactive: false; spacing: 15
		    delegate: Item {
			width: reviewsListView.width; height: Math.max(50, reviewTextLabel.implicitHeight + authorLayout.implicitHeight + 15)
			RowLayout {
			    anchors.fill: parent; spacing: 15; Layout.alignment: Qt.AlignTop
			    Text { text: model.emoji; font.pixelSize: 28; Layout.alignment: Qt.AlignTop; Layout.topMargin: 2 }
			    ColumnLayout {
				Layout.fillWidth: true; spacing: 3
				RowLayout {
				    id: authorLayout; Layout.fillWidth: true
				    Label {
					text: model.author === app_page.currentUsername ? model.author + " (You)" : model.author
					font.bold: true; font.pixelSize: 14; color: model.author === app_page.currentUsername ? "#00FFCC" : "#FFF"
				    }
				}
				Label { id: reviewTextLabel; text: model.body; font.pixelSize: 13; color: "#BBB"; wrapMode: Text.Wrap; Layout.fillWidth: true }
			    }
			}
		    }
		}
	    }
	}

	// ==========================================
	// БЛОК ОСТАВЛЕНИЯ ОТЗЫВА (НАМЕРТВО ВНИЗУ ПОД СКРОЛЛОМ)
	// ==========================================
	Rectangle {
	    id: reviewInputBox
	    Layout.fillWidth: true
	    implicitHeight: reviewInputColumn.implicitHeight + 20
	    color: "#4c000000"
	    border.color: "#19262626"
	    border.width: 1
	    //opacity: 0.3

	    ColumnLayout {
		id: reviewInputColumn
		anchors.fill: parent
		anchors.margins: 10
		spacing: 8

		RowLayout {
		    Layout.fillWidth: true
		    Label {
			text: app_page.hasMyReview ? qsTr("Edit your review:") : qsTr("Leave a review:")
			color: "#00FFCC"; font.bold: true; font.pixelSize: 12
			Layout.fillWidth: true
		    }
		}

		// Эмодзи-селектор
		RowLayout {
		    id: emojiSelector
		    spacing: 12
		    property string selectedEmoji: "💚"
		    property string selectedRatingStr: "THUMBS_UP"
		    function select(ratingStr, emoji) { selectedRatingStr = ratingStr; selectedEmoji = emoji }

		    Repeater {
			model: [
			    { rStr: "THUMBS_UP", e: "💚" },
			    { rStr: "HAPPY", e: "😁" },
			    { rStr: "NEUTRAL", e: "😐" },
			    { rStr: "BUGGY", e: "👎🏻" },
			    { rStr: "THUMBS_DOWN", e: "👾" }
			]
			delegate: Rectangle {
			    width: 32; height: 32
			    color: emojiSelector.selectedRatingStr === modelData.rStr ? "#262626" : "transparent"
			    radius: 6
			    border.color: emojiSelector.selectedRatingStr === modelData.rStr ? "#00FFCC" : "transparent"
			    Text { text: modelData.e; font.pixelSize: 20; anchors.centerIn: parent }
			    MouseArea { anchors.fill: parent; onClicked: emojiSelector.select(modelData.rStr, modelData.e) }
			}
		    }
		}

		// Инпут и кнопка отправки в один ряд для экономии высоты
		RowLayout {
		    Layout.fillWidth: true
		    spacing: 8

		    TextField {
			id: reviewTextField
			Layout.fillWidth: true
			placeholderText: qsTr("Write your honest review here...")
			color: "#FFF"; font.pixelSize: 13
			background: Rectangle { color: "#101010"; radius: 4; border.color: "#222" }
			activeFocusOnPress: true
			selectByMouse: true
			mouseSelectionMode: TextEdit.SelectWords
		    }

		    Button {
			id: sendReviewBtn
			text: app_page.hasMyReview ? "✓" : "➔"
			enabled: reviewTextField.text.trim() !== ""
			background: Rectangle { implicitWidth: 42; implicitHeight: 32; color: sendReviewBtn.enabled ? "#00FFCC" : "#222"; radius: 4 }
			contentItem: Text { text: sendReviewBtn.text; font.bold: true; color: "#000"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
			onClicked: {
			    reviewsApiSystem.sendReview(
				"Bravada-N-A-A-R-und-JWB-Tutant-xamon", app_page.repoName, app_page.currentUsername,
				emojiSelector.selectedRatingStr, reviewTextField.text, app_page.currentOsType, app_page.currentDevice
			    )
			    reviewSystem.addLocalReview(app_page.repoName, app_page.currentUsername, emojiSelector.selectedRatingStr, reviewTextField.text)
			    reviewTextField.text = ""
			    reviewTextField.focus = false // Снимаем фокус, чтобы клава пряталась
			}
		    }
		}
	    }
	}
    }

    // ==========================================
    // POPUP ДЛЯ МАНУАЛЬНОЙ УСТАНОВКИ (ЧИСТЫЙ QT QUICK)
    // ==========================================
    Popup {
	id: manualInstallPopup
	anchors.centerIn: parent
	width: parent.width * 0.85
	modal: true
	focus: true
	closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

	background: Rectangle {
	    color: "#161616"
	    radius: 10
	    border.color: "#333333"
	    border.width: 1
	}

	contentItem: ColumnLayout {
	    spacing: 14
	    anchors.margins: 5

	    Label {
		text: qsTr("System Blocked Installation")
		color: "#FFA500"
		font.bold: true
		font.pixelSize: 15
		Layout.alignment: Qt.AlignHCenter
	    }

	    Label {
		text: qsTr("Ubuntu Touch system frameworks sometimes block direct auto-installation due to AppArmor policies or internal storage lockouts. Don't worry, your package is safely cached locally.")
		color: "#CCCCCC"
		font.pixelSize: 12
		wrapMode: Text.Wrap
		Layout.fillWidth: true
	    }

	    Rectangle {
		Layout.fillWidth: true
		height: 1
		color: "#222"
	    }

	    Label {
		text: qsTr("How to install manually:")
		color: "#00FFCC"
		font.bold: true
		font.pixelSize: 12
	    }

	    Label {
		text: qsTr("1. Click the green button below to open File Manager.\n2. Locate the downloaded .click package.\n3. Tap on it and select 'Install' inside the native system OS installer.")
		color: "#BBB"
		font.pixelSize: 12
		wrapMode: Text.Wrap
		Layout.fillWidth: true
	    }

	    // КНОПКА ВНУТРИ ПОПАПА
	    Button {
		id: popupManualBtn
		Layout.fillWidth: true
		Layout.topMargin: 5

		background: Rectangle {
		    implicitHeight: 40
		    color: "#00FFCC"
		    radius: 6
		}

		contentItem: Text {
		    text: qsTr("OPEN FILE MANAGER")
		    font.bold: true
		    color: "#000"
		    horizontalAlignment: Text.AlignHCenter
		    verticalAlignment: Text.AlignVCenter
		}

		onClicked: {
		    var cachePath = appInstaller.getLocalFilePath();
		    if (cachePath !== "") {
			console.log(">>> [JWB-UI] Opening File Manager at path: " + cachePath)
			Qt.openUrlExternally("file://" + cachePath);
		    }
		    manualInstallPopup.close() // Закрываем попап после нажатия
		}
	    }
	}
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:980;width:640}
}
##^##*/
