import qbs
import qbs.File

CppApplication {
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.network" }
    Depends { name: "Qt.qml" }
    Depends { name: "Qt.quick" }
    Depends { name: "Qt.concurrent" }
    Depends { name: "Qt.core" }
    Depends {
	name: "Qt.dbus"
	condition: qbs.targetOS.contains("linux")
    }
    // Additional import path used to resolve QML modules in Qt Creator's code model
    property pathList qmlImportPaths: []


    cpp.cxxLanguageVersion: "c++14"

    cpp.defines: [
	// The following define makes your compiler emit warnings if you use
	// any Qt feature that has been marked deprecated (the exact warnings
	// depend on your compiler). Please consult the documentation of the
	// deprecated API in order to know how to port your code away from it.
	"QT_DEPRECATED_WARNINGS",

	// You can also make your code fail to compile if it uses deprecated APIs.
	// In order to do so, uncomment the following line.
	// You can also select to disable deprecated APIs only up to a certain version of Qt.
	//"QT_DISABLE_DEPRECATED_BEFORE=0x060000" // disables all the APIs deprecated before Qt 6.0.0
    ]

    files: [
        "Plug-ins/appsreviewsapisystem.cpp",
        "Plug-ins/appsreviewsapisystem.h",
        "Plug-ins/categoriesmodel.cpp",
        "Plug-ins/categoriesmodel.h",
        "Plug-ins/searchmodel.cpp",
        "Plug-ins/searchmodel.h",
        "main.cpp",
        "qml.qrc",
        "Plug-ins/api.cpp",
        "Plug-ins/api.h",
        "Plug-ins/bypass.cpp",
        "Plug-ins/bypass.h",
        "Plug-ins/jwbstore.cpp",
        "Plug-ins/jwbstore.h",
    ]

    Group {
	    fileTagsFilter: "application"
	    qbs.install: true
	    qbs.installDir: "." // Для Ubuntu Touch бинарник обычно лежит в корне рядом с манифестом
	}

	// ВАЖНО: Подключаем метаданные Ubuntu Touch, чтобы они попадали в сборку
	Group {
	    name: "Ubuntu Touch Metadata"
	    files: [
		"manifest.json",
		"jwb-denda.desktop", // Проверь точное имя своего .desktop файла!
		"jwb-denda.apparmor"
	    ]
	    qbs.install: true
	    qbs.installDir: "." // Все метафайлы ДОЛЖНЫ лежать в корне клик-пакета
	}
	// ВАЖНО: Подключаем метаданные Ubuntu Touch, чтобы они попадали в сборку
	Group {
	    name: "Ubuntu Touch App Root"
	    files: ["*.qml", "*.js", "Modules/*.js", "Wizard/*", "Assets/**/**"]
	    qbs.install: true
	    qbs.installDir: "." // Все метафайлы ДОЛЖНЫ лежать в корне клик-пакета
	}
}
